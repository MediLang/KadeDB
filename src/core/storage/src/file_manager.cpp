#include "kadedb/storage/file_manager.h"
#include "kadedb/storage/file_manager_internal.h"
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>  // for fstat
#include <fcntl.h>
#include <system_error>
#include <crc32c/crc32c.h>
#include <algorithm>
#include <errno.h>
#include <cstring>  // for memcpy

// Constants
static constexpr size_t INITIAL_PAGES = 16;  // Initial number of pages to allocate

namespace kadedb {
namespace storage {

// Private implementation (PIMPL)
class FileHandle {
public:
    FileHandle() = default;
    ~FileHandle() { close(); }
    
    // Public accessor methods
    const FileManager::FileHeader* header() const { 
        return reinterpret_cast<const FileManager::FileHeader*>(mapped_data_); 
    }
    FileManager::FileHeader* header_mutable() { 
        return reinterpret_cast<FileManager::FileHeader*>(mapped_data_); 
    }
    int fd() const { return fd_; }
    void* mapped_data() const { return mapped_data_; }
    size_t file_size() const { return file_size_; }
    
    // Flush changes to disk
    std::error_code flush() {
        if (mapped_data_ != MAP_FAILED) {
            if (::msync(mapped_data_, file_size_, MS_SYNC) == -1) {
                return std::error_code(errno, std::system_category());
            }
        }
        return std::error_code{};
    }
    
    // Extend file by specified number of pages
    std::error_code extend_file(uint32_t num_pages) {
        if (fd_ == -1 || !mapped_data_) {
            return std::make_error_code(std::errc::bad_file_descriptor);
        }
        
        // Calculate new file size
        const size_t new_file_size = file_size_ + (num_pages * page_size_);
        
        // Extend the file
        if (::ftruncate(fd_, new_file_size) == -1) {
            return std::error_code(errno, std::system_category());
        }
        
        // Remap the file
        void* new_mapping = ::mremap(mapped_data_, file_size_, new_file_size, MREMAP_MAYMOVE);
        if (new_mapping == MAP_FAILED) {
            return std::error_code(errno, std::system_category());
        }
        
        mapped_data_ = new_mapping;
        file_size_ = new_file_size;
        page_count_ += num_pages;
        
        // Update the header
        auto* hdr = header_mutable();
        hdr->page_count = page_count_;
        
        return std::error_code{};
    }
    
    // Setters
    void set_mapped_data(void* data) { mapped_data_ = data; }
    void set_file_size(size_t size) { file_size_ = size; }
    void set_page_count(uint64_t count) { page_count_ = count; }

    std::error_code create(const std::string& filename, uint32_t page_size) {
        if (is_open()) {
            return std::make_error_code(std::errc::device_or_resource_busy);
        }
        
        if (page_size < sizeof(FileManager::FileHeader) + sizeof(FileManager::PageHeader)) {
            return std::make_error_code(std::errc::invalid_argument);
        }

        // Create and open the file
        int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            return std::error_code(errno, std::system_category());
        }

        // Initialize file header
        FileManager::FileHeader header{};
        std::memcpy(header.signature, FileManager::FILE_SIGNATURE, 6);
        header.version = FileManager::CURRENT_VERSION;
        header.page_size = page_size;
        header.page_count = 1;  // Just the header page initially
        header.free_page_list = 0;  // No free pages initially

        // Write the header
        if (::write(fd, &header, sizeof(FileManager::FileHeader)) != sizeof(FileManager::FileHeader)) {
            auto err = errno;
            ::close(fd);
            return std::error_code(err, std::system_category());
        }

        // Initialize the first page (header page)
        FileManager::PageHeader page_header{};
        std::vector<char> page(page_size, 0);
        std::memcpy(page.data(), &page_header, sizeof(FileManager::PageHeader));
        
        if (::write(fd, page.data() + sizeof(FileManager::FileHeader), 
                   page_size - sizeof(FileManager::FileHeader)) != 
            static_cast<ssize_t>(page_size - sizeof(FileManager::FileHeader))) {
            auto err = errno;
            ::close(fd);
            return std::error_code(err, std::system_category());
        }

        // Memory map the file
        return map_file(fd, filename, page_size);
    }

    std::error_code open(const std::string& filename) {
        if (is_open()) {
            return std::make_error_code(std::errc::device_or_resource_busy);
        }
        
        // Reset state
        fd_ = -1;
        mapped_data_ = MAP_FAILED;
        file_size_ = 0;
        page_count_ = 0;

        // Open the file
        int fd = ::open(filename.c_str(), O_RDWR);
        if (fd == -1) {
            return std::error_code(errno, std::system_category());
        }

        // Read and validate the header
        FileManager::FileHeader header;
        if (::read(fd, &header, sizeof(header)) != sizeof(header)) {
            auto err = errno;
            ::close(fd);
            return std::error_code(err, std::system_category());
        }

        // Verify signature and version
        if (std::memcmp(header.signature, FileManager::FILE_SIGNATURE, 6) != 0) {
            ::close(fd);
            return std::make_error_code(std::errc::invalid_argument);
        }

        if (header.version > FileManager::CURRENT_VERSION) {
            ::close(fd);
            return std::make_error_code(std::errc::invalid_argument);
        }

        // Memory map the file
        auto ec = map_file(fd, filename, header.page_size);
        if (ec) return ec;
        
        // Initialize free page list
        FileManager::FileHeader* hdr = header_mutable();
        if (hdr) {
            hdr->free_page_list = 0;  // No free pages initially
        }
        
        // Pre-allocate initial pages
        return extend_file(INITIAL_PAGES - 1);  // -1 because we already have the header page
    }

    void close() {
        if (mapped_data_ != MAP_FAILED) {
            ::msync(mapped_data_, file_size_, MS_SYNC);
            ::munmap(mapped_data_, file_size_);
            mapped_data_ = MAP_FAILED;
        }
        if (fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }
        file_size_ = 0;
        page_count_ = 0;
    }

    bool is_open() const { return fd_ != -1; }
    uint32_t page_size() const { return page_size_; }
    uint64_t page_count() const { return page_count_; }

    FileManager::Page* get_page(uint64_t page_id) {
        if (page_id >= page_count()) {
            return nullptr;
        }
        void* data = mapped_data();
        if (!data) {
            return nullptr;
        }
        size_t page_offset = sizeof(FileManager::FileHeader) + 
                       (page_id * (sizeof(FileManager::PageHeader) + page_size()));
        return reinterpret_cast<FileManager::Page*>(static_cast<char*>(data) + page_offset);
    }

    uint32_t calculate_checksum(const FileManager::Page* page) const {
        if (!page) return 0;
        
        // Calculate checksum of page data (excluding the checksum field itself)
        const uint8_t* data = reinterpret_cast<const uint8_t*>(page);
        size_t offset = offsetof(FileManager::Page, header.checksum) + sizeof(page->header.checksum);
        size_t length = page_size_ - offset;
        
        // Simple XOR-based checksum as fallback
        uint32_t checksum = 0;
        for (size_t i = 0; i < length; ++i) {
            checksum ^= static_cast<uint32_t>(data[offset + i]) << ((i % 4) * 8);
        }
        return checksum;
    }

private:
    std::error_code map_file(int fd, const std::string& filename, uint32_t page_size) {
        struct stat st;
        
        // Get file size
        if (::fstat(fd, &st) == -1) {
            auto err = errno;
            ::close(fd);
            return std::error_code(err, std::system_category());
        }

        // Memory map the file
        void* mapped = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mapped == MAP_FAILED) {
            auto err = errno;
            ::close(fd);
            return std::error_code(err, std::system_category());
        }

        // Update state
        fd_ = fd;
        mapped_data_ = mapped;
        file_size_ = st.st_size;
        page_size_ = page_size;
        page_count_ = (file_size_ - sizeof(FileManager::FileHeader)) / 
                     (sizeof(FileManager::PageHeader) + page_size);

        return std::error_code{};
    }

    int fd_ = -1;
    void* mapped_data_ = MAP_FAILED;
    size_t file_size_ = 0;
    uint32_t page_size_ = 0;
    uint64_t page_count_ = 0;
};

// FileManager implementation
FileManager::FileManager() : impl_(std::make_unique<FileHandle>()) {}
FileManager::~FileManager() = default;

std::error_code FileManager::create_file(const std::string& filename, uint32_t page_size) {
    return impl_->create(filename, page_size);
}

std::error_code FileManager::open_file(const std::string& filename) {
    return impl_->open(filename);
}

void FileManager::close_file() {
    impl_->close();
}

bool FileManager::is_open() const {
    return impl_->is_open();
}

FileManager::Page* FileManager::allocate_page(uint32_t page_type) {
    if (!is_open()) {
        return nullptr;
    }

    // Check free list first
    FileHeader* header = const_cast<FileHeader*>(impl_->header());
    if (header->free_page_list != 0) {
        // Reuse a free page
        Page* page = impl_->get_page(header->free_page_list);
        if (!page) return nullptr;
        
        header->free_page_list = page->header.next_free;
        page->header.page_type = page_type;
        page->header.checksum = 0;  // Will be set on write
        return page;
    } else {
        // Allocate a new page at the end of the file
        // Implementation for extending the file would go here
        return nullptr;  // Not implemented yet
    }
}

void FileManager::free_page(uint64_t page_id) {
    if (!is_open() || page_id == 0) return;  // Can't free header page
    
    FileHeader* header = const_cast<FileHeader*>(impl_->header());
    Page* page = impl_->get_page(page_id);
    if (!page) return;
    
    // Add to free list
    page->header.next_free = header->free_page_list;
    page->header.page_type = 0xFFFFFFFF;  // Mark as free
    header->free_page_list = page_id;
}

FileManager::Page* FileManager::read_page(uint64_t page_id) {
    if (!is_open()) return nullptr;
    return impl_->get_page(page_id);
}

std::error_code FileManager::write_page(uint64_t page_id) {
    if (!is_open()) {
        return std::make_error_code(std::errc::not_connected);
    }
    
    Page* page = impl_->get_page(page_id);
    if (!page) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    
    // Update checksum
    if (page) {
        page->header.checksum = impl_->calculate_checksum(page);
    }
    
    // The page is already in memory and will be written back by the OS
    // Force write to disk if needed
    return impl_->flush();
}

std::error_code FileManager::flush() {
    if (!is_open()) {
        return std::make_error_code(std::errc::not_connected);
    }
    
    if (::msync(impl_->mapped_data(), impl_->file_size(), MS_SYNC) == -1) {
        return std::error_code(errno, std::system_category());
    }
    return std::error_code{};
}

uint32_t FileManager::page_size() const {
    return impl_ ? impl_->page_size() : 0;
}

uint64_t FileManager::page_count() const {
    return impl_ ? impl_->page_count() : 0;
}

void FileManager::for_each_page(std::function<void(uint64_t, Page*, uint32_t)> callback) {
    if (!is_open()) return;
    
    const FileHeader* header = get_file_header();
    if (!header) return;
        
    (void)header;  // Mark as used to avoid unused variable warning
    const uint64_t total_pages = page_count();
    
    for (uint64_t i = 1; i < total_pages; ++i) {
        Page* page = impl_->get_page(i);
        if (page && page->header.page_type != 0xFFFFFFFF) {  // Skip free pages
            callback(i, page, page->header.page_type);
        }
    }
}

std::error_code FileManager::extend_file(size_t num_pages) {
    if (!is_open()) {
        return std::make_error_code(std::errc::not_connected);
    }
    
    if (num_pages == 0) {
        return std::error_code{};  // Nothing to do
    }
    
    // Get the current file header
    FileHeader* header = get_file_header_mutable();
    if (!header) {
        return std::make_error_code(std::errc::io_error);
    }
    
    const uint32_t page_size = header->page_size;
    const size_t page_data_size = sizeof(PageHeader) + page_size;
    const uint64_t first_new_page = header->page_count;
    
    // Calculate new file size
    const off_t current_size = lseek(impl_->fd(), 0, SEEK_END);
    const off_t new_size = current_size + (num_pages * page_data_size);
    
    // Extend the file
    if (ftruncate(impl_->fd(), new_size) != 0) {
        return std::error_code(errno, std::system_category());
    }
    
    // Remap the file if needed
    if (static_cast<size_t>(new_size) > impl_->file_size()) {
        void* new_mapping = mremap(impl_->mapped_data(), impl_->file_size(), 
                                 new_size, MREMAP_MAYMOVE);
        if (new_mapping == MAP_FAILED) {
            return std::error_code(errno, std::system_category());
        }
        
        // Update internal state
        impl_->set_mapped_data(new_mapping);
        impl_->set_file_size(new_size);
        impl_->set_page_count((new_size - sizeof(FileHeader)) / page_data_size);
    }
    
    // Initialize new pages and add to free list
    for (size_t i = 0; i < num_pages; ++i) {
        uint64_t page_id = first_new_page + i;
        Page* page = impl_->get_page(page_id);
        if (page) {
            // Initialize page header
            page->header.page_type = 0xFFFFFFFF;  // Mark as free
            page->header.next_free = header->free_page_list;
            
            // Add to free list
            header->free_page_list = page_id;
        }
    }
    
    // Update page count in header
    if (header) {
        header->page_count += num_pages;
    }
    
    return std::error_code{};
}

std::error_code FileManager::validate_header() const {
    const FileHeader* header = get_file_header();
    
    // Check signature
    if (std::memcmp(header->signature, FILE_SIGNATURE, 6) != 0) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    
    // Check version
    if (header->version > CURRENT_VERSION) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    
    // Check page size is reasonable
    if (header->page_size < 512 || header->page_size > 65536) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    
    return std::error_code{};
}

const FileManager::FileHeader* FileManager::get_file_header() const {
    if (!is_open()) return nullptr;
    if (!impl_) return nullptr;
    return impl_->header();
}

FileManager::FileHeader* FileManager::get_file_header_mutable() {
    if (!is_open()) return nullptr;
    if (!impl_) return nullptr;
    return impl_->header_mutable();
}

} // namespace storage
} // namespace kadedb

#include "kadedb/storage/page/page_manager.h"
#include "kadedb/storage/page/page.h"
#include "kadedb/storage/page/page_header.h"
#include "kadedb/storage/file_manager.h"
#include "kadedb/storage/file_manager_internal.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <system_error>
#include <unistd.h>
#include <sys/mman.h>
#include <unordered_set>
#include <vector>
#include <utility>
#include <cstdint>
#include <cstddef>
#include <iterator>

// For PageId::value_type
#include "kadedb/storage/page/page_id.h"

namespace kadedb {
namespace storage {

// PageCacheEntry is defined in the header file

PageManager::PageManager(std::shared_ptr<FileManager> file_manager, size_t cache_size)
    : file_manager_(std::move(file_manager)),
      cache_size_(cache_size),
      page_size_(0) {  // Will be initialized from file manager
    if (!file_manager_) {
        throw std::invalid_argument("File manager cannot be null");
    }
    if (cache_size == 0) {
        throw std::invalid_argument("Cache size must be greater than 0");
    }
    
    // Get the page size from the file manager
    page_size_ = file_manager_->page_size();
    if (page_size_ == 0) {
        throw std::runtime_error("Invalid page size from file manager");
    }
}

std::shared_ptr<Page> PageManager::fetch_page(PageId page_id) {
    if (!page_id.is_valid()) {
        throw std::invalid_argument("Invalid page ID");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if page is in cache
    auto it = page_cache_.find(page_id);
    if (it != page_cache_.end()) {
        // Move to front of LRU list
        lru_list_.remove(page_id);
        lru_list_.push_front(page_id);
        
        // Return a copy of the cached page
        if (it->second.page) {
            return it->second.page;
        }
        // If page is null in cache (shouldn't happen), fall through to disk read
    }
    
    // Read the page data from disk
    std::vector<Page::Byte> page_data(page_size_);
    std::fill(page_data.begin(), page_data.end(), std::byte{0});
    try {
        read_page_from_disk(page_id, page_data.data());
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to read page " + std::to_string(page_id.value()) + 
                              " from disk: " + e.what());
    }
    
    // Create a new page
    std::shared_ptr<Page> page;
    try {
        page = std::make_shared<Page>(page_id, page_data.data(), page_size_);
        if (!page) {
            throw std::runtime_error("Failed to allocate memory for page");
        }
        
        // Add to cache
        add_to_cache(page);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to create page object for page " + 
                              std::to_string(page_id.value()) + 
                              ": " + e.what());
    }
    
    return page;
}

std::shared_ptr<Page> PageManager::new_page(PageType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Allocate a new page ID
    PageId page_id = allocate_new_page();
    
    // Create a new page with the specified type
    auto page = std::make_shared<Page>(page_id, page_size_);
    
    // Initialize the page header
    auto* hdr = page->mutable_header();
    if (hdr) {
        hdr->initialize(type, page_size_);
        hdr->set_dirty(true);
    }
    
    // Add to cache
    add_to_cache(page);
    
    return page;
}

void PageManager::mark_dirty(const std::shared_ptr<Page>& page) {
    if (!page) {
        return;
    }
    
    PageId page_id = page->page_id();
    if (!page_id.is_valid()) {
        return;  // Skip invalid page IDs
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find the page in cache
    auto it = page_cache_.find(page_id);
    if (it != page_cache_.end()) {
        // Update the dirty flag
        it->second.is_dirty = true;
        
        // Update the page's header
        if (it->second.page) {
            auto* hdr = it->second.page->mutable_header();
            if (hdr) {
                hdr->set_dirty(true);
            }
        }
        
        // Move to front of LRU list
        lru_list_.remove(page_id);
        lru_list_.push_front(page_id);
    } else {
        try {
            // If not in cache, add it with dirty flag set
            auto result = page_cache_.emplace(page_id, PageCacheEntry{page});
            if (result.second) {  // Insertion successful
                result.first->second.is_dirty = true;
                
                // Update the page's header
                if (result.first->second.page) {
                    auto* hdr = result.first->second.page->mutable_header();
                    if (hdr) {
                        hdr->set_dirty(true);
                    }
                }
                
                lru_list_.push_front(page_id);
                
                // If cache is full, evict a page
                if (page_cache_.size() > cache_size_) {
                    evict_page();
                }
            }
        } catch (const std::exception& e) {
            // Log error but don't throw from mark_dirty
            (void)e;
        }
    }
}

void PageManager::write_page(const std::shared_ptr<Page>& page, bool force) {
    if (!page) {
        throw std::invalid_argument("Page cannot be null");
    }
    
    PageId page_id = page->page_id();
    if (!page_id.is_valid()) {
        throw std::invalid_argument("Invalid page ID");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find the page in cache
    auto it = page_cache_.find(page_id);
    const void* page_data = page->data();
    
    if (it == page_cache_.end()) {
        // If page is not in cache but force is true, write it directly
        if (force) {
            try {
                write_page_to_disk(page_id, page_data);
                // Add to cache without marking as dirty
                add_to_cache(page);
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to write page " + std::to_string(page_id.value()) + 
                                      ": " + e.what());
            }
        } else {
            throw std::runtime_error("Page " + std::to_string(page_id.value()) + 
                                  " not found in cache and force=false");
        }
        return;
    }
    
    // If the page is dirty or force is true, write it to disk
    if (it->second.is_dirty || force) {
        try {
            // Use the page data from the cache if available, otherwise use the provided page
            const void* data_to_write = it->second.page ? it->second.page->data() : page_data;
            write_page_to_disk(page_id, data_to_write);
            
            // Update the cache entry
            it->second.is_dirty = false;
            
            // Update the page's header if it's still in memory
            if (it->second.page) {
                auto* hdr = it->second.page->mutable_header();
                if (hdr) {
                    hdr->set_dirty(false);
                }
            }
            
            // Update the LRU list
            lru_list_.remove(page_id);
            lru_list_.push_front(page_id);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to write page " + std::to_string(page_id.value()) + 
                                  ": " + e.what());
        }
    }
}

void PageManager::flush_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create a copy of the dirty pages to avoid holding the lock during I/O
    std::vector<std::pair<PageId, std::shared_ptr<Page>>> dirty_pages;
    for (const auto& [page_id, entry] : page_cache_) {
        if (entry.is_dirty) {
            dirty_pages.emplace_back(page_id, entry.page);
        }
    }
    
    // Write all dirty pages to disk
    for (const auto& [page_id, page] : dirty_pages) {
        try {
            // Write the page data to disk
            write_page_to_disk(page_id, page->data());
            
            // Update the cache entry if it still exists
            auto it = page_cache_.find(page_id);
            if (it != page_cache_.end()) {
                it->second.is_dirty = false;
                
                // Update the page's dirty flag if the page is still in memory
                if (it->second.page) {
                    auto* hdr = it->second.page->mutable_header();
                    if (hdr) {
                        hdr->set_dirty(false);
                    }
                }
            }
        } catch (const std::exception& e) {
            // Log error but continue with other pages
            (void)e;
        }
    }
    
    // Flush file system buffers
    if (file_manager_) {
        try {
            std::error_code ec = file_manager_->flush();
            if (ec) {
                throw std::runtime_error("Failed to flush file system buffers: " + ec.message());
            }
        } catch (const std::exception& e) {
            // Log error but don't throw from flush_all
            (void)e;
        }
    }
}

void PageManager::free_page(PageId page_id) {
    if (!page_id.is_valid()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the page is in cache
    auto cache_it = page_cache_.find(page_id);
    if (cache_it != page_cache_.end()) {
        // If the page is dirty, write it back to disk
        if (cache_it->second.is_dirty) {
            try {
                write_page_to_disk(page_id, cache_it->second.page->data());
            } catch (const std::exception& e) {
                // Log error but continue with freeing the page
                (void)e;
            }
        }
        
        // Remove from cache
        lru_list_.remove(page_id);
        page_cache_.erase(cache_it);
    }
    
    // Add to free list for reuse
    free_list_.push_back(page_id);
    
    // Notify the file manager to free the page
    if (file_manager_) {
        try {
            file_manager_->free_page(page_id.value());
        } catch (const std::exception& e) {
            // Log error but continue
            (void)e;
        }
    }
}

uint64_t PageManager::page_count() const {
    return file_manager_ ? file_manager_->page_count() : 0;
}

bool PageManager::evict_page() {
    if (lru_list_.empty()) {
        return false;  // No pages to evict
    }
    
    // Find the first unpinned page from the back of the LRU list
    for (auto it = lru_list_.rbegin(); it != lru_list_.rend(); ++it) {
        PageId page_id = *it;
        auto cache_it = page_cache_.find(page_id);
        
        // Skip if not found in cache (shouldn't happen)
        if (cache_it == page_cache_.end()) {
            lru_list_.erase(std::next(it).base());
            return true;  // Removed invalid entry, try again
        }
        
        // Skip pinned pages
        if (cache_it->second.page->is_pinned()) {
            continue;
        }
        
        // Found an unpinned page, write it back if dirty
        if (cache_it->second.is_dirty) {
            try {
                write_page_to_disk(page_id, cache_it->second.page->data());
                
                // Update the page's dirty flag
                auto* hdr = cache_it->second.page->mutable_header();
                if (hdr) {
                    hdr->set_dirty(false);
                }
            } catch (const std::exception&) {
                // Skip this page if we can't write it back
                continue;
            }
        }
        
        // Remove from cache and LRU list
        lru_list_.erase(std::next(it).base());
        page_cache_.erase(cache_it);
        return true;
    }

    return false;
}

void PageManager::add_to_cache(std::shared_ptr<Page> page) {
    if (!page) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    PageId page_id = page->page_id();
    
    // Check if page is already in cache
    auto it = page_cache_.find(page_id);
    if (it != page_cache_.end()) {
        // Update existing entry
        it->second.page = std::move(page);
        it->second.is_dirty = false;
        
        // Move to front of LRU list
        lru_list_.remove(page_id);
        lru_list_.push_front(page_id);
        return;
    }
    
    // If cache is full, evict pages until we have space
    while (!lru_list_.empty() && page_cache_.size() >= cache_size_) {
        if (!evict_page()) {
            throw std::runtime_error("Failed to evict page from cache");
        }
    }
    
    // Add the page to the cache
    auto result = page_cache_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(page->page_id()),
        std::forward_as_tuple(page)
    );
    
    if (!result.second) {
        throw std::runtime_error("Failed to insert page into cache");
    }
    
    // Add to LRU list and pin the page
    lru_list_.push_front(page->page_id());
    page->pin();
    
    // Mark as dirty if needed
    if (page->is_dirty()) {
        result.first->second.is_dirty = true;
    }
}

void PageManager::read_page_from_disk(PageId page_id, void* buffer) {
    if (!page_id.is_valid()) {
        throw std::invalid_argument("Invalid page ID");
    }
    
    if (!buffer) {
        throw std::invalid_argument("Buffer cannot be null");
    }
    
    if (!file_manager_) {
        throw std::runtime_error("FileManager not available");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Read the page from disk
    FileManager::Page* page = file_manager_->read_page(page_id.value());
    if (!page) {
        throw std::runtime_error("Failed to read page " + std::to_string(page_id.value()) + " from disk");
    }
    
    try {
        // Copy the page data to the buffer
        std::memcpy(buffer, page->data, page_size_);
        
        // Verify the page header
        auto* header = reinterpret_cast<PageHeader*>(buffer);
        if (!header->is_valid()) {
            throw std::runtime_error("Invalid page header for page " + std::to_string(page_id.value()));
        }
        
        // Update the page number in the header to match the page ID
        if (header->page_num != page_id.value()) {
            header->page_num = page_id.value();
        }
    } catch (...) {
        // Ensure we don't leak the page if an exception occurs
        delete page;
        throw;
    }
    
    // Free the page memory
    delete page;
}

void PageManager::write_page_to_disk(PageId page_id, const void* data) {
    if (!page_id.is_valid()) {
        throw std::invalid_argument("Invalid page ID");
    }
    
    if (!data) {
        throw std::invalid_argument("Data cannot be null");
    }
    
    if (!file_manager_) {
        throw std::runtime_error("FileManager not available");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Verify the page header before writing
    const auto* header = reinterpret_cast<const PageHeader*>(data);
    if (!header->is_valid()) {
        throw std::runtime_error("Invalid page header for page " + std::to_string(page_id.value()));
    }
    
    // Create a new page with the file manager
    FileManager::Page* page = new (std::nothrow) FileManager::Page;
    if (!page) {
        throw std::runtime_error("Failed to allocate memory for page");
    }
    
    try {
        // Initialize the page header
        page->header.page_type = static_cast<uint32_t>(header->type);
        page->header.checksum = header->checksum;
        
        // Copy the data to the page
        std::memcpy(page->data, data, page_size_);
        
        // Write the page to disk
        std::error_code ec = file_manager_->write_page(page_id.value());
        if (ec) {
            throw std::runtime_error("Failed to write page to disk: " + ec.message());
        }
        
        // Flush to ensure the data is written to disk
        ec = file_manager_->flush();
        if (ec) {
            throw std::runtime_error("Failed to flush page to disk: " + ec.message());
        }
    } catch (...) {
        // Ensure we don't leak the page if an exception occurs
        delete page;
        throw;
    }
    
    // Free the page memory
    delete page;
}

PageId PageManager::allocate_new_page() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // First, try to reuse a page from the free list
    if (!free_list_.empty()) {
        PageId page_id = free_list_.front();
        free_list_.pop_front();
        
        // Verify the page ID is valid
        if (!page_id.is_valid()) {
            free_list_.push_back(page_id);  // Put it back in a different position
            throw std::runtime_error("Invalid page ID in free list");
        }
        
        // Initialize the page on disk with zeros and a valid header
        std::vector<uint8_t> zero_page(page_size_, 0);
        PageHeader* header = reinterpret_cast<PageHeader*>(zero_page.data());
        header->initialize(PageType::FREE, page_size_);
        header->page_num = page_id.value();
        
        try {
            write_page_to_disk(page_id, zero_page.data());
            return page_id;
        } catch (const std::exception& e) {
            // If we can't initialize the page, put it back in the free list
            free_list_.push_back(page_id);
            throw std::runtime_error("Failed to initialize reused page: " + std::string(e.what()));
        }
    }
    
    // No free pages available, allocate a new one
    if (!file_manager_) {
        throw std::runtime_error("FileManager not available");
    }
    
    // Allocate a new page in the file
    FileManager::Page* new_page = nullptr;
    try {
        new_page = file_manager_->allocate_page(static_cast<uint32_t>(PageType::FREE));
        if (!new_page) {
            throw std::runtime_error("File manager returned null page");
        }
        
        // Get the page ID from the file manager
        uint64_t page_num = 0;  // This should be set by the file manager
        
        // Create a new page ID
        PageId new_page_id(page_num);
        
        // Initialize the page header
        PageHeader* header = reinterpret_cast<PageHeader*>(new_page->data);
        header->initialize(PageType::FREE, page_size_);
        header->page_num = new_page_id.value();
        
        // Write the page to disk
        std::error_code ec = file_manager_->write_page(page_num);
        if (ec) {
            throw std::runtime_error("Failed to write new page: " + ec.message());
        }
        
        // Flush to ensure the data is written to disk
        ec = file_manager_->flush();
        if (ec) {
            throw std::runtime_error("Failed to flush new page: " + ec.message());
        }
        
        // Free the page memory
        delete new_page;
        
        return new_page_id;
    } catch (const std::exception& e) {
        // Clean up if an error occurs
        if (new_page) {
            delete new_page;
        }
        throw std::runtime_error("Failed to allocate new page: " + std::string(e.what()));
    }
}

} // namespace storage
} // namespace kadedb

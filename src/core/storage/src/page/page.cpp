#include "kadedb/storage/page/page.h"
#include "kadedb/storage/crc32c.h"
#include <cstring>
#include <stdexcept>
#include <utility>
#include <cassert>

namespace kadedb {
namespace storage {

Page::Page(PageId page_id, Byte* data, uint32_t page_size)
    : page_id_(page_id), 
      data_(data, data + page_size) {
    // Initialize the page header if it's a new page
    if (page_id_.is_valid() && header()) {
        auto* hdr = mutable_header();
        hdr->initialize(PageType::DATA, page_size);
        hdr->set_dirty(true);
    }
}

Page::Page(PageId page_id, uint32_t page_size)
    : page_id_(page_id),
      data_(page_size) {
    // Initialize the page header if it's a new page
    if (page_id_.is_valid() && header()) {
        auto* hdr = mutable_header();
        hdr->initialize(PageType::DATA, page_size);
        hdr->set_dirty(true);
    }
}

Page::Data Page::allocate(size_t size) {
    if (!has_space(size)) {
        throw std::runtime_error("Not enough space in page");
    }
    
    auto* hdr = mutable_header();
    if (!hdr) {
        throw std::runtime_error("Invalid page header");
    }
    
    Byte* ptr = data_.data() + hdr->free_offset;
    
    // Update free space information
    hdr->free_offset += static_cast<uint16_t>(size);
    hdr->free_space -= static_cast<uint16_t>(size);
    hdr->set_dirty(true);
    
    return {ptr, size};
}

void Page::free(size_t offset, size_t size) {
    // In a real implementation, we would track freed blocks and potentially
    // merge adjacent free blocks. For now, we just mark the page as dirty.
    (void)offset; // Unused parameter
    (void)size;   // Unused parameter
    
    auto* hdr = mutable_header();
    if (hdr) {
        hdr->set_dirty(true);
    }
}

void Page::update_checksum() {
    auto* hdr = mutable_header();
    if (!hdr) return;
    
    // Reset checksum before calculation
    hdr->checksum = 0;
    
    // Calculate checksum over header and data
    uint32_t crc = 0;
    const auto* header_data = reinterpret_cast<const char*>(hdr);
    crc = CRC32C::Extend(crc, 
        reinterpret_cast<const std::byte*>(header_data), 
        sizeof(PageHeader));
    
    // Only checksum the user data (after the header)
    if (data_.size() > sizeof(PageHeader)) {
        crc = CRC32C::Extend(crc, 
            data_.data() + sizeof(PageHeader), 
            data_.size() - sizeof(PageHeader));
    }
    
    // Store the final checksum and mark as dirty
    hdr->checksum = crc;
    hdr->set_dirty(true);
}

bool Page::verify_checksum() const {
    const auto* hdr = header();
    if (!hdr || hdr->checksum == 0) {
        return true;  // No checksum to verify
    }
    
    // Calculate the expected checksum
    uint32_t calculated = 0;
    PageHeader temp_header = *hdr;
    temp_header.checksum = 0;
    
    const auto* header_data = reinterpret_cast<const char*>(&temp_header);
    calculated = CRC32C::Extend(calculated, 
        reinterpret_cast<const std::byte*>(header_data), 
        sizeof(PageHeader));
    
    if (data_.size() > sizeof(PageHeader)) {
        calculated = CRC32C::Extend(
            calculated,
            data_.data() + sizeof(PageHeader),
            data_.size() - sizeof(PageHeader)
        );
    }
    
    return calculated == hdr->checksum;
}

} // namespace storage
} // namespace kadedb

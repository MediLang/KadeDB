#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <kadedb/storage/page/page_id.h>
#include <kadedb/storage/page/page_type.h>

namespace kadedb {
namespace storage {

// Forward declarations
class Page;

// Default page size (can be overridden per database)
static constexpr uint32_t PAGE_SIZE = 4096;  // 4KB pages by default

// Page header structure - must be exactly 32 bytes
#pragma pack(push, 1)
struct PageHeader {
    // Common fields (8 bytes)
    uint32_t checksum;      // CRC32C checksum of the page (excluding this field)
    uint16_t page_size;     // Size of the page
    PageType type;          // Type of the page
    uint8_t  flags;         // Flags (dirty, overflow, etc.)
    
    // Page ID (8 bytes)
    uint64_t page_num;      // Page number in the file
    
    // Free space management (4 bytes)
    uint16_t free_space;    // Number of free bytes in the page
    uint16_t free_offset;   // Offset to the first free byte
    
    // Overflow page management (8 bytes)
    uint64_t next_overflow; // Next overflow page ID (0 if none)
    
    // Owner page (for overflow pages) (4 bytes)
    uint32_t owner_page;    // Owning page ID (for overflow pages)
    
    // Initialize the header
    void initialize(PageType page_type, uint16_t page_size) {
        checksum = 0;
        this->page_size = page_size;
        type = page_type;
        flags = 0;
        page_num = 0;
        free_space = page_size - sizeof(PageHeader);
        free_offset = sizeof(PageHeader);
        next_overflow = 0;
        owner_page = 0;
    }
    
    // Getters and setters for flags
    bool is_dirty() const { return (flags & 0x01) != 0; }
    void set_dirty(bool dirty) { 
        if (dirty) flags |= 0x01;
        else flags &= ~0x01;
    }
    
    bool is_overflow_page() const { return (flags & 0x02) != 0; }
    void set_overflow_page(bool overflow) { 
        if (overflow) flags |= 0x02;
        else flags &= ~0x02;
    }
    
    // Getters and setters for page IDs
    PageId next_overflow_id() const { 
        return PageId(next_overflow);
    }
    
    void set_next_overflow(PageId id) {
        next_overflow = id.value();
    }
    
    PageId owner_page_id() const {
        return PageId(owner_page);
    }
    
    void set_owner_page(PageId id) {
        owner_page = id.value();
    }
    
    // Validate the page header
    bool is_valid() const {
        return type != PageType::INVALID && 
               free_offset >= sizeof(PageHeader) &&
               free_space <= (page_size - sizeof(PageHeader));
    }
};
#pragma pack(pop)

static_assert(sizeof(PageHeader) == 32, "PageHeader size mismatch");

} // namespace storage
} // namespace kadedb

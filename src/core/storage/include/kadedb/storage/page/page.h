#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <iterator>
#include <type_traits>
#include <stdexcept>
#include "kadedb/storage/page/page_header.h"
#include "kadedb/storage/page/page_id.h"
#include "kadedb/storage/crc32c.h"
#include "kadedb/storage/span.h"

namespace kadedb {
namespace storage {

// Forward declarations
class PageManager;

/**
 * @brief Represents a database page in memory
 * 
 * The Page class provides an interface to access and modify the contents
 * of a database page. It includes methods for managing free space,
 * serializing/deserializing data, and handling overflow pages.
 */
class Page {
public:
    using Byte = std::byte;
    using Data = span<Byte>;
    
    /**
     * @brief Construct a new Page object with pre-allocated data
     * 
     * @param page_id The ID of this page
     * @param data The raw page data (must be at least page_size bytes)
     * @param page_size The size of the page in bytes
     */
    Page(PageId page_id, Byte* data, uint32_t page_size);
    
    /**
     * @brief Construct a new Page object with internal storage
     * 
     * @param page_id The ID of this page
     * @param page_size The size of the page in bytes
     */
    explicit Page(PageId page_id, uint32_t page_size);
    
    // Non-copyable but movable
    Page(const Page&) = delete;
    Page& operator=(const Page&) = delete;
    Page(Page&&) = default;
    Page& operator=(Page&&) = default;
    
    ~Page() = default;
    
    // Accessors
    PageId page_id() const noexcept { return page_id_; }
    PageType type() const noexcept { 
        const auto* hdr = header();
        return hdr ? hdr->type : PageType::INVALID; 
    }
    bool is_dirty() const noexcept { 
        const auto* hdr = header();
        return hdr && hdr->is_dirty(); 
    }
    bool is_overflow() const noexcept { 
        const auto* hdr = header();
        return hdr && hdr->is_overflow_page(); 
    }
    PageId next_overflow() const noexcept { 
        const auto* hdr = header();
        return hdr ? hdr->next_overflow_id() : PageId(); 
    }
    PageId owner_page() const noexcept { 
        const auto* hdr = header();
        return hdr ? hdr->owner_page_id() : PageId(); 
    }
    
    // Pin management
    bool is_pinned() const noexcept { 
        return pin_count_ > 0; 
    }
    
    void pin() noexcept { 
        ++pin_count_; 
    }
    
    void unpin() noexcept { 
        if (pin_count_ > 0) {
            --pin_count_; 
        }
    }
    
    // Raw data access
    const Byte* data() const noexcept { return data_.data(); }
    Byte* mutable_data() { 
        auto* hdr = mutable_header();
        if (hdr) {
            hdr->set_dirty(true);
        }
        return data_.data(); 
    }
    
    // Get the size of the page data
    size_t size() const noexcept { return data_.size(); }
    
    // Page header access
    const PageHeader* header() const noexcept { 
        return reinterpret_cast<const PageHeader*>(data_.data());
    }
    PageHeader* mutable_header() { 
        // Directly access the data vector to avoid recursion with mutable_data()
        if (!data_.empty()) {
            auto* hdr = reinterpret_cast<PageHeader*>(data_.data());
            hdr->set_dirty(true);
            return hdr;
        }
        return nullptr;
    }
    
    // Accessors for user data (data after the header)
    span<const Byte> get_user_data() const {
        const size_t header_size = sizeof(PageHeader);
        return {data() + header_size, data_.size() - header_size};
    }
    
    span<Byte> get_mutable_user_data() {
        const size_t header_size = sizeof(PageHeader);
        return {mutable_data() + header_size, data_.size() - header_size};
    }
    
    // Free space management
    uint32_t free_space() const noexcept { 
        auto hdr = header();
        return hdr ? hdr->free_space : 0; 
    }
    
    bool has_space(size_t required) const noexcept { 
        auto hdr = header();
        return hdr && hdr->free_space >= required; 
    }
    
    /**
     * @brief Allocate space in the page
     * 
     * @param size Number of bytes to allocate
     * @return std::span<Byte> View of the allocated space
     * @throws std::runtime_error if there's not enough space
     */
    Data allocate(size_t size);
    
    /**
     * @brief Free previously allocated space
     * 
     * Note: This is a simple implementation. A real system would need
     * a more sophisticated free space management strategy.
     * 
     * @param offset Offset of the data to free
     * @param size Size of the data to free
     */
    void free(size_t offset, size_t size);
    
    /**
     * @brief Set the page as dirty
     */
    void set_dirty(bool dirty = true) {
        auto* hdr = mutable_header();
        if (hdr) {
            hdr->set_dirty(dirty);
        }
    }
    
    /**
     * @brief Update the page checksum
     */
    void update_checksum();
    
    /**
     * @brief Verify the page checksum
     */
    bool verify_checksum() const;
    
    /**
     * @brief Get the page size
     */
    uint32_t page_size() const noexcept { 
        return static_cast<uint32_t>(data_.size()); 
    }
    
private:
    friend class PageManager;
    // Data members
    PageId page_id_;
    std::vector<Byte> data_;
    std::size_t pin_count_ = 0;
    
    // Helper to get a pointer to a specific offset in the page
    Byte* get_pointer(size_t offset) {
        return data_.data() + offset;
    }
    
    const Byte* get_pointer(size_t offset) const {
        return data_.data() + offset;
    }
};

} // namespace storage
} // namespace kadedb

#pragma once

#include <cstdint>
#include <string>
#include <functional>

namespace kadedb {
namespace storage {

/**
 * @brief Represents a page identifier in the database
 * 
 * PageId is a strongly-typed wrapper around a 64-bit unsigned integer
 * that uniquely identifies a page in the database file.
 */
class PageId {
public:
    using value_type = uint64_t;
    
    /**
     * @brief Construct an invalid page ID
     */
    constexpr PageId() noexcept : value_(INVALID) {}
    
    /**
     * @brief Construct a page ID from a raw value
     */
    explicit constexpr PageId(value_type value) noexcept : value_(value) {}
    
    /**
     * @brief Get the raw value of the page ID
     */
    constexpr value_type value() const noexcept { return value_; }
    
    /**
     * @brief Check if the page ID is valid
     */
    constexpr bool is_valid() const noexcept { return value_ != INVALID; }
    
    // Comparison operators
    constexpr bool operator==(const PageId& other) const noexcept { return value_ == other.value_; }
    constexpr bool operator!=(const PageId& other) const noexcept { return !(*this == other); }
    constexpr bool operator<(const PageId& other) const noexcept { return value_ < other.value_; }
    constexpr bool operator<=(const PageId& other) const noexcept { return value_ <= other.value_; }
    constexpr bool operator>(const PageId& other) const noexcept { return value_ > other.value_; }
    constexpr bool operator>=(const PageId& other) const noexcept { return value_ >= other.value_; }
    
    /**
     * @brief Convert to string representation
     */
    std::string to_string() const { return std::to_string(value_); }
    
    /**
     * @brief Hash function for PageId
     */
    struct Hash {
        std::size_t operator()(const PageId& id) const noexcept {
            return std::hash<value_type>{}(id.value_);
        }
    };
    
    // Special values
    static constexpr value_type INVALID = 0;
private:
    value_type value_;
};

// Non-member functions
constexpr PageId make_page_id(PageId::value_type value) {
    return PageId(value);
}

} // namespace storage
} // namespace kadedb

// Specialize std::hash for PageId
namespace std {

template<>
struct hash<kadedb::storage::PageId> {
    size_t operator()(const kadedb::storage::PageId& id) const {
        return hash<kadedb::storage::PageId::value_type>{}(id.value());
    }
};

} // namespace std

#pragma once

namespace kadedb {
namespace storage {

/**
 * @brief Enumeration of page types in the database
 */
enum class PageType : uint8_t {
    INVALID = 0,    // Invalid or uninitialized page
    DATA = 1,       // Regular data page
    INDEX = 2,      // Index page (B-tree, etc.)
    META = 3,       // Metadata page
    FREE = 4,       // Free space management page
    // Add more page types as needed
};

} // namespace storage
} // namespace kadedb

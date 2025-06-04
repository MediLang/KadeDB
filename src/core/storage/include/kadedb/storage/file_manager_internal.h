#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <kadedb/storage/file_manager.h>

namespace kadedb {
namespace storage {

// Forward declarations
class FileHandle;

/**
 * @brief Internal implementation details for FileManager
 * 
 * This header contains private implementation details that should not be exposed
 * in the public API. It's included by implementation files that need access to
 * the internal structures of FileManager.
 */

// Internal page structure
struct InternalPage {
    FileManager::PageHeader header;
    std::vector<std::byte> data;
    bool is_dirty{false};
    
    explicit InternalPage(uint32_t page_size) : data(page_size - sizeof(FileManager::PageHeader)) {}
};

} // namespace storage
} // namespace kadedb

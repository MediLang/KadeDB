#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>  // For std::hash
#include <kadedb/storage/file_manager.h>
#include <kadedb/storage/page/page.h>
#include <kadedb/storage/page/page_id.h>
#include <kadedb/storage/page/page_type.h>
#include <kadedb/storage/span.h>

namespace kadedb {
namespace storage {

// Forward declarations
class FileManager;

/**
 * @brief Manages database pages in memory and on disk
 * 
 * The PageManager is responsible for:
 * - Managing the page cache
 * - Allocating and deallocating pages
 * - Reading and writing pages to disk
 * - Managing free space
 */
class PageManager {
public:
    /**
     * @brief Construct a new Page Manager
     * 
     * @param file_manager The FileManager to use for I/O operations
     * @param cache_size Maximum number of pages to keep in memory
     */
    explicit PageManager(std::shared_ptr<FileManager> file_manager, size_t cache_size = 1000);
    
    ~PageManager() = default;
    
    // Non-copyable, non-movable
    PageManager(const PageManager&) = delete;
    PageManager& operator=(const PageManager&) = delete;
    PageManager(PageManager&&) = delete;
    PageManager& operator=(PageManager&&) = delete;
    
    /**
     * @brief Fetch a page from disk or cache
     * 
     * @param page_id The ID of the page to fetch
     * @return std::shared_ptr<Page> The requested page
     * @throws std::runtime_error if the page cannot be read
     */
    std::shared_ptr<Page> fetch_page(PageId page_id);
    
    /**
     * @brief Create a new page
     * 
     * @param type The type of page to create
     * @return std::shared_ptr<Page> The new page
     * @throws std::runtime_error if no more pages can be allocated
     */
    std::shared_ptr<Page> new_page(PageType type = PageType::DATA);
    
    /**
     * @brief Mark a page as dirty, indicating it needs to be written to disk
     * 
     * @param page The page to mark as dirty
     */
    void mark_dirty(const std::shared_ptr<Page>& page);
    
    /**
     * @brief Write a dirty page to disk
     * 
     * @param page The page to write
     * @param force If true, write even if the page is not dirty
     */
    void write_page(const std::shared_ptr<Page>& page, bool force = false);
    
    /**
     * @brief Write all dirty pages to disk
     */
    void flush_all();
    
    /**
     * @brief Free a page, making it available for reuse
     * 
     * @param page_id The ID of the page to free
     */
    void free_page(PageId page_id);
    
    /**
     * @brief Get the page size in bytes
     */
    uint32_t page_size() const { return page_size_; }
    
    /**
     * @brief Get the number of pages currently in the cache
     */
    size_t cache_size() const { return page_cache_.size(); }
    
    /**
     * @brief Get the number of pages in the database
     */
    uint64_t page_count() const;
    
private:
    // Cache entry structure
    struct PageCacheEntry {
        std::shared_ptr<Page> page;
        bool is_dirty;
        
        // Default constructor required for unordered_map::operator[]
        PageCacheEntry() noexcept : page(nullptr), is_dirty(false) {}
        
        explicit PageCacheEntry(std::shared_ptr<Page> p) noexcept
            : page(std::move(p)), is_dirty(false) {}
            
        // Disable copy and move to prevent accidental slicing
        PageCacheEntry(const PageCacheEntry&) = delete;
        PageCacheEntry& operator=(const PageCacheEntry&) = delete;
        PageCacheEntry(PageCacheEntry&&) = default;
        PageCacheEntry& operator=(PageCacheEntry&&) = default;
    };
    
    // File manager for I/O operations
    std::shared_ptr<FileManager> file_manager_;
    
    // Cache configuration
    size_t cache_size_;
    uint32_t page_size_;
    
    // Page cache
    std::unordered_map<PageId, PageCacheEntry, PageId::Hash> page_cache_;
    
    // LRU list for page replacement (most recently used at front)
    std::list<PageId> lru_list_;
    
    // Free list of reusable page IDs (using a list for O(1) removal from front)
    std::list<PageId> free_list_;
    
    // Synchronization
    mutable std::mutex mutex_;
    
    // Private methods
    bool evict_page();
    
    /**
     * @brief Add a page to the cache
     * 
     * @param page The page to add to the cache
     * @throws std::runtime_error if the page cannot be added to the cache
     */
    void add_to_cache(std::shared_ptr<Page> page);
    
    /**
     * @brief Read a page from disk into the provided buffer
     * 
     * @param page_id The ID of the page to read
     * @param buffer The buffer to read the page into (must be at least page_size_ bytes)
     * @throws std::runtime_error if the page cannot be read
     */
    void read_page_from_disk(PageId page_id, void* buffer);
    
    /**
     * @brief Write a page to disk
     * 
     * @param page_id The ID of the page to write
     * @param data The page data to write (must be exactly page_size_ bytes)
     * @throws std::runtime_error if the page cannot be written
     */
    void write_page_to_disk(PageId page_id, const void* data);
    
    /**
     * @brief Allocate a new page ID, either from the free list or by extending the file
     * 
     * @return PageId The newly allocated page ID
     * @throws std::runtime_error if no more pages can be allocated
     */
    PageId allocate_new_page();
};

} // namespace storage
} // namespace kadedb

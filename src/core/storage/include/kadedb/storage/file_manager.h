#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <system_error>
#include <functional>
#include <vector>

namespace kadedb {
namespace storage {

// Forward declarations
class FileHandle;

/**
 * @brief Manages database file operations with a custom format
 * 
 * The FileManager handles all low-level file I/O operations for the database,
 * including creating, opening, and managing database files with a custom format.
 */
class FileManager {
public:
    static constexpr uint32_t DEFAULT_PAGE_SIZE = 4096;  // 4KB pages by default
    static constexpr const char FILE_SIGNATURE[7] = "KADEDB";
    static constexpr uint16_t CURRENT_VERSION = 1;

    /**
     * @brief File header structure
     */
    struct FileHeader {
        char signature[6];        // File signature "KADEDB"
        uint16_t version;         // File format version
        uint32_t page_size;       // Size of each page in bytes
        uint64_t page_count;      // Total number of pages in the file
        uint64_t free_page_list;  // Pointer to the first free page
        uint8_t reserved[100];    // Reserved for future use
    };

    /**
     * @brief Page header structure
     */
    struct PageHeader {
        uint64_t next_free;       // Next free page in the free list
        uint32_t page_type;       // Type of the page
        uint32_t checksum;        // Checksum for data integrity
        uint64_t lsn;             // Log sequence number for recovery
    };

    /**
     * @brief Represents a page in memory
     */
    struct Page {
        PageHeader header;        // Page header
        char data[];              // Page data (flexible array member)
    };
    
    /**
     * @brief Constructor
     */
    FileManager();
    
    /**
     * @brief Destructor
     */
    ~FileManager();

    // Disable copy and move
    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;
    FileManager(FileManager&&) = delete;
    FileManager& operator=(FileManager&&) = delete;

    /**
     * @brief Creates a new database file
     * @param filename Path to the database file
     * @param page_size Size of each page in bytes
     * @return std::error_code Error code if operation fails
     */
    std::error_code create_file(const std::string& filename, uint32_t page_size = DEFAULT_PAGE_SIZE);

    /**
     * @brief Opens an existing database file
     * @param filename Path to the database file
     * @return std::error_code Error code if operation fails
     */
    std::error_code open_file(const std::string& filename);

    /**
     * @brief Closes the currently open file
     */
    void close_file();

    /**
     * @brief Checks if a file is currently open
     * @return true if a file is open, false otherwise
     */
    bool is_open() const;

    /**
     * @brief Allocates a new page
     * @param page_type Type of the page
     * @return Page pointer on success, nullptr on failure
     */
    Page* allocate_page(uint32_t page_type);

    /**
     * @brief Frees a page
     * @param page_id ID of the page to free
     */
    void free_page(uint64_t page_id);

    /**
     * @brief Reads a page into memory
     * @param page_id ID of the page to read
     * @return Page pointer on success, nullptr on failure
     */
    Page* read_page(uint64_t page_id);

    /**
     * @brief Writes a page to disk
     * @param page_id ID of the page to write
     * @return std::error_code Error code if operation fails
     */
    std::error_code write_page(uint64_t page_id);

    /**
     * @brief Flushes all pending writes to disk
     * @return std::error_code Error code if operation fails
     */
    std::error_code flush();

    /**
     * @brief Gets the page size
     * @return Page size in bytes
     */
    uint32_t page_size() const;

    /**
     * @brief Gets the total number of pages in the file
     * @return Total number of pages
     */
    uint64_t page_count() const;

    /**
     * @brief Iterates over all pages in the file
     * @param callback Function to call for each page (page_id, page, page_type)
     */
    void for_each_page(std::function<void(uint64_t, Page*, uint32_t)> callback);

    /**
     * @brief Extends the file with additional pages
     * @param num_pages Number of pages to add
     * @return std::error_code Error code if operation fails
     */
    std::error_code extend_file(size_t num_pages = 1);

    /**
     * @brief Validates the file header
     * @return std::error_code Error code if validation fails
     */
    std::error_code validate_header() const;

    /**
     * @brief Gets the file header
     * @return Pointer to the file header
     */
    const FileHeader* get_file_header() const;

    /**
     * @brief Gets the file header (mutable)
     * @return Pointer to the file header
     */
    FileHeader* get_file_header_mutable();

private:
    std::unique_ptr<FileHandle> impl_;  // PIMPL idiom
    static constexpr size_t INITIAL_PAGES = 32;  // Initial number of pages to allocate
    static constexpr size_t EXTENSION_FACTOR = 2;  // How much to grow the file by when extending
};

} // namespace storage
} // namespace kadedb

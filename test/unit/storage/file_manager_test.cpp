#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <unordered_set>
#include <kadedb/storage/file_manager.h>

using namespace std::string_literals;

namespace fs = std::filesystem;
using namespace kadedb::storage;

class FileManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for test files
        test_dir = fs::temp_directory_path() / "kadedb_test";
        fs::create_directories(test_dir);
        test_file = test_dir / "test.db";
    }

    void TearDown() override {
        // Clean up test files
        if (fs::exists(test_file)) {
            fs::remove(test_file);
        }
        if (fs::exists(test_dir)) {
            fs::remove(test_dir);
        }
    }

    fs::path test_dir;
    fs::path test_file;
    const uint32_t TEST_PAGE_SIZE = 4096;  // 4KB pages for testing
    const uint32_t TEST_DATA_SIZE = 1024;  // 1KB test data
};

TEST_F(FileManagerTest, CreateAndOpenFile) {
    // Test creating a new file
    {
        FileManager fm;
        auto ec = fm.create_file(test_file.string(), TEST_PAGE_SIZE);
        EXPECT_FALSE(ec) << "Failed to create file: " << ec.message();
        EXPECT_TRUE(fm.is_open());
        EXPECT_EQ(fm.page_size(), TEST_PAGE_SIZE);
        EXPECT_GE(fm.page_count(), 1);  // At least header page
        
        // Verify file exists and has reasonable size
        EXPECT_TRUE(fs::exists(test_file));
        EXPECT_GT(fs::file_size(test_file), 0);
    }

    // Test opening the existing file
    {
        FileManager fm;
        auto ec = fm.open_file(test_file.string());
        EXPECT_FALSE(ec) << "Failed to open file: " << ec.message();
        EXPECT_TRUE(fm.is_open());
        EXPECT_EQ(fm.page_size(), TEST_PAGE_SIZE);
        EXPECT_GE(fm.page_count(), 1);
        
        // Test file header
        auto* header = fm.get_file_header();
        ASSERT_NE(header, nullptr);
        EXPECT_EQ(std::string(header->signature, 6), FileManager::FILE_SIGNATURE);
        EXPECT_EQ(header->version, FileManager::CURRENT_VERSION);
        EXPECT_EQ(header->page_size, TEST_PAGE_SIZE);
    }
    
    // Test opening non-existent file
    {
        FileManager fm;
        auto ec = fm.open_file("nonexistent_file.db");
        EXPECT_TRUE(ec);
        EXPECT_FALSE(fm.is_open());
    }
}

TEST_F(FileManagerTest, PageAllocationAndFree) {
    FileManager fm;
    auto ec = fm.create_file(test_file.string(), TEST_PAGE_SIZE);
    ASSERT_FALSE(ec) << "Failed to create file: " << ec.message();
    
    const uint32_t NUM_PAGES = 10;
    std::vector<uint64_t> page_ids;
    
    // Allocate multiple pages
    for (uint32_t i = 0; i < NUM_PAGES; ++i) {
        auto* page = fm.allocate_page(i % 3 + 1);  // Cycle through page types 1-3
        ASSERT_NE(page, nullptr) << "Failed to allocate page " << i;
        
        // Write unique data to each page
        std::string test_data = "Page " + std::to_string(i) + " data";
        std::memcpy(page->data, test_data.data(), std::min(test_data.size(), TEST_DATA_SIZE));
        
        // Write page to disk
        uint64_t page_id = i + 1;  // Page 0 is header
        ec = fm.write_page(page_id);
        EXPECT_FALSE(ec) << "Failed to write page " << page_id << ": " << ec.message();
        
        page_ids.push_back(page_id);
    }
    
    // Verify all pages were allocated
    EXPECT_GE(fm.page_count(), NUM_PAGES + 1);  // +1 for header page
    
    // Free some pages
    for (size_t i = 0; i < page_ids.size(); i += 2) {
        fm.free_page(page_ids[i]);
    }
    
    // Allocate new pages - should reuse freed pages
    std::unordered_set<uint64_t> new_page_ids;
    for (int i = 0; i < 3; ++i) {
        auto* page = fm.allocate_page(4);  // New page type
        ASSERT_NE(page, nullptr);
        
        uint64_t page_id = NUM_PAGES + 1 + i;
        new_page_ids.insert(page_id);
        
        // Write some data
        std::string test_data = "New page " + std::to_string(i);
        std::memcpy(page->data, test_data.data(), std::min(test_data.size(), TEST_DATA_SIZE));
        
        ec = fm.write_page(page_id);
        EXPECT_FALSE(ec) << "Failed to write new page " << page_id;
    }
    
    // Verify page reuse (some of the new pages should have reused freed pages)
    bool found_reused_page = false;
    for (size_t i = 0; i < page_ids.size(); i += 2) {
        if (new_page_ids.count(page_ids[i]) > 0) {
            found_reused_page = true;
            break;
        }
    }
    EXPECT_TRUE(found_reused_page) << "Expected to find at least one reused page";
}

TEST_F(FileManagerTest, PageReadingAndValidation) {
    const std::string TEST_DATA = "Test data for reading with some more content to fill the page";
    
    // First create and write pages with different types
    {
        FileManager fm;
        auto ec = fm.create_file(test_file.string(), TEST_PAGE_SIZE);
        ASSERT_FALSE(ec) << "Failed to create file: " << ec.message();
        
        // Write multiple pages with different types
        for (uint32_t i = 0; i < 5; ++i) {
            auto* page = fm.allocate_page(i % 3 + 1);  // Types 1-3
            ASSERT_NE(page, nullptr) << "Failed to allocate page " << i;
            
            // Fill page with test data
            std::string page_data = TEST_DATA + " #" + std::to_string(i);
            size_t copy_size = std::min(page_data.size(), TEST_PAGE_SIZE);
            std::memcpy(page->data, page_data.data(), copy_size);
            
            // Write the page
            uint64_t page_id = i + 1;  // Page 0 is header
            ec = fm.write_page(page_id);
            ASSERT_FALSE(ec) << "Failed to write page " << page_id;
        }
        
        fm.flush();
    }
    
    // Reopen and verify all pages
    {
        FileManager fm;
        auto ec = fm.open_file(test_file.string());
        ASSERT_FALSE(ec) << "Failed to open file: " << ec.message();
        
        // Use for_each_page to verify all pages
        std::vector<uint64_t> found_pages;
        fm.for_each_page([&](uint64_t page_id, FileManager::Page* page, uint32_t page_type) {
            EXPECT_NE(page, nullptr);
            EXPECT_GE(page_type, 1);
            EXPECT_LE(page_type, 3);
            
            // Verify page data
            std::string expected = TEST_DATA + " #" + std::to_string(page_id - 1);
            std::string actual(page->data, std::min(expected.size(), TEST_PAGE_SIZE));
            EXPECT_EQ(actual, expected.substr(0, actual.size()));
            
            found_pages.push_back(page_id);
        });
        
        // Should have found all pages we wrote
        EXPECT_EQ(found_pages.size(), 5);
        
        // Test reading individual pages
        for (uint64_t page_id : found_pages) {
            auto* page = fm.read_page(page_id);
            ASSERT_NE(page, nullptr) << "Failed to read page " << page_id;
            
            // Verify checksum
            uint32_t original_checksum = page->header.checksum;
            uint32_t calculated_checksum = fm.calculate_checksum(page);
            EXPECT_EQ(original_checksum, calculated_checksum) 
                << "Checksum mismatch for page " << page_id;
        }
        
        // Test reading invalid page
        EXPECT_EQ(fm.read_page(999), nullptr);
    }
}

TEST_F(FileManagerTest, FileExtensionAndFreeList) {
    FileManager fm;
    auto ec = fm.create_file(test_file.string(), TEST_PAGE_SIZE);
    ASSERT_FALSE(ec) << "Failed to create file: " << ec.message();
    
    const size_t initial_pages = fm.page_count();
    
    // Allocate all available pages
    std::vector<uint64_t> allocated_pages;
    while (true) {
        auto* page = fm.allocate_page(1);
        if (!page) break;
        
        uint64_t page_id = fm.page_count() - 1;  // Last page is the one just allocated
        allocated_pages.push_back(page_id);
        
        // Write some data to the page
        std::string data = "Page " + std::to_string(page_id);
        std::memcpy(page->data, data.data(), std::min(data.size(), TEST_PAGE_SIZE));
        
        ec = fm.write_page(page_id);
        ASSERT_FALSE(ec) << "Failed to write page " << page_id;
    }
    
    // Should have extended the file
    EXPECT_GT(fm.page_count(), initial_pages);
    
    // Free some pages
    for (size_t i = 0; i < allocated_pages.size(); i += 2) {
        fm.free_page(allocated_pages[i]);
    }
    
    // Free list should now have these pages
    // Allocate new pages - should reuse freed pages first
    std::vector<uint64_t> new_pages;
    for (int i = 0; i < 5; ++i) {
        auto* page = fm.allocate_page(2);
        ASSERT_NE(page, nullptr) << "Failed to allocate page after free";
        
        uint64_t page_id = allocated_pages.size() + i;
        new_pages.push_back(page_id);
        
        // Verify we got a page from the free list if possible
        if (i < allocated_pages.size() / 2) {
            // Should be one of the freed pages
            bool is_reused = false;
            for (size_t j = 0; j < allocated_pages.size(); j += 2) {
                if (page_id == allocated_pages[j]) {
                    is_reused = true;
                    break;
                }
            }
            EXPECT_TRUE(is_reused) << "Page " << page_id << " should be reused from free list";
        }
    }
    
    // Test file extension under stress
    const size_t STRESS_PAGES = 100;
    std::vector<uint64_t> stress_pages;
    for (size_t i = 0; i < STRESS_PAGES; ++i) {
        auto* page = fm.allocate_page(3);
        ASSERT_NE(page, nullptr) << "Failed to allocate page " << i << " in stress test";
        
        uint64_t page_id = fm.page_count() - 1;
        stress_pages.push_back(page_id);
        
        // Write some data
        std::string data = "Stress page " + std::to_string(i);
        std::memcpy(page->data, data.data(), std::min(data.size(), TEST_PAGE_SIZE));
        
        ec = fm.write_page(page_id);
        ASSERT_FALSE(ec) << "Failed to write stress page " << i;
        
        // Every 10 pages, free some pages to create fragmentation
        if (i > 0 && i % 10 == 0) {
            size_t free_index = stress_pages.size() - 5;
            if (free_index > 0) {
                for (size_t j = 0; j < 3; ++j) {
                    if (free_index + j < stress_pages.size()) {
                        fm.free_page(stress_pages[free_index + j]);
                    }
                }
            }
        }
    }
    
    // Final flush to ensure everything is written
    ec = fm.flush();
    EXPECT_FALSE(ec) << "Failed to flush after stress test";
}

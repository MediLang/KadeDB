#include <gtest/gtest.h>
#include <kadedb/storage/page/page.h>
#include <kadedb/storage/page/page_manager.h>
#include <kadedb/storage/file_manager.h>
#include <filesystem>
#include <random>

namespace kadedb {
namespace storage {

class PageManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for test files
        test_dir_ = "./test_page_manager";
        std::filesystem::create_directories(test_dir_);
        
        // Create a file manager for testing
        file_manager_ = std::make_shared<FileManager>(test_dir_ + "/test.db");
        
        // Create a page manager with a small cache size for testing
        page_manager_ = std::make_unique<PageManager>(file_manager_, 10);
    }
    
    void TearDown() override {
        // Clean up
        page_manager_.reset();
        file_manager_.reset();
        std::filesystem::remove_all(test_dir_);
    }
    
    std::string test_dir_;
    std::shared_ptr<FileManager> file_manager_;
    std::unique_ptr<PageManager> page_manager_;
};

TEST_F(PageManagerTest, CreateAndRetrievePage) {
    // Create a new page
    auto page = page_manager_->new_page(PageType::DATA);
    ASSERT_NE(page, nullptr);
    
    // Write some data to the page
    const char* test_data = "Hello, Page!";
    auto data_span = page->allocate(std::strlen(test_data) + 1);
    std::memcpy(data_span.data(), test_data, data_span.size());
    
    // Mark as dirty and write back
    page_manager_->mark_dirty(page);
    page_manager_->write_page(page);
    
    // Get the page ID and release our reference
    PageId page_id = page->page_id();
    page.reset();
    
    // Fetch the page again
    auto fetched_page = page_manager_->fetch_page(page_id);
    ASSERT_NE(fetched_page, nullptr);
    
    // Verify the data
    auto read_span = std::string_view(
        reinterpret_cast<const char*>(fetched_page->data() + sizeof(PageHeader)),
        std::strlen(test_data) + 1);
    
    EXPECT_STREQ(read_span.data(), test_data);
}

TEST_F(PageManagerTest, FreePageReuse) {
    // Create and free a page
    auto page1 = page_manager_->new_page(PageType::DATA);
    PageId page1_id = page1->page_id();
    
    // Write some data to the page
    const char* test_data = "Test data";
    auto data_span = page1->allocate(std::strlen(test_data) + 1);
    std::memcpy(data_span.data(), test_data, data_span.size());
    
    // Free the page
    page1.reset();
    page_manager_->free_page(page1_id);
    
    // Allocate a new page - should reuse the freed page
    auto page2 = page_manager_->new_page(PageType::DATA);
    
    // The new page should have the same ID as the freed one
    EXPECT_EQ(page2->page_id(), page1_id);
    
    // The page should be initialized (header should be valid)
    EXPECT_EQ(page2->type(), PageType::DATA);
    EXPECT_GE(page2->free_space(), PageHeader::PAGE_SIZE - sizeof(PageHeader));
}

TEST_F(PageManagerTest, PageCacheEviction) {
    // Fill the cache (size is 10)
    std::vector<std::shared_ptr<Page>> pages;
    for (int i = 0; i < 15; ++i) {
        auto page = page_manager_->new_page(PageType::DATA);
        // Write some unique data to each page
        std::string data = "Page " + std::to_string(i);
        auto span = page->allocate(data.size() + 1);
        std::memcpy(span.data(), data.c_str(), span.size());
        page_manager_->mark_dirty(page);
        pages.push_back(page);
    }
    
    // The first few pages should have been evicted from the cache
    // Try to access the first page - should be read from disk
    PageId first_page_id = pages[0]->page_id();
    pages[0].reset();  // Release our reference
    
    auto fetched_page = page_manager_->fetch_page(first_page_id);
    ASSERT_NE(fetched_page, nullptr);
    
    // Verify the data
    std::string expected = "Page 0";
    auto read_span = std::string_view(
        reinterpret_cast<const char*>(fetched_page->data() + sizeof(PageHeader)),
        expected.size() + 1);
    
    EXPECT_STREQ(read_span.data(), expected.c_str());
}

TEST_F(PageManagerTest, PageChecksum) {
    // Create a new page
    auto page = page_manager_->new_page(PageType::DATA);
    
    // Write some data
    const char* test_data = "Test checksum";
    auto data_span = page->allocate(std::strlen(test_data) + 1);
    std::memcpy(data_span.data(), test_data, data_span.size());
    
    // Update checksum and verify
    page->update_checksum();
    EXPECT_TRUE(page->verify_checksum());
    
    // Corrupt the data and verify checksum fails
    auto* data = page->mutable_data();
    data[sizeof(PageHeader)] = ~data[sizeof(PageHeader)];  // Flip some bits
    EXPECT_FALSE(page->verify_checksum());
}

TEST_F(PageManagerTest, MultiplePageTypes) {
    // Test creating different page types
    auto data_page = page_manager_->new_page(PageType::DATA);
    auto index_page = page_manager_->new_page(PageType::INDEX);
    auto overflow_page = page_manager_->new_page(PageType::OVERFLOW);
    
    EXPECT_EQ(data_page->type(), PageType::DATA);
    EXPECT_EQ(index_page->type(), PageType::INDEX);
    EXPECT_EQ(overflow_page->type(), PageType::OVERFLOW);
    
    // Verify the headers are properly initialized
    EXPECT_GE(data_page->free_space(), PageHeader::PAGE_SIZE - sizeof(PageHeader));
    EXPECT_GE(index_page->free_space(), PageHeader::PAGE_SIZE - sizeof(PageHeader));
    EXPECT_GE(overflow_page->free_space(), PageHeader::PAGE_SIZE - sizeof(PageHeader));
}

} // namespace storage
} // namespace kadedb

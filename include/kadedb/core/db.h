#ifndef KADEDB_CORE_DB_H
#define KADEDB_CORE_DB_H

/**
 * @file db.h
 * @brief Core database interface for KadeDB
 * 
 * This file contains the core database interface for KadeDB.
 */

#include <string>
#include <memory>
#include <kadedb/core/export.h>

namespace kadedb {
namespace core {

/**
 * @class KadeDB
 * @brief Main database class for KadeDB
 */
class KadeDB {
public:
    /**
     * @brief Construct a new KadeDB object
     */
    KadeDB();
    
    /**
     * @brief Destroy the KadeDB object
     */
    ~KadeDB();
    
    // Prevent copying
    KadeDB(const KadeDB&) = delete;
    KadeDB& operator=(const KadeDB&) = delete;
    
    /**
     * @brief Open the database
     * @param path Path to the database directory
     * @return true if opened successfully, false otherwise
     */
    bool open(const std::string& path);
    
    /**
     * @brief Close the database
     */
    void close();
    
    /**
     * @brief Check if the database is open
     * @return true if open, false otherwise
     */
    bool is_open() const;
    
    /**
     * @brief Get the last error message
     * @return Last error message
     */
    std::string last_error() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace core
} // namespace kadedb

#endif // KADEDB_CORE_DB_H

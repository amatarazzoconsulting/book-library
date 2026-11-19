# Universal Database Manipulator

**By Anthony Matarazzo (c) 2026**

*100-page technical reference, translated to Markdown from `Universal Database Manipulator - Proton Docs.pdf`. Copyright (c) Anthony Matarazzo 2026. Released under the MIT License.*

## License

```text
MIT License

Copyright (c) 2026 Anthony Matarazzo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Table of Contents

- [1. Introduction](#1-introduction)
- [2. Installation](#2-installation)
- [3. Quick Start](#3-quick-start)
- [4. Core Concepts](#4-core-concepts)
- [5. Database Drivers](#5-database-drivers)
- [6. File Format Drivers](#6-file-format-drivers)
- [7. Streaming Operations](#7-streaming-operations)
- [8. Query Building](#8-query-building)
- [9. Indexing and Search](#9-indexing-and-search)
- [10. Backup and Restore](#10-backup-and-restore)
- [11. Transactions](#11-transactions)
- [12. Advanced Features](#12-advanced-features)
- [13. Best Practices](#13-best-practices)
- [14. API Reference](#14-api-reference)
- [15. Troubleshooting](#15-troubleshooting)
- [16. Examples](#16-examples)
- [Introduction](#introduction)
  - [Overview](#overview)
  - [Design Philosophy](#design-philosophy)
- [Installation](#installation)
  - [Prerequisites](#prerequisites)
  - [Building from Source](#building-from-source)
  - [Integration with CMake](#integration-with-cmake)
  - [Integration with Conan](#integration-with-conan)
- [Quick Start](#quick-start)
  - [Minimal Example](#minimal-example)
  - [Loading CSV File](#loading-csv-file)
  - [Transfer Between Databases](#transfer-between-databases)
- [Core Concepts](#core-concepts)
  - [Value System](#value-system)
  - [Row Structure](#row-structure)
  - [Connection String Formats](#connection-string-formats)
- [Database Drivers](#database-drivers)
  - [SQLite Driver](#sqlite-driver)
  - [MySQL Driver](#mysql-driver)
  - [PostgreSQL Driver](#postgresql-driver)
  - [Oracle Driver](#oracle-driver)
- [File Format Drivers](#file-format-drivers)
  - [CSV Driver](#csv-driver)
  - [JSON Driver](#json-driver)
  - [XML Driver](#xml-driver)
  - [YAML Driver](#yaml-driver)
  - [INI Driver](#ini-driver)
- [Streaming Operations](#streaming-operations)
  - [Basic Streaming](#basic-streaming)
  - [Streaming with Pipeline](#streaming-with-pipeline)
  - [Parallel Processing](#parallel-processing)
  - [Custom Stream Implementation](#custom-stream-implementation)
- [Query Building](#query-building)
  - [Basic Queries](#basic-queries)
  - [Complex Conditions](#complex-conditions)
  - [Getting Results](#getting-results)
- [Indexing and Search](#indexing-and-search)
  - [Creating Indexes](#creating-indexes)
  - [Searching with Indexes](#searching-with-indexes)
  - [Global Search Across Databases](#global-search-across-databases)
- [Backup and Restore](#backup-and-restore)
  - [Basic Backup Operations](#basic-backup-operations)
  - [Advanced Backup Features](#advanced-backup-features)
  - [Point-in-Time Recovery](#point-in-time-recovery)
  - [Export/Import Backups](#exportimport-backups)
- [Transactions](#transactions)
  - [Basic Transaction Usage](#basic-transaction-usage)
  - [Nested Transactions (Savepoints)](#nested-transactions-savepoints)
  - [Isolation Levels](#isolation-levels)
- [Advanced Features](#advanced-features)
  - [Connection Pooling](#connection-pooling)
  - [Prepared Statements](#prepared-statements)
  - [Bulk Operations](#bulk-operations)
  - [Performance Monitoring](#performance-monitoring)
  - [Real-time Notifications (PostgreSQL)](#real-time-notifications-postgresql)
  - [Working with JSON](#working-with-json)
  - [Working with Spatial Data](#working-with-spatial-data)
- [Best Practices](#best-practices)
  - [Performance](#performance)
- [1. Use Batch Operations](#1-use-batch-operations)
- [2. Use Transactions for Multiple Operations](#2-use-transactions-for-multiple-operations)
- [3. Use Prepared Statements for Repeated Queries](#3-use-prepared-statements-for-repeated-queries)
- [4. Use Connection Pooling for High Concurrency](#4-use-connection-pooling-for-high-concurrency)
- [5. Stream Large Datasets Instead of Loading All](#5-stream-large-datasets-instead-of-loading-all)
  - [Memory Management](#memory-management)
- [1. Use Smart Pointers](#1-use-smart-pointers)
- [2. Clear Large Batches When Done](#2-clear-large-batches-when-done)
  - [Error Handling](#error-handling)
- [1. Always Use Try-Catch](#1-always-use-try-catch)
- [2. Rollback Failed Transactions](#2-rollback-failed-transactions)
  - [Security](#security)
- [1. Never Hardcode Credentials](#1-never-hardcode-credentials)
- [2. Use Connection Encryption](#2-use-connection-encryption)
- [API Reference](#api-reference)
  - [Core Classes](#core-classes)
  - [Key Functions](#key-functions)
- [Troubleshooting](#troubleshooting)
  - [Common Errors and Solutions](#common-errors-and-solutions)
  - [Debugging Tips](#debugging-tips)
- [1. Enable Query Logging](#1-enable-query-logging)
- [2. Check Connection Status](#2-check-connection-status)
- [3. Validate Backups](#3-validate-backups)
- [4. Get Database Statistics](#4-get-database-statistics)
- [Examples](#examples)
  - [Complete Application Example](#complete-application-example)
  - [Data Migration Example](#data-migration-example)
- [Appendix](#appendix)
  - [Driver Support Matrix](#driver-support-matrix)
  - [Version History](#version-history)
  - [License](#license)
- [Conclusion](#conclusion)
- [Source Code](#source-code)
  - [MIT License](#mit-license)

---

## 1. Introduction

## 2. Installation

## 3. Quick Start

## 4. Core Concepts

## 5. Database Drivers

## 6. File Format Drivers

## 7. Streaming Operations

## 8. Query Building

## 9. Indexing and Search

## 10. Backup and Restore

## 11. Transactions

## 12. Advanced Features

## 13. Best Practices

## 14. API Reference

## 15. Troubleshooting

## 16. Examples

## Introduction

### Overview

The Universal Database Access Module is a comprehensive C++17 library that provides a unified interface for working with multiple database systems and file formats. It supports:

Relational Databases: MySQL, PostgreSQL, Oracle, SQLite File Formats: CSV, JSON, XML, YAML, INI Custom Formats: WebTable System integration Features: Streaming, indexing, backup/restore, transactions, cross-format transfers

### Design Philosophy

```text
Unified API: Same interface for all database types Streaming First: Process large datasets without memory overflow Type Safety: Strong typing with  std::variant -based value system Extensibility: Easy to add new drivers and formats Performance: Connection pooling, prepared statements, batch operations
```

## Installation

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake
sudo apt-get install libmysqlclient-dev
sudo apt-get install libpq-dev
sudo apt-get install libsqlite3-dev
sudo apt-get install libarchive-dev

# Oracle (download from Oracle website)
# Set ORACLE_HOME environment variable
```

### Building from Source

```bash
# Clone or download the library
git clone https://github.com/yourrepo/universal-db.git
cd universal-db

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Install (optional)
sudo make install
```

### Integration with CMake

```cmake
# In your CMakeLists.txt
find_package(UniversalDB REQUIRED)
target_link_libraries(your_target PRIVATE UniversalDB::UniversalDB)
```

### Integration with Conan

```python
# conanfile.txt
[requires]
universal-db/1.0.0

[generators]
cmake
```

## Quick Start

### Minimal Example

```cpp
#include <universal_db/universal_db.hpp>
#include <iostream>

using namespace udb;

int main() {
    try {
        // Create database connection
        UniversalDB db("sqlite", "test.db");

        // Create table
        db->get_driver().create_table("users", {
            {"id", "INTEGER PRIMARY KEY"},
            {"name", "TEXT"},
            {"age", "INTEGER"}
        });

        // Insert data

        db->insert("users", {{"name", "Alice"}, {"age", 30}});        db->insert("users", {{"name", "Bob"}, {"age", 25}});

        // Query data
        auto stream = db->query()
            .select({"name", "age"})
            .from("", "users")
            .where("age > 25")
            .execute();

        // Process results
        Row row;
        while (stream->next(row)) {

            std::cout << "Name: " << value_to_string(row["name"])                       << ", Age: " << value_to_string(row["age"]) << "\n";        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
```

### Loading CSV File

```cpp
// Load CSV into database
UniversalDB db("sqlite", "data.db");
db.load_csv("data.csv", "my_table", ",");
```

### Transfer Between Databases

```cpp
// Transfer data from MySQL to PostgreSQL

UniversalDB db("mysql", "host=localhost user=root db=source");db.attach_database("pg", "postgresql", "host=localhost user=postgres db=target");
db.transfer_between_drivers("", "source_table", "pg", "target_table");
```

## Core Concepts

### Value System

The library uses a variant-based value system for type-safe data handling:

```cpp
using Value = std::variant<
    std::monostate,  // NULL
    int,             // Integer
    long long,       // 64-bit integer
    double,          // Floating point
    std::string,     // String
    bool,            // Boolean
    std::vector<uint8_t>  // Binary data
>;
```

- Utility Functions:

```cpp
// Convert Value to string
std::string value_to_string(const Value& v);

// Convert Value to double
double value_to_double(const Value& v);

// Convert Value to integer
int value_to_int(const Value& v);

// Convert Value to boolean
bool value_to_bool(const Value& v);
```

### Row Structure

```cpp
using Row = std::unordered_map<std::string, Value>;
using RowPtr = std::shared_ptr<Row>;
```

### Connection String Formats

SQLite:

```text
"test.db"                           # File path
":memory:"                          # In-memory database
```

- MySQL:

```text
"host=localhost user=root password=pass db=test port=3306"
```

- PostgreSQL:

```text
"host=localhost port=5432 dbname=test user=postgres password=pass"
```

- Oracle:

```text
"user=scott password=tiger db=orcl host=localhost port=1521"
```

## Database Drivers

### SQLite Driver

Features:

File-based and in-memory databases

ACID compliant

Zero configuration

Example:

```cpp
UniversalDB db("sqlite", "database.db");

// In-memory database
UniversalDB db_mem("sqlite", ":memory:");
```

### MySQL Driver

Features:

Connection pooling

Master/slave replication

Full-text search

Spatial extensions

JSON support

Example:

```cpp
UniversalDB db("mysql", "host=localhost user=root password=secret db=test");

// With connection pool
MySQLDriver driver(true, 20);  // 20 connections in pool
```

- Advanced Operations:

```cpp
auto& mysql = db->get_driver();

// Create full-text index
mysql.create_fulltext_index("articles", "content");

// Insert JSON data

mysql.insert_json("products", "metadata", R"({"price": 100, "in_stock": true})");

// Spatial query
auto results = mysql.spatial_query("locations", "coord", -74.006, 40.7128, 1000);
```

### PostgreSQL Driver

Features:

- Advanced indexing (GIN, GiST, BRIN)

Full-text search with ranking

JSONB support

Listen/Notify for real-time updates

Logical replication

Example:

```cpp
UniversalDB db("postgresql", "host=localhost user=postgres db=test");

// Create JSONB index
db->get_driver().create_jsonb_index("products", "metadata", "price");

// Full-text search with relevance

auto results = db->get_driver().full_text_search("articles", "title,content",

                                                   "database AND performance", 10);

// Listen for notifications

db->get_driver().listen("channel", [](const std::string& payload) {    std::cout << "Received: " << payload << "\n";
});
```

### Oracle Driver

Features:

PL/SQL support

Flashback queries

Partitioning

Materialized views

RMAN backup

Data Guard

Example:

```cpp
UniversalDB db("oracle", "user=scott password=tiger db=orcl");

// Create partitioned table
db->get_driver().create_partition_table("sales", {
    {"id", "NUMBER"},
    {"sale_date", "DATE"},
    {"amount", "NUMBER(10,2)"}
}, "sale_date", {"'2024-01-01'", "'2024-07-01'", "MAXVALUE"});

// Flashback query

auto stream = db->get_driver().flashback_query("employees",     std::chrono::system_clock::now() - std::chrono::hours(24));

// Use sequence
int next_id = db->get_driver().get_sequence_nextval("user_seq");
```

## File Format Drivers

### CSV Driver

```cpp
// Load CSV file
UniversalDB db("sqlite", "test.db");
db.load_csv("data.csv", "my_table");

// With custom delimiter
db.load_csv("data.tsv", "my_table", "\t");

// Export to CSV
db.export_to_csv("my_table", "output.csv", ",");
```

### JSON Driver

```cpp
// Load JSON array
db.load_json("data.json", "json_table");

// JSON format expected:
// [{"id": 1, "name": "Alice"}, {"id": 2, "name": "Bob"}]
```

### XML Driver

```cpp
// Load XML file
db.load_xml("data.xml", "xml_table");

// XML format expected:
// <root>
//   <record><id>1</id><name>Alice</name></record>
//   <record><id>2</id><name>Bob</name></record>
// </root>
```

### YAML Driver

```cpp
// Load YAML file
db.load_yaml("data.yaml", "yaml_table");

// YAML format expected:
// - id: 1
//   name: Alice
// - id: 2
//   name: Bob
```

### INI Driver

```cpp
// Load INI file
db.load_ini("config.ini", "ini_table");

// INI format expected:
// [section1]
// key1=value1
// key2=value2
```

## Streaming Operations

### Basic Streaming

```cpp
// Create a stream from a table
auto stream = db->get_driver().scan_table("large_table");

// Process row by row
Row row;
while (stream->next(row)) {
    // Process each row without loading all into memory
    process_row(row);
}
```

### Streaming with Pipeline

```cpp
// Create pipeline with transformations
Pipeline pipeline;

pipeline.add_adaptor<RoundAdaptor>(2);      // Round to 2 decimals pipeline.add_adaptor<ScaleAdaptor>(1.8);    // Multiply by 1.8 pipeline.add_adaptor<UnitConvertAdaptor>(si, "C", "F", {"temperature"});

// Process stream through pipeline
pipeline.process(stream, [](Row& row) {
    // Handle transformed row
});
```

### Parallel Processing

```cpp
// Process stream in parallel
pipeline.process_parallel(stream, [](Row& row) {
    // This runs in multiple threads
    process_row(row);
}, 8);  // Use 8 threads
```

### Custom Stream Implementation

```cpp
class CustomStream : public IRecordStream {
    std::vector<Row> data;
    size_t pos = 0;

public:
    bool next(Row& out) override {
        if (pos >= data.size()) return false;
        out = data[pos++];
        return true;
    }

    void reset() override { pos = 0; }

    size_t estimate_size() const override { return data.size(); }};
```

## Query Building

### Basic Queries

```cpp
// Select all
auto stream = db->query()
    .select_all()
    .from("", "users")
    .execute();

// Select specific columns
auto stream = db->query()
    .select({"name", "age", "email"})
    .from("", "users")
    .where("age > 25")
    .order_by("age", "DESC")
    .limit(10)
    .execute();
```

### Complex Conditions

```cpp
// Multiple conditions
auto stream = db->query()
    .select({"name", "age"})
    .from("", "users")

    .where("age BETWEEN 20 AND 30 AND city = 'New York'")    .execute();

// Join across tables (requires driver support)
auto stream = db->query()
    .select({"users.name", "orders.total"})

    .from("", "users JOIN orders ON users.id = orders.user_id")    .where("orders.total > 100")
    .execute();
```

### Getting Results

```cpp
// Stream results (memory efficient)
auto stream = query.execute();
Row row;
while (stream->next(row)) {
    process(row);
}

// Get all results at once (small datasets)
auto results = query.execute_all();
for (const auto& row : results) {
    process(row);
}

// Get count only
size_t count = query.count();
```

## Indexing and Search

### Creating Indexes

```cpp
// Basic index
db->create_index("users", "email");

// Unique index
db->create_index("users", "username", true);

// MySQL full-text index
auto& mysql = db->get_driver();
mysql.create_fulltext_index("articles", "content");

// PostgreSQL GIN index for JSON
auto& pg = db->get_driver();
pg.create_gin_index("products", "metadata");

// PostgreSQL partial index
pg.create_partial_index("orders", "status", "status = 'pending'");

// Oracle bitmap index
auto& oracle = db->get_driver();
oracle.create_bitmap_index("sales", "region");
```

### Searching with Indexes

```cpp
// Search using index
auto row_ids = db->search("users", "email", "alice@example.com");

// Full-text search (PostgreSQL)

auto results = pg.full_text_search("articles", "title,content",                                     "database AND performance", 10);

// Spatial search (MySQL)

auto locations = mysql.spatial_query("places", "location",                                      -74.006, 40.7128, 1000);  // Within 1km
```

### Global Search Across Databases

```cpp
// Search across all attached databases
auto results = db->search_all("email", "alice@example.com");
for (const auto& result : results) {

    std::cout << "Found in " << result.database << "." << result.table << "\n";

    std::cout << "Relevance: " << result.relevance << "\n";}

// Full-text search across all databases

auto ft_results = db->full_text_search("database query", {"title", "content"});
```

## Backup and Restore

### Basic Backup Operations

```cpp
// Create full backup
db->backup("backup_20241201", "Full backup before migration");

// List all backups
db->list_backups();

// Restore from backup
db->restore("backup_20241201");

// Delete backup
BackupManager bm("./backups");
bm.delete_backup("old_backup");
```

### Advanced Backup Features

```cpp
BackupManager bm("./backups");

// Create incremental backup
bm.create_incremental_backup(driver, "base_backup_20241201");

// Create differential backup
bm.create_differential_backup(driver, "full_backup_20241201");

// Set compression (0-9)
bm.set_compression_level(9);
bm.set_compression_type("zstd");

// Enable encryption
bm.enable_encryption("my_secure_password");

// Cloud backup to AWS S3
bm.configure_cloud_backup("aws", "my-bucket", "access_key", "secret_key");bm.upload_to_cloud("backup_20241201");

// Scheduled backups (cron format: minute hour day month weekday)bm.schedule_backup(driver, "0 2 * * *", "daily_backup");  // 2 AM daily
```

### Point-in-Time Recovery

```cpp
// Create restore point
RestorePoint point("backup_20241201", std::chrono::system_clock::now());

// Add changes to track
point.add_change(before_row, after_row);

// Rollback to point
point.rollback(driver);

// Restore to specific timestamp
db->restore_to_point("backup_20241201", timestamp);
```

### Export/Import Backups

```cpp
// Export backup to external location
bm.export_backup("backup_20241201", "/mnt/external/backup.tar.gz");

// Import backup from external location
bm.import_backup("/mnt/external/backup.tar.gz");
```

## Transactions

### Basic Transaction Usage

```cpp
// Start transaction
db->begin();

try {

    db->insert("accounts", {{"id", 1}, {"balance", 1000}});    db->insert("accounts", {{"id", 2}, {"balance", 500}});    db->update("accounts", {{"balance", 900}}, "id = 1");    db->update("accounts", {{"balance", 600}}, "id = 2");

    db->commit();  // Commit if all succeeded
} catch (const std::exception& e) {
    db->rollback();  // Rollback on error

    std::cerr << "Transaction failed: " << e.what() << "\n";}
```

### Nested Transactions (Savepoints)

PostgreSQL:

```cpp
auto& pg = db->get_driver();
pg.begin_transaction();
pg.savepoint("sp1");

try {
    pg.insert("users", {{"name", "Alice"}});
    pg.savepoint("sp2");

    try {
        pg.insert("users", {{"name", "Bob"}});
        pg.commit();  // Commit inner
    } catch (...) {
        pg.rollback_to_savepoint("sp2");
    }

    pg.release_savepoint("sp1");
    pg.commit();
} catch (...) {
    pg.rollback_to_savepoint("sp1");
}
```

### Isolation Levels

```cpp
// Set transaction isolation level

db->get_driver().set_transaction_isolation("SERIALIZABLE");db->get_driver().set_transaction_read_only(true);

db->begin();
// Perform read-only operations
db->commit();
```

## Advanced Features

### Connection Pooling

```cpp
// Enable connection pool (MySQL, PostgreSQL, Oracle)
MySQLDriver driver(true, 20);  // 20 connections in pool

// Pool automatically manages connections
driver.insert("table", row);  // Acquires from pool
// Connection automatically returned to pool
```

### Prepared Statements

```cpp
// Prepared statements are automatically cached
for (int i = 0; i < 1000; i++) {
    // Same SQL pattern - uses cached prepared statement
    driver.insert("users", {{"id", i}, {"name", "User" +
std::to_string(i)}});
}
```

### Bulk Operations

```cpp
// Bulk insert (MySQL)
auto bulk = driver.create_bulk_inserter("users", 1000);
for (int i = 0; i < 100000; i++) {

    bulk.add({{"id", i}, {"name", "User" + std::to_string(i)}});}
bulk.flush();  // Auto-flushes when batch size reached

// Bulk copy (PostgreSQL)

auto copier = pg.create_bulk_copier("users", {"id", "name"});for (int i = 0; i < 100000; i++) {

    copier.add_row({{"id", i}, {"name", "User" + std::to_string(i)}});}
copier.flush();

// Bulk insert with array binding (Oracle)

auto inserter = oracle.create_bulk_insert("users", {"id", "name"}, 1000);for (int i = 0; i < 100000; i++) {

    inserter.add({{"id", i}, {"name", "User" + std::to_string(i)}});}
```

### Performance Monitoring

```cpp
// Get query metrics
auto metrics = driver.get_query_metrics();
for (const auto& m : metrics) {
    std::cout << "Query: " << m.query << "\n";

    std::cout << "  Executions: " << m.execution_count << "\n";    std::cout << "  Avg time: " << (m.total_time_ms / m.execution_count) << "ms\n";

    std::cout << "  Min time: " << m.min_time_ms << "ms\n";    std::cout << "  Max time: " << m.max_time_ms << "ms\n";}

// Reset metrics
driver.reset_metrics();

// Explain query plan

driver.explain_query("SELECT * FROM users WHERE age > 25");driver.explain_analyze("SELECT * FROM users WHERE age > 25");  // PostgreSQL

// Get table statistics
driver.get_table_statistics("users");
```

### Real-time Notifications (PostgreSQL)

```cpp
auto& pg = db->get_driver();

// Listen for notifications

pg.listen("user_updates", [](const std::string& payload) {    std::cout << "User updated: " << payload << "\n";
});

// Send notification
pg.notify("user_updates", "user_id=123");

// Unlisten
pg.unlisten("user_updates");
```

### Working with JSON

```cpp
// Insert JSON data (MySQL, PostgreSQL, Oracle)
db->get_driver().insert_json("products", "metadata",

                              R"({"name": "Laptop", "price": 999, "specs": {"ram": "16GB"}})");

// Query JSON field

std::string price = db->get_driver().query_json("products", "metadata", "price");

// Create JSON index (PostgreSQL)
pg.create_jsonb_index("products", "metadata", "price");

// Query JSON array (PostgreSQL)
auto results = pg.query_jsonb_array("products", "metadata", "specs.ram");
```

### Working with Spatial Data

```cpp
// Insert point (MySQL, PostgreSQL with PostGIS, Oracle Spatial)db->get_driver().insert_point("locations", "coord", -74.006, 40.7128);

// Spatial query - find points within radius

auto results = db->get_driver().spatial_query("locations", "coord",                                                 -74.006, 40.7128, 1000);  // 1km radius

// Create spatial index
db->get_driver().create_spatial_index("locations", "coord");
```

## Best Practices

### Performance

## 1. Use Batch Operations

```cpp
// Bad
for (auto& row : rows) {
    driver.insert("table", row);  // 1000 separate calls
}

// Good
driver.insert_batch("table", rows);  // Single call
```

## 2. Use Transactions for Multiple Operations

```cpp
driver.begin_transaction();
for (auto& row : rows) {
    driver.insert("table", row);
}
driver.commit();
```

## 3. Use Prepared Statements for Repeated Queries

```cpp
// Library automatically caches prepared statements
for (int i = 0; i < 10000; i++) {
    driver.insert("users", {{"id", i}, {"name", "User" +
std::to_string(i)}});
}
```

## 4. Use Connection Pooling for High Concurrency

```cpp
MySQLDriver driver(true, 20);  // 20 connections in pool
```

## 5. Stream Large Datasets Instead of Loading All

```cpp
// Bad - loads everything
auto rows = driver.query("SELECT * FROM large_table").execute_all();

// Good - streams row by row
auto stream = driver.scan_table("large_table");
Row row;
while (stream->next(row)) {
    process(row);
}
```

### Memory Management

## 1. Use Smart Pointers

```cpp
std::unique_ptr<IRecordStream> stream = driver.query(sql);
```

## 2. Clear Large Batches When Done

```cpp
{
    auto stream = driver.scan_table("large_table");
    // Process stream
} // Stream automatically destroyed
```

### Error Handling

## 1. Always Use Try-Catch

```cpp
try {
    db->insert("users", row);
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
}
```

## 2. Rollback Failed Transactions

```cpp
db->begin();
try {
    // Operations
    db->commit();
} catch (...) {
    db->rollback();
    throw;
}
```

### Security

## 1. Never Hardcode Credentials

```cpp
// Bad

UniversalDB db("mysql", "host=localhost user=root password=hardcoded db=test");

// Good - use environment variables
std::string password = std::getenv("DB_PASSWORD");

UniversalDB db("mysql", "host=localhost user=root password=" + password + " db=test");
```

## 2. Use Connection Encryption

```cpp
// MySQL with SSL

"host=localhost user=root password=pass db=test ssl-ca=ca.pem ssl-cert=client-cert.pem"

// PostgreSQL with SSL
"host=localhost port=5432 dbname=test sslmode=require"
```

## API Reference

### Core Classes

| Class | Description |
| --- | --- |
| UniversalDatabase | Main database interface |
| UniversalDB | Convenience wrapper |
| IDatabaseDriver | Driver interface |
| IRecordStream | Streaming interface |
| BackupManager | Backup/restore management |
| Pipeline | Transformation pipeline |
| Index<T> | Index container |

### Key Functions

UniversalDatabase

```cpp
// Constructor

UniversalDatabase(const std::string& driver_type, const std::string& conn_string);

// Driver management

void attach_database(const std::string& alias, const std::string& driver_type,
                     const std::string& conn_string);
IDatabaseDriver& get_driver(const std::string& alias = "");

// File operations

void load_csv(const std::string& filename, const std::string& table,               const std::string& delimiter = ",");

void load_json(const std::string& filename, const std::string& table);void export_to_csv(const std::string& table, const std::string& filename);

// Query
QueryBuilder query();
void insert(const std::string& table, const Row& row);
void insert_batch(const std::string& table, const std::vector<Row>& rows);

// Backup

void backup(const std::string& name = "", const std::string& description = "");
void restore(const std::string& backup_id);
void list_backups() const;

// Transactions
void begin();
void commit();
void rollback();

IDatabaseDriver
```

```cpp
// Connection

virtual void connect(const std::string& conn_string) = 0;virtual void disconnect() = 0;

// CRUD

virtual void insert(const std::string& table, const Row& row) = 0;virtual void insert_batch(const std::string& table, const std::vector<Row>& rows) = 0;

virtual void update(const std::string& table, const Row& row, const std::string& where) = 0;
virtual void remove(const std::string& table, const std::string& where) = 0;

// Query

virtual std::unique_ptr<IRecordStream> query(const std::string& sql) = 0;virtual std::unique_ptr<IRecordStream> scan_table(const std::string& table) = 0;

// Schema
virtual void create_table(const std::string& table,

                         const std::vector<std::pair<std::string, std::string>>& columns) = 0;
virtual void drop_table(const std::string& table) = 0;

// Index

virtual void create_index(const std::string& table, const std::string& column, bool unique = false) = 0;

virtual void drop_index(const std::string& table, const std::string& column) = 0;

// Transaction
virtual void begin_transaction() = 0;
virtual void commit() = 0;
virtual void rollback() = 0;

// Backup
virtual void backup(const std::string& backup_path) = 0;
virtual void restore(const std::string& backup_path) = 0;
```

## Troubleshooting

### Common Errors and Solutions

Connection Errors

| Error | Solution |
| --- | --- |
| Can't connect to MySQL server | Check host, port, and firewall |
| FATAL: no pg_hba.conf entry | Configure PostgreSQL authentication |
| ORA-12154: TNS could not | Check Oracle TNSNAMES.ora |

```cpp
resolve

Performance Issues

Issue
Slow inserts
High memory usage
Slow queries

Backup/Restore Issues

Issue
Backup failed
Restore failed
```

### Debugging Tips

## 1. Enable Query Logging

```cpp
db->enable_query_logging("queries.log");
```

## 2. Check Connection Status

```cpp
if (!driver.is_connected()) {
    std::cerr << "Not connected!\n";
}
```

## 3. Validate Backups

```cpp
BackupManager bm;
bm.verify_backup("backup_id");
```

## 4. Get Database Statistics

```cpp
auto stats = db->get_stats();
std::cout << "Tables: " << stats.total_tables << "\n";
std::cout << "Rows: " << stats.total_rows << "\n";
```

## Examples

### Complete Application Example

```cpp
#include <universal_db/universal_db.hpp>
#include <iostream>
#include <string>

using namespace udb;

class UserManager {
    UniversalDB& db;

public:
    UserManager(UniversalDB& database) : db(database) {
        // Ensure users table exists
        db.get_driver().create_table("users", {

            {"id", "INTEGER PRIMARY KEY AUTO_INCREMENT"},            {"username", "VARCHAR(50) UNIQUE"},
            {"email", "VARCHAR(100)"},

            {"created_at", "TIMESTAMP DEFAULT CURRENT_TIMESTAMP"}        });
    }

    bool add_user(const std::string& username, const std::string& email) {        try {

            db.insert("users", {{"username", username}, {"email", email}});            return true;
        } catch (const std::exception& e) {

            std::cerr << "Failed to add user: " << e.what() << "\n";            return false;
        }
    }

    std::vector<std::string> list_users() {
        std::vector<std::string> users;
        auto stream = db.query()
            .select({"username", "email"})
            .from("", "users")
            .order_by("created_at", "DESC")
            .execute();

        Row row;
        while (stream->next(row)) {
            std::string user = value_to_string(row["username"]) +
                               " <" + value_to_string(row["email"]) + ">";            users.push_back(user);
        }
        return users;
    }

    void backup_users() {

        db.backup("users_backup", "Backup of users table");        db.export_to_csv("users", "users_backup.csv");
    }
};

int main() {
    try {
        // Initialize database
        UniversalDB db("sqlite", "app.db");

        // Create user manager
        UserManager users(db);

        // Add users
        users.add_user("alice", "alice@example.com");
        users.add_user("bob", "bob@example.com");

        // List users
        auto user_list = users.list_users();
        std::cout << "Users:\n";
        for (const auto& user : user_list) {
            std::cout << "  " << user << "\n";
        }

        // Backup
        users.backup_users();

        // Get statistics
        auto stats = db->get_stats();
        std::cout << "\nDatabase Statistics:\n";

        std::cout << "  Tables: " << stats.total_tables << "\n";        std::cout << "  Rows: " << stats.total_rows << "\n";

    } catch (const std::exception& e) {

        std::cerr << "Fatal error: " << e.what() << "\n";        return 1;
    }

    return 0;
}
```

### Data Migration Example

```cpp
#include <universal_db/universal_db.hpp>

using namespace udb;

class DataMigrator {
public:

    void migrate_csv_to_postgres(const std::string& csv_file,                                   const std::string& pg_conn) {        // Source: CSV file
        UniversalDB source("csv", csv_file);

        // Target: PostgreSQL
        UniversalDB target("postgresql", pg_conn);

        // Create table if not exists

        target->get_driver().create_table("imported_data", {            {"id", "SERIAL PRIMARY KEY"},
            {"name", "TEXT"},
            {"value", "NUMERIC"}
        });

        // Transfer data
        auto stream = source->open_csv(csv_file);

        target->transfer(*stream, target->get_driver(), "imported_data", 1000);

        // Create indexes for performance
        target->create_index("imported_data", "name");

        std::cout << "Migration complete!\n";
    }

    void migrate_database(const std::string& source_type,                          const std::string& source_conn,                          const std::string& target_type,                          const std::string& target_conn) {        UniversalDB source(source_type, source_conn);
        source.attach_database("target", target_type, target_conn);

        // Get all tables from source
        auto tables = source->get_driver().list_tables();

        for (const auto& table : tables) {

            std::cout << "Migrating table: " << table << "\n";            source.transfer_between_drivers("", table, "target", table);
        }

        // Create backup before migration
        source.backup("pre_migration_backup", "Backup before migration");

        std::cout << "Database migration complete!\n";
    }
};

int main() {
    DataMigrator migrator;

    // Migrate CSV to PostgreSQL
    migrator.migrate_csv_to_postgres("data.csv",

                                     "host=localhost user=postgres db=mydb");

    // Migrate MySQL to PostgreSQL

    migrator.migrate_database("mysql", "host=localhost user=root db=source",                              "postgresql", "host=localhost user=postgres db=target");

    return 0;
}
```

## Appendix

### Driver Support Matrix

Feature  SQLite  MySQL  Postgre  Oracle  CSV  JSON

```sql
ACID
❌Transacti
```

```ons
Indexes
Foreign
❌Keys
```

```json
❌Support
Full-Text
❌Search
Spatial
❌Data
Streamin
✅g
Parallel
✅Processi
ng
Backup/R
```

### Version History

```text
Version    Date       Features
1.0.0      2024-01-   Initial
           01         release
1.1.0      2024-03-   Added
           15         Oracle
```

```support
1.2.0
01
```

```encryptio
n
1.3.0
10
```

### License

MIT License - See LICENSE file for details

## Conclusion

The Universal Database Access Module provides a powerful, flexible, and efficient way to work with multiple database systems and file formats from a single C++ API. With features like streaming, parallel processing, comprehensive backup/restore, and driver-specific optimizations, it's suitable for both small applications and enterprise-scale data processing systems.

## Source Code

Project Structure

```text
universal_db/
│
```

```text
├── CMakeLists.txt
├── LICENSE
├── README.md
```

│

```text
├── include/
│   └── universal_db/
│       ├── universal_db.hpp          # MAIN HEADER - Fluent API (unified)
│       │
│       ├── core/
│       │   ├── types.hpp              # Value, Row, basic types
│       │   ├── exceptions.hpp         # Exception classes
│       │   └── constants.hpp          # Global constants
│       │
│       ├── streaming/
│       │   ├── record_stream.hpp      # IRecordStream interface
│       │   ├── vector_stream.hpp      # Vector-backed stream
│       │   ├── file_stream.hpp        # File streaming
│       │   └── parallel_stream.hpp    # Parallel processing stream
│       │
│       ├── query/
│       │   ├── query_builder.hpp      # Fluent SQL builder
│       │   ├── result.hpp             # Result set with iterators
│       │   └── expression.hpp         # Expression system
│       │
│       ├── table/
│       │   ├── table.hpp              # Table class (fluent API)
│       │   ├── schema.hpp             # Schema management
│       │   ├── column.hpp             # Column definition
│       │   └── constraint.hpp         # Constraints (PK, FK, CHECK)
│       │
│       ├── index/
│       │   ├── index.hpp              # Index base class
│       │   ├── btree_index.hpp        # B-tree implementation
│       │   ├── hash_index.hpp         # Hash index
│       │   └── fulltext_index.hpp     # Full-text search index
│       │
│       ├── drivers/
│       │   ├── driver_interface.hpp   # IDatabaseDriver interface
│       │   ├── driver_factory.hpp     # Factory pattern
│       │   │
│       │   ├── memory/
│       │   │   ├── memory_driver.hpp  # In-memory driver
│       │   │   └── memory_driver.cpp
│       │   │
│       │   ├── csv/
│       │   │   ├── csv_driver.hpp     # CSV file driver
│       │   │   └── csv_driver.cpp
│       │   │
│       │   ├── sqlite/
│       │   │   ├── sqlite_driver.hpp  # SQLite driver
│       │   │   └── sqlite_driver.cpp
│       │   │
│       │   ├── mysql/
│       │   │   ├── mysql_driver.hpp   # MySQL driver
│       │   │   └── mysql_driver.cpp
│       │   │
│       │   ├── postgres/
│       │   │   ├── postgres_driver.hpp # PostgreSQL driver
│       │   │   └── postgres_driver.cpp
│       │   │
│       │   ├── oracle/
│       │   │   ├── oracle_driver.hpp  # Oracle driver
│       │   │   └── oracle_driver.cpp
│       │   │
│       │   └── file_formats/
│       │       ├── json_driver.hpp    # JSON file driver
│       │       ├── json_driver.cpp
│       │       ├── xml_driver.hpp     # XML file driver
│       │       ├── xml_driver.cpp
│       │       ├── yaml_driver.hpp    # YAML file driver
│       │       ├── yaml_driver.cpp
│       │       └── ini_driver.hpp     # INI file driver
│       │
│       ├── transaction/
│       │   ├── transaction.hpp        # Transaction manager
│       │   ├── savepoint.hpp          # Savepoint support
│       │   └── mvcc.hpp               # Multi-version concurrency
│       │
│       ├── backup/
│       │   ├── backup.hpp             # Backup manager (fluent)
│       │   ├── backup.cpp
│       │   ├── compression.hpp        # Compression utils
│       │   ├── encryption.hpp         # Encryption utils
│       │   └── cloud.hpp              # Cloud storage integration
│       │
│       ├── conversion/
│       │   ├── converter.hpp          # Type conversion
│       │   ├── unit.hpp               # Unit system
│       │   └── format.hpp             # Formatting utilities
│       │
│       └── utils/
│           ├── string_utils.hpp       # String helpers
│           ├── file_utils.hpp         # File operations
│           ├── time_utils.hpp         # Time utilities
│           └── pool.hpp               # Connection pool
```

│

```text
├── src/
│   ├── universal_db.cpp               # Main implementation
│   │
│   ├── core/
│   │   ├── types.cpp
│   │   ├── exceptions.cpp
│   │   └── constants.cpp
│   │
│   ├── streaming/
│   │   ├── record_stream.cpp
│   │   ├── vector_stream.cpp
│   │   ├── file_stream.cpp
│   │   └── parallel_stream.cpp
│   │
│   ├── query/
│   │   ├── query_builder.cpp
│   │   ├── result.cpp
│   │   └── expression.cpp
│   │
│   ├── table/
│   │   ├── table.cpp
│   │   ├── schema.cpp
│   │   ├── column.cpp
│   │   └── constraint.cpp
│   │
│   ├── index/
│   │   ├── index.cpp
│   │   ├── btree_index.cpp
│   │   ├── hash_index.cpp
│   │   └── fulltext_index.cpp
│   │
│   ├── drivers/
│   │   ├── driver_interface.cpp
│   │   ├── driver_factory.cpp
│   │   │
│   │   ├── memory/
│   │   │   └── memory_driver.cpp
│   │   │
│   │   ├── csv/
│   │   │   └── csv_driver.cpp
│   │   │
│   │   ├── sqlite/
│   │   │   └── sqlite_driver.cpp
│   │   │
│   │   ├── mysql/
│   │   │   └── mysql_driver.cpp
│   │   │
│   │   ├── postgres/
│   │   │   └── postgres_driver.cpp
│   │   │
│   │   ├── oracle/
│   │   │   └── oracle_driver.cpp
│   │   │
│   │   └── file_formats/
│   │       ├── json_driver.cpp
│   │       ├── xml_driver.cpp
│   │       ├── yaml_driver.cpp
│   │       └── ini_driver.cpp
│   │
│   ├── transaction/
│   │   ├── transaction.cpp
│   │   ├── savepoint.cpp
│   │   └── mvcc.cpp
│   │
│   ├── backup/
│   │   ├── backup.cpp
│   │   ├── compression.cpp
│   │   ├── encryption.cpp
│   │   └── cloud.cpp
│   │
│   ├── conversion/
│   │   ├── converter.cpp
│   │   ├── unit.cpp
│   │   └── format.cpp
│   │
│   └── utils/
│       ├── string_utils.cpp
│       ├── file_utils.cpp
│       ├── time_utils.cpp
│       └── pool.cpp
```

│

```text
├── examples/
│   ├── basic_usage.cpp                # Basic CRUD operations
│   ├── fluent_api.cpp                 # Fluent API examples
│   ├── csv_operations.cpp             # CSV loading/saving
│   ├── transactions.cpp               # Transaction examples
│   ├── backup_restore.cpp             # Backup/restore demo
│   ├── query_builder.cpp              # Query builder examples
│   ├── indexing.cpp                   # Index creation/search
│   ├── file_formats.cpp               # JSON/XML/YAML/INI
│   ├── database_transfer.cpp          # Transfer between DBs
│   ├── parallel_processing.cpp        # Parallel streams
│   └── unit_conversion.cpp            # Scientific units
```

│

```text
├── tests/
│   ├── unit/
│   │   ├── test_types.cpp
│   │   ├── test_streaming.cpp
│   │   ├── test_query.cpp
│   │   ├── test_table.cpp
│   │   ├── test_index.cpp
│   │   └── test_transaction.cpp
│   │
│   ├── integration/
│   │   ├── test_memory_driver.cpp
│   │   ├── test_csv_driver.cpp
│   │   ├── test_sqlite_driver.cpp
│   │   ├── test_mysql_driver.cpp
│   │   ├── test_postgres_driver.cpp
│   │   └── test_oracle_driver.cpp
│   │
│   └── benchmarks/
│       ├── bench_insert.cpp
│       ├── bench_query.cpp
│       ├── bench_streaming.cpp
│       └── bench_parallel.cpp
```

│

```text
├── tools/
│   ├── db_migrate.cpp                 # Database migration tool
│   ├── db_backup_cli.cpp              # Command-line backup
│   ├── csv_import.cpp                 # CSV import utility
│   └── schema_generator.cpp           # Schema code generator
```

│

```text
├── docs/
│   ├── api/
│   │   ├── database.md
│   │   ├── table.md
│   │   ├── query.md
│   │   ├── result.md
│   │   └── backup.md
│   │
│   ├── guides/
│   │   ├── getting_started.md
│   │   ├── fluent_api_guide.md
│   │   ├── migration_guide.md
│   │   └── best_practices.md
│   │
│   └── examples/
│       ├── example1_basic.cpp
│       ├── example2_csv.cpp
│       └── example3_transfer.cpp
```

│

```text
└── scripts/
    ├── build.sh
    ├── test.sh
    ├── install.sh
    └── generate_docs.sh
```

Root Directory Files

CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(UniversalDB VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TOOLS "Build command-line tools" ON)
option(USE_SYSTEM_LIBS "Use system libraries" OFF)

# Find dependencies
find_package(Threads REQUIRED)
if(USE_SYSTEM_LIBS)
    find_package(SQLite3 QUIET)
    find_package(PostgreSQL QUIET)
    find_package(MySQL QUIET)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Collect source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/universal_db/*.hpp")

# Create library
add_library(universal_db STATIC ${SOURCES} ${HEADERS})
target_include_directories(universal_db PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(universal_db Threads::Threads)

# Enable warnings
if(MSVC)
    target_compile_options(universal_db PRIVATE /W4)
else()
    target_compile_options(universal_db PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install
install(TARGETS universal_db DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
# Build examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Build tools
if(BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# CPack for packaging
set(CPACK_PACKAGE_NAME "UniversalDB")
set(CPACK_PACKAGE_VERSION "2.0.0")
set(CPACK_PACKAGE_DESCRIPTION "Universal Database Access Module")
include(CPack)
LICENSE
```

### MIT License

Copyright (c) 2024 UniversalDB Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy

of this software and associated documentation files (the "Software"), to deal

in the Software without restriction, including without limitation the rights

to use, copy, modify, merge, publish, distribute, sublicense, and/or sell

copies of the Software, and to permit persons to whom the Software is

furnished to do so, subject to the following conditions:

- The above copyright notice and this permission notice shall be included in all

copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR

IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,

FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE

AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER

LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,

OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE

SOFTWARE.

README.md

```markdown
# Universal Database Module (UDM) v2.0

A fluent, header-only C++17 library for unified database access with dot-based API.

## Features
```

- **Fluent API** - No more `->get_driver()`, just `db.table("users").select()`

- **Multiple Databases** - MySQL, PostgreSQL, Oracle, SQLite

- **File Formats** - CSV, JSON, XML, YAML, INI

- **In-Memory Database** - Fast, ACID-compliant

- **Streaming** - Process GB of data with minimal memory

- **Indexing** - B-tree, hash, full-text

- **Backup/Restore** - With compression and encryption

- **Transactions** - ACID with MVCC

- **Unit System** - Scientific unit conversion

\## Quick Start

```cpp

```markdown
#include <universal_db/universal_db.hpp>
using namespace udb;

int main() {
    // Create database
    Database db = udb::db("memory", "");

    // Create table and insert data
    db.table("users")
        .create({{"id", "INT"}, {"name", "TEXT"}})
        .insert({{"id", 1}, {"name", "Alice"}})
        .insert({{"id", 2}, {"name", "Bob"}});

    // Query with fluent API
    for (auto& row : db.table("users").select("*").execute()) {
        std::cout << vstr(row.at("name")) << "\n";
    }

    return 0;
}
Build
```

```bash
mkdir build && cd build
cmake ..
```

```make
sudo make install
Documentation
```

See docs/ for complete documentation.

License

MIT License

```text
---

## include/universal_db/universal_db.hpp

```cpp
// universal_db.hpp - Main unified header for fluent API
#pragma once

// Core
#include "universal_db/core/types.hpp"
#include "universal_db/core/exceptions.hpp"
#include "universal_db/core/constants.hpp"

// Streaming
#include "universal_db/streaming/record_stream.hpp"
#include "universal_db/streaming/vector_stream.hpp"
#include "universal_db/streaming/file_stream.hpp"
#include "universal_db/streaming/parallel_stream.hpp"

// Query
#include "universal_db/query/query_builder.hpp"
#include "universal_db/query/result.hpp"
#include "universal_db/query/expression.hpp"

// Table
#include "universal_db/table/table.hpp"
#include "universal_db/table/schema.hpp"
#include "universal_db/table/column.hpp"
#include "universal_db/table/constraint.hpp"

// Index
#include "universal_db/index/index.hpp"
#include "universal_db/index/btree_index.hpp"
#include "universal_db/index/hash_index.hpp"
#include "universal_db/index/fulltext_index.hpp"

// Drivers
#include "universal_db/drivers/driver_interface.hpp"
#include "universal_db/drivers/driver_factory.hpp"
#include "universal_db/drivers/memory/memory_driver.hpp"
#include "universal_db/drivers/csv/csv_driver.hpp"
#include "universal_db/drivers/sqlite/sqlite_driver.hpp"
#include "universal_db/drivers/mysql/mysql_driver.hpp"
#include "universal_db/drivers/postgres/postgres_driver.hpp"
#include "universal_db/drivers/oracle/oracle_driver.hpp"
#include "universal_db/drivers/file_formats/json_driver.hpp"
#include "universal_db/drivers/file_formats/xml_driver.hpp"
#include "universal_db/drivers/file_formats/yaml_driver.hpp"
#include "universal_db/drivers/file_formats/ini_driver.hpp"

// Transaction
#include "universal_db/transaction/transaction.hpp"
#include "universal_db/transaction/savepoint.hpp"
#include "universal_db/transaction/mvcc.hpp"

// Backup
#include "universal_db/backup/backup.hpp"
#include "universal_db/backup/compression.hpp"
#include "universal_db/backup/encryption.hpp"
#include "universal_db/backup/cloud.hpp"

// Conversion
#include "universal_db/conversion/converter.hpp"
#include "universal_db/conversion/unit.hpp"
#include "universal_db/conversion/format.hpp"

// Utils
#include "universal_db/utils/string_utils.hpp"
#include "universal_db/utils/file_utils.hpp"
#include "universal_db/utils/time_utils.hpp"
#include "universal_db/utils/pool.hpp"

namespace udb {

/**
```

- Global convenience function

- Creates and connects a database in one line

```text
 *
```

- Usage: auto db = udb::db("sqlite", "test.db");

```text
 */
inline Database db(const std::string& driver, const std::string& conn = "") {
    Database d(driver, conn);
    d.connect();
    return d;
}

/**
```

- Version information

```text
 */
inline std::string version() {
    return "2.0.0";
}

inline int version_major() { return 2; }
inline int version_minor() { return 0; }
inline int version_patch() { return 0; }
} // namespace udb
include/universal_db/core/types.hpp
```

```cpp
#pragma once

#include <variant>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <chrono>

namespace udb {

/**
```

- Universal value type - can hold any database value

```cpp
 */
using Value = std::variant<
    std::monostate,     // NULL
    int,                // 32-bit integer
    long long,          // 64-bit integer
    double,             // Floating point
    std::string,        // Text
    bool,               // Boolean
    std::vector<uint8_t> // Binary data (BLOB)
>;

/**
```

- Row - unordered map of column name to value

```cpp
 */
using Row = std::unordered_map<std::string, Value>;
using RowPtr = std::shared_ptr<Row>;
/**
```

- Row comparison for sorting

```cpp
 */
struct RowComparator {
    std::string column;
    bool ascending = true;

    bool operator()(const Row& a, const Row& b) const {
        auto it_a = a.find(column);
        auto it_b = b.find(column);

        if (it_a == a.end() && it_b == b.end()) return false;
        if (it_a == a.end()) return false;
        if (it_b == b.end()) return true;

        double va = value_to_double(it_a->second);
        double vb = value_to_double(it_b->second);

        return ascending ? va < vb : va > vb;
    }
};

/**
```

- Value conversion utilities

```cpp
 */
inline std::string value_to_string(const Value& v) {
    if (std::holds_alternative<std::monostate>(v)) return "NULL";
    if (std::holds_alternative<int>(v)) return std::to_string(std::get<int>(v));
    if (std::holds_alternative<long long>(v)) return std::to_string(std::get<long long>(v));
    if (std::holds_alternative<double>(v)) {
        std::string s = std::to_string(std::get<double>(v));
        // Remove trailing zeros
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    }
    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? "true" : "false";
    if (std::holds_alternative<std::vector<uint8_t>>(v)) return "[BLOB]";
    return "";
}

inline double value_to_double(const Value& v) {
    if (std::holds_alternative<int>(v)) return static_cast<double>(std::get<int>(v));
    if (std::holds_alternative<long long>(v)) return static_cast<double>(std::get<long long>(v));
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
    if (std::holds_alternative<std::string>(v)) {
        try {
            return std::stod(std::get<std::string>(v));
        } catch (...) {
            return 0.0;
        }
    }
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? 1.0 : 0.0;
    return 0.0;
}

inline long long value_to_int(const Value& v) {
    if (std::holds_alternative<int>(v)) return std::get<int>(v);
    if (std::holds_alternative<long long>(v)) return std::get<long long>(v);
    if (std::holds_alternative<double>(v)) return static_cast<long long>(std::get<double>(v));
    if (std::holds_alternative<std::string>(v)) {
        try {
            return std::stoll(std::get<std::string>(v));
        } catch (...) {
            return 0;
        }
    }
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? 1 : 0;
    return 0;
}

inline bool value_to_bool(const Value& v) {
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v);
    if (std::holds_alternative<int>(v)) return std::get<int>(v) != 0;
    if (std::holds_alternative<long long>(v)) return std::get<long long>(v) != 0;
    if (std::holds_alternative<double>(v)) return std::get<double>(v) != 0.0;
    if (std::holds_alternative<std::string>(v)) {
        std::string s = std::get<std::string>(v);
        return s == "true" || s == "1" || s == "yes" || s == "on";
    }
    return false;
}

/**
```

- Type information for schema

```cpp
 */
enum class DataType {
    UNKNOWN,
    INTEGER,
    BIGINT,
    REAL,
    TEXT,
    BLOB,
    BOOLEAN,
    DATE,
    TIME,
    TIMESTAMP,
```

```json
};

inline DataType string_to_type(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "int" || lower == "integer") return DataType::INTEGER;
    if (lower == "bigint" || lower == "long") return DataType::BIGINT;
    if (lower == "real" || lower == "float" || lower == "double") return DataType::REAL;
    if (lower == "text" || lower == "string" || lower == "varchar") return DataType::TEXT;
    if (lower == "blob" || lower == "binary") return DataType::BLOB;
    if (lower == "bool" || lower == "boolean") return DataType::BOOLEAN;
    if (lower == "date") return DataType::DATE;
    if (lower == "time") return DataType::TIME;
    if (lower == "timestamp" || lower == "datetime") return DataType::TIMESTAMP;
    if (lower == "json") return DataType::JSON;

    return DataType::UNKNOWN;
}

} // namespace udb
include/universal_db/core/exceptions.hpp
```

```cpp
#pragma once

#include <stdexcept>
#include <string>

namespace udb {

/**
```

- Base exception for all database errors

```cpp
 */
class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& msg) : std::runtime_error(msg) {}
    explicit DatabaseException(const char* msg) : std::runtime_error(msg) {}
};

/**
```

- Connection-related errors

```cpp
 */
class ConnectionException : public DatabaseException {
public:

    explicit ConnectionException(const std::string& msg) : DatabaseException("Connection: " + msg) {}
};

/**
```

- Query execution errors

```cpp
 */
class QueryException : public DatabaseException {
private:
    std::string sql;
public:
    QueryException(const std::string& msg, const std::string& query = "")
        : DatabaseException("Query: " + msg), sql(query) {}

    std::string get_sql() const { return sql; }
};

/**
```

- Schema-related errors

```cpp
 */
class SchemaException : public DatabaseException {
public:
    explicit SchemaException(const std::string& msg) : DatabaseException("Schema: " + msg) {}
};

/**
```

- Constraint violation errors

```cpp
 */
class ConstraintException : public DatabaseException {
public:

    explicit ConstraintException(const std::string& msg) : DatabaseException("Constraint: " + msg) {}
};

/**
```

- Transaction errors

```cpp
 */
class TransactionException : public DatabaseException {
public:

    explicit TransactionException(const std::string& msg) : DatabaseException("Transaction: " + msg) {}
};

/**
```

- Backup/Restore errors

```cpp
 */
class BackupException : public DatabaseException {
public:
    explicit BackupException(const std::string& msg) : DatabaseException("Backup: " + msg) {}
};

/**
```

- Driver not found error

```cpp
 */
class DriverNotFoundException : public DatabaseException {
public:
    explicit DriverNotFoundException(const std::string& driver)
        : DatabaseException("Driver not found: " + driver) {}
};

/**
```

- Table not found error

```cpp
 */
class TableNotFoundException : public DatabaseException {
public:
    explicit TableNotFoundException(const std::string& table)
        : DatabaseException("Table not found: " + table) {}
};

/**
```

- Column not found error

```cpp
 */
class ColumnNotFoundException : public DatabaseException {
public:
    explicit ColumnNotFoundException(const std::string& column)
        : DatabaseException("Column not found: " + column) {}
};

} // namespace udb
include/universal_db/core/constants.hpp
```

```cpp
#pragma once

#include <string>
#include <unordered_map>

namespace udb {
/**
```

- Driver type constants

```cpp
 */
namespace Drivers {
    constexpr const char* MEMORY = "memory";
    constexpr const char* CSV = "csv";
    constexpr const char* SQLITE = "sqlite";
    constexpr const char* MYSQL = "mysql";
    constexpr const char* POSTGRESQL = "postgresql";
    constexpr const char* ORACLE = "oracle";
    constexpr const char* JSON = "json";
    constexpr const char* XML = "xml";
    constexpr const char* YAML = "yaml";
    constexpr const char* INI = "ini";
}

/**
```

- SQL keywords

```cpp
 */
namespace SQL {
    constexpr const char* SELECT = "SELECT";
    constexpr const char* INSERT = "INSERT";
    constexpr const char* UPDATE = "UPDATE";
    constexpr const char* DELETE = "DELETE";
    constexpr const char* FROM = "FROM";
    constexpr const char* WHERE = "WHERE";
    constexpr const char* ORDER_BY = "ORDER BY";
    constexpr const char* GROUP_BY = "GROUP BY";
    constexpr const char* LIMIT = "LIMIT";
    constexpr const char* OFFSET = "OFFSET";
    constexpr const char* JOIN = "JOIN";
    constexpr const char* ON = "ON";
    constexpr const char* AND = "AND";
    constexpr const char* OR = "OR";
    constexpr const char* ASC = "ASC";
    constexpr const char* DESC = "DESC";
}

/**
```

- Default configuration values

```cpp
 */
namespace Defaults {
    constexpr size_t CACHE_SIZE = 10000;
    constexpr size_t BATCH_SIZE = 1000;
    constexpr size_t CONNECTION_POOL_SIZE = 10;
    constexpr int COMPRESSION_LEVEL = 6;
    constexpr int QUERY_TIMEOUT_SECONDS = 30;
}

/**
```

- Error codes

```cpp
 */
enum class ErrorCode {
    SUCCESS = 0,
    CONNECTION_FAILED = 1001,
    QUERY_FAILED = 1002,
    TABLE_NOT_FOUND = 1003,
    COLUMN_NOT_FOUND = 1004,
    CONSTRAINT_VIOLATION = 1005,
    DUPLICATE_KEY = 1006,
    TRANSACTION_FAILED = 1007,
    BACKUP_FAILED = 1008,
    RESTORE_FAILED = 1009,
    DRIVER_NOT_FOUND = 1010
};
/**
```

- Log levels

```cpp
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    NONE
};

} // namespace udb
include/universal_db/streaming/record_stream.hpp
```

```cpp
#pragma once

#include "../core/types.hpp"
#include <memory>

namespace udb {

/**
```

- RecordStream - Pull-based streaming interface

```cpp
 *
```

- Allows processing of large datasets without loading all rows into memory.

```cpp
 *
```

- Usage:

- auto stream = db.query("SELECT * FROM large_table");

- Row row;

- while (stream->next(row)) {

- process(row);

- }

```cpp
 */
class RecordStream {
public:
    virtual ~RecordStream() = default;

    /**
```

- Get the next row from the stream

- @param out Reference to Row that will be filled with the next row

- @return true if a row was retrieved, false if end of stream

```cpp
     */
    virtual bool next(Row& out) = 0;

    /**
```

- Reset the stream to the beginning

```cpp
     */
    virtual void reset() = 0;

    /**
```

- Estimated number of rows (may be 0 if unknown)

```cpp
     */
    virtual size_t size_hint() const { return 0; }

    /**
```

- Get column names (if known)

```cpp
     */
    virtual std::vector<std::string> columns() const { return {}; }

    /**
```

- Skip a number of rows

- @param n Number of rows to skip

- @return Number of rows actually skipped

```cpp
     */
    virtual size_t skip(size_t n) {
        size_t skipped = 0;
        Row dummy;
        while (skipped < n && next(dummy)) {
            skipped++;
        }
        return skipped;
    }
};

/**
```

- Stream iterator for range-based for loops

```cpp
 */
class StreamIterator {
    std::unique_ptr<RecordStream> stream;
    Row current;
    bool has_current;

public:
    StreamIterator(std::unique_ptr<RecordStream> s)
        : stream(std::move(s)), has_current(false) {
        ++(*this);
    }

    StreamIterator& operator++() {
        has_current = stream->next(current);
        return *this;
    }

    const Row& operator*() const { return current; }
    const Row* operator->() const { return &current; }

    bool operator!=(const StreamIterator& other) const {
        return has_current != other.has_current;
    }

    static StreamIterator end() {
        StreamIterator it(nullptr);
        it.has_current = false;
        return it;
    }

private:
    StreamIterator(std::unique_ptr<RecordStream> s) : stream(std::move(s)), has_current(false) {}
};

} // namespace udb
include/universal_db/streaming/vector_stream.hpp
```

```cpp
#pragma once

#include "record_stream.hpp"
#include <vector>

namespace udb {

/**
```

- VectorStream - Stream backed by a vector of rows

```cpp
 *
```

- Useful for testing or for small datasets.

```cpp
 */
class VectorStream : public RecordStream {
    std::vector<Row> data;
    size_t position = 0;
    std::vector<std::string> column_names;

public:
    VectorStream() = default;

    explicit VectorStream(std::vector<Row> rows)
        : data(std::move(rows)) {
        if (!data.empty()) {
            for (const auto& [key, _] : data[0]) {
                column_names.push_back(key);
            }
        }
    }

    VectorStream(std::vector<Row> rows, std::vector<std::string> cols)
        : data(std::move(rows)), column_names(std::move(cols)) {}

    bool next(Row& out) override {
        if (position >= data.size()) return false;
        out = data[position++];
        return true;
    }

    void reset() override {
        position = 0;
    }

    size_t size_hint() const override {
        return data.size();
    }

    std::vector<std::string> columns() const override {
        return column_names;
    }

    void add_row(const Row& row) {
        data.push_back(row);
    }

    void clear() {
        data.clear();
        position = 0;
    }
};

} // namespace udb
include/universal_db/streaming/file_stream.hpp
```

```cpp
#pragma once

#include "record_stream.hpp"
#include <fstream>
#include <sstream>

namespace udb {

/**
```

- FileStream - Stream CSV/TSV files directly

```cpp
 */
class FileStream : public RecordStream {
    std::ifstream file;
    std::vector<std::string> headers;
    std::string delimiter;
    size_t line_number = 0;
    size_t total_lines = 0;
    bool has_header;

    std::vector<std::string> split(const std::string& line) {
        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, delimiter[0])) {
            tokens.push_back(token);
        }
        return tokens;
    }

public:
    FileStream(const std::string& path, const std::string& delim = ",", bool header = true)
        : delimiter(delim), has_header(header) {
        file.open(path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + path);
        }

        // Read header if present
        std::string line;
        if (has_header && std::getline(file, line)) {
            headers = split(line);
        }

        // Count lines for hint
        std::streampos pos = file.tellg();
        while (std::getline(file, line)) {
            total_lines++;
        }
        file.clear();
        file.seekg(has_header ? std::streampos(std::getline(file, line) ? file.tellg() : 0) : 0);
    }

    ~FileStream() {
        if (file.is_open()) file.close();
    }

    bool next(Row& out) override {
        std::string line;
        if (!std::getline(file, line)) return false;

        auto tokens = split(line);

        if (has_header && !headers.empty()) {
            for (size_t i = 0; i < headers.size() && i < tokens.size(); i++) {
                out[headers[i]] = tokens[i];
            }
        } else {
            for (size_t i = 0; i < tokens.size(); i++) {
                out[std::to_string(i)] = tokens[i];
            }
        }

        line_number++;
        return true;
    }

    void reset() override {
        file.clear();
        file.seekg(0);
        line_number = 0;

        if (has_header) {
            std::string line;
            std::getline(file, line);
        }
    }
    size_t size_hint() const override {
        return total_lines;
    }

    std::vector<std::string> columns() const override {
        return headers;
    }
};

} // namespace udb
include/universal_db/streaming/parallel_stream.hpp
```

```cpp
#pragma once

#include "record_stream.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

namespace udb {

/**
```

- ParallelStream - Thread-safe stream with prefetching

```cpp
 *
```

- Reads ahead in a background thread to improve performance.

```cpp
 */
class ParallelStream : public RecordStream {
    std::unique_ptr<RecordStream> source;
    std::queue<Row> buffer;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> done{false};
    std::thread worker;
    size_t max_buffer_size;

public:
    ParallelStream(std::unique_ptr<RecordStream> src, size_t buffer_size = 1000)
        : source(std::move(src)), max_buffer_size(buffer_size) {
        worker = std::thread([this]() {
            Row row;
            while (source->next(row)) {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this]() { return buffer.size() < max_buffer_size; });
                buffer.push(std::move(row));
                cv.notify_one();
            }
            done = true;
            cv.notify_one();
        });
    }

    ~ParallelStream() {
        if (worker.joinable()) {
            worker.join();
        }
    }

    bool next(Row& out) override {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !buffer.empty() || done; });

        if (buffer.empty()) return false;

        out = std::move(buffer.front());
        buffer.pop();
        cv.notify_one();
        return true;
    }

    void reset() override {
        std::lock_guard<std::mutex> lock(mtx);
        while (!buffer.empty()) buffer.pop();
        source->reset();
        done = false;
    }

    size_t size_hint() const override {
        return source->size_hint();
    }

    std::vector<std::string> columns() const override {
        return source->columns();
    }
};

/**
```

- TransformStream - Apply transformation to each row

```cpp
 */
template<typename TransformFunc>
class TransformStream : public RecordStream {
    std::unique_ptr<RecordStream> source;
    TransformFunc transform;

public:
    TransformStream(std::unique_ptr<RecordStream> src, TransformFunc fn)
        : source(std::move(src)), transform(std::move(fn)) {}
    bool next(Row& out) override {
        if (!source->next(out)) return false;
        transform(out);
        return true;
    }

    void reset() override {
        source->reset();
    }

    size_t size_hint() const override {
        return source->size_hint();
    }

    std::vector<std::string> columns() const override {
        return source->columns();
    }
};

} // namespace udb
include/universal_db/query/query_builder.hpp
```

```cpp
#pragma once

#include "../core/types.hpp"
#include "../core/exceptions.hpp"
#include <string>
#include <vector>
#include <sstream>

namespace udb {

/**
```

- QueryBuilder - Fluent SQL query builder

```cpp
 *
```

- Usage:

- auto query = QueryBuilder()

- .select("name", "age")

- .from("users")

- .where("age > 18")

- .order_by("name", "ASC")

- .limit(10)

- .build();

```cpp
 */
class QueryBuilder {
    enum class QueryType { SELECT, INSERT, UPDATE, DELETE };

    QueryType type = QueryType::SELECT;
    std::vector<std::string> select_columns;
    std::string from_table;
    std::string where_clause;
    std::string order_by_column;
    std::string order_direction = "ASC";
    int limit_value = -1;
    int offset_value = 0;
    std::vector<std::string> group_by_columns;
    std::string having_clause;
    std::vector<std::pair<std::string, std::string>> join_clauses;

    // INSERT-specific
    std::vector<std::string> insert_columns;
    std::vector<std::vector<Value>> insert_values;

    // UPDATE-specific
    std::vector<std::pair<std::string, Value>> update_set;
public:
    QueryBuilder() = default;

    // SELECT methods
    QueryBuilder& select() {
        type = QueryType::SELECT;
        return *this;
    }

    QueryBuilder& select(const std::string& columns) {
        type = QueryType::SELECT;
        select_columns.clear();

        size_t pos = 0, start = 0;
        while ((pos = columns.find(',', start)) != std::string::npos) {
            std::string col = columns.substr(start, pos - start);
            col.erase(0, col.find_first_not_of(" \t"));
            col.erase(col.find_last_not_of(" \t") + 1);
            select_columns.push_back(col);
            start = pos + 1;
        }
        std::string last = columns.substr(start);
        last.erase(0, last.find_first_not_of(" \t"));
        last.erase(last.find_last_not_of(" \t") + 1);
        select_columns.push_back(last);

        return *this;
    }

    QueryBuilder& select(const std::vector<std::string>& columns) {
        type = QueryType::SELECT;
        select_columns = columns;
        return *this;
    }

    QueryBuilder& from(const std::string& table) {
        from_table = table;
        return *this;
    }

    QueryBuilder& where(const std::string& condition) {
        where_clause = condition;
        return *this;
    }

    QueryBuilder& order_by(const std::string& column, const std::string& dir = "ASC") {
        order_by_column = column;
        order_direction = dir;
        return *this;
    }

    QueryBuilder& limit(int n) {
        limit_value = n;
        return *this;
    }

    QueryBuilder& offset(int n) {
        offset_value = n;
        return *this;
    }

    QueryBuilder& group_by(const std::string& columns) {
        group_by_columns.clear();
        size_t pos = 0, start = 0;
        while ((pos = columns.find(',', start)) != std::string::npos) {
            group_by_columns.push_back(columns.substr(start, pos - start));
            start = pos + 1;
        }
        group_by_columns.push_back(columns.substr(start));
        return *this;
    }

    QueryBuilder& having(const std::string& condition) {
        having_clause = condition;
        return *this;
    }

    QueryBuilder& join(const std::string& table, const std::string& on, const std::string& type = "INNER") {
        join_clauses.push_back({type, table + " ON " + on});
        return *this;
    }

    // INSERT methods
    QueryBuilder& insert_into(const std::string& table) {
        type = QueryType::INSERT;
        from_table = table;
        return *this;
    }

    QueryBuilder& columns(const std::vector<std::string>& cols) {
        insert_columns = cols;
        return *this;
    }

    QueryBuilder& values(const std::vector<Value>& vals) {
        insert_values.push_back(vals);
        return *this;
    }
    QueryBuilder& value(const Row& row) {
        std::vector<Value> vals;
        for (const auto& [_, val] : row) {
            vals.push_back(val);
        }
        insert_values.push_back(vals);
        return *this;
    }

    // UPDATE methods
    QueryBuilder& update(const std::string& table) {
        type = QueryType::UPDATE;
        from_table = table;
        return *this;
    }

    QueryBuilder& set(const std::string& column, const Value& value) {
        update_set.push_back({column, value});
        return *this;
    }

    QueryBuilder& set(const Row& row) {
        for (const auto& [col, val] : row) {
            update_set.push_back({col, val});
        }
        return *this;
    }

    // DELETE methods
    QueryBuilder& delete_from(const std::string& table) {
        type = QueryType::DELETE;
        from_table = table;
        return *this;
    }

    // Build SQL string
    std::string build() const {
        std::stringstream sql;

        switch (type) {
            case QueryType::SELECT:
                sql << "SELECT ";
                if (select_columns.empty()) {
                    sql << "*";
                } else {
                    for (size_t i = 0; i < select_columns.size(); i++) {
                        if (i > 0) sql << ", ";
                        sql << select_columns[i];
                    }
                }
                sql << " FROM " << from_table;

                for (const auto& [type, clause] : join_clauses) {
                    sql << " " << type << " JOIN " << clause;
                }

                if (!where_clause.empty()) {
                    sql << " WHERE " << where_clause;
                }

                if (!group_by_columns.empty()) {
                    sql << " GROUP BY ";
                    for (size_t i = 0; i < group_by_columns.size(); i++) {
                        if (i > 0) sql << ", ";
                        sql << group_by_columns[i];
                    }
                }

                if (!having_clause.empty()) {
                    sql << " HAVING " << having_clause;
                }

                if (!order_by_column.empty()) {
                    sql << " ORDER BY " << order_by_column << " " << order_direction;
                }

                if (limit_value > 0) {
                    sql << " LIMIT " << limit_value;
                }

                if (offset_value > 0) {
                    sql << " OFFSET " << offset_value;
                }
                break;

            case QueryType::INSERT:
                sql << "INSERT INTO " << from_table;
                if (!insert_columns.empty()) {
                    sql << " (";
                    for (size_t i = 0; i < insert_columns.size(); i++) {
                        if (i > 0) sql << ", ";
                        sql << insert_columns[i];
                    }
                    sql << ")";
                }
                sql << " VALUES ";
                for (size_t i = 0; i < insert_values.size(); i++) {
                    if (i > 0) sql << ", ";
                    sql << "(";
                    for (size_t j = 0; j < insert_values[i].size(); j++) {
                        if (j > 0) sql << ", ";
                        sql << "?";
                    }
                    sql << ")";
                }
                break;

            case QueryType::UPDATE:
                sql << "UPDATE " << from_table << " SET ";
                for (size_t i = 0; i < update_set.size(); i++) {
                    if (i > 0) sql << ", ";
                    sql << update_set[i].first << " = ?";
                }
                if (!where_clause.empty()) {
                    sql << " WHERE " << where_clause;
                }
                break;

            case QueryType::DELETE:
                sql << "DELETE FROM " << from_table;
                if (!where_clause.empty()) {
                    sql << " WHERE " << where_clause;
                }
                break;
        }

        return sql.str();
    }

    // Clear all clauses
    void clear() {
        type = QueryType::SELECT;
        select_columns.clear();
        from_table.clear();
        where_clause.clear();
        order_by_column.clear();
        order_direction = "ASC";
        limit_value = -1;
        offset_value = 0;
        group_by_columns.clear();
        having_clause.clear();
        join_clauses.clear();
        insert_columns.clear();
        insert_values.clear();
        update_set.clear();
    }
};

} // namespace udb
include/universal_db/query/result.hpp
```

```cpp
#pragma once

#include "../streaming/record_stream.hpp"
#include <vector>
#include <iterator>

namespace udb {

/**
```

- Result - Wrapper around RecordStream with convenience methods

```cpp
 *
```

- Supports:

- - Range-based for loops

- - Getting all rows as vector

- - Counting rows

- - Getting first row

```cpp
 */
class Result {
    std::unique_ptr<RecordStream> stream;

public:
    Result() = default;

    explicit Result(std::unique_ptr<RecordStream> s) : stream(std::move(s)) {}

    Result(Result&&) = default;
    Result& operator=(Result&&) = default;

    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    /**
```

- Get iterator for range-based for loop

```cpp
     */
    StreamIterator begin() {
        return StreamIterator(std::move(stream));
    }

    StreamIterator end() {
        return StreamIterator::end();
    }

    /**
```

- Get all rows as a vector (use only for small datasets)

```cpp
     */
    std::vector<Row> all() {
        std::vector<Row> rows;
        Row row;
        while (stream && stream->next(row)) {
            rows.push_back(std::move(row));
        }
        return rows;
    }

    /**
```

- Count rows in result (consumes the stream)

```cpp
     */
    size_t count() {
        size_t cnt = 0;
        Row dummy;
        while (stream && stream->next(dummy)) {
            cnt++;
        }
        return cnt;
    }

    /**
```

- Get first row only

```cpp
     */
    Row first() {
        Row row;
        if (stream && stream->next(row)) {
            return row;
        }
        return {};
    }

    /**
```

- Check if result is empty

```cpp
     */
    bool empty() {
        return count() == 0;
    }

    /**
```

- Check if stream is valid

```cpp
     */
    explicit operator bool() const {
        return stream != nullptr;
    }

    /**
```

- Get underlying stream

```cpp
     */
    RecordStream* get_stream() const {
        return stream.get();
    }

    /**
```

- Release ownership of stream

```cpp
     */
    std::unique_ptr<RecordStream> release() {
        return std::move(stream);
    }
};

} // namespace udb
include/universal_db/query/expression.hpp
```

```cpp
#pragma once

#include "../core/types.hpp"
#include <functional>
#include <cmath>
#include <regex>

namespace udb {

/**
```

- Expression evaluator for WHERE clauses

```cpp
 */
class Expression {
public:
    virtual ~Expression() = default;
    virtual bool evaluate(const Row& row) const = 0;
    virtual std::string to_string() const = 0;
};

/**
```

- Column comparison expression: column op value

```cpp
 */
class CompareExpression : public Expression {
    std::string column;
    std::string op;
    Value value;

    bool compare(const Value& left, const Value& right) const {
        if (op == "=" || op == "==") return value_to_string(left) == value_to_string(right);
        if (op == "!=" || op == "<>") return value_to_string(left) != value_to_string(right);
        if (op == ">") return value_to_double(left) > value_to_double(right);
        if (op == ">=") return value_to_double(left) >= value_to_double(right);
        if (op == "<") return value_to_double(left) < value_to_double(right);
        if (op == "<=") return value_to_double(left) <= value_to_double(right);
        if (op == "LIKE") {
            std::string pattern = value_to_string(right);
            std::regex regex("^" + std::regex_replace(pattern, std::regex("%"), ".*") + "$");
            return std::regex_match(value_to_string(left), regex);
        }
        return false;
    }

public:
    CompareExpression(const std::string& col, const std::string& oper, const Value& val)
        : column(col), op(oper), value(val) {}

    bool evaluate(const Row& row) const override {
        auto it = row.find(column);
        if (it == row.end()) return false;
        return compare(it->second, value);
    }

    std::string to_string() const override {
        return column + " " + op + " " + value_to_string(value);
    }
};

/**
```

- AND expression: left AND right

```cpp
 */
class AndExpression : public Expression {
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

public:
    AndExpression(std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : left(std::move(l)), right(std::move(r)) {}

    bool evaluate(const Row& row) const override {
        return left->evaluate(row) && right->evaluate(row);
    }

    std::string to_string() const override {
        return "(" + left->to_string() + " AND " + right->to_string() + ")";
    }
};

/**
```

- OR expression: left OR right

```cpp
 */
class OrExpression : public Expression {
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

public:
    OrExpression(std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : left(std::move(l)), right(std::move(r)) {}

    bool evaluate(const Row& row) const override {
        return left->evaluate(row) || right->evaluate(row);
    }

    std::string to_string() const override {
        return "(" + left->to_string() + " OR " + right->to_string() + ")";
    }
};

/**
```

- NOT expression: NOT expr

```cpp
 */
class NotExpression : public Expression {
    std::unique_ptr<Expression> expr;
public:
    explicit NotExpression(std::unique_ptr<Expression> e) : expr(std::move(e)) {}

    bool evaluate(const Row& row) const override {
        return !expr->evaluate(row);
    }

    std::string to_string() const override {
        return "NOT (" + expr->to_string() + ")";
    }
};

/**
```

- Expression parser

```cpp
 */
class ExpressionParser {
public:
    static std::unique_ptr<Expression> parse(const std::string& condition) {
        // Simple parser for "column = value" format
        std::regex re(R"((\w+)\s*(=|!=|<>|>|<|>=|<=|LIKE)\s*(.+))");
        std::smatch match;

        if (std::regex_match(condition, match, re)) {
            std::string col = match[1];
            std::string op = match[2];
            std::string val = match[3];

            // Remove quotes from string values
            if (val.front() == '\'' && val.back() == '\'') {
                val = val.substr(1, val.length() - 2);
            }
            // Try to parse as number
            try {
                double dval = std::stod(val);
                return std::make_unique<CompareExpression>(col, op, dval);
            } catch (...) {
                return std::make_unique<CompareExpression>(col, op, val);
            }
        }

        return nullptr;
    }
};

} // namespace udb
include/universal_db/table/table.hpp
```

```cpp
#pragma once

#include "../core/types.hpp"
#include "../core/exceptions.hpp"
#include "../query/result.hpp"
#include "../query/query_builder.hpp"
#include "../streaming/vector_stream.hpp"
#include "schema.hpp"
#include <memory>
#include <unordered_map>

namespace udb {

// Forward declarations
class Database;

/**
```

- Table - Fluent table operations

```cpp
 *
```

- Provides a fluent interface for database table operations.

```cpp
 *
```

- Usage:

- db.table("users")

- .create({{"id", "INT"}, {"name", "TEXT"}})

- .insert({{"id", 1}, {"name", "Alice"}})

- .where("id = 1")

- .update({{"name", "Bob"}});

```cpp
 */
class Table {
    std::string name;
    Database* db;

    // Query state for chaining
    mutable std::string pending_where;
    mutable std::string pending_order;
    mutable std::string pending_order_dir;
    mutable int pending_limit = -1;
    mutable int pending_offset = 0;
    mutable std::vector<std::string> pending_select;

public:
    Table(const std::string& n, Database* d) : name(n), db(d) {}

    /**
```

- Get table name

```cpp
     */
    const std::string& get_name() const { return name; }

    /**
```

- Create table with schema

```cpp
     */
    Table& create(const std::vector<std::pair<std::string, std::string>>& columns);

    /**
```

- Create table if not exists

```cpp
     */
    Table& create_if_not_exists(const std::vector<std::pair<std::string, std::string>>& columns);

    /**
```

- Drop table

```cpp
     */
    Table& drop();

    /**
```

- Drop table if exists

```cpp
     */
    Table& drop_if_exists();

    /**
```

- Truncate table (remove all rows)

```cpp
     */
    Table& truncate();

    /**
```

- Insert a single row

```cpp
     */
    Table& insert(const Row& row);

    /**
```

- Insert multiple rows

```cpp
     */
    Table& insert(const std::vector<Row>& rows);
    /**
```

- Insert or update (upsert)

```cpp
     */
    Table& upsert(const Row& row, const std::string& key_column);

    /**
```

- Update rows matching condition

```cpp
     */
    Table& update(const Row& updates, const std::string& where = "");

    /**
```

- Remove rows matching condition

```cpp
     */
    Table& remove(const std::string& where = "");

    /**
```

- Remove by primary key

```cpp
     */
    Table& remove_by_id(const Value& id);

    /**
```

- Select query

```cpp
     */
    Result select(const std::string& columns = "*");

    /**
```

- Select all rows

```cpp
     */
    Result all() { return select("*"); }

    /**
```

- Find by primary key

```cpp
     */
    Row find(const Value& id);

    /**
```

- Find by column value

```cpp
     */
    Result find_by(const std::string& column, const Value& value);

    /**
```

- Get first row

```cpp
     */
    Row first();

    /**
```

- Count rows

```cpp
     */
    size_t count();

    /**
```

- Count with condition

```cpp
     */
    size_t count(const std::string& where);

    /**
```

- Check if table exists

```cpp
     */
    bool exists();

    /**
```

- Check if row exists

```cpp
     */
    bool exists(const std::string& where);

    /**
```

- Add column

```cpp
     */
    Table& add_column(const std::string& column, const std::string& type);

    /**
```

- Drop column

```cpp
     */
    Table& drop_column(const std::string& column);

    /**
```

- Rename column

```cpp
     */
    Table& rename_column(const std::string& old_name, const std::string& new_name);

    /**
```

- Create index

```cpp
     */
    Table& create_index(const std::string& column, bool unique = false);

    /**
```

- Create composite index

```cpp
     */
    Table& create_composite_index(const std::vector<std::string>& columns, bool unique = false);

    /**
```

- Drop index

```cpp
     */
    Table& drop_index(const std::string& column);

    /**
```

- Get table schema

```cpp
     */
    Schema get_schema();
    /**
```

- Get column names

```cpp
     */
    std::vector<std::string> get_columns();

    /**
```

- Query building (fluent)

```cpp
     */
    Table& where(const std::string& condition);
    Table& order_by(const std::string& column, const std::string& dir = "ASC");
    Table& limit(int n);
    Table& offset(int n);
    Table& select(const std::vector<std::string>& columns);

    /**
```

- Execute built query

```cpp
     */
    Result execute();

    /**
```

- Get results as vector

```cpp
     */
    std::vector<Row> get();

    // Batch operations
    class BatchInserter {
        Table& table;
        std::vector<Row> batch;
        size_t batch_size;

    public:
        BatchInserter(Table& t, size_t size = 1000) : table(t), batch_size(size) {}
        BatchInserter& add(const Row& row) {
            batch.push_back(row);
            if (batch.size() >= batch_size) {
                flush();
            }
            return *this;
        }

        void flush() {
            if (!batch.empty()) {
                table.insert(batch);
                batch.clear();
            }
        }

        ~BatchInserter() {
            flush();
        }
    };

    BatchInserter batch(size_t size = 1000) {
        return BatchInserter(*this, size);
    }
};

} // namespace udb
include/universal_db/table/schema.hpp
```

```cpp
#pragma once

#include "../core/types.hpp"
#include "column.hpp"
#include "constraint.hpp"
#include <string>
#include <vector>

namespace udb {

/**
```

- Schema - Table schema definition

```cpp
 */
class Schema {
    std::string table_name;
    std::vector<Column> columns;
    std::vector<Constraint> constraints;
    std::unordered_map<std::string, size_t> column_index;

public:
    Schema() = default;
    explicit Schema(const std::string& name) : table_name(name) {}

    const std::string& get_table_name() const { return table_name; }

    Schema& add_column(const Column& column) {
        column_index[column.name] = columns.size();
        columns.push_back(column);
        return *this;
    }

    Schema& add_constraint(const Constraint& constraint) {
        constraints.push_back(constraint);
        return *this;
    }

    const std::vector<Column>& get_columns() const { return columns; }
    const std::vector<Constraint>& get_constraints() const { return constraints; }

    bool has_column(const std::string& name) const {
        return column_index.find(name) != column_index.end();
    }

    const Column& get_column(const std::string& name) const {
        auto it = column_index.find(name);
        if (it == column_index.end()) {
            throw ColumnNotFoundException(name);
        }
        return columns[it->second];
    }

    size_t get_column_count() const { return columns.size(); }

    std::vector<std::string> get_column_names() const {
        std::vector<std::string> names;
        for (const auto& col : columns) {
            names.push_back(col.name);
        }
        return names;
    }

    std::string to_ddl() const {
        std::string sql = "CREATE TABLE " + table_name + " (\n";
        for (size_t i = 0; i < columns.size(); i++) {
            if (i > 0) sql += ",\n";
            sql += "  " + columns[i].to_ddl();
        }
        for (const auto& constraint : constraints) {
            sql += ",\n  " + constraint.to_sql();
        }
        sql += "\n)";
        return sql;
    }
};

} // namespace udb
include/universal_db/table/column.hpp
```

```cpp
#pragma once

#include "../core/types.hpp"
#include <string>
#include <optional>

namespace udb {

/**
```

- Column definition

```cpp
 */
class Column {
public:
    std::string name;
    DataType type;
    bool nullable = true;
    bool primary_key = false;
    bool auto_increment = false;
    bool unique = false;
    std::optional<Value> default_value;
    int length = 0;  // For VARCHAR
    int precision = 0;  // For DECIMAL
    int scale = 0;  // For DECIMAL

    Column() = default;
    Column(const std::string& n, DataType t) : name(n), type(t) {}

    static Column integer(const std::string& name, bool pk = false) {
        Column col(name, DataType::INTEGER);
        if (pk) col.primary_key = true;
        return col;
    }

    static Column text(const std::string& name, int len = 0) {
        Column col(name, DataType::TEXT);
        col.length = len;
        return col;
    }

    static Column real(const std::string& name) {
        return Column(name, DataType::REAL);
    }

    static Column blob(const std::string& name) {
        return Column(name, DataType::BLOB);
    }

    static Column boolean(const std::string& name) {
        return Column(name, DataType::BOOLEAN);
    }

    Column& not_null() {
        nullable = false;
        return *this;
    }

    Column& with_default(const Value& val) {
        default_value = val;
        return *this;
    }

    Column& with_autoincrement() {
        auto_increment = true;
        return *this;
    }

    Column& with_unique() {
        unique = true;
        return *this;
    }

    std::string to_ddl() const {
        std::string sql = name + " ";

        switch (type) {
            case DataType::INTEGER:
                sql += auto_increment ? "INTEGER PRIMARY KEY AUTOINCREMENT" : "INTEGER";
                break;
            case DataType::BIGINT:
                sql += "BIGINT";
                break;
            case DataType::REAL:
                sql += "REAL";
                break;
            case DataType::TEXT:
                sql += length > 0 ? "VARCHAR(" + std::to_string(length) + ")" : "TEXT";
                break;
            case DataType::BLOB:
                sql += "BLOB";
                break;
            case DataType::BOOLEAN:
                sql += "BOOLEAN";
                break;
            case DataType::DATE:
                sql += "DATE";
                break;
            case DataType::TIME:
                sql += "TIME";
                break;
            case DataType::TIMESTAMP:
                sql += "TIMESTAMP";
                break;
            case DataType::JSON:
                sql += "JSON";
                break;
            default:
                sql += "TEXT";
        }

        if (!nullable) sql += " NOT NULL";
        if (unique && !primary_key) sql += " UNIQUE";
        if (default_value.has_value()) {
            sql += " DEFAULT " + value_to_string(default_value.value());
        }

        return sql;
    }
};

} // namespace udb
include/universal_db/table/constraint.hpp
```

```cpp
#pragma once
#include <string>
#include <vector>

namespace udb {

/**
```

- Constraint types

```cpp
 */
enum class ConstraintType {
    PRIMARY_KEY,
    FOREIGN_KEY,
    UNIQUE,
    CHECK
};

/**
```

- Constraint definition

```cpp
 */
class Constraint {
    ConstraintType type;
    std::string name;
    std::vector<std::string> columns;
    std::string reference_table;
    std::vector<std::string> reference_columns;
    std::string check_expression;
    std::string on_delete;
    std::string on_update;

public:
    static Constraint primary_key(const std::vector<std::string>& cols) {
        Constraint c;
        c.type = ConstraintType::PRIMARY_KEY;
        c.columns = cols;
        return c;
    }

    static Constraint foreign_key(const std::string& col, const std::string& ref_table, const std::string& ref_col) {
        Constraint c;
        c.type = ConstraintType::FOREIGN_KEY;
        c.columns = {col};
        c.reference_table = ref_table;
        c.reference_columns = {ref_col};
        return c;
    }

    static Constraint unique(const std::vector<std::string>& cols) {
        Constraint c;
        c.type = ConstraintType::UNIQUE;
        c.columns = cols;
        return c;
    }

    static Constraint check(const std::string& expr) {
        Constraint c;
        c.type = ConstraintType::CHECK;
        c.check_expression = expr;
        return c;
    }

    Constraint& named(const std::string& n) {
        name = n;
        return *this;
    }
    Constraint& on_delete_cascade() {
        on_delete = "ON DELETE CASCADE";
        return *this;
    }

    Constraint& on_update_cascade() {
        on_update = "ON UPDATE CASCADE";
        return *this;
    }

    std::string to_sql() const {
        std::string sql;

        if (!name.empty()) {
            sql += "CONSTRAINT " + name + " ";
        }

        switch (type) {
            case ConstraintType::PRIMARY_KEY:
                sql += "PRIMARY KEY (";
                for (size_t i = 0; i < columns.size(); i++) {
                    if (i > 0) sql += ", ";
                    sql += columns[i];
                }
                sql += ")";
                break;

            case ConstraintType::FOREIGN_KEY:
                sql += "FOREIGN KEY (";
                for (size_t i = 0; i < columns.size(); i++) {
                    if (i > 0) sql += ", ";
                    sql += columns[i];
                }
                sql += ") REFERENCES " + reference_table + " (";
                for (size_t i = 0; i < reference_columns.size(); i++) {
                    if (i > 0) sql += ", ";
                    sql += reference_columns[i];
                }
                sql += ")";
                if (!on_delete.empty()) sql += " " + on_delete;
                if (!on_update.empty()) sql += " " + on_update;
                break;

            case ConstraintType::UNIQUE:
                sql += "UNIQUE (";
                for (size_t i = 0; i < columns.size(); i++) {
                    if (i > 0) sql += ", ";
                    sql += columns[i];
                }
                sql += ")";
                break;

            case ConstraintType::CHECK:
                sql += "CHECK (" + check_expression + ")";
                break;
        }

        return sql;
    }
};

} // namespace udb
```

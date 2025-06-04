# KadeDB: A Multi-Model Database for Healthcare and Beyond

[![CI/CD Pipeline](https://github.com/MediLang/KadeDB/actions/workflows/ci.yml/badge.svg)](https://github.com/MediLang/KadeDB/actions/workflows/ci.yml)
[![Code Coverage](https://codecov.io/gh/MediLang/KadeDB/branch/main/graph/badge.svg)](https://codecov.io/gh/MediLang/KadeDB)
[![Documentation Status](https://readthedocs.org/projects/kadedb/badge/?version=latest)](https://kadedb.readthedocs.io/en/latest/?badge=latest)
[![Docker Pulls](https://img.shields.io/docker/pulls/medilang/kadedb)](https://hub.docker.com/r/medilang/kadedb)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

## Overview

KadeDB is a versatile multi-model database designed to support a wide range of applications, from healthcare to finance, logistics, manufacturing, smart cities, and scientific research. It unifies:

- **Relational** storage for structured data (e.g., records, inventories)
- **Document** storage for semi-structured data (e.g., logs, configurations)
- **Time-series** storage for temporal data (e.g., sensor readings, market ticks)
- **Graph** storage for network data (e.g., networks, relationships)

Optimized for high-performance computing (HPC) on servers and edge nodes, KadeDB includes a lightweight client, KadeDB-Lite, for resource-constrained IoT and wearable devices.

### Components

- **KadeDB**: Built in C++ for top performance, GPU acceleration, and scalability, ideal for compute-intensive analytics and large-scale data management.
- **KadeDB-Lite**: Built in C for minimal resource usage, targeting IoT devices with a RocksDB-based embedded storage engine and low-bandwidth syncing.

KadeDB: Built in C++ for top performance, GPU acceleration, and scalability, ideal for compute-intensive analytics and large-scale data management.
KadeDB-Lite: Built in C for minimal resource usage, targeting IoT devices with a RocksDB-based embedded storage engine and low-bandwidth syncing.

## Supported Use Cases

### Healthcare
- Electronic Health Records (EHRs)
- Telemedicine
- Molecular medicine (via Medi and Medi-CMM)

### Other Industries
- **Finance**: Trading, fraud detection
- **Logistics**: Supply chain management
- **Manufacturing**: Smart factories
- **Smart Cities**: Traffic management
- **Research**: Climate modeling, scientific computing

## Problem Statement

Modern applications across industries require databases that:

- Manage diverse data types
- Scale for large datasets
- Support HPC for analytics
- Operate on resource-constrained IoT devices

### Industry-Specific Challenges

- **Healthcare**: Seamless integration for EHRs, drug discovery, and wearable monitoring with strict compliance (e.g., HIPAA)
- **Finance**: High-frequency trading data processing
- **Logistics**: Complex supply chain networks
- **Smart Cities**: Real-time sensor network management

### The Gap in Existing Solutions

Existing databases—relational (PostgreSQL), document (MongoDB), time-series (TimescaleDB), and graph (Neo4j)—excel in specific areas but lack unified support for multi-model data, HPC, and IoT. KadeDB addresses this with a single, extensible database, with KadeDB-Lite enabling lightweight IoT deployments.
## Solution

KadeDB is a multi-model database with two complementary components:

### KadeDB (Server)
- **Language**: C++
- **Environment**: HPC and edge environments
- **Storage Types**: Relational, document, time-series, and graph
- **Features**:
  - GPU-accelerated processing
  - Distributed scaling
  - High-performance computing capabilities

### KadeDB-Lite (Client)
- **Language**: C
- **Target**: IoT devices
- **Resource Requirements**:
  - RAM: 64 KB–1 MB
  - Storage: 512 KB–32 MB
- **Features**:
  - RocksDB-based storage engine
  - Minimal KadeQL subset
  - MQTT/CoAP syncing

## Key Features

### Multi-Model Storage
- **Relational**: ACID-compliant tables for records, inventories, and transactions
- **Document**: JSON-like storage for logs, configurations, and metadata
- **Time-Series**: Optimized for sensor data, market ticks, and simulations
- **Graph**: Nodes/edges for networks, relationships, and dependencies

### KadeQL
- Domain-agnostic query language
- Supports SQL-like, Cypher-like, and time-series queries
- Integrated with Medi's DSL for healthcare

### Performance & Optimization
- **HPC Optimization**:
  - GPU-accelerated processing (C++ with CUDA)
  - In-memory caching
  - Parallel queries for analytics and simulations

### IoT Support
- KadeDB-Lite's embedded storage for low-power devices
- Efficient syncing to KadeDB servers

### Interoperability
- **Healthcare**: FHIR, HL7
- **Finance**: AMQP
- **Manufacturing**: OPC UA
- **APIs**: REST, Python bindings

### Security & Compliance
- **Encryption**:
  - KadeDB: AES-256
  - KadeDB-Lite: AES-128
- **Access Control**: Role-based access
- **Audit Logging**: Comprehensive tracking
- **Standards Compliance**: HIPAA, GDPR, PCI DSS, ISO 27001

### Advanced Features
- **Reproducibility**: Version control for datasets and parameters
- **Scalability**: Distributed architecture for large-scale datasets

## Grand Vision

KadeDB aims to be a universal database for data-intensive applications, with:

- **Core Focus**: Healthcare (Medi/Medi-CMM)
- **Extensibility** to:
  - Finance
  - Logistics
  - Manufacturing
  - Smart Cities
  - Scientific Research

By unifying HPC, IoT, and multi-model storage in a single repository, KadeDB:
1. Simplifies development
2. Ensures compliance
3. Drives innovation across sectors
   - From precision medicine to smart factories

## Building from Source

### Prerequisites

- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- CMake 3.14 or higher
- Git
- [RocksDB](https://github.com/facebook/rocksdb) (6.8.1 or later)
- [ANTLR4](https://www.antlr.org/) (4.9 or later)
- [OpenSSL](https://www.openssl.org/) (1.1.1 or later)
- [Google Test](https://github.com/google/googletest) (for tests, optional)
- [Google Benchmark](https://github.com/google/benchmark) (for benchmarks, optional)

### Project Structure

```
KadeDB/
├── cmake/                 # CMake modules and configuration
│   ├── Modules/          # Find modules for dependencies
│   ├── KadeDBConfig.cmake.in  # Package configuration template
│   └── cmake_uninstall.cmake.in  # Uninstall script template
├── docs/                  # Documentation
├── include/               # Public headers
│   └── kadedb/           # KadeDB public API
│       ├── core/         # Core database interfaces
│       ├── storage/      # Storage engine interfaces
│       └── server/       # Server-specific headers
├── src/                  # Source code
│   ├── core/             # Core database implementation
│   │   ├── storage/      # Storage engines
│   │   ├── query/        # Query processing
│   │   └── CMakeLists.txt
│   ├── server/           # Server implementation
│   │   ├── http/        # HTTP server
│   │   ├── api/         # API endpoints
│   │   └── CMakeLists.txt
│   └── lite/             # KadeDB-Lite (future)
├── test/                 # Test suite
│   ├── unit/            # Unit tests
│   └── integration/      # Integration tests
├── CMakeLists.txt        # Main CMake configuration
└── README.md            # This file
```

### Build Instructions

```bash
# Clone the repository
git clone --recurse-submodules https://github.com/MediLang/KadeDB.git
cd KadeDB

# Create build directory and configure
mkdir -p build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTS=ON

# Build the project
make -j$(nproc)

# Run tests (if built)
ctest --output-on-failure

# Install the library and executables
sudo make install

# To uninstall (if needed)
sudo make uninstall
```

#### Build Options

- `-DBUILD_SHARED_LIBS=ON/OFF`: Build shared libraries (default: ON)
- `-DBUILD_TESTS=ON/OFF`: Build tests (default: OFF)
- `-DBUILD_BENCHMARKS=ON/OFF`: Build benchmarks (default: OFF)
- `-DUSE_SYSTEM_DEPS=ON/OFF`: Use system-installed dependencies (default: OFF)
- `-DCMAKE_BUILD_TYPE=Debug/Release/RelWithDebInfo`: Set build type (default: Debug)
- `-DCMAKE_INSTALL_PREFIX=/path`: Set installation prefix (default: /usr/local)

### Development Workflow

1. **Clone the repository** with submodules:
   ```bash
   git clone --recurse-submodules https://github.com/MediLang/KadeDB.git
   ```

2. **Build with development tools**:
   ```bash
   mkdir -p build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DENABLE_ASAN=ON
   make -j$(nproc)
   ```

3. **Run tests**:
   ```bash
   ctest --output-on-failure -j$(nproc)
   ```

4. **Build documentation**:
   ```bash
   make docs
   ```

### Using as a Dependency

KadeDB provides CMake package configuration files for easy integration into other projects:

```cmake
find_package(KadeDB REQUIRED)
target_link_libraries(your_target PRIVATE KadeDB::Core)
```
## CI/CD Pipeline

KadeDB uses GitHub Actions for continuous integration and deployment. The pipeline includes:

### Build and Test
- **Platforms**: Linux, macOS, Windows
- **Compilers**: GCC, Clang, MSVC
- **Build Types**: Debug, Release, RelWithDebInfo
- **Static Analysis**: clang-tidy, cppcheck
- **Code Formatting**: clang-format
- **Testing**: Unit tests, integration tests, benchmarks

### Code Quality
- Automated code formatting checks
- Static code analysis
- Code coverage reporting
- Dependency updates (Dependabot)

### Deployment
- **Docker Images**: Automated builds for all releases
- **Documentation**: Auto-deployed to GitHub Pages
- **Packages**: Source and binary packages

### Release Process
1. Create a version tag (e.g., `v1.0.0`)
2. Push the tag to trigger the release workflow
3. The workflow will:
   - Build and test the code
   - Create a GitHub release
   - Build and push Docker images
   - Publish packages (if configured)

## Key Features
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Or use system dependencies (if available)
cmake .. -DUSE_SYSTEM_DEPS=ON
make -j$(nproc)
```

### Key Features

- **Cross-platform**: Builds on Linux, macOS, and Windows
- **Flexible Dependency Management**: Supports both system and bundled dependencies
- **Multiple Build Types**: Supports Debug, Release, RelWithDebInfo, and MinSizeRel
- **Sanitizers**: Integration with AddressSanitizer, ThreadSanitizer, and UndefinedBehaviorSanitizer
- **Code Coverage**: Generate coverage reports with `-DENABLE_COVERAGE=ON`
- **LTO/IPO Support**: Enable optimizations with `-DENABLE_LTO=ON` and `-DENABLE_IPO=ON`
- **Testing**: Built-in support for Google Test and Google Benchmark

### Dependency Management

KadeDB uses a flexible dependency management system that supports both system-installed and bundled dependencies. By default, it will download and build all required dependencies.

#### Available Options

- `USE_SYSTEM_DEPS`: Use system-installed dependencies when available (default: OFF)
- `USE_SYSTEM_ROCKSDB`: Use system-installed RocksDB (default: value of USE_SYSTEM_DEPS)
- `USE_SYSTEM_ANTLR`: Use system-installed ANTLR4 (default: value of USE_SYSTEM_DEPS)
- `BUILD_TESTS`: Build tests (default: OFF)
- `BUILD_BENCHMARKS`: Build benchmarks (default: OFF)

#### Version Control

You can specify versions for dependencies:

```bash
cmake .. \
  -DKADEDB_ROCKSDB_VERSION=8.0.0 \
  -DKADEDB_ANTLR_VERSION=4.9.3 \
  -DKADEDB_OPENSSL_VERSION=1.1.1
```

For more details, see the [Dependency Management Documentation](cmake/README.md).

## Repository Structure

```
KadeDB/
├── kadedb-server/          # C++-based server-side database
│   ├── relational/         # Relational storage
│   ├── document/           # Document storage
│   ├── time-series/        # Time-series storage
│   ├── graph/              # Graph storage
│   ├── kadeql/             # Full KadeQL parser
│   └── backend/            # C++ with CUDA, Python bindings
├── kadedb-lite/            # C-based embedded client for IoT devices
│   ├── storage/            # RocksDB-based embedded storage
│   ├── kadeql-lite/        # Minimal KadeQL subset
│   ├── sync/               # MQTT/CoAP sync logic
│   ├── compliance/         # AES-128 encryption, audit logging
│   └── build/              # ARM-specific build scripts
├── docs/                   # Documentation
├── tests/                  # Testing framework
├── examples/               # Code samples for healthcare and other industries
├── README.md               # This file
└── .github/                # CI/CD pipelines
```

## Technical Architecture

### KadeDB (Server)

| Component           | Details                                                                 |
|---------------------|-------------------------------------------------------------------------|
| **Language**        | C++ (C++17/20) for performance, GPU support, and ecosystem             |
| **Storage Engine**  | Hybrid row-based, columnar, and graph storage                         |
| **Query Engine**    | Full KadeQL parser for complex queries across industries              |
| **HPC**             | CUDA for GPU tasks, distributed sharding, in-memory caching           |
| **Interoperability**| Python bindings (pybind11), REST APIs, and extensible connectors      |


### KadeDB-Lite (Client)

| Component          | Details                                                                 |
|--------------------|-------------------------------------------------------------------------|
| **Language**       | C for minimal footprint and ARM portability                           |
| **Storage Engine** | RocksDB-based key-value store with time-series and simplified relational support |
| **Query Engine**   | Minimal KadeQL subset (e.g., SELECT, INSERT)                          |
| **Sync**           | MQTT/CoAP with CBOR payloads for low-bandwidth syncing               |
| **Compliance**     | Mbed TLS for AES-128 encryption, minimal logging                     |


## Programming Language Rationale

### KadeDB (C++)
- **Why C++?**
  - Top performance in HPC tasks (simulations, analytics)
  - Rich ecosystem (Boost, pybind11)
  - Flexibility for various industries (finance, manufacturing)
  - GPU acceleration (CUDA) support
  - Distributed queries for data-intensive applications

### KadeDB-Lite (C)
- **Why C?**
  - Minimal binary size (<100 KB)
  - Low RAM usage (<500 KB)
  - Ideal for IoT devices:
    - Healthcare wearables
    - Logistics tracking
    - Smart city sensors
  - Native RocksDB C API integration
  - Excellent ARM portability

## Applications Across Industries

### Healthcare (Medi/Medi-CMM)

#### 1. Electronic Health Records (EHRs)
- **Storage**: Relational for patient records, document for notes
- **Example**:
  ```javascript
  let patient = Patient.from_fhir("patient.json");
  patient.save_to_kadedb("ehr", "patient_records");
  ```

#### 2. Telemedicine
- **Storage**: Time-series for vitals, graph for patient-provider networks
- **Example**:
  ```javascript
  let session = TelemedicineSession.new();
  session.save_to_kadedb("telemedicine", "sessions");
  ```

#### 3. Drug Discovery
- **Storage**: Relational for PDB files, document for screening results
- **Example**:
  ```python
  screening = VirtualScreening.new().execute(parallel_jobs=64);
  screening.save_to_kadedb("drug_discovery", "hits");
  ```



### Finance

#### 1. High-Frequency Trading
- **Storage**: Time-series for market ticks, relational for trades
- **Example Query**:
  ```sql
  FROM KadeDB.finance
  SELECT trades
  WHERE volume > 1000;
  ```

#### 2. Fraud Detection
- **Storage**:
  - Graph for transaction networks
  - Document for audit logs

### Logistics

#### 1. Supply Chain Optimization
- **Storage**:
  - Graph for supplier networks
  - Time-series for shipments
- **Example Query**:
  ```sql
  FROM KadeDB.logistics
  SELECT shipments
  WHERE delay > 2
  LINKED_TO supplier.network;
  ```

#### 2. IoT Tracking
- **Implementation**:
  ```javascript
  let sensor = GPSSensor.read_value();
  sensor.save_to_kadedb("local", "location");
  ```



### Manufacturing

#### 1. Smart Factory Analytics
- **Storage**:
  - Time-series for machine sensors
  - Relational for schedules
- **Example Query**:
  ```sql
  FROM KadeDB.manufacturing
  SELECT vibration
  WHERE value > 0.5;
  ```

#### 2. IoT Sensors
- **Implementation**:
  ```javascript
  let sensor = VibrationSensor.read_value();
  sensor.save_to_kadedb("local", "vibration");
  ```



### Smart Cities

#### 1. Traffic Management
- **Storage**:
  - Time-series for vehicle sensors
  - Graph for road networks
- **Example Query**:
  ```sql
  FROM KadeDB.smart_cities
  SELECT traffic
  WHERE speed < 20;
  ```

#### 2. Smart Streetlights
- **Implementation**:
  ```javascript
  let sensor = LightSensor.read_value();
  sensor.save_to_kadedb("local", "light");
  ```



### Scientific Research

#### 1. Climate Modeling
- **Storage**:
  - Time-series for weather data
  - Document for metadata
- **Example Query**:
  ```sql
  FROM KadeDB.research
  SELECT temp
  WHERE value > 25;
  ```

#### 2. Environmental Sensors
- **Implementation**:
  ```javascript
  let sensor = CO2Sensor.read_value();
  sensor.save_to_kadedb("local", "co2");
  ```



## Validation

### Key Strengths

1. **Versatility**
   - Multi-model storage
   - HPC capabilities
   - IoT support through KadeDB-Lite
   - Cross-industry applicability

2. **Healthcare Alignment**
   - Core support for Medi/Medi-CMM
   - Shared data models across applications
   - Compliance with healthcare standards (HIPAA, GDPR)

### Feature Comparison

| Feature | KadeDB | PostgreSQL | MongoDB | TimescaleDB | Neo4j |
|---------|--------|------------|---------|-------------|-------|
| **Multi-Model** | ✅ | 🟡 (JSONB) | 🟡 (Documents) | 🟡 (Time-Series) | 🟡 (Graph) |
| **IoT Support** | ✅ (KadeDB-Lite) | ❌ | ❌ | ❌ | ❌ |
| **HPC Optimization** | ✅ | 🟡 | 🟡 | ✅ | 🟡 |
| **Compliance** | ✅ | ✅ | 🟡 | ✅ | 🟡 |
| **Industry Extensibility** | ✅ | 🟡 | 🟡 | 🟡 | 🟡 |




## Development Plan

| Phase | Timeline | Key Milestones |
|-------|----------|-----------------|
| **Phase 1** | 2025–2026 | - Prototype KadeDB server (C++, relational/document)<br>- KadeDB-Lite (C, time-series)<br>- Healthcare and IoT focus |
| **Phase 2** | 2026–2027 | - Add time-series/graph models<br>- HPC optimizations (CUDA)<br>- Industry connectors (AMQP, OPC UA) |
| **Phase 3** | 2027–2028 | - Full multi-model support<br>- Compliance certifications (HIPAA, GDPR, PCI DSS) |
| **Phase 4** | 2028+ | - Open-source release<br>- Expand to new industries |

## Hardware Requirements

| Component | Specifications |
|-----------|----------------|
| **Server** | - Multi-core CPUs<br>- GPUs (NVIDIA CUDA)<br>- SSDs<br>- 64 GB+ RAM |
| **IoT Devices** | - ARM Cortex-M<br>- 64 KB–1 MB RAM<br>- 512 KB–32 MB storage |
| **Edge Nodes** | - 4-core CPUs<br>- 16 GB RAM<br>- SSDs |

## Getting Started

### 1. Clone Repository
```bash
git clone https://github.com/MediLang/KadeDB.git
cd KadeDB
```

### 2. Build KadeDB-Lite (C, IoT)
```bash
cd kadedb-lite
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=arm-gcc.cmake ..
make
```

### 3. Build KadeDB (C++, Server)
```bash
cd ../../kadedb-server
mkdir build && cd build
cmake ..
make
```

### 4. Run Example
```javascript
// Connect to KadeDB
let db = KadeDB.connect("localhost:5432", credentials);

// Example: Save sensor data (KadeDB-Lite)
let sensor = VibrationSensor.read_value();
sensor.save_to_kadedb("local", "vibration");

// Example: Save trade data (KadeDB)
db.save_trade(trade_data);
```

### 5. Example Query
```sql
FROM KadeDB.manufacturing
SELECT vibration
WHERE value > 0.5;
```



## Contributing

We welcome contributions in the following areas:
- C++ (KadeDB) and C (KadeDB-Lite) development
- Storage engines
- KadeQL extensions
- Industry-specific connectors

Please see our [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the **MIT License**, ensuring open access for developers across industries.

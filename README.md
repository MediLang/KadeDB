# KadeDB: A Multi-Model Database for Healthcare and Beyond

[![CI/CD Pipeline](https://github.com/MediLang/KadeDB/actions/workflows/ci.yml/badge.svg)](https://github.com/MediLang/KadeDB/actions/workflows/ci.yml)
[![Code Coverage](https://codecov.io/gh/MediLang/KadeDB/branch/main/graph/badge.svg)](https://codecov.io/gh/MediLang/KadeDB)
[![Documentation Status](https://readthedocs.org/projects/kadedb/badge/?version=latest)](https://kadedb.readthedocs.io/en/latest/?badge=latest)
[![Docker Pulls](https://img.shields.io/docker/pulls/medilang/kadedb)](https://hub.docker.com/r/medilang/kadedb)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Overview

KadeDB is a versatile multi-model database designed to support a wide range of applications, from healthcare to finance, logistics, manufacturing, smart cities, and scientific research. It unifies:

- **Relational** storage for structured data (e.g., records, inventories)
- **Document** storage for semi-structured data (e.g., logs, configurations)
- **Time-series** storage for temporal data (e.g., sensor readings, market ticks)
- **Graph** storage for network data (e.g., networks, relationships)

Optimized for high-performance computing (HPC) on servers and edge nodes, KadeDB includes a lightweight client, KadeDB-Lite, for resource-constrained IoT and wearable devices.

### Components

- **KadeDB Core (C++)**: High-performance engine with GPU acceleration and distributed scalability for compute-intensive analytics and storage internals.
- **KadeDB Services (Rust)**: Secure, async service layer (REST/gRPC), connectors, auth/RBAC, and orchestration built with Rust for memory safety and reliability.
- **KadeDB-Lite (C)**: Minimal footprint client for IoT/wearables with RocksDB-based embedded storage and low-bandwidth syncing.

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

### KadeDB Core (C++)
- **Language**: C++
- **Environment**: HPC and edge environments
- **Storage Types**: Relational, document, time-series, and graph
- **Features**:
  - GPU-accelerated processing
  - Distributed scaling
  - High-performance computing capabilities

### KadeDB Services (Rust)
- **Language**: Rust
- **Role**: Network services (REST/gRPC), query orchestration, connectors (AMQP/HTTP), auth/RBAC
- **Features**:
  - Async I/O (tokio), reliability (clippy/rustfmt), safety across concurrency
  - Safe FFI to C++ core via C ABI or `cxx` crate
  - Zero-copy data exchange using Arrow C Data Interface

 
### KadeDB-Lite (C)
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
- Rust toolchain (stable) with `rustup` and `cargo` (for services)
- Protobuf compiler `protoc` (optional, for gRPC/tonic services)

## Repository Structure

```
KadeDB/
├── cpp/                     # C++ core (engine, kernels, CUDA, public headers)
│   ├── include/             # Public headers (C++ API + C ABI)
│   │   └── kadedb/
│   ├── src/                 # Core engine implementation (storage, query, exec)
│   ├── cuda/                # GPU kernels / CUDA code (optional)
│   └── CMakeLists.txt
├── rust/                    # Rust workspace (services, connectors)
│   ├── Cargo.toml           # Workspace manifest
│   ├── services/            # REST/gRPC services, orchestration
│   └── connectors/          # AMQP/HTTP/OPC UA connectors
├── lite/                    # KadeDB-Lite (C for IoT)
│   ├── src/
│   └── CMakeLists.txt
├── bindings/                # Language bindings and FFI
│   ├── c/                   # Stable C ABI headers for the core
│   └── python/              # pybind11 or PyO3 wrappers
├── cmake/                   # CMake configs, Find modules, uninstall scripts
├── docs/                    # Documentation
├── examples/                # Samples for healthcare and other industries
├── test/                    # Unit/integration tests (C++ and Rust)
├── .github/                 # CI/CD pipelines
└── README.md                # This file
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

# Build the C++ core
make -j$(nproc)

# Run core tests (if built)
ctest --output-on-failure

# Install the core library and executables (optional)
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

### Build Orchestration (Top-Level)

- Build the C++ core with CMake/Ninja or Make in `cpp/`.
- Build Rust services with Cargo in `rust/` (workspace-aware).
- Build KadeDB-Lite with CMake in `lite/` using the appropriate toolchain file for your target (e.g., ARM).
- There is no cross-language super-build; keep C++ and Rust build systems independent and join them at the FFI boundary.

### Development Workflow

1. **Clone the repository** with submodules:
   ```bash
   git clone --recurse-submodules https://github.com/MediLang/KadeDB.git
   ```

2. **Build core with development tools**:
   ```bash
   mkdir -p build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DENABLE_ASAN=ON
   make -j$(nproc)
   ```

3. **Run core tests**:
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

## FFI Boundary and Bindings

KadeDB integrates C++ and Rust at a stable boundary via either a C ABI or the `cxx` crate. Public headers live in `cpp/include/kadedb/`, and exported C headers live in `bindings/c/`.

### Option A: C ABI (stable, language-agnostic)

`bindings/c/kadedb.h`:
```c
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KadeDBCore KadeDBCore;

KadeDBCore* kadedb_core_new();
void        kadedb_core_free(KadeDBCore*);
const char* kadedb_core_version(const KadeDBCore*);

#ifdef __cplusplus
}
#endif
```

Rust (using `bindgen` or hand-written FFI) can call these functions, and any string ownership rules should be documented (e.g., who frees returned strings).

### Option B: cxx crate (safe C++ interop)

Rust (`rust/services/<svc-name>/src/ffi.rs`):
```rust
#[cxx::bridge]
mod ffi {
    unsafe extern "C++" {
        include!("kadedb/include/kadedb/core.hpp");
        type Core;
        fn version(self: &Core) -> String;
    }

    extern "Rust" {
        fn log_message(msg: &str);
    }
}

pub fn log_message(msg: &str) {
    println!("[svc] {msg}");
}
```

C++ (`cpp/include/kadedb/core.hpp`):
```cpp
#pragma once
#include <string>

namespace kadedb {
class Core {
public:
  std::string version() const; // e.g., "kadedb-0.1.0"
};
}
```

Keep exported APIs small and stable. Prefer plain-old-data and slices over complex templates across the boundary. For columnar exchange, consider the Arrow C Data Interface.
## CI/CD Pipeline

KadeDB uses GitHub Actions for continuous integration and deployment. The pipeline includes:

### Build and Test
- **Platforms**: Linux, macOS, Windows
- **Compilers**: GCC, Clang, MSVC (C++)
- **Rust Toolchain**: stable (cargo build/test), clippy, rustfmt
- **Build Types**: Debug, Release, RelWithDebInfo
- **Static Analysis**: clang-tidy, cppcheck (C++) and clippy (Rust)
- **Code Formatting**: clang-format (C++) and rustfmt (Rust)
- **Testing**: Unit/integration tests for C++ and Rust; benchmarks

### Code Quality
- Automated code formatting checks
- Static code analysis
- Code coverage reporting
- Dependency updates (Dependabot)

### Deployment
- **Docker Images**: Automated builds for all releases
- **Documentation**: Auto-deployed to Read the Docs
- **Packages**: Source and binary packages

### Release Process
1. Create a version tag (e.g., `v1.0.0`)
2. Push the tag to trigger the release workflow
3. The workflow will:
   - Build and test the code
   - Create a GitHub release
   - Build and push Docker images
   - Publish packages (if configured)

### Build System Features

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

## Technical Architecture

### KadeDB Core (C++)

| Component           | Details                                                                 |
|---------------------|-------------------------------------------------------------------------|
| **Language**        | C++ (C++17/20) for performance, GPU support, and ecosystem             |
| **Storage Engine**  | Hybrid row-based, columnar, and graph storage                         |
| **Query Engine**    | Full KadeQL parser for complex queries across industries              |
| **HPC**             | CUDA for GPU tasks, distributed sharding, in-memory caching           |
| **Interoperability**| Python bindings (pybind11), REST APIs, and extensible connectors      |


### KadeDB Services (Rust)

| Component           | Details                                                                 |
|---------------------|-------------------------------------------------------------------------|
| **Language**        | Rust (stable) for memory safety, async, and reliability                 |
| **Role**            | REST/gRPC services, orchestration, connectors, auth/RBAC               |
| **Async Runtime**   | tokio, axum/hyper, tonic                                                |
| **Interoperability**| FFI to C++ core via C ABI or `cxx`; Arrow C Data Interface for zero-copy |


### KadeDB-Lite (C)

| Component          | Details                                                                 |
|--------------------|-------------------------------------------------------------------------|
| **Language**       | C for minimal footprint and ARM portability                           |
| **Storage Engine** | RocksDB-based key-value store with time-series and simplified relational support |
| **Query Engine**   | Minimal KadeQL subset (e.g., SELECT, INSERT)                          |
| **Sync**           | MQTT/CoAP with CBOR payloads for low-bandwidth syncing               |
| **Compliance**     | Mbed TLS for AES-128 encryption, minimal logging                     |


## Programming Language Rationale

### KadeDB Core (C++)
- **Why C++?**
  - Top performance in HPC tasks (simulations, analytics)
  - Rich ecosystem (Boost, pybind11)
  - Flexibility for various industries (finance, manufacturing)
  - GPU acceleration (CUDA) support
  - Distributed queries for data-intensive applications

### KadeDB Services (Rust)
- **Why Rust?**
  - Memory safety and fearless concurrency for services and orchestration
  - Excellent async ecosystem (tokio, axum, tonic) for high-throughput networking
  - Strong tooling (clippy, rustfmt) and fewer memory-safety CVEs
  - Ergonomic connectors/adapters for protocols (HTTP/AMQP/etc.)
  - Clean FFI to C++ core via `cxx` or stable C ABI; zero-copy with Arrow C Data Interface

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

### 2. Build KadeDB Core (C++)
```bash
cd cpp
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### 3. Build KadeDB Services (Rust)
```bash
cd ../../rust
cargo build --workspace --release
```

#### Run a sample Rust service
```bash
# Replace <svc-name> with an actual service name in `rust/services/`
cargo run -p services/<svc-name>

# Example (if you have an HTTP gateway service):
# cargo run -p services/http-gateway
```

### 4. Build KadeDB-Lite (C, IoT)
```bash
cd ../lite
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=arm-gcc.cmake ..
make
```

### 5. Run Example
```javascript
// Connect to KadeDB
let db = KadeDB.connect("localhost:5432", credentials);

// Example: Save sensor data (KadeDB-Lite)
let sensor = VibrationSensor.read_value();
sensor.save_to_kadedb("local", "vibration");

// Example: Save trade data (KadeDB)
db.save_trade(trade_data);
```

### 6. Example Query
```sql
FROM KadeDB.manufacturing
SELECT vibration
WHERE value > 0.5;
```



## Contributing

We welcome contributions in the following areas:
- C++ (KadeDB Core), Rust (KadeDB Services), and C (KadeDB-Lite) development
- Storage engines
- KadeQL extensions
- Industry-specific connectors

Please see our [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the [MIT License](./LICENSE), ensuring open access for developers across industries.

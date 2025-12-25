# KadeDB TODO

This file tracks discrepancies between the README claims and the current codebase implementation.

---

## Unimplemented Features

### Storage Models

| Feature | README Claim | Status |
|---------|--------------|--------|
| **Time-series storage** | "Time-series storage for temporal data (e.g., sensor readings, market ticks)" | ❌ Not implemented |
| **Graph storage** | "Graph storage for network data (e.g., networks, relationships)" | ❌ Not implemented |

### Components

| Component | README Claim | Status |
|-----------|--------------|--------|
| **KadeDB Services (Rust)** | "Secure, async service layer (REST/gRPC), connectors, auth/RBAC, and orchestration built with Rust" | ❌ No Rust code found in codebase |
| **GPU acceleration** | "High-performance engine with GPU acceleration" | ❌ No CUDA/GPU code found |
| **Distributed scalability** | "distributed scalability for compute-intensive analytics" | ❌ Not implemented |

---

## Action Items

### High Priority

#### 1. Time-Series Storage Model

- [ ] **Design time-series schema**
  - [ ] Define `TimeSeriesSchema` class with timestamp column, value columns, and optional tags
  - [ ] Support configurable time granularity (nanoseconds, milliseconds, seconds)
  - [ ] Design retention policies (TTL, max rows)
- [ ] **Implement time-series storage engine**
  - [ ] Create `TimeSeriesStorage` class in `cpp/include/kadedb/timeseries/`
  - [ ] Implement time-partitioned storage (hourly/daily buckets)
  - [ ] Add efficient range queries by time window
  - [ ] Implement downsampling/aggregation (avg, min, max, sum, count)
- [ ] **Add time-series query support to KadeQL**
  - [ ] Extend parser for `SELECT ... WHERE timestamp BETWEEN ... AND ...`
  - [ ] Add time-based aggregation functions (`TIME_BUCKET()`, `FIRST()`, `LAST()`)
- [ ] **Create tests and examples**
  - [ ] Unit tests in `cpp/test/timeseries_test.cpp`
  - [ ] Example in `cpp/examples/timeseries_example.cpp`
- [ ] **Update documentation**
  - [ ] Add `docs/sphinx/timeseries_api.rst`
  - [ ] Update README to reflect implementation status

#### 2. Graph Storage Model

- [ ] **Design graph schema**
  - [ ] Define `Node` and `Edge` types with property support
  - [ ] Design index structures for adjacency lookups
  - [ ] Support labeled edges and typed relationships
- [ ] **Implement graph storage engine**
  - [ ] Create `GraphStorage` class in `cpp/include/kadedb/graph/`
  - [ ] Implement node/edge CRUD operations
  - [ ] Add adjacency list storage with efficient neighbor lookups
  - [ ] Implement basic traversal algorithms (BFS, DFS)
- [ ] **Add graph query support**
  - [ ] Design query syntax for path traversal (e.g., Cypher-like or custom)
  - [ ] Implement `MATCH` patterns for subgraph queries
  - [ ] Add shortest path and connectivity queries
- [ ] **Create tests and examples**
  - [ ] Unit tests in `cpp/test/graph_test.cpp`
  - [ ] Example in `cpp/examples/graph_example.cpp`
- [ ] **Update documentation**
  - [ ] Add `docs/sphinx/graph_api.rst`
  - [ ] Update README to reflect implementation status

#### 3. Rust Services Layer

- [ ] **Set up Rust project structure**
  - [ ] Create `services/` directory with `Cargo.toml`
  - [ ] Configure workspace in root or standalone crate
  - [ ] Set up CI for Rust builds (add to `.github/workflows/ci.yml`)
- [ ] **Implement REST API service**
  - [ ] Use `axum` or `actix-web` framework
  - [ ] Create endpoints: `POST /query`, `GET /health`, `POST /tables`
  - [ ] Add JSON serialization for query results
- [ ] **Implement gRPC service**
  - [ ] Define `.proto` files in `services/proto/`
  - [ ] Generate Rust bindings with `tonic`
  - [ ] Implement streaming query results
- [ ] **Add authentication and RBAC**
  - [ ] Implement JWT-based authentication
  - [ ] Define role-based permissions (read, write, admin)
  - [ ] Add middleware for auth enforcement
- [ ] **FFI bridge to C++ core**
  - [ ] Use `cxx` or `bindgen` to call C ABI from Rust
  - [ ] Wrap storage operations in async Rust tasks
- [ ] **Create tests and examples**
  - [ ] Integration tests for REST/gRPC endpoints
  - [ ] Example client in `services/examples/`
- [ ] **Update documentation**
  - [ ] Add `docs/sphinx/services_api.rst`
  - [ ] Update README to reflect implementation status

---

### Medium Priority

#### 4. GPU Acceleration

- [ ] **Evaluate GPU use cases**
  - [ ] Identify compute-intensive operations (aggregations, joins, scans)
  - [ ] Benchmark CPU vs potential GPU speedup
- [ ] **Set up CUDA/GPU build infrastructure**
  - [ ] Add CMake option `KADEDB_ENABLE_GPU`
  - [ ] Detect CUDA toolkit and configure compilation
  - [ ] Add CI job for GPU builds (optional, or document manual build)
- [ ] **Implement GPU-accelerated kernels**
  - [ ] Create `cpp/src/gpu/` directory for CUDA code
  - [ ] Implement parallel scan/filter kernel
  - [ ] Implement parallel aggregation kernel
- [ ] **Integrate with storage engine**
  - [ ] Add GPU execution path in query executor
  - [ ] Implement memory transfer optimizations (pinned memory, async copy)
- [ ] **Create tests and benchmarks**
  - [ ] GPU unit tests (requires CUDA-capable CI or manual testing)
  - [ ] Benchmark comparing CPU vs GPU performance
- [ ] **Update documentation**
  - [ ] Add GPU build instructions to `docs/sphinx/guides/getting_started.md`
  - [ ] Update README to reflect implementation status

#### 5. Distributed Scalability

- [ ] **Design distributed architecture**
  - [ ] Define sharding strategy (hash-based, range-based)
  - [ ] Design replication model (leader-follower, multi-leader)
  - [ ] Plan consensus protocol (Raft, or external coordinator like etcd)
- [ ] **Implement cluster membership**
  - [ ] Node discovery and heartbeat mechanism
  - [ ] Cluster configuration management
- [ ] **Implement distributed query execution**
  - [ ] Query routing to appropriate shards
  - [ ] Distributed aggregation (map-reduce style)
  - [ ] Result merging from multiple nodes
- [ ] **Implement data replication**
  - [ ] Write-ahead log replication
  - [ ] Failover and leader election
- [ ] **Create tests and examples**
  - [ ] Multi-node integration tests (Docker Compose or similar)
  - [ ] Example cluster setup scripts
- [ ] **Update documentation**
  - [ ] Add `docs/sphinx/guides/distributed_setup.md`
  - [ ] Update README to reflect implementation status

---

### Low Priority

#### 6. Docker Support

- [ ] **Create Dockerfile**
  - [ ] Multi-stage build: build stage (compile) + runtime stage (minimal image)
  - [ ] Support for both Debug and Release builds
  - [ ] Include KadeDB-Lite CLI as entrypoint option
- [ ] **Create docker-compose.yml**
  - [ ] Single-node development setup
  - [ ] (Future) Multi-node cluster setup
- [ ] **Add to CI**
  - [ ] Build and push Docker image on release tags
  - [ ] Publish to Docker Hub (`medilang/kadedb`)
- [ ] **Update documentation**
  - [ ] Add Docker usage instructions to README
  - [ ] Verify Docker badge links correctly

#### 7. ReadTheDocs Integration

- [ ] **Verify `.readthedocs.yaml` exists and is correct**
  - [ ] Check Sphinx build configuration
  - [ ] Ensure `docs/sphinx/conf.py` is compatible
- [ ] **Test ReadTheDocs build**
  - [ ] Trigger manual build on ReadTheDocs dashboard
  - [ ] Fix any build errors
- [ ] **Update badge URL if needed**
  - [ ] Ensure badge in README points to correct project

---

## Notes

- **Relational storage**: ✅ Implemented (`cpp/src/`, `cpp/include/`)
- **Document storage**: ✅ Implemented (`cpp/src/`, `cpp/include/`)
- **KadeDB Core (C++)**: ✅ Implemented
- **KadeDB-Lite (C)**: ✅ Implemented (`lite/`)
- **C ABI Bindings**: ✅ Implemented (`bindings/c/`)
- **Examples**: ✅ All paths exist (`cpp/examples/`, `examples/`)
- **Docs**: ✅ All paths exist (`docs/sphinx/guides/`)

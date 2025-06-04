# Build stage
FROM ubuntu:24.04 as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    libssl-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /kadedb

# Copy source code
COPY . .


# Build
RUN mkdir -p build && \
    cd build && \
    cmake -G Ninja .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_TESTS=OFF \
        -DCMAKE_INSTALL_LIBDIR=lib && \
    cmake --build . --target install -- -j$(nproc)

# Runtime image
FROM ubuntu:24.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    zlib1g \
    && rm -rf /var/lib/apt/lists/*

# Copy installed files from builder
COPY --from=builder /usr/local/ /usr/local/

# Create non-root user
RUN useradd -r -s /usr/sbin/nologin kadedb && \
    mkdir -p /var/lib/kadedb && \
    chown -R kadedb:kadedb /var/lib/kadedb

USER kadedb
WORKDIR /var/lib/kadedb

# Expose default ports
EXPOSE 8080 8081

# Set entrypoint
ENTRYPOINT ["kadedb"]
CMD ["--help"]

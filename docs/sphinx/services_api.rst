Rust Services API
=================

.. contents::
   :local:
   :depth: 2

Overview
--------

KadeDB includes an async Rust services layer under ``services/``.
It provides:

- A REST API server (Axum)
- A gRPC server (Tonic)
- Shared JWT authentication and RBAC enforcement
- A direct-linked FFI bridge to the C ABI (``bindings/c``)

REST Service
------------

Package:

- ``services/api`` (crate: ``kadedb-services-api``)

Run:

.. code-block:: bash

   cargo run -p kadedb-services-api --manifest-path services/Cargo.toml

Endpoints
~~~~~~~~~

- ``GET /health``
- ``POST /query`` (requires read permission when auth is enabled)
- ``POST /tables`` (requires write permission when auth is enabled)

Example requests
~~~~~~~~~~~~~~~~

.. code-block:: bash

   curl -sS http://127.0.0.1:8080/health

.. code-block:: bash

   curl -sS -X POST http://127.0.0.1:8080/query \
     -H 'content-type: application/json' \
     -d '{"query":"SELECT 1"}'

gRPC Service
------------

Package:

- ``services/grpc`` (crate: ``kadedb-services-grpc``)

Run:

.. code-block:: bash

   cargo run -p kadedb-services-grpc --manifest-path services/Cargo.toml

RPCs
~~~~

- ``Query(QueryRequest) returns (stream QueryRow)``

Authentication and RBAC
-----------------------

Auth is configured via environment variables shared by REST and gRPC:

- ``KADEDB_AUTH_ENABLED``: ``true``/``false`` (or ``1``/``0``)
- ``KADEDB_JWT_SECRET``: shared secret for HS256 JWT verification

When enabled:

- REST expects ``Authorization: Bearer <token>``
- gRPC expects metadata ``authorization: Bearer <token>``

Role claims
~~~~~~~~~~~

JWTs must include the ``role`` claim:

- ``read``
- ``write``
- ``admin``

FFI Bridge
----------

The services layer includes a Rust FFI crate:

- ``services/ffi`` (crate: ``kadedb-services-ffi``)

It links directly to the C ABI library built by CMake:

- ``kadedb_c`` (e.g. ``build/debug/lib/libkadedb_c.so``)

FFI calls are wrapped using ``tokio::task::spawn_blocking`` to avoid blocking
async runtimes.

Examples CLI
------------

A small CLI exists under ``services/examples`` (crate: ``kadedb-services-examples``):

.. code-block:: bash

   cargo run -p kadedb-services-examples --manifest-path services/Cargo.toml -- --help

The CLI can call both REST and gRPC endpoints and optionally attach a JWT token.

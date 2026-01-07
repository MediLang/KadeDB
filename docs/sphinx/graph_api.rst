Graph Storage API
=================

.. contents::
  :local:
  :depth: 2

Overview
--------

KadeDB provides a graph storage interface and an in-memory implementation.
The API is designed around:

- graph lifecycle management (create/drop/list)
- node/edge CRUD
- adjacency-based neighbor lookups
- simple traversal algorithms (BFS/DFS)
- a lightweight graph query layer (see :cpp:func:`kadedb::executeGraphQuery`)

Core Interfaces
---------------

GraphStorage
~~~~~~~~~~~~

.. doxygenclass:: kadedb::GraphStorage
  :project: KadeDB
  :members:
  :protected-members:
  :undoc-members:

In-Memory Implementation
------------------------

InMemoryGraphStorage
~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: kadedb::InMemoryGraphStorage
  :project: KadeDB
  :members:
  :protected-members:
  :undoc-members:

Schemas and Related Types
-------------------------

Node / Edge
~~~~~~~~~~~

.. doxygenstruct:: kadedb::Node
  :project: KadeDB
  :members:
  :undoc-members:

.. doxygenstruct:: kadedb::Edge
  :project: KadeDB
  :members:
  :undoc-members:

Graph Query
-----------

The graph query entrypoint returns a :cpp:class:`kadedb::ResultSet` for easy
integration with existing tooling.

.. doxygenfunction:: kadedb::executeGraphQuery
  :project: KadeDB

Examples and Tests
------------------

- Example: ``cpp/examples/graph_example.cpp`` (built as ``kadedb_graph_example``)
- Unit test: ``cpp/test/graph_test.cpp`` (runs as ``kadedb_graph_test``)

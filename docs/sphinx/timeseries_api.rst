Time-Series Storage API
=======================

.. contents::
  :local:
  :depth: 2

Overview
--------

KadeDB provides a time-series storage interface and an in-memory implementation.
The API is designed around:

- a schema (:cpp:class:`kadedb::TimeSeriesSchema`) containing a timestamp column,
  optional tag columns, and value columns
- series lifecycle management (create/drop/list)
- efficient time-window reads via :cpp:func:`kadedb::TimeSeriesStorage::rangeQuery`
- downsampling/aggregation via :cpp:func:`kadedb::TimeSeriesStorage::aggregate`

Core Interfaces
---------------

TimeSeriesStorage
~~~~~~~~~~~~~~~~~

.. doxygenclass:: kadedb::TimeSeriesStorage
  :project: KadeDB
  :members:
  :protected-members:
  :undoc-members:

In-Memory Implementation
------------------------

InMemoryTimeSeriesStorage
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: kadedb::InMemoryTimeSeriesStorage
  :project: KadeDB
  :members:
  :protected-members:
  :undoc-members:

Schemas and Related Types
-------------------------

TimeSeriesSchema
~~~~~~~~~~~~~~~~

.. doxygenclass:: kadedb::TimeSeriesSchema
  :project: KadeDB
  :members:
  :protected-members:
  :undoc-members:

RetentionPolicy
~~~~~~~~~~~~~~~

.. doxygenstruct:: kadedb::RetentionPolicy
  :project: KadeDB
  :members:
  :undoc-members:

TimePartition
~~~~~~~~~~~~~

.. doxygenenum:: kadedb::TimePartition
  :project: KadeDB

TimeAggregation
~~~~~~~~~~~~~~~

.. doxygenenum:: kadedb::TimeAggregation
  :project: KadeDB

TimeGranularity
~~~~~~~~~~~~~~~

.. doxygenenum:: kadedb::TimeGranularity
  :project: KadeDB

Predicate
~~~~~~~~~

Time-series queries can optionally include a :cpp:struct:`kadedb::Predicate` to
filter rows inside the requested time window.

.. doxygenstruct:: kadedb::Predicate
  :project: KadeDB
  :members:
  :undoc-members:

Examples and Tests
------------------

- Example: ``cpp/examples/timeseries_example.cpp`` (built as
  ``kadedb_timeseries_example``)
- Unit test: ``cpp/test/timeseries_test.cpp`` (runs as ``kadedb_timeseries_test``)

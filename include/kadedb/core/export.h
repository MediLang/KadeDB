#ifndef KADEDB_CORE_EXPORT_H
#define KADEDB_CORE_EXPORT_H

#include <kadedb/config.h>

// Platform detection
#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef KADEDB_CORE_STATIC_DEFINE
    #define KADEDB_CORE_EXPORT
    #define KADEDB_CORE_NO_EXPORT
  #else
    #ifdef KadeDB_core_EXPORTS
      #define KADEDB_CORE_EXPORT __declspec(dllexport)
    #else
      #define KADEDB_CORE_EXPORT __declspec(dllimport)
    #endif
    #define KADEDB_CORE_NO_EXPORT
  #endif
#else
  #define KADEDB_CORE_EXPORT __attribute__((visibility("default")))
  #define KADEDB_CORE_NO_EXPORT __attribute__((visibility("hidden")))
#endif

// API decoration for public API
#ifdef KADEDB_CORE_STATIC_DEFINE
  #define KADEDB_CORE_API
#else
  #define KADEDB_CORE_API KADEDB_CORE_EXPORT
#endif

#endif // KADEDB_CORE_EXPORT_H

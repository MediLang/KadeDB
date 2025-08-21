#include "kadedb/version.h"
#include "kadedb/kadedb.h"

extern "C" {

const char* KadeDB_GetVersion() {
  // Safe to return string literal defined by CMake at configure time
  return KADEDB_VERSION;
}

}

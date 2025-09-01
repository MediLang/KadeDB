#include "kadedb/core.h"
#include <cassert>
#include <iostream>

int main() {
  std::string v = kadedb::GetVersion();
  std::cout << "KadeDB version: " << v << "\n";
  // Basic sanity: non-empty and contains at least one dot
  assert(!v.empty());
  assert(v.find('.') != std::string::npos);
  return 0;
}

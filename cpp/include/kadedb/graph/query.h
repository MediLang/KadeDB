#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "kadedb/graph/storage.h"
#include "kadedb/result.h"

namespace kadedb {

Result<ResultSet> executeGraphQuery(const GraphStorage &storage,
                                    const std::string &query);

} // namespace kadedb

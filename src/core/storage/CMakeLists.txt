# Find CRC32C package
find_package(Crc32c REQUIRED)

add_library(kadedb_storage
    src/file_manager.cpp
)

target_include_directories(kadedb_storage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(kadedb_storage
    PRIVATE
        kadedb_common
        Crc32c::crc32c
)

target_compile_definitions(kadedb_storage
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG=1>
)

target_compile_options(kadedb_storage
    PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
)

# Add tests if available
if(BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt)
    add_subdirectory(test)
endif()

# Installation
install(TARGETS kadedb_storage
    EXPORT kadedb-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

cmake_minimum_required(VERSION 3.14)

# Define the KadeDB server library
add_library(kadedb
    src/main.cpp
)

target_include_directories(kadedb
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(kadedb
    PRIVATE
        Threads::Threads
)

# Install rules
install(TARGETS kadedb
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

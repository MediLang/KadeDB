#.rst:
# FindANTLR4
# ---------
#
# Find the native ANTLR4 includes and libraries.
#
# This module defines:
#
# ::
#
#   ANTLR4_FOUND - True if ANTLR4 found.
#   ANTLR4_INCLUDE_DIRS - where to find antlr4-runtime.h, etc.
#   ANTLR4_LIBRARIES - List of libraries when using ANTLR4.
#   ANTLR4_RUNTIME_LIBRARIES - List of runtime libraries.
#   ANTLR4_STATIC_LIBRARIES - List of static libraries.
#   ANTLR4_SHARED_LIBRARIES - List of shared libraries.
#   ANTLR4_VERSION - The version of ANTLR4 found.
#
# Hints
# ^^^^^
#
# A user may set ``ANTLR4_ROOT`` to an ANTLR4 installation root to tell this
# module where to look.

# Look for the header file.
find_path(ANTLR4_INCLUDE_DIR
  NAMES antlr4-runtime.h
  PATH_SUFFIXES antlr4-runtime
  HINTS ${ANTLR4_ROOT} ENV ANTLR4_ROOT
  PATH_SUFFIXES include
)

# Look for the library.
set(ANTLR4_LIB_NAMES
  antlr4_shared
  antlr4-runtime
  antlr4-runtime-cpp
  antlr4-runtime-cpp-shared
  antlr4-runtime-cpp-static
)

find_library(ANTLR4_LIBRARY
  NAMES ${ANTLR4_LIB_NAMES}
  HINTS ${ANTLR4_ROOT} ENV ANTLR4_ROOT
  PATH_SUFFIXES lib ${CMAKE_INSTALL_LIBDIR}
)

# Handle the QUIETLY and REQUIRED arguments and set ANTLR4_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ANTLR4
  FOUND_VAR ANTLR4_FOUND
  REQUIRED_VARS ANTLR4_LIBRARY ANTLR4_INCLUDE_DIR
)

if(ANTLR4_FOUND)
  set(ANTLR4_LIBRARIES ${ANTLR4_LIBRARY})
  set(ANTLR4_INCLUDE_DIRS ${ANTLR4_INCLUDE_DIR})
  
  # Extract version from header
  if(EXISTS "${ANTLR4_INCLUDE_DIR}/antlr4-runtime/antlr4-common.h")
    file(STRINGS "${ANTLR4_INCLUDE_DIR}/antlr4-runtime/antlr4-common.h" _version_str
         REGEX "#define ANTLR_VERSION[ \t]+")
    if(_version_str)
      string(REGEX REPLACE ".*ANTLR_VERSION[ \t]+\"([0-9]+\.[0-9]+\.[0-9]+).*" "\\1" 
             ANTLR4_VERSION "${_version_str}")
    endif()
    unset(_version_str)
  endif()
  
  # Create imported targets
  if(NOT TARGET ANTLR4::ANTLR4)
    add_library(ANTLR4::ANTLR4 UNKNOWN IMPORTED)
    set_target_properties(ANTLR4::ANTLR4 PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${ANTLR4_INCLUDE_DIR}"
      IMPORTED_LOCATION "${ANTLR4_LIBRARY}"
    )
    
    # Add system dependencies
    if(UNIX)
      find_package(Threads REQUIRED)
      set_property(TARGET ANTLR4::ANTLR4 APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES Threads::Threads
      )
      
      if(CMAKE_DL_LIBS)
        set_property(TARGET ANTLR4::ANTLR4 APPEND PROPERTY
          INTERFACE_LINK_LIBRARIES ${CMAKE_DL_LIBS}
        )
      endif()
    endif()
  endif()
  
  # Set runtime and static/shared libraries
  set(ANTLR4_RUNTIME_LIBRARIES ${ANTLR4_LIBRARIES})
  
  if(ANTLR4_LIBRARY MATCHES "shared" OR NOT ANTLR4_LIBRARY MATCHES "static")
    set(ANTLR4_SHARED_LIBRARIES ${ANTLR4_LIBRARIES})
  else()
    set(ANTLR4_STATIC_LIBRARIES ${ANTLR4_LIBRARIES})
  endif()
  
  mark_as_advanced(ANTLR4_INCLUDE_DIR ANTLR4_LIBRARY)
endif()

# Handle components
if(ANTLR4_FOUND)
  set(ANTLR4_${ANTLR4_FIND_COMPONENT}_FOUND TRUE)
  
  # Add tool component if requested
  if("tool" IN_LIST ANTLR4_FIND_COMPONENTS)
    find_program(ANTLR4_EXECUTABLE
      NAMES antlr4 antlr4-cpp
      HINTS ${ANTLR4_ROOT} ENV ANTLR4_ROOT
      PATH_SUFFIXES bin
    )
    
    if(ANTLR4_EXECUTABLE)
      set(ANTLR4_TOOL_FOUND TRUE)
      
      # Create a function to generate C++ code from grammar files
      function(antlr4_generate_cpp)
        set(options)
        set(oneValueArgs OUTPUT_DIRECTORY PACKAGE_NAME GENERATED_OUTPUTS)
        set(multiValueArgs GRAMMAR_FILES COMPILE_FLAGS DEPENDS)
        cmake_parse_arguments(ANTLR4 "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
        
        if(NOT ANTLR4_GRAMMAR_FILES)
          message(SEND_ERROR "Error: ANTLR4: No grammar files specified")
          return()
        endif()
        
        if(NOT ANTLR4_OUTPUT_DIRECTORY)
          set(ANTLR4_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/antlr4_generated_src)
        endif()
        
        file(MAKE_DIRECTORY ${ANTLR4_OUTPUT_DIRECTORY})
        
        set(ANTLR4_GENERATED_SRCS)
        
        foreach(grammar_file ${ANTLR4_GRAMMAR_FILES})
          get_filename_component(grammar_name ${grammar_file} NAME_WE)
          
          # Generate C++ files
          add_custom_command(
            OUTPUT
              ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Lexer.h
              ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Lexer.cpp
              ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Parser.h
              ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Parser.cpp
              ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Listener.h
              ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Listener.cpp
              ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}BaseListener.h
              ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}BaseListener.cpp
              ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Visitor.h
              ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Visitor.cpp
              ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}BaseVisitor.h
              ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}BaseVisitor.cpp
            COMMAND ${ANTLR4_EXECUTABLE}
              -Werror
              -D${ANTLR4_OUTPUT_DIRECTORY}
              -package ${ANTLR4_PACKAGE_NAME}
              -no-listener
              -visitor
              -o ${ANTLR4_OUTPUT_DIRECTORY}
              ${ANTLR4_COMPILE_FLAGS}
              ${grammar_file}
            DEPENDS ${grammar_file} ${ANTLR4_DEPENDS}
            COMMENT "Generating C++ code from ${grammar_file}"
            VERBATIM
          )
          
          list(APPEND ANTLR4_GENERATED_SRCS
            ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Lexer.h
            ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Lexer.cpp
            ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Parser.h
            ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Parser.cpp
            ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Visitor.h
            ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}Visitor.cpp
            ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}BaseVisitor.h
            ${ANTLR4_OUTPUT_DIRECTORY}/${grammar_name}BaseVisitor.cpp
          )
        endforeach()
        
        set_source_files_properties(
          ${ANTLR4_GENERATED_SRCS}
          PROPERTIES GENERATED TRUE
        )
        
        if(ANTLR4_GENERATED_OUTPUTS)
          set(${ANTLR4_GENERATED_OUTPUTS} ${ANTLR4_GENERATED_SRCS} PARENT_SCOPE)
        endif()
      endfunction()
    else()
      set(ANTLR4_TOOL_FOUND FALSE)
      if(ANTLR4_FIND_REQUIRED_tool)
        message(FATAL_ERROR "ANTLR4 tool not found")
      endif()
    endif()
  endif()
  
  # Add all components to the found variables
  set(ANTLR4_LIBRARIES ${ANTLR4_LIBRARIES} PARENT_SCOPE)
  set(ANTLR4_INCLUDE_DIRS ${ANTLR4_INCLUDE_DIRS} PARENT_SCOPE)
  set(ANTLR4_FOUND ${ANTLR4_FOUND} PARENT_SCOPE)
  set(ANTLR4_VERSION ${ANTLR4_VERSION} PARENT_SCOPE)
  
  if(ANTLR4_TOOL_FOUND)
    set(ANTLR4_TOOL_FOUND ${ANTLR4_TOOL_FOUND} PARENT_SCOPE)
    set(ANTLR4_EXECUTABLE ${ANTLR4_EXECUTABLE} PARENT_SCOPE)
  endif()
  
  mark_as_advanced(ANTLR4_EXECUTABLE)
endif()

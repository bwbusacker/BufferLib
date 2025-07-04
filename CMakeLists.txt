cmake_minimum_required(VERSION 3.10)

# Project name and version
project(BufferLib VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard  
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set CMake policy for Boost
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find required packages
# Handle boost finding robustly across different systems
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# Try to find Boost
find_package(Boost REQUIRED COMPONENTS thread system)

# Debug information for boost
message(STATUS "Boost version: ${Boost_VERSION}")
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
message(STATUS "Boost thread library: ${Boost_THREAD_LIBRARY}")
message(STATUS "Boost system library: ${Boost_SYSTEM_LIBRARY}")

# Test boost installation
if(Boost_FOUND)
    message(STATUS "✅ Boost found successfully")
    message(STATUS "   Version: ${Boost_VERSION}")
    if(TARGET Boost::thread)
        message(STATUS "   ✅ Boost::thread target available")
    else()
        message(WARNING "   ❌ Boost::thread target NOT available")
    endif()
    if(TARGET Boost::system)
        message(STATUS "   ✅ Boost::system target available") 
    else()
        message(WARNING "   ❌ Boost::system target NOT available")
    endif()
else()
    message(FATAL_ERROR "❌ Boost NOT found - this will cause build failures")
endif()

# Define library sources
set(LIBRARY_SOURCES
    Buffer.cpp
    CompareResult.cpp
    Random32.cpp
    Utils.cpp
)

# Define library headers
set(LIBRARY_HEADERS
    Buffer.h
    CompareResult.h
    Random32.h
    Utils.h
    TypeDefs.h
    Errors.h
    Printable.h
)

# Create the library
add_library(BufferLib STATIC ${LIBRARY_SOURCES})

# Set library properties
set_target_properties(BufferLib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "buffer"
)

# Include directories
target_include_directories(BufferLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Link libraries
if(TARGET Boost::thread AND TARGET Boost::system)
    # Modern CMake approach with imported targets
    target_link_libraries(BufferLib PUBLIC 
        Boost::thread
        Boost::system
    )
    message(STATUS "Using modern Boost imported targets")
else()
    # Legacy approach using variables
    target_include_directories(BufferLib PUBLIC ${Boost_INCLUDE_DIRS})
    target_link_libraries(BufferLib PUBLIC ${Boost_LIBRARIES})
    message(STATUS "Using legacy Boost variables: ${Boost_LIBRARIES}")
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(BufferLib PRIVATE /W4)
else()
    target_compile_options(BufferLib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Code coverage option
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Add coverage flags
        target_compile_options(BufferLib PRIVATE --coverage -fprofile-arcs -ftest-coverage)
        target_link_libraries(BufferLib PRIVATE --coverage)
        
        # Set coverage flags for tests as well
        set(CMAKE_CXX_FLAGS_COVERAGE "--coverage -fprofile-arcs -ftest-coverage -g -O0")
        set(CMAKE_C_FLAGS_COVERAGE "--coverage -fprofile-arcs -ftest-coverage -g -O0")
        set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "--coverage")
        
        message(STATUS "Code coverage enabled")
    else()
        message(WARNING "Code coverage is only supported with GCC or Clang")
    endif()
endif()

# Enable OpenMP if available (used in Buffer.cpp)
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(BufferLib PRIVATE OpenMP::OpenMP_CXX)
endif()

# Optional: Build examples
option(BUILD_EXAMPLES "Build example programs" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Optional: Build tests
option(BUILD_TESTS "Build test programs" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    
    # Register tests with CTest
    add_test(NAME SimpleTests COMMAND simple_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    add_test(NAME UnitTests COMMAND unit_tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    add_test(NAME PerformanceTests COMMAND performance_tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
endif()

# Installation rules
install(TARGETS BufferLib
    EXPORT BufferLibTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(FILES ${LIBRARY_HEADERS}
    DESTINATION include/bufferlib
)

# Export targets
install(EXPORT BufferLibTargets
    FILE BufferLibTargets.cmake
    NAMESPACE BufferLib::
    DESTINATION lib/cmake/BufferLib
)

# Create and install config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    BufferLibConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/BufferLibConfig.cmake
    INSTALL_DESTINATION lib/cmake/BufferLib
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/BufferLibConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/BufferLibConfigVersion.cmake
    DESTINATION lib/cmake/BufferLib
) 
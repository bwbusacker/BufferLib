# Tests CMakeLists.txt

# Simple test
add_executable(simple_test simple_test.cpp)
target_link_libraries(simple_test PRIVATE BufferLib)

# Unit tests
add_executable(unit_tests unit_tests.cpp)
target_link_libraries(unit_tests PRIVATE BufferLib)

# Add coverage flags if enabled
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(simple_test PRIVATE --coverage -fprofile-arcs -ftest-coverage)
        target_link_libraries(simple_test PRIVATE --coverage)
        target_compile_options(unit_tests PRIVATE --coverage -fprofile-arcs -ftest-coverage)
        target_link_libraries(unit_tests PRIVATE --coverage)
    endif()
endif()

# Install tests
install(TARGETS simple_test unit_tests
    DESTINATION bin/tests
) 
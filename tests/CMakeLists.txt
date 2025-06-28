# Tests CMakeLists.txt

# Simple test
add_executable(simple_test simple_test.cpp)
target_link_libraries(simple_test PRIVATE BufferLib)

# Unit tests
add_executable(unit_tests unit_tests.cpp)
target_link_libraries(unit_tests PRIVATE BufferLib)

# Install tests
install(TARGETS simple_test unit_tests
    DESTINATION bin/tests
) 
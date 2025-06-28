# Examples CMakeLists.txt

# Basic usage example
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example PRIVATE BufferLib)

# Buffer operations example
add_executable(buffer_operations buffer_operations.cpp)
target_link_libraries(buffer_operations PRIVATE BufferLib)

# Install examples
install(TARGETS basic_example buffer_operations
    DESTINATION bin/examples
) 
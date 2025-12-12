cmake_minimum_required(VERSION 3.10)
project(merkle_tree CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Find required packages
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(merkle_tree main.cpp)

# Include directories
target_include_directories(merkle_tree PRIVATE ${ZLIB_INCLUDE_DIRS})
target_compile_definitions(merkle_tree PRIVATE NDEBUG)
# Link libraries
target_link_libraries(merkle_tree PRIVATE
	Threads::Threads
	${ZLIB_LIBRARIES}
	event
)

# Add pthread flags for compatibility
set_target_properties(merkle_tree PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

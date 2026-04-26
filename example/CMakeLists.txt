cmake_minimum_required(VERSION 4.2)
project(mongoose_cpp_example)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(.. mongoose_cpp)

add_executable(mongoose_cpp_example
        src/main.cpp
)

target_link_libraries(mongoose_cpp_example PRIVATE mongoose_cpp)

if (MSVC)
    target_compile_options(mongoose_cpp_example PRIVATE /EHsc /GR-)
else ()
    target_compile_options(mongoose_cpp_example PRIVATE -fno-exceptions -fno-rtti)
endif ()

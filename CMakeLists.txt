cmake_minimum_required(VERSION 4.2)
project(mongoose_cpp)

set(CMAKE_CXX_STANDARD 20)

add_library(mongoose_cpp STATIC
        vendors/mongoose/mongoose.c
        src/async_response.cpp
        src/request.cpp
        src/response.cpp
        src/server.cpp
)

target_include_directories(mongoose_cpp PUBLIC
        vendors/mongoose
        include/
)

if (MSVC)
    target_compile_options(mongoose_cpp PRIVATE /EHsc /GR-)
else ()
    target_compile_options(mongoose_cpp PRIVATE -fno-exceptions -fno-rtti)
endif ()

cmake_minimum_required(VERSION 4.2)
project(mongoose_cpp)

set(CMAKE_CXX_STANDARD 20)

##
# Options
#
set(MGPP_TLS "builtin" CACHE STRING "TLS backend to use: none, builtin, openssl, wolfssl")
set_property(CACHE MGPP_TLS PROPERTY STRINGS "none" "builtin" "openssl" "wolfssl")
string(TOLOWER "${MGPP_TLS}" MGPP_TLS_LOWER)

##
# Static library
#
add_library(${PROJECT_NAME} STATIC
        src/web/async_response.cpp
        src/web/request.cpp
        src/web/response.cpp
        src/web/web_endpoint.cpp

        src/endpoint.cpp
        src/server.cpp

        vendors/mongoose/mongoose.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
        src/
        src/web/

        vendors/mongoose/
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc /GR-)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions -fno-rtti)
endif ()

##
# Configure TLS
if (MGPP_TLS_LOWER STREQUAL "builtin")
    target_compile_definitions(${PROJECT_NAME} PUBLIC MG_TLS=MG_TLS_BUILTIN)

    if (WIN32)
        target_link_libraries(${PROJECT_NAME} PUBLIC crypt32 secur32 ws2_32)
    endif ()

    message(STATUS "TLS enabled (builtin)")
    #elseif (MGPP_TLS_LOWER STREQUAL "openssl")
    #    TODO: find package and use
    #
    #    message(STATUS "TLS enabled (OpenSSL)")
    #elseif (MGPP_TLS_LOWER STREQUAL "wolfssl")
    #    TODO: find package and use
    #
    #    message(STATUS "TLS enabled (wolfSSL)")
else ()
    target_compile_definitions(${PROJECT_NAME} PUBLIC MG_TLS=MG_TLS_NONE)

    message(STATUS "TLS is disabled")
endif ()
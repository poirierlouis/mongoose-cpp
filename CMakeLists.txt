cmake_minimum_required(VERSION 3.15)
project(mgxx)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##
# Options
#
set(MGXX_TLS "builtin" CACHE STRING "TLS backend to use: none, builtin, openssl, mbedtls")
set_property(CACHE MGXX_TLS PROPERTY STRINGS "none" "builtin" "openssl" "mbedtls")
string(TOLOWER "${MGXX_TLS}" MGXX_TLS_LOWER)

# TODO: add option/definition to let end-user disable exceptions

##
# Mongoose constants
#
set(MG_ENABLE_IPV6 "0" CACHE STRING "Mongoose: enable IPv6. (default is 0)")
set(MG_IO_SIZE "2048" CACHE STRING "Mongoose: granularity of the send/recv IO buffer growth. It also sets the maximum UDP message size. (default is 2048)")
set(MG_MAX_RECV_SIZE "3145728" CACHE STRING "Mongoose: maximum recv buffer size. If a connection's receive buffer grows more, the connection gets closed automatically. (default is 3145728)")
set(MG_MAX_HTTP_HEADERS "40" CACHE STRING "Mongoose: maximum number of HTTP headers. (default is 40)")

##
# Optionally build examples
#
set(MGXX_BUILD_EXAMPLES_DEFAULT OFF)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(MGXX_BUILD_EXAMPLES_DEFAULT ON)
endif()
option(MGXX_BUILD_EXAMPLES "Build examples" ${MGXX_BUILD_EXAMPLES_DEFAULT})

##
# Static library
#
add_library(${PROJECT_NAME} STATIC
        src/http/async_request.cpp
        src/http/async_response.cpp
        src/http/async_stream.cpp
        src/http/common.cpp
        src/http/endpoint.cpp
        src/http/headers.cpp
        src/http/remote_context.cpp
        src/http/request.cpp
        src/http/response.cpp

        src/endpoint.cpp
        src/server.cpp

        vendors/mongoose/mongoose.c
)
add_library(mgxx::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_include_directories(${PROJECT_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendors/mongoose/>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
        MG_ENABLE_IPV6=${MG_ENABLE_IPV6}
        MG_IO_SIZE=${MG_IO_SIZE}
        MG_MAX_RECV_SIZE=${MG_MAX_RECV_SIZE}
        MG_MAX_HTTP_HEADERS=${MG_MAX_HTTP_HEADERS}
)
if (MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _HAS_EXCEPTIONS=0)
    target_compile_options(${PROJECT_NAME} PRIVATE /GR- /EHs- /EHc- /Zc:throwingNew-)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions -fno-rtti)
endif ()

##
# Configure TLS
#
if (MGXX_TLS_LOWER STREQUAL "builtin")
    target_compile_definitions(${PROJECT_NAME} PUBLIC MG_TLS=MG_TLS_BUILTIN)

    message(STATUS "TLS enabled (builtin)")
elseif (MGXX_TLS_LOWER STREQUAL "openssl")
    target_compile_definitions(${PROJECT_NAME} PUBLIC MG_TLS=MG_TLS_OPENSSL)

    find_package(OpenSSL REQUIRED)

    target_link_libraries(${PROJECT_NAME} PUBLIC
            OpenSSL::SSL
            OpenSSL::Crypto)

    message(STATUS "TLS enabled (OpenSSL)")
elseif (MGXX_TLS_LOWER STREQUAL "mbedtls")
    target_compile_definitions(${PROJECT_NAME} PUBLIC MG_TLS=MG_TLS_MBED)

    find_package(MbedTLS REQUIRED)

    target_link_libraries(${PROJECT_NAME} PUBLIC
            MbedTLS::mbedtls
            MbedTLS::tfpsacrypto
            MbedTLS::mbedx509)

    message(STATUS "TLS enabled (Mbed-TLS)")
#elseif (MGXX_TLS_LOWER STREQUAL "wolfssl")
#    TODO: find package and use
#
#    message(STATUS "TLS enabled (wolfSSL)")
else ()
    target_compile_definitions(${PROJECT_NAME} PUBLIC MG_TLS=MG_TLS_NONE)

    message(STATUS "TLS is disabled")
endif ()

if (MGXX_BUILD_EXAMPLES)
    message(STATUS "Configuring examples...")
    add_subdirectory(example)
endif ()
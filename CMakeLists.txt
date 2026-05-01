cmake_minimum_required(VERSION 3.15)
project(mongoose_cpp)

set(CMAKE_CXX_STANDARD 20)

##
# Options
#
set(MGPP_TLS "builtin" CACHE STRING "TLS backend to use: none, builtin, openssl, mbedtls, wolfssl")
set_property(CACHE MGPP_TLS PROPERTY STRINGS "none" "builtin" "openssl" "mbedtls" "wolfssl")
string(TOLOWER "${MGPP_TLS}" MGPP_TLS_LOWER)

if("${CMAKE_PROJECT_NAME}" STREQUAL "mongoose_cpp")
    option(BUILD_EXAMPLES "Build examples" ON)
else()
    option(BUILD_EXAMPLES "Build examples" OFF)
endif()

##
# Static library
#
add_library(${PROJECT_NAME} STATIC
        src/web/async_request.cpp
        src/web/async_response.cpp
        src/web/request.cpp
        src/web/response.cpp
        src/web/web_endpoint.cpp

        src/endpoint.cpp
        src/remote_context.cpp
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

    message(STATUS "TLS enabled (builtin)")
elseif (MGPP_TLS_LOWER STREQUAL "openssl")
    target_compile_definitions(${PROJECT_NAME} PUBLIC MG_TLS=MG_TLS_OPENSSL)

    find_package(OpenSSL REQUIRED)

    target_link_libraries(${PROJECT_NAME} PUBLIC
            OpenSSL::SSL
            OpenSSL::Crypto)

    message(STATUS "TLS enabled (OpenSSL)")
elseif (MGPP_TLS_LOWER STREQUAL "mbedtls")
    target_compile_definitions(${PROJECT_NAME} PUBLIC MG_TLS=MG_TLS_MBED)

    find_package(MbedTLS REQUIRED)

    target_link_libraries(${PROJECT_NAME} PUBLIC
            MbedTLS::mbedtls
            MbedTLS::tfpsacrypto
            MbedTLS::mbedx509)

    message(STATUS "TLS enabled (Mbed-TLS)")
#elseif (MGPP_TLS_LOWER STREQUAL "wolfssl")
#    TODO: find package and use
#
#    message(STATUS "TLS enabled (wolfSSL)")
else ()
    target_compile_definitions(${PROJECT_NAME} PUBLIC MG_TLS=MG_TLS_NONE)

    message(STATUS "TLS is disabled")
endif ()

if (BUILD_EXAMPLES)
    message(STATUS "Configuring examples...")
    add_subdirectory(example)
endif ()
cmake_minimum_required(VERSION 3.15)
project(mgxx_example_simple)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
        src/main.cpp
)

set(MG_ENABLE_IPV6 "1" CACHE STRING "" FORCE)
set(MG_IO_SIZE "32768" CACHE STRING "" FORCE)

find_program(OPENSSL_BIN openssl)
if (OPENSSL_BIN)
    if(WIN32)
        set(TLS_SCRIPT "${CMAKE_SOURCE_DIR}/examples/tls.cmd")
    else()
        set(TLS_SCRIPT "${CMAKE_SOURCE_DIR}/examples/tls.sh")
    endif()

    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${TLS_SCRIPT}" "${OPENSSL_BIN}" "${CMAKE_SOURCE_DIR}/build"
            COMMENT "Generating self-signed certificates..."
    )
else ()
    message(WARNING "OpenSSL not found. Please generate certificates manually.")
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE mgxx::mgxx)
if (MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _HAS_EXCEPTIONS=0)
    target_compile_options(${PROJECT_NAME} PRIVATE /GR- /EHs- /EHc- /Zc:throwingNew-)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions -fno-rtti)
endif ()

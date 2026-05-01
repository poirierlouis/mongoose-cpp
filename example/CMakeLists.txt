cmake_minimum_required(VERSION 3.15)
project(mongoose_cpp_example)
set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME}
        src/main.cpp
)

find_program(OPENSSL_BIN openssl)
if (OPENSSL_BIN)
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${CMAKE_SOURCE_DIR}/example/tls.cmd" "${OPENSSL_BIN}" "${CMAKE_SOURCE_DIR}/build"
            COMMENT "Generating self-signed certificates..."
    )
    target_sources(${PROJECT_NAME} PRIVATE "${CERT_FILE}" "${KEY_FILE}")
else ()
    message(WARNING "OpenSSL not found. Please generate certificates manually.")
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE mongoose_cpp)
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc /GR-)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions -fno-rtti)
endif ()

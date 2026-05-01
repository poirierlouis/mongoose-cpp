cmake_minimum_required(VERSION 4.2)
project(mongoose_cpp_example)
set(CMAKE_CXX_STANDARD 20)

#set(MGPP_TLS "builtin" CACHE STRING "" FORCE)
add_subdirectory(.. mongoose_cpp)

add_executable(${PROJECT_NAME}
        src/main.cpp
)

find_program(OPENSSL_BIN openssl)
if (OPENSSL_BIN)
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${CMAKE_SOURCE_DIR}/tls.cmd" "${OPENSSL_BIN}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Generating self-signed certificates...\n${CMAKE_SOURCE_DIR}/tls.cmd\n${OPENSSL_BIN}"
    )
    target_sources(${PROJECT_NAME} PRIVATE "${CERT_FILE}" "${KEY_FILE}")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_TLS)
else ()
    message(WARNING "OpenSSL not found. Please generate certificates manually.")
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE mongoose_cpp)
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc /GR-)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions -fno-rtti)
endif ()

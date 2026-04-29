cmake_minimum_required(VERSION 4.2)
project(mongoose_cpp_example)
set(CMAKE_CXX_STANDARD 20)

#set(MGPP_TLS "builtin" CACHE STRING "" FORCE)
add_subdirectory(.. mongoose_cpp)

add_executable(${PROJECT_NAME}
        src/main.cpp
)

find_program(OPENSSL_EXECUTABLE openssl)
if (OPENSSL_EXECUTABLE)
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${OPENSSL_EXECUTABLE} req -x509 -newkey rsa:4096
            -keyout "$<TARGET_FILE_DIR:${PROJECT_NAME}>/key.pem"
            -out "$<TARGET_FILE_DIR:${PROJECT_NAME}>/cert.pem"
            -sha512 -days 365 -nodes -subj "/CN=localhost"
            COMMENT "Generating self-signed certificate..."
    )
    target_sources(${PROJECT_NAME} PRIVATE "${CERT_FILE}" "${KEY_FILE}")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_TLS)
else ()
    message(WARNING "OpenSSL not found: running examples with TLS disabled.")
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE mongoose_cpp)
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc /GR-)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions -fno-rtti)
endif ()

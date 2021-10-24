project(rusync_server_tests)

add_executable(${PROJECT_NAME} BinaryWriterTests.cpp)

target_link_directories(${PROJECT_NAME} PUBLIC 
    ${CONAN_LIB_DIRS_GTEST})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_ROOT}/common
    ${PROJECT_ROOT}/server/src
    ${CONAN_INCLUDE_DIRS_GTEST})

target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS_GTEST})

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

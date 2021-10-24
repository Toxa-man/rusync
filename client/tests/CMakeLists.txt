project(rusync_client_tests)

add_executable(${PROJECT_NAME} BinaryParserTests.cpp UtilTests.cpp)

target_link_directories(${PROJECT_NAME} PUBLIC 
    ${CONAN_LIB_DIRS_GTEST})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_ROOT}/common
    ${PROJECT_ROOT}/client/src
    ${CONAN_INCLUDE_DIRS_GTEST})

target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS_GTEST})

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

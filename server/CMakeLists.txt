project (rusync_server)


# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

add_subdirectory(tests)

file(GLOB SOURCE_FILES 
        "src/main.cpp"
        "src/ServerSync.cpp"
        "${PROJECT_ROOT}/common/DirEntry.cpp")
add_executable(${PROJECT_NAME} ${SOURCE_FILES} )
target_link_directories(${PROJECT_NAME} PUBLIC 
    ${CONAN_LIB_DIRS_LIBNGHTTP2}
    ${CONAN_LIB_DIRS_BOOST} 
    ${CONAN_LIB_DIRS_OPENSSL} 
    ${CONAN_LIB_DIRS_XXHASH})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_ROOT}/common
    ${CONAN_INCLUDE_DIRS_LIBNGHTTP2}
    ${CONAN_INCLUDE_DIRS_BOOST} 
    ${CONAN_INCLUDE_DIRS_XXHASH}
    ${CONAN_INCLUDE_DIRS_RAPIDJSON})
target_link_libraries(${PROJECT_NAME}
    ${CONAN_PKG_LIBS_XXHASH}
    ${CONAN_LIBS_LIBNGHTTP2} 
    boost_system
    boost_thread
    ${CONAN_PKG_LIBS_OPENSSL}
    pthread
    dl
)

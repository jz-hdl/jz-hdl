cmake_minimum_required(VERSION 3.16)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" JZ_HDL_VERSION)
string(STRIP "${JZ_HDL_VERSION}" JZ_HDL_VERSION)
if(NOT JZ_HDL_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must contain MAJOR.MINOR.PATCH")
endif()

project(jz_hdl VERSION "${JZ_HDL_VERSION}" LANGUAGES C CXX)

include(CTest)

include(FetchContent)
FetchContent_Declare(
    sqlite3
    URL https://www.sqlite.org/2025/sqlite-amalgamation-3490100.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(sqlite3)

add_library(jz_sqlite3_lib STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(jz_sqlite3_lib PUBLIC ${sqlite3_SOURCE_DIR})
set_target_properties(jz_sqlite3_lib PROPERTIES C_EXTENSIONS ON C_STANDARD 99)
target_compile_options(jz_sqlite3_lib PRIVATE -w)
target_compile_definitions(jz_sqlite3_lib PRIVATE _BSD_SOURCE)

find_package(Threads)
if(Threads_FOUND)
    target_link_libraries(jz_sqlite3_lib PUBLIC Threads::Threads)
endif()

include(CheckLibraryExists)
check_library_exists(dl dlopen "" HAVE_LIBDL)
if(HAVE_LIBDL)
    target_link_libraries(jz_sqlite3_lib PUBLIC dl)
endif()

set(CPACK_PACKAGE_NAME "jz-hdl")
set(CPACK_PACKAGE_VENDOR "jz-hdl")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "jz-hdl-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")

set(CPACK_GENERATOR "TGZ")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL Compiler Viewer Docs)
set(CPACK_COMPONENT_COMPILER_DISPLAY_NAME "JZ-HDL Compiler")
set(CPACK_COMPONENT_VIEWER_DISPLAY_NAME "JZ-HDL Viewer")
set(CPACK_COMPONENT_DOCS_DISPLAY_NAME "JZ-HDL Specifications")

set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "jz-hdl-${PROJECT_VERSION}-source")
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/release/"
    "/\\.git/"
    "/\\.github/"
    "/\\.DS_Store"
)

add_subdirectory(compiler)
add_subdirectory(specification)
add_subdirectory(viewer)

include(CPack)

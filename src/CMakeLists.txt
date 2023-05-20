cmake_minimum_required(VERSION 2.6)
set(PROJECT_NAME cgpt)
set(RELEASE_DATE 2023-04-09)
project(${PROJECT_NAME})
enable_testing()

# Selects which programming languages are needed to build the project.
project(cgpt LANGUAGES C)

option(CLOG_OPTION "This is log option" OFF)
if(CLOG_OPTION)
    add_compile_definitions(__GPTCLOG__)
endif()
# Appends elements to the list. If no variable named <list> exists 
# in the current scope its value is treated as empty and the elements
# are appended to that empty list.
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CMakePackageConfigHelpers)
include(CMakeDependentOption)
include(CheckCCompilerFlag)
include(GNUInstallDirs)
include(CTest)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD 99)

set(gpt_src
    src/cJSON.c
    src/gpt_linenoise.c
    src/gpt_common.c
    src/gpt_json.c
    src/gpt_log.c
    src/gpt_main.c
)

add_executable(cgpt ${gpt_src})
target_include_directories(cgpt
    PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
set_target_properties(cgpt PROPERTIES OUTPUT_NAME "cgpt")
target_link_libraries(cgpt -lm -lpthread)
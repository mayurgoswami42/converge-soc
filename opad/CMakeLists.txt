cmake_minimum_required(VERSION 3.28.3)
project(OPAD VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/..)

set(ABSL_ENABLE_INSTALL ON CACHE BOOL "" FORCE)
set(RE2_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(${PROJECT_ROOT}/external/abseil-cpp ${CMAKE_BINARY_DIR}/external/abseil-cpp)
add_subdirectory(${PROJECT_ROOT}/external/re2 ${CMAKE_BINARY_DIR}/external/re2)

add_library(utils_lib utils/utils.cpp)
add_library(io_manager_lib io_manager/reader.cpp)
add_library(parser_lib parser/parser.cpp)
add_library(detector_lib detector/detector.cpp)
add_library(application_lib application/application.cpp)

target_include_directories(utils_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(io_manager_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(parser_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(detector_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(application_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(parser_lib PUBLIC re2)
target_link_libraries(application_lib PUBLIC io_manager_lib parser_lib detector_lib utils_lib)

add_executable(opad main.cpp)
target_link_libraries(opad PRIVATE application_lib)
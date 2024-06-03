cmake_minimum_required(VERSION 3.10)

project(NebulaEmu)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src src)

add_executable(${PROJECT_NAME} ${src})

target_include_directories(${PROJECT_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} SDL2)

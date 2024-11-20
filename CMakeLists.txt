cmake_minimum_required(VERSION 3.10)

project(NebulaEmu)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src src)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
  if(NOT $ENV{SDL2_INCLUDE} STREQUAL "")
    include_directories($ENV{SDL2_INCLUDE})
  else()
    message(
      FATAL_ERROR
        "can not find SDL2_INCLUDE, please create an environment variable named SDL2_INCLUDE for the SDL2 header file directory"
    )
  endif()

  if(NOT $ENV{SDL2_LIB} STREQUAL "")
    link_directories($ENV{SDL2_LIB})
  else()
    message(
      FATAL_ERROR
        "can not find SDL2_LIB, please create an environment variable named SDL2_LIB for the libSDL2.dll file directory"
    )
  endif()

endif()

add_executable(${PROJECT_NAME} ${src})

target_link_libraries(${PROJECT_NAME} SDL2)

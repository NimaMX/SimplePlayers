cmake_minimum_required(VERSION 3.22.1)

project(WavPlayer)

set(SRC_FILES main.cpp)

set(LINK_SYMS asound)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} ${LINK_SYMS})

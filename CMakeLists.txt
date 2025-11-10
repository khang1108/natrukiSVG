cmake_minimum_required(VERSION 3.16)

project(NaTruKi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Settings for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Network)

add_subdirectory(src/svg)
add_subdirectory(src/ui)

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME}
    svg_lib
    ui_lib
    Qt6::Widgets
    Qt6::Core
    Qt6::Network
)
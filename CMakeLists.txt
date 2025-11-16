cmake_minimum_required(VERSION 3.16)

project(NaTruKi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compile_commands.json generation for clangd and other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#* Settings for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 packages (fixed typo: Networks -> Network)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Network)

# Add subdirectories (this will create svg_lib and ui_lib)
add_subdirectory(src/svg)
add_subdirectory(src/ui)

# Main executable
add_executable(${PROJECT_NAME}
    src/ui/main.cpp
    apps/mainwindow.ui
    resources/resources.qrc
)

# Link libraries to main executable
target_link_libraries(${PROJECT_NAME}
    svg_lib
    ui_lib
    Qt6::Widgets
    Qt6::Core
    Qt6::Network
)

# Include directories for main executable
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/svg
    ${CMAKE_SOURCE_DIR}/include/ui
    ${CMAKE_SOURCE_DIR}/third-party/rapidxml
    ${Qt6Widgets_INCLUDE_DIRS}
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Network_INCLUDE_DIRS}
)
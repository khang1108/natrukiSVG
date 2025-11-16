# Collect all source files in this directory
file(GLOB SVG_SOURCES *.cpp)

# Create static library
add_library(svg_lib STATIC ${SVG_SOURCES})

# Link Qt6 libraries
target_link_libraries(svg_lib
    Qt6::Widgets
    Qt6::Core
)

# Include directories for svg_lib
# PUBLIC means these directories will be propagated to targets that link to svg_lib
target_include_directories(svg_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/svg
    ${PROJECT_SOURCE_DIR}/third-party/rapidxml
)
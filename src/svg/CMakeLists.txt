file(GLOB SVG_SOURCES *.cpp)

add_library(svg_lib STATIC ${SVG_SOURCES})

target_link_libraries(svg_lib
    Qt6::Widgets
    Qt6::Core
)

target_include_directories(svg_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/include/svg
)
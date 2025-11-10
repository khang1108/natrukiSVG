# 1. Liệt kê TẤT CẢ các file .cpp của bạn
set(SVG_SOURCES
    SVGDocument.cpp
    SVGElement.cpp
    SVGStyle.cpp
    SVGTransform.cpp
    SVGFactory.cpp
    SVGGroup.cpp
    SVGRect.cpp
    SVGCircle.cpp
    SVGEllipse.cpp
    SVGLine.cpp
    SVGPolygon.cpp
    SVGPolyline.cpp
    SVGText.cpp
)

# 2. Tạo thư viện
add_library(svg_lib STATIC ${SVG_SOURCES})

# 3. Liên kết Qt
target_link_libraries(svg_lib
    PRIVATE
        Qt6::Widgets
        Qt6::Core
)

# 4. Chỉ định các thư mục "include"
target_include_directories(svg_lib 
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include/svg
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third-party/rapidxml
)
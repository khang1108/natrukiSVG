file(GLOB UI_SOURCES *.cpp)

add_library(ui_lib STATIC ${UI_SOURCES})

target_link_libraries(ui_lib 
    Qt6::Widgets
    #Qt6::Network
    Qt6::Core
)

target_include_directories(ui_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/third-party/rapidxml  # <--- SỬA LỖI Ở ĐÂY
)
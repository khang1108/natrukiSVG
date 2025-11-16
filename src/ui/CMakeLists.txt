# Collect all source files in this directory
file(GLOB UI_SOURCES *.cpp)

# Collect header files that need MOC processing
set(UI_HEADERS_FOR_MOC
    ${PROJECT_SOURCE_DIR}/include/ui/mainwindow.h
)

# Manually wrap the UI file to generate ui_mainwindow.h
qt6_wrap_ui(UI_HEADERS ${PROJECT_SOURCE_DIR}/apps/mainwindow.ui)

# Create static library
# Include headers so MOC can process them
add_library(ui_lib STATIC 
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${UI_HEADERS_FOR_MOC}
)

# Disable AUTOUIC for this target to prevent conflicts
# But keep AUTOMOC enabled (it's needed for Q_OBJECT macro)
set_target_properties(ui_lib PROPERTIES
    AUTOUIC OFF
    AUTOMOC ON
)

# Link libraries
# ui_lib depends on svg_lib to use SVG classes
target_link_libraries(ui_lib 
    svg_lib
    Qt6::Widgets
    #Qt6::Network
    Qt6::Core
)

# Include directories for ui_lib
# PUBLIC means these directories will be propagated to targets that link to ui_lib
target_include_directories(ui_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/ui
    ${PROJECT_SOURCE_DIR}/include/svg
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/ui_lib_autogen/include
)
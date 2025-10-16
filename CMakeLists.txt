cmake_minimum_required(VERSION 3.21.1)
project(UniversityCAD LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(UniversityCAD)

target_sources (UniversityCAD PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc

    ${CMAKE_CURRENT_SOURCE_DIR}/ui/CadWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/CadWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/windows/Control.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/windows/Control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/windows/Properties.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/windows/Properties.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/windows/Viewport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/windows/Viewport.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/draw/Draw.h
    ${CMAKE_CURRENT_SOURCE_DIR}/draw/SegmentDraw.h
    ${CMAKE_CURRENT_SOURCE_DIR}/draw/SegmentDraw.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/core/Enums.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Scene.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/objects/Object.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/objects/Point.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/objects/Point.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/objects/Segment.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/objects/Segment.cpp
)

target_include_directories(UniversityCAD PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/core/objects

    ${CMAKE_CURRENT_SOURCE_DIR}/draw

    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/windows
)

target_link_libraries(UniversityCAD PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Svg
)

include(GNUInstallDirs)

install(TARGETS UniversityCAD
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET UniversityCAD
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

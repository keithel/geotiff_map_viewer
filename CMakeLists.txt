cmake_minimum_required(VERSION 3.16)

project(geotiff_viewer VERSION 0.1 LANGUAGES CXX)
set(PROJECT_BINARY_NAME "app${CMAKE_PROJECT_NAME}")

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "$ENV{HOME}" ${CMAKE_PREFIX_PATH})

find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIRS})

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(${PROJECT_BINARY_NAME}
    main.cpp
)

qt_add_qml_module(${PROJECT_BINARY_NAME}
    URI geotiff_viewer
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        geotiffhandler.h
        geotiffhandler.cpp
        SOURCES geotiffhandler.h geotiffhandler.cpp
)

# Leave for image resources, etc.
qt_add_resources(${PROJECT_BINARY_NAME} "resources"
    PREFIX "/"
    FILES
)

add_custom_target(related-files
    SOURCES
        README.md
        LICENSE
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(${PROJECT_BINARY_NAME} PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${PROJECT_BINARY_NAME}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(${PROJECT_BINARY_NAME}
    PRIVATE Qt6::Quick
    PRIVATE ${GDAL_LIBRARIES}
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_BINARY_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

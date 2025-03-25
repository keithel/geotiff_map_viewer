cmake_minimum_required(VERSION 3.16)

project(geotiff_viewer VERSION 0.1 LANGUAGES CXX)
set(PROJECT_BINARY_NAME "app${CMAKE_PROJECT_NAME}")

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the home directory to the CMAKE_PREFIX_PATH so that libraries installed with PREFIX set to
# home will be found. In my case, that is where I installed GDAL to.
set(CMAKE_PREFIX_PATH "$ENV{HOME}" ${CMAKE_PREFIX_PATH})

find_package(Qt6 REQUIRED COMPONENTS Quick Positioning Location HttpServer)
find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIRS})

qt_standard_project_setup(REQUIRES 6.5)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

qt_add_executable(${PROJECT_BINARY_NAME}
    src/main.cpp
)

set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)

qt_add_qml_module(${PROJECT_BINARY_NAME}
    URI geotiff_viewer
    VERSION 1.0
    QML_FILES
        qml/Main.qml
    SOURCES
        src/geotiffhandler.h
        src/geotiffhandler.cpp
        src/geotiffimageprovider.h
        src/geotiffimageprovider.cpp
        src/appconfig.h
        src/appconfig.cpp
        src/thunderforestconfigserver.h
        src/thunderforestconfigserver.cpp
        src/geotiffoverlay.h
        src/geotiffoverlay.cpp
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
    PRIVATE Qt6::Quick Qt::Positioning Qt::Location Qt::HttpServer Qt::LocationPrivate
    PRIVATE ${GDAL_LIBRARIES}
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_BINARY_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

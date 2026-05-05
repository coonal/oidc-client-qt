cmake_minimum_required(VERSION 3.16)

project(oidc-client-qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use Qt5 with explicit search path
# set(Qt5_DIR "/home/coonal/Qt/5.15.2/gcc_64/lib/cmake/Qt5")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets Network)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets Network)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    oidcconfig.cpp
    oidcconfig.h
    logindialog.cpp
    logindialog.h
    simplelogindialog.cpp
    simplelogindialog.h
    authcodedialog.cpp
    authcodedialog.h
)

add_executable(oidc-client-qt
    ${PROJECT_SOURCES}
)

target_link_libraries(oidc-client-qt PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets 
    Qt${QT_VERSION_MAJOR}::Network)

# Add link search path for Qt libraries and system libraries
# target_link_directories(oidc-client-qt PRIVATE /home/coonal/Qt/5.15.2/gcc_64/lib)

# Set RPATH
set_target_properties(oidc-client-qt PROPERTIES
    # BUILD_RPATH "/home/coonal/Qt/5.15.2/gcc_64/lib"
    # INSTALL_RPATH "/home/coonal/Qt/5.15.2/gcc_64/lib"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS oidc-client-qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

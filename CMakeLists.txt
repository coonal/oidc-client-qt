cmake_minimum_required(VERSION 3.16)

project(oidc-client-qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
#set(CMAKE__GLIBCXX_USE_CXX11_ABI=0)
#set(CMAKE_CXX_COMPILER_ABI OFF)

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

target_link_libraries(oidc-client-qt PRIVATE stdc++)

#target_compile_definitions(oidc-client-qt PUBLIC _GLIBCXX_USE_CXX11_ABI=0)
#target_compile_definitions(oidc-client-qt PUBLIC __LINUX__=1)

#set_target_properties(oidc-client-qt PROPERTIES
#    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
#    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
#    MACOSX_BUNDLE TRUE
#    WIN32_EXECUTABLE TRUE
#)

include(GNUInstallDirs)
#install(TARGETS oidc-client-qt
#    BUNDLE DESTINATION .
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#)

#
cmake_minimum_required(VERSION 3.16)

project(controller_qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Charts SerialPort)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts SerialPort)
set(PROJECT_SOURCES

        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        core.cpp
        core.h
        qtmodbus.cpp
        qtmodbus.h
        circleindicator.h
        circleindicator.cpp
        signalprocessor.cpp
        signalprocessor.h
        chart.cpp
        chart.h
        mychartview.cpp
        mychartview.h
        serialib.cpp
        serialib.h
        Section.cpp
        Section.h

)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    qt_add_executable(controller_qt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        circleindicator.h circleindicator.cpp
        signalprocessor.h signalprocessor.cpp
        chart.h chart.cpp
        mychartview.h mychartview.cpp
        Section.h Section.cpp
        common_macro.h



    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET controller_qt APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(controller_qt SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(controller_qt
            ${PROJECT_SOURCES}
        )
    endif()
endif()

include_directories($ENV{HOME}/mingw64-prefix/include)
link_directories($ENV{HOME}/mingw64-prefix)

target_link_libraries(controller_qt PRIVATE Qt6::Widgets Qt6::Charts Qt6::SerialPort)

target_link_libraries(controller_qt PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
    modbus
)
set_target_properties(controller_qt PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "/usr/local/lib"
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.controller_qt)
endif()
set_target_properties(controller_qt PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS controller_qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include_directories("/usr/local/include")
include_directories(${CMAKE_SOURCE_DIR})

link_directories("/usr/local/lib")
set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}    # по умолчанию
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(controller_qt)
endif()

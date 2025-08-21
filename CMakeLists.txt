cmake_minimum_required(VERSION 3.16)

project(controller_qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt настройки
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Поиск Qt компонентов
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Charts SerialPort)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts SerialPort)

# Автоматический сбор всех исходных файлов
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "*.cpp"
    "*.h"
    "*.ui"
)

# Исключить файлы сборки
list(FILTER PROJECT_SOURCES EXCLUDE REGEX "build/.*")

# Создание исполняемого файла
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(controller_qt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(controller_qt SHARED ${PROJECT_SOURCES})
    else()
        add_executable(controller_qt ${PROJECT_SOURCES})
    endif()
endif()

target_include_directories(controller_qt PRIVATE ${CMAKE_SOURCE_DIR}/gui)

# Подключение библиотек
target_link_libraries(controller_qt PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::SerialPort
)


# Настройки для разных платформ
if(MINGW OR WIN32)
    # Пути для MinGW (если используется)
    if(EXISTS "$ENV{HOME}/mingw64-prefix")
        include_directories($ENV{HOME}/mingw64-prefix/include)
        link_directories($ENV{HOME}/mingw64-prefix)
    endif()
endif()

# Общие пути
include_directories(${CMAKE_SOURCE_DIR})
if(EXISTS "/usr/local/include")
    include_directories("/usr/local/include")
    link_directories("/usr/local/lib")
endif()

# Настройки RPATH для Linux
if(UNIX AND NOT APPLE)
    set_target_properties(controller_qt PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "/usr/local/lib"
    )
endif()

# Настройки для macOS/iOS
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

# Настройки для поиска UI файлов
set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Установка
include(GNUInstallDirs)
install(TARGETS controller_qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Финализация для Qt6
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(controller_qt)
endif()

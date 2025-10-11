cmake_minimum_required(VERSION 3.16)

project(GUI-PROFILE VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- MENSAJE DE DEPURACIÓN 1 ---
message(STATUS "DEBUG: Estoy a punto de buscar los paquetes de Qt...")

# Módulos Qt necesarios: Widgets, Network y Charts
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Charts)

# --- MENSAJE DE DEPURACIÓN 2 ---
message(STATUS "DEBUG: Búsqueda de paquetes finalizada. Versión de Qt encontrada: ${QT_VERSION_MAJOR}")

set(PROJECT_SOURCES
    # Forms
        mainwindow.ui

    # Header Files
        ListaGuardado.h
        mainwindow.h
        memory_instrumentation.h
        ServidorSocket.h

    # Source Files
        main.cpp
        mainwindow.cpp
        memory_instrumentation.cpp
        ListaGuardado.cpp
        ServidorSocket.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(GUI-PROFILE
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(GUI-PROFILE SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(GUI-PROFILE
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(GUI-PROFILE PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Charts)

# ... (el resto del archivo no cambia) ...

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.GUI-PROFILE)
endif()
set_target_properties(GUI-PROFILE PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS GUI-PROFILE
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(GUI-PROFILE)
endif()


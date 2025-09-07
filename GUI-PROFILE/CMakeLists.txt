cmake_minimum_required(VERSION 3.16)

project(GUI-PROFILE VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt para la GUI (incluye Charts y Network)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Charts Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts Network)

# ===== Librería de instrumentación (no depende de Qt) =====
add_library(memprof_runtime STATIC
    memory_instrumentation.cpp
)
target_include_directories(memprof_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ws2_32 para sockets en Windows (MinGW/MSVC)
if (WIN32)
  target_link_libraries(memprof_runtime PRIVATE ws2_32)
endif()

# ===== GUI (servidor de sockets) =====
add_executable(GUI-PROFILE
    main.cpp
    mainwindow.h
    mainwindow.cpp
    mainwindow.ui
    ListaGuardado.h
    memory_instrumentation.h
)
target_link_libraries(GUI-PROFILE
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Charts
        Qt${QT_VERSION_MAJOR}::Network
        memprof_runtime
)
set_target_properties(GUI-PROFILE PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
if(QT_VERSION_MAJOR EQUAL 6)
  qt_finalize_executable(GUI-PROFILE)
endif()

# ===== Programa medible externo (cliente de sockets) =====
add_executable(MemTargetDemo
    demo_app.cpp
    ListaGuardado.h
    memory_instrumentation.h
)
target_link_libraries(MemTargetDemo PRIVATE memprof_runtime)

include(GNUInstallDirs)
install(TARGETS GUI-PROFILE MemTargetDemo
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE  DESTINATION .
)




# CMakeLists.txt para el Programa de Prueba

cmake_minimum_required(VERSION 3.16)
project(ProgramaDePrueba LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontrar las dependencias de Qt
find_package(Qt6 REQUIRED COMPONENTS Core Network)

# --- CONFIGURACIÓN DE RUTAS ---
# Aquí le decimos a CMake dónde encontrar los archivos de tu biblioteca.

# 1. Ruta a la carpeta que contiene los archivos .h (la raíz de tu proyecto de librería)
set(PROFILER_INCLUDE_DIR "C:/Users/luisa/Documents/GitHub/Proyecto1_Datos22/Memory-lib")

# 2. Ruta a la carpeta que contiene el archivo .a compilado
set(PROFILER_LIBRARY_DIR "C:/Users/luisa/Documents/GitHub/Proyecto1_Datos22/Memory-lib/build/Desktop_Qt_6_9_2_MinGW_64_bit-Debug")

# --- FIN DE LA CONFIGURACIÓN ---

# Creamos el programa ejecutable a partir de su main.cpp
add_executable(ProgramaDePrueba main.cpp)

# Le decimos al compilador que busque los archivos #include en la carpeta de tu librería
target_include_directories(ProgramaDePrueba PUBLIC ${PROFILER_INCLUDE_DIR})

# Le decimos al enlazador (linker) que busque el archivo .a en la carpeta de compilación de tu librería
target_link_directories(ProgramaDePrueba PUBLIC ${PROFILER_LIBRARY_DIR})

# Finalmente, enlazamos nuestro programa con la biblioteca "ProfilerLib" y con Qt
target_link_libraries(ProgramaDePrueba PUBLIC ProfilerLib Qt6::Core Qt6::Network)

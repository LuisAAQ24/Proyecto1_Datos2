#ifndef MEMORY_INSTRUMENTATION_H
#define MEMORY_INSTRUMENTATION_H


#include "ListaGuardado.h"
#include <vector>
#include <iostream>
#include "ServidorSocket.h"


// Servidor global (opcional, puntero)
extern ServidorSocket* servidorSocket;


// Flag global para activar profiler
extern bool profilerActivo;


// Funciones para JSON y reporte
void guardarReporteJSON();
void reporteAlSalir();


// Sobrecarga global de new/delete (declaraciones)
// Versión con file/line que será invocada por el macro `new`
void* operator new(std::size_t tamano, const char* file, int line);
void* operator new[](std::size_t tamano, const char* file, int line);


// Versiones "fallback" sin file/line (para librerías, STL, etc.)
void* operator new(std::size_t tamano);
void* operator new[](std::size_t tamano);


// delete normales
void operator delete(void* direccion) noexcept;
void operator delete(void* direccion, std::size_t) noexcept;
void operator delete[](void* direccion) noexcept;
void operator delete[](void* direccion, std::size_t) noexcept;


// Macro para capturar archivo y línea: solo se define si NO_TRACK_NEW no está definido.
// De esta forma los archivos que definen el propio sistema de tracking pueden
// definir NO_TRACK_NEW antes de incluir este header para evitar recursión del macro.
#ifndef NO_TRACK_NEW
#define new new(__FILE__, __LINE__)
#endif


#endif // MEMORY_INSTRUMENTATION_H


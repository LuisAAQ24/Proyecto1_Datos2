#ifndef MEMORY_INSTRUMENTATION_H
#define MEMORY_INSTRUMENTATION_H

#include "ListaGuardado.h"
#include <vector>
#include <iostream>

// Flag global para activar profiler
extern bool profilerActivo;

// Funciones para JSON y reporte
void guardarReporteJSON();
void reporteAlSalir();

// Sobrecarga global de new/delete
void* operator new(std::size_t tamano);
void operator delete(void* direccion) noexcept;
void* operator new[](std::size_t tamano);
void operator delete[](void* direccion) noexcept;

#endif


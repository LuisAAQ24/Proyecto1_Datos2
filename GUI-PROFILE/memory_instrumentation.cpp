#include "ListaGuardado.h"
#include <iostream>
#include <cstdlib>

// Definición de la lista global
ListaGuardado listaGlobal;

// Flag para activar profiler solo después de inicializar Qt
bool profilerActivo = false;

// Sobrecarga de new/delete
void* operator new(std::size_t tamano) {
    void* direccion = std::malloc(tamano);
    if (!direccion) throw std::bad_alloc();
    if (profilerActivo)
        listaGlobal.agregar(direccion, tamano, "desconocido");
    return direccion;
}

void operator delete(void* direccion) noexcept {
    if (profilerActivo)
        listaGlobal.eliminar(direccion);
    std::free(direccion);
}

void* operator new[](std::size_t tamano) {
    void* direccion = std::malloc(tamano);
    if (!direccion) throw std::bad_alloc();
    if (profilerActivo)
        listaGlobal.agregar(direccion, tamano, "arreglo");
    return direccion;
}

void operator delete[](void* direccion) noexcept {
    if (profilerActivo)
        listaGlobal.eliminar(direccion);
    std::free(direccion);
}

// Reporte automático de fugas
void reporteAlSalir() {
    listaGlobal.reportarFugas();
}







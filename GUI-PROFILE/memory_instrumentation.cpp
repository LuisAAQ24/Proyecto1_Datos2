#include "ListaGuardado.h"
#include "memory_instrumentation.h"
#include <cstdlib>
#include <new>
#include <string>

ListaGuardado listaGlobal;
bool profilerActivo = false;


// Helper para obtener nombre de archivo
static std::string nombreArchivo(const char* rutaCompleta) {
    std::string ruta(rutaCompleta);
    size_t pos = ruta.find_last_of("/\\");
    if (pos != std::string::npos) return ruta.substr(pos + 1);
    return ruta;
}

#include "ListaGuardado.h"
#include <cstdlib>
#include <new>
#include <iostream>

// Variables globales de control
extern bool profilerActivo; // ya la tienes en memory_instrumentation.h

// --------------------------
// operator new
// --------------------------
void* operator new(std::size_t tamano) {
    void* ptr = std::malloc(tamano);
    if (!ptr) throw std::bad_alloc();
    if (profilerActivo) {
        std::cout << "[NEW] ptr=" << ptr << " size=" << tamano << "\n";
        listaGlobal.agregar(ptr, tamano, "new", "");
    }
    return ptr;
}
void operator delete(void* direccion, std::size_t) noexcept {
    if (profilerActivo && direccion) {
        std::cout << "[DELETE sized] ptr=" << direccion << "\n";
        listaGlobal.eliminar(direccion);
    }
    std::free(direccion);
}



// --------------------------
// operator new[] (arrays)
// --------------------------
void* operator new[](std::size_t tamano) {
    void* ptr = std::malloc(tamano);
    if (!ptr) throw std::bad_alloc();

    if (profilerActivo) {
        listaGlobal.agregar(ptr, tamano, "new[]", "");
    }
    return ptr;
}


void operator delete[](void* direccion, std::size_t) noexcept {
    if (profilerActivo && direccion) {
        std::cout << "[DELETE[] sized] ptr=" << direccion << "\n";
        listaGlobal.eliminar(direccion);
    }
    std::free(direccion);
}




// =====================
// Funciones de reporte
// =====================
void guardarReporteJSON() {
    auto fugas = listaGlobal.reportLeaks();
    listaGlobal.exportJSON(fugas);
}

void reporteAlSalir() {
    auto fugas = listaGlobal.reportLeaks();
    if (!fugas.empty()) {
        std::cout << "⚠️ Se detectaron fugas de memoria: " << fugas.size() << std::endl;
    } else {
        std::cout << "✅ No se detectaron fugas de memoria." << std::endl;
    }
}


















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

// Sobrecarga de new/delete
void* operator new(std::size_t tamano) {
    void* direccion = std::malloc(tamano);
    if (!direccion) throw std::bad_alloc();

    if (profilerActivo) {
        try {
            listaGlobal.agregar(direccion, tamano, "desconocido", nombreArchivo(__FILE__));
        } catch (...) {
            std::cerr << "⚠️ Error al agregar nodo en ListaGuardado\n";
        }
    }

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
        listaGlobal.agregar(direccion, tamano, "arreglo", nombreArchivo(__FILE__));
    return direccion;
}

void operator delete[](void* direccion) noexcept {
    if (profilerActivo)
        listaGlobal.eliminar(direccion);
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


















#include "ListaGuardado.h"
#include "memory_instrumentation.h"
#include <QTcpSocket>

#include <cstdlib>
#include <new>
#include <string>
#include <sstream>

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
void enviarResumenSocket() {
    auto fugas = listaGlobal.reportLeaks();
    std::string json = "{\"leaks\":[";
    for (size_t i = 0; i < fugas.size(); ++i) {
        const auto& f = fugas[i];

        // convertir el puntero a string (ej. "0x7ffee1234")
        std::ostringstream oss;
        oss << f.direccion;

        json += "{";
        json += "\"ptr\":\"" + oss.str() + "\",";
        json += "\"size\":" + std::to_string(f.tamano);
        json += "}";
        if (i + 1 < fugas.size()) json += ",";
    }
    json += "]}\n"; // 👈 importante para que la GUI lo lea con readLine()

    qDebug() << "[LIB] json enviado:" << QString::fromStdString(json);

    QTcpSocket sock;
    sock.connectToHost("127.0.0.1", 5050);
    if (sock.waitForConnected(500)) {
        sock.write(json.c_str(), json.size());
        sock.flush();
        sock.waitForBytesWritten(500);
    } else {
        qWarning() << "[LIB] no se pudo conectar al servidor GUI";
    }
}


void reporteAlSalir() {
    auto fugas = listaGlobal.reportLeaks();
    if (!fugas.empty()) {
        std::cout << "⚠️ Se detectaron fugas de memoria: " << fugas.size() << std::endl;
    } else {
        std::cout << "✅ No se detectaron fugas de memoria." << std::endl;
    }
}


















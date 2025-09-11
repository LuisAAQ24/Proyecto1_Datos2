#include "ListaGuardado.h"
#include "memory_instrumentation.h"
#include <cstdlib>
#include <new>
#include <string>
#include <iostream>

#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include "ServidorSocket.h"

// Variables globales
ListaGuardado listaGlobal;
bool profilerActivo = false;
ServidorSocket* servidorSocket = nullptr; // puntero global al socket

// ====================================================
// Helper para obtener el nombre corto de un archivo
// ====================================================
static std::string nombreArchivo(const char* rutaCompleta) {
    std::string ruta(rutaCompleta);
    size_t pos = ruta.find_last_of("/\\");
    if (pos != std::string::npos) return ruta.substr(pos + 1);
    return ruta;
}

// ====================================================
// Helper para enviar JSON al servidor
// ====================================================
static void enviarAlSocket(void* ptr, size_t tamano, const std::string& tipo, const std::string& archivo)
{
    if (servidorSocket)
    {
        QJsonObject obj;
        obj["direccion"] = QString::number(reinterpret_cast<quintptr>(ptr), 16);
        obj["tamano"] = static_cast<int>(tamano);
        obj["tipo"] = QString::fromStdString(tipo);
        obj["archivo"] = QString::fromStdString(archivo);
        obj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QJsonDocument doc(obj);
        servidorSocket->enviarJSON(doc.toJson(QJsonDocument::Compact));
    }
}

// --------------------------
// operator new
// --------------------------
void* operator new(std::size_t tamano) {
    void* ptr = std::malloc(tamano);
    if (!ptr) throw std::bad_alloc();
    if (profilerActivo) {
        std::cout << "[NEW] ptr=" << ptr << " size=" << tamano << "\n";
        listaGlobal.agregar(ptr, tamano, "new", nombreArchivo(__FILE__));
        enviarAlSocket(ptr, tamano, "new", nombreArchivo(__FILE__));
    }
    return ptr;
}

// --------------------------
// operator delete (sized)
// --------------------------
void operator delete(void* direccion, std::size_t) noexcept {
    if (profilerActivo && direccion) {
        std::cout << "[DELETE sized] ptr=" << direccion << "\n";
        listaGlobal.eliminar(direccion);
        enviarAlSocket(direccion, 0, "delete", "");
    }
    std::free(direccion);
}

// --------------------------
// operator delete (sin tamaño)
// --------------------------
void operator delete(void* direccion) noexcept {
    if (profilerActivo && direccion) {
        std::cout << "[DELETE unsized] ptr=" << direccion << "\n";
        listaGlobal.eliminar(direccion);
        enviarAlSocket(direccion, 0, "delete", "");
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
        std::cout << "[NEW[]] ptr=" << ptr << " size=" << tamano << "\n";
        listaGlobal.agregar(ptr, tamano, "new[]", nombreArchivo(__FILE__));
        enviarAlSocket(ptr, tamano, "new[]", nombreArchivo(__FILE__));
    }
    return ptr;
}

// --------------------------
// operator delete[] (sized)
// --------------------------
void operator delete[](void* direccion, std::size_t) noexcept {
    if (profilerActivo && direccion) {
        std::cout << "[DELETE[] sized] ptr=" << direccion << "\n";
        listaGlobal.eliminar(direccion);
        enviarAlSocket(direccion, 0, "delete[]", "");
    }
    std::free(direccion);
}

// --------------------------
// operator delete[] (sin tamaño)
// --------------------------
void operator delete[](void* direccion) noexcept {
    if (profilerActivo && direccion) {
        std::cout << "[DELETE[] unsized] ptr=" << direccion << "\n";
        listaGlobal.eliminar(direccion);
        enviarAlSocket(direccion, 0, "delete[]", "");
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



















#define NO_TRACK_NEW
#include "ListaGuardado.h"
#include "memory_instrumentation.h"
#include <cstdlib>
#include <new>
#include <string>
#include <iostream>
#include <sstream>

#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

#include <QFile>
#include <QJsonArray>
#include <QIODevice>


ListaGuardado listaGlobal;


static std::string nombreArchivo(const char* rutaCompleta) {
    if (!rutaCompleta) return "<unknown>";
    std::string ruta(rutaCompleta);
    size_t pos = ruta.find_last_of("/\\");
    if (pos != std::string::npos) return ruta.substr(pos + 1);
    return ruta;
}


static std::string archivoLineaAstring(const char* file, int line) {
    std::ostringstream oss;
    oss << nombreArchivo(file) << ":" << line;
    return oss.str();
}


static void enviarAlSocket(void* ptr, size_t tamano, const std::string& tipo, const std::string& archivo) {
    if (g_clientSocket) { // <-- Usa la variable del SERVIDOR
        QJsonObject obj;
        obj["direccion"] = QString::number(reinterpret_cast<quintptr>(ptr), 16);
        obj["tamano"] = static_cast<int>(tamano);
        obj["tipo"] = QString::fromStdString(tipo);
        obj["archivo"] = QString::fromStdString(archivo);
        obj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QJsonDocument doc(obj);
        g_clientSocket->write(doc.toJson(QJsonDocument::Compact) + "\n");
    }
}


void* operator new(std::size_t tamano, const char* file, int line) {
    void* ptr = std::malloc(tamano);
    if (!ptr) throw std::bad_alloc();


    if (profilerActivo) {
        std::string archivoCorto = nombreArchivo(file);
        std::string archivoLinea = archivoLineaAstring(file, line);


        if (archivoCorto != "memory_instrumentation.cpp" &&
            archivoCorto != "ListaGuardado.cpp" &&
            archivoCorto != "ServidorSocket.cpp" &&
            archivoCorto.find("qtcpsocket") == std::string::npos &&
            archivoCorto.find("qobject") == std::string::npos &&
            archivoCorto.find("qbytearray") == std::string::npos) {


            std::cout << "[NEW] ptr=" << ptr << " size=" << tamano << " archivo=" << archivoLinea << "\n";
            listaGlobal.agregar(ptr, tamano, "new", archivoLinea);
            enviarAlSocket(ptr, tamano, "new", archivoLinea);
        }
    }


    return ptr;
}
void* operator new[](std::size_t tamano, const char* file, int line) {
    void* ptr = std::malloc(tamano);
    if (!ptr) throw std::bad_alloc();


    if (profilerActivo) {
        std::string archivoLinea = archivoLineaAstring(file, line);
        std::cout << "[NEW[]] ptr=" << ptr << " size=" << tamano << " archivo=" << archivoLinea << "\n";
        listaGlobal.agregar(ptr, tamano, "new[]", archivoLinea);
        enviarAlSocket(ptr, tamano, "new[]", archivoLinea);
    }
    return ptr;
}


void operator delete(void* direccion, std::size_t) noexcept {
    if (profilerActivo && direccion) {
        std::cout << "[DELETE sized] ptr=" << direccion << "\n";
        listaGlobal.eliminar(direccion);
        enviarAlSocket(direccion, 0, "delete", "");
    }
    std::free(direccion);
}


//ARREGLO CON TAMAÑO
void operator delete[](void* direccion, std::size_t) noexcept {
    if (profilerActivo && direccion) {
        std::cout << "[DELETE[] sized] ptr=" << direccion << "\n";
        //listaGlobal.eliminar(direccion);
        enviarAlSocket(direccion, 0, "delete[]", "");
    }
    std::free(direccion);
}

//ARREGLO SIN TAMAÑO
void operator delete[](void* direccion) noexcept {
    if (profilerActivo && direccion) {
        std::cout << "[DELETE[]] ptr=" << direccion << "\n";
        //listaGlobal.eliminar(direccion);
        enviarAlSocket(direccion, 0, "delete[]", "");
    }
    std::free(direccion);
}




void* operator new(std::size_t tamano) {
    // Llama a nuestra versión de rastreo con información de archivo desconocida
    return operator new(tamano, "Unknown (lib/STL)", 0);
}

// new[] estándar
void* operator new[](std::size_t tamano) {
    // Llama a nuestra versión de rastreo con información de archivo desconocida
    return operator new[](tamano, "Unknown (lib/STL)", 0);
}

// delete estándar
void operator delete(void* direccion) noexcept {
    if (profilerActivo && direccion) {
        std::cout << "[DELETE] ptr=" << direccion << "\n";
        //listaGlobal.eliminar(direccion);
        enviarAlSocket(direccion, 0, "delete", "");
    }
    std::free(direccion);
}

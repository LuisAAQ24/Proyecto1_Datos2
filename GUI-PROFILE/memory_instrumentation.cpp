#define NO_TRACK_NEW
#include "ListaGuardado.h"
#include "memory_instrumentation.h"
#include <cstdlib>
#include <new>
#include <string>
#include <iostream>
#include <sstream>


#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include "ServidorSocket.h"
#include <QFile>
#include <QJsonArray>
#include <QIODevice>


ListaGuardado listaGlobal;
bool profilerActivo = false;
ServidorSocket* servidorSocket = nullptr;


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
    if (servidorSocket) {
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
        listaGlobal.eliminar(direccion);
        enviarAlSocket(direccion, 0, "delete[]", "");
    }
    std::free(direccion);
}

//ARREGLO SIN TAMAÑO
void operator delete[](void* direccion) noexcept {
    if (profilerActivo && direccion) {
        std::cout << "[DELETE[]] ptr=" << direccion << "\n";
        listaGlobal.eliminar(direccion);
        enviarAlSocket(direccion, 0, "delete[]", "");
    }
    std::free(direccion);
}



//INCESESARIO
void guardarReporteJSON() {
    auto fugas = listaGlobal.reportLeaks();
    QJsonObject root;
    root["metricas"] = listaGlobal.obtenerMetricas();
    QJsonArray fugasArray;
    for (const auto& f : fugas) {
        QJsonObject obj;
        obj["direccion"] = QString::number(reinterpret_cast<quintptr>(f.direccion), 16);
        obj["tamano"] = static_cast<int>(f.tamano);
        obj["tipo"] = QString::fromStdString(f.tipo);
        obj["archivo"] = QString::fromStdString(f.archivo);
        obj["marcaDeTiempo"] = static_cast<qint64>(f.marcaDeTiempo);
        fugasArray.append(obj);
    }
    root["fugas"] = fugasArray;


    QString path = "C:/Users/cesar/Documents/Proyecto1_Datos2/memory_report.json";
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(root);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
    std::cout << "Archivo JSON generado: " << path.toStdString() << "\n";
}


void reporteAlSalir() {
    auto fugas = listaGlobal.reportLeaks();
    if (!fugas.empty()) {
        std::cout << "⚠️ Se detectaron fugas de memoria: " << fugas.size() << std::endl;
    } else {
        std::cout << "✅ No se detectaron fugas de memoria." << std::endl;
    }
}

void enviarResumenPorArchivo() {
    if (!servidorSocket) return;

    // Agrupar por archivo
    std::map<std::string, std::pair<int, size_t>> resumen;
    Guardado* actual = listaGlobal.getInicio();
    while (actual) {
        resumen[actual->archivo].first++;                  // conteo asignaciones
        resumen[actual->archivo].second += actual->tamano; // memoria total
        actual = actual->siguiente;
    }

    // Construir JSON
    QJsonObject root;
    root["tipo"] = "resumen_por_archivo";
    QJsonArray archivosArray;
    for (auto& [nombre, datos] : resumen) {
        QJsonObject obj;
        obj["nombre"] = QString::fromStdString(nombre);
        obj["conteo_asignaciones"] = datos.first;
        obj["memoria_total"] = static_cast<int>(datos.second);
        archivosArray.append(obj);
    }
    root["archivos"] = archivosArray;

    QJsonDocument doc(root);
    servidorSocket->enviarJSON(doc.toJson(QJsonDocument::Compact));
}

/*Ejemplo de JSON que se enviaría por socket:

                                             Resumen por archivo:
{
    "tipo": "resumen_por_archivo",
             "archivos": [
                              {
                                  "nombre": "main.cpp",
                                  "conteo_asignaciones": 1,
                                  "memoria_total": 40
                              }
    ]
}
*/

void enviarReporteLeaks() {
    if (!servidorSocket) return;

    auto fugas = listaGlobal.reportLeaks();

    size_t totalFugado = 0;
    size_t leakMasGrande = 0;
    std::string archivoMasLeaks;
    std::map<std::string, int> conteoPorArchivo;

    // Analizar fugas
    for (const auto& f : fugas) {
        totalFugado += f.tamano;
        if (f.tamano > leakMasGrande) {
            leakMasGrande = f.tamano;
            archivoMasLeaks = f.archivo; // opcional, si quieres que el leak más grande tenga su archivo
        }
        conteoPorArchivo[f.archivo]++;
    }

    // Encontrar archivo con más fugas
    int maxLeaks = 0;
    for (auto& [archivo, conteo] : conteoPorArchivo) {
        if (conteo > maxLeaks) {
            maxLeaks = conteo;
            archivoMasLeaks = archivo;
        }
    }

    // Construir JSON
    QJsonObject root;
    root["tipo"] = "reporte_leaks";
    root["total_fugado"] = static_cast<int>(totalFugado);
    root["tasa_leaks"] = (listaGlobal.getTotalAsignaciones() > 0)
                             ? double(fugas.size()) / listaGlobal.getTotalAsignaciones()
                             : 0.0;

    // Leak más grande
    if (!fugas.empty()) {
        QJsonObject leakObj;
        leakObj["tamano"] = static_cast<int>(leakMasGrande);
        leakObj["archivo"] = QString::fromStdString(archivoMasLeaks);
        root["leak_mas_grande"] = leakObj;
    }

    root["archivo_mas_leaks"] = QString::fromStdString(archivoMasLeaks);

    // Lista de fugas
    QJsonArray leaksArray;
    for (const auto& f : fugas) {
        QJsonObject obj;
        obj["direccion"] = QString::number(reinterpret_cast<quintptr>(f.direccion), 16);
        obj["tamano"] = static_cast<int>(f.tamano);
        obj["archivo"] = QString::fromStdString(f.archivo);
        leaksArray.append(obj);
    }
    root["leaks"] = leaksArray;

    QJsonDocument doc(root);
    servidorSocket->enviarJSON(doc.toJson(QJsonDocument::Compact));
}

/*Reporte de leaks:
{
  "tipo": "reporte_leaks",
  "total_fugado": 0,
  "tasa_leaks": 0.0,
  "leak_mas_grande": {
    "tamano": 0,
    "archivo": ""
  },
  "archivo_mas_leaks": "",
  "leaks": []
}
*/


















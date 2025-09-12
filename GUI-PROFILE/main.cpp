#include <QApplication>
#include <QTimer>
#include "mainwindow.h"
#include "ListaGuardado.h"
#include "memory_instrumentation.h"
#include "ServidorSocket.h"
#include <string>
#include <iostream>

struct Dummy {
    std::string texto;
    Dummy() = default;
    Dummy(const std::string& t) : texto(t) {
        std::cout << "Dummy creado: " << texto << "\n";
    }
    ~Dummy() {
        std::cout << "Dummy destruido: " << texto << "\n";
    }
};

void pruebasMemoria() {
    profilerActivo = true; // 🔹 activar profiler primero
    listaGlobal.limpiar(); // 🔹 limpiar lista y reiniciar métricas

    // ---------------------------
    // Fugas intencionales
    // ---------------------------
    int* p = new int(99);  // ahora solo se cuenta esta asignación
    delete p;
    int* a = new int(19);
    // ---------------------------
    // Guardar JSON y reporte
    // ---------------------------
    guardarReporteJSON();
    reporteAlSalir();

    // Si querés liberar memoria para no dejar fuga:
    // delete p;

    profilerActivo = false;
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Crear GUI
    MainWindow window;
    window.show();

    // ---------------------------
    // Inicializar servidor socket
    // ---------------------------
    servidorSocket = new ServidorSocket(&window);
    if(!servidorSocket->listen(QHostAddress::Any, 12345))
        qDebug() << "Error iniciando servidor:" << servidorSocket->errorString();
    else
        qDebug() << "Servidor socket iniciado en puerto 12345";

    // Ejecutar pruebas de memoria después de 1 segundo
    QTimer::singleShot(1000, pruebasMemoria);

    return app.exec();
}






















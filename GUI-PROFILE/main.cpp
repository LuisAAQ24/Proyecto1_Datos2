#include <QApplication>
#include <QTimer>
#include "mainwindow.h"
#include "ListaGuardado.h"
#include "memory_instrumentation.h"
#include "ServidorSocket.h"
#include <string>
#include <iostream>



void pruebasMemoria() {
    profilerActivo = true;          // Activar profiler
    listaGlobal.limpiar();           // Limpiar cualquier registro previo

    std::cout << "=== Inicio de prueba de memoria ===\n";

    // Asignaciones simples
    int* p1 = new int(99);
    int* p2 = new int(42);
    int* arr = new int[5]{1, 2, 3, 4, 5};

    struct Dummy { int x; double y; };
    Dummy* d1 = new Dummy{10, 3.14};
    Dummy* d2 = new Dummy{20, 6.28};
    double* f = new double(2.718);


    delete p1;
    delete d1;
    delete[] arr;

    // Guardamos reporte JSON y mostramos métricas
    guardarReporteJSON();
    reporteAlSalir();

    // Limpiar profiler
    profilerActivo = false;
    listaGlobal.limpiar();

    std::cout << "=== Fin de prueba de memoria ===\n";
}
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Crear GUI
    MainWindow window;
    window.show();

    servidorSocket = new ServidorSocket(&window);
    if(!servidorSocket->listen(QHostAddress::Any, 12345))
        qDebug() << "Error iniciando servidor:" << servidorSocket->errorString();
    else
        qDebug() << "Servidor socket iniciado en puerto 12345";

    // Ejecutar pruebas de memoria después de 1 segundo
    QTimer::singleShot(1000, pruebasMemoria);

    return app.exec();
}






















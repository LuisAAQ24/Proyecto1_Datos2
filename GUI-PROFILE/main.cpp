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

// Función que hace pruebas de memoria y genera reporte
void pruebasMemoria() {
    listaGlobal.limpiar();
    profilerActivo = true;

    // ---------------------------
    // Asignaciones correctas
    // ---------------------------
    int* a = new int(42);
    double* b = new double(3.14159);
    Dummy* d1 = new Dummy("objeto 1");

    int* arr = new int[100];      // array simple

    // Array de Dummy seguro
    Dummy* arrObj = new Dummy[3];
    arrObj[0] = Dummy("uno");
    arrObj[1] = Dummy("dos");
    arrObj[2] = Dummy("tres");

    // Liberar correctamente
    delete a;
    delete b;
    delete d1;
    delete[] arr;
    delete[] arrObj;

    // ---------------------------
    // Fugas intencionales
    // ---------------------------
    new int(99);                // fuga simple
    new double[50];             // fuga array
    new Dummy("fuga objeto");   // fuga objeto

    // ---------------------------
    // Guardar JSON y reporte
    // ---------------------------
    guardarReporteJSON();
    reporteAlSalir();

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






















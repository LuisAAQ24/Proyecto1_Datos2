#include <QApplication>
#include <QTimer>
#include "mainwindow.h"
#include "ListaGuardado.h"
#include "memory_instrumentation.h"
#include <string>

struct Dummy {
    std::string texto;
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
    Dummy* arrObj = new Dummy[3] {{"uno"}, {"dos"}, {"tres"}};

    // Liberar correctamente
    delete a;
    delete b;
    delete d1;
    delete[] arr;
    delete[] arrObj;

    // ---------------------------
    // Fugas intencionales
    // ---------------------------
    new int(99);                          // fuga simple
    new double[50];                       // fuga array
    new Dummy("fuga objeto");             // fuga objeto

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
    MainWindow window;
    window.show();

    QTimer::singleShot(1000, pruebasMemoria);

    return app.exec();
}























#include <QApplication>
#include "mainwindow.h"
#include "ListaGuardado.h"

extern void reporteAlSalir();
extern bool profilerActivo;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    // Dejamos que Qt haga su loop primero
    int resultado = app.exec();

    // Activar profiler para pruebas de memoria
    profilerActivo = true;

    // Pruebas de memoria después de cerrar la ventana
    int* a = new int(10);
    delete a;

    int* arr = new int[5];
    delete[] arr;

    double* d = new double[3]; // fuga intencional
    char* c = new char('X');
    delete c;
    float* f = new float[10];  // fuga intencional

    // Reporte de fugas
    reporteAlSalir();

    return resultado;
}










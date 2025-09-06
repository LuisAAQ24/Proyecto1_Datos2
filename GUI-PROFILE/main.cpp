#include <QApplication>
#include <QTimer>
#include "mainwindow.h"
#include "ListaGuardado.h"
#include "memory_instrumentation.h"

// Función que hace pruebas de memoria y genera reporte
void pruebasMemoria() {
    // Limpiar memoria vieja
    listaGlobal.limpiar();

    // Activar profiler solo para tus pruebas
    profilerActivo = true;

    // ---------------------------
    // Asignaciones de prueba
    // ---------------------------
    int* a = new int(10);
    // ---------------------------
    // Guardar JSON y reporte
    // ---------------------------
    guardarReporteJSON();
    reporteAlSalir();

    // Desactivar profiler después de pruebas
    profilerActivo = false;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();

    // Esperar 1 segundo antes de ejecutar pruebas de memoria
    QTimer::singleShot(1000, pruebasMemoria);

    // Mantener la ventana abierta
    return app.exec();
}






















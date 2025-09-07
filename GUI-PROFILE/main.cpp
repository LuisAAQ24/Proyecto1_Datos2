#include <QApplication>
#include "mainwindow.h"
#include "memory_instrumentation.h"

// NO medir a la propia GUI
extern bool profilerActivo;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Desactiva medición en este proceso (GUI es el "medidor")
    profilerActivo = false;

    MainWindow w;
    w.show();

    // NO llamamos reporteAlSalir aquí para no activar impresiones adicionales.
    return app.exec();
}











#include <QApplication>
#include "mainwindow.h"
#include "memory_instrumentation.h"

// La GUI es el medidor: NO se mide a sí misma
extern bool profilerActivo;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    profilerActivo = false; // desactivado en la GUI

    MainWindow w;
    w.show();

    return app.exec();
}













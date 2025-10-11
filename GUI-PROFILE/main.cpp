#include <QApplication>
#include <QTimer>
#include "mainwindow.h"
#include "ServidorSocket.h"

ListaGuardado listaGlobal;
ServidorSocket* servidorSocket = nullptr;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();


    servidorSocket = new ServidorSocket(&window);


    if (!servidorSocket->listen(QHostAddress::Any, 12345)) {
        qDebug() << "Error iniciando servidor:" << servidorSocket->errorString();
    } else {
        qDebug() << "Servidor socket iniciado en puerto 12345. Esperando conexiones...";
    }


    return app.exec();
}






















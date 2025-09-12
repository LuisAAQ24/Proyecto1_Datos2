#include "ServidorSocket.h"
#include <iostream>

ServidorSocket::ServidorSocket(QObject* parent) : QTcpServer(parent) { }

void ServidorSocket::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket* cliente = new QTcpSocket(this);
    cliente->setSocketDescriptor(socketDescriptor);

    std::cout << "[SOCKET] Nueva conexión entrante, socketDescriptor="
              << socketDescriptor
              << " ptr=" << cliente
              << "\n";

    connect(cliente, &QTcpSocket::disconnected, this, &ServidorSocket::clienteDesconectado);

    clientes.append(cliente);
}

void ServidorSocket::clienteDesconectado() {
    QTcpSocket* cliente = qobject_cast<QTcpSocket*>(sender());
    if (cliente) {
        std::cout << "[SOCKET] Cliente desconectado ptr=" << cliente << "\n";
        clientes.removeAll(cliente);  // quitar de la lista
        cliente->deleteLater();       // se libera seguro al terminar eventos pendientes
    }
}

void ServidorSocket::enviarJSON(const QString& json)
{
    QByteArray data = json.toUtf8() + "\n"; // salto de línea como separador
    for(QTcpSocket* cliente : clientes)
    {
        if(cliente->state() == QAbstractSocket::ConnectedState) {
            std::cout << "[SOCKET] Enviando JSON a cliente ptr=" << cliente << "\n";
            cliente->write(data);
        }
    }
}



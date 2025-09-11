#include "ServidorSocket.h"
#include <QDebug>

ServidorSocket::ServidorSocket(QObject* parent) : QTcpServer(parent) { }

void ServidorSocket::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket* cliente = new QTcpSocket(this);
    cliente->setSocketDescriptor(socketDescriptor);

    connect(cliente, &QTcpSocket::disconnected, this, &ServidorSocket::clienteDesconectado);

    clientes.append(cliente);
}

void ServidorSocket::clienteDesconectado()
{
    QTcpSocket* cliente = qobject_cast<QTcpSocket*>(sender());
    if(cliente) {
        clientes.removeAll(cliente);
        cliente->deleteLater();
    }
}

void ServidorSocket::enviarJSON(const QString& json)
{
    QByteArray data = json.toUtf8() + "\n"; // salto de línea como separador
    for(QTcpSocket* cliente : clientes)
    {
        if(cliente->state() == QAbstractSocket::ConnectedState)
            cliente->write(data);
    }
}

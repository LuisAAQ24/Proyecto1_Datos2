#include "ServidorSocket.h"
#include <iostream>
#include <QJsonDocument>
#include <QJsonObject>
#include "ListaGuardado.h"

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
    connect(cliente, &QTcpSocket::readyRead, this, &ServidorSocket::leerDatosCliente);

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

void ServidorSocket::leerDatosCliente()
{
    QTcpSocket* cliente = qobject_cast<QTcpSocket*>(sender());
    if (!cliente) return;

    // El cliente envía cada JSON con un '\n'. Leemos línea por línea.
    while (cliente->canReadLine()) {
        QByteArray jsonData = cliente->readLine().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);

        if (doc.isNull() || !doc.isObject()) {
            continue; // Ignorar si no es un JSON válido
        }

        QJsonObject jsonObj = doc.object();
        QString tipo = jsonObj["tipo"].toString();

        // Convertir la dirección de memoria (en formato string hex) a un puntero
        bool ok;
        void* direccion = reinterpret_cast<void*>(jsonObj["direccion"].toString().toULongLong(&ok, 16));

        if (tipo.startsWith("new")) {
            size_t tamano = static_cast<size_t>(jsonObj["tamano"].toInt());
            std::string archivo = jsonObj["archivo"].toString().toStdString();
            listaGlobal.agregar(direccion, tamano, tipo.toStdString(), archivo);

        } else if (tipo.startsWith("delete")) {
            listaGlobal.eliminar(direccion);
        }
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



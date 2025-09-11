#ifndef SERVIDORSOCKET_H
#define SERVIDORSOCKET_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QString>

class ServidorSocket : public QTcpServer
{
    Q_OBJECT

public:
    ServidorSocket(QObject* parent = nullptr);

    void enviarJSON(const QString& json); // enviar a todos los clientes

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void clienteDesconectado();

private:
    QList<QTcpSocket*> clientes;
};

#endif // SERVIDORSOCKET_H

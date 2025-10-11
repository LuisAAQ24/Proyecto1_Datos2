// main.cpp (Programa Cliente de Ejemplo)

#include <QCoreApplication>
#include <QTcpSocket>
#include <QTimer>
#include <iostream>


#include "memory_instrumentation.h"

// Tu biblioteca espera que estas variables globales existan.
// Este programa cliente es responsable de definirlas y gestionarlas.
QTcpSocket* g_clientSocket = nullptr;
bool profilerActivo = false;

// Una clase de ejemplo para probar asignaciones de objetos
class Vehiculo {
public:
    Vehiculo(const char* nombre) : nombre_(nombre) {
        std::cout << "Constructor de Vehiculo: " << nombre_ << std::endl;
    }
    ~Vehiculo() {
        std::cout << "Destructor de Vehiculo: " << nombre_ << std::endl;
    }
private:
    std::string nombre_;
};

// Esta función contiene el código que queremos perfilar
void ejecutarPruebasDeMemoria() {
    if (!profilerActivo) {
        std::cout << "Profiler inactivo, no se registrarán las asignaciones." << std::endl;
        return;
    }

    std::cout << "\n--- INICIO: Demostración de uso de memoria ---\n" << std::endl;

    // 1. Asignación y liberación de un objeto simple (new / delete)
    // Tu profiler debería registrar una asignación y una liberación.
    std::cout << "[PRUEBA 1] Asignando y liberando un objeto 'Vehiculo'." << std::endl;
    Vehiculo* coche = new Vehiculo("Coche");
    delete coche;
    std::cout << "[PRUEBA 1] Finalizada.\n" << std::endl;


    // 2. Asignación y liberación de un arreglo (new[] / delete[])
    // Tu profiler debería registrar una asignación de arreglo y su liberación.
    std::cout << "[PRUEBA 2] Asignando y liberando un arreglo de 1024 enteros." << std::endl;
    int* arreglo_grande = new int[1024];
    for (int i = 0; i < 1024; ++i) {
        arreglo_grande[i] = i;
    }
    delete[] arreglo_grande;
    std::cout << "[PRUEBA 2] Finalizada.\n" << std::endl;


    // 3. Simulación de una fuga de memoria
    // Tu profiler debería registrar esta asignación, pero NUNCA su liberación.
    // Debería aparecer en el reporte de "Memory Leaks" de tu GUI.
    std::cout << "[PRUEBA 3] Asignando un objeto 'Vehiculo' que NO será liberado (fuga)." << std::endl;
    Vehiculo* moto_fugada = new Vehiculo("Moto Fugada");
    std::cout << "[PRUEBA 3] El puntero a 'moto_fugada' se perderá. La memoria no se liberará.\n" << std::endl;


    std::cout << "--- FIN: Demostración de uso de memoria ---\n" << std::endl;
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    const QString serverAddress = "127.0.0.1";
    const quint16 serverPort = 12345; // Asegúrate que este puerto coincida con tu servidor

    // 1. Crear el socket y asignarlo a la variable global
    g_clientSocket = new QTcpSocket(&a);

    // 2. Conectar las señales para gestionar el estado del profiler
    QObject::connect(g_clientSocket, &QTcpSocket::connected, [&]() {
        std::cout << "[CLIENTE] Conectado exitosamente al servidor. Profiler ACTIVADO." << std::endl;
        profilerActivo = true;

        // Una vez conectados, ejecutamos el código que queremos analizar
        ejecutarPruebasDeMemoria();

        // Después de las pruebas, esperamos un poco y nos desconectamos
        std::cout << "[CLIENTE] Pruebas finalizadas. Desconectando en 2 segundos..." << std::endl;
        QTimer::singleShot(2000, [&]() {
            g_clientSocket->disconnectFromHost();
        });
    });

    QObject::connect(g_clientSocket, &QTcpSocket::disconnected, [&]() {
        std::cout << "[CLIENTE] Desconectado del servidor. Profiler DESACTIVADO." << std::endl;
        profilerActivo = false;
        a.quit(); // Salir de la aplicación una vez desconectado
    });

    QObject::connect(g_clientSocket, &QAbstractSocket::errorOccurred,
                     [&](QAbstractSocket::SocketError) {
                         std::cerr << "[CLIENTE] Error de conexión: " << g_clientSocket->errorString().toStdString() << std::endl;
                         profilerActivo = false;
                         a.quit(); // Salir si no se puede conectar
                     });

    // 3. Intentar la conexión
    std::cout << "[CLIENTE] Intentando conectar a " << serverAddress.toStdString()
              << ":" << serverPort << "..." << std::endl;
    g_clientSocket->connectToHost(serverAddress, serverPort);

    return a.exec();
}

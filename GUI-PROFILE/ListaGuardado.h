#ifndef LISTA_GUARDADO_H
#define LISTA_GUARDADO_H

#include <ctime>
#include <string>
#include <iostream>

// Nodo de la lista
struct Guardado {
    void* direccion;
    size_t tamano;
    std::string tipo;
    time_t marcaDeTiempo;
    Guardado* siguiente;
};

// Lista de asignaciones
class ListaGuardado {
private:
    Guardado* inicio = nullptr;

public:
    void agregar(void* direccion, size_t tamano, const std::string& tipo) {
        Guardado* nodo = new Guardado{direccion, tamano, tipo, std::time(nullptr), inicio};
        inicio = nodo;
    }

    void eliminar(void* direccion) {
        Guardado* anterior = nullptr;
        Guardado* actual = inicio;
        while (actual) {
            if (actual->direccion == direccion) {
                if (anterior) anterior->siguiente = actual->siguiente;
                else inicio = actual->siguiente;
                delete actual;
                return;
            }
            anterior = actual;
            actual = actual->siguiente;
        }
    }

    void reportarFugas() {
        Guardado* actual = inicio;
        std::cout << "\n===== REPORTE DE FUGAS =====\n";
        if (!actual) {
            std::cout << "No se detectaron fugas de memoria ✅\n";
            return;
        }
        while (actual) {
            std::cout << "Fuga en direccion " << actual->direccion
                      << " | " << actual->tamano << " bytes"
                      << " | Tiempo: " << actual->marcaDeTiempo << std::endl;
            actual = actual->siguiente;
        }
    }
};

// Lista global
extern ListaGuardado listaGlobal;

#endif






#ifndef LISTA_GUARDADO_H
#define LISTA_GUARDADO_H

#include <ctime>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

struct Guardado {
    void* direccion;
    size_t tamano;
    std::string tipo;
    std::string archivo;
    time_t marcaDeTiempo;
    Guardado* siguiente;
};

struct Fuga {
    void* direccion;
    size_t tamano;
    std::string tipo;
    std::string archivo;
    time_t marcaDeTiempo;
};

class ListaGuardado {
private:
    Guardado* inicio = nullptr;

public:
    void agregar(void* direccion, size_t tamano, const std::string& tipo, const std::string& archivo);
    void eliminar(void* direccion);
    std::vector<Fuga> reportLeaks();
    void exportJSON(const std::vector<Fuga>& fugas, const std::string& filename = "memory_report.json");
    void limpiar();

    // Getter para inicio (opcional)
    Guardado* getInicio() { return inicio; }
};

// Declaración global
extern ListaGuardado listaGlobal;

#endif









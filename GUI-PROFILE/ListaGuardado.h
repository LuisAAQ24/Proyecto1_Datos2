#ifndef LISTA_GUARDADO_H
#define LISTA_GUARDADO_H

#include <ctime>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <QJsonObject>  // Para usar QJsonObject en métricas

// Nodo que representa cada bloque de memoria
struct Guardado {
    void* direccion;
    size_t tamano;
    std::string tipo;
    std::string archivo;
    time_t marcaDeTiempo;
    Guardado* siguiente;
};

// Estructura para reportar fugas
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

    // 📊 Métricas generales
    size_t totalAsignaciones = 0;
    size_t totalLiberaciones = 0;
    size_t memoriaActual = 0;
    size_t maxMemoriaUsada = 0;

public:
    // Métodos para agregar/eliminar bloques
    void agregar(void* direccion, size_t tamano, const std::string& tipo, const std::string& archivo);
    void eliminar(void* direccion);
    void limpiar();

    // Reporte de fugas
    std::vector<Fuga> reportLeaks();

    // Exportar fugas + métricas a JSON
    void exportJSON(const std::vector<Fuga>& fugas, const std::string& filename = "memory_report.json");

    // Obtener métricas generales en formato QJsonObject
    QJsonObject obtenerMetricas();

    // Getter opcional para inicio
    Guardado* getInicio() { return inicio; }
};

// Declaración global
extern ListaGuardado listaGlobal;

#endif








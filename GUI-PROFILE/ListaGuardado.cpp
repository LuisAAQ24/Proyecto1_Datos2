#define NO_TRACK_NEW
#include "ListaGuardado.h"
#include <cstdlib>
#include <new>


void ListaGuardado::agregar(void* direccion, size_t tamano, const std::string& tipo, const std::string& archivo) {
    if (archivo.find("Qt") != std::string::npos ||
        archivo.find("qtcpsocket") != std::string::npos ||
        archivo.find("qobject") != std::string::npos) return;


    Guardado* actual = inicio;
    while (actual) {
        if (actual->direccion == direccion) return;
        actual = actual->siguiente;
    }


    Guardado* nodo = static_cast<Guardado*>(std::malloc(sizeof(Guardado)));
    if (!nodo) throw std::bad_alloc();
    new (&nodo->tipo) std::string(tipo);
    new (&nodo->archivo) std::string(archivo);
    nodo->direccion = direccion;
    nodo->tamano = tamano;
    nodo->marcaDeTiempo = std::time(nullptr);
    nodo->siguiente = inicio;
    inicio = nodo;


    totalAsignaciones++;
    memoriaActual += tamano;
    if (memoriaActual > maxMemoriaUsada) maxMemoriaUsada = memoriaActual;
}


void ListaGuardado::eliminar(void* direccion) {
    Guardado* anterior = nullptr;
    Guardado* actual = inicio;
    while (actual) {
        if (actual->direccion == direccion) {
            if (anterior) anterior->siguiente = actual->siguiente;
            else inicio = actual->siguiente;
            totalLiberaciones++;
            memoriaActual -= actual->tamano;
            actual->tipo.~basic_string();
            actual->archivo.~basic_string();
            std::free(actual);
            return;
        }
        anterior = actual;
        actual = actual->siguiente;
    }
}


void ListaGuardado::limpiar() {
    Guardado* actual = inicio;
    while (actual) {
        Guardado* siguiente = actual->siguiente;
        actual->tipo.~basic_string();
        actual->archivo.~basic_string();
        std::free(actual);
        actual = siguiente;
    }
    inicio = nullptr;
    totalAsignaciones = 0;
    totalLiberaciones = 0;
    memoriaActual = 0;
    maxMemoriaUsada = 0;
}


std::vector<Fuga> ListaGuardado::reportLeaks() {
    std::vector<Fuga> fugas;
    Guardado* actual = inicio;
    while (actual) {
        Fuga f{actual->direccion, actual->tamano, actual->tipo, actual->archivo, actual->marcaDeTiempo};
        fugas.push_back(f);
        actual = actual->siguiente;
    }
    return fugas;
}


QJsonObject ListaGuardado::obtenerMetricas() {
    QJsonObject obj;
    obj["totalAsignaciones"] = static_cast<int>(totalAsignaciones);
    obj["totalLiberaciones"] = static_cast<int>(totalLiberaciones);
    obj["memoriaActual"] = static_cast<int>(memoriaActual);
    obj["maxMemoriaUsada"] = static_cast<int>(maxMemoriaUsada);
    return obj;
}





#include "ListaGuardado.h"
#include <cstdlib>
#include <new>

ListaGuardado listaGlobal; // ÚNICA definición global

void ListaGuardado::agregar(void* direccion, size_t tamano, const std::string& tipo, const std::string& archivo) {
    Guardado* nodo = static_cast<Guardado*>(std::malloc(sizeof(Guardado)));
    if (!nodo) throw std::bad_alloc();

    nodo->direccion = direccion;
    nodo->tamano = tamano;
    nodo->tipo = tipo;
    nodo->archivo = archivo;
    nodo->marcaDeTiempo = std::time(nullptr);
    nodo->siguiente = inicio;

    inicio = nodo;
}

void ListaGuardado::eliminar(void* direccion) {
    Guardado* anterior = nullptr;
    Guardado* actual = inicio;
    while (actual) {
        if (actual->direccion == direccion) {
            if (anterior) anterior->siguiente = actual->siguiente;
            else inicio = actual->siguiente;
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
        std::free(actual);
        actual = siguiente;
    }
    inicio = nullptr;
}

std::vector<Fuga> ListaGuardado::reportLeaks() {
    std::vector<Fuga> fugas;
    Guardado* actual = inicio;
    while (actual) {
        Fuga f{
            actual->direccion,
            actual->tamano,
            actual->tipo,
            actual->archivo,
            actual->marcaDeTiempo
        };
        fugas.push_back(f);
        actual = actual->siguiente;
    }
    return fugas;
}

void ListaGuardado::exportJSON(const std::vector<Fuga>& fugas, const std::string& /*filename ignorado*/) {
    // Ruta fija
    std::string fullpath = "C:/Users/cesar/Documents/Proyecto1_Datos2/memory_report.json";
    std::ofstream file(fullpath);
    if (!file.is_open()) {
        std::cerr << "Error al abrir el archivo JSON para escritura\n";
        return;
    }

    file << "{\n  \"fugas\": [\n";
    for (size_t i = 0; i < fugas.size(); ++i) {
        const Fuga& f = fugas[i];
        file << "    {\n";
        file << "      \"direccion\": \"" << f.direccion << "\",\n";
        file << "      \"tamano\": " << f.tamano << ",\n";
        file << "      \"tipo\": \"" << f.tipo << "\",\n";
        file << "      \"archivo\": \"" << f.archivo << "\",\n";
        file << "      \"marcaDeTiempo\": " << f.marcaDeTiempo << "\n";
        file << "    }";
        if (i != fugas.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ]\n}";
    file.close();

    std::cout << "Archivo JSON generado: " << fullpath << "\n";
}





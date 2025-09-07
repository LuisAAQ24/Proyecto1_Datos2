#ifndef LISTA_GUARDADO_H
#define LISTA_GUARDADO_H

// Implementación ligera y sin asignaciones con new/delete para evitar reentrancia.
// Usa malloc/free internamente y un spinlock sencillo.

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <atomic>
#include <iostream>

struct Guardado {
    void* direccion;
    std::size_t tamano;
    char tipo[32];          // tipo comprimido (sin std::string)
    std::time_t marcaDeTiempo;
    Guardado* siguiente;
};

class ListaGuardado {
public:
    ListaGuardado() : cabeza(nullptr), conteo(0), bytes_totales(0) {
        flag.clear();
    }

    // Agrega un registro (NO usa new, solo malloc)
    void agregar(void* direccion, std::size_t tamano, const char* tipo) {
        if (!direccion) return;
        Guardado* nodo = static_cast<Guardado*>(std::malloc(sizeof(Guardado)));
        if (!nodo) return; // sin memoria, omitimos
        nodo->direccion = direccion;
        nodo->tamano = tamano;
        std::time(&nodo->marcaDeTiempo);
        std::memset(nodo->tipo, 0, sizeof(nodo->tipo));
        if (tipo) {
            std::strncpy(nodo->tipo, tipo, sizeof(nodo->tipo) - 1);
        } else {
            std::strncpy(nodo->tipo, "desconocido", sizeof(nodo->tipo) - 1);
        }

        lock();
        nodo->siguiente = cabeza;
        cabeza = nodo;
        ++conteo;
        bytes_totales += static_cast<long long>(tamano);
        unlock();
    }

    // Elimina el registro de 'direccion' (si existe) y devuelve el tamaño asociado.
    std::size_t eliminar_y_tamano(void* direccion) {
        if (!direccion) return 0;
        lock();
        Guardado* prev = nullptr;
        Guardado* cur = cabeza;
        while (cur) {
            if (cur->direccion == direccion) {
                // quitar
                if (prev) prev->siguiente = cur->siguiente;
                else      cabeza = cur->siguiente;
                std::size_t tam = cur->tamano;
                --conteo;
                bytes_totales -= static_cast<long long>(tam);
                unlock();
                std::free(cur);
                return tam;
            }
            prev = cur;
            cur = cur->siguiente;
        }
        unlock();
        return 0;
    }

    // Elimina (ignora tamaño devuelto)
    void eliminar(void* direccion) {
        (void)eliminar_y_tamano(direccion);
    }

    // Volcado básico de fugas
    void reportarFugas(std::ostream& os = std::cout) {
        lock();
        if (cabeza == nullptr) {
            os << "[ListaGuardado] Sin registros pendientes.\n";
            unlock();
            return;
        }
        os << "[ListaGuardado] Registros pendientes (posibles fugas):\n";
        Guardado* cur = cabeza;
        int idx = 0;
        while (cur) {
            os << "  #" << idx++
               << " ptr=" << cur->direccion
               << " bytes=" << cur->tamano
               << " tipo=" << cur->tipo
               << " t=" << static_cast<long long>(cur->marcaDeTiempo)
               << "\n";
            cur = cur->siguiente;
        }
        os << "Total pendientes: " << conteo
           << "  Bytes vivos estimados: " << bytes_totales << "\n";
        unlock();
    }

    long long count() const { return conteo.load(std::memory_order_relaxed); }
    long long bytes() const { return bytes_totales.load(std::memory_order_relaxed); }

private:
    // Spinlock simple
    void lock()   { while (flag.test_and_set(std::memory_order_acquire)) { } }
    void unlock() { flag.clear(std::memory_order_release); }

    Guardado* cabeza;
    std::atomic<long long> conteo;
    std::atomic<long long> bytes_totales;
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

// Lista global (definida en memory_instrumentation.cpp)
extern ListaGuardado listaGlobal;

#endif // LISTA_GUARDADO_H







#ifndef LISTA_GUARDADO_H
#define LISTA_GUARDADO_H

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <atomic>
#include <iostream>

// Lista sin usar new/delete (evita reentrancia). Usa malloc/free y spinlock.
struct Guardado {
    void* direccion;
    std::size_t tamano;
    char tipo[16];       // "new" / "new[]"
    char archivo[128];
    int  linea;
    std::time_t marcaDeTiempo;
    Guardado* siguiente;
};

class ListaGuardado {
public:
    ListaGuardado() : cabeza(nullptr), conteo(0), bytes_totales(0) { flag.clear(); }

    // === FIRMA NUEVA: 5 argumentos (ptr, bytes, tipo, archivo, linea)
    void agregar(void* ptr, std::size_t bytes, const char* tipo, const char* archivo, int linea) {
        if (!ptr) return;
        Guardado* n = static_cast<Guardado*>(std::malloc(sizeof(Guardado)));
        if (!n) return;
        n->direccion = ptr;
        n->tamano = bytes;
        std::memset(n->tipo, 0, sizeof(n->tipo));
        std::memset(n->archivo, 0, sizeof(n->archivo));
        if (tipo)    std::strncpy(n->tipo,    tipo,    sizeof(n->tipo)    - 1);
        if (archivo) std::strncpy(n->archivo, archivo, sizeof(n->archivo) - 1);
        n->linea = linea;
        std::time(&n->marcaDeTiempo);

        lock();
        n->siguiente = cabeza;
        cabeza = n;
        ++conteo;
        bytes_totales += (long long)bytes;
        unlock();
    }

    // === FIRMA NUEVA: devuelve tamaño y opcionalmente archivo/linea
    std::size_t eliminar_y_tamano(void* ptr, const char** archivoOut = nullptr, int* lineaOut = nullptr) {
        if (!ptr) return 0;
        lock();
        Guardado* prev = nullptr;
        Guardado* cur  = cabeza;
        while (cur) {
            if (cur->direccion == ptr) {
                if (prev) prev->siguiente = cur->siguiente;
                else      cabeza = cur->siguiente;

                std::size_t sz = cur->tamano;
                if (archivoOut) *archivoOut = cur->archivo;
                if (lineaOut)   *lineaOut   = cur->linea;

                --conteo;
                bytes_totales -= (long long)sz;
                unlock();
                std::free(cur);
                return sz;
            }
            prev = cur;
            cur  = cur->siguiente;
        }
        unlock();
        return 0;
    }

    // Versión simple (compatible con código viejo)
    void eliminar(void* ptr) { (void)eliminar_y_tamano(ptr); }

    void reportarFugas(std::ostream& os = std::cout) {
        lock();
        if (!cabeza) {
            os << "[ListaGuardado] Sin registros pendientes.\n";
            unlock();
            return;
        }
        os << "[ListaGuardado] Posibles fugas:\n";
        Guardado* cur = cabeza;
        int i = 0; long long totalB = 0;
        while (cur) {
            os << "  #" << i++
               << " ptr="   << cur->direccion
               << " bytes=" << cur->tamano
               << " tipo="  << cur->tipo
               << " file="  << cur->archivo << ":" << cur->linea
               << " t="     << (long long)cur->marcaDeTiempo
               << "\n";
            totalB += (long long)cur->tamano;
            cur = cur->siguiente;
        }
        os << "Total: " << i << " asignaciones; " << totalB << " bytes.\n";
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

// Definida en memory_instrumentation.cpp
extern ListaGuardado listaGlobal;

#endif // LISTA_GUARDADO_H








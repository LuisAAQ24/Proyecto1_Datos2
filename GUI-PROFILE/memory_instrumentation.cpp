#include "memory_instrumentation.h"
#include "ListaGuardado.h"

#include <atomic>
#include <cstdlib>
#include <new>
#include <cstdio>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>

// ===== Globals =====
bool profilerActivo = false;
static std::atomic<long long> g_allocs_vivas{0};
static std::atomic<long long> g_bytes_vivos{0};
static std::atomic<long long> g_total_allocs{0};

// Lista global concreta
ListaGuardado listaGlobal;

// ====== Reporter por stdout ======
static std::atomic<bool> g_reporter_on{false};
static std::thread g_reporter_thread;
static std::mutex g_reporter_mtx;

long long conteoAsignacionesVivas() { return g_allocs_vivas.load(std::memory_order_relaxed); }
long long bytesVivos()              { return g_bytes_vivos.load(std::memory_order_relaxed); }

void reporteAlSalir() {
    bool prev = profilerActivo;
    profilerActivo = false; // Evita contaminar conteos por asignaciones del streaming
    const auto vivas = conteoAsignacionesVivas();

    std::cout << "[Profiler] Asignaciones vivas al salir: " << vivas << "\n";
    if (vivas == 0) std::cout << "[Profiler] No se detectaron fugas.\n";
    else            std::cout << "[Profiler] Posibles fugas detectadas (ver listado).\n";

    // Listado detallado
    listaGlobal.reportarFugas(std::cout);
    profilerActivo = prev;
}

// Helper: eliminar y obtener tamaño (si no existe, devuelve 0)
static std::size_t pop_size_for(void* p) {
    return listaGlobal.eliminar_y_tamano(p);
}

// ===== Reentrancia: evitamos trabajo dentro de nuestros hooks si se vuelve a llamar =====
static thread_local bool g_inHook = false;

// ===== Sobrecargas globales new/delete =====
void* operator new(std::size_t tamano) {
    void* direccion = std::malloc(tamano);
    if (!direccion) throw std::bad_alloc();

    if (profilerActivo && !g_inHook) {
        g_inHook = true;
        g_allocs_vivas.fetch_add(1, std::memory_order_relaxed);
        g_total_allocs.fetch_add(1, std::memory_order_relaxed);
        g_bytes_vivos.fetch_add(static_cast<long long>(tamano), std::memory_order_relaxed);
        // No usar new aquí, solo APIs de ListaGuardado (usa malloc)
        listaGlobal.agregar(direccion, tamano, "new");
        g_inHook = false;
    }
    return direccion;
}

void operator delete(void* direccion) noexcept {
    if (!direccion) return;

    if (profilerActivo && !g_inHook) {
        g_inHook = true;
        g_allocs_vivas.fetch_sub(1, std::memory_order_relaxed);
        std::size_t tam = pop_size_for(direccion);
        if (tam > 0) {
            g_bytes_vivos.fetch_sub(static_cast<long long>(tam), std::memory_order_relaxed);
        }
        g_inHook = false;
    }
    std::free(direccion);
}

void* operator new[](std::size_t tamano) {
    void* direccion = std::malloc(tamano);
    if (!direccion) throw std::bad_alloc();

    if (profilerActivo && !g_inHook) {
        g_inHook = true;
        g_allocs_vivas.fetch_add(1, std::memory_order_relaxed);
        g_total_allocs.fetch_add(1, std::memory_order_relaxed);
        g_bytes_vivos.fetch_add(static_cast<long long>(tamano), std::memory_order_relaxed);
        listaGlobal.agregar(direccion, tamano, "new[]");
        g_inHook = false;
    }
    return direccion;
}

void operator delete[](void* direccion) noexcept {
    if (!direccion) return;

    if (profilerActivo && !g_inHook) {
        g_inHook = true;
        g_allocs_vivas.fetch_sub(1, std::memory_order_relaxed);
        std::size_t tam = pop_size_for(direccion);
        if (tam > 0) {
            g_bytes_vivos.fetch_sub(static_cast<long long>(tam), std::memory_order_relaxed);
        }
        g_inHook = false;
    }
    std::free(direccion);
}

// ===== Emisión por stdout para que otro proceso lea y grafique =====
void memprof_start_stdout_report(int interval_ms) {
    std::lock_guard<std::mutex> lock(g_reporter_mtx);
    if (g_reporter_on.load()) return;

    g_reporter_on.store(true);
    g_reporter_thread = std::thread([interval_ms]{
        using namespace std::chrono;
        const auto t0 = steady_clock::now();
        while (g_reporter_on.load()) {
            auto dt = duration_cast<milliseconds>(steady_clock::now() - t0).count();
            auto live  = g_allocs_vivas.load(std::memory_order_relaxed);
            auto bytes = g_bytes_vivos.load(std::memory_order_relaxed);
            auto tot   = g_total_allocs.load(std::memory_order_relaxed);

            std::printf("MEMPROF t_ms=%lld live=%lld bytes=%lld total=%lld\n",
                        static_cast<long long>(dt),
                        static_cast<long long>(live),
                        static_cast<long long>(bytes),
                        static_cast<long long>(tot));
            std::fflush(stdout);

            std::this_thread::sleep_for(milliseconds(interval_ms));
        }
    });
}

void memprof_stop_stdout_report() {
    std::lock_guard<std::mutex> lock(g_reporter_mtx);
    if (!g_reporter_on.load()) return;
    g_reporter_on.store(false);
    if (g_reporter_thread.joinable()) g_reporter_thread.join();
}







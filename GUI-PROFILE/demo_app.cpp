#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>

#include "memory_instrumentation.h"

// Este programa SÍ se mide a sí mismo
extern bool profilerActivo;

int main() {
    // Activa la instrumentación en este proceso
    profilerActivo = true;

    // Emite métricas por stdout cada 200 ms con el formato:
    // MEMPROF t_ms=<T> live=<N> bytes=<B> total=<A>
    memprof_start_stdout_report(200);

    std::vector<void*> leaks;              // acumulamos algunas fugas simuladas
    std::mt19937 rng{12345};
    std::uniform_int_distribution<int> distChunk(1, 64);  // tamaño en KB
    std::uniform_int_distribution<int> distAction(0, 9);

    const int STEPS = 300; // ~60 s si dormimos 200 ms
    for (int i = 0; i < STEPS; ++i) {
        // Crear varios bloques en cada iteración
        int blocks = 5 + (i % 10);
        for (int b = 0; b < blocks; ++b) {
            int kb = distChunk(rng);
            std::size_t sz = static_cast<std::size_t>(kb) * 1024u;

            char* p = new char[sz];
            p[0] = static_cast<char>(kb);
            p[sz - 1] = static_cast<char>(kb);

            // Con cierta probabilidad, simulamos fuga (no liberamos)
            if (distAction(rng) < 2) {
                leaks.push_back(p);
            } else {
                delete[] p;
            }
        }

        // Liberar parte de lo "fugado" de vez en cuando (comportamiento realista)
        if (i % 25 == 0 && !leaks.empty()) {
            const int toFree = static_cast<int>(leaks.size()) / 3;
            for (int k = 0; k < toFree; ++k) {
                delete[] static_cast<char*>(leaks.back());
                leaks.pop_back();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Dejamos algunas fugas sin liberar a propósito para el reporte final
    memprof_stop_stdout_report();
    reporteAlSalir();
    return 0;
}

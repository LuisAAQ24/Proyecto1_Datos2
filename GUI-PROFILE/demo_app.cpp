// 1) Incluye primero tus headers estándar (y cualquier otro de sistema/Qt)
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>

// 2) Recién ahora activamos la redefinición de 'new' con file/line
#define MEMPROF_REDEFINE_NEW 1
#include "memory_instrumentation.h"

// 3) Resto del código de la demo
extern bool profilerActivo;

int main() {
    // Activa instrumentación
    profilerActivo = true;

    // Conexión al servidor de la GUI (que escucha en localhost:5555)
    memprof_set_server("127.0.0.1", 5555);
    if (!memprof_connect()) {
        std::cerr << "No se pudo conectar al servidor de la GUI.\n";
        return 1;
    }
    // Ticks para la línea de tiempo
    memprof_start_ticks(200);

    std::vector<void*> leaks;
    std::mt19937 rng{12345};
    std::uniform_int_distribution<int> distChunk(1, 64);  // KB
    std::uniform_int_distribution<int> distAction(0, 9);

    const int STEPS = 300; // ~60 s si dormimos 200 ms
    for (int i = 0; i < STEPS; ++i) {
        int blocks = 5 + (i % 10);
        for (int b = 0; b < blocks; ++b) {
            int kb = distChunk(rng);
            std::size_t sz = static_cast<std::size_t>(kb) * 1024u;

            // Gracias a MEMPROF_REDEFINE_NEW, estas líneas llevan __FILE__/__LINE__
            char* p = new char[sz];
            p[0] = static_cast<char>(kb);
            p[sz - 1] = static_cast<char>(kb);

            if (distAction(rng) < 2) {
                leaks.push_back(p);   // simular fuga
            } else {
                delete[] p;
            }
        }

        if (i % 25 == 0 && !leaks.empty()) {
            const int toFree = static_cast<int>(leaks.size()) / 3;
            for (int k = 0; k < toFree; ++k) {
                delete[] static_cast<char*>(leaks.back());
                leaks.pop_back();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Dejamos algunas fugas a propósito para que la GUI las reporte al desconectar.
    memprof_stop_ticks();
    memprof_disconnect();

    // Reporte textual (opcional, para consola)
    reporteAlSalir();
    return 0;
}



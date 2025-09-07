#ifndef MEMORY_INSTRUMENTATION_H
#define MEMORY_INSTRUMENTATION_H

#include <cstddef>

// ===== Flag global para habilitar/deshabilitar el profiler en ESTE proceso =====
extern bool profilerActivo;

// ===== Métricas en este proceso =====
long long conteoAsignacionesVivas();
long long bytesVivos();

// ===== Reporte al finalizar (dump de posibles fugas) =====
void reporteAlSalir();

// ===== Reporte periódico por stdout (para que otro proceso lo lea) =====
// Formato de línea:
// MEMPROF t_ms=<T> live=<N> bytes=<B> total=<A>
void memprof_start_stdout_report(int interval_ms = 200);
void memprof_stop_stdout_report();

// Lista global (definida en .cpp) para quienes necesiten consultarla
class ListaGuardado;
extern ListaGuardado listaGlobal;

#endif // MEMORY_INSTRUMENTATION_H

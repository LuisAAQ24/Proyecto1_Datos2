#ifndef MEMORY_INSTRUMENTATION_H
#define MEMORY_INSTRUMENTATION_H

#include <cstddef>
#include <cstdint>

// ===== Flag global de runtime =====
extern bool profilerActivo;

// ===== Métricas internas =====
long long conteoAsignacionesVivas();
long long bytesVivos();
void reporteAlSalir();

// ===== Cliente de sockets (para enviar a la GUI) =====
void memprof_set_server(const char* host, uint16_t port);
bool memprof_connect();
void memprof_disconnect();

// Envía ticks periódicos (cada interval_ms) con:
// {"type":"tick","live":N,"bytes":B,"total":A,"t_ms":T}
void memprof_start_ticks(int interval_ms=200);
void memprof_stop_ticks();

// ===== Sobrecargas de 'new' con archivo/línea (declaraciones) =====
// IMPORTANTE: estas declaraciones deben ser visibles en el TU que use el macro.
void* operator new  (std::size_t size, const char* file, int line);
void* operator new[](std::size_t size, const char* file, int line);
void  operator delete  (void* p, const char* file, int line) noexcept;
void  operator delete[](void* p, const char* file, int line) noexcept;

// ===== Macros opcionales para capturar archivo/linea =====
// Actívalo SOLO en el programa medido, y SIEMPRE después de incluir
// headers del sistema/Qt (para no romperlos).
#ifdef MEMPROF_REDEFINE_NEW
#define new new(__FILE__, __LINE__)
#endif

// ===== Lista global =====
class ListaGuardado;
extern ListaGuardado listaGlobal;

#endif // MEMORY_INSTRUMENTATION_H

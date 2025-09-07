#include "memory_instrumentation.h"
#include "ListaGuardado.h"

#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <new>
#include <thread>
#include <mutex>
#include <chrono>
#include <ctime>
#include <cstdint>

// ===== Sockets (BSD / WinSock) =====
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
static SOCKET g_sock = INVALID_SOCKET;
static bool   g_wsa_init = false;
static bool socket_init() {
    if (!g_wsa_init) {
        WSADATA wsaData{};
        if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) return false;
        g_wsa_init = true;
    }
    return true;
}
static void socket_close() {
    if (g_sock != INVALID_SOCKET) { closesocket(g_sock); g_sock = INVALID_SOCKET; }
}
static int socket_send(const char* data, int len) {
    if (g_sock == INVALID_SOCKET) return -1;
    return send(g_sock, data, len, 0);
}
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
static int g_sock = -1;
static bool socket_init() { return true; }
static void socket_close() { if (g_sock >= 0) { ::close(g_sock); g_sock = -1; } }
static int socket_send(const char* data, int len) {
    if (g_sock < 0) return -1;
    return ::send(g_sock, data, len, 0);
}
#endif

// ===== Globals =====
bool profilerActivo = false;
static std::atomic<long long> g_allocs_vivas{0};
static std::atomic<long long> g_bytes_vivos{0};
static std::atomic<long long> g_total_allocs{0};

ListaGuardado listaGlobal;

// servidor destino
static char g_host[128] = "127.0.0.1";
static uint16_t g_port = 5555;

// envío seguro
static std::mutex g_send_mtx;

static std::atomic<bool> g_tick_on{false};
static std::thread g_tick_thread;
static std::chrono::steady_clock::time_point g_start_tp;

// reentrancia
static thread_local bool g_inHook = false;

// ===== Helpers =====
static long long now_ms_epoch() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
static long long now_ms_zero() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now() - g_start_tp).count();
}
static void send_json_line(const char* json) {
    std::lock_guard<std::mutex> lk(g_send_mtx);
    socket_send(json, (int)std::strlen(json));
    socket_send("\n", 1);
}

static void instrument_after_alloc(void* ptr, std::size_t bytes, bool isArray,
                                   const char* file, int line)
{
    // Actualiza métricas/estructura
    g_allocs_vivas.fetch_add(1, std::memory_order_relaxed);
    g_total_allocs.fetch_add(1, std::memory_order_relaxed);
    g_bytes_vivos.fetch_add((long long)bytes, std::memory_order_relaxed);
    listaGlobal.agregar(ptr, bytes, isArray ? "new[]" : "new", file ? file : "desconocido", line);

    // Envía JSON si hay socket
    if (
#ifdef _WIN32
        g_sock != INVALID_SOCKET
#else
        g_sock >= 0
#endif
        ) {
        // {"type":"alloc","ptr":123,"bytes":4096,"array":true,"file":"x.cpp","line":42,"ts_ms":1234}
        char buf[512];
        std::snprintf(buf, sizeof(buf),
                      "{\"type\":\"alloc\",\"ptr\":%llu,\"bytes\":%llu,"
                      "\"array\":%s,\"file\":\"%s\",\"line\":%d,\"ts_ms\":%lld}",
                      (unsigned long long)(uintptr_t)ptr,
                      (unsigned long long)bytes,
                      isArray ? "true" : "false",
                      file ? file : "desconocido",
                      line,
                      (long long)now_ms_epoch());
        send_json_line(buf);
    }
}

static void instrument_before_free(void* ptr)
{
    // extrae tamaño + archivo/linea (si están)
    const char* file = nullptr;
    int line = -1;
    std::size_t sz = listaGlobal.eliminar_y_tamano(ptr, &file, &line);
    if (sz > 0) {
        g_allocs_vivas.fetch_sub(1, std::memory_order_relaxed);
        g_bytes_vivos.fetch_sub((long long)sz, std::memory_order_relaxed);
    }

    if (
#ifdef _WIN32
        g_sock != INVALID_SOCKET
#else
        g_sock >= 0
#endif
        ) {
        // {"type":"free","ptr":123,"ts_ms":2345}
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "{\"type\":\"free\",\"ptr\":%llu,\"ts_ms\":%lld}",
                      (unsigned long long)(uintptr_t)ptr,
                      (long long)now_ms_epoch());
        send_json_line(buf);
    }
}

// ===== API =====
long long conteoAsignacionesVivas() { return g_allocs_vivas.load(std::memory_order_relaxed); }
long long bytesVivos()              { return g_bytes_vivos.load(std::memory_order_relaxed); }

void reporteAlSalir() {
    bool prev = profilerActivo;
    profilerActivo = false;
    listaGlobal.reportarFugas();
    profilerActivo = prev;
}

void memprof_set_server(const char* host, uint16_t port) {
    if (host && *host) {
        std::snprintf(g_host, sizeof(g_host), "%s", host);
        g_host[sizeof(g_host)-1] = '\0';
    }
    g_port = port;
}

bool memprof_connect() {
    if (!socket_init()) return false;

    // Cerrar si había uno abierto
    socket_close();

    // Resolver y conectar
#ifdef _WIN32
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;

    char portStr[16]; std::snprintf(portStr, sizeof(portStr), "%u", (unsigned)g_port);
    if (getaddrinfo(g_host, portStr, &hints, &res) != 0 || !res) return false;

    SOCKET s = INVALID_SOCKET;
    for (addrinfo* p = res; p; p = p->ai_next) {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == INVALID_SOCKET) continue;
        if (connect(s, p->ai_addr, (int)p->ai_addrlen) == 0) { g_sock = s; break; }
        closesocket(s); s = INVALID_SOCKET;
    }
    freeaddrinfo(res);
    if (g_sock == INVALID_SOCKET) return false;
#else
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;

    char portStr[16]; std::snprintf(portStr, sizeof(portStr), "%u", (unsigned)g_port);
    if (getaddrinfo(g_host, portStr, &hints, &res) != 0 || !res) return false;

    int s = -1;
    for (addrinfo* p = res; p; p = p->ai_next) {
        s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s < 0) continue;
        if (::connect(s, p->ai_addr, p->ai_addrlen) == 0) { g_sock = s; break; }
        ::close(s); s = -1;
    }
    freeaddrinfo(res);
    if (g_sock < 0) return false;
#endif

    // Punto de referencia de tiempo para ticks
    g_start_tp = std::chrono::steady_clock::now();
    return true;
}

void memprof_disconnect() {
    memprof_stop_ticks();
    socket_close();
}

void memprof_start_ticks(int interval_ms) {
    if (g_tick_on.load()) return;
    g_tick_on.store(true);
    g_start_tp = std::chrono::steady_clock::now();

    g_tick_thread = std::thread([interval_ms]{
        using namespace std::chrono;
        while (g_tick_on.load()) {
            long long t_ms = now_ms_zero();
            long long live = g_allocs_vivas.load(std::memory_order_relaxed);
            long long byts = g_bytes_vivos.load(std::memory_order_relaxed);
            long long tot  = g_total_allocs.load(std::memory_order_relaxed);

            char buf[256];
            std::snprintf(buf, sizeof(buf),
                          "{\"type\":\"tick\",\"live\":%lld,\"bytes\":%lld,\"total\":%lld,\"t_ms\":%lld}",
                          live, byts, tot, t_ms);
            send_json_line(buf);

            std::this_thread::sleep_for(milliseconds(interval_ms));
        }
    });
}

void memprof_stop_ticks() {
    if (!g_tick_on.load()) return;
    g_tick_on.store(false);
    if (g_tick_thread.joinable()) g_tick_thread.join();
}

// ===== Sobrecargas new/delete =====

// Formas "normales"
void* operator new(std::size_t size) {
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    if (profilerActivo && !g_inHook) {
        g_inHook = true;
        instrument_after_alloc(p, size, false, "desconocido", -1);
        g_inHook = false;
    }
    return p;
}
void operator delete(void* p) noexcept {
    if (!p) return;
    if (profilerActivo && !g_inHook) {
        g_inHook = true;
        instrument_before_free(p);
        g_inHook = false;
    }
    std::free(p);
}

void* operator new[](std::size_t size) {
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    if (profilerActivo && !g_inHook) {
        g_inHook = true;
        instrument_after_alloc(p, size, true, "desconocido", -1);
        g_inHook = false;
    }
    return p;
}
void operator delete[](void* p) noexcept {
    if (!p) return;
    if (profilerActivo && !g_inHook) {
        g_inHook = true;
        instrument_before_free(p);
        g_inHook = false;
    }
    std::free(p);
}

// Formas con archivo/linea (habilitadas si el programa define MEMPROF_REDEFINE_NEW)
void* operator new(std::size_t size, const char* file, int line) {
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    if (profilerActivo && !g_inHook) {
        g_inHook = true;
        instrument_after_alloc(p, size, false, file, line);
        g_inHook = false;
    }
    return p;
}
void* operator new[](std::size_t size, const char* file, int line) {
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    if (profilerActivo && !g_inHook) {
        g_inHook = true;
        instrument_after_alloc(p, size, true, file, line);
        g_inHook = false;
    }
    return p;
}
// deletes de “matching” para el caso de excepciones en el ctor
void operator delete(void* p, const char*, int) noexcept { std::free(p); }
void operator delete[](void* p, const char*, int) noexcept { std::free(p); }







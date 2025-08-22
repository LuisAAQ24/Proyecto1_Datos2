#include <iostream>
#include <cstdlib>
#include <new>

// Sobrecarga de new
void* operator new(std::size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    std::cout << "[new] Asignados " << size << " bytes en " << ptr << std::endl;
    return ptr;
}

// Sobrecarga de delete
void operator delete(void* ptr) noexcept {
    std::cout << "[delete] Liberando memoria en " << ptr << std::endl;
    std::free(ptr);
}

// Sobrecarga de new[] (arreglos)
void* operator new[](std::size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    std::cout << "[new[]] Asignados " << size << " bytes en " << ptr << std::endl;
    return ptr;
}

// Sobrecarga de delete[] (arreglos)
void operator delete[](void* ptr) noexcept {
    std::cout << "[delete[]] Liberando memoria en " << ptr << std::endl;
    std::free(ptr);
}

// Función main para probar
int main() {
    int* a = new int;
    delete a;

    int* arr = new int[5];
    delete[] arr;

    double* d = new double[3];
    delete[] d;

    char* c = new char;
    delete c;

    return 0;
}




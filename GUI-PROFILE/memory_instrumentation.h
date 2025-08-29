#ifndef MEMORY_INSTRUMENTATION_H
#define MEMORY_INSTRUMENTATION_H

#include <cstddef>

void* operator new(std::size_t size);
void operator delete(void* ptr) noexcept;
void* operator new[](std::size_t size);
void operator delete[](void* ptr) noexcept;

#endif

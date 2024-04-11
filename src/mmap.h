#ifndef MMAP_H
#define MMAP_H

#include "types.h"

extern void pru_init();
extern void gpio_init();

void* mmap_get_mapping(uintptr_t key, size_t size);

#endif
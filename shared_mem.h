/*

- Shared memory space (CPU RAM)
- Also interfacing for triggering cpu interrupts
- Communication via buses

*/
#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include "rom.h"
#include <stdint.h>

typedef struct {
	uint8_t cpu_memory[0xFFFF];
} SharedMemory;

uint8_t read_cpu_memory(SharedMemory* mem, uint16_t addr);
void write_cpu_memory(SharedMemory* mem, uint16_t addr, uint8_t byte);
void load_pgr_banks(SharedMemory* mem, Rom *rom);

#endif

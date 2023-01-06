/*

- Shared memory space (CPU RAM)
- Also interfacing for triggering cpu interrupts
- Communication via buses

*/
#include "utils.h"

typedef struct {
  uint8 cpu_memory[0xFFFF];
} SharedMemory;

uint8 read_cpu_memory(SharedMemory* mem, uint16 addr);
void write_cpu_memory(SharedMemory* mem, uint16 addr, uint8 byte);

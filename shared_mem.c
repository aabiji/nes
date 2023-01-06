#include "shared_mem.h"

uint8 read_cpu_memory(SharedMemory *mem, uint16 addr)
{
  return mem->cpu_memory[addr];
}

void write_cpu_memory(SharedMemory *mem, uint16 addr, uint8 byte)
{
  mem->cpu_memory[addr] = byte;
}

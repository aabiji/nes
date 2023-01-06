#include "cpu.h"
#include <stdio.h>

void init_cpu(Cpu *cpu, SharedMemory *mem)
{
  cpu->cycle_count = 0;
  cpu->memspace = mem;
}
  
uint8 read_byte(Cpu* cpu, uint16 addr)
{
  cpu->cycle_count ++;
  return read_cpu_memory(cpu->memspace, addr);
}

void write_byte(Cpu* cpu, uint16 addr, uint8 byte)
{
  cpu->cycle_count ++;
  write_cpu_memory(cpu->memspace, addr, byte);
}

void execute_cycles(Cpu *cpu)
{
  printf("%d\n", read_byte(cpu, 0xFFFA));
  
  printf("%d\n", cpu->cycle_count);
}


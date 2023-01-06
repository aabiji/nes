#include "cpu.h"
#include <stdio.h>

static uint8 read_byte(Cpu* cpu, uint16 addr)
{
  cpu->cycle_count ++;
  return read_cpu_memory(cpu->memspace, addr);
}

static void write_byte(Cpu* cpu, uint16 addr, uint8 byte)
{
  cpu->cycle_count ++;
  write_cpu_memory(cpu->memspace, addr, byte);
}

/* Writing status flags to a byte */
static uint8 write_status_flag(Cpu *cpu)
{
  uint8 byte;

  byte &=  cpu->C;
  byte &= (cpu->Z << 1);
  byte &= (cpu->I << 2);
  byte &= (cpu->D << 3);
  byte &= (cpu->B << 4);
  byte &= (1 << 5); // Unused flag always set to 1
  byte &= (cpu->V << 6);
  byte &= (cpu->N << 7);
  
  return byte;
}

/* Reading a byte into the status flags */
static void read_status_flag(Cpu *cpu, uint8 byte)
{
  cpu->C = byte & 1;
  cpu->Z = byte & 2;
  cpu->I = byte & 4;
  cpu->D = byte & 8;
  cpu->B = byte & 16;
  cpu->V = byte & 64;
  cpu->N = byte & 128;
}

void init_cpu(Cpu *cpu, SharedMemory *mem)
{
  cpu->cycle_count = 0;
  cpu->memspace = mem;

  cpu->PC = 0xFFFC;
  cpu->SP = 0xFD;
  cpu->X = 0;
  cpu->Y = 0;
  cpu->A = 0;

  read_status_flag(cpu, 0x34);
}

void execute_cpu_instructions(Cpu *cpu)
{
  printf("%d\n", read_byte(cpu, 0xFFFA));
  printf("%d\n", cpu->cycle_count);
}

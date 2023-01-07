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

/* Fetch memory location for instruction's operations */
static uint16
fetch_instruction_addr(Cpu *cpu, int addr_mode, bool is_read, bool is_rmw)
{
  uint16 memory_addr = 0;

  switch (addr_mode)
  {
  case implied:
  case accumulator:
  case immediate:
  case relative:
	memory_addr = read_byte(cpu, cpu->PC++);
	break;

  case zero_page:
	memory_addr = read_byte(cpu, cpu->PC++);
	break;

  case zero_page_x:
  {
	uint8 addr = read_byte(cpu, cpu->PC++);
	read_byte(cpu, (0x00FF | addr));
	addr += cpu->X;
	memory_addr = (0x00FF | addr);
	break;
  }

  case zero_page_y:
  {
	uint8 addr = read_byte(cpu, cpu->PC++);
	read_byte(cpu, (0x00FF | addr));
	addr += cpu->Y;
	memory_addr = (0x00FF | addr);
	break;
  }

  case absolute:
  {
	uint8 low = read_byte(cpu, cpu->PC++);
	uint8 high = read_byte(cpu, cpu->PC++);
	memory_addr = (high << 8) | low;
	break;
  }

  case absolute_x:
  {
	uint8 low = read_byte(cpu, cpu->PC++);
	uint8 high = read_byte(cpu, cpu->PC++);
	uint8 new_low = low + cpu->X;
	
	if (new_low < low)
	{
	  high ++;
	  if (is_read)
		read_byte(cpu, (high << 8) | new_low);
	}

	if (!is_read && is_rmw)
	{
	  read_byte(cpu, (high << 8) | new_low);
	}

	memory_addr = (high << 8) | new_low;
	break;
  }

  case absolute_y:
  {
	uint8 low = read_byte(cpu, cpu->PC++);
	uint8 high = read_byte(cpu, cpu->PC++);
	uint8 new_low = low + cpu->Y;
	
	if (new_low < low)
	{
	  high ++;
	  if (is_read)
		read_byte(cpu, (high << 8) | new_low);
	}

	if (!is_read && is_rmw)
	  read_byte(cpu, (high << 8) | new_low);
	
	memory_addr = (high << 8) | new_low;
	break;
  }

  case indirect:
  {
	uint8 pointer_low = read_byte(cpu, cpu->PC++);
	uint8 pointer_high = read_byte(cpu, cpu->PC++);
	uint8 addr_low = read_byte(cpu, (pointer_high << 8) | pointer_low);
	uint8 addr_high = read_byte(cpu, ((pointer_high << 8) | pointer_low) + 1);
    memory_addr = (addr_high << 8) | addr_low;
	break;
  }

  case indirect_x:
  {
	uint8 pointer_addr = read_byte(cpu, cpu->PC++);
	read_byte(cpu, (0x0000 | pointer_addr));
	pointer_addr += cpu->X;
	uint8 low = read_byte(cpu, (0x0000 | pointer_addr));
	uint8 high = read_byte(cpu, (0x0000 | pointer_addr) + 1);
	memory_addr = (high << 8) | low;
	break;
  }

  case indirect_y:
  {
	uint8 pointer_addr = read_byte(cpu, cpu->PC++);
	uint8 addr_low = read_byte(cpu, (0x0000 | pointer_addr));
	uint8 addr_high = read_byte(cpu, (0x0000 | pointer_addr) + 1);
	uint8 new_low = addr_low + cpu->Y;

	if (new_low < addr_low)
	{
	  addr_high ++;
	  if (is_read)
		read_byte(cpu, (addr_high << 8) | addr_low);
	}

	if (!is_read && is_rmw)
	  read_byte(cpu, (addr_high << 8) | addr_low);

	memory_addr = (addr_high << 8) | new_low;
	break;
  }
  
  }

  return memory_addr;
}

void LDA(Cpu *cpu, int addr_mode)
{
  uint16 addr = fetch_instruction_addr(cpu, addr_mode, true, false);
  uint8 byte = read_byte(cpu, addr);
  cpu->A = byte;
  
  cpu->N = (cpu->A & 0x80) != 0;
  cpu->Z = (cpu->A == 0);
}

void LDX(Cpu *cpu, int addr_mode)
{
  uint16 addr = fetch_instruction_addr(cpu, addr_mode, true, false);
  uint8 byte = read_byte(cpu, addr);
  cpu->X = byte;
  
  cpu->N = (cpu->X & 0x80) != 0;
  cpu->Z = (cpu->X == 0);
}

void LDY(Cpu *cpu, int addr_mode)
{
  uint16 addr = fetch_instruction_addr(cpu, addr_mode, true, false);
  uint8 byte = read_byte(cpu, addr);
  cpu->Y = byte;

  cpu->X = (cpu->Y & 0x80) != 0;
  cpu->Z = (cpu->Y == 0);
}

void STA(Cpu *cpu, int addr_mode)
{
  uint16 addr = fetch_instruction_addr(cpu, addr_mode, false, false);
  write_byte(cpu, addr, cpu->A);
}

void STX(Cpu *cpu, int addr_mode)
{
  uint16 addr = fetch_instruction_addr(cpu, addr_mode, false, false);
  write_byte(cpu, addr, cpu->X);
}

void STY(Cpu *cpu, int addr_mode)
{
  uint16 addr = fetch_instruction_addr(cpu, addr_mode, false, false);
  write_byte(cpu, addr, cpu->Y);
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

static void (*instruction)(Cpu *cpu, int addr_mode);
void execute_cpu_instructions(Cpu *cpu)
{
  while (cpu->cycle_count < CPU_CYCLES_PER_FRAME)
  {
	uint8 opcode = read_byte(cpu, cpu->PC++);
	int addr_mode = addressing_modes[opcode];
	
	instruction = opcodes[opcode];
	(*instruction)(cpu, addr_mode);
  }
}

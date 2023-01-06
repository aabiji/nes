/*

2A03 implementation
- a 6502 without a BCD running a pAPU coprocessor

Ideas
- Breaking instructions into fetching operands,
  and actual logic
- fetch_operand(addr_mode);
- Shared memory for communication between components
- Official instructions
- Unofficial instructions
- Functions pointers to instructions to modularize them
- Interrupt handling
- Test suite, comparing cpu state to known log

 */
#include "utils.h"
#include "shared_mem.h"

typedef struct {
  int cycle_count;
  SharedMemory *memspace;
} Cpu;

void init_cpu(Cpu *cpu, SharedMemory *mem);

uint8 read_byte(Cpu* cpu, uint16 addr);
void write_byte(Cpu* cpu, uint16 addr, uint8 byte);
void execute_cycles(Cpu *cpu);

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
  uint8 A;   // Accumulator
  uint8 X;   // X register
  uint8 Y;   // Y register
  uint8 SP;  // Stack pointer
  uint16 PC; // Program counter

  /* Processor status flags */
  uint8 C; // Carry
  uint8 Z; // Zero
  uint8 I; // Interrupt disable
  uint8 D; // Decimal
  uint8 B; // Break
  uint8 V; // Overflow
  uint8 N; // Negative
  
  int cycle_count;
  SharedMemory *memspace;
} Cpu;

enum addressing_modes {
  implied     = 0,
  accumulator = 1,
  immediate   = 2,
  zero_page   = 3,
  zero_page_x = 4,
  zero_page_y = 5,
  relative    = 6,
  absolute    = 7,
  absolute_x  = 8,
  absolute_y  = 9,
  indirect    = 10,
  indirect_x  = 11, // Indexed indirect
  indirect_y  = 12, // Indirect indexed

  // Custom addressing modes for instructions
  // taking an extra cycle to correct the high bit
  // during a read
  absolute_x_read = 13,
  absolute_y_read = 14,
  indirect_y_read = 15
};

void init_cpu(Cpu *cpu, SharedMemory *mem);
void execute_cpu_instructions(Cpu *cpu);

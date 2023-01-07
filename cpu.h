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

#define CPU_CYCLES_PER_FRAME 1

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
};

// Official instructions
void LDA(Cpu *cpu, int addr_mode); // Load accumulator 
void LDX(Cpu *cpu, int addr_mode); // Load X register
void LDY(Cpu *cpu, int addr_mode); // Load Y register 
void STA(Cpu *cpu, int addr_mode); // Store accumulator
void STX(Cpu *cpu, int addr_mode); // Store X register
void STY(Cpu *cpu, int addr_mode); // Store Y register
void TAX(Cpu *cpu, int addr_mode); // Transfer accumulator to X
void TAY(Cpu *cpu, int addr_mode); // Transfer accumulator to Y
void TXA(Cpu *cpu, int addr_mode); // Transfer X to accumulator
void TYA(Cpu *cpu, int addr_mode); // Transfer Y to accumulator
void TSX(Cpu *cpu, int addr_mode); // Transfer SP to X
void TXS(Cpu *cpu, int addr_mode); // Transfer X to SP
void PHA(Cpu *cpu, int addr_mode); // Push accumulator to stack
void PHP(Cpu *cpu, int addr_mode); // Push processor status to stack
void PLA(Cpu *cpu, int addr_mode); // Pull accumulator from stack
void PLP(Cpu *cpu, int addr_mode); // Pull processor status from stack
void AND(Cpu *cpu, int addr_mode); // AND
void EOR(Cpu *cpu, int addr_mode); // XOR
void ORA(Cpu *cpu, int addr_mode); // OR
void BIT(Cpu *cpu, int addr_mode); // Bit test
void ADC(Cpu *cpu, int addr_mode); // Add with carry
void SBC(Cpu *cpu, int addr_mode); // Subtract with carry
void CMP(Cpu *cpu, int addr_mode); // Compare accumulator
void CPX(Cpu *cpu, int addr_mode); // Compare X register
void CPY(Cpu *cpu, int addr_mode); // Compare Y register
void INC(Cpu *cpu, int addr_mode); // Increment value in memory
void INX(Cpu *cpu, int addr_mode); // Increment X register 
void INY(Cpu *cpu, int addr_mode); // Increment Y register 
void DEC(Cpu *cpu, int addr_mode); // Decrement value in memory
void DEX(Cpu *cpu, int addr_mode); // Decrement X register
void DEY(Cpu *cpu, int addr_mode); // Decrement Y register
void ASL(Cpu *cpu, int addr_mode); // Arithmetic shift left
void LSR(Cpu *cpu, int addr_mode); // Arithmetic shift right
void ROL(Cpu *cpu, int addr_mode); // Rotate left
void ROR(Cpu *cpu, int addr_mode); // Rotate right
void JMP(Cpu *cpu, int addr_mode); // Jump
void JSR(Cpu *cpu, int addr_mode); // Jump to subroutine
void RTS(Cpu *cpu, int addr_mode); // Return from subroutine
void BCC(Cpu *cpu, int addr_mode); // Branch if carry flag clear
void BCS(Cpu *cpu, int addr_mode); // Branch if carry flag set
void BNE(Cpu *cpu, int addr_mode); // Branch if zero flag clear
void BEQ(Cpu *cpu, int addr_mode); // Branch if zero flag set
void BPL(Cpu *cpu, int addr_mode); // Branch if negative flag clear
void BMI(Cpu *cpu, int addr_mode); // Branch if negative flag set
void BVC(Cpu *cpu, int addr_mode); // Branch if overflow flag clear
void BVS(Cpu *cpu, int addr_mode); // Branch if overflow flag set
void CLC(Cpu *cpu, int addr_mode); // Clear carry flag
void CLD(Cpu *cpu, int addr_mode); // Clear decimal flag
void CLI(Cpu *cpu, int addr_mode); // Clear interrupt disable
void CLV(Cpu *cpu, int addr_mode); // Clear overflow flag
void SEC(Cpu *cpu, int addr_mode); // Set carry flag
void SED(Cpu *cpu, int addr_mode); // Set deciaml flag
void SEI(Cpu *cpu, int addr_mode); // Set interrupt disable
void BRK(Cpu *cpu, int addr_mode); // Break
void NOP(Cpu *cpu, int addr_mode); // No operation
void RTI(Cpu *cpu, int addr_mode); // Return from interrupt

// For now we're replacing unofficial instructions
// with NOP
static void (*opcodes[256]) (Cpu *cpu, int addr_mode) = {
  // 0,    1,    2,    3,    4,    5,   6,     7,    8,    9,   A,     B,    C,    D,   E,     F
  &BRK, &ORA, &NOP, &NOP, &NOP, &ORA, &ASL, &NOP, &PHP, &ORA, &ASL, &NOP, &NOP, &ORA, &ASL, &NOP, // 0
  &BPL, &ORA, &NOP, &NOP, &NOP, &ORA, &ASL, &NOP, &CLC, &ORA, &NOP, &NOP, &NOP, &ORA, &ASL, &NOP, // 1
  &JSR, &AND, &NOP, &NOP, &BIT, &AND, &ROL, &NOP, &PLP, &AND, &ROL, &NOP, &BIT, &AND, &ROL, &NOP, // 2
  &BMI, &AND, &NOP, &NOP, &NOP, &AND, &ROL, &NOP, &SEC, &AND, &NOP, &NOP, &NOP, &AND, &ROL, &NOP, // 3
  &RTI, &EOR, &NOP, &NOP, &NOP, &EOR, &LSR, &NOP, &PHA, &EOR, &LSR, &NOP, &JMP, &EOR, &LSR, &NOP, // 4
  &BVC, &EOR, &NOP, &NOP, &NOP, &EOR, &LSR, &NOP, &CLI, &EOR, &NOP, &NOP, &NOP, &EOR, &LSR, &NOP, // 5
  &RTS, &ADC, &NOP, &NOP, &NOP, &ADC, &ROR, &NOP, &PLA, &ADC, &ROR, &NOP, &JMP, &ADC, &ROR, &NOP, // 6
  &BVS, &ADC, &NOP, &NOP, &NOP, &ADC, &ROR, &NOP, &SEI, &ADC, &NOP, &NOP, &NOP, &ADC, &ROR, &NOP, // 7
  &NOP, &STA, &NOP, &NOP, &STY, &STA, &STX, &NOP, &DEY, &NOP, &TXA, &NOP, &STY, &STA, &STX, &NOP, // 8
  &BCC, &STA, &NOP, &NOP, &STY, &STA, &STX, &NOP, &TYA, &STA, &TXS, &NOP, &NOP, &STA, &NOP, &NOP, // 9
  &LDY, &LDA, &LDX, &NOP, &LDY, &LDA, &LDX, &NOP, &TAY, &LDA, &TAX, &NOP, &LDY, &LDA, &LDX, &NOP, // A
  &BCS, &LDA, &NOP, &NOP, &LDY, &LDA, &LDX, &NOP, &CLV, &LDA, &TSX, &NOP, &LDY, &LDA, &LDX, &NOP, // B
  &CPY, &CMP, &NOP, &NOP, &CPY, &CMP, &DEC, &NOP, &INY, &CMP, &DEX, &NOP, &CPY, &CMP, &DEX, &NOP, // C
  &BNE, &CMP, &NOP, &NOP, &NOP, &CMP, &DEC, &NOP, &CLD, &CMP, &NOP, &NOP, &NOP, &CMP, &DEC, &NOP, // D
  &CPX, &SBC, &NOP, &NOP, &CPX, &SBC, &INC, &NOP, &INX, &SBC, &NOP, &NOP, &CPX, &SBC, &INC, &NOP, // E
  &BEQ, &SBC, &NOP, &NOP, &NOP, &SBC, &INC, &NOP, &SED, &SBC, &NOP, &NOP, &NOP, &SBC, &INC, &NOP  // F
};

static int addressing_modes[256] = {
//0   1  2   3  4  5  6  7  8  9  A  B   C  D  E  F
  0, 11, 0, 11, 3, 3, 3, 3, 0, 2, 1, 2,  7, 7, 7, 7, // 0
  6, 12, 0, 12, 4, 4, 4, 4, 0, 9, 0, 9,  8, 8, 8, 8, // 1
  7, 11, 0, 11, 3, 3, 3, 3, 0, 2, 1, 2,  7, 7, 7, 7, // 2
  6, 12, 0, 12, 4, 4, 4, 4, 0, 9, 0, 9,  8, 8, 8, 8, // 3
  0, 11, 0, 11, 3, 3, 3, 3, 0, 2, 1, 2,  7, 7, 7, 7, // 4
  6, 12, 0, 12, 4, 4, 4, 4, 0, 9, 0, 9,  8, 8, 8, 8, // 5
  0, 11, 0, 11, 3, 3, 3, 3, 0, 2, 1, 2, 10, 7, 7, 7, // 6
  6, 12, 0, 12, 4, 4, 4, 4, 0, 9, 0, 9,  8, 8, 8, 8, // 7
  2, 11, 2, 11, 3, 3, 3, 3, 0, 2, 0, 2,  8, 8, 8, 8, // 8
  6, 12, 0, 12, 4, 4, 5, 5, 0, 9, 0, 9,  8, 8, 9, 9, // 9
  2, 11, 2, 11, 3, 3, 3, 3, 0, 2, 0, 2,  7, 7, 7, 7, // A
  6, 12, 0, 12, 4, 4, 5, 5, 0, 9, 0, 9,  8, 8, 9, 9, // B
  2, 11, 2, 11, 3, 3, 3, 3, 0, 2, 0, 2,  7, 7, 7, 7, // C
  6, 12, 0, 12, 4, 4, 4, 4, 0, 9, 0, 9,  8, 8, 8, 8, // D
  2, 11, 2, 11, 3, 3, 3, 3, 0, 2, 0, 2,  7, 7, 7, 7, // E
  6, 12, 0, 12, 4, 4, 4, 4, 0, 9, 0, 9,  8, 8, 8, 8  // F
};

void init_cpu(Cpu *cpu, SharedMemory *mem);
void execute_cpu_instructions(Cpu *cpu);

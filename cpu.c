#include "cpu.h"

static uint8_t read_byte(Cpu* cpu, uint16_t addr)
{
	cpu->cycle_count ++;
	return read_cpu_memory(cpu->memspace, addr);
}

static void write_byte(Cpu* cpu, uint16_t addr, uint8_t byte)
{
	cpu->cycle_count ++;
	write_cpu_memory(cpu->memspace, addr, byte);
}

static uint8_t pop_stack(Cpu *cpu)
{
	// Accounting for the extra cycle used to preincrement the SP
	cpu->cycle_count ++;
	cpu->SP ++;
	
	uint8_t byte = read_byte(cpu, (0x0000 | cpu->SP));
	return byte;
}

static void push_stack(Cpu *cpu, uint8_t byte)
{
	write_byte(cpu, (0x0000 | cpu->SP), byte);
	cpu->SP --;
}

static void set_negative_and_zero(Cpu *cpu, uint8_t byte)
{
	cpu->Z = (byte == 0);
	cpu->N = (byte & 0x80) != 0;
}

static void add_with_carry(Cpu *cpu, uint8_t byte)
{
	uint8_t result = cpu->A + byte + cpu->C;

	cpu->C = (cpu->A + byte + cpu->C) > 0xFF;
	cpu->Z = (cpu->A == 0);
	cpu->N = (cpu->A & 0x80) != 0;
	cpu->V = ((cpu->A ^ result) & (byte ^ result)) & 0x80;

	cpu->A = result;
}

/* Writing status flags to a byte */
static uint8_t write_status_flag(Cpu *cpu)
{
	uint8_t byte;

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
static void read_status_flag(Cpu *cpu, uint8_t byte)
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
static uint16_t fetch_instruction_addr(Cpu *cpu, int addr_mode, bool is_read)
{
	uint16_t memory_addr = 0;

	switch (addr_mode)
	{
		case implied:
		case accumulator:
			memory_addr = read_byte(cpu, cpu->PC);
			break;
		
		case immediate:
		case relative:
			memory_addr = read_byte(cpu, cpu->PC++);
			break;

		case zero_page:
			memory_addr = read_byte(cpu, cpu->PC++);
			break;

		case zero_page_x:
		{
			uint8_t addr = read_byte(cpu, cpu->PC++);
			read_byte(cpu, (0x00FF | addr));
			addr += cpu->X;
			memory_addr = (0x00FF | addr);
			break;
		}

		case zero_page_y:
		{
			uint8_t addr = read_byte(cpu, cpu->PC++);
			read_byte(cpu, (0x00FF | addr));
			addr += cpu->Y;
			memory_addr = (0x00FF | addr);
			break;
		}

		case absolute:
		{
			uint8_t low = read_byte(cpu, cpu->PC++);
			uint8_t high = read_byte(cpu, cpu->PC++);
			memory_addr = (high << 8) | low;
			break;
		}

		case absolute_x:
		{
			uint8_t low = read_byte(cpu, cpu->PC++);
			uint8_t high = read_byte(cpu, cpu->PC++);
			uint8_t new_low = low + cpu->X;
			
			if (new_low < low)
			{
				high ++;
				if (is_read)
				read_byte(cpu, (high << 8) | new_low);
			}

			if (!is_read)
			{
				read_byte(cpu, (high << 8) | new_low);
			}

			memory_addr = (high << 8) | new_low;
			break;
		}

		case absolute_y:
		{
			uint8_t low = read_byte(cpu, cpu->PC++);
			uint8_t high = read_byte(cpu, cpu->PC++);
			uint8_t new_low = low + cpu->Y;
			
			if (new_low < low)
			{
				high ++;
				if (is_read)
				read_byte(cpu, (high << 8) | new_low);
			}

			if (!is_read)
				read_byte(cpu, (high << 8) | new_low);
			
			memory_addr = (high << 8) | new_low;
			break;
		}

		case indirect:
		{
			uint8_t pointer_low = read_byte(cpu, cpu->PC++);
			uint8_t pointer_high = read_byte(cpu, cpu->PC++);
			uint8_t addr_low = read_byte(cpu, (pointer_high << 8) | pointer_low);
			uint8_t addr_high = read_byte(cpu, ((pointer_high << 8) | pointer_low) + 1);
			memory_addr = (addr_high << 8) | addr_low;
			break;
		}

		case indirect_x:
		{
			uint8_t pointer_addr = read_byte(cpu, cpu->PC++);
			read_byte(cpu, (0x0000 | pointer_addr));
			pointer_addr += cpu->X;
			uint8_t low = read_byte(cpu, (0x0000 | pointer_addr));
			uint8_t high = read_byte(cpu, (0x0000 | pointer_addr) + 1);
			memory_addr = (high << 8) | low;
			break;
		}

		case indirect_y:
		{
			uint8_t pointer_addr = read_byte(cpu, cpu->PC++);
			uint8_t addr_low = read_byte(cpu, (0x0000 | pointer_addr));
			uint8_t addr_high = read_byte(cpu, (0x0000 | pointer_addr) + 1);
			uint8_t new_low = addr_low + cpu->Y;

			if (new_low < addr_low)
			{
				addr_high ++;
				if (is_read)
				read_byte(cpu, (addr_high << 8) | addr_low);
			}

			if (!is_read)
				read_byte(cpu, (addr_high << 8) | addr_low);

			memory_addr = (addr_high << 8) | new_low;
			break;
		}
	}

	return memory_addr;
}

void branch(Cpu *cpu, bool condition)
{
	uint8_t operand = (uint8_t) fetch_instruction_addr(cpu, relative, false);
	uint8_t PCL = (cpu->PC & 0x00FF);
	uint8_t PCH = (cpu->PC & 0xFF00) >> 8;
	uint8_t new_PCL = PCL + operand;
	
	if (condition)
	{
		read_byte(cpu, cpu->PC);
		if (new_PCL < PCL)
		{
			PCH += 1;
			cpu->PC = (PCH << 8) | new_PCL;
			read_byte(cpu, cpu->PC++);
		} else {
			cpu->PC = (PCH << 8) | new_PCL;
			cpu->PC += 1;
		}
		return;
	}

	cpu->PC += 1;
}

void BRK(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, immediate, false); // So that we read and increment PC
	cpu->B = 1;
	uint8_t PCH = (cpu->PC & 0xFF00) >> 8;
	uint8_t PCL = (cpu->PC & 0x00FF);
	push_stack(cpu, PCH);
	push_stack(cpu, PCL);
	push_stack(cpu, write_status_flag(cpu));
	PCL = read_byte(cpu, 0xFFFE);
	PCH = read_byte(cpu, 0xFFFF);
	cpu->PC = (PCH << 8) | PCL;
}

void RTI(Cpu *cpu, int addr_mode)
{
	cpu->cycle_count -= 1;
	read_status_flag(cpu, pop_stack(cpu));
	uint8_t PCL = pop_stack(cpu);
	uint8_t PCH = pop_stack(cpu);
	cpu->PC = (PCH << 8) | PCL;
}

void LDA(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, true);
	uint8_t byte = read_byte(cpu, addr);
	cpu->A = byte;
	set_negative_and_zero(cpu, cpu->A);
}

void LDX(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, true);
	uint8_t byte = read_byte(cpu, addr);
	cpu->X = byte;
	set_negative_and_zero(cpu, cpu->X);
}

void LDY(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, true);
	uint8_t byte = read_byte(cpu, addr);
	cpu->Y = byte;
	set_negative_and_zero(cpu, cpu->Y);
}

void STA(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, false);
	write_byte(cpu, addr, cpu->A);
}

void STX(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, false);
	write_byte(cpu, addr, cpu->X);
}

void STY(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, false);
	write_byte(cpu, addr, cpu->Y);
}

void TAX(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->X = cpu->A;
	set_negative_and_zero(cpu, cpu->X);
}

void TAY(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->Y = cpu->A;
	set_negative_and_zero(cpu, cpu->Y);
}

void TSX(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->X = cpu->SP;
	set_negative_and_zero(cpu, cpu->X);
}

void TXA(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->A = cpu->X;
	set_negative_and_zero(cpu, cpu->A);
}

void TXS(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->SP = cpu->X;
}

void TYA(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->A = cpu->Y;
	set_negative_and_zero(cpu, cpu->A);
}

void PHA(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	push_stack(cpu, cpu->A);
}

void PHP(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	push_stack(cpu, write_status_flag(cpu));
}

void PLA(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->A = pop_stack(cpu);
	set_negative_and_zero(cpu, cpu->A);
}

void PLP(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	read_status_flag(cpu, pop_stack(cpu));
}

void AND(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, true);
	uint8_t byte = read_byte(cpu, addr);
	cpu->A &= byte;
	set_negative_and_zero(cpu, cpu->A);
}

void EOR(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, true);
	uint8_t byte = read_byte(cpu, addr);
	cpu->A ^= byte;
	set_negative_and_zero(cpu, cpu->A);
}

void ORA(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, true);
	uint8_t byte = read_byte(cpu, addr);
	cpu->A |= byte;
	set_negative_and_zero(cpu, cpu->A);
}

void BIT(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, true);
	uint8_t byte = read_byte(cpu, addr);
	uint8_t temp_and = cpu->A & byte;

	cpu->Z = (temp_and == 0);
	cpu->V = (byte & 0x40) != 0;
	cpu->N = (byte & 0x80) != 0;
}

void ADC(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, true);
	uint8_t byte = read_byte(cpu, addr);
	add_with_carry(cpu, byte);
}

void SBC(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, true);
	uint8_t byte = read_byte(cpu, addr);
	add_with_carry(cpu, (255 - byte));
}

void CMP(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, true);
	uint8_t byte = read_byte(cpu, addr);
	uint8_t result = cpu->A - byte;
	cpu->C = (cpu->A >= byte);
	cpu->Z = (cpu->A == byte);
	cpu->N = (result & 0x80) != 0;
}

void CPX(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, true);
	uint8_t byte = read_byte(cpu, addr);
	uint8_t result = cpu->X - byte;
	cpu->C = (cpu->X >= byte);
	cpu->Z = (cpu->X == byte);
	cpu->N = (result & 0x80) != 0;
}

void CPY(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, true);
	uint8_t byte = read_byte(cpu, addr);
	uint8_t result = cpu->Y - byte;
	cpu->C = (cpu->Y >= byte);
	cpu->Z = (cpu->Y == byte);
	cpu->N = (result & 0x80) != 0;
}

void INC(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, false);
	uint8_t byte = read_byte(cpu, addr);
	write_byte(cpu, addr, byte);
	byte += 1;
	
	write_byte(cpu, addr, byte);
	set_negative_and_zero(cpu, byte);
}

void DEC(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, false);
	uint8_t byte = read_byte(cpu, addr);
	write_byte(cpu, addr, byte);
	byte -= 1;

	write_byte(cpu, addr, byte);
	set_negative_and_zero(cpu, byte);
}

void INX(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->X += 1;
	set_negative_and_zero(cpu, cpu->X);
}

void INY(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->Y += 1;
	set_negative_and_zero(cpu, cpu->Y);
}

void DEX(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->X -= 1;
	set_negative_and_zero(cpu, cpu->X);
}

void DEY(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->Y -= 1;
	set_negative_and_zero(cpu, cpu->Y);
}

void ASL(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, false);

	if (addr_mode == accumulator)
	{
		cpu->C = (cpu->A & 0x80) != 0;
		cpu->A = cpu->A << 1;
		set_negative_and_zero(cpu, cpu->A);
		return;
	}

	uint8_t byte = read_byte(cpu, addr);
	write_byte(cpu, addr, byte);
	cpu->C = (byte & 0x80) != 0;

	byte = byte << 1;
	write_byte(cpu, addr, byte);
	set_negative_and_zero(cpu, byte);
}

void LSR(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, false);

	if (addr_mode == accumulator)
	{
		cpu->C = (cpu->A & 0x01) != 0;
		cpu->A = cpu->A >> 1;
		set_negative_and_zero(cpu, cpu->A);
		return;
	}

	uint8_t byte = read_byte(cpu, addr);
	write_byte(cpu, addr, byte);
	cpu->C = (byte & 0x01) != 0;

	byte = byte >> 1;
	write_byte(cpu, addr, byte);
	set_negative_and_zero(cpu, byte);
}

void ROL(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, false);

	if (addr_mode == accumulator)
	{
		uint8_t old_bit_7 = (cpu->A & 0x80) != 0;
		cpu->A = cpu->A << 1;
		cpu->A |= cpu->C;
		cpu->C = old_bit_7;
		set_negative_and_zero(cpu, cpu->A);
		return;
	}

	uint8_t byte = read_byte(cpu, addr);
	write_byte(cpu, addr, byte);
	
	uint8_t old_bit_7 = (byte & 0x80) != 0;
	byte = byte << 1;
	byte |= cpu->C;
	cpu->C = old_bit_7;

	write_byte(cpu, addr, byte);
	set_negative_and_zero(cpu, byte);
}	

void ROR(Cpu *cpu, int addr_mode)
{
	uint16_t addr = fetch_instruction_addr(cpu, addr_mode, false);

	if (addr_mode == accumulator)
	{
		uint8_t old_bit_0 = (cpu->A & 0x01) != 0;
		cpu->A = cpu->A >> 1;
		cpu->A |= (cpu->C == 1) << 7;
		cpu->C = old_bit_0;
		set_negative_and_zero(cpu, cpu->A);
		return;
	}

	uint8_t byte = read_byte(cpu, addr);
	write_byte(cpu, addr, byte);
	
	uint8_t old_bit_0 = (byte & 0x01) != 0;
	byte = byte >> 1;
	byte |= (cpu->C == 1) << 7;
	cpu->C = old_bit_0;

	write_byte(cpu, addr, byte);
	set_negative_and_zero(cpu, byte);
}	

void JMP(Cpu *cpu, int addr_mode)
{
	uint16_t jmp_addr = fetch_instruction_addr(cpu, addr_mode, false);
	cpu->PC = jmp_addr;
}

void JSR(Cpu *cpu, int addr_mode)
{
	cpu->cycle_count ++;
	uint16_t jmp_addr = fetch_instruction_addr(cpu, addr_mode, false);
	push_stack(cpu, (cpu->PC & 0xFF00) >> 8);
	push_stack(cpu, (uint8_t) (cpu->PC & 0x00FF) - 1);
	cpu->PC = jmp_addr;
}

void RTS(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	uint8_t pc_low = pop_stack(cpu);
	uint8_t pc_high = pop_stack(cpu);
	cpu->PC = (pc_high << 8) | pc_low;
	cpu->PC ++;
}

void BCC(Cpu *cpu, int addr_mode)
{
	branch(cpu, (cpu->C == 0));
}

void BCS(Cpu *cpu, int addr_mode)
{
	branch(cpu, (cpu->C == 1));
}

void BNE(Cpu *cpu, int addr_mode)
{
	branch(cpu, (cpu->Z == 0));
}

void BEQ(Cpu *cpu, int addr_mode)
{
	branch(cpu, (cpu->Z == 1));
}

void BPL(Cpu *cpu, int addr_mode)
{
	branch(cpu, (cpu->N == 0));
}

void BMI(Cpu *cpu, int addr_mode)
{
	branch(cpu, (cpu->N == 1));
}

void BVC(Cpu *cpu, int addr_mode)
{
	branch(cpu, (cpu->V == 0));
}

void BVS(Cpu *cpu, int addr_mode)
{
	branch(cpu, (cpu->V == 1));
}

void CLC(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->C = 0;
}

void CLI(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->I = 0;
}

void CLD(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->D = 0;
}

void CLV(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->V = 0;
}

void SEC(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->C = 1;
}

void SEI(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->I = 0;
}

void SED(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
	cpu->D = 1;
}

void NOP(Cpu *cpu, int addr_mode)
{
	fetch_instruction_addr(cpu, addr_mode, false);
}

void init_cpu(Cpu *cpu, SharedMemory *mem, bool debug_state)
{
	cpu->cycle_count = 0;
	cpu->memspace = mem;

	cpu->should_log = debug_state;
	if (cpu->should_log)
		init_logger(&cpu->logger, "debug.log");
	
	cpu->PC = 0xFFFC;
	cpu->SP = 0xFD;
	cpu->X = 0;
	cpu->Y = 0;
	cpu->A = 0;
	read_status_flag(cpu, 0x34);
}

void cleanup_cpu(Cpu *cpu)
{
	if (cpu->should_log)
		cleanup_logger(&cpu->logger);
}

static void (*instruction)(Cpu *cpu, int addr_mode);
void execute_cpu_instructions(Cpu *cpu)
{
	while (cpu->cycle_count < CPU_CYCLES_PER_FRAME)
	{
		uint8_t opcode = read_byte(cpu, cpu->PC++);
		int addr_mode = addressing_modes[opcode];
		
		instruction = opcodes[opcode];
		(*instruction)(cpu, addr_mode);
	}
}
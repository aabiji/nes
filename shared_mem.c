#include "shared_mem.h"

uint8_t read_cpu_memory(SharedMemory *mem, uint16_t addr)
{
	return mem->cpu_memory[addr];
}

void write_cpu_memory(SharedMemory *mem, uint16_t addr, uint8_t byte)
{
	mem->cpu_memory[addr] = byte;
}

// load_chr_rom
void load_pgr_banks(SharedMemory *mem, Rom *rom)
{
	int pgr_offset = 0;

	// If more than two 16 kb banks, use mapper
	if (rom->pgr_rom_size <= (16384 * 2))
	{
		for (int i = 0; i < 16384; i++)
			mem->cpu_memory[0x8000 + i] = rom->pgr_rom[pgr_offset + i];

		pgr_offset = (rom->pgr_rom_size == 16384) ? 0 : 16384;
		
		for (int i = 0; i < 16384; i++)
			mem->cpu_memory[0xC000 + i] = rom->pgr_rom[pgr_offset + i];
	}
}
 
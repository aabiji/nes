#include "cpu.h"
#include "rom.h"

int main()
{
	/*
	Cpu cpu;
	SharedMemory mem;

	init_cpu(&cpu, &mem, false);
	execute_cpu_instructions(&cpu);
	cleanup_cpu(&cpu);
	*/

	Rom rom = load_rom("nestest.nes");
}

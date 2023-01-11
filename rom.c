#include "rom.h"

// TODO: free rom raw eventually

static void rom_test(Rom *rom)
{
	if (rom->has_trainer)
	{
		printf("TRAINER (512 bytes)\n");
		for (int i = 0; i < 512; i++)
		{
			printf("%02X  ", rom->trainer[i]);
			if (((i + 1) % 15) == 0) printf("\n");
		}
	}

	printf("\nPGR ROM (%d bytes)\n", rom->pgr_rom_size);
	for (int i = 0; i < rom->pgr_rom_size; i++)
	{
		printf("%02X  ", rom->pgr_rom[i]);
		if (((i + 1) % 15) == 0) printf("\n");
	}

	printf("\nCHR ROM {%d bytes)\n", rom->chr_rom_size);
	for (int i = 0; i < rom->chr_rom_size; i++)
	{
		printf("%02X  ", rom->chr_rom[i]);
		if (((i + 1) % 15) == 0) printf("\n");
	}
	printf("\n");
}

static Rom read_file_contents(char* filename)
{
	Rom rom;
	FILE* fp = fopen(filename, "rb");

	fseek(fp, 0L, SEEK_END);
	int file_size = ftell(fp);
	rewind(fp);

	rom.raw = malloc(file_size * sizeof(uint8_t));
	rom.raw_size = file_size;
	
	fread(rom.raw, file_size, 1, fp);

	free(fp);
	return rom;
}

Rom parse_rom_flags(char* filename)
{
	Rom rom = read_file_contents(filename);

	rom.pgr_rom_size = rom.raw[4] * 16384;
	rom.chr_rom_size = rom.raw[5] * 8192;

	rom.mapper = (rom.raw[7] & 0xF0) | ((rom.raw[6] & 0xF0) >> 4);
	rom.is_vertical_mirroring = (rom.raw[6] & 0x01) != 0;

	rom.has_pgr_ram = (rom.raw[6] & 0x02) != 0;
	rom.pgr_ram_size = (rom.raw[8]);
	
	rom.has_trainer = (rom.raw[6] & 0x03) != 0;
	if (rom.has_trainer) rom.trainer = malloc(512 * sizeof(uint8_t));

	rom.has_vram = (rom.raw[6] & 0x04) != 0;

	if (((rom.raw[7] & 0b00001100) >> 2) == 2)
		printf("Nes 2.0 format not supported.\n");
	
	return rom;
}

Rom load_rom(char *filename)
{
	Rom rom = parse_rom_flags(filename);

	rom.pgr_rom = malloc(rom.pgr_rom_size * sizeof(uint8_t));
	rom.chr_rom = malloc(rom.chr_rom_size * sizeof(uint8_t));

	int read_offset = (rom.has_trainer ? 512 : 0) + 16;
	for (int i = 0; i < rom.pgr_rom_size; i++)
		rom.pgr_rom[i] = rom.raw[read_offset + i];

	read_offset += rom.pgr_rom_size;
	for (int i = 0; i < rom.chr_rom_size; i++)
		rom.chr_rom[i] = rom.raw[read_offset + i];

	return rom;
}
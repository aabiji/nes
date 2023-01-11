/* Parsing the INES file format */
/* Ines 2.0 is not supported. */
#ifndef ROM_H_
#define ROM_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct parser {
	uint8_t* raw;
	uint8_t* pgr_rom;
	uint8_t* chr_rom;
	uint8_t* trainer;

	int raw_size;
	int chr_rom_size;
	int pgr_rom_size;
	int pgr_ram_size;

	bool has_pgr_ram;
	bool has_trainer;
	bool has_vram; /* 4 screen vram */
	bool is_vertical_mirroring;

	int mapper;
} Rom;

Rom parse_rom_flags(char *filename);
Rom load_rom(char *filename);

#endif

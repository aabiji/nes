#include <stdlib.h>
#include <stdio.h>

typedef struct log
{
	FILE* file_stream;
	char* outfile;
} Logger;

void init_logger(Logger *logger, char* outfile);
void write_log(Logger *logger, char* buffer);
void cleanup_logger(Logger *logger);

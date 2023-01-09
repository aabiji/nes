#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct
{
  FILE* file_stream;
  char* outfile;
} Logger;

void init_logger(Logger *logger, char* outfile);
void write_log(Logger *logger, char* buffer);
void cleanup_logger(Logger *logger);

#include "log.h"

void init_logger(Logger *logger, char* outfile)
{
  logger->file_stream = fopen(outfile, "a");
  if (logger->file_stream == NULL)
  {
    printf("File stream was unable to be created: %s\n", outfile);
	exit(0);
  }
}

void write_log(Logger *logger, char* buffer)
{
  fprintf(logger->file_stream, buffer, 0);
}

void cleanup_logger(Logger *logger)
{
  fclose(logger->file_stream);
}

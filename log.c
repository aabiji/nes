#include "log.h"

void init_logger(Logger *logger, char* outfile)
{
  logger->file_stream = fopen(outfile, "a");
  assert(logger->file_stream != NULL);
}

void write_log(Logger *logger, char* buffer)
{
  fprintf(logger->file_stream, buffer, 0);
}

void cleanup_logger(Logger *logger)
{
  fclose(logger->file_stream);
}

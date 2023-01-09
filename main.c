#include "cpu.h"
#include "log.h"

int main()
{
  //Cpu cpu;
  //SharedMemory mem;

  //init_cpu(&cpu, &mem);
  //execute_cpu_instructions(&cpu);

  Logger log;
  init_logger(&log, "test.txt");
  write_log(&log, "Hello world\n");
  cleanup_logger(&log);
}

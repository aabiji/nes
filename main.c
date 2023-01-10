#include "cpu.h"

int main()
{
  Cpu cpu;
  SharedMemory mem;

  init_cpu(&cpu, &mem, true);
  execute_cpu_instructions(&cpu);
  cleanup_cpu(&cpu);
}

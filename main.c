#include "cpu.h"

int main()
{
  Cpu cpu;
  SharedMemory mem;

  init_cpu(&cpu, &mem);
  execute_cpu_instructions(&cpu);
}

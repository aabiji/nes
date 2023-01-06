#include "cpu.h"

int main()
{
  Cpu cpu;
  SharedMemory mem;

  init_cpu(&cpu, &mem);
  execute_cycles(&cpu);
}

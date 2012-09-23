#include <stdio.h>
#include <stdint.h>

static inline void cpuid(int code, unsigned int *a, unsigned int *b, unsigned int *d) {
  asm volatile("cpuid":"=a"(*a),"=b"(*b),"=d"(*d):"a"(code):"ecx");
}
int main(int argc, char **argv) {
  unsigned int a = 0, b = 0, d = 0;
  cpuid(1,&a,&b,&d);
  printf("0x%010x | 0x%010x | 0x%010x \n", a,b,d);
  return 0;
}


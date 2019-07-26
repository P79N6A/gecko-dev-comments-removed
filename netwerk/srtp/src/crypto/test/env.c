











































#include <stdio.h>
#include <string.h>     
#include "config.h"

int 
main(void) {
  int err_count = 0;
  char *str;

#ifdef WORDS_BIGENDIAN
  printf("CPU set to big-endian\t\t\t(WORDS_BIGENDIAN == 1)\n");
#else
  printf("CPU set to little-endian\t\t(WORDS_BIGENDIAN == 0)\n");
#endif

#ifdef CPU_RISC
  printf("CPU set to RISC\t\t\t\t(CPU_RISC == 1)\n");
#elif defined(CPU_CISC)
  printf("CPU set to CISC\t\t\t\t(CPU_CISC == 1)\n");
#else
  printf("CPU set to an unknown type, probably due to a configuration error\n");
  err_count++;
#endif

#ifdef CPU_ALTIVEC
  printf("CPU set to ALTIVEC\t\t\t\t(CPU_ALTIVEC == 0)\n");
#endif

#ifndef NO_64BIT_MATH
  printf("using native 64-bit type\t\t(NO_64_BIT_MATH == 0)\n");
#else
  printf("using built-in 64-bit math\t\t(NO_64_BIT_MATH == 1)\n");
#endif

#ifdef ERR_REPORTING_STDOUT
  printf("using stdout for error reporting\t(ERR_REPORTING_STDOUT == 1)\n");
#endif

#ifdef DEV_URANDOM
  str = DEV_URANDOM;
#else
  str = "";
#endif
  printf("using %s as a random source\t(DEV_URANDOM == %s)\n",
	 str, str);
  if (strcmp("", str) == 0) {
    err_count++;
  }
  
  if (err_count)
    printf("warning: configuration is probably in error "
	   "(found %d problems)\n", err_count);

  return err_count;
}

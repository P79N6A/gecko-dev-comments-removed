





#include <stdio.h>

extern "C" {

  
  
  void MozillaRegisterDebugFD(int fd) {}
  void MozillaRegisterDebugFILE(FILE *f) {}
  void MozillaUnRegisterDebugFD(int fd) {}
  void MozillaUnRegisterDebugFILE(FILE *f) {}

}
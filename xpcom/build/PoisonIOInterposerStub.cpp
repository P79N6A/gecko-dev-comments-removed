





#include <stdio.h>

extern "C" {

void MozillaRegisterDebugFD(int aFd) {}
void MozillaRegisterDebugFILE(FILE* aFile) {}
void MozillaUnRegisterDebugFD(int aFd) {}
void MozillaUnRegisterDebugFILE(FILE* aFile) {}

}  

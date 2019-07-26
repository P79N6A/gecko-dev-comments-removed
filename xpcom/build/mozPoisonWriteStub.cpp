





#include <stdio.h>
namespace mozilla {
void PoisonWrite() {
}
void DisableWritePoisoning() {
}
void EnableWritePoisoning() {
}
void InitWritePoisoning() {
}
}
extern "C" {
    void MozillaRegisterDebugFD(int fd) {
    }
    void MozillaRegisterDebugFILE(FILE *f) {
    }
    void MozillaUnRegisterDebugFD(int fd) {
    }
    void MozillaUnRegisterDebugFILE(FILE *f) {
    }
}







#include <stdio.h>
namespace mozilla {
void PoisonWrite() {
}
void DisableWritePoisoning() {
}
void EnableWritePoisoning() {
}
}
extern "C" {
    void MozillaRegisterDebugFD(int fd) {
    }
    void MozillaUnRegisterDebugFD(int fd) {
    }
    void MozillaUnRegisterDebugFILE(FILE *f) {
    }
}

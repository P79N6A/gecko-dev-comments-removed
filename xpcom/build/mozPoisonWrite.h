





#ifndef MOZPOISONWRITE_H
#define MOZPOISONWRITE_H

#include "mozilla/Types.h"
#include <stdio.h>

MOZ_BEGIN_EXTERN_C
  void MozillaRegisterDebugFD(int fd);
  void MozillaRegisterDebugFILE(FILE *f);
  void MozillaUnRegisterDebugFD(int fd);
  void MozillaUnRegisterDebugFILE(FILE *f);
MOZ_END_EXTERN_C

#ifdef __cplusplus
namespace mozilla {
enum ShutdownChecksMode {
  SCM_CRASH,
  SCM_RECORD,
  SCM_NOTHING
};
extern ShutdownChecksMode gShutdownChecks;

void PoisonWrite();
void DisableWritePoisoning();
void EnableWritePoisoning();
}
#endif

#endif

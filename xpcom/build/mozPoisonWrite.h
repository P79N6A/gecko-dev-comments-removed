





#include "mozilla/Types.h"

MOZ_BEGIN_EXTERN_C
  void MozillaRegisterDebugFD(int fd);
  void MozillaUnRegisterDebugFD(int fd);
MOZ_END_EXTERN_C

#ifdef __cplusplus
namespace mozilla {
void PoisonWrite();
void DisableWritePoisoning();
}
#endif

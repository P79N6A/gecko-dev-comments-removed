#include "nsError.h"
#include "mozilla/Assertions.h"



MOZ_STATIC_ASSERT(sizeof(nsresult) == sizeof(uint32_t),
                  "nsresult must be 32 bits");

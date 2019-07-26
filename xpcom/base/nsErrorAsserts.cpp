#include "mozilla/Assertions.h"

#include "nsError.h"



static_assert(((nsresult)0) < ((nsresult)-1),
              "nsresult must be an unsigned type");
static_assert(sizeof(nsresult) == sizeof(uint32_t),
              "nsresult must be 32 bits");








#ifndef mozilla_IntegerPrintfMacros_h_
#define mozilla_IntegerPrintfMacros_h_






















#if defined(MOZ_CUSTOM_INTTYPES_H)
#  include MOZ_CUSTOM_INTTYPES_H
#elif defined(_MSC_VER)
#  include "mozilla/MSIntTypes.h"
#else
#  include <inttypes.h>
#endif

#endif  

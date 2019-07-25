








#ifndef mozilla_StandardInteger_h_
#define mozilla_StandardInteger_h_

























#if defined(MOZ_CUSTOM_STDINT_H)
#  include MOZ_CUSTOM_STDINT_H
#elif defined(_MSC_VER) && _MSC_VER < 1600
#  include "mozilla/MSStdInt.h"
#else
#  include <stdint.h>
#endif

#endif  

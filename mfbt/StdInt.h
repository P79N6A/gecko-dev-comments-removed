









































#ifndef mozilla_StdInt_h_
#define mozilla_StdInt_h_




















#if defined(MOZ_CUSTOM_STDINT_H)
#  include MOZ_CUSTOM_STDINT_H
#elif defined(_MSC_VER) && _MSC_VER < 1600
#  include "mozilla/MSStdInt.h"
#else
   



#  include <sys/../stdint.h>
#endif

#endif  

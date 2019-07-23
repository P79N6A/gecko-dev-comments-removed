







































#ifndef mozilla_unused_h
#define mozilla_unused_h

#include "nscore.h"

namespace mozilla {





struct NS_COM_GLUE unused_t { };

extern const unused_t unused NS_COM_GLUE;

template<typename T>
inline void operator<<(const unused_t& , const T& ) { }

}

#endif 

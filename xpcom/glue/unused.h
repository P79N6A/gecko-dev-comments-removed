





#ifndef mozilla_unused_h
#define mozilla_unused_h

#include "nscore.h"

namespace mozilla {





struct unused_t
{
};

extern const unused_t unused;

template<typename T>
inline void
operator<<(const unused_t& , const T& )
{
}

}

#endif 









#ifndef m_cpp_utils_h__
#define m_cpp_utils_h__

#include "mozilla/Attributes.h"

namespace mozilla {

#define DISALLOW_ASSIGNMENT(T) \
  void operator=(const T& other) MOZ_DELETE

#define DISALLOW_COPY(T) \
  T(const T& other) MOZ_DELETE


#define DISALLOW_COPY_ASSIGN(T) \
  DISALLOW_COPY(T); \
  DISALLOW_ASSIGNMENT(T)

}  
#endif

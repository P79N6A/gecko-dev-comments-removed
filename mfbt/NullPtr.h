







#ifndef mozilla_NullPtr_h
#define mozilla_NullPtr_h

#include "mozilla/TypeTraits.h"

namespace mozilla {








template<typename T>
struct IsNullPointer : FalseType {};

template<>
struct IsNullPointer<decltype(nullptr)> : TrueType {};

} 

#endif 

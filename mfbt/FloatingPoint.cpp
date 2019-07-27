







#include "mozilla/FloatingPoint.h"

namespace mozilla {

bool
IsFloat32Representable(double aFloat32)
{
  float asFloat = static_cast<float>(aFloat32);
  double floatAsDouble = static_cast<double>(asFloat);
  return floatAsDouble == aFloat32;
}

} 

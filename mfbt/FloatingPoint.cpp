






#include "mozilla/FloatingPoint.h"

namespace mozilla {

bool
IsFloat32Representable(double x)
{
    float asFloat = static_cast<float>(x);
    double floatAsDouble = static_cast<double>(asFloat);
    return floatAsDouble == x;
}

} 

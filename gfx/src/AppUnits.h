




#ifndef mozilla_AppUnits_h
#define mozilla_AppUnits_h

#include <stdint.h>

namespace mozilla {
inline int32_t AppUnitsPerCSSPixel() { return 60; }
inline int32_t AppUnitsPerCSSInch() { return 96 * AppUnitsPerCSSPixel(); }
}
#endif 

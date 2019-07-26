




#ifndef mozilla_AppUnits_h
#define mozilla_AppUnits_h

#include <stdint.h>

namespace mozilla {
static int32_t AppUnitsPerCSSPixel() { return 60; }
static int32_t AppUnitsPerCSSInch() { return 96 * AppUnitsPerCSSPixel(); }
}
#endif 

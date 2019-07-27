







#if defined(_WIN64)

#ifdef _MSC_VER
#if _MSC_VER >= 1700
#include "vpx_scale_rtcd_x86_64-win64-vs11.h"
#else
#include "vpx_scale_rtcd_x86_64-win64-vs8.h"
#endif
#else
#include "vpx_scale_rtcd_x86_64-win64-gcc.h"
#endif

#elif defined(_WIN32)

#ifdef _MSC_VER
#if _MSC_VER >= 1700
#include "vpx_scale_rtcd_x86-win32-vs11.h"
#else
#include "vpx_scale_rtcd_x86-win32-vs8.h"
#endif
#else
#include "vpx_scale_rtcd_x86-win32-gcc.h"
#endif

#elif defined(__APPLE__) && defined(__x86_64__)

#include "vpx_scale_rtcd_x86_64-darwin9-gcc.h"

#elif defined(__APPLE__) && defined(__i386__)

#include "vpx_scale_rtcd_x86-darwin9-gcc.h"

#elif defined(__ELF__) && (defined(__i386) || defined(__i386__))

#include "vpx_scale_rtcd_x86-linux-gcc.h"

#elif defined(__ELF__) && (defined(__x86_64) || defined(__x86_64__))

#include "vpx_scale_rtcd_x86_64-linux-gcc.h"

#elif defined(VPX_ARM_ASM)

#include "vpx_scale_rtcd_armv7-android-gcc.h"

#else

#include "vpx_scale_rtcd_generic-gnu.h"
#endif

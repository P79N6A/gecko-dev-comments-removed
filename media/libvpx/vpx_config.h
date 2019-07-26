#if defined(VPX_X86_ASM)

#if defined(_WIN64)

#ifdef _MSC_VER
#include "vpx_config_x86_64-win64-vs8.h"
#else
#include "vpx_config_x86_64-win64-gcc.h"
#endif

#elif defined(_WIN32)

#ifdef _MSC_VER
#include "vpx_config_x86-win32-vs8.h"
#else
#include "vpx_config_x86-win32-gcc.h"
#endif

#elif defined(__APPLE__) && defined(__x86_64__)

#include "vpx_config_x86_64-darwin9-gcc.h"

#elif defined(__APPLE__) && defined(__i386__)

#include "vpx_config_x86-darwin9-gcc.h"

#elif defined(__ELF__) && (defined(__i386) || defined(__i386__))

#include "vpx_config_x86-linux-gcc.h"

#elif defined(__ELF__) && (defined(__x86_64) || defined(__x86_64__))

#include "vpx_config_x86_64-linux-gcc.h"

#else
#error VPX_X86_ASM is defined, but assembly not supported on this platform!
#endif

#elif defined(VPX_ARM_ASM)


#include "vpx_config_armv7-android-gcc.h"

#else

#include "vpx_config_generic-gnu.h"
#endif



#if defined(MOZ_VPX_ERROR_CONCEALMENT)
#undef CONFIG_ERROR_CONCEALMENT
#define CONFIG_ERROR_CONCEALMENT 1
#endif

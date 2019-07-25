#if defined(VPX_X86_ASM)

#if defined(WIN32) && !defined(__GNUC__) && !defined(_M_X64)

#include "vpx_config_x86-win32-vs8.c"

#elif defined(__APPLE__) && defined(__x86_64__)

#include "vpx_config_x86_64-darwin9-gcc.c"

#elif defined(__APPLE__) && defined(__i386__)

#include "vpx_config_x86-darwin9-gcc.c"

#elif defined(__linux__) && defined(__i386__)

#include "vpx_config_x86-linux-gcc.c"

#elif defined(__linux__) && defined(__x86_64__)

#include "vpx_config_x86_64-linux-gcc.c"

#elif defined(__sun) && defined(__i386)

#include "vpx_config_x86-linux-gcc.h"

#elif defined(__sun) && defined(__x86_64)

#include "vpx_config_x86_64-linux-gcc.h"

#elif defined(_MSC_VER) && defined(_M_X64)
#include "vpx_config_x86_64-win64-vs8.c"

#else
#error VPX_X86_ASM is defined, but assembly not supported on this platform!
#endif

#elif defined(VPX_ARM_ASM)

#if defined(__linux__) && defined(__GNUC__)
#include "vpx_config_arm-linux-gcc.c"

#else
#error VPX_ARM_ASM is defined, but assembly not supported on this platform!
#endif

#else

#include "vpx_config_generic-gnu.c"
#endif


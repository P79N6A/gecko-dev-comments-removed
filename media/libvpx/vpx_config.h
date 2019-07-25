#if defined(VPX_X86_ASM)

#if defined(_WIN32) && !defined(__GNUC__) && defined(_M_IX86)

#include "vpx_config_x86-win32-vs8.h"

#elif defined(__APPLE__) && defined(__x86_64__)

#include "vpx_config_x86_64-darwin9-gcc.h"

#elif defined(__APPLE__) && defined(__i386__)

#include "vpx_config_x86-darwin9-gcc.h"

#elif defined(__linux__) && defined(__i386__)

#include "vpx_config_x86-linux-gcc.h"

#elif defined(__linux__) && defined(__x86_64__)

#include "vpx_config_x86_64-linux-gcc.h"

#elif defined(__sun) && defined(__i386)

#include "vpx_config_x86-linux-gcc.h"

#elif defined(__sun) && defined(__x86_64)

#include "vpx_config_x86_64-linux-gcc.h"

#elif defined(_MSC_VER) && defined(_M_X64)

#include "vpx_config_x86_64-win64-vs8.h"

#else
#error VPX_X86_ASM is defined, but assembly not supported on this platform!
#endif

#elif defined(VPX_ARM_ASM)

#if defined(__linux__) && defined(__GNUC__)

#include "vpx_config_arm-linux-gcc.h"

#else
#error VPX_ARM_ASM is defined, but assembly not supported on this platform!
#endif

#else

#include "vpx_config_generic-gnu.h"
#endif



#if defined(MOZ_VP8_ERROR_CONCEALMENT)
#undef CONFIG_ERROR_CONCEALMENT
#define CONFIG_ERROR_CONCEALMENT 1
#endif


#if defined(MOZ_VP8_ENCODER)
#undef CONFIG_VP8_ENCODER
#undef CONFIG_ENCODERS
#define CONFIG_VP8_ENCODER 1
#define CONFIG_ENCODERS 1
#endif

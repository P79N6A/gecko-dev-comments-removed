







#ifndef SkUtilsArm_DEFINED
#define SkUtilsArm_DEFINED

#include "SkUtils.h"







#define SK_ARM_NEON_MODE_NONE     0
#define SK_ARM_NEON_MODE_ALWAYS   1
#define SK_ARM_NEON_MODE_DYNAMIC  2

#if defined(SK_CPU_ARM32) && defined(__ARM_HAVE_OPTIONAL_NEON_SUPPORT)
#  define SK_ARM_NEON_MODE  SK_ARM_NEON_MODE_DYNAMIC
#elif defined(SK_CPU_ARM32) && defined(__ARM_HAVE_NEON) || defined(SK_CPU_ARM64)
#  define SK_ARM_NEON_MODE  SK_ARM_NEON_MODE_ALWAYS
#else
#  define SK_ARM_NEON_MODE  SK_ARM_NEON_MODE_NONE
#endif


#define SK_ARM_NEON_IS_NONE    (SK_ARM_NEON_MODE == SK_ARM_NEON_MODE_NONE)
#define SK_ARM_NEON_IS_ALWAYS  (SK_ARM_NEON_MODE == SK_ARM_NEON_MODE_ALWAYS)
#define SK_ARM_NEON_IS_DYNAMIC (SK_ARM_NEON_MODE == SK_ARM_NEON_MODE_DYNAMIC)





#if SK_ARM_NEON_IS_NONE
static inline bool sk_cpu_arm_has_neon(void) {
    return false;
}
#elif SK_ARM_NEON_IS_ALWAYS
static inline bool sk_cpu_arm_has_neon(void) {
    return true;
}
#else 

extern bool sk_cpu_arm_has_neon(void) SK_PURE_FUNC;
#endif


























#if SK_ARM_NEON_IS_NONE
#  define SK_ARM_NEON_WRAP(x)   (x)
#elif SK_ARM_NEON_IS_ALWAYS
#  define SK_ARM_NEON_WRAP(x)   (x ## _neon)
#elif SK_ARM_NEON_IS_DYNAMIC
#  define SK_ARM_NEON_WRAP(x)   (sk_cpu_arm_has_neon() ? x ## _neon : x)
#endif

#endif 

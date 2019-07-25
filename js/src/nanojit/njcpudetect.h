






































#ifndef __njcpudetect__
#define __njcpudetect__






















#if defined(__ARM_ARCH__)

    #define NJ_COMPILER_ARM_ARCH __ARM_ARCH__


#elif     defined(__ARM_ARCH_7__) || \
        defined(__ARM_ARCH_7A__) || \
        defined(__ARM_ARCH_7M__) || \
        defined(__ARM_ARCH_7R__) || \
        defined(_ARM_ARCH_7)

    #define NJ_COMPILER_ARM_ARCH 7

#elif   defined(__ARM_ARCH_6__) || \
        defined(__ARM_ARCH_6J__) || \
        defined(__ARM_ARCH_6T2__) || \
        defined(__ARM_ARCH_6Z__) || \
        defined(__ARM_ARCH_6ZK__) || \
        defined(__ARM_ARCH_6M__) || \
        defined(_ARM_ARCH_6)

    #define NJ_COMPILER_ARM_ARCH 6

#elif   defined(__ARM_ARCH_5__) || \
        defined(__ARM_ARCH_5T__) || \
        defined(__ARM_ARCH_5E__) || \
        defined(__ARM_ARCH_5TE__)

    #define NJ_COMPILER_ARM_ARCH 5

#elif   defined(__ARM_ARCH_4T__)

    #define NJ_COMPILER_ARM_ARCH 4


#elif defined(_MSC_VER) && defined(_M_ARM)

    #define NJ_COMPILER_ARM_ARCH _M_ARM


#elif defined(__TARGET_ARCH_ARM)

    #define NJ_COMPILER_ARM_ARCH __TARGET_ARCH_ARM

#else

    
    #define NJ_COMPILER_ARM_ARCH "Unable to determine valid NJ_COMPILER_ARM_ARCH (nanojit only supports ARMv4T or later)"

#endif

#endif 

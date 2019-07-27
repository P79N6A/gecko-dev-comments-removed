



#ifndef SANDBOX_LINUX_SERVICES_ANDROID_UCONTEXT_H_
#define SANDBOX_LINUX_SERVICES_ANDROID_UCONTEXT_H_

#if defined(__ANDROID__)

#if defined(__arm__)
#include "sandbox/linux/services/android_arm_ucontext.h"
#elif defined(__i386__)
#include "sandbox/linux/services/android_i386_ucontext.h"
#elif defined(__x86_64__)
#include "sandbox/linux/services/android_x86_64_ucontext.h"
#elif defined(__mips__)
#include "sandbox/linux/services/android_mips_ucontext.h"
#elif defined(__aarch64__)
#include "sandbox/linux/services/android_arm64_ucontext.h"
#else
#error "No support for your architecture in Android header"
#endif

#else  
#error "Android header file included on non Android."
#endif  

#endif  

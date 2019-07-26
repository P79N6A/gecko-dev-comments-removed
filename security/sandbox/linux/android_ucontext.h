







#ifndef SANDBOX_LINUX_SERVICES_ANDROID_UCONTEXT_H_
#define SANDBOX_LINUX_SERVICES_ANDROID_UCONTEXT_H_

#if defined(__ANDROID__)

#if defined(__arm__)
#include "android_arm_ucontext.h"
#elif defined(__i386__)
#include "android_i386_ucontext.h"
#else
#error "No support for your architecture in Android header"
#endif

#else  
#error "Android header file included on non Android."
#endif  

#endif  

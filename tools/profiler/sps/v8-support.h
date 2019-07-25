







































#ifndef V8_SUPPORT_H_
#define V8_SUPPORT_H_

#if defined(_M_X64) || defined(__x86_64__)
#define V8_HOST_ARCH_X64 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#define V8_HOST_ARCH_IA32 1
#elif defined(__ARMEL__)
#define V8_HOST_ARCH_ARM 1
#else
#warning Please add support for your architecture in chromium_types.h
#endif

typedef int32_t Atomic32;

#if defined(V8_HOST_ARCH_X64) || defined(V8_HOST_ARCH_IA32) || defined(V8_HOST_ARCH_ARM)
inline void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
}
#endif


const int kMaxInt = 0x7FFFFFFF;
const int kMinInt = -kMaxInt - 1;



#define DISALLOW_COPY_AND_ASSIGN(TypeName)      \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)




template <typename T>
static inline void USE(T) { }

class Malloced {
};

#endif 





#ifndef mozilla_LinuxSignal_h
#define mozilla_LinuxSignal_h

namespace mozilla {

#if defined(__arm__)









template <void (*H)(int, siginfo_t*, void*)>
__attribute__((naked)) void
SignalTrampoline(int aSignal, siginfo_t* aInfo, void* aContext)
{
  asm volatile (
    "nop; nop; nop; nop"
    : : : "memory");

  
  

  asm volatile (
    "bx %0"
    :
    : "r"(H), "l"(aSignal), "l"(aInfo), "l"(aContext)
    : "memory");
}

# define MOZ_SIGNAL_TRAMPOLINE(h) (mozilla::SignalTrampoline<h>)

#else 

# define MOZ_SIGNAL_TRAMPOLINE(h) (h)

#endif 

} 

#endif 

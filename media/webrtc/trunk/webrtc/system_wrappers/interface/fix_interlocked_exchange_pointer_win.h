















#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_FIX_INTERLOCKED_EXCHANGE_POINTER_WINDOWS_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_FIX_INTERLOCKED_EXCHANGE_POINTER_WINDOWS_H_

#include <windows.h>


#if !defined(_WIN64) && defined(_Wp64)

#ifdef InterlockedExchangePointer
#undef InterlockedExchangePointer



inline void* InterlockedExchangePointer(void* volatile* target, void* value) {
  return reinterpret_cast<void*>(static_cast<LONG_PTR>(InterlockedExchange(
      reinterpret_cast<volatile LONG*>(target),
      static_cast<LONG>(reinterpret_cast<LONG_PTR>(value)))));
}
#endif  

#endif 

#endif 

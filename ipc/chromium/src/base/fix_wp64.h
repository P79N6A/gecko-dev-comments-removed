






#ifndef BASE_FIX_WP64_H__
#define BASE_FIX_WP64_H__

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

#ifdef SetWindowLongPtrA
#undef SetWindowLongPtrA



inline LONG_PTR SetWindowLongPtrA(HWND window, int index, LONG_PTR new_long) {
  return ::SetWindowLongA(window, index, static_cast<LONG>(new_long));
}
#endif  

#ifdef SetWindowLongPtrW
#undef SetWindowLongPtrW
inline LONG_PTR SetWindowLongPtrW(HWND window, int index, LONG_PTR new_long) {
  return ::SetWindowLongW(window, index, static_cast<LONG>(new_long));
}
#endif  

#ifdef GetWindowLongPtrA
#undef GetWindowLongPtrA
inline LONG_PTR GetWindowLongPtrA(HWND window, int index) {
  return ::GetWindowLongA(window, index);
}
#endif  

#ifdef GetWindowLongPtrW
#undef GetWindowLongPtrW
inline LONG_PTR GetWindowLongPtrW(HWND window, int index) {
  return ::GetWindowLongW(window, index);
}
#endif  

#ifdef SetClassLongPtrA
#undef SetClassLongPtrA
inline LONG_PTR SetClassLongPtrA(HWND window, int index, LONG_PTR new_long) {
  return ::SetClassLongA(window, index, static_cast<LONG>(new_long));
}
#endif  

#ifdef SetClassLongPtrW
#undef SetClassLongPtrW
inline LONG_PTR SetClassLongPtrW(HWND window, int index, LONG_PTR new_long) {
  return ::SetClassLongW(window, index, static_cast<LONG>(new_long));
}
#endif  

#endif  

#endif  




























#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H

#ifndef LIBFFI_H
#error "Please do not include ffitarget.h directly into your source.  Use ffi.h instead."
#endif



#ifndef LIBFFI_ASM
typedef unsigned long          ffi_arg;
typedef signed long            ffi_sarg;

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,
  FFI_EABI,
  FFI_LAST_ABI,
  FFI_DEFAULT_ABI = FFI_EABI
} ffi_abi;
#endif



#define FFI_CLOSURES 1
#define FFI_NATIVE_RAW_API 0

#ifdef __FRV_FDPIC__

#define FFI_TRAMPOLINE_SIZE (8*4)
#else

#define FFI_TRAMPOLINE_SIZE (5*4)
#endif

#endif

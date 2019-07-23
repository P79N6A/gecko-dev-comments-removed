

























#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H



#ifndef LIBFFI_ASM
typedef unsigned long          ffi_arg;
typedef signed long            ffi_sarg;

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,

#ifdef MOXIE
  FFI_EABI,
  FFI_DEFAULT_ABI = FFI_EABI,
#endif

  FFI_LAST_ABI = FFI_DEFAULT_ABI + 1
} ffi_abi;
#endif



#define FFI_CLOSURES 0
#define FFI_NATIVE_RAW_API 0


#define FFI_TRAMPOLINE_SIZE (5*4)

#endif



























#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H

#if defined (__s390x__)
#ifndef S390X
#define S390X
#endif
#endif



#ifndef LIBFFI_ASM
typedef unsigned long          ffi_arg;
typedef signed long            ffi_sarg;

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,
  FFI_SYSV,
  FFI_LAST_ABI,
  FFI_DEFAULT_ABI = FFI_SYSV
} ffi_abi;
#endif




#define FFI_CLOSURES 1
#ifdef S390X
#define FFI_TRAMPOLINE_SIZE 32
#else
#define FFI_TRAMPOLINE_SIZE 16
#endif
#define FFI_NATIVE_RAW_API 0

#endif


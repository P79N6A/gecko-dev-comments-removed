

























#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H



#if defined(__arch64__) || defined(__sparcv9)
#ifndef SPARC64
#define SPARC64
#endif
#endif

#ifndef LIBFFI_ASM
typedef unsigned long          ffi_arg;
typedef signed long            ffi_sarg;

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,
  FFI_V8,
  FFI_V8PLUS,
  FFI_V9,
  FFI_LAST_ABI,
#ifdef SPARC64
  FFI_DEFAULT_ABI = FFI_V9
#else
  FFI_DEFAULT_ABI = FFI_V8
#endif
} ffi_abi;
#endif



#define FFI_CLOSURES 1
#define FFI_NATIVE_RAW_API 0

#ifdef SPARC64
#define FFI_TRAMPOLINE_SIZE 24
#else
#define FFI_TRAMPOLINE_SIZE 16
#endif

#endif



























#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H



#if defined (X86_64) && defined (__i386__)
#undef X86_64
#define X86
#endif



#ifndef LIBFFI_ASM
#ifndef _WIN64
typedef unsigned long          ffi_arg;
#else
typedef unsigned __int64       ffi_arg;
#endif
typedef signed long            ffi_sarg;

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,

  
  FFI_SYSV,
#ifndef _WIN64
  FFI_STDCALL,
#endif
  
  FFI_DEFAULT_ABI = FFI_SYSV,

  


   







  FFI_LAST_ABI = FFI_DEFAULT_ABI + 1
} ffi_abi;
#endif



#define FFI_CLOSURES 1

#ifdef _WIN64
#define FFI_TRAMPOLINE_SIZE 29
#define FFI_NATIVE_RAW_API 0
#else
#define FFI_TRAMPOLINE_SIZE 15
#define FFI_NATIVE_RAW_API 1	/* x86 has native raw api support */
#endif

#endif


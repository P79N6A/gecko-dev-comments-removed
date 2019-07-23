



























#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H



#if defined (X86_64) && defined (__i386__)
#undef X86_64
#define X86
#endif

#ifdef X86_WIN64
#define FFI_SIZEOF_ARG 8
#define USE_BUILTIN_FFS 0 // not yet implemented in mingw-64 
#endif



#ifndef LIBFFI_ASM
#ifdef X86_WIN64
#ifdef _MSC_VER
typedef unsigned __int64       ffi_arg;
typedef __int64                ffi_sarg;
#else
typedef unsigned long long     ffi_arg;
typedef long long              ffi_sarg;
#endif
#else
typedef unsigned long          ffi_arg;
typedef signed long            ffi_sarg;
#endif

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,

  
#ifdef X86_WIN32
  FFI_SYSV,
  FFI_STDCALL,
  
  FFI_DEFAULT_ABI = FFI_SYSV,
#endif

#ifdef X86_WIN64
  FFI_WIN64,
  FFI_DEFAULT_ABI = FFI_WIN64,
#else

  
#if !defined(X86_WIN32) && (defined(__i386__) || defined(__x86_64__))
  FFI_SYSV,
  FFI_UNIX64,   
#ifdef __i386__
  FFI_DEFAULT_ABI = FFI_SYSV,
#else
  FFI_DEFAULT_ABI = FFI_UNIX64,
#endif
#endif
#endif

  FFI_LAST_ABI = FFI_DEFAULT_ABI + 1
} ffi_abi;
#endif



#define FFI_CLOSURES 1
#define FFI_TYPE_SMALL_STRUCT_1B (FFI_TYPE_LAST + 1)
#define FFI_TYPE_SMALL_STRUCT_2B (FFI_TYPE_LAST + 2)
#define FFI_TYPE_SMALL_STRUCT_4B (FFI_TYPE_LAST + 3)

#if defined (X86_64) || (defined (__x86_64__) && defined (X86_DARWIN))
#define FFI_TRAMPOLINE_SIZE 24
#define FFI_NATIVE_RAW_API 0
#else
#ifdef X86_WIN32
#define FFI_TRAMPOLINE_SIZE 13
#else
#ifdef X86_WIN64
#define FFI_TRAMPOLINE_SIZE 29
#define FFI_NATIVE_RAW_API 0
#define FFI_NO_RAW_API 1
#else
#define FFI_TRAMPOLINE_SIZE 10
#endif
#endif
#ifndef X86_WIN64
#define FFI_NATIVE_RAW_API 1	/* x86 has native raw api support */
#endif
#endif

#endif


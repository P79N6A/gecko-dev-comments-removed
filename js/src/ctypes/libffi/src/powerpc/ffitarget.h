




























#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H

#ifndef LIBFFI_H
#error "Please do not include ffitarget.h directly into your source.  Use ffi.h instead."
#endif



#if defined (POWERPC) && defined (__powerpc64__)	
#ifndef POWERPC64
#define POWERPC64
#endif
#elif defined (POWERPC_DARWIN) && defined (__ppc64__)	
#ifndef POWERPC64
#define POWERPC64
#endif
#elif defined (POWERPC_AIX) && defined (__64BIT__)	
#ifndef POWERPC64
#define POWERPC64
#endif
#endif

#ifndef LIBFFI_ASM
typedef unsigned long          ffi_arg;
typedef signed long            ffi_sarg;

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,

#ifdef POWERPC
  FFI_SYSV,
  FFI_GCC_SYSV,
  FFI_LINUX64,
  FFI_LINUX,
  FFI_LINUX_SOFT_FLOAT,
# if defined(POWERPC64)
  FFI_DEFAULT_ABI = FFI_LINUX64,
# elif defined(__NO_FPRS__)
  FFI_DEFAULT_ABI = FFI_LINUX_SOFT_FLOAT,
# elif (__LDBL_MANT_DIG__ == 106)
  FFI_DEFAULT_ABI = FFI_LINUX,
# else
  FFI_DEFAULT_ABI = FFI_GCC_SYSV,
# endif
#endif

#ifdef POWERPC_AIX
  FFI_AIX,
  FFI_DARWIN,
  FFI_DEFAULT_ABI = FFI_AIX,
#endif

#ifdef POWERPC_DARWIN
  FFI_AIX,
  FFI_DARWIN,
  FFI_DEFAULT_ABI = FFI_DARWIN,
#endif

#ifdef POWERPC_FREEBSD
  FFI_SYSV,
  FFI_GCC_SYSV,
  FFI_LINUX64,
  FFI_LINUX,
  FFI_LINUX_SOFT_FLOAT,
  FFI_DEFAULT_ABI = FFI_SYSV,
#endif

  FFI_LAST_ABI
} ffi_abi;
#endif



#define FFI_CLOSURES 1
#define FFI_NATIVE_RAW_API 0
#if defined (POWERPC) || defined (POWERPC_FREEBSD)
# define FFI_TARGET_SPECIFIC_VARIADIC 1
# define FFI_EXTRA_CIF_FIELDS unsigned nfixedargs
#endif





#define FFI_TYPE_UINT128 (FFI_TYPE_LAST + 1)




#define FFI_SYSV_TYPE_SMALL_STRUCT (FFI_TYPE_LAST + 2)


#define FFI_V2_TYPE_FLOAT_HOMOG		(FFI_TYPE_LAST + 1)
#define FFI_V2_TYPE_DOUBLE_HOMOG	(FFI_TYPE_LAST + 2)
#define FFI_V2_TYPE_SMALL_STRUCT	(FFI_TYPE_LAST + 3)

#if _CALL_ELF == 2
# define FFI_TRAMPOLINE_SIZE 32
#else
# if defined(POWERPC64) || defined(POWERPC_AIX)
#  define FFI_TRAMPOLINE_SIZE 24
# else 
#  define FFI_TRAMPOLINE_SIZE 40
# endif
#endif

#ifndef LIBFFI_ASM
#if defined(POWERPC_DARWIN) || defined(POWERPC_AIX)
struct ffi_aix_trampoline_struct {
    void * code_pointer;	
    void * toc;			
    void * static_chain;	
};
#endif
#endif

#endif

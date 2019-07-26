




























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
#ifndef POWERPC_DARWIN64
#define POWERPC_DARWIN64
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

#if defined (POWERPC_AIX)
  FFI_AIX,
  FFI_DARWIN,
  FFI_DEFAULT_ABI = FFI_AIX,
  FFI_LAST_ABI

#elif defined (POWERPC_DARWIN)
  FFI_AIX,
  FFI_DARWIN,
  FFI_DEFAULT_ABI = FFI_DARWIN,
  FFI_LAST_ABI

#else
  


  FFI_COMPAT_SYSV,
  FFI_COMPAT_GCC_SYSV,
  FFI_COMPAT_LINUX64,
  FFI_COMPAT_LINUX,
  FFI_COMPAT_LINUX_SOFT_FLOAT,

# if defined (POWERPC64)
  



  FFI_LINUX = 8,
  
  FFI_LINUX_STRUCT_ALIGN = 1,
  FFI_LINUX_LONG_DOUBLE_128 = 2,
  FFI_DEFAULT_ABI = (FFI_LINUX
#  ifdef __STRUCT_PARM_ALIGN__
		     | FFI_LINUX_STRUCT_ALIGN
#  endif
#  ifdef __LONG_DOUBLE_128__
		     | FFI_LINUX_LONG_DOUBLE_128
#  endif
		     ),
  FFI_LAST_ABI = 12

# else
  

  FFI_SYSV = 8,
  
  FFI_SYSV_SOFT_FLOAT = 1,
  FFI_SYSV_STRUCT_RET = 2,
  FFI_SYSV_IBM_LONG_DOUBLE = 4,
  FFI_SYSV_LONG_DOUBLE_128 = 16,

  FFI_DEFAULT_ABI = (FFI_SYSV
#  ifdef __NO_FPRS__
		     | FFI_SYSV_SOFT_FLOAT
#  endif
#  if (defined (__SVR4_STRUCT_RETURN)					\
       || defined (POWERPC_FREEBSD) && !defined (__AIX_STRUCT_RETURN))
		     | FFI_SYSV_STRUCT_RET
#  endif
#  if __LDBL_MANT_DIG__ == 106
		     | FFI_SYSV_IBM_LONG_DOUBLE
#  endif
#  ifdef __LONG_DOUBLE_128__
		     | FFI_SYSV_LONG_DOUBLE_128
#  endif
		     ),
  FFI_LAST_ABI = 32
# endif
#endif

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
#  if defined(POWERPC_DARWIN64)
#    define FFI_TRAMPOLINE_SIZE 48
#  else
#    define FFI_TRAMPOLINE_SIZE 24
#  endif
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

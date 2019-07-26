
























#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H

#ifndef LIBFFI_H
#error "Please do not include ffitarget.h directly into your source.  Use ffi.h instead."
#endif

#ifndef LIBFFI_ASM

#include <arch/abi.h>

typedef uint_reg_t ffi_arg;
typedef int_reg_t  ffi_sarg;

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,
  FFI_UNIX,
  FFI_LAST_ABI,
  FFI_DEFAULT_ABI = FFI_UNIX
} ffi_abi;
#endif


#define FFI_CLOSURES 1

#ifdef __tilegx__

# define FFI_SIZEOF_ARG 8
# ifdef __LP64__
#  define FFI_TRAMPOLINE_SIZE (8 * 5)  /* 5 bundles */
# else
#  define FFI_TRAMPOLINE_SIZE (8 * 3)  /* 3 bundles */
# endif
#else
# define FFI_SIZEOF_ARG 4
# define FFI_TRAMPOLINE_SIZE 8 /* 1 bundle */
#endif
#define FFI_NATIVE_RAW_API 0

#endif

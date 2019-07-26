




















#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H

#ifndef LIBFFI_H
#error "Please do not include ffitarget.h directly into your source.  Use ffi.h instead."
#endif

#ifndef LIBFFI_ASM
typedef unsigned long ffi_arg;
typedef signed long ffi_sarg;

typedef enum ffi_abi
  {
    FFI_FIRST_ABI = 0,
    FFI_SYSV,
    FFI_LAST_ABI,
    FFI_DEFAULT_ABI = FFI_SYSV
  } ffi_abi;
#endif



#define FFI_CLOSURES 1
#define FFI_TRAMPOLINE_SIZE 36
#define FFI_NATIVE_RAW_API 0



#if defined (__APPLE__)
#define FFI_TARGET_SPECIFIC_VARIADIC
#define FFI_EXTRA_CIF_FIELDS unsigned aarch64_flags; unsigned aarch64_nfixedargs
#else
#define FFI_EXTRA_CIF_FIELDS unsigned aarch64_flags
#endif

#define AARCH64_FFI_WITH_V_BIT 0

#define AARCH64_N_XREG 32
#define AARCH64_N_VREG 32
#define AARCH64_CALL_CONTEXT_SIZE (AARCH64_N_XREG * 8 + AARCH64_N_VREG * 16)

#endif

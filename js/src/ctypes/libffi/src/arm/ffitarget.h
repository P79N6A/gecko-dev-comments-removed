




























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
  FFI_SYSV,
  FFI_VFP,
  FFI_LAST_ABI,
#ifdef __ARM_PCS_VFP
  FFI_DEFAULT_ABI = FFI_VFP,
#else
  FFI_DEFAULT_ABI = FFI_SYSV,
#endif
} ffi_abi;
#endif

#define FFI_EXTRA_CIF_FIELDS			\
  int vfp_used;					\
  short vfp_reg_free, vfp_nargs;		\
  signed char vfp_args[16]			\


#define FFI_TYPE_STRUCT_VFP_FLOAT  (FFI_TYPE_LAST + 1)
#define FFI_TYPE_STRUCT_VFP_DOUBLE (FFI_TYPE_LAST + 2)

#define FFI_TARGET_SPECIFIC_VARIADIC



#define FFI_CLOSURES 1
#define FFI_TRAMPOLINE_SIZE 20
#define FFI_NATIVE_RAW_API 0

#endif

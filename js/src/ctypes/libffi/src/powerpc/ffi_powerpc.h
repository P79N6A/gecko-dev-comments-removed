





























enum {
  
  
  FLAG_RETURNS_SMST	= 1 << (31-31), 
  FLAG_RETURNS_NOTHING  = 1 << (31-30),
  FLAG_RETURNS_FP       = 1 << (31-29),
  FLAG_RETURNS_64BITS   = 1 << (31-28),

  
  FLAG_RETURNS_128BITS  = 1 << (31-27),

  FLAG_COMPAT		= 1 << (31- 8), 

  
  FLAG_ARG_NEEDS_COPY   = 1 << (31- 7), 
  FLAG_ARG_NEEDS_PSAVE  = FLAG_ARG_NEEDS_COPY, 
  FLAG_FP_ARGUMENTS     = 1 << (31- 6), 
  FLAG_4_GPR_ARGUMENTS  = 1 << (31- 5),
  FLAG_RETVAL_REFERENCE = 1 << (31- 4)
};

typedef union
{
  float f;
  double d;
} ffi_dblfl;

void FFI_HIDDEN ffi_closure_SYSV (void);
void FFI_HIDDEN ffi_call_SYSV(extended_cif *, unsigned, unsigned, unsigned *,
			      void (*)(void));

void FFI_HIDDEN ffi_prep_types_sysv (ffi_abi);
ffi_status FFI_HIDDEN ffi_prep_cif_sysv (ffi_cif *);
int FFI_HIDDEN ffi_closure_helper_SYSV (ffi_closure *, void *, unsigned long *,
					ffi_dblfl *, unsigned long *);

void FFI_HIDDEN ffi_call_LINUX64(extended_cif *, unsigned long, unsigned long,
				 unsigned long *, void (*)(void));
void FFI_HIDDEN ffi_closure_LINUX64 (void);

void FFI_HIDDEN ffi_prep_types_linux64 (ffi_abi);
ffi_status FFI_HIDDEN ffi_prep_cif_linux64 (ffi_cif *);
ffi_status FFI_HIDDEN ffi_prep_cif_linux64_var (ffi_cif *, unsigned int,
						unsigned int);
void FFI_HIDDEN ffi_prep_args64 (extended_cif *, unsigned long *const);
int FFI_HIDDEN ffi_closure_helper_LINUX64 (ffi_closure *, void *,
					   unsigned long *, ffi_dblfl *);

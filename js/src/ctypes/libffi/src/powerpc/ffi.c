





























#include "ffi.h"
#include "ffi_common.h"
#include "ffi_powerpc.h"

#if HAVE_LONG_DOUBLE_VARIANT

void FFI_HIDDEN
ffi_prep_types (ffi_abi abi)
{
# if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
#  ifdef POWERPC64
  ffi_prep_types_linux64 (abi);
#  else
  ffi_prep_types_sysv (abi);
#  endif
# endif
}
#endif


ffi_status FFI_HIDDEN
ffi_prep_cif_machdep (ffi_cif *cif)
{
#ifdef POWERPC64
  return ffi_prep_cif_linux64 (cif);
#else
  return ffi_prep_cif_sysv (cif);
#endif
}

ffi_status FFI_HIDDEN
ffi_prep_cif_machdep_var (ffi_cif *cif,
			  unsigned int nfixedargs MAYBE_UNUSED,
			  unsigned int ntotalargs MAYBE_UNUSED)
{
#ifdef POWERPC64
  return ffi_prep_cif_linux64_var (cif, nfixedargs, ntotalargs);
#else
  return ffi_prep_cif_sysv (cif);
#endif
}

void
ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  








  unsigned long smst_buffer[8];
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  ecif.rvalue = rvalue;
  if ((cif->flags & FLAG_RETURNS_SMST) != 0)
    ecif.rvalue = smst_buffer;
  

  else if (!rvalue && cif->rtype->type == FFI_TYPE_STRUCT)
    ecif.rvalue = alloca (cif->rtype->size);

#ifdef POWERPC64
  ffi_call_LINUX64 (&ecif, -(long) cif->bytes, cif->flags, ecif.rvalue, fn);
#else
  ffi_call_SYSV (&ecif, -cif->bytes, cif->flags, ecif.rvalue, fn);
#endif

  
  if (rvalue && ecif.rvalue == smst_buffer)
    {
      unsigned int rsize = cif->rtype->size;
#ifndef __LITTLE_ENDIAN__
      

# ifndef POWERPC64
      if (rsize <= 4)
	memcpy (rvalue, (char *) smst_buffer + 4 - rsize, rsize);
      else
# endif
	


	if (rsize <= 8)
	  memcpy (rvalue, (char *) smst_buffer + 8 - rsize, rsize);
	else
#endif
	  memcpy (rvalue, smst_buffer, rsize);
    }
}


ffi_status
ffi_prep_closure_loc (ffi_closure *closure,
		      ffi_cif *cif,
		      void (*fun) (ffi_cif *, void *, void **, void *),
		      void *user_data,
		      void *codeloc)
{
#ifdef POWERPC64
  return ffi_prep_closure_loc_linux64 (closure, cif, fun, user_data, codeloc);
#else
  return ffi_prep_closure_loc_sysv (closure, cif, fun, user_data, codeloc);
#endif
}

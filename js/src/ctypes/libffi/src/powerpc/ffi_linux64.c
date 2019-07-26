





























#include "ffi.h"

#ifdef POWERPC64
#include "ffi_common.h"
#include "ffi_powerpc.h"



enum {
  NUM_GPR_ARG_REGISTERS64 = 8,
  NUM_FPR_ARG_REGISTERS64 = 13
};
enum { ASM_NEEDS_REGISTERS64 = 4 };


#if HAVE_LONG_DOUBLE_VARIANT && FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE

void FFI_HIDDEN
ffi_prep_types_linux64 (ffi_abi abi)
{
  if ((abi & (FFI_LINUX | FFI_LINUX_LONG_DOUBLE_128)) == FFI_LINUX)
    {
      ffi_type_longdouble.size = 8;
      ffi_type_longdouble.alignment = 8;
    }
  else
    {
      ffi_type_longdouble.size = 16;
      ffi_type_longdouble.alignment = 16;
    }
}
#endif


#if _CALL_ELF == 2
static unsigned int
discover_homogeneous_aggregate (const ffi_type *t, unsigned int *elnum)
{
  switch (t->type)
    {
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
      *elnum = 1;
      return (int) t->type;

    case FFI_TYPE_STRUCT:;
      {
	unsigned int base_elt = 0, total_elnum = 0;
	ffi_type **el = t->elements;
	while (*el)
	  {
	    unsigned int el_elt, el_elnum = 0;
	    el_elt = discover_homogeneous_aggregate (*el, &el_elnum);
	    if (el_elt == 0
		|| (base_elt && base_elt != el_elt))
	      return 0;
	    base_elt = el_elt;
	    total_elnum += el_elnum;
	    if (total_elnum > 8)
	      return 0;
	    el++;
	  }
	*elnum = total_elnum;
	return base_elt;
      }

    default:
      return 0;
    }
}
#endif



static ffi_status
ffi_prep_cif_linux64_core (ffi_cif *cif)
{
  ffi_type **ptr;
  unsigned bytes;
  unsigned i, fparg_count = 0, intarg_count = 0;
  unsigned flags = cif->flags;
#if _CALL_ELF == 2
  unsigned int elt, elnum;
#endif

#if FFI_TYPE_LONGDOUBLE == FFI_TYPE_DOUBLE
  
  if ((cif->abi & FFI_LINUX_LONG_DOUBLE_128) != 0)
    return FFI_BAD_ABI;
#endif

  

#if _CALL_ELF == 2
  
  bytes = (4 + ASM_NEEDS_REGISTERS64) * sizeof (long);

  
  bytes += NUM_GPR_ARG_REGISTERS64 * sizeof (long);
#else
  

  bytes = (6 + ASM_NEEDS_REGISTERS64) * sizeof (long);

  
  bytes += 2 * NUM_GPR_ARG_REGISTERS64 * sizeof (long);
#endif

  
  switch (cif->rtype->type)
    {
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
    case FFI_TYPE_LONGDOUBLE:
      if ((cif->abi & FFI_LINUX_LONG_DOUBLE_128) != 0)
	flags |= FLAG_RETURNS_128BITS;
      
#endif
    case FFI_TYPE_DOUBLE:
      flags |= FLAG_RETURNS_64BITS;
      
    case FFI_TYPE_FLOAT:
      flags |= FLAG_RETURNS_FP;
      break;

    case FFI_TYPE_UINT128:
      flags |= FLAG_RETURNS_128BITS;
      
    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
      flags |= FLAG_RETURNS_64BITS;
      break;

    case FFI_TYPE_STRUCT:
#if _CALL_ELF == 2
      elt = discover_homogeneous_aggregate (cif->rtype, &elnum);
      if (elt)
	{
	  if (elt == FFI_TYPE_DOUBLE)
	    flags |= FLAG_RETURNS_64BITS;
	  flags |= FLAG_RETURNS_FP | FLAG_RETURNS_SMST;
	  break;
	}
      if (cif->rtype->size <= 16)
	{
	  flags |= FLAG_RETURNS_SMST;
	  break;
	}
#endif
      intarg_count++;
      flags |= FLAG_RETVAL_REFERENCE;
      
    case FFI_TYPE_VOID:
      flags |= FLAG_RETURNS_NOTHING;
      break;

    default:
      
      break;
    }

  for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++)
    {
      unsigned int align;

      switch ((*ptr)->type)
	{
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
	  if ((cif->abi & FFI_LINUX_LONG_DOUBLE_128) != 0)
	    {
	      fparg_count++;
	      intarg_count++;
	    }
	  
#endif
	case FFI_TYPE_DOUBLE:
	case FFI_TYPE_FLOAT:
	  fparg_count++;
	  intarg_count++;
	  if (fparg_count > NUM_FPR_ARG_REGISTERS64)
	    flags |= FLAG_ARG_NEEDS_PSAVE;
	  break;

	case FFI_TYPE_STRUCT:
	  if ((cif->abi & FFI_LINUX_STRUCT_ALIGN) != 0)
	    {
	      align = (*ptr)->alignment;
	      if (align > 16)
		align = 16;
	      align = align / 8;
	      if (align > 1)
		intarg_count = ALIGN (intarg_count, align);
	    }
	  intarg_count += ((*ptr)->size + 7) / 8;
#if _CALL_ELF == 2
	  elt = discover_homogeneous_aggregate (*ptr, &elnum);
	  if (elt)
	    {
	      fparg_count += elnum;
	      if (fparg_count > NUM_FPR_ARG_REGISTERS64)
		flags |= FLAG_ARG_NEEDS_PSAVE;
	    }
	  else
#endif
	    {
	      if (intarg_count > NUM_GPR_ARG_REGISTERS64)
		flags |= FLAG_ARG_NEEDS_PSAVE;
	    }
	  break;

	case FFI_TYPE_POINTER:
	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	case FFI_TYPE_INT:
	case FFI_TYPE_UINT32:
	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT16:
	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT8:
	case FFI_TYPE_SINT8:
	  

	  intarg_count++;
	  if (intarg_count > NUM_GPR_ARG_REGISTERS64)
	    flags |= FLAG_ARG_NEEDS_PSAVE;
	  break;
	default:
	  FFI_ASSERT (0);
	}
    }

  if (fparg_count != 0)
    flags |= FLAG_FP_ARGUMENTS;
  if (intarg_count > 4)
    flags |= FLAG_4_GPR_ARGUMENTS;

  
  if (fparg_count != 0)
    bytes += NUM_FPR_ARG_REGISTERS64 * sizeof (double);

  
#if _CALL_ELF == 2
  if ((flags & FLAG_ARG_NEEDS_PSAVE) != 0)
    bytes += intarg_count * sizeof (long);
#else
  if (intarg_count > NUM_GPR_ARG_REGISTERS64)
    bytes += (intarg_count - NUM_GPR_ARG_REGISTERS64) * sizeof (long);
#endif

  
  bytes = (bytes + 15) & ~0xF;

  cif->flags = flags;
  cif->bytes = bytes;

  return FFI_OK;
}

ffi_status FFI_HIDDEN
ffi_prep_cif_linux64 (ffi_cif *cif)
{
  if ((cif->abi & FFI_LINUX) != 0)
    cif->nfixedargs = cif->nargs;
#if _CALL_ELF != 2
  else if (cif->abi == FFI_COMPAT_LINUX64)
    {
      

      cif->flags |= FLAG_COMPAT;
      
      cif->abi = FFI_LINUX | FFI_LINUX_LONG_DOUBLE_128;
    }
#endif
  else
    return FFI_BAD_ABI;
  return ffi_prep_cif_linux64_core (cif);
}

ffi_status FFI_HIDDEN
ffi_prep_cif_linux64_var (ffi_cif *cif,
			  unsigned int nfixedargs,
			  unsigned int ntotalargs MAYBE_UNUSED)
{
  if ((cif->abi & FFI_LINUX) != 0)
    cif->nfixedargs = nfixedargs;
#if _CALL_ELF != 2
  else if (cif->abi == FFI_COMPAT_LINUX64)
    {
      

      cif->flags |= FLAG_COMPAT;
      
      cif->abi = FFI_LINUX | FFI_LINUX_LONG_DOUBLE_128;
    }
#endif
  else
    return FFI_BAD_ABI;
#if _CALL_ELF == 2
  cif->flags |= FLAG_ARG_NEEDS_PSAVE;
#endif
  return ffi_prep_cif_linux64_core (cif);
}




































void FFI_HIDDEN
ffi_prep_args64 (extended_cif *ecif, unsigned long *const stack)
{
  const unsigned long bytes = ecif->cif->bytes;
  const unsigned long flags = ecif->cif->flags;

  typedef union
  {
    char *c;
    unsigned long *ul;
    float *f;
    double *d;
    size_t p;
  } valp;

  
  valp stacktop;

  

  valp gpr_base;
  valp gpr_end;
  valp rest;
  valp next_arg;

  

  valp fpr_base;
  unsigned int fparg_count;

  unsigned int i, words, nargs, nfixedargs;
  ffi_type **ptr;
  double double_tmp;
  union
  {
    void **v;
    char **c;
    signed char **sc;
    unsigned char **uc;
    signed short **ss;
    unsigned short **us;
    signed int **si;
    unsigned int **ui;
    unsigned long **ul;
    float **f;
    double **d;
  } p_argv;
  unsigned long gprvalue;
  unsigned long align;

  stacktop.c = (char *) stack + bytes;
  gpr_base.ul = stacktop.ul - ASM_NEEDS_REGISTERS64 - NUM_GPR_ARG_REGISTERS64;
  gpr_end.ul = gpr_base.ul + NUM_GPR_ARG_REGISTERS64;
#if _CALL_ELF == 2
  rest.ul = stack + 4 + NUM_GPR_ARG_REGISTERS64;
#else
  rest.ul = stack + 6 + NUM_GPR_ARG_REGISTERS64;
#endif
  fpr_base.d = gpr_base.d - NUM_FPR_ARG_REGISTERS64;
  fparg_count = 0;
  next_arg.ul = gpr_base.ul;

  
  FFI_ASSERT (((unsigned long) (char *) stack & 0xF) == 0);
  FFI_ASSERT (((unsigned long) stacktop.c & 0xF) == 0);
  FFI_ASSERT ((bytes & 0xF) == 0);

  
  if (flags & FLAG_RETVAL_REFERENCE)
    *next_arg.ul++ = (unsigned long) (char *) ecif->rvalue;

  
  p_argv.v = ecif->avalue;
  nargs = ecif->cif->nargs;
#if _CALL_ELF != 2
  nfixedargs = (unsigned) -1;
  if ((flags & FLAG_COMPAT) == 0)
#endif
    nfixedargs = ecif->cif->nfixedargs;
  for (ptr = ecif->cif->arg_types, i = 0;
       i < nargs;
       i++, ptr++, p_argv.v++)
    {
#if _CALL_ELF == 2
      unsigned int elt, elnum;
#endif

      switch ((*ptr)->type)
	{
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
	  if ((ecif->cif->abi & FFI_LINUX_LONG_DOUBLE_128) != 0)
	    {
	      double_tmp = (*p_argv.d)[0];
	      if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
		{
		  *fpr_base.d++ = double_tmp;
# if _CALL_ELF != 2
		  if ((flags & FLAG_COMPAT) != 0)
		    *next_arg.d = double_tmp;
# endif
		}
	      else
		*next_arg.d = double_tmp;
	      if (++next_arg.ul == gpr_end.ul)
		next_arg.ul = rest.ul;
	      fparg_count++;
	      double_tmp = (*p_argv.d)[1];
	      if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
		{
		  *fpr_base.d++ = double_tmp;
# if _CALL_ELF != 2
		  if ((flags & FLAG_COMPAT) != 0)
		    *next_arg.d = double_tmp;
# endif
		}
	      else
		*next_arg.d = double_tmp;
	      if (++next_arg.ul == gpr_end.ul)
		next_arg.ul = rest.ul;
	      fparg_count++;
	      FFI_ASSERT (__LDBL_MANT_DIG__ == 106);
	      FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	      break;
	    }
	  
#endif
	case FFI_TYPE_DOUBLE:
	  double_tmp = **p_argv.d;
	  if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
	    {
	      *fpr_base.d++ = double_tmp;
#if _CALL_ELF != 2
	      if ((flags & FLAG_COMPAT) != 0)
		*next_arg.d = double_tmp;
#endif
	    }
	  else
	    *next_arg.d = double_tmp;
	  if (++next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  fparg_count++;
	  FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;

	case FFI_TYPE_FLOAT:
	  double_tmp = **p_argv.f;
	  if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
	    {
	      *fpr_base.d++ = double_tmp;
#if _CALL_ELF != 2
	      if ((flags & FLAG_COMPAT) != 0)
		*next_arg.f = (float) double_tmp;
#endif
	    }
	  else
	    *next_arg.f = (float) double_tmp;
	  if (++next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  fparg_count++;
	  FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;

	case FFI_TYPE_STRUCT:
	  if ((ecif->cif->abi & FFI_LINUX_STRUCT_ALIGN) != 0)
	    {
	      align = (*ptr)->alignment;
	      if (align > 16)
		align = 16;
	      if (align > 1)
		next_arg.p = ALIGN (next_arg.p, align);
	    }
#if _CALL_ELF == 2
	  elt = discover_homogeneous_aggregate (*ptr, &elnum);
	  if (elt)
	    {
	      union {
		void *v;
		float *f;
		double *d;
	      } arg;

	      arg.v = *p_argv.v;
	      if (elt == FFI_TYPE_FLOAT)
		{
		  do
		    {
		      double_tmp = *arg.f++;
		      if (fparg_count < NUM_FPR_ARG_REGISTERS64
			  && i < nfixedargs)
			*fpr_base.d++ = double_tmp;
		      else
			*next_arg.f = (float) double_tmp;
		      if (++next_arg.f == gpr_end.f)
			next_arg.f = rest.f;
		      fparg_count++;
		    }
		  while (--elnum != 0);
		  if ((next_arg.p & 3) != 0)
		    {
		      if (++next_arg.f == gpr_end.f)
			next_arg.f = rest.f;
		    }
		}
	      else
		do
		  {
		    double_tmp = *arg.d++;
		    if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
		      *fpr_base.d++ = double_tmp;
		    else
		      *next_arg.d = double_tmp;
		    if (++next_arg.d == gpr_end.d)
		      next_arg.d = rest.d;
		    fparg_count++;
		  }
		while (--elnum != 0);
	    }
	  else
#endif
	    {
	      words = ((*ptr)->size + 7) / 8;
	      if (next_arg.ul >= gpr_base.ul && next_arg.ul + words > gpr_end.ul)
		{
		  size_t first = gpr_end.c - next_arg.c;
		  memcpy (next_arg.c, *p_argv.c, first);
		  memcpy (rest.c, *p_argv.c + first, (*ptr)->size - first);
		  next_arg.c = rest.c + words * 8 - first;
		}
	      else
		{
		  char *where = next_arg.c;

#ifndef __LITTLE_ENDIAN__
		  

		  if ((*ptr)->size < 8)
		    where += 8 - (*ptr)->size;
#endif
		  memcpy (where, *p_argv.c, (*ptr)->size);
		  next_arg.ul += words;
		  if (next_arg.ul == gpr_end.ul)
		    next_arg.ul = rest.ul;
		}
	    }
	  break;

	case FFI_TYPE_UINT8:
	  gprvalue = **p_argv.uc;
	  goto putgpr;
	case FFI_TYPE_SINT8:
	  gprvalue = **p_argv.sc;
	  goto putgpr;
	case FFI_TYPE_UINT16:
	  gprvalue = **p_argv.us;
	  goto putgpr;
	case FFI_TYPE_SINT16:
	  gprvalue = **p_argv.ss;
	  goto putgpr;
	case FFI_TYPE_UINT32:
	  gprvalue = **p_argv.ui;
	  goto putgpr;
	case FFI_TYPE_INT:
	case FFI_TYPE_SINT32:
	  gprvalue = **p_argv.si;
	  goto putgpr;

	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	case FFI_TYPE_POINTER:
	  gprvalue = **p_argv.ul;
	putgpr:
	  *next_arg.ul++ = gprvalue;
	  if (next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  break;
	}
    }

  FFI_ASSERT (flags & FLAG_4_GPR_ARGUMENTS
	      || (next_arg.ul >= gpr_base.ul
		  && next_arg.ul <= gpr_base.ul + 4));
}


#if _CALL_ELF == 2
#define MIN_CACHE_LINE_SIZE 8

static void
flush_icache (char *wraddr, char *xaddr, int size)
{
  int i;
  for (i = 0; i < size; i += MIN_CACHE_LINE_SIZE)
    __asm__ volatile ("icbi 0,%0;" "dcbf 0,%1;"
		      : : "r" (xaddr + i), "r" (wraddr + i) : "memory");
  __asm__ volatile ("icbi 0,%0;" "dcbf 0,%1;" "sync;" "isync;"
		    : : "r"(xaddr + size - 1), "r"(wraddr + size - 1)
		    : "memory");
}
#endif

ffi_status
ffi_prep_closure_loc_linux64 (ffi_closure *closure,
			      ffi_cif *cif,
			      void (*fun) (ffi_cif *, void *, void **, void *),
			      void *user_data,
			      void *codeloc)
{
#if _CALL_ELF == 2
  unsigned int *tramp = (unsigned int *) &closure->tramp[0];

  if (cif->abi < FFI_LINUX || cif->abi >= FFI_LAST_ABI)
    return FFI_BAD_ABI;

  tramp[0] = 0xe96c0018;	
  tramp[1] = 0xe98c0010;	
  tramp[2] = 0x7d8903a6;	
  tramp[3] = 0x4e800420;	
				
				
  *(void **) &tramp[4] = (void *) ffi_closure_LINUX64;
  *(void **) &tramp[6] = codeloc;
  flush_icache ((char *)tramp, (char *)codeloc, FFI_TRAMPOLINE_SIZE);
#else
  void **tramp = (void **) &closure->tramp[0];

  if (cif->abi < FFI_LINUX || cif->abi >= FFI_LAST_ABI)
    return FFI_BAD_ABI;

  
  memcpy (tramp, (char *) ffi_closure_LINUX64, 16);
  tramp[2] = tramp[1];
  tramp[1] = codeloc;
#endif

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return FFI_OK;
}


int FFI_HIDDEN
ffi_closure_helper_LINUX64 (ffi_closure *closure, void *rvalue,
			    unsigned long *pst, ffi_dblfl *pfr)
{
  
  

  

  void **avalue;
  ffi_type **arg_types;
  unsigned long i, avn, nfixedargs;
  ffi_cif *cif;
  ffi_dblfl *end_pfr = pfr + NUM_FPR_ARG_REGISTERS64;
  unsigned long align;

  cif = closure->cif;
  avalue = alloca (cif->nargs * sizeof (void *));

  

  if (cif->rtype->type == FFI_TYPE_STRUCT
      && (cif->flags & FLAG_RETURNS_SMST) == 0)
    {
      rvalue = (void *) *pst;
      pst++;
    }

  i = 0;
  avn = cif->nargs;
#if _CALL_ELF != 2
  nfixedargs = (unsigned) -1;
  if ((cif->flags & FLAG_COMPAT) == 0)
#endif
    nfixedargs = cif->nfixedargs;
  arg_types = cif->arg_types;

  
  while (i < avn)
    {
      unsigned int elt, elnum;

      switch (arg_types[i]->type)
	{
	case FFI_TYPE_SINT8:
	case FFI_TYPE_UINT8:
#ifndef __LITTLE_ENDIAN__
	  avalue[i] = (char *) pst + 7;
	  pst++;
	  break;
#endif

	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT16:
#ifndef __LITTLE_ENDIAN__
	  avalue[i] = (char *) pst + 6;
	  pst++;
	  break;
#endif

	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT32:
#ifndef __LITTLE_ENDIAN__
	  avalue[i] = (char *) pst + 4;
	  pst++;
	  break;
#endif

	case FFI_TYPE_SINT64:
	case FFI_TYPE_UINT64:
	case FFI_TYPE_POINTER:
	  avalue[i] = pst;
	  pst++;
	  break;

	case FFI_TYPE_STRUCT:
	  if ((cif->abi & FFI_LINUX_STRUCT_ALIGN) != 0)
	    {
	      align = arg_types[i]->alignment;
	      if (align > 16)
		align = 16;
	      if (align > 1)
		pst = (unsigned long *) ALIGN ((size_t) pst, align);
	    }
	  elt = 0;
#if _CALL_ELF == 2
	  elt = discover_homogeneous_aggregate (arg_types[i], &elnum);
#endif
	  if (elt)
	    {
	      union {
		void *v;
		unsigned long *ul;
		float *f;
		double *d;
		size_t p;
	      } to, from;

	      



	      if (pfr + elnum <= end_pfr)
		to.v = pfr;
	      else
		to.v = pst;

	      avalue[i] = to.v;
	      from.ul = pst;
	      if (elt == FFI_TYPE_FLOAT)
		{
		  do
		    {
		      if (pfr < end_pfr && i < nfixedargs)
			{
			  *to.f = (float) pfr->d;
			  pfr++;
			}
		      else
			*to.f = *from.f;
		      to.f++;
		      from.f++;
		    }
		  while (--elnum != 0);
		}
	      else
		{
		  do
		    {
		      if (pfr < end_pfr && i < nfixedargs)
			{
			  *to.d = pfr->d;
			  pfr++;
			}
		      else
			*to.d = *from.d;
		      to.d++;
		      from.d++;
		    }
		  while (--elnum != 0);
		}
	    }
	  else
	    {
#ifndef __LITTLE_ENDIAN__
	      

	      if (arg_types[i]->size < 8)
		avalue[i] = (char *) pst + 8 - arg_types[i]->size;
	      else
#endif
		avalue[i] = pst;
	    }
	  pst += (arg_types[i]->size + 7) / 8;
	  break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
	  if ((cif->abi & FFI_LINUX_LONG_DOUBLE_128) != 0)
	    {
	      if (pfr + 1 < end_pfr && i + 1 < nfixedargs)
		{
		  avalue[i] = pfr;
		  pfr += 2;
		}
	      else
		{
		  if (pfr < end_pfr && i < nfixedargs)
		    {
		      

		      *pst = *(unsigned long *) pfr;
		      pfr++;
		    }
		  avalue[i] = pst;
		}
	      pst += 2;
	      break;
	    }
	  
#endif
	case FFI_TYPE_DOUBLE:
	  
	  

	  if (pfr < end_pfr && i < nfixedargs)
	    {
	      avalue[i] = pfr;
	      pfr++;
	    }
	  else
	    avalue[i] = pst;
	  pst++;
	  break;

	case FFI_TYPE_FLOAT:
	  if (pfr < end_pfr && i < nfixedargs)
	    {
	      

	      pfr->f = (float) pfr->d;
	      avalue[i] = pfr;
	      pfr++;
	    }
	  else
	    avalue[i] = pst;
	  pst++;
	  break;

	default:
	  FFI_ASSERT (0);
	}

      i++;
    }


  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  
  if ((cif->flags & FLAG_RETURNS_SMST) != 0)
    {
      if ((cif->flags & FLAG_RETURNS_FP) == 0)
	return FFI_V2_TYPE_SMALL_STRUCT + cif->rtype->size - 1;
      else if ((cif->flags & FLAG_RETURNS_64BITS) != 0)
	return FFI_V2_TYPE_DOUBLE_HOMOG;
      else
	return FFI_V2_TYPE_FLOAT_HOMOG;
    }
  return cif->rtype->type;
}
#endif

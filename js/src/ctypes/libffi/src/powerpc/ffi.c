




























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>
#include <stdio.h>


extern void ffi_closure_SYSV (void);
extern void FFI_HIDDEN ffi_closure_LINUX64 (void);

enum {
  
  FLAG_RETURNS_SMST	= 1 << (31-31), 
  FLAG_RETURNS_NOTHING  = 1 << (31-30), 
#ifndef __NO_FPRS__
  FLAG_RETURNS_FP       = 1 << (31-29),
#endif
  FLAG_RETURNS_64BITS   = 1 << (31-28),

  FLAG_RETURNS_128BITS  = 1 << (31-27), 

  FLAG_ARG_NEEDS_COPY   = 1 << (31- 7),
  FLAG_ARG_NEEDS_PSAVE  = FLAG_ARG_NEEDS_COPY, 
#ifndef __NO_FPRS__
  FLAG_FP_ARGUMENTS     = 1 << (31- 6), 
#endif
  FLAG_4_GPR_ARGUMENTS  = 1 << (31- 5),
  FLAG_RETVAL_REFERENCE = 1 << (31- 4)
};


#define ASM_NEEDS_REGISTERS 4
#define NUM_GPR_ARG_REGISTERS 8
#ifndef __NO_FPRS__
# define NUM_FPR_ARG_REGISTERS 8
#endif



























void
ffi_prep_args_SYSV (extended_cif *ecif, unsigned *const stack)
{
  const unsigned bytes = ecif->cif->bytes;
  const unsigned flags = ecif->cif->flags;

  typedef union {
    char *c;
    unsigned *u;
    long long *ll;
    float *f;
    double *d;
  } valp;

  
  valp stacktop;

  

  valp gpr_base;
  int intarg_count;

#ifndef __NO_FPRS__
  

  valp fpr_base;
  int fparg_count;
#endif

  

  valp copy_space;

  
  valp next_arg;

  int i;
  ffi_type **ptr;
#ifndef __NO_FPRS__
  double double_tmp;
#endif
  union {
    void **v;
    char **c;
    signed char **sc;
    unsigned char **uc;
    signed short **ss;
    unsigned short **us;
    unsigned int **ui;
    long long **ll;
    float **f;
    double **d;
  } p_argv;
  size_t struct_copy_size;
  unsigned gprvalue;

  stacktop.c = (char *) stack + bytes;
  gpr_base.u = stacktop.u - ASM_NEEDS_REGISTERS - NUM_GPR_ARG_REGISTERS;
  intarg_count = 0;
#ifndef __NO_FPRS__
  fpr_base.d = gpr_base.d - NUM_FPR_ARG_REGISTERS;
  fparg_count = 0;
  copy_space.c = ((flags & FLAG_FP_ARGUMENTS) ? fpr_base.c : gpr_base.c);
#else
  copy_space.c = gpr_base.c;
#endif
  next_arg.u = stack + 2;

  
  FFI_ASSERT (((unsigned long) (char *) stack & 0xF) == 0);
  FFI_ASSERT (((unsigned long) copy_space.c & 0xF) == 0);
  FFI_ASSERT (((unsigned long) stacktop.c & 0xF) == 0);
  FFI_ASSERT ((bytes & 0xF) == 0);
  FFI_ASSERT (copy_space.c >= next_arg.c);

  
  if (flags & FLAG_RETVAL_REFERENCE)
    {
      *gpr_base.u++ = (unsigned long) (char *) ecif->rvalue;
      intarg_count++;
    }

  
  p_argv.v = ecif->avalue;
  for (ptr = ecif->cif->arg_types, i = ecif->cif->nargs;
       i > 0;
       i--, ptr++, p_argv.v++)
    {
      unsigned short typenum = (*ptr)->type;

      
      if (ecif->cif->abi == FFI_LINUX_SOFT_FLOAT) {
		if (typenum == FFI_TYPE_FLOAT)
			typenum = FFI_TYPE_UINT32;
		if (typenum == FFI_TYPE_DOUBLE)
			typenum = FFI_TYPE_UINT64;
		if (typenum == FFI_TYPE_LONGDOUBLE)
			typenum = FFI_TYPE_UINT128;
      } else if (ecif->cif->abi != FFI_LINUX) {
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
		if (typenum == FFI_TYPE_LONGDOUBLE)
			typenum = FFI_TYPE_STRUCT;
#endif
      }

      
      switch (typenum) {
#ifndef __NO_FPRS__
	case FFI_TYPE_FLOAT:
	  
	  double_tmp = **p_argv.f;
	  if (fparg_count >= NUM_FPR_ARG_REGISTERS)
	    {
	      *next_arg.f = (float) double_tmp;
	      next_arg.u += 1;
	      intarg_count++;
	    }
	  else
	    *fpr_base.d++ = double_tmp;
	  fparg_count++;
	  FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;

	case FFI_TYPE_DOUBLE:
	  
	  double_tmp = **p_argv.d;

	  if (fparg_count >= NUM_FPR_ARG_REGISTERS)
	    {
	      if (intarg_count >= NUM_GPR_ARG_REGISTERS
		  && intarg_count % 2 != 0)
		{
		  intarg_count++;
		  next_arg.u++;
		}
	      *next_arg.d = double_tmp;
	      next_arg.u += 2;
	    }
	  else
	    *fpr_base.d++ = double_tmp;
	  fparg_count++;
	  FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
	      double_tmp = (*p_argv.d)[0];

	      if (fparg_count >= NUM_FPR_ARG_REGISTERS - 1)
		{
		  if (intarg_count >= NUM_GPR_ARG_REGISTERS
		      && intarg_count % 2 != 0)
		    {
		      intarg_count++;
		      next_arg.u++;
		    }
		  *next_arg.d = double_tmp;
		  next_arg.u += 2;
		  double_tmp = (*p_argv.d)[1];
		  *next_arg.d = double_tmp;
		  next_arg.u += 2;
		}
	      else
		{
		  *fpr_base.d++ = double_tmp;
		  double_tmp = (*p_argv.d)[1];
		  *fpr_base.d++ = double_tmp;
		}

	      fparg_count += 2;
	      FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;
#endif
#endif 

	





	case FFI_TYPE_UINT128: {
		unsigned int int_tmp = (*p_argv.ui)[0];
		unsigned int ii;
		if (intarg_count >= NUM_GPR_ARG_REGISTERS - 3) {
			if (intarg_count < NUM_GPR_ARG_REGISTERS)
				intarg_count += NUM_GPR_ARG_REGISTERS - intarg_count;
			*(next_arg.u++) = int_tmp;
			for (ii = 1; ii < 4; ii++) {
				int_tmp = (*p_argv.ui)[ii];
				*(next_arg.u++) = int_tmp;
			}
		} else {
			*(gpr_base.u++) = int_tmp;
			for (ii = 1; ii < 4; ii++) {
				int_tmp = (*p_argv.ui)[ii];
				*(gpr_base.u++) = int_tmp;
			}
		}
		intarg_count += 4;
		break;
	}

	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	  if (intarg_count == NUM_GPR_ARG_REGISTERS-1)
	    intarg_count++;
	  if (intarg_count >= NUM_GPR_ARG_REGISTERS)
	    {
	      if (intarg_count % 2 != 0)
		{
		  intarg_count++;
		  next_arg.u++;
		}
	      *next_arg.ll = **p_argv.ll;
	      next_arg.u += 2;
	    }
	  else
	    {
	      






	      if (intarg_count % 2 != 0)
		{
		  intarg_count ++;
		  gpr_base.u++;
		}
	      *gpr_base.ll++ = **p_argv.ll;
	    }
	  intarg_count += 2;
	  break;

	case FFI_TYPE_STRUCT:
	  struct_copy_size = ((*ptr)->size + 15) & ~0xF;
	  copy_space.c -= struct_copy_size;
	  memcpy (copy_space.c, *p_argv.c, (*ptr)->size);

	  gprvalue = (unsigned long) copy_space.c;

	  FFI_ASSERT (copy_space.c > next_arg.c);
	  FFI_ASSERT (flags & FLAG_ARG_NEEDS_COPY);
	  goto putgpr;

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

	case FFI_TYPE_INT:
	case FFI_TYPE_UINT32:
	case FFI_TYPE_SINT32:
	case FFI_TYPE_POINTER:

	  gprvalue = **p_argv.ui;

	putgpr:
	  if (intarg_count >= NUM_GPR_ARG_REGISTERS)
	    *next_arg.u++ = gprvalue;
	  else
	    *gpr_base.u++ = gprvalue;
	  intarg_count++;
	  break;
	}
    }

  
  FFI_ASSERT (copy_space.c >= next_arg.c);
  FFI_ASSERT (gpr_base.u <= stacktop.u - ASM_NEEDS_REGISTERS);
  



#ifndef __NO_FPRS__
  if (fparg_count > NUM_FPR_ARG_REGISTERS)
    intarg_count -= fparg_count - NUM_FPR_ARG_REGISTERS;
  FFI_ASSERT (fpr_base.u
	      <= stacktop.u - ASM_NEEDS_REGISTERS - NUM_GPR_ARG_REGISTERS);
#endif
  FFI_ASSERT (flags & FLAG_4_GPR_ARGUMENTS || intarg_count <= 4);
}


enum {
  NUM_GPR_ARG_REGISTERS64 = 8,
  NUM_FPR_ARG_REGISTERS64 = 13
};
enum { ASM_NEEDS_REGISTERS64 = 4 };

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




































void FFI_HIDDEN
ffi_prep_args64 (extended_cif *ecif, unsigned long *const stack)
{
  const unsigned long bytes = ecif->cif->bytes;
  const unsigned long flags = ecif->cif->flags;

  typedef union {
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
  union {
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
#ifdef __STRUCT_PARM_ALIGN__
  unsigned long align;
#endif

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
  nfixedargs = ecif->cif->nfixedargs;
  for (ptr = ecif->cif->arg_types, i = 0;
       i < nargs;
       i++, ptr++, p_argv.v++)
    {
      unsigned int elt, elnum;

      switch ((*ptr)->type)
	{
	case FFI_TYPE_FLOAT:
	  double_tmp = **p_argv.f;
	  if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
	    *fpr_base.d++ = double_tmp;
	  else
	    *next_arg.f = (float) double_tmp;
	  if (++next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  fparg_count++;
	  FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;

	case FFI_TYPE_DOUBLE:
	  double_tmp = **p_argv.d;
	  if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
	    *fpr_base.d++ = double_tmp;
	  else
	    *next_arg.d = double_tmp;
	  if (++next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  fparg_count++;
	  FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
	  double_tmp = (*p_argv.d)[0];
	  if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
	    *fpr_base.d++ = double_tmp;
	  else
	    *next_arg.d = double_tmp;
	  if (++next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  fparg_count++;
	  double_tmp = (*p_argv.d)[1];
	  if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
	    *fpr_base.d++ = double_tmp;
	  else
	    *next_arg.d = double_tmp;
	  if (++next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  fparg_count++;
	  FFI_ASSERT (__LDBL_MANT_DIG__ == 106);
	  FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;
#endif

	case FFI_TYPE_STRUCT:
#ifdef __STRUCT_PARM_ALIGN__
	  align = (*ptr)->alignment;
	  if (align > __STRUCT_PARM_ALIGN__)
	    align = __STRUCT_PARM_ALIGN__;
	  if (align > 1)
	    next_arg.p = ALIGN (next_arg.p, align);
#endif
	  elt = 0;
#if _CALL_ELF == 2
	  elt = discover_homogeneous_aggregate (*ptr, &elnum);
#endif
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




static ffi_status
ffi_prep_cif_machdep_core (ffi_cif *cif)
{
  
  ffi_type **ptr;
  unsigned bytes;
  unsigned i, fparg_count = 0, intarg_count = 0;
  unsigned flags = cif->flags;
  unsigned struct_copy_size = 0;
  unsigned type = cif->rtype->type;
  unsigned size = cif->rtype->size;

  

  if (cif->abi != FFI_LINUX64)
    {
      
      bytes = (2 + ASM_NEEDS_REGISTERS) * sizeof (int);

      
      bytes += NUM_GPR_ARG_REGISTERS * sizeof (int);
    }
  else
    {
      
#if _CALL_ELF == 2
      
      bytes = (4 + ASM_NEEDS_REGISTERS64) * sizeof (long);

      
      bytes += NUM_GPR_ARG_REGISTERS64 * sizeof (long);
#else
      

      bytes = (6 + ASM_NEEDS_REGISTERS64) * sizeof (long);

      
      bytes += 2 * NUM_GPR_ARG_REGISTERS64 * sizeof (long);
#endif
    }

  















  
  if (cif->abi == FFI_LINUX_SOFT_FLOAT)
    {
      if (type == FFI_TYPE_FLOAT)
	type = FFI_TYPE_UINT32;
      if (type == FFI_TYPE_DOUBLE)
	type = FFI_TYPE_UINT64;
      if (type == FFI_TYPE_LONGDOUBLE)
	type = FFI_TYPE_UINT128;
    }
  else if (cif->abi != FFI_LINUX
	   && cif->abi != FFI_LINUX64)
    {
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
      if (type == FFI_TYPE_LONGDOUBLE)
	type = FFI_TYPE_STRUCT;
#endif
    }

  switch (type)
    {
#ifndef __NO_FPRS__
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
    case FFI_TYPE_LONGDOUBLE:
      flags |= FLAG_RETURNS_128BITS;
      
#endif
    case FFI_TYPE_DOUBLE:
      flags |= FLAG_RETURNS_64BITS;
      
    case FFI_TYPE_FLOAT:
      flags |= FLAG_RETURNS_FP;
      break;
#endif

    case FFI_TYPE_UINT128:
      flags |= FLAG_RETURNS_128BITS;
      
    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
      flags |= FLAG_RETURNS_64BITS;
      break;

    case FFI_TYPE_STRUCT:
      









      if (cif->abi == FFI_SYSV && size <= 8)
	{
	  flags |= FLAG_RETURNS_SMST;
	  break;
	}
#if _CALL_ELF == 2
      if (cif->abi == FFI_LINUX64)
	{
	  unsigned int elt, elnum;
	  elt = discover_homogeneous_aggregate (cif->rtype, &elnum);
	  if (elt)
	    {
	      if (elt == FFI_TYPE_DOUBLE)
		flags |= FLAG_RETURNS_64BITS;
	      flags |= FLAG_RETURNS_FP | FLAG_RETURNS_SMST;
	      break;
	    }
	  if (size <= 16)
	    {
	      flags |= FLAG_RETURNS_SMST;
	      break;
	    }
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

  if (cif->abi != FFI_LINUX64)
    




    for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++)
      {
	unsigned short typenum = (*ptr)->type;

	
	if (cif->abi == FFI_LINUX_SOFT_FLOAT) {
		if (typenum == FFI_TYPE_FLOAT)
			typenum = FFI_TYPE_UINT32;
		if (typenum == FFI_TYPE_DOUBLE)
			typenum = FFI_TYPE_UINT64;
		if (typenum == FFI_TYPE_LONGDOUBLE)
			typenum = FFI_TYPE_UINT128;
	} else if (cif->abi != FFI_LINUX && cif->abi != FFI_LINUX64) {
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
		if (typenum == FFI_TYPE_LONGDOUBLE)
			typenum = FFI_TYPE_STRUCT;
#endif
	}

	switch (typenum) {
#ifndef __NO_FPRS__
	  case FFI_TYPE_FLOAT:
	    fparg_count++;
	    
	    break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	  case FFI_TYPE_LONGDOUBLE:
	    fparg_count++;
	    
#endif
	  case FFI_TYPE_DOUBLE:
	    fparg_count++;
	    

	    if (fparg_count > NUM_FPR_ARG_REGISTERS
		&& intarg_count >= NUM_GPR_ARG_REGISTERS
		&& intarg_count % 2 != 0)
	      intarg_count++;
	    break;
#endif
	  case FFI_TYPE_UINT128:
		




		if (intarg_count >= NUM_GPR_ARG_REGISTERS - 3
				&& intarg_count < NUM_GPR_ARG_REGISTERS)
			intarg_count = NUM_GPR_ARG_REGISTERS;
		intarg_count += 4;
		break;

	  case FFI_TYPE_UINT64:
	  case FFI_TYPE_SINT64:
	    








	    if (intarg_count == NUM_GPR_ARG_REGISTERS-1
		|| intarg_count % 2 != 0)
	      intarg_count++;
	    intarg_count += 2;
	    break;

	  case FFI_TYPE_STRUCT:
	    



	    struct_copy_size += ((*ptr)->size + 15) & ~0xF;
	    

	  case FFI_TYPE_POINTER:
	  case FFI_TYPE_INT:
	  case FFI_TYPE_UINT32:
	  case FFI_TYPE_SINT32:
	  case FFI_TYPE_UINT16:
	  case FFI_TYPE_SINT16:
	  case FFI_TYPE_UINT8:
	  case FFI_TYPE_SINT8:
	    

	    intarg_count++;
	    break;
	  default:
		FFI_ASSERT (0);
	  }
      }
  else
    for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++)
      {
	unsigned int elt, elnum;
#ifdef __STRUCT_PARM_ALIGN__
	unsigned int align;
#endif

	switch ((*ptr)->type)
	  {
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	  case FFI_TYPE_LONGDOUBLE:
	    fparg_count += 2;
	    intarg_count += 2;
	    if (fparg_count > NUM_FPR_ARG_REGISTERS64)
	      flags |= FLAG_ARG_NEEDS_PSAVE;
	    break;
#endif
	  case FFI_TYPE_FLOAT:
	  case FFI_TYPE_DOUBLE:
	    fparg_count++;
	    intarg_count++;
	    if (fparg_count > NUM_FPR_ARG_REGISTERS64)
	      flags |= FLAG_ARG_NEEDS_PSAVE;
	    break;

	  case FFI_TYPE_STRUCT:
#ifdef __STRUCT_PARM_ALIGN__
	    align = (*ptr)->alignment;
	    if (align > __STRUCT_PARM_ALIGN__)
	      align = __STRUCT_PARM_ALIGN__;
	    align = align / 8;
	    if (align > 1)
	      intarg_count = ALIGN (intarg_count, align);
#endif
	    intarg_count += ((*ptr)->size + 7) / 8;
	    elt = 0;
#if _CALL_ELF == 2
	    elt = discover_homogeneous_aggregate (*ptr, &elnum);
#endif
	    if (elt)
	      {
		fparg_count += elnum;
		if (fparg_count > NUM_FPR_ARG_REGISTERS64)
		  flags |= FLAG_ARG_NEEDS_PSAVE;
	      }
	    else
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

#ifndef __NO_FPRS__
  if (fparg_count != 0)
    flags |= FLAG_FP_ARGUMENTS;
#endif
  if (intarg_count > 4)
    flags |= FLAG_4_GPR_ARGUMENTS;
  if (struct_copy_size != 0)
    flags |= FLAG_ARG_NEEDS_COPY;

  if (cif->abi != FFI_LINUX64)
    {
#ifndef __NO_FPRS__
      
      if (fparg_count != 0)
	bytes += NUM_FPR_ARG_REGISTERS * sizeof (double);
#endif

      
      if (intarg_count > NUM_GPR_ARG_REGISTERS)
	bytes += (intarg_count - NUM_GPR_ARG_REGISTERS) * sizeof (int);
#ifndef __NO_FPRS__
      if (fparg_count > NUM_FPR_ARG_REGISTERS)
	bytes += (fparg_count - NUM_FPR_ARG_REGISTERS) * sizeof (double);
#endif
    }
  else
    {
#ifndef __NO_FPRS__
      
      if (fparg_count != 0)
	bytes += NUM_FPR_ARG_REGISTERS64 * sizeof (double);
#endif

      
#if _CALL_ELF == 2
      if ((flags & FLAG_ARG_NEEDS_PSAVE) != 0)
	bytes += intarg_count * sizeof (long);
#else
      if (intarg_count > NUM_GPR_ARG_REGISTERS64)
	bytes += (intarg_count - NUM_GPR_ARG_REGISTERS64) * sizeof (long);
#endif
    }

  
  bytes = (bytes + 15) & ~0xF;

  
  bytes += struct_copy_size;

  cif->flags = flags;
  cif->bytes = bytes;

  return FFI_OK;
}

ffi_status
ffi_prep_cif_machdep (ffi_cif *cif)
{
  cif->nfixedargs = cif->nargs;
  return ffi_prep_cif_machdep_core (cif);
}

ffi_status
ffi_prep_cif_machdep_var (ffi_cif *cif,
			  unsigned int nfixedargs,
			  unsigned int ntotalargs MAYBE_UNUSED)
{
  cif->nfixedargs = nfixedargs;
#if _CALL_ELF == 2
  if (cif->abi == FFI_LINUX64)
    cif->flags |= FLAG_ARG_NEEDS_PSAVE;
#endif
  return ffi_prep_cif_machdep_core (cif);
}

extern void ffi_call_SYSV(extended_cif *, unsigned, unsigned, unsigned *,
			  void (*fn)(void));
extern void FFI_HIDDEN ffi_call_LINUX64(extended_cif *, unsigned long,
					unsigned long, unsigned long *,
					void (*fn)(void));

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

  switch (cif->abi)
    {
#ifndef POWERPC64
# ifndef __NO_FPRS__
    case FFI_SYSV:
    case FFI_GCC_SYSV:
    case FFI_LINUX:
# endif
    case FFI_LINUX_SOFT_FLOAT:
      ffi_call_SYSV (&ecif, -cif->bytes, cif->flags, ecif.rvalue, fn);
      break;
#else
    case FFI_LINUX64:
      ffi_call_LINUX64 (&ecif, -(long) cif->bytes, cif->flags, ecif.rvalue, fn);
      break;
#endif
    default:
      FFI_ASSERT (0);
      break;
    }

  
  if (rvalue && ecif.rvalue == smst_buffer)
    {
      unsigned int rsize = cif->rtype->size;
#ifndef __LITTLE_ENDIAN__
      

      if (cif->abi == FFI_SYSV && rsize <= 4)
	memcpy (rvalue, (char *) smst_buffer + 4 - rsize, rsize);
      


      else if (rsize <= 8)
	memcpy (rvalue, (char *) smst_buffer + 8 - rsize, rsize);
      else
#endif
	memcpy (rvalue, smst_buffer, rsize);
    }
}


#if !defined POWERPC64 || _CALL_ELF == 2
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
ffi_prep_closure_loc (ffi_closure *closure,
		      ffi_cif *cif,
		      void (*fun) (ffi_cif *, void *, void **, void *),
		      void *user_data,
		      void *codeloc)
{
#ifdef POWERPC64
# if _CALL_ELF == 2
  unsigned int *tramp = (unsigned int *) &closure->tramp[0];

  if (cif->abi != FFI_LINUX64)
    return FFI_BAD_ABI;

  tramp[0] = 0xe96c0018;	
  tramp[1] = 0xe98c0010;	
  tramp[2] = 0x7d8903a6;	
  tramp[3] = 0x4e800420;	
				
				
  *(void **) &tramp[4] = (void *) ffi_closure_LINUX64;
  *(void **) &tramp[6] = codeloc;
  flush_icache ((char *)tramp, (char *)codeloc, FFI_TRAMPOLINE_SIZE);
# else
  void **tramp = (void **) &closure->tramp[0];

  if (cif->abi != FFI_LINUX64)
    return FFI_BAD_ABI;
  
  memcpy (tramp, (char *) ffi_closure_LINUX64, 16);
  tramp[2] = codeloc;
# endif
#else
  unsigned int *tramp;

  if (! (cif->abi == FFI_GCC_SYSV 
	 || cif->abi == FFI_SYSV
	 || cif->abi == FFI_LINUX
	 || cif->abi == FFI_LINUX_SOFT_FLOAT))
    return FFI_BAD_ABI;

  tramp = (unsigned int *) &closure->tramp[0];
  tramp[0] = 0x7c0802a6;  
  tramp[1] = 0x4800000d;  
  tramp[4] = 0x7d6802a6;  
  tramp[5] = 0x7c0803a6;  
  tramp[6] = 0x800b0000;  
  tramp[7] = 0x816b0004;  
  tramp[8] = 0x7c0903a6;  
  tramp[9] = 0x4e800420;  
  *(void **) &tramp[2] = (void *) ffi_closure_SYSV; 
  *(void **) &tramp[3] = codeloc;                   

  
  flush_icache ((char *)tramp, (char *)codeloc, FFI_TRAMPOLINE_SIZE);
#endif

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return FFI_OK;
}

typedef union
{
  float f;
  double d;
} ffi_dblfl;

int ffi_closure_helper_SYSV (ffi_closure *, void *, unsigned long *,
			     ffi_dblfl *, unsigned long *);









int
ffi_closure_helper_SYSV (ffi_closure *closure, void *rvalue,
			 unsigned long *pgr, ffi_dblfl *pfr,
			 unsigned long *pst)
{
  
  
  
  

  void **          avalue;
  ffi_type **      arg_types;
  long             i, avn;
#ifndef __NO_FPRS__
  long             nf = 0;   
#endif
  long             ng = 0;   

  ffi_cif *cif = closure->cif;
  unsigned       size     = cif->rtype->size;
  unsigned short rtypenum = cif->rtype->type;

  avalue = alloca (cif->nargs * sizeof (void *));

  
  if (cif->abi == FFI_LINUX_SOFT_FLOAT) {
	if (rtypenum == FFI_TYPE_FLOAT)
		rtypenum = FFI_TYPE_UINT32;
	if (rtypenum == FFI_TYPE_DOUBLE)
		rtypenum = FFI_TYPE_UINT64;
	if (rtypenum == FFI_TYPE_LONGDOUBLE)
		rtypenum = FFI_TYPE_UINT128;
  } else if (cif->abi != FFI_LINUX && cif->abi != FFI_LINUX64) {
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	if (rtypenum == FFI_TYPE_LONGDOUBLE)
		rtypenum = FFI_TYPE_STRUCT;
#endif
  }


  



  if (rtypenum == FFI_TYPE_STRUCT && ((cif->abi != FFI_SYSV) || (size > 8))) {
      rvalue = (void *) *pgr;
      ng++;
      pgr++;
    }

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;

  
  while (i < avn) {
      unsigned short typenum = arg_types[i]->type;

      
      if (cif->abi == FFI_LINUX_SOFT_FLOAT) {
		if (typenum == FFI_TYPE_FLOAT)
			typenum = FFI_TYPE_UINT32;
		if (typenum == FFI_TYPE_DOUBLE)
			typenum = FFI_TYPE_UINT64;
		if (typenum == FFI_TYPE_LONGDOUBLE)
			typenum = FFI_TYPE_UINT128;
      } else if (cif->abi != FFI_LINUX && cif->abi != FFI_LINUX64) {
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
		if (typenum == FFI_TYPE_LONGDOUBLE)
			typenum = FFI_TYPE_STRUCT;
#endif
      }

      switch (typenum) {
#ifndef __NO_FPRS__
	case FFI_TYPE_FLOAT:
	  




	  

	  if (nf < 8)
	    {
	      double temp = pfr->d;
	      pfr->f = (float) temp;
	      avalue[i] = pfr;
	      nf++;
	      pfr++;
	    }
	  else
	    {
	      




	      avalue[i] = pst;
	      pst += 1;
	    }
	  break;

	case FFI_TYPE_DOUBLE:
	  
	  

	  if (nf < 8)
	    {
	      avalue[i] = pfr;
	      nf++;
	      pfr++;
	    }
	  else
	    {
	      if (((long) pst) & 4)
		pst++;
	      avalue[i] = pst;
	      pst += 2;
	    }
	  break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
	  if (nf < 7)
	    {
	      avalue[i] = pfr;
	      pfr += 2;
	      nf += 2;
	    }
	  else
	    {
	      if (((long) pst) & 4)
		pst++;
	      avalue[i] = pst;
	      pst += 4;
	      nf = 8;
	    }
	  break;
#endif
#endif 

	case FFI_TYPE_UINT128:
		



		if (ng < 5) {
			avalue[i] = pgr;
			pgr += 4;
			ng += 4;
		} else {
			avalue[i] = pst;
			pst += 4;
			ng = 8+4;
		}
		break;

	case FFI_TYPE_SINT8:
	case FFI_TYPE_UINT8:
#ifndef __LITTLE_ENDIAN__
	  
	  if (ng < 8)
	    {
	      avalue[i] = (char *) pgr + 3;
	      ng++;
	      pgr++;
	    }
	  else
	    {
	      avalue[i] = (char *) pst + 3;
	      pst++;
	    }
	  break;
#endif

	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT16:
#ifndef __LITTLE_ENDIAN__
	  
	  if (ng < 8)
	    {
	      avalue[i] = (char *) pgr + 2;
	      ng++;
	      pgr++;
	    }
	  else
	    {
	      avalue[i] = (char *) pst + 2;
	      pst++;
	    }
	  break;
#endif

	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT32:
	case FFI_TYPE_POINTER:
	  
	  if (ng < 8)
	    {
	      avalue[i] = pgr;
	      ng++;
	      pgr++;
	    }
	  else
	    {
	      avalue[i] = pst;
	      pst++;
	    }
	  break;

	case FFI_TYPE_STRUCT:
	  

	  if (ng < 8)
	    {
	      avalue[i] = (void *) *pgr;
	      ng++;
	      pgr++;
	    }
	  else
	    {
	      avalue[i] = (void *) *pst;
	      pst++;
	    }
	  break;

	case FFI_TYPE_SINT64:
	case FFI_TYPE_UINT64:
	  








	  if (ng < 7)
	    {
	      if (ng & 0x01)
		{
		  
		  ng++;
		  pgr++;
		}
	      avalue[i] = pgr;
	      ng += 2;
	      pgr += 2;
	    }
	  else
	    {
	      if (((long) pst) & 4)
		pst++;
	      avalue[i] = pst;
	      pst += 2;
	      ng = 8;
	    }
	  break;

	default:
		FFI_ASSERT (0);
	}

      i++;
    }


  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  







  if (cif->abi == FFI_SYSV && rtypenum == FFI_TYPE_STRUCT && size <= 8)
    return (FFI_SYSV_TYPE_SMALL_STRUCT - 1) + size;
  return rtypenum;
}

int FFI_HIDDEN ffi_closure_helper_LINUX64 (ffi_closure *, void *,
					   unsigned long *, ffi_dblfl *);

int FFI_HIDDEN
ffi_closure_helper_LINUX64 (ffi_closure *closure, void *rvalue,
			    unsigned long *pst, ffi_dblfl *pfr)
{
  
  

  

  void **avalue;
  ffi_type **arg_types;
  unsigned long i, avn, nfixedargs;
  ffi_cif *cif;
  ffi_dblfl *end_pfr = pfr + NUM_FPR_ARG_REGISTERS64;
#ifdef __STRUCT_PARM_ALIGN__
  unsigned long align;
#endif

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
#ifdef __STRUCT_PARM_ALIGN__
	  align = arg_types[i]->alignment;
	  if (align > __STRUCT_PARM_ALIGN__)
	    align = __STRUCT_PARM_ALIGN__;
	  if (align > 1)
	    pst = (unsigned long *) ALIGN ((size_t) pst, align);
#endif
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

	case FFI_TYPE_FLOAT:
	  




	  

	  if (pfr < end_pfr && i < nfixedargs)
	    {
	      double temp = pfr->d;
	      pfr->f = (float) temp;
	      avalue[i] = pfr;
	      pfr++;
	    }
	  else
	    avalue[i] = pst;
	  pst++;
	  break;

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

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
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
#endif

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

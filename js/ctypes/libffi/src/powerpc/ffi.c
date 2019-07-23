


























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>
#include <stdio.h>


extern void ffi_closure_SYSV (void);
extern void FFI_HIDDEN ffi_closure_LINUX64 (void);

enum {
  
  FLAG_RETURNS_SMST	= 1 << (31-31), 
  FLAG_RETURNS_NOTHING  = 1 << (31-30), 
  FLAG_RETURNS_FP       = 1 << (31-29),
  FLAG_RETURNS_64BITS   = 1 << (31-28),

  FLAG_RETURNS_128BITS  = 1 << (31-27), 

  FLAG_SYSV_SMST_R4     = 1 << (31-16), 

  FLAG_SYSV_SMST_R3     = 1 << (31-15), 

  FLAG_ARG_NEEDS_COPY   = 1 << (31- 7),
  FLAG_FP_ARGUMENTS     = 1 << (31- 6), 
  FLAG_4_GPR_ARGUMENTS  = 1 << (31- 5),
  FLAG_RETVAL_REFERENCE = 1 << (31- 4)
};


unsigned int NUM_GPR_ARG_REGISTERS = 8;
#ifndef __NO_FPRS__
unsigned int NUM_FPR_ARG_REGISTERS = 8;
#else
unsigned int NUM_FPR_ARG_REGISTERS = 0;
#endif

enum { ASM_NEEDS_REGISTERS = 4 };



























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

  

  valp fpr_base;
  int fparg_count;

  

  valp copy_space;

  
  valp next_arg;

  int i, ii MAYBE_UNUSED;
  ffi_type **ptr;
  double double_tmp;
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

  if (ecif->cif->abi == FFI_LINUX_SOFT_FLOAT)
    NUM_FPR_ARG_REGISTERS = 0;

  stacktop.c = (char *) stack + bytes;
  gpr_base.u = stacktop.u - ASM_NEEDS_REGISTERS - NUM_GPR_ARG_REGISTERS;
  intarg_count = 0;
  fpr_base.d = gpr_base.d - NUM_FPR_ARG_REGISTERS;
  fparg_count = 0;
  copy_space.c = ((flags & FLAG_FP_ARGUMENTS) ? fpr_base.c : gpr_base.c);
  next_arg.u = stack + 2;

  
  FFI_ASSERT (((unsigned) (char *) stack & 0xF) == 0);
  FFI_ASSERT (((unsigned) copy_space.c & 0xF) == 0);
  FFI_ASSERT (((unsigned) stacktop.c & 0xF) == 0);
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
      switch ((*ptr)->type)
	{
	case FFI_TYPE_FLOAT:
	  
	  if (ecif->cif->abi == FFI_LINUX_SOFT_FLOAT)
	    goto soft_float_prep;
	  double_tmp = **p_argv.f;
	  if (fparg_count >= NUM_FPR_ARG_REGISTERS)
	    {
	      *next_arg.f = (float) double_tmp;
	      next_arg.u += 1;
	    }
	  else
	    *fpr_base.d++ = double_tmp;
	  fparg_count++;
	  FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;

	case FFI_TYPE_DOUBLE:
	  
	  if (ecif->cif->abi == FFI_LINUX_SOFT_FLOAT)
	    goto soft_double_prep;
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
	  if ((ecif->cif->abi != FFI_LINUX)
		&& (ecif->cif->abi != FFI_LINUX_SOFT_FLOAT))
	    goto do_struct;
	  




	  if (ecif->cif->abi == FFI_LINUX_SOFT_FLOAT)
	    {
	      unsigned int int_tmp = (*p_argv.ui)[0];
	      if (intarg_count >= NUM_GPR_ARG_REGISTERS - 3)
		{
		  if (intarg_count < NUM_GPR_ARG_REGISTERS)
		    intarg_count += NUM_GPR_ARG_REGISTERS - intarg_count;
		  *next_arg.u = int_tmp;
		  next_arg.u++;
		  for (ii = 1; ii < 4; ii++)
		    {
		      int_tmp = (*p_argv.ui)[ii];
		      *next_arg.u = int_tmp;
		      next_arg.u++;
		    }
		}
	      else
		{
		  *gpr_base.u++ = int_tmp;
		  for (ii = 1; ii < 4; ii++)
		    {
		      int_tmp = (*p_argv.ui)[ii];
		      *gpr_base.u++ = int_tmp;
		    }
		}
	      intarg_count +=4;
	    }
	  else
	    {
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
	    }
	  break;
#endif

	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	soft_double_prep:
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
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	do_struct:
#endif
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
	soft_float_prep:

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
  FFI_ASSERT (fpr_base.u
	      <= stacktop.u - ASM_NEEDS_REGISTERS - NUM_GPR_ARG_REGISTERS);
  FFI_ASSERT (flags & FLAG_4_GPR_ARGUMENTS || intarg_count <= 4);
}


enum {
  NUM_GPR_ARG_REGISTERS64 = 8,
  NUM_FPR_ARG_REGISTERS64 = 13
};
enum { ASM_NEEDS_REGISTERS64 = 4 };



































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
  } valp;

  
  valp stacktop;

  

  valp gpr_base;
  valp gpr_end;
  valp rest;
  valp next_arg;

  

  valp fpr_base;
  int fparg_count;

  int i, words;
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

  stacktop.c = (char *) stack + bytes;
  gpr_base.ul = stacktop.ul - ASM_NEEDS_REGISTERS64 - NUM_GPR_ARG_REGISTERS64;
  gpr_end.ul = gpr_base.ul + NUM_GPR_ARG_REGISTERS64;
  rest.ul = stack + 6 + NUM_GPR_ARG_REGISTERS64;
  fpr_base.d = gpr_base.d - NUM_FPR_ARG_REGISTERS64;
  fparg_count = 0;
  next_arg.ul = gpr_base.ul;

  
  FFI_ASSERT (((unsigned long) (char *) stack & 0xF) == 0);
  FFI_ASSERT (((unsigned long) stacktop.c & 0xF) == 0);
  FFI_ASSERT ((bytes & 0xF) == 0);

  
  if (flags & FLAG_RETVAL_REFERENCE)
    *next_arg.ul++ = (unsigned long) (char *) ecif->rvalue;

  
  p_argv.v = ecif->avalue;
  for (ptr = ecif->cif->arg_types, i = ecif->cif->nargs;
       i > 0;
       i--, ptr++, p_argv.v++)
    {
      switch ((*ptr)->type)
	{
	case FFI_TYPE_FLOAT:
	  double_tmp = **p_argv.f;
	  *next_arg.f = (float) double_tmp;
	  if (++next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  if (fparg_count < NUM_FPR_ARG_REGISTERS64)
	    *fpr_base.d++ = double_tmp;
	  fparg_count++;
	  FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;

	case FFI_TYPE_DOUBLE:
	  double_tmp = **p_argv.d;
	  *next_arg.d = double_tmp;
	  if (++next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  if (fparg_count < NUM_FPR_ARG_REGISTERS64)
	    *fpr_base.d++ = double_tmp;
	  fparg_count++;
	  FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
	  double_tmp = (*p_argv.d)[0];
	  *next_arg.d = double_tmp;
	  if (++next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  if (fparg_count < NUM_FPR_ARG_REGISTERS64)
	    *fpr_base.d++ = double_tmp;
	  fparg_count++;
	  double_tmp = (*p_argv.d)[1];
	  *next_arg.d = double_tmp;
	  if (++next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  if (fparg_count < NUM_FPR_ARG_REGISTERS64)
	    *fpr_base.d++ = double_tmp;
	  fparg_count++;
	  FFI_ASSERT (__LDBL_MANT_DIG__ == 106);
	  FFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;
#endif

	case FFI_TYPE_STRUCT:
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

	      

	      if ((*ptr)->size < 8)
		where += 8 - (*ptr)->size;

	      memcpy (where, *p_argv.c, (*ptr)->size);
	      next_arg.ul += words;
	      if (next_arg.ul == gpr_end.ul)
		next_arg.ul = rest.ul;
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




ffi_status
ffi_prep_cif_machdep (ffi_cif *cif)
{
  
  int i;
  ffi_type **ptr;
  unsigned bytes;
  int fparg_count = 0, intarg_count = 0;
  unsigned flags = 0;
  unsigned struct_copy_size = 0;
  unsigned type = cif->rtype->type;
  unsigned size = cif->rtype->size;

  if (cif->abi == FFI_LINUX_SOFT_FLOAT)
    NUM_FPR_ARG_REGISTERS = 0;

  if (cif->abi != FFI_LINUX64)
    {
      


      
      bytes = (2 + ASM_NEEDS_REGISTERS) * sizeof (int);

      
      bytes += NUM_GPR_ARG_REGISTERS * sizeof (int);
    }
  else
    {
      

      

      bytes = (6 + ASM_NEEDS_REGISTERS64) * sizeof (long);

      
      bytes += 2 * NUM_GPR_ARG_REGISTERS64 * sizeof (long);
    }

  















  switch (type)
    {
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
    case FFI_TYPE_LONGDOUBLE:
      if (cif->abi != FFI_LINUX && cif->abi != FFI_LINUX64
	&& cif->abi != FFI_LINUX_SOFT_FLOAT)
	goto byref;
      flags |= FLAG_RETURNS_128BITS;
      
#endif
    case FFI_TYPE_DOUBLE:
      flags |= FLAG_RETURNS_64BITS;
      
    case FFI_TYPE_FLOAT:
      
      if (cif->abi != FFI_LINUX_SOFT_FLOAT)
	flags |= FLAG_RETURNS_FP;
      break;

    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
      flags |= FLAG_RETURNS_64BITS;
      break;

    case FFI_TYPE_STRUCT:
      if (cif->abi == FFI_SYSV)
	{
	  



	  
	  if (size <= 8)
	    {
	      flags |= FLAG_RETURNS_SMST;
	      


	      if (size <= 4)
		{
		  flags |= FLAG_SYSV_SMST_R3;
		  flags |= 8 * (4 - size) << 4;
		  break;
		}
	      
	      if  (size <= 8)
		{
		  flags |= FLAG_SYSV_SMST_R4;
		  flags |= 8 * (8 - size) << 4;
		  break;
		}
	    }
	}
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
    byref:
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
	switch ((*ptr)->type)
	  {
	  case FFI_TYPE_FLOAT:
	    
	    if (cif->abi == FFI_LINUX_SOFT_FLOAT)
	      goto soft_float_cif;
	    fparg_count++;
	    
	    break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	  case FFI_TYPE_LONGDOUBLE:
	    if (cif->abi != FFI_LINUX && cif->abi != FFI_LINUX_SOFT_FLOAT)
	      goto do_struct;
	    if (cif->abi == FFI_LINUX_SOFT_FLOAT)
	      {
		if (intarg_count >= NUM_GPR_ARG_REGISTERS - 3
		  || intarg_count < NUM_GPR_ARG_REGISTERS)
		  


		  intarg_count += NUM_GPR_ARG_REGISTERS - intarg_count;
		intarg_count += 4;
		break;
	      }
	    else
	      fparg_count++;
	    
#endif
	  case FFI_TYPE_DOUBLE:
	    
	    if (cif->abi == FFI_LINUX_SOFT_FLOAT)
	      goto soft_double_cif;
	    fparg_count++;
	    

	    if (fparg_count > NUM_FPR_ARG_REGISTERS
		&& intarg_count >= NUM_GPR_ARG_REGISTERS
		&& intarg_count % 2 != 0)
	      intarg_count++;
	    break;

	  case FFI_TYPE_UINT64:
	  case FFI_TYPE_SINT64:
	  soft_double_cif:
	    








	    if (intarg_count == NUM_GPR_ARG_REGISTERS-1
		|| intarg_count % 2 != 0)
	      intarg_count++;
	    intarg_count += 2;
	    break;

	  case FFI_TYPE_STRUCT:
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	  do_struct:
#endif
	    



	    struct_copy_size += ((*ptr)->size + 15) & ~0xF;
	    

	  default:
	  soft_float_cif:
	    

	    intarg_count++;
	    break;
	  }
      }
  else
    for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++)
      {
	switch ((*ptr)->type)
	  {
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	  case FFI_TYPE_LONGDOUBLE:
	    if (cif->abi == FFI_LINUX_SOFT_FLOAT)
	      intarg_count += 4;
	    else
	      {
		fparg_count += 2;
		intarg_count += 2;
	      }
	    break;
#endif
	  case FFI_TYPE_FLOAT:
	  case FFI_TYPE_DOUBLE:
	    fparg_count++;
	    intarg_count++;
	    break;

	  case FFI_TYPE_STRUCT:
	    intarg_count += ((*ptr)->size + 7) / 8;
	    break;

	  default:
	    

	    intarg_count++;
	    break;
	  }
      }

  if (fparg_count != 0)
    flags |= FLAG_FP_ARGUMENTS;
  if (intarg_count > 4)
    flags |= FLAG_4_GPR_ARGUMENTS;
  if (struct_copy_size != 0)
    flags |= FLAG_ARG_NEEDS_COPY;

  if (cif->abi != FFI_LINUX64)
    {
      
      if (fparg_count != 0)
	bytes += NUM_FPR_ARG_REGISTERS * sizeof (double);

      
      if (intarg_count > NUM_GPR_ARG_REGISTERS)
	bytes += (intarg_count - NUM_GPR_ARG_REGISTERS) * sizeof (int);
      if (fparg_count > NUM_FPR_ARG_REGISTERS)
	bytes += (fparg_count - NUM_FPR_ARG_REGISTERS) * sizeof (double);
    }
  else
    {
      
      if (fparg_count != 0)
	bytes += NUM_FPR_ARG_REGISTERS64 * sizeof (double);

      
      if (intarg_count > NUM_GPR_ARG_REGISTERS64)
	bytes += (intarg_count - NUM_GPR_ARG_REGISTERS64) * sizeof (long);
    }

  
  bytes = (bytes + 15) & ~0xF;

  
  bytes += struct_copy_size;

  cif->flags = flags;
  cif->bytes = bytes;

  return FFI_OK;
}

extern void ffi_call_SYSV(extended_cif *, unsigned, unsigned, unsigned *,
			  void (*fn)(void));
extern void FFI_HIDDEN ffi_call_LINUX64(extended_cif *, unsigned long,
					unsigned long, unsigned long *,
					void (*fn)(void));

void
ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  
  

  if ((rvalue == NULL) && (cif->rtype->type == FFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;


  switch (cif->abi)
    {
#ifndef POWERPC64
    case FFI_SYSV:
    case FFI_GCC_SYSV:
    case FFI_LINUX:
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
}


#ifndef POWERPC64
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
  void **tramp = (void **) &closure->tramp[0];

  FFI_ASSERT (cif->abi == FFI_LINUX64);
  
  memcpy (tramp, (char *) ffi_closure_LINUX64, 16);
  tramp[2] = codeloc;
#else
  unsigned int *tramp;

  FFI_ASSERT (cif->abi == FFI_GCC_SYSV || cif->abi == FFI_SYSV);

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
  long             nf;   
  long             ng;   
  ffi_cif *        cif;
  double           temp;
  unsigned         size;

  cif = closure->cif;
  avalue = alloca (cif->nargs * sizeof (void *));
  size = cif->rtype->size;

  nf = 0;
  ng = 0;

  




  if ((cif->rtype->type == FFI_TYPE_STRUCT
       && !((cif->abi == FFI_SYSV) && (size <= 8)))
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
      || (cif->rtype->type == FFI_TYPE_LONGDOUBLE
	  && cif->abi != FFI_LINUX && cif->abi != FFI_LINUX_SOFT_FLOAT)
#endif
      )
    {
      rvalue = (void *) *pgr;
      ng++;
      pgr++;
    }

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;

  
  while (i < avn)
    {
      switch (arg_types[i]->type)
	{
	case FFI_TYPE_SINT8:
	case FFI_TYPE_UINT8:
	  
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

	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT16:
	  
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

	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT32:
	case FFI_TYPE_POINTER:
	soft_float_closure:
	  
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
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	do_struct:
#endif
	  

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
	soft_double_closure:
	  








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
	    }
	  break;

	case FFI_TYPE_FLOAT:
	  
	  if (cif->abi == FFI_LINUX_SOFT_FLOAT)
	    goto soft_float_closure;
	  




	  

	  if (nf < 8)
	    {
	      temp = pfr->d;
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
	  
	  if (cif->abi == FFI_LINUX_SOFT_FLOAT)
	    goto soft_double_closure;
	  
	  

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
	  if (cif->abi != FFI_LINUX && cif->abi != FFI_LINUX_SOFT_FLOAT)
	    goto do_struct;
	  if (cif->abi == FFI_LINUX_SOFT_FLOAT)
	    { 

	      if (ng < 5)
		{
		  avalue[i] = pgr;
		  pgr += 4;
		  ng += 4;
		}
	      else
		{
		  avalue[i] = pst;
		  pst += 4;
		}
	      break;
	    }
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

	default:
	  FFI_ASSERT (0);
	}

      i++;
    }


  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  







  if (cif->abi == FFI_SYSV && cif->rtype->type == FFI_TYPE_STRUCT
      && size <= 8)
    return (FFI_SYSV_TYPE_SMALL_STRUCT - 1) + size;
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
  else if (cif->rtype->type == FFI_TYPE_LONGDOUBLE
	   && cif->abi != FFI_LINUX && cif->abi != FFI_LINUX_SOFT_FLOAT)
    return FFI_TYPE_STRUCT;
#endif
  

  if (cif->abi == FFI_LINUX_SOFT_FLOAT)
    {
      switch (cif->rtype->type)
	{
	case FFI_TYPE_FLOAT:
	  return FFI_TYPE_UINT32;
	  break;
	case FFI_TYPE_DOUBLE:
	  return FFI_TYPE_UINT64;
	  break;
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
	  return FFI_TYPE_UINT128;
	  break;
#endif
	default:
	  return cif->rtype->type;
	}
    }
  else
    {
      return cif->rtype->type;
    }
}

int FFI_HIDDEN ffi_closure_helper_LINUX64 (ffi_closure *, void *,
					   unsigned long *, ffi_dblfl *);

int FFI_HIDDEN
ffi_closure_helper_LINUX64 (ffi_closure *closure, void *rvalue,
			    unsigned long *pst, ffi_dblfl *pfr)
{
  
  

  

  void **avalue;
  ffi_type **arg_types;
  long i, avn;
  ffi_cif *cif;
  ffi_dblfl *end_pfr = pfr + NUM_FPR_ARG_REGISTERS64;

  cif = closure->cif;
  avalue = alloca (cif->nargs * sizeof (void *));

  

  if (cif->rtype->type == FFI_TYPE_STRUCT)
    {
      rvalue = (void *) *pst;
      pst++;
    }

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;

  
  while (i < avn)
    {
      switch (arg_types[i]->type)
	{
	case FFI_TYPE_SINT8:
	case FFI_TYPE_UINT8:
	  avalue[i] = (char *) pst + 7;
	  pst++;
	  break;

	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT16:
	  avalue[i] = (char *) pst + 6;
	  pst++;
	  break;

	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT32:
	  avalue[i] = (char *) pst + 4;
	  pst++;
	  break;

	case FFI_TYPE_SINT64:
	case FFI_TYPE_UINT64:
	case FFI_TYPE_POINTER:
	  avalue[i] = pst;
	  pst++;
	  break;

	case FFI_TYPE_STRUCT:
	  

	  if (arg_types[i]->size < 8)
	    avalue[i] = (char *) pst + 8 - arg_types[i]->size;
	  else
	    avalue[i] = pst;
	  pst += (arg_types[i]->size + 7) / 8;
	  break;

	case FFI_TYPE_FLOAT:
	  




	  

	  if (pfr < end_pfr)
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
	  
	  

	  if (pfr < end_pfr)
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
	  if (pfr + 1 < end_pfr)
	    {
	      avalue[i] = pfr;
	      pfr += 2;
	    }
	  else
	    {
	      if (pfr < end_pfr)
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

  
  return cif->rtype->type;
}

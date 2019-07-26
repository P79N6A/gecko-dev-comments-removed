





























#include "ffi.h"

#ifndef POWERPC64
#include "ffi_common.h"
#include "ffi_powerpc.h"



#define ASM_NEEDS_REGISTERS 4
#define NUM_GPR_ARG_REGISTERS 8
#define NUM_FPR_ARG_REGISTERS 8


#if HAVE_LONG_DOUBLE_VARIANT && FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE

void FFI_HIDDEN
ffi_prep_types_sysv (ffi_abi abi)
{
  if ((abi & (FFI_SYSV | FFI_SYSV_LONG_DOUBLE_128)) == FFI_SYSV)
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


static int
translate_float (int abi, int type)
{
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
  if (type == FFI_TYPE_LONGDOUBLE
      && (abi & FFI_SYSV_LONG_DOUBLE_128) == 0)
    type = FFI_TYPE_DOUBLE;
#endif
  if ((abi & FFI_SYSV_SOFT_FLOAT) != 0)
    {
      if (type == FFI_TYPE_FLOAT)
	type = FFI_TYPE_UINT32;
      else if (type == FFI_TYPE_DOUBLE)
	type = FFI_TYPE_UINT64;
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
      else if (type == FFI_TYPE_LONGDOUBLE)
	type = FFI_TYPE_UINT128;
    }
  else if ((abi & FFI_SYSV_IBM_LONG_DOUBLE) == 0)
    {
      if (type == FFI_TYPE_LONGDOUBLE)
	type = FFI_TYPE_STRUCT;
#endif
    }
  return type;
}


static ffi_status
ffi_prep_cif_sysv_core (ffi_cif *cif)
{
  ffi_type **ptr;
  unsigned bytes;
  unsigned i, fparg_count = 0, intarg_count = 0;
  unsigned flags = cif->flags;
  unsigned struct_copy_size = 0;
  unsigned type = cif->rtype->type;
  unsigned size = cif->rtype->size;

  


  
  bytes = (2 + ASM_NEEDS_REGISTERS) * sizeof (int);

  
  bytes += NUM_GPR_ARG_REGISTERS * sizeof (int);

  










  type = translate_float (cif->abi, type);

  switch (type)
    {
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
    case FFI_TYPE_LONGDOUBLE:
      flags |= FLAG_RETURNS_128BITS;
      
#endif
    case FFI_TYPE_DOUBLE:
      flags |= FLAG_RETURNS_64BITS;
      
    case FFI_TYPE_FLOAT:
      flags |= FLAG_RETURNS_FP;
#ifdef __NO_FPRS__
      return FFI_BAD_ABI;
#endif
      break;

    case FFI_TYPE_UINT128:
      flags |= FLAG_RETURNS_128BITS;
      
    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
      flags |= FLAG_RETURNS_64BITS;
      break;

    case FFI_TYPE_STRUCT:
      


      if ((cif->abi & FFI_SYSV_STRUCT_RET) != 0 && size <= 8)
	{
	  flags |= FLAG_RETURNS_SMST;
	  break;
	}
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
      unsigned short typenum = (*ptr)->type;

      typenum = translate_float (cif->abi, typenum);

      switch (typenum)
	{
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
#ifdef __NO_FPRS__
	  return FFI_BAD_ABI;
#endif
	  break;

	case FFI_TYPE_FLOAT:
	  fparg_count++;
#ifdef __NO_FPRS__
	  return FFI_BAD_ABI;
#endif
	  break;

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

  if (fparg_count != 0)
    flags |= FLAG_FP_ARGUMENTS;
  if (intarg_count > 4)
    flags |= FLAG_4_GPR_ARGUMENTS;
  if (struct_copy_size != 0)
    flags |= FLAG_ARG_NEEDS_COPY;

  
  if (fparg_count != 0)
    bytes += NUM_FPR_ARG_REGISTERS * sizeof (double);

  
  if (intarg_count > NUM_GPR_ARG_REGISTERS)
    bytes += (intarg_count - NUM_GPR_ARG_REGISTERS) * sizeof (int);
  if (fparg_count > NUM_FPR_ARG_REGISTERS)
    bytes += (fparg_count - NUM_FPR_ARG_REGISTERS) * sizeof (double);

  
  bytes = (bytes + 15) & ~0xF;

  
  bytes += struct_copy_size;

  cif->flags = flags;
  cif->bytes = bytes;

  return FFI_OK;
}

ffi_status FFI_HIDDEN
ffi_prep_cif_sysv (ffi_cif *cif)
{
  if ((cif->abi & FFI_SYSV) == 0)
    {
      
      cif->flags |= FLAG_COMPAT;
      switch (cif->abi)
	{
	default:
	  return FFI_BAD_ABI;

	case FFI_COMPAT_SYSV:
	  cif->abi = FFI_SYSV | FFI_SYSV_STRUCT_RET | FFI_SYSV_LONG_DOUBLE_128;
	  break;

	case FFI_COMPAT_GCC_SYSV:
	  cif->abi = FFI_SYSV | FFI_SYSV_LONG_DOUBLE_128;
	  break;

	case FFI_COMPAT_LINUX:
	  cif->abi = (FFI_SYSV | FFI_SYSV_IBM_LONG_DOUBLE
		      | FFI_SYSV_LONG_DOUBLE_128);
	  break;

	case FFI_COMPAT_LINUX_SOFT_FLOAT:
	  cif->abi = (FFI_SYSV | FFI_SYSV_SOFT_FLOAT | FFI_SYSV_IBM_LONG_DOUBLE
		      | FFI_SYSV_LONG_DOUBLE_128);
	  break;
	}
    }
  return ffi_prep_cif_sysv_core (cif);
}



























void FFI_HIDDEN
ffi_prep_args_SYSV (extended_cif *ecif, unsigned *const stack)
{
  const unsigned bytes = ecif->cif->bytes;
  const unsigned flags = ecif->cif->flags;

  typedef union
  {
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
  union
  {
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
      unsigned int typenum = (*ptr)->type;

      typenum = translate_float (ecif->cif->abi, typenum);

      
      switch (typenum)
	{
#ifndef __NO_FPRS__
# if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
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
# endif
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
#endif 

	case FFI_TYPE_UINT128:
	  



	  {
	    unsigned int int_tmp;
	    unsigned int ii;
	    if (intarg_count >= NUM_GPR_ARG_REGISTERS - 3)
	      {
		if (intarg_count < NUM_GPR_ARG_REGISTERS)
		  intarg_count = NUM_GPR_ARG_REGISTERS;
		for (ii = 0; ii < 4; ii++)
		  {
		    int_tmp = (*p_argv.ui)[ii];
		    *next_arg.u++ = int_tmp;
		  }
	      }
	    else
	      {
		for (ii = 0; ii < 4; ii++)
		  {
		    int_tmp = (*p_argv.ui)[ii];
		    *gpr_base.u++ = int_tmp;
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

ffi_status FFI_HIDDEN
ffi_prep_closure_loc_sysv (ffi_closure *closure,
			   ffi_cif *cif,
			   void (*fun) (ffi_cif *, void *, void **, void *),
			   void *user_data,
			   void *codeloc)
{
  unsigned int *tramp;

  if (cif->abi < FFI_SYSV || cif->abi >= FFI_LAST_ABI)
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

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return FFI_OK;
}








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

  
  rtypenum = translate_float (cif->abi, rtypenum);

  



  if (rtypenum == FFI_TYPE_STRUCT
      && !((cif->abi & FFI_SYSV_STRUCT_RET) != 0 && size <= 8))
    {
      rvalue = (void *) *pgr;
      ng++;
      pgr++;
    }

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;

  
  while (i < avn) {
    unsigned short typenum = arg_types[i]->type;

    
    typenum = translate_float (cif->abi, typenum);

    switch (typenum)
      {
#ifndef __NO_FPRS__
      case FFI_TYPE_FLOAT:
	


	if (nf < NUM_FPR_ARG_REGISTERS)
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
	if (nf < NUM_FPR_ARG_REGISTERS)
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

# if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
      case FFI_TYPE_LONGDOUBLE:
	if (nf < NUM_FPR_ARG_REGISTERS - 1)
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
# endif
#endif

      case FFI_TYPE_UINT128:
	

	if (ng < NUM_GPR_ARG_REGISTERS - 3)
	  {
	    avalue[i] = pgr;
	    pgr += 4;
	    ng += 4;
	  }
	else
	  {
	    avalue[i] = pst;
	    pst += 4;
	    ng = 8+4;
	  }
	break;

      case FFI_TYPE_SINT8:
      case FFI_TYPE_UINT8:
#ifndef __LITTLE_ENDIAN__
	if (ng < NUM_GPR_ARG_REGISTERS)
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
	if (ng < NUM_GPR_ARG_REGISTERS)
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
	if (ng < NUM_GPR_ARG_REGISTERS)
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
	

	if (ng < NUM_GPR_ARG_REGISTERS)
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
	







	if (ng < NUM_GPR_ARG_REGISTERS - 1)
	  {
	    if (ng & 1)
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
	    ng = NUM_GPR_ARG_REGISTERS;
	  }
	break;

      default:
	FFI_ASSERT (0);
      }

    i++;
  }

  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  





  if (rtypenum == FFI_TYPE_STRUCT
      && (cif->abi & FFI_SYSV_STRUCT_RET) != 0 && size <= 8)
    return FFI_SYSV_TYPE_SMALL_STRUCT - 1 + size;
  return rtypenum;
}
#endif






























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>

extern void ffi_closure_ASM (void);

enum {
  
  FLAG_RETURNS_NOTHING  = 1 << (31-30), 
  FLAG_RETURNS_FP       = 1 << (31-29),
  FLAG_RETURNS_64BITS   = 1 << (31-28),
  FLAG_RETURNS_128BITS  = 1 << (31-31),

  FLAG_ARG_NEEDS_COPY   = 1 << (31- 7),
  FLAG_FP_ARGUMENTS     = 1 << (31- 6), 
  FLAG_4_GPR_ARGUMENTS  = 1 << (31- 5),
  FLAG_RETVAL_REFERENCE = 1 << (31- 4)
};


enum {
  NUM_GPR_ARG_REGISTERS = 8,
  NUM_FPR_ARG_REGISTERS = 13
};
enum { ASM_NEEDS_REGISTERS = 4 };



























void
ffi_prep_args (extended_cif *ecif, unsigned long *const stack)
{
  const unsigned bytes = ecif->cif->bytes;
  const unsigned flags = ecif->cif->flags;
  const unsigned nargs = ecif->cif->nargs;
  const ffi_abi abi = ecif->cif->abi;

  
  unsigned long *const stacktop = stack + (bytes / sizeof(unsigned long));

  

  double *fpr_base = (double *) (stacktop - ASM_NEEDS_REGISTERS) - NUM_FPR_ARG_REGISTERS;
  int fparg_count = 0;


  
  unsigned long *next_arg = stack + 6; 

  int i;
  double double_tmp;
  void **p_argv = ecif->avalue;
  unsigned long gprvalue;
  ffi_type** ptr = ecif->cif->arg_types;
  char *dest_cpy;
  unsigned size_al = 0;

  
  FFI_ASSERT(((unsigned) (char *) stack & 0xF) == 0);
  FFI_ASSERT(((unsigned) (char *) stacktop & 0xF) == 0);
  FFI_ASSERT((bytes & 0xF) == 0);

  



  if (flags & FLAG_RETVAL_REFERENCE)
    *next_arg++ = (unsigned long) (char *) ecif->rvalue;

  
  for (i = nargs; i > 0; i--, ptr++, p_argv++)
    {
      switch ((*ptr)->type)
	{
	


	case FFI_TYPE_FLOAT:
	  double_tmp = *(float *) *p_argv;
	  if (fparg_count >= NUM_FPR_ARG_REGISTERS)
	    *(double *)next_arg = double_tmp;
	  else
	    *fpr_base++ = double_tmp;
	  next_arg++;
	  fparg_count++;
	  FFI_ASSERT(flags & FLAG_FP_ARGUMENTS);
	  break;

	case FFI_TYPE_DOUBLE:
	  double_tmp = *(double *) *p_argv;
	  if (fparg_count >= NUM_FPR_ARG_REGISTERS)
	    *(double *)next_arg = double_tmp;
	  else
	    *fpr_base++ = double_tmp;
#ifdef POWERPC64
	  next_arg++;
#else
	  next_arg += 2;
#endif
	  fparg_count++;
	  FFI_ASSERT(flags & FLAG_FP_ARGUMENTS);
	  break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE

	case FFI_TYPE_LONGDOUBLE:
#ifdef POWERPC64
	  if (fparg_count < NUM_FPR_ARG_REGISTERS)
	    *(long double *) fpr_base++ = *(long double *) *p_argv;
	  else
	    *(long double *) next_arg = *(long double *) *p_argv;
	  next_arg += 2;
	  fparg_count += 2;
#else
	  double_tmp = ((double *) *p_argv)[0];
	  if (fparg_count < NUM_FPR_ARG_REGISTERS)
	    *fpr_base++ = double_tmp;
	  else
	    *(double *) next_arg = double_tmp;
	  next_arg += 2;
	  fparg_count++;

	  double_tmp = ((double *) *p_argv)[1];
	  if (fparg_count < NUM_FPR_ARG_REGISTERS)
	    *fpr_base++ = double_tmp;
	  else
	    *(double *) next_arg = double_tmp;
	  next_arg += 2;
	  fparg_count++;
#endif
	  FFI_ASSERT(flags & FLAG_FP_ARGUMENTS);
	  break;
#endif
	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
#ifdef POWERPC64
	  gprvalue = *(long long *) *p_argv;
	  goto putgpr;
#else
	  *(long long *) next_arg = *(long long *) *p_argv;
	  next_arg += 2;
#endif
	  break;
	case FFI_TYPE_POINTER:
	  gprvalue = *(unsigned long *) *p_argv;
	  goto putgpr;
	case FFI_TYPE_UINT8:
	  gprvalue = *(unsigned char *) *p_argv;
	  goto putgpr;
	case FFI_TYPE_SINT8:
	  gprvalue = *(signed char *) *p_argv;
	  goto putgpr;
	case FFI_TYPE_UINT16:
	  gprvalue = *(unsigned short *) *p_argv;
	  goto putgpr;
	case FFI_TYPE_SINT16:
	  gprvalue = *(signed short *) *p_argv;
	  goto putgpr;

	case FFI_TYPE_STRUCT:
#ifdef POWERPC64
	  dest_cpy = (char *) next_arg;
	  size_al = (*ptr)->size;
	  if ((*ptr)->elements[0]->type == 3)
	    size_al = ALIGN((*ptr)->size, 8);
	  if (size_al < 3 && abi == FFI_DARWIN)
	    dest_cpy += 4 - size_al;

	  memcpy ((char *) dest_cpy, (char *) *p_argv, size_al);
	  next_arg += (size_al + 7) / 8;
#else
	  dest_cpy = (char *) next_arg;

	  


	  size_al = (*ptr)->size;
	  

	  if ((*ptr)->elements[0]->type == FFI_TYPE_DOUBLE)
	    size_al = ALIGN((*ptr)->size, 8);
	  if (size_al < 3 && abi == FFI_DARWIN)
	    dest_cpy += 4 - size_al;

	  memcpy((char *) dest_cpy, (char *) *p_argv, size_al);
	  next_arg += (size_al + 3) / 4;
#endif
	  break;

	case FFI_TYPE_INT:
	case FFI_TYPE_SINT32:
	  gprvalue = *(signed int *) *p_argv;
	  goto putgpr;

	case FFI_TYPE_UINT32:
	  gprvalue = *(unsigned int *) *p_argv;
	putgpr:
	  *next_arg++ = gprvalue;
	  break;
	default:
	  break;
	}
    }

  
  
  
  
  
}




static void
darwin_adjust_aggregate_sizes (ffi_type *s)
{
  int i;

  if (s->type != FFI_TYPE_STRUCT)
    return;

  s->size = 0;
  for (i = 0; s->elements[i] != NULL; i++)
    {
      ffi_type *p;
      int align;
      
      p = s->elements[i];
      darwin_adjust_aggregate_sizes (p);
      if (i == 0
	  && (p->type == FFI_TYPE_UINT64
	      || p->type == FFI_TYPE_SINT64
	      || p->type == FFI_TYPE_DOUBLE
	      || p->alignment == 8))
	align = 8;
      else if (p->alignment == 16 || p->alignment < 4)
	align = p->alignment;
      else
	align = 4;
      s->size = ALIGN(s->size, align) + p->size;
    }
  
  s->size = ALIGN(s->size, s->alignment);
  
  if (s->elements[0]->type == FFI_TYPE_UINT64
      || s->elements[0]->type == FFI_TYPE_SINT64
      || s->elements[0]->type == FFI_TYPE_DOUBLE
      || s->elements[0]->alignment == 8)
    s->alignment = s->alignment > 8 ? s->alignment : 8;
  
}




static void
aix_adjust_aggregate_sizes (ffi_type *s)
{
  int i;

  if (s->type != FFI_TYPE_STRUCT)
    return;

  s->size = 0;
  for (i = 0; s->elements[i] != NULL; i++)
    {
      ffi_type *p;
      int align;
      
      p = s->elements[i];
      aix_adjust_aggregate_sizes (p);
      align = p->alignment;
      if (i != 0 && p->type == FFI_TYPE_DOUBLE)
	align = 4;
      s->size = ALIGN(s->size, align) + p->size;
    }
  
  s->size = ALIGN(s->size, s->alignment);
  
  if (s->elements[0]->type == FFI_TYPE_UINT64
      || s->elements[0]->type == FFI_TYPE_SINT64
      || s->elements[0]->type == FFI_TYPE_DOUBLE
      || s->elements[0]->alignment == 8)
    s->alignment = s->alignment > 8 ? s->alignment : 8;
  
}


ffi_status
ffi_prep_cif_machdep (ffi_cif *cif)
{
  
  unsigned i;
  ffi_type **ptr;
  unsigned bytes;
  int fparg_count = 0, intarg_count = 0;
  unsigned flags = 0;
  unsigned size_al = 0;

  



  if (cif->abi == FFI_DARWIN)
    {
      darwin_adjust_aggregate_sizes (cif->rtype);
      for (i = 0; i < cif->nargs; i++)
	darwin_adjust_aggregate_sizes (cif->arg_types[i]);
    }

  if (cif->abi == FFI_AIX)
    {
      aix_adjust_aggregate_sizes (cif->rtype);
      for (i = 0; i < cif->nargs; i++)
	aix_adjust_aggregate_sizes (cif->arg_types[i]);
    }

  


  bytes = (6 + ASM_NEEDS_REGISTERS) * sizeof(long);

  









  switch (cif->rtype->type)
    {

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
    case FFI_TYPE_LONGDOUBLE:
      flags |= FLAG_RETURNS_128BITS;
      flags |= FLAG_RETURNS_FP;
      break;
#endif

    case FFI_TYPE_DOUBLE:
      flags |= FLAG_RETURNS_64BITS;
      
    case FFI_TYPE_FLOAT:
      flags |= FLAG_RETURNS_FP;
      break;

    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
#ifdef POWERPC64
    case FFI_TYPE_POINTER:
#endif
      flags |= FLAG_RETURNS_64BITS;
      break;

    case FFI_TYPE_STRUCT:
      flags |= FLAG_RETVAL_REFERENCE;
      flags |= FLAG_RETURNS_NOTHING;
      intarg_count++;
      break;
    case FFI_TYPE_VOID:
      flags |= FLAG_RETURNS_NOTHING;
      break;

    default:
      
      break;
    }

  



  for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++)
    {
      switch ((*ptr)->type)
	{
	case FFI_TYPE_FLOAT:
	case FFI_TYPE_DOUBLE:
	  fparg_count++;
	  

	  if (fparg_count > NUM_FPR_ARG_REGISTERS
	      && intarg_count%2 != 0)
	    intarg_count++;
	  break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE

	case FFI_TYPE_LONGDOUBLE:
	  fparg_count += 2;
	  

	  if (fparg_count > NUM_FPR_ARG_REGISTERS
	      && intarg_count%2 != 0)
	    intarg_count++;
	  intarg_count +=2;
	  break;
#endif

	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	  



	  if (intarg_count == NUM_GPR_ARG_REGISTERS-1
	      || (intarg_count >= NUM_GPR_ARG_REGISTERS && intarg_count%2 != 0))
	    intarg_count++;
	  intarg_count += 2;
	  break;

	case FFI_TYPE_STRUCT:
	  size_al = (*ptr)->size;
	  

	  if ((*ptr)->elements[0]->type == FFI_TYPE_DOUBLE)
	    size_al = ALIGN((*ptr)->size, 8);
#ifdef POWERPC64
	  intarg_count += (size_al + 7) / 8;
#else
	  intarg_count += (size_al + 3) / 4;
#endif
	  break;

	default:
	  

	  intarg_count++;
	  break;
	}
    }

  if (fparg_count != 0)
    flags |= FLAG_FP_ARGUMENTS;

  
  if (fparg_count != 0)
    bytes += NUM_FPR_ARG_REGISTERS * sizeof(double);

  
#ifdef POWERPC64
  if ((intarg_count + fparg_count) > NUM_GPR_ARG_REGISTERS)
    bytes += (intarg_count + fparg_count) * sizeof(long);
#else
  if ((intarg_count + 2 * fparg_count) > NUM_GPR_ARG_REGISTERS)
    bytes += (intarg_count + 2 * fparg_count) * sizeof(long);
#endif
  else
    bytes += NUM_GPR_ARG_REGISTERS * sizeof(long);

  
  bytes = (bytes + 15) & ~0xF;

  cif->flags = flags;
  cif->bytes = bytes;

  return FFI_OK;
}

extern void ffi_call_AIX(extended_cif *, long, unsigned, unsigned *,
			 void (*fn)(void), void (*fn2)(void));
extern void ffi_call_DARWIN(extended_cif *, long, unsigned, unsigned *,
			    void (*fn)(void), void (*fn2)(void));

void
ffi_call (ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  


  if ((rvalue == NULL) &&
      (cif->rtype->type == FFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca (cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;

  switch (cif->abi)
    {
    case FFI_AIX:
      ffi_call_AIX(&ecif, -(long)cif->bytes, cif->flags, ecif.rvalue, fn,
		   FFI_FN(ffi_prep_args));
      break;
    case FFI_DARWIN:
      ffi_call_DARWIN(&ecif, -(long)cif->bytes, cif->flags, ecif.rvalue, fn,
		      FFI_FN(ffi_prep_args));
      break;
    default:
      FFI_ASSERT(0);
      break;
    }
}

static void flush_icache(char *);
static void flush_range(char *, int);




typedef struct aix_fd_struct {
  void *code_pointer;
  void *toc;
} aix_fd;






















































ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned int *tramp;
  struct ffi_aix_trampoline_struct *tramp_aix;
  aix_fd *fd;

  switch (cif->abi)
    {
    case FFI_DARWIN:

      FFI_ASSERT (cif->abi == FFI_DARWIN);

      tramp = (unsigned int *) &closure->tramp[0];
      tramp[0] = 0x7c0802a6;  
      tramp[1] = 0x429f000d;  
      tramp[4] = 0x7d6802a6;  
      tramp[5] = 0x818b0000;  
      tramp[6] = 0x7c0803a6;  
      tramp[7] = 0x7d8903a6;  
      tramp[8] = 0x816b0004;  
      tramp[9] = 0x4e800420;  
      tramp[2] = (unsigned long) ffi_closure_ASM; 
      tramp[3] = (unsigned long) codeloc; 

      closure->cif = cif;
      closure->fun = fun;
      closure->user_data = user_data;

      
      flush_range(codeloc, FFI_TRAMPOLINE_SIZE);

      break;

    case FFI_AIX:

      tramp_aix = (struct ffi_aix_trampoline_struct *) (closure->tramp);
      fd = (aix_fd *)(void *)ffi_closure_ASM;

      FFI_ASSERT (cif->abi == FFI_AIX);

      tramp_aix->code_pointer = fd->code_pointer;
      tramp_aix->toc = fd->toc;
      tramp_aix->static_chain = codeloc;
      closure->cif = cif;
      closure->fun = fun;
      closure->user_data = user_data;

    default:

      FFI_ASSERT(0);
      break;
    }
  return FFI_OK;
}

static void
flush_icache(char *addr)
{
#ifndef _AIX
  __asm__ volatile (
		"dcbf 0,%0\n"
		"\tsync\n"
		"\ticbi 0,%0\n"
		"\tsync\n"
		"\tisync"
		: : "r"(addr) : "memory");
#endif
}

static void
flush_range(char * addr1, int size)
{
#define MIN_LINE_SIZE 32
  int i;
  for (i = 0; i < size; i += MIN_LINE_SIZE)
    flush_icache(addr1+i);
  flush_icache(addr1+size-1);
}

typedef union
{
  float f;
  double d;
} ffi_dblfl;

int
ffi_closure_helper_DARWIN (ffi_closure *, void *,
			   unsigned long *, ffi_dblfl *);








int
ffi_closure_helper_DARWIN (ffi_closure *closure, void *rvalue,
			   unsigned long *pgr, ffi_dblfl *pfr)
{
  



  typedef double ldbits[2];

  union ldu
  {
    ldbits lb;
    long double ld;
  };

  void **          avalue;
  ffi_type **      arg_types;
  long             i, avn;
  ffi_cif *        cif;
  ffi_dblfl *      end_pfr = pfr + NUM_FPR_ARG_REGISTERS;
  unsigned         size_al;

  cif = closure->cif;
  avalue = alloca (cif->nargs * sizeof(void *));

  

  if (cif->rtype->type == FFI_TYPE_STRUCT)
    {
      rvalue = (void *) *pgr;
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
#ifdef POWERPC64
	  avalue[i] = (char *) pgr + 7;
#else
	  avalue[i] = (char *) pgr + 3;
#endif
	  pgr++;
	  break;

	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT16:
#ifdef POWERPC64
	  avalue[i] = (char *) pgr + 6;
#else
	  avalue[i] = (char *) pgr + 2;
#endif
	  pgr++;
	  break;

	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT32:
#ifdef POWERPC64
	  avalue[i] = (char *) pgr + 4;
#else
	case FFI_TYPE_POINTER:
	  avalue[i] = pgr;
#endif
	  pgr++;
	  break;

	case FFI_TYPE_STRUCT:
#ifdef POWERPC64
	  size_al = arg_types[i]->size;
	  if (arg_types[i]->elements[0]->type == FFI_TYPE_DOUBLE)
	    size_al = ALIGN (arg_types[i]->size, 8);
	  if (size_al < 3 && cif->abi == FFI_DARWIN)
	    avalue[i] = (char *) pgr + 8 - size_al;
	  else
	    avalue[i] = pgr;
	  pgr += (size_al + 7) / 8;
#else
	  

	  size_al = arg_types[i]->size;
	  

	  if (arg_types[i]->elements[0]->type == FFI_TYPE_DOUBLE)
	    size_al = ALIGN(arg_types[i]->size, 8);
	  if (size_al < 3 && cif->abi == FFI_DARWIN)
	    avalue[i] = (char*) pgr + 4 - size_al;
	  else
	    avalue[i] = pgr;
	  pgr += (size_al + 3) / 4;
#endif
	  break;

	case FFI_TYPE_SINT64:
	case FFI_TYPE_UINT64:
#ifdef POWERPC64
	case FFI_TYPE_POINTER:
	  avalue[i] = pgr;
	  pgr++;
	  break;
#else

	  avalue[i] = pgr;
	  pgr += 2;
	  break;
#endif

	case FFI_TYPE_FLOAT:
	  

	  if (pfr < end_pfr)
	    {
	      double temp = pfr->d;
	      pfr->f = (float) temp;
	      avalue[i] = pfr;
	      pfr++;
	    }
	  else
	    {
	      avalue[i] = pgr;
	    }
	  pgr++;
	  break;

	case FFI_TYPE_DOUBLE:
	  

	  if (pfr < end_pfr)
	    {
	      avalue[i] = pfr;
	      pfr++;
	    }
	  else
	    {
	      avalue[i] = pgr;
	    }
#ifdef POWERPC64
	  pgr++;
#else
	  pgr += 2;
#endif
	  break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE

	case FFI_TYPE_LONGDOUBLE:
#ifdef POWERPC64
	  if (pfr + 1 < end_pfr)
	    {
	      avalue[i] = pfr;
	      pfr += 2;
	    }
	  else
	    {
	      if (pfr < end_pfr)
		{
		  *pgr = *(unsigned long *) pfr;
		  pfr++;
		}
	      avalue[i] = pgr;
	    }
	  pgr += 2;
#else  
	  

	  if (pfr + 1 < end_pfr)
	    {
	      avalue[i] = pfr;
	      pfr += 2;
	    }
	  


	  else if (pfr + 1 == end_pfr)
	    {
	      union ldu temp_ld;
	      memcpy (&temp_ld.lb[0], pfr, sizeof(ldbits));
	      memcpy (&temp_ld.lb[1], pgr + 2, sizeof(ldbits));
	      avalue[i] = &temp_ld.ld;
	      pfr++;
	    }
	  else
	    {
	      avalue[i] = pgr;
	    }
	  pgr += 4;
#endif  
	  break;
#endif
	default:
	  FFI_ASSERT(0);
	}
      i++;
    }

  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  
  return cif->rtype->type;
}

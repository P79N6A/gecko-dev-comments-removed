




























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>
#include <stdarg.h>

#ifdef __x86_64__

#define MAX_GPR_REGS 6
#define MAX_SSE_REGS 8

#if defined(__INTEL_COMPILER)
#include "xmmintrin.h"
#define UINT128 __m128
#else
#if defined(__SUNPRO_C)
#include <sunmedia_types.h>
#define UINT128 __m128i
#else
#define UINT128 __int128_t
#endif
#endif

union big_int_union
{
  UINT32 i32;
  UINT64 i64;
  UINT128 i128;
};

struct register_args
{
  
  UINT64 gpr[MAX_GPR_REGS];
  union big_int_union sse[MAX_SSE_REGS];
};

extern void ffi_call_unix64 (void *args, unsigned long bytes, unsigned flags,
			     void *raddr, void (*fnaddr)(void), unsigned ssecount);












enum x86_64_reg_class
  {
    X86_64_NO_CLASS,
    X86_64_INTEGER_CLASS,
    X86_64_INTEGERSI_CLASS,
    X86_64_SSE_CLASS,
    X86_64_SSESF_CLASS,
    X86_64_SSEDF_CLASS,
    X86_64_SSEUP_CLASS,
    X86_64_X87_CLASS,
    X86_64_X87UP_CLASS,
    X86_64_COMPLEX_X87_CLASS,
    X86_64_MEMORY_CLASS
  };

#define MAX_CLASSES 4

#define SSE_CLASS_P(X)	((X) >= X86_64_SSE_CLASS && X <= X86_64_SSEUP_CLASS)








static enum x86_64_reg_class
merge_classes (enum x86_64_reg_class class1, enum x86_64_reg_class class2)
{
  
  if (class1 == class2)
    return class1;

  

  if (class1 == X86_64_NO_CLASS)
    return class2;
  if (class2 == X86_64_NO_CLASS)
    return class1;

  
  if (class1 == X86_64_MEMORY_CLASS || class2 == X86_64_MEMORY_CLASS)
    return X86_64_MEMORY_CLASS;

  
  if ((class1 == X86_64_INTEGERSI_CLASS && class2 == X86_64_SSESF_CLASS)
      || (class2 == X86_64_INTEGERSI_CLASS && class1 == X86_64_SSESF_CLASS))
    return X86_64_INTEGERSI_CLASS;
  if (class1 == X86_64_INTEGER_CLASS || class1 == X86_64_INTEGERSI_CLASS
      || class2 == X86_64_INTEGER_CLASS || class2 == X86_64_INTEGERSI_CLASS)
    return X86_64_INTEGER_CLASS;

  

  if (class1 == X86_64_X87_CLASS
      || class1 == X86_64_X87UP_CLASS
      || class1 == X86_64_COMPLEX_X87_CLASS
      || class2 == X86_64_X87_CLASS
      || class2 == X86_64_X87UP_CLASS
      || class2 == X86_64_COMPLEX_X87_CLASS)
    return X86_64_MEMORY_CLASS;

  
  return X86_64_SSE_CLASS;
}









static size_t
classify_argument (ffi_type *type, enum x86_64_reg_class classes[],
		   size_t byte_offset)
{
  switch (type->type)
    {
    case FFI_TYPE_UINT8:
    case FFI_TYPE_SINT8:
    case FFI_TYPE_UINT16:
    case FFI_TYPE_SINT16:
    case FFI_TYPE_UINT32:
    case FFI_TYPE_SINT32:
    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
    case FFI_TYPE_POINTER:
      {
	size_t size = byte_offset + type->size;

	if (size <= 4)
	  {
	    classes[0] = X86_64_INTEGERSI_CLASS;
	    return 1;
	  }
	else if (size <= 8)
	  {
	    classes[0] = X86_64_INTEGER_CLASS;
	    return 1;
	  }
	else if (size <= 12)
	  {
	    classes[0] = X86_64_INTEGER_CLASS;
	    classes[1] = X86_64_INTEGERSI_CLASS;
	    return 2;
	  }
	else if (size <= 16)
	  {
	    classes[0] = classes[1] = X86_64_INTEGERSI_CLASS;
	    return 2;
	  }
	else
	  FFI_ASSERT (0);
      }
    case FFI_TYPE_FLOAT:
      if (!(byte_offset % 8))
	classes[0] = X86_64_SSESF_CLASS;
      else
	classes[0] = X86_64_SSE_CLASS;
      return 1;
    case FFI_TYPE_DOUBLE:
      classes[0] = X86_64_SSEDF_CLASS;
      return 1;
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
    case FFI_TYPE_LONGDOUBLE:
      classes[0] = X86_64_X87_CLASS;
      classes[1] = X86_64_X87UP_CLASS;
      return 2;
#endif
    case FFI_TYPE_STRUCT:
      {
	const size_t UNITS_PER_WORD = 8;
	size_t words = (type->size + UNITS_PER_WORD - 1) / UNITS_PER_WORD;
	ffi_type **ptr;
	int i;
	enum x86_64_reg_class subclasses[MAX_CLASSES];

	
	if (type->size > 32)
	  return 0;

	for (i = 0; i < words; i++)
	  classes[i] = X86_64_NO_CLASS;

	

	if (!words)
	  {
	    classes[0] = X86_64_NO_CLASS;
	    return 1;
	  }

	
	for (ptr = type->elements; *ptr != NULL; ptr++)
	  {
	    size_t num;

	    byte_offset = ALIGN (byte_offset, (*ptr)->alignment);

	    num = classify_argument (*ptr, subclasses, byte_offset % 8);
	    if (num == 0)
	      return 0;
	    for (i = 0; i < num; i++)
	      {
		size_t pos = byte_offset / 8;
		classes[i + pos] =
		  merge_classes (subclasses[i], classes[i + pos]);
	      }

	    byte_offset += (*ptr)->size;
	  }

	if (words > 2)
	  {
	    



	    if (classes[0] != X86_64_SSE_CLASS)
	      return 0;

	    for (i = 1; i < words; i++)
	      if (classes[i] != X86_64_SSEUP_CLASS)
		return 0;
	  }

	
	for (i = 0; i < words; i++)
	  {
	    

	    if (classes[i] == X86_64_MEMORY_CLASS)
	      return 0;

	    

	    if (classes[i] == X86_64_SSEUP_CLASS
		&& classes[i - 1] != X86_64_SSE_CLASS
		&& classes[i - 1] != X86_64_SSEUP_CLASS)
	      {
		
		FFI_ASSERT (i != 0);
		classes[i] = X86_64_SSE_CLASS;
	      }

	    

	    if (classes[i] == X86_64_X87UP_CLASS
		&& (classes[i - 1] != X86_64_X87_CLASS))
	      {
		
		FFI_ASSERT (i != 0);
		return 0;
	      }
	  }
	return words;
      }

    default:
      FFI_ASSERT(0);
    }
  return 0; 
}





static size_t
examine_argument (ffi_type *type, enum x86_64_reg_class classes[MAX_CLASSES],
		  _Bool in_return, int *pngpr, int *pnsse)
{
  size_t n;
  int i, ngpr, nsse;

  n = classify_argument (type, classes, 0);
  if (n == 0)
    return 0;

  ngpr = nsse = 0;
  for (i = 0; i < n; ++i)
    switch (classes[i])
      {
      case X86_64_INTEGER_CLASS:
      case X86_64_INTEGERSI_CLASS:
	ngpr++;
	break;
      case X86_64_SSE_CLASS:
      case X86_64_SSESF_CLASS:
      case X86_64_SSEDF_CLASS:
	nsse++;
	break;
      case X86_64_NO_CLASS:
      case X86_64_SSEUP_CLASS:
	break;
      case X86_64_X87_CLASS:
      case X86_64_X87UP_CLASS:
      case X86_64_COMPLEX_X87_CLASS:
	return in_return != 0;
      default:
	abort ();
      }

  *pngpr = ngpr;
  *pnsse = nsse;

  return n;
}



ffi_status
ffi_prep_cif_machdep (ffi_cif *cif)
{
  int gprcount, ssecount, i, avn, ngpr, nsse, flags;
  enum x86_64_reg_class classes[MAX_CLASSES];
  size_t bytes, n;

  gprcount = ssecount = 0;

  flags = cif->rtype->type;
  if (flags != FFI_TYPE_VOID)
    {
      n = examine_argument (cif->rtype, classes, 1, &ngpr, &nsse);
      if (n == 0)
	{
	  

	  gprcount++;
	  
	  flags = FFI_TYPE_VOID;
	}
      else if (flags == FFI_TYPE_STRUCT)
	{
	  
	  _Bool sse0 = SSE_CLASS_P (classes[0]);
	  _Bool sse1 = n == 2 && SSE_CLASS_P (classes[1]);
	  if (sse0 && !sse1)
	    flags |= 1 << 8;
	  else if (!sse0 && sse1)
	    flags |= 1 << 9;
	  else if (sse0 && sse1)
	    flags |= 1 << 10;
	  
	  flags |= cif->rtype->size << 12;
	}
    }

  


  for (bytes = 0, i = 0, avn = cif->nargs; i < avn; i++)
    {
      if (examine_argument (cif->arg_types[i], classes, 0, &ngpr, &nsse) == 0
	  || gprcount + ngpr > MAX_GPR_REGS
	  || ssecount + nsse > MAX_SSE_REGS)
	{
	  long align = cif->arg_types[i]->alignment;

	  if (align < 8)
	    align = 8;

	  bytes = ALIGN (bytes, align);
	  bytes += cif->arg_types[i]->size;
	}
      else
	{
	  gprcount += ngpr;
	  ssecount += nsse;
	}
    }
  if (ssecount)
    flags |= 1 << 11;
  cif->flags = flags;
  cif->bytes = (unsigned)ALIGN (bytes, 8);

  return FFI_OK;
}

void
ffi_call (ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  enum x86_64_reg_class classes[MAX_CLASSES];
  char *stack, *argp;
  ffi_type **arg_types;
  int gprcount, ssecount, ngpr, nsse, i, avn;
  _Bool ret_in_memory;
  struct register_args *reg_args;

  
  FFI_ASSERT (cif->abi == FFI_UNIX64);

  


  ret_in_memory = (cif->rtype->type == FFI_TYPE_STRUCT
		   && (cif->flags & 0xff) == FFI_TYPE_VOID);
  if (rvalue == NULL && ret_in_memory)
    rvalue = alloca (cif->rtype->size);

  
  stack = alloca (sizeof (struct register_args) + cif->bytes + 4*8);
  reg_args = (struct register_args *) stack;
  argp = stack + sizeof (struct register_args);

  gprcount = ssecount = 0;

  

  if (ret_in_memory)
    reg_args->gpr[gprcount++] = (unsigned long) rvalue;

  avn = cif->nargs;
  arg_types = cif->arg_types;

  for (i = 0; i < avn; ++i)
    {
      size_t n, size = arg_types[i]->size;

      n = examine_argument (arg_types[i], classes, 0, &ngpr, &nsse);
      if (n == 0
	  || gprcount + ngpr > MAX_GPR_REGS
	  || ssecount + nsse > MAX_SSE_REGS)
	{
	  long align = arg_types[i]->alignment;

	  
	  if (align < 8)
	    align = 8;

	  
	  argp = (void *) ALIGN (argp, align);
	  memcpy (argp, avalue[i], size);
	  argp += size;
	}
      else
	{
	  
	  char *a = (char *) avalue[i];
	  int j;

	  for (j = 0; j < n; j++, a += 8, size -= 8)
	    {
	      switch (classes[j])
		{
		case X86_64_INTEGER_CLASS:
		case X86_64_INTEGERSI_CLASS:
		  



		  switch (arg_types[i]->type)
		    {
		    case FFI_TYPE_SINT8:
		      *(SINT64 *)&reg_args->gpr[gprcount] = (SINT64) *((SINT8 *) a);
		      break;
		    case FFI_TYPE_SINT16:
		      *(SINT64 *)&reg_args->gpr[gprcount] = (SINT64) *((SINT16 *) a);
		      break;
		    case FFI_TYPE_SINT32:
		      *(SINT64 *)&reg_args->gpr[gprcount] = (SINT64) *((SINT32 *) a);
		      break;
		    default:
		      reg_args->gpr[gprcount] = 0;
		      memcpy (&reg_args->gpr[gprcount], a, size < 8 ? size : 8);
		    }
		  gprcount++;
		  break;
		case X86_64_SSE_CLASS:
		case X86_64_SSEDF_CLASS:
		  reg_args->sse[ssecount++].i64 = *(UINT64 *) a;
		  break;
		case X86_64_SSESF_CLASS:
		  reg_args->sse[ssecount++].i32 = *(UINT32 *) a;
		  break;
		default:
		  abort();
		}
	    }
	}
    }

  ffi_call_unix64 (stack, cif->bytes + sizeof (struct register_args),
		   cif->flags, rvalue, fn, ssecount);
}


extern void ffi_closure_unix64(void);

ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  volatile unsigned short *tramp;

  
  {
    int abi = cif->abi;
    if (UNLIKELY (! (abi > FFI_FIRST_ABI && abi < FFI_LAST_ABI)))
      return FFI_BAD_ABI;
  }

  tramp = (volatile unsigned short *) &closure->tramp[0];

  tramp[0] = 0xbb49;		
  *((unsigned long long * volatile) &tramp[1])
    = (unsigned long) ffi_closure_unix64;
  tramp[5] = 0xba49;		
  *((unsigned long long * volatile) &tramp[6])
    = (unsigned long) codeloc;

  

  tramp[10] = cif->flags & (1 << 11) ? 0x49f9 : 0x49f8;

  tramp[11] = 0xe3ff;			

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return FFI_OK;
}

int
ffi_closure_unix64_inner(ffi_closure *closure, void *rvalue,
			 struct register_args *reg_args, char *argp)
{
  ffi_cif *cif;
  void **avalue;
  ffi_type **arg_types;
  long i, avn;
  int gprcount, ssecount, ngpr, nsse;
  int ret;

  cif = closure->cif;
  avalue = alloca(cif->nargs * sizeof(void *));
  gprcount = ssecount = 0;

  ret = cif->rtype->type;
  if (ret != FFI_TYPE_VOID)
    {
      enum x86_64_reg_class classes[MAX_CLASSES];
      size_t n = examine_argument (cif->rtype, classes, 1, &ngpr, &nsse);
      if (n == 0)
	{
	  

	  rvalue = (void *) (unsigned long) reg_args->gpr[gprcount++];
	  
	  ret = FFI_TYPE_VOID;
	}
      else if (ret == FFI_TYPE_STRUCT && n == 2)
	{
	  
	  _Bool sse0 = SSE_CLASS_P (classes[0]);
	  _Bool sse1 = SSE_CLASS_P (classes[1]);
	  if (!sse0 && sse1)
	    ret |= 1 << 8;
	  else if (sse0 && !sse1)
	    ret |= 1 << 9;
	}
    }

  avn = cif->nargs;
  arg_types = cif->arg_types;

  for (i = 0; i < avn; ++i)
    {
      enum x86_64_reg_class classes[MAX_CLASSES];
      size_t n;

      n = examine_argument (arg_types[i], classes, 0, &ngpr, &nsse);
      if (n == 0
	  || gprcount + ngpr > MAX_GPR_REGS
	  || ssecount + nsse > MAX_SSE_REGS)
	{
	  long align = arg_types[i]->alignment;

	  
	  if (align < 8)
	    align = 8;

	  
	  argp = (void *) ALIGN (argp, align);
	  avalue[i] = argp;
	  argp += arg_types[i]->size;
	}
      

      else if (n == 1
	       || (n == 2 && !(SSE_CLASS_P (classes[0])
			       || SSE_CLASS_P (classes[1]))))
	{
	  
	  if (SSE_CLASS_P (classes[0]))
	    {
	      avalue[i] = &reg_args->sse[ssecount];
	      ssecount += n;
	    }
	  else
	    {
	      avalue[i] = &reg_args->gpr[gprcount];
	      gprcount += n;
	    }
	}
      
      else
	{
	  char *a = alloca (16);
	  int j;

	  avalue[i] = a;
	  for (j = 0; j < n; j++, a += 8)
	    {
	      if (SSE_CLASS_P (classes[j]))
		memcpy (a, &reg_args->sse[ssecount++], 8);
	      else
		memcpy (a, &reg_args->gpr[gprcount++], 8);
	    }
	}
    }

  
  closure->fun (cif, rvalue, avalue, closure->user_data);

  
  return ret;
}

#endif 

























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>























extern UINT64 ffi_call_sysv (void (*) (char *, extended_cif *),
			     extended_cif *,
			     unsigned, 
			     void (*fn) (void));
extern void ffi_closure_sysv (void);



ffi_status ffi_prep_cif_machdep (ffi_cif *cif)
{
  



  if (cif->bytes < 16)
    cif->bytes = 16;
  else
    cif->bytes = (cif->bytes + 3) & ~3;

  return FFI_OK;
}








void ffi_prep_args (char *stack, extended_cif *ecif)
{
  char *argp = stack;
  unsigned int i;

  

  if (ecif->cif->rtype->type == FFI_TYPE_STRUCT
      && ecif->cif->rtype->size > 8)
    {
      (*(void **) argp) = ecif->rvalue;
      argp += 4;
    }

  for (i = 0; i < ecif->cif->nargs; i++)
    {
      void *avalue = ecif->avalue[i];
      ffi_type *atype = ecif->cif->arg_types[i];
      size_t size = atype->size;
      size_t alignment = atype->alignment;

      
      if ((alignment - 1) & (unsigned) argp)
	argp = (char *) ALIGN (argp, alignment);

      

      if (size < sizeof (int))
	{
	  size = sizeof (int);
	  switch (atype->type)
	    {
	    case FFI_TYPE_SINT8:
	      *(signed int *) argp = (signed int) *(SINT8 *) avalue;
	      break;
		  
	    case FFI_TYPE_UINT8:
	      *(unsigned int *) argp = (unsigned int) *(UINT8 *) avalue;
	      break;
		  
	    case FFI_TYPE_SINT16:
	      *(signed int *) argp = (signed int) *(SINT16 *) avalue;
	      break;
		  
	    case FFI_TYPE_UINT16:
	      *(unsigned int *) argp = (unsigned int) *(UINT16 *) avalue;
	      break;

	    case FFI_TYPE_STRUCT:
	      memcpy (argp, avalue, atype->size);
	      break;

	    default:
	      FFI_ASSERT(0);
	    }
	}
      else if (size == sizeof (int))
	*(unsigned int *) argp = (unsigned int) *(UINT32 *) avalue;
      else
	memcpy (argp, avalue, size);
      argp += size;
    }
}






void ffi_call (ffi_cif *cif, void (*fn) (void), void *rvalue, void **avalue)
{

  extended_cif ecif;
  UINT64 result;

  


  int bigret = (cif->rtype->type == FFI_TYPE_STRUCT
		&& cif->rtype->size > 8);

  ecif.cif = cif;
  ecif.avalue = avalue;

  

  if (rvalue == NULL && bigret)
    ecif.rvalue = alloca (cif->rtype->size);
  else
    ecif.rvalue = rvalue;

  result = ffi_call_sysv (ffi_prep_args, &ecif, cif->bytes, fn);

  


  if (rvalue && !bigret)
    switch (cif->rtype->size)
      {
      case 1:
	*(UINT8 *)rvalue = (UINT8) result;
	break;
      case 2:
	*(UINT16 *)rvalue = (UINT16) result;
	break;
      case 4:
	*(UINT32 *)rvalue = (UINT32) result;
	break;
      case 8:
	*(UINT64 *)rvalue = (UINT64) result;
	break;
      default:
	memcpy (rvalue, (void *)&result, cif->rtype->size);
	break;
      }
}





static UINT64
ffi_closure_helper (unsigned char *args,
		    ffi_closure *closure)
{
  ffi_cif *cif = closure->cif;
  unsigned char *argp = args;
  void **parsed_args = alloca (cif->nargs * sizeof (void *));
  UINT64 result;
  void *retptr;
  unsigned int i;

  


  if (cif->rtype->type == FFI_TYPE_STRUCT
      && cif->rtype->size > 8)
    {
      retptr = *((void **) argp);
      argp += 4;
    }
  else
    retptr = (void *) &result;

  
  for (i = 0; i < cif->nargs; i++)
    {
      size_t size = cif->arg_types[i]->size;
      size_t alignment = cif->arg_types[i]->alignment;

      
      if ((alignment - 1) & (unsigned) argp)
	argp = (char *) ALIGN (argp, alignment);

      
      if (size < sizeof (int))
	size = sizeof (int);

      
      parsed_args[i] = argp;
      argp += size;
    }

  
  (closure->fun) (cif, retptr, parsed_args, closure->user_data);
  return result;
}




ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun) (ffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned int *tramp = (unsigned int *) &closure->tramp[0];
  int i;

  if (cif->abi != FFI_SYSV)
    return FFI_BAD_ABI;

  














#define HI(x) ((((unsigned int) (x)) >> 16) & 0xffff)
#define LO(x) (((unsigned int) (x)) & 0xffff)
  tramp[0] = (0 << 27) | (8 << 22) | (HI (ffi_closure_sysv) << 6) | 0x34;
  tramp[1] = (8 << 27) | (8 << 22) | (LO (ffi_closure_sysv) << 6) | 0x14;
  tramp[2] = (0 << 27) | (9 << 22) | (HI (ffi_closure_helper) << 6) | 0x34;
  tramp[3] = (9 << 27) | (9 << 22) | (LO (ffi_closure_helper) << 6) | 0x14;
  tramp[4] = (0 << 27) | (10 << 22) | (HI (closure) << 6) | 0x34;
  tramp[5] = (10 << 27) | (10 << 22) | (LO (closure) << 6) | 0x14;
  tramp[6] = (8 << 27) | (0x0d << 11) | 0x3a;
#undef HI
#undef LO

  

  for (i = 0; i < 7; i++)
    asm volatile ("flushd 0(%0); flushi %0" :: "r"(tramp + i) : "memory");
  asm volatile ("flushp" ::: "memory");

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return FFI_OK;
}


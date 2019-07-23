

























#include <ffi.h>
#include <ffi_common.h>
#include <stdlib.h>



#if defined(__LONG_DOUBLE_128__)
# if FFI_TYPE_LONGDOUBLE != 4
#  error FFI_TYPE_LONGDOUBLE out of date
# endif
#else
# undef FFI_TYPE_LONGDOUBLE
# define FFI_TYPE_LONGDOUBLE 4
#endif

extern void ffi_call_osf(void *, unsigned long, unsigned, void *, void (*)(void))
  FFI_HIDDEN;
extern void ffi_closure_osf(void) FFI_HIDDEN;


ffi_status
ffi_prep_cif_machdep(ffi_cif *cif)
{
  

  if (cif->bytes < 6*FFI_SIZEOF_ARG)
    cif->bytes = 6*FFI_SIZEOF_ARG;

  
  switch (cif->rtype->type)
    {
    case FFI_TYPE_STRUCT:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
      cif->flags = cif->rtype->type;
      break;

    case FFI_TYPE_LONGDOUBLE:
      
      cif->flags = FFI_TYPE_STRUCT;
      break;

    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }
  
  return FFI_OK;
}


void
ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  unsigned long *stack, *argp;
  long i, avn;
  ffi_type **arg_types;
  
  

  if (rvalue == NULL && cif->flags == FFI_TYPE_STRUCT)
    rvalue = alloca(cif->rtype->size);

  

  argp = stack = alloca(cif->bytes + 4*FFI_SIZEOF_ARG);

  if (cif->flags == FFI_TYPE_STRUCT)
    *(void **) argp++ = rvalue;

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;

  while (i < avn)
    {
      size_t size = (*arg_types)->size;

      switch ((*arg_types)->type)
	{
	case FFI_TYPE_SINT8:
	  *(SINT64 *) argp = *(SINT8 *)(* avalue);
	  break;
		  
	case FFI_TYPE_UINT8:
	  *(SINT64 *) argp = *(UINT8 *)(* avalue);
	  break;
		  
	case FFI_TYPE_SINT16:
	  *(SINT64 *) argp = *(SINT16 *)(* avalue);
	  break;
		  
	case FFI_TYPE_UINT16:
	  *(SINT64 *) argp = *(UINT16 *)(* avalue);
	  break;
		  
	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT32:
	  
	  *(SINT64 *) argp = *(SINT32 *)(* avalue);
	  break;
		  
	case FFI_TYPE_SINT64:
	case FFI_TYPE_UINT64:
	case FFI_TYPE_POINTER:
	  *(UINT64 *) argp = *(UINT64 *)(* avalue);
	  break;

	case FFI_TYPE_FLOAT:
	  if (argp - stack < 6)
	    {
	      

	      *(double *) argp = *(float *)(* avalue);
	    }
	  else
	    *(float *) argp = *(float *)(* avalue);
	  break;

	case FFI_TYPE_DOUBLE:
	  *(double *) argp = *(double *)(* avalue);
	  break;

	case FFI_TYPE_LONGDOUBLE:
	  
	  *(long double **) argp = (long double *)(* avalue);
	  size = sizeof (long double *);
	  break;

	case FFI_TYPE_STRUCT:
	  memcpy(argp, *avalue, (*arg_types)->size);
	  break;

	default:
	  FFI_ASSERT(0);
	}

      argp += ALIGN(size, FFI_SIZEOF_ARG) / FFI_SIZEOF_ARG;
      i++, arg_types++, avalue++;
    }

  ffi_call_osf(stack, cif->bytes, cif->flags, rvalue, fn);
}


ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned int *tramp;

  tramp = (unsigned int *) &closure->tramp[0];
  tramp[0] = 0x47fb0401;	
  tramp[1] = 0xa77b0010;	
  tramp[2] = 0x6bfb0000;	
  tramp[3] = 0x47ff041f;	
  *(void **) &tramp[4] = ffi_closure_osf;

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  





  asm volatile ("call_pal 0x86" : : : "memory");

  return FFI_OK;
}


long FFI_HIDDEN
ffi_closure_osf_inner(ffi_closure *closure, void *rvalue, unsigned long *argp)
{
  ffi_cif *cif;
  void **avalue;
  ffi_type **arg_types;
  long i, avn, argn;

  cif = closure->cif;
  avalue = alloca(cif->nargs * sizeof(void *));

  argn = 0;

  

  if (cif->flags == FFI_TYPE_STRUCT)
    {
      rvalue = (void *) argp[0];
      argn = 1;
    }

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;
  
  
  while (i < avn)
    {
      size_t size = arg_types[i]->size;

      switch (arg_types[i]->type)
	{
	case FFI_TYPE_SINT8:
	case FFI_TYPE_UINT8:
	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT16:
	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT32:
	case FFI_TYPE_SINT64:
	case FFI_TYPE_UINT64:
	case FFI_TYPE_POINTER:
	case FFI_TYPE_STRUCT:
	  avalue[i] = &argp[argn];
	  break;

	case FFI_TYPE_FLOAT:
	  if (argn < 6)
	    {
	      

	      *(float *)&argp[argn - 6] = *(double *)&argp[argn - 6];
	      avalue[i] = &argp[argn - 6];
	    }
	  else
	    avalue[i] = &argp[argn];
	  break;

	case FFI_TYPE_DOUBLE:
	  avalue[i] = &argp[argn - (argn < 6 ? 6 : 0)];
	  break;

	case FFI_TYPE_LONGDOUBLE:
	  
	  avalue[i] = (long double *) argp[argn];
	  size = sizeof (long double *);
	  break;

	default:
	  abort ();
	}

      argn += ALIGN(size, FFI_SIZEOF_ARG) / FFI_SIZEOF_ARG;
      i++;
    }

  
  closure->fun (cif, rvalue, avalue, closure->user_data);

  
  return cif->rtype->type;
}

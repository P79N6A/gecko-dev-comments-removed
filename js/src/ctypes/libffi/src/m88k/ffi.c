












































#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>
#include <unistd.h>

void ffi_call_OBSD (unsigned int, extended_cif *, unsigned int, void *,
		    void (*fn) ());
void *ffi_prep_args (void *, extended_cif *);
void ffi_closure_OBSD (ffi_closure *);
void ffi_closure_struct_OBSD (ffi_closure *);
unsigned int ffi_closure_OBSD_inner (ffi_closure *, void *, unsigned int *,
				     char *);
void ffi_cacheflush_OBSD (unsigned int, unsigned int);

#define CIF_FLAGS_INT		(1 << 0)
#define CIF_FLAGS_DINT		(1 << 1)








void *
ffi_prep_args (void *stack, extended_cif *ecif)
{
  unsigned int i;
  void **p_argv;
  char *argp, *stackp;
  unsigned int *regp;
  unsigned int regused;
  ffi_type **p_arg;
  void *struct_value_ptr;

  regp = (unsigned int *)stack;
  stackp = (char *)(regp + 8);
  regused = 0;

  if (ecif->cif->rtype->type == FFI_TYPE_STRUCT
      && !ecif->cif->flags)
    struct_value_ptr = ecif->rvalue;
  else
    struct_value_ptr = NULL;

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types; i != 0; i--, p_arg++)
    {
      size_t z;
      unsigned short t, a;

      z = (*p_arg)->size;
      t = (*p_arg)->type;
      a = (*p_arg)->alignment;

      






      if (t == FFI_TYPE_STRUCT)
	{
	  if (z == sizeof (int) && a == sizeof (int) && regused < 8)
	    argp = (char *)regp;
	  else
	    argp = stackp;
	}
      else
	{
	  if (z > sizeof (int) && regused < 8 - 1)
	    {
	      
	      if (regused & 1)
		{
		  regp++;
		  regused++;
		}
	    }
	  if (regused < 8)
	    argp = (char *)regp;
	  else
	    argp = stackp;
	}

      
      if (argp == stackp && a > sizeof (int))
	{
	  stackp = (char *) ALIGN(stackp, a);
	  argp = stackp;
	}

      switch (t)
	{
	case FFI_TYPE_SINT8:
	  *(signed int *) argp = (signed int) *(SINT8 *) *p_argv;
	  break;

	case FFI_TYPE_UINT8:
	  *(unsigned int *) argp = (unsigned int) *(UINT8 *) *p_argv;
	  break;

	case FFI_TYPE_SINT16:
	  *(signed int *) argp = (signed int) *(SINT16 *) *p_argv;
	  break;

	case FFI_TYPE_UINT16:
	  *(unsigned int *) argp = (unsigned int) *(UINT16 *) *p_argv;
	  break;

	case FFI_TYPE_INT:
	case FFI_TYPE_FLOAT:
	case FFI_TYPE_UINT32:
	case FFI_TYPE_SINT32:
	case FFI_TYPE_POINTER:
	  *(unsigned int *) argp = *(unsigned int *) *p_argv;
	  break;

	case FFI_TYPE_DOUBLE:
	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	case FFI_TYPE_STRUCT:
	  memcpy (argp, *p_argv, z);
	  break;

	default:
	  FFI_ASSERT (0);
	}

      
      if ((sizeof (int) - 1) & z)
	z = ALIGN(z, sizeof (int));

      p_argv++;

      

      if (argp == (char *)regp && regused < 8)
	{
	  regp += z / sizeof (int);
	  regused += z / sizeof (int);
	}
      else
	stackp += z;
    }

  return struct_value_ptr;
}


ffi_status
ffi_prep_cif_machdep (ffi_cif *cif)
{
  
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
      cif->flags = 0;
      break;

    case FFI_TYPE_STRUCT:
      if (cif->rtype->size == sizeof (int) &&
	  cif->rtype->alignment == sizeof (int))
	cif->flags = CIF_FLAGS_INT;
      else
	cif->flags = 0;
      break;

    case FFI_TYPE_DOUBLE:
    case FFI_TYPE_SINT64:
    case FFI_TYPE_UINT64:
      cif->flags = CIF_FLAGS_DINT;
      break;

    default:
      cif->flags = CIF_FLAGS_INT;
      break;
    }

  return FFI_OK;
}

void
ffi_call (ffi_cif *cif, void (*fn) (), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  


  if (rvalue == NULL
      && cif->rtype->type == FFI_TYPE_STRUCT
      && (cif->rtype->size != sizeof (int)
	  || cif->rtype->alignment != sizeof (int)))
    ecif.rvalue = alloca (cif->rtype->size);
  else
    ecif.rvalue = rvalue;

  switch (cif->abi)
    {
    case FFI_OBSD:
      ffi_call_OBSD (cif->bytes, &ecif, cif->flags, ecif.rvalue, fn);
      break;

    default:
      FFI_ASSERT (0);
      break;
    }
}





static void
ffi_prep_closure_args_OBSD (ffi_cif *cif, void **avalue, unsigned int *regp,
			    char *stackp)
{
  unsigned int i;
  void **p_argv;
  char *argp;
  unsigned int regused;
  ffi_type **p_arg;

  regused = 0;

  p_argv = avalue;

  for (i = cif->nargs, p_arg = cif->arg_types; i != 0; i--, p_arg++)
    {
      size_t z;
      unsigned short t, a;

      z = (*p_arg)->size;
      t = (*p_arg)->type;
      a = (*p_arg)->alignment;

      






      if (t == FFI_TYPE_STRUCT)
	{
	  if (z == sizeof (int) && a == sizeof (int) && regused < 8)
	    argp = (char *)regp;
	  else
	    argp = stackp;
	}
      else
	{
	  if (z > sizeof (int) && regused < 8 - 1)
	    {
	      
	      if (regused & 1)
		{
		  regp++;
		  regused++;
		}
	    }
	  if (regused < 8)
	    argp = (char *)regp;
	  else
	    argp = stackp;
	}

      
      if (argp == stackp && a > sizeof (int))
	{
	  stackp = (char *) ALIGN(stackp, a);
	  argp = stackp;
	}

      if (z < sizeof (int) && t != FFI_TYPE_STRUCT)
	*p_argv = (void *) (argp + sizeof (int) - z);
      else
	*p_argv = (void *) argp;

      
      if ((sizeof (int) - 1) & z)
	z = ALIGN(z, sizeof (int));

      p_argv++;

      

      if (argp == (char *)regp && regused < 8)
	{
	  regp += z / sizeof (int);
	  regused += z / sizeof (int);
	}
      else
	stackp += z;
    }
}

unsigned int
ffi_closure_OBSD_inner (ffi_closure *closure, void *resp, unsigned int *regp,
			char *stackp)
{
  ffi_cif *cif;
  void **arg_area;

  cif = closure->cif;
  arg_area = (void**) alloca (cif->nargs * sizeof (void *));

  ffi_prep_closure_args_OBSD(cif, arg_area, regp, stackp);

  (closure->fun) (cif, resp, arg_area, closure->user_data);

  return cif->flags;
}

ffi_status
ffi_prep_closure_loc (ffi_closure* closure, ffi_cif* cif,
		      void (*fun)(ffi_cif*,void*,void**,void*),
		      void *user_data, void *codeloc)
{
  unsigned int *tramp = (unsigned int *) codeloc;
  void *fn;

  FFI_ASSERT (cif->abi == FFI_OBSD);

  if (cif->rtype->type == FFI_TYPE_STRUCT && !cif->flags)
    fn = &ffi_closure_struct_OBSD;
  else
    fn = &ffi_closure_OBSD;

  
  tramp[0] = 0x5d400000 | (((unsigned int)fn) >> 16);
  
  tramp[1] = 0x5da00000 | ((unsigned int)closure >> 16);
  
  tramp[2] = 0x594a0000 | (((unsigned int)fn) & 0xffff);
  
  tramp[3] = 0xf400c40a;
  
  tramp[4] = 0x59ad0000 | ((unsigned int)closure & 0xffff);

  ffi_cacheflush_OBSD((unsigned int)codeloc, FFI_TRAMPOLINE_SIZE);

  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}

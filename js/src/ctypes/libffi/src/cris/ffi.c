



























#include <ffi.h>
#include <ffi_common.h>

#define STACK_ARG_SIZE(x) ALIGN(x, FFI_SIZEOF_ARG)

static ffi_status
initialize_aggregate_packed_struct (ffi_type * arg)
{
  ffi_type **ptr;

  FFI_ASSERT (arg != NULL);

  FFI_ASSERT (arg->elements != NULL);
  FFI_ASSERT (arg->size == 0);
  FFI_ASSERT (arg->alignment == 0);

  ptr = &(arg->elements[0]);

  while ((*ptr) != NULL)
    {
      if (((*ptr)->size == 0)
	  && (initialize_aggregate_packed_struct ((*ptr)) != FFI_OK))
	return FFI_BAD_TYPEDEF;

      FFI_ASSERT (ffi_type_test ((*ptr)));

      arg->size += (*ptr)->size;

      arg->alignment = (arg->alignment > (*ptr)->alignment) ?
	arg->alignment : (*ptr)->alignment;

      ptr++;
    }

  if (arg->size == 0)
    return FFI_BAD_TYPEDEF;
  else
    return FFI_OK;
}

int
ffi_prep_args (char *stack, extended_cif * ecif)
{
  unsigned int i;
  unsigned int struct_count = 0;
  void **p_argv;
  char *argp;
  ffi_type **p_arg;

  argp = stack;

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       (i != 0); i--, p_arg++)
    {
      size_t z;

      switch ((*p_arg)->type)
	{
	case FFI_TYPE_STRUCT:
	  {
	    z = (*p_arg)->size;
	    if (z <= 4)
	      {
		memcpy (argp, *p_argv, z);
		z = 4;
	      }
	    else if (z <= 8)
	      {
		memcpy (argp, *p_argv, z);
		z = 8;
	      }
	    else
	      {
		unsigned int uiLocOnStack;
		z = sizeof (void *);
		uiLocOnStack = 4 * ecif->cif->nargs + struct_count;
		struct_count = struct_count + (*p_arg)->size;
		*(unsigned int *) argp =
		  (unsigned int) (UINT32 *) (stack + uiLocOnStack);
		memcpy ((stack + uiLocOnStack), *p_argv, (*p_arg)->size);
	      }
	    break;
	  }
	default:
	  z = (*p_arg)->size;
	  if (z < sizeof (int))
	    {
	      switch ((*p_arg)->type)
		{
		case FFI_TYPE_SINT8:
		  *(signed int *) argp = (signed int) *(SINT8 *) (*p_argv);
		  break;

		case FFI_TYPE_UINT8:
		  *(unsigned int *) argp =
		    (unsigned int) *(UINT8 *) (*p_argv);
		  break;

		case FFI_TYPE_SINT16:
		  *(signed int *) argp = (signed int) *(SINT16 *) (*p_argv);
		  break;

		case FFI_TYPE_UINT16:
		  *(unsigned int *) argp =
		    (unsigned int) *(UINT16 *) (*p_argv);
		  break;

		default:
		  FFI_ASSERT (0);
		}
	      z = sizeof (int);
	    }
	  else if (z == sizeof (int))
	    *(unsigned int *) argp = (unsigned int) *(UINT32 *) (*p_argv);
	  else
	    memcpy (argp, *p_argv, z);
	  break;
	}
      p_argv++;
      argp += z;
    }

  return (struct_count);
}

ffi_status FFI_HIDDEN
ffi_prep_cif_core (ffi_cif * cif,
	           ffi_abi abi, unsigned int isvariadic,
		   unsigned int nfixedargs, unsigned int ntotalargs,
	           ffi_type * rtype, ffi_type ** atypes)
{
  unsigned bytes = 0;
  unsigned int i;
  ffi_type **ptr;

  FFI_ASSERT (cif != NULL);
  FFI_ASSERT((!isvariadic) || (nfixedargs >= 1));
  FFI_ASSERT(nfixedargs <= ntotalargs);
  FFI_ASSERT (abi > FFI_FIRST_ABI && abi < FFI_LAST_ABI);

  cif->abi = abi;
  cif->arg_types = atypes;
  cif->nargs = ntotalargs;
  cif->rtype = rtype;

  cif->flags = 0;

  if ((cif->rtype->size == 0)
      && (initialize_aggregate_packed_struct (cif->rtype) != FFI_OK))
    return FFI_BAD_TYPEDEF;

  FFI_ASSERT_VALID_TYPE (cif->rtype);

  for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++)
    {
      if (((*ptr)->size == 0)
	  && (initialize_aggregate_packed_struct ((*ptr)) != FFI_OK))
	return FFI_BAD_TYPEDEF;

      FFI_ASSERT_VALID_TYPE (*ptr);

      if (((*ptr)->alignment - 1) & bytes)
	bytes = ALIGN (bytes, (*ptr)->alignment);
      if ((*ptr)->type == FFI_TYPE_STRUCT)
	{
	  if ((*ptr)->size > 8)
	    {
	      bytes += (*ptr)->size;
	      bytes += sizeof (void *);
	    }
	  else
	    {
	      if ((*ptr)->size > 4)
		bytes += 8;
	      else
		bytes += 4;
	    }
	}
      else
	bytes += STACK_ARG_SIZE ((*ptr)->size);
    }

  cif->bytes = bytes;

  return ffi_prep_cif_machdep (cif);
}

ffi_status
ffi_prep_cif_machdep (ffi_cif * cif)
{
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
    case FFI_TYPE_STRUCT:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
    case FFI_TYPE_SINT64:
    case FFI_TYPE_UINT64:
      cif->flags = (unsigned) cif->rtype->type;
      break;

    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }

  return FFI_OK;
}

extern void ffi_call_SYSV (int (*)(char *, extended_cif *),
			   extended_cif *,
			   unsigned, unsigned, unsigned *, void (*fn) ())
     __attribute__ ((__visibility__ ("hidden")));

void
ffi_call (ffi_cif * cif, void (*fn) (), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  if ((rvalue == NULL) && (cif->rtype->type == FFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca (cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;

  switch (cif->abi)
    {
    case FFI_SYSV:
      ffi_call_SYSV (ffi_prep_args, &ecif, cif->bytes,
		     cif->flags, ecif.rvalue, fn);
      break;
    default:
      FFI_ASSERT (0);
      break;
    }
}





extern const char ffi_cris_trampoline_template[]
 __attribute__ ((__visibility__ ("hidden")));



extern const int ffi_cris_trampoline_fn_offset
 __attribute__ ((__visibility__ ("hidden")));



extern const int ffi_cris_trampoline_closure_offset
 __attribute__ ((__visibility__ ("hidden")));








static unsigned long long
ffi_prep_closure_inner (void **params, ffi_closure* closure)
{
  char *register_args = (char *) params;
  void *struct_ret = params[5];
  char *stack_args = params[6];
  char *ptr = register_args;
  ffi_cif *cif = closure->cif;
  ffi_type **arg_types = cif->arg_types;

  
  void **avalue = alloca (closure->cif->nargs * sizeof(void *));
  int i;
  int doing_regs;
  long long llret = 0;

  
  for (i = 0, doing_regs = 1; i < cif->nargs; i++)
    {
      
      if (arg_types[i]->size <= 4)
	{
	  avalue[i] = ptr;
	  ptr += 4;
	}
      else if (arg_types[i]->size <= 8)
	{
	  avalue[i] = ptr;
	  ptr += 8;
	}
      else
	{
	  FFI_ASSERT (arg_types[i]->type == FFI_TYPE_STRUCT);

	  
	  avalue[i] = *(void **) ptr;
	  ptr += 4;
	}

      


      if (doing_regs && ptr >= register_args + 4*4)
	{
	  ptr = stack_args + ((ptr > register_args + 4*4) ? 4 : 0);
	  doing_regs = 0;
	}
    }

  
  (closure->fun) (cif,

		  cif->rtype->type == FFI_TYPE_STRUCT
		  


		  ? struct_ret

		  


		  : (void *) &llret,

		  avalue, closure->user_data);

  return llret;
}



ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif *, void *, void **, void*),
		      void *user_data,
		      void *codeloc)
{
  void *innerfn = ffi_prep_closure_inner;
  FFI_ASSERT (cif->abi == FFI_SYSV);
  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;
  memcpy (closure->tramp, ffi_cris_trampoline_template,
	  FFI_CRIS_TRAMPOLINE_CODE_PART_SIZE);
  memcpy (closure->tramp + ffi_cris_trampoline_fn_offset,
	  &innerfn, sizeof (void *));
  memcpy (closure->tramp + ffi_cris_trampoline_closure_offset,
	  &codeloc, sizeof (void *));

  return FFI_OK;
}

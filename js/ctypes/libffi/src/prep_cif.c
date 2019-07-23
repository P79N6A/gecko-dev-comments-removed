























#include <ffi.h>
#include <ffi_common.h>
#include <stdlib.h>



#define STACK_ARG_SIZE(x) ALIGN(x, FFI_SIZEOF_ARG)




static ffi_status initialize_aggregate(ffi_type *arg)
{
  ffi_type **ptr;

  FFI_ASSERT(arg != NULL);

  FFI_ASSERT(arg->elements != NULL);
  FFI_ASSERT(arg->size == 0);
  FFI_ASSERT(arg->alignment == 0);

  ptr = &(arg->elements[0]);

  while ((*ptr) != NULL)
    {
      if (((*ptr)->size == 0) && (initialize_aggregate((*ptr)) != FFI_OK))
	return FFI_BAD_TYPEDEF;

      
      FFI_ASSERT_VALID_TYPE(*ptr);

      arg->size = ALIGN(arg->size, (*ptr)->alignment);
      arg->size += (*ptr)->size;

      arg->alignment = (arg->alignment > (*ptr)->alignment) ?
	arg->alignment : (*ptr)->alignment;

      ptr++;
    }

  






  arg->size = ALIGN (arg->size, arg->alignment);

  if (arg->size == 0)
    return FFI_BAD_TYPEDEF;
  else
    return FFI_OK;
}

#ifndef __CRIS__







ffi_status ffi_prep_cif(ffi_cif *cif, ffi_abi abi, unsigned int nargs,
			ffi_type *rtype, ffi_type **atypes)
{
  unsigned bytes = 0;
  unsigned int i;
  ffi_type **ptr;

  FFI_ASSERT(cif != NULL);
  FFI_ASSERT((abi > FFI_FIRST_ABI) && (abi <= FFI_DEFAULT_ABI));

  cif->abi = abi;
  cif->arg_types = atypes;
  cif->nargs = nargs;
  cif->rtype = rtype;

  cif->flags = 0;

  
  if ((cif->rtype->size == 0) && (initialize_aggregate(cif->rtype) != FFI_OK))
    return FFI_BAD_TYPEDEF;

  
  FFI_ASSERT_VALID_TYPE(cif->rtype);

  
#if !defined M68K && !defined __x86_64__ && !defined S390 && !defined PA
  
  if (cif->rtype->type == FFI_TYPE_STRUCT
#ifdef SPARC
      && (cif->abi != FFI_V9 || cif->rtype->size > 32)
#endif
#ifdef X86_DARWIN
      && (cif->rtype->size > 8)
#endif
     )
    bytes = STACK_ARG_SIZE(sizeof(void*));
#endif

  for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++)
    {

      
      if (((*ptr)->size == 0) && (initialize_aggregate((*ptr)) != FFI_OK))
	return FFI_BAD_TYPEDEF;

      

      FFI_ASSERT_VALID_TYPE(*ptr);

#if !defined __x86_64__ && !defined S390 && !defined PA
#ifdef SPARC
      if (((*ptr)->type == FFI_TYPE_STRUCT
	   && ((*ptr)->size > 16 || cif->abi != FFI_V9))
	  || ((*ptr)->type == FFI_TYPE_LONGDOUBLE
	      && cif->abi != FFI_V9))
	bytes += sizeof(void*);
      else
#endif
	{
	  
	  if (((*ptr)->alignment - 1) & bytes)
	    bytes = ALIGN(bytes, (*ptr)->alignment);

	  bytes += STACK_ARG_SIZE((*ptr)->size);
	}
#endif
    }

  cif->bytes = bytes;

  
  return ffi_prep_cif_machdep(cif);
}
#endif 

#if FFI_CLOSURES

ffi_status
ffi_prep_closure (ffi_closure* closure,
		  ffi_cif* cif,
		  void (*fun)(ffi_cif*,void*,void**,void*),
		  void *user_data)
{
  return ffi_prep_closure_loc (closure, cif, fun, user_data, closure);
}

#endif

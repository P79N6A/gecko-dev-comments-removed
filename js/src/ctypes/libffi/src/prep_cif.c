
























#include <ffi.h>
#include <ffi_common.h>
#include <stdlib.h>



#define STACK_ARG_SIZE(x) ALIGN(x, FFI_SIZEOF_ARG)




static ffi_status initialize_aggregate(ffi_type *arg)
{
  ffi_type **ptr;

  if (UNLIKELY(arg == NULL || arg->elements == NULL))
    return FFI_BAD_TYPEDEF;

  arg->size = 0;
  arg->alignment = 0;

  ptr = &(arg->elements[0]);

  if (UNLIKELY(ptr == 0))
    return FFI_BAD_TYPEDEF;

  while ((*ptr) != NULL)
    {
      if (UNLIKELY(((*ptr)->size == 0)
		    && (initialize_aggregate((*ptr)) != FFI_OK)))
	return FFI_BAD_TYPEDEF;

      
      FFI_ASSERT_VALID_TYPE(*ptr);

      arg->size = ALIGN(arg->size, (*ptr)->alignment);
      arg->size += (*ptr)->size;

      arg->alignment = (arg->alignment > (*ptr)->alignment) ?
	arg->alignment : (*ptr)->alignment;

      ptr++;
    }

  






  arg->size = ALIGN (arg->size, arg->alignment);

  

#ifdef FFI_AGGREGATE_ALIGNMENT
  if (FFI_AGGREGATE_ALIGNMENT > arg->alignment)
    arg->alignment = FFI_AGGREGATE_ALIGNMENT;
#endif

  if (arg->size == 0)
    return FFI_BAD_TYPEDEF;
  else
    return FFI_OK;
}

#ifndef __CRIS__














ffi_status FFI_HIDDEN ffi_prep_cif_core(ffi_cif *cif, ffi_abi abi,
			     unsigned int isvariadic,
                             unsigned int nfixedargs,
                             unsigned int ntotalargs,
			     ffi_type *rtype, ffi_type **atypes)
{
  unsigned bytes = 0;
  unsigned int i;
  ffi_type **ptr;

  FFI_ASSERT(cif != NULL);
  FFI_ASSERT((!isvariadic) || (nfixedargs >= 1));
  FFI_ASSERT(nfixedargs <= ntotalargs);

  if (! (abi > FFI_FIRST_ABI && abi < FFI_LAST_ABI))
    return FFI_BAD_ABI;

  cif->abi = abi;
  cif->arg_types = atypes;
  cif->nargs = ntotalargs;
  cif->rtype = rtype;

  cif->flags = 0;

#if HAVE_LONG_DOUBLE_VARIANT
  ffi_prep_types (abi);
#endif

  
  if ((cif->rtype->size == 0) && (initialize_aggregate(cif->rtype) != FFI_OK))
    return FFI_BAD_TYPEDEF;

  
  FFI_ASSERT_VALID_TYPE(cif->rtype);

  
#if !defined M68K && !defined X86_ANY && !defined S390 && !defined PA
  
  if (cif->rtype->type == FFI_TYPE_STRUCT
#ifdef SPARC
      && (cif->abi != FFI_V9 || cif->rtype->size > 32)
#endif
#ifdef TILE
      && (cif->rtype->size > 10 * FFI_SIZEOF_ARG)
#endif
#ifdef XTENSA
      && (cif->rtype->size > 16)
#endif
#ifdef NIOS2
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

#if !defined X86_ANY && !defined S390 && !defined PA
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
	    bytes = (unsigned)ALIGN(bytes, (*ptr)->alignment);

#ifdef TILE
	  if (bytes < 10 * FFI_SIZEOF_ARG &&
	      bytes + STACK_ARG_SIZE((*ptr)->size) > 10 * FFI_SIZEOF_ARG)
	    {
	      

	      bytes = 10 * FFI_SIZEOF_ARG;
	    }
#endif
#ifdef XTENSA
	  if (bytes <= 6*4 && bytes + STACK_ARG_SIZE((*ptr)->size) > 6*4)
	    bytes = 6*4;
#endif

	  bytes += STACK_ARG_SIZE((*ptr)->size);
	}
#endif
    }

  cif->bytes = bytes;

  
#ifdef FFI_TARGET_SPECIFIC_VARIADIC
  if (isvariadic)
	return ffi_prep_cif_machdep_var(cif, nfixedargs, ntotalargs);
#endif

  return ffi_prep_cif_machdep(cif);
}
#endif 

ffi_status ffi_prep_cif(ffi_cif *cif, ffi_abi abi, unsigned int nargs,
			     ffi_type *rtype, ffi_type **atypes)
{
  return ffi_prep_cif_core(cif, abi, 0, nargs, nargs, rtype, atypes);
}

ffi_status ffi_prep_cif_var(ffi_cif *cif,
                            ffi_abi abi,
                            unsigned int nfixedargs,
                            unsigned int ntotalargs,
                            ffi_type *rtype,
                            ffi_type **atypes)
{
  return ffi_prep_cif_core(cif, abi, 1, nfixedargs, ntotalargs, rtype, atypes);
}

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

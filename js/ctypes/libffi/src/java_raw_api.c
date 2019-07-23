




































#include <ffi.h>
#include <ffi_common.h>
#include <stdlib.h>

#if !defined(NO_JAVA_RAW_API) && !defined(FFI_NO_RAW_API)

size_t
ffi_java_raw_size (ffi_cif *cif)
{
  size_t result = 0;
  int i;

  ffi_type **at = cif->arg_types;

  for (i = cif->nargs-1; i >= 0; i--, at++)
    {
      switch((*at) -> type) {
	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	case FFI_TYPE_DOUBLE:
	  result += 2 * FFI_SIZEOF_JAVA_RAW;
	  break;
	case FFI_TYPE_STRUCT:
	  
	  abort();
	default:
	  result += FFI_SIZEOF_JAVA_RAW;
      }
    }

  return result;
}


void
ffi_java_raw_to_ptrarray (ffi_cif *cif, ffi_java_raw *raw, void **args)
{
  unsigned i;
  ffi_type **tp = cif->arg_types;

#if WORDS_BIGENDIAN

  for (i = 0; i < cif->nargs; i++, tp++, args++)
    {
      switch ((*tp)->type)
	{
	case FFI_TYPE_UINT8:
	case FFI_TYPE_SINT8:
	  *args = (void*) ((char*)(raw++) + 3);
	  break;

	case FFI_TYPE_UINT16:
	case FFI_TYPE_SINT16:
	  *args = (void*) ((char*)(raw++) + 2);
	  break;

#if FFI_SIZEOF_JAVA_RAW == 8
	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	case FFI_TYPE_DOUBLE:
	  *args = (void *)raw;
	  raw += 2;
	  break;
#endif

	case FFI_TYPE_POINTER:
	  *args = (void*) &(raw++)->ptr;
	  break;

	default:
	  *args = raw;
	  raw +=
	    ALIGN ((*tp)->size, sizeof(ffi_java_raw)) / sizeof(ffi_java_raw);
	}
    }

#else 

#if !PDP

  
  for (i = 0; i < cif->nargs; i++, tp++, args++)
    {
#if FFI_SIZEOF_JAVA_RAW == 8
      switch((*tp)->type) {
	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	case FFI_TYPE_DOUBLE:
	  *args = (void*) raw;
	  raw += 2;
	  break;
	default:
	  *args = (void*) raw++;
      }
#else 
	*args = (void*) raw;
	raw +=
	  ALIGN ((*tp)->size, sizeof(ffi_java_raw)) / sizeof(ffi_java_raw);
#endif 
    }

#else
#error "pdp endian not supported"
#endif 

#endif 
}

void
ffi_java_ptrarray_to_raw (ffi_cif *cif, void **args, ffi_java_raw *raw)
{
  unsigned i;
  ffi_type **tp = cif->arg_types;

  for (i = 0; i < cif->nargs; i++, tp++, args++)
    {
      switch ((*tp)->type)
	{
	case FFI_TYPE_UINT8:
#if WORDS_BIGENDIAN
	  *(UINT32*)(raw++) = *(UINT8*) (*args);
#else
	  (raw++)->uint = *(UINT8*) (*args);
#endif
	  break;

	case FFI_TYPE_SINT8:
#if WORDS_BIGENDIAN
	  *(SINT32*)(raw++) = *(SINT8*) (*args);
#else
	  (raw++)->sint = *(SINT8*) (*args);
#endif
	  break;

	case FFI_TYPE_UINT16:
#if WORDS_BIGENDIAN
	  *(UINT32*)(raw++) = *(UINT16*) (*args);
#else
	  (raw++)->uint = *(UINT16*) (*args);
#endif
	  break;

	case FFI_TYPE_SINT16:
#if WORDS_BIGENDIAN
	  *(SINT32*)(raw++) = *(SINT16*) (*args);
#else
	  (raw++)->sint = *(SINT16*) (*args);
#endif
	  break;

	case FFI_TYPE_UINT32:
#if WORDS_BIGENDIAN
	  *(UINT32*)(raw++) = *(UINT32*) (*args);
#else
	  (raw++)->uint = *(UINT32*) (*args);
#endif
	  break;

	case FFI_TYPE_SINT32:
#if WORDS_BIGENDIAN
	  *(SINT32*)(raw++) = *(SINT32*) (*args);
#else
	  (raw++)->sint = *(SINT32*) (*args);
#endif
	  break;

	case FFI_TYPE_FLOAT:
	  (raw++)->flt = *(FLOAT32*) (*args);
	  break;

#if FFI_SIZEOF_JAVA_RAW == 8
	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	case FFI_TYPE_DOUBLE:
	  raw->uint = *(UINT64*) (*args);
	  raw += 2;
	  break;
#endif

	case FFI_TYPE_POINTER:
	  (raw++)->ptr = **(void***) args;
	  break;

	default:
#if FFI_SIZEOF_JAVA_RAW == 8
	  FFI_ASSERT(0);	
#else
	  memcpy ((void*) raw->data, (void*)*args, (*tp)->size);
	  raw +=
	    ALIGN ((*tp)->size, sizeof(ffi_java_raw)) / sizeof(ffi_java_raw);
#endif
	}
    }
}

#if !FFI_NATIVE_RAW_API

static void
ffi_java_rvalue_to_raw (ffi_cif *cif, void *rvalue)
{
#if WORDS_BIGENDIAN && FFI_SIZEOF_ARG == 8
  switch (cif->rtype->type)
    {
    case FFI_TYPE_UINT8:
    case FFI_TYPE_UINT16:
    case FFI_TYPE_UINT32:
      *(UINT64 *)rvalue <<= 32;
      break;

    case FFI_TYPE_SINT8:
    case FFI_TYPE_SINT16:
    case FFI_TYPE_SINT32:
    case FFI_TYPE_INT:
#if FFI_SIZEOF_JAVA_RAW == 4
    case FFI_TYPE_POINTER:
#endif
      *(SINT64 *)rvalue <<= 32;
      break;

    default:
      break;
    }
#endif
}

static void
ffi_java_raw_to_rvalue (ffi_cif *cif, void *rvalue)
{
#if WORDS_BIGENDIAN && FFI_SIZEOF_ARG == 8
  switch (cif->rtype->type)
    {
    case FFI_TYPE_UINT8:
    case FFI_TYPE_UINT16:
    case FFI_TYPE_UINT32:
      *(UINT64 *)rvalue >>= 32;
      break;

    case FFI_TYPE_SINT8:
    case FFI_TYPE_SINT16:
    case FFI_TYPE_SINT32:
    case FFI_TYPE_INT:
#if FFI_SIZEOF_JAVA_RAW == 4
    case FFI_TYPE_POINTER:
#endif
      *(SINT64 *)rvalue >>= 32;
      break;

    default:
      break;
    }
#endif
}








void ffi_java_raw_call (ffi_cif *cif, void (*fn)(void), void *rvalue,
			ffi_java_raw *raw)
{
  void **avalue = (void**) alloca (cif->nargs * sizeof (void*));
  ffi_java_raw_to_ptrarray (cif, raw, avalue);
  ffi_call (cif, fn, rvalue, avalue);
  ffi_java_rvalue_to_raw (cif, rvalue);
}

#if FFI_CLOSURES		

static void
ffi_java_translate_args (ffi_cif *cif, void *rvalue,
		    void **avalue, void *user_data)
{
  ffi_java_raw *raw = (ffi_java_raw*)alloca (ffi_java_raw_size (cif));
  ffi_raw_closure *cl = (ffi_raw_closure*)user_data;

  ffi_java_ptrarray_to_raw (cif, avalue, raw);
  (*cl->fun) (cif, rvalue, raw, cl->user_data);
  ffi_java_raw_to_rvalue (cif, rvalue);
}

ffi_status
ffi_prep_java_raw_closure_loc (ffi_java_raw_closure* cl,
			       ffi_cif *cif,
			       void (*fun)(ffi_cif*,void*,ffi_java_raw*,void*),
			       void *user_data,
			       void *codeloc)
{
  ffi_status status;

  status = ffi_prep_closure_loc ((ffi_closure*) cl,
				 cif,
				 &ffi_java_translate_args,
				 codeloc,
				 codeloc);
  if (status == FFI_OK)
    {
      cl->fun       = fun;
      cl->user_data = user_data;
    }

  return status;
}





ffi_status
ffi_prep_java_raw_closure (ffi_java_raw_closure* cl,
			   ffi_cif *cif,
			   void (*fun)(ffi_cif*,void*,ffi_java_raw*,void*),
			   void *user_data)
{
  return ffi_prep_java_raw_closure_loc (cl, cif, fun, user_data, cl);
}

#endif 
#endif 
#endif 

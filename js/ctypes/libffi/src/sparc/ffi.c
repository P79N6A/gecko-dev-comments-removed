

























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>





void ffi_prep_args_v8(char *stack, extended_cif *ecif)
{
  int i;
  void **p_argv;
  char *argp;
  ffi_type **p_arg;

  
  argp = stack + 16*sizeof(int);

  



  *(int *) argp = (long)ecif->rvalue;

  
  argp += sizeof(int);

#ifdef USING_PURIFY
  


  ((int*)argp)[0] = 0;
  ((int*)argp)[1] = 0;
  ((int*)argp)[2] = 0;
  ((int*)argp)[3] = 0;
  ((int*)argp)[4] = 0;
  ((int*)argp)[5] = 0;
#endif

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types; i; i--, p_arg++)
    {
      size_t z;

	  if ((*p_arg)->type == FFI_TYPE_STRUCT
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	      || (*p_arg)->type == FFI_TYPE_LONGDOUBLE
#endif
	      )
	    {
	      *(unsigned int *) argp = (unsigned long)(* p_argv);
	      z = sizeof(int);
	    }
	  else
	    {
	      z = (*p_arg)->size;
	      if (z < sizeof(int))
		{
		  z = sizeof(int);
		  switch ((*p_arg)->type)
		    {
		    case FFI_TYPE_SINT8:
		      *(signed int *) argp = *(SINT8 *)(* p_argv);
		      break;
		      
		    case FFI_TYPE_UINT8:
		      *(unsigned int *) argp = *(UINT8 *)(* p_argv);
		      break;
		      
		    case FFI_TYPE_SINT16:
		      *(signed int *) argp = *(SINT16 *)(* p_argv);
		      break;
		      
		    case FFI_TYPE_UINT16:
		      *(unsigned int *) argp = *(UINT16 *)(* p_argv);
		      break;

		    default:
		      FFI_ASSERT(0);
		    }
		}
	      else
		{
		  memcpy(argp, *p_argv, z);
		}
	    }
	  p_argv++;
	  argp += z;
    }
  
  return;
}

int ffi_prep_args_v9(char *stack, extended_cif *ecif)
{
  int i, ret = 0;
  int tmp;
  void **p_argv;
  char *argp;
  ffi_type **p_arg;

  tmp = 0;

  
  argp = stack + 16*sizeof(long long);

#ifdef USING_PURIFY
  


  ((long long*)argp)[0] = 0;
  ((long long*)argp)[1] = 0;
  ((long long*)argp)[2] = 0;
  ((long long*)argp)[3] = 0;
  ((long long*)argp)[4] = 0;
  ((long long*)argp)[5] = 0;
#endif

  p_argv = ecif->avalue;

  if (ecif->cif->rtype->type == FFI_TYPE_STRUCT &&
      ecif->cif->rtype->size > 32)
    {
      *(unsigned long long *) argp = (unsigned long)ecif->rvalue;
      argp += sizeof(long long);
      tmp = 1;
    }

  for (i = 0, p_arg = ecif->cif->arg_types; i < ecif->cif->nargs;
       i++, p_arg++)
    {
      size_t z;

      z = (*p_arg)->size;
      switch ((*p_arg)->type)
	{
	case FFI_TYPE_STRUCT:
	  if (z > 16)
	    {
	      
	      *(unsigned long long *) argp = (unsigned long)* p_argv;
	      argp += sizeof(long long);
	      tmp++;
	      p_argv++;
	      continue;
	    }
	  
	case FFI_TYPE_FLOAT:
	case FFI_TYPE_DOUBLE:
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
#endif
	  ret = 1; 
	  break;
	}
      if (z < sizeof(long long))
	{
	  switch ((*p_arg)->type)
	    {
	    case FFI_TYPE_SINT8:
	      *(signed long long *) argp = *(SINT8 *)(* p_argv);
	      break;

	    case FFI_TYPE_UINT8:
	      *(unsigned long long *) argp = *(UINT8 *)(* p_argv);
	      break;

	    case FFI_TYPE_SINT16:
	      *(signed long long *) argp = *(SINT16 *)(* p_argv);
	      break;

	    case FFI_TYPE_UINT16:
	      *(unsigned long long *) argp = *(UINT16 *)(* p_argv);
	      break;

	    case FFI_TYPE_SINT32:
	      *(signed long long *) argp = *(SINT32 *)(* p_argv);
	      break;

	    case FFI_TYPE_UINT32:
	      *(unsigned long long *) argp = *(UINT32 *)(* p_argv);
	      break;

	    case FFI_TYPE_FLOAT:
	      *(float *) (argp + 4) = *(FLOAT32 *)(* p_argv); 
	      break;

	    case FFI_TYPE_STRUCT:
	      memcpy(argp, *p_argv, z);
	      break;

	    default:
	      FFI_ASSERT(0);
	    }
	  z = sizeof(long long);
	  tmp++;
	}
      else if (z == sizeof(long long))
	{
	  memcpy(argp, *p_argv, z);
	  z = sizeof(long long);
	  tmp++;
	}
      else
	{
	  if ((tmp & 1) && (*p_arg)->alignment > 8)
	    {
	      tmp++;
	      argp += sizeof(long long);
	    }
	  memcpy(argp, *p_argv, z);
	  z = 2 * sizeof(long long);
	  tmp += 2;
	}
      p_argv++;
      argp += z;
    }

  return ret;
}


ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  int wordsize;

  if (cif->abi != FFI_V9)
    {
      wordsize = 4;

      


      if (cif->rtype->type != FFI_TYPE_STRUCT)
	cif->bytes += wordsize;

      

  
      if (cif->bytes < 4*6+4)
	cif->bytes = 4*6+4;
    }
  else
    {
      wordsize = 8;

      

  
      if (cif->bytes < 8*6)
	cif->bytes = 8*6;
    }

  


  cif->bytes += 16 * wordsize;

  


  cif->bytes = ALIGN(cif->bytes, 2 * wordsize);

  
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
    case FFI_TYPE_LONGDOUBLE:
#endif
      cif->flags = cif->rtype->type;
      break;

    case FFI_TYPE_STRUCT:
      if (cif->abi == FFI_V9 && cif->rtype->size > 32)
	cif->flags = FFI_TYPE_VOID;
      else
	cif->flags = FFI_TYPE_STRUCT;
      break;

    case FFI_TYPE_SINT64:
    case FFI_TYPE_UINT64:
      if (cif->abi != FFI_V9)
	{
	  cif->flags = FFI_TYPE_SINT64;
	  break;
	}
      
    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }
  return FFI_OK;
}

int ffi_v9_layout_struct(ffi_type *arg, int off, char *ret, char *intg, char *flt)
{
  ffi_type **ptr = &arg->elements[0];

  while (*ptr != NULL)
    {
      if (off & ((*ptr)->alignment - 1))
	off = ALIGN(off, (*ptr)->alignment);

      switch ((*ptr)->type)
	{
	case FFI_TYPE_STRUCT:
	  off = ffi_v9_layout_struct(*ptr, off, ret, intg, flt);
	  off = ALIGN(off, FFI_SIZEOF_ARG);
	  break;
	case FFI_TYPE_FLOAT:
	case FFI_TYPE_DOUBLE:
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
#endif
	  memmove(ret + off, flt + off, (*ptr)->size);
	  off += (*ptr)->size;
	  break;
	default:
	  memmove(ret + off, intg + off, (*ptr)->size);
	  off += (*ptr)->size;
	  break;
	}
      ptr++;
    }
  return off;
}


#ifdef SPARC64
extern int ffi_call_v9(void *, extended_cif *, unsigned, 
		       unsigned, unsigned *, void (*fn)(void));
#else
extern int ffi_call_v8(void *, extended_cif *, unsigned, 
		       unsigned, unsigned *, void (*fn)(void));
#endif

void ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;
  void *rval = rvalue;

  ecif.cif = cif;
  ecif.avalue = avalue;

  
  

  ecif.rvalue = rvalue;
  if (cif->rtype->type == FFI_TYPE_STRUCT)
    {
      if (cif->rtype->size <= 32)
	rval = alloca(64);
      else
	{
	  rval = NULL;
	  if (rvalue == NULL)
	    ecif.rvalue = alloca(cif->rtype->size);
	}
    }

  switch (cif->abi) 
    {
    case FFI_V8:
#ifdef SPARC64
      
      FFI_ASSERT(0);
#else
      ffi_call_v8(ffi_prep_args_v8, &ecif, cif->bytes, 
		  cif->flags, rvalue, fn);
#endif
      break;
    case FFI_V9:
#ifdef SPARC64
      ffi_call_v9(ffi_prep_args_v9, &ecif, cif->bytes,
		  cif->flags, rval, fn);
      if (rvalue && rval && cif->rtype->type == FFI_TYPE_STRUCT)
	ffi_v9_layout_struct(cif->rtype, 0, (char *)rvalue, (char *)rval, ((char *)rval)+32);
#else
      
      FFI_ASSERT(0);
#endif
      break;
    default:
      FFI_ASSERT(0);
      break;
    }

}


#ifdef SPARC64
extern void ffi_closure_v9(void);
#else
extern void ffi_closure_v8(void);
#endif

ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned int *tramp = (unsigned int *) &closure->tramp[0];
  unsigned long fn;
#ifdef SPARC64
  

  FFI_ASSERT (cif->abi == FFI_V9);
  fn = (unsigned long) ffi_closure_v9;
  tramp[0] = 0x83414000;	
  tramp[1] = 0xca586010;	
  tramp[2] = 0x81c14000;	
  tramp[3] = 0x01000000;	
  *((unsigned long *) &tramp[4]) = fn;
#else
  unsigned long ctx = (unsigned long) codeloc;
  FFI_ASSERT (cif->abi == FFI_V8);
  fn = (unsigned long) ffi_closure_v8;
  tramp[0] = 0x03000000 | fn >> 10;	
  tramp[1] = 0x05000000 | ctx >> 10;	
  tramp[2] = 0x81c06000 | (fn & 0x3ff);	
  tramp[3] = 0x8410a000 | (ctx & 0x3ff);
#endif

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  
#ifdef SPARC64
  asm volatile ("flush	%0" : : "r" (closure) : "memory");
  asm volatile ("flush	%0" : : "r" (((char *) closure) + 8) : "memory");
#else
  asm volatile ("iflush	%0" : : "r" (closure) : "memory");
  asm volatile ("iflush	%0" : : "r" (((char *) closure) + 8) : "memory");
#endif

  return FFI_OK;
}

int
ffi_closure_sparc_inner_v8(ffi_closure *closure,
  void *rvalue, unsigned long *gpr, unsigned long *scratch)
{
  ffi_cif *cif;
  ffi_type **arg_types;
  void **avalue;
  int i, argn;

  cif = closure->cif;
  arg_types = cif->arg_types;
  avalue = alloca(cif->nargs * sizeof(void *));

  

  if (cif->flags == FFI_TYPE_STRUCT
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE  
      || cif->flags == FFI_TYPE_LONGDOUBLE
#endif
     )
    rvalue = (void *) gpr[0];

  
  argn = 1;

  
  for (i = 0; i < cif->nargs; i++)
    {
      if (arg_types[i]->type == FFI_TYPE_STRUCT
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	  || arg_types[i]->type == FFI_TYPE_LONGDOUBLE
#endif
         )
	{
	  
	  avalue[i] = (void *)gpr[argn++];
	}
      else if ((arg_types[i]->type == FFI_TYPE_DOUBLE
	       || arg_types[i]->type == FFI_TYPE_SINT64
	       || arg_types[i]->type == FFI_TYPE_UINT64)
	       
	       && (argn % 2) != 0)
	{
	  
	  scratch[0] = gpr[argn];
	  scratch[1] = gpr[argn+1];
	  avalue[i] = scratch;
	  scratch -= 2;
	  argn += 2;
	}
      else
	{
	  
	  argn += ALIGN(arg_types[i]->size, FFI_SIZEOF_ARG) / FFI_SIZEOF_ARG;
	  avalue[i] = ((char *) &gpr[argn]) - arg_types[i]->size;
	}
    }

  
  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  
  return cif->rtype->type;
}

int
ffi_closure_sparc_inner_v9(ffi_closure *closure,
  void *rvalue, unsigned long *gpr, double *fpr)
{
  ffi_cif *cif;
  ffi_type **arg_types;
  void **avalue;
  int i, argn, fp_slot_max;

  cif = closure->cif;
  arg_types = cif->arg_types;
  avalue = alloca(cif->nargs * sizeof(void *));

  

  if (cif->flags == FFI_TYPE_VOID
      && cif->rtype->type == FFI_TYPE_STRUCT)
    {
      rvalue = (void *) gpr[0];
      
      argn = 1;
    }
  else
    argn = 0;

  fp_slot_max = 16 - argn;

  
  for (i = 0; i < cif->nargs; i++)
    {
      if (arg_types[i]->type == FFI_TYPE_STRUCT)
	{
	  if (arg_types[i]->size > 16)
	    {
	      
	      avalue[i] = (void *)gpr[argn++];
	    }
	  else
	    {
	      
	      ffi_v9_layout_struct(arg_types[i],
				   0,
				   (char *) &gpr[argn],
				   (char *) &gpr[argn],
				   (char *) &fpr[argn]);
	      avalue[i] = &gpr[argn];
	      argn += ALIGN(arg_types[i]->size, FFI_SIZEOF_ARG) / FFI_SIZEOF_ARG;
	    }
	}
      else
	{
	  
	  argn += ALIGN(arg_types[i]->size, FFI_SIZEOF_ARG) / FFI_SIZEOF_ARG;

	  if (i < fp_slot_max
	      && (arg_types[i]->type == FFI_TYPE_FLOAT
		  || arg_types[i]->type == FFI_TYPE_DOUBLE
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
		  || arg_types[i]->type == FFI_TYPE_LONGDOUBLE
#endif
		  ))
	    avalue[i] = ((char *) &fpr[argn]) - arg_types[i]->size;
	  else
	    avalue[i] = ((char *) &gpr[argn]) - arg_types[i]->size;
	}
    }

  
  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  
  return cif->rtype->type;
}

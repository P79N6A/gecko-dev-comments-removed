




















#include <stdio.h>

#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>


#if defined (__APPLE__)
#define AARCH64_STACK_ALIGN 1
#else
#define AARCH64_STACK_ALIGN 16
#endif

#define N_X_ARG_REG 8
#define N_V_ARG_REG 8

#define AARCH64_FFI_WITH_V (1 << AARCH64_FFI_WITH_V_BIT)

union _d
{
  UINT64 d;
  UINT32 s[2];
};

struct call_context
{
  UINT64 x [AARCH64_N_XREG];
  struct
  {
    union _d d[2];
  } v [AARCH64_N_VREG];
};

#if defined (__clang__) && defined (__APPLE__)
extern void
sys_icache_invalidate (void *start, size_t len);
#endif

static inline void
ffi_clear_cache (void *start, void *end)
{
#if defined (__clang__) && defined (__APPLE__)
	sys_icache_invalidate (start, (char *)end - (char *)start);
#elif defined (__GNUC__)
	__builtin___clear_cache (start, end);
#else
#error "Missing builtin to flush instruction cache"
#endif
}

static void *
get_x_addr (struct call_context *context, unsigned n)
{
  return &context->x[n];
}

static void *
get_s_addr (struct call_context *context, unsigned n)
{
#if defined __AARCH64EB__
  return &context->v[n].d[1].s[1];
#else
  return &context->v[n].d[0].s[0];
#endif
}

static void *
get_d_addr (struct call_context *context, unsigned n)
{
#if defined __AARCH64EB__
  return &context->v[n].d[1];
#else
  return &context->v[n].d[0];
#endif
}

static void *
get_v_addr (struct call_context *context, unsigned n)
{
  return &context->v[n];
}




static void *
get_basic_type_addr (unsigned short type, struct call_context *context,
		     unsigned n)
{
  switch (type)
    {
    case FFI_TYPE_FLOAT:
      return get_s_addr (context, n);
    case FFI_TYPE_DOUBLE:
      return get_d_addr (context, n);
#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
    case FFI_TYPE_LONGDOUBLE:
      return get_v_addr (context, n);
#endif
    case FFI_TYPE_UINT8:
    case FFI_TYPE_SINT8:
    case FFI_TYPE_UINT16:
    case FFI_TYPE_SINT16:
    case FFI_TYPE_UINT32:
    case FFI_TYPE_SINT32:
    case FFI_TYPE_INT:
    case FFI_TYPE_POINTER:
    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
      return get_x_addr (context, n);
    case FFI_TYPE_VOID:
      return NULL;
    default:
      FFI_ASSERT (0);
      return NULL;
    }
}



static size_t
get_basic_type_alignment (unsigned short type)
{
  switch (type)
    {
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
      return sizeof (UINT64);
#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
    case FFI_TYPE_LONGDOUBLE:
      return sizeof (long double);
#endif
    case FFI_TYPE_UINT8:
    case FFI_TYPE_SINT8:
#if defined (__APPLE__)
	  return sizeof (UINT8);
#endif
    case FFI_TYPE_UINT16:
    case FFI_TYPE_SINT16:
#if defined (__APPLE__)
	  return sizeof (UINT16);
#endif
    case FFI_TYPE_UINT32:
    case FFI_TYPE_INT:
    case FFI_TYPE_SINT32:
#if defined (__APPLE__)
	  return sizeof (UINT32);
#endif
    case FFI_TYPE_POINTER:
    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
      return sizeof (UINT64);

    default:
      FFI_ASSERT (0);
      return 0;
    }
}



static size_t
get_basic_type_size (unsigned short type)
{
  switch (type)
    {
    case FFI_TYPE_FLOAT:
      return sizeof (UINT32);
    case FFI_TYPE_DOUBLE:
      return sizeof (UINT64);
#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
    case FFI_TYPE_LONGDOUBLE:
      return sizeof (long double);
#endif
    case FFI_TYPE_UINT8:
      return sizeof (UINT8);
    case FFI_TYPE_SINT8:
      return sizeof (SINT8);
    case FFI_TYPE_UINT16:
      return sizeof (UINT16);
    case FFI_TYPE_SINT16:
      return sizeof (SINT16);
    case FFI_TYPE_UINT32:
      return sizeof (UINT32);
    case FFI_TYPE_INT:
    case FFI_TYPE_SINT32:
      return sizeof (SINT32);
    case FFI_TYPE_POINTER:
    case FFI_TYPE_UINT64:
      return sizeof (UINT64);
    case FFI_TYPE_SINT64:
      return sizeof (SINT64);

    default:
      FFI_ASSERT (0);
      return 0;
    }
}

extern void
ffi_call_SYSV (unsigned (*)(struct call_context *context, unsigned char *,
			    extended_cif *),
               struct call_context *context,
               extended_cif *,
               size_t,
               void (*fn)(void));

extern void
ffi_closure_SYSV (ffi_closure *);



static unsigned
is_floating_type (unsigned short type)
{
  return (type == FFI_TYPE_FLOAT || type == FFI_TYPE_DOUBLE
	  || type == FFI_TYPE_LONGDOUBLE);
}



static unsigned short
get_homogeneous_type (ffi_type *ty)
{
  if (ty->type == FFI_TYPE_STRUCT && ty->elements)
    {
      unsigned i;
      unsigned short candidate_type
	= get_homogeneous_type (ty->elements[0]);
      for (i =1; ty->elements[i]; i++)
	{
	  unsigned short iteration_type = 0;
	  


	  if (ty->elements[i]->type == FFI_TYPE_STRUCT
	      && ty->elements[i]->elements)
	    {
	      iteration_type = get_homogeneous_type (ty->elements[i]);
	    }
	  else
	    {
	      iteration_type = ty->elements[i]->type;
	    }

	  
	  if (candidate_type != iteration_type)
	    return FFI_TYPE_STRUCT;
	}
      return candidate_type;
    }

  

  return ty->type;
}







static unsigned
element_count (ffi_type *ty)
{
  if (ty->type == FFI_TYPE_STRUCT && ty->elements)
    {
      unsigned n;
      unsigned elems = 0;
      for (n = 0; ty->elements[n]; n++)
	{
	  if (ty->elements[n]->type == FFI_TYPE_STRUCT
	      && ty->elements[n]->elements)
	    elems += element_count (ty->elements[n]);
	  else
	    elems++;
	}
      return elems;
    }
  return 0;
}








static int
is_hfa (ffi_type *ty)
{
  if (ty->type == FFI_TYPE_STRUCT
      && ty->elements[0]
      && is_floating_type (get_homogeneous_type (ty)))
    {
      unsigned n = element_count (ty);
      return n >= 1 && n <= 4;
    }
  return 0;
}













static int
is_register_candidate (ffi_type *ty)
{
  switch (ty->type)
    {
    case FFI_TYPE_VOID:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
    case FFI_TYPE_LONGDOUBLE:
#endif
    case FFI_TYPE_UINT8:
    case FFI_TYPE_UINT16:
    case FFI_TYPE_UINT32:
    case FFI_TYPE_UINT64:
    case FFI_TYPE_POINTER:
    case FFI_TYPE_SINT8:
    case FFI_TYPE_SINT16:
    case FFI_TYPE_SINT32:
    case FFI_TYPE_INT:
    case FFI_TYPE_SINT64:
      return 1;

    case FFI_TYPE_STRUCT:
      if (is_hfa (ty))
        {
          return 1;
        }
      else if (ty->size > 16)
        {
          



          return 0;
        }
      else
        {
          

          return (ty->size + 7) / 8 < N_X_ARG_REG;
        }
      break;

    default:
      FFI_ASSERT (0);
      break;
    }

  return 0;
}




static int
is_v_register_candidate (ffi_type *ty)
{
  return is_floating_type (ty->type)
	   || (ty->type == FFI_TYPE_STRUCT && is_hfa (ty));
}







struct arg_state
{
  unsigned ngrn;                
  unsigned nsrn;                
  size_t nsaa;                  

#if defined (__APPLE__)
  unsigned allocating_variadic;
#endif
};


static void
arg_init (struct arg_state *state, size_t call_frame_size)
{
  state->ngrn = 0;
  state->nsrn = 0;
  state->nsaa = 0;

#if defined (__APPLE__)
  state->allocating_variadic = 0;
#endif
}




static unsigned
available_x (struct arg_state *state)
{
  return N_X_ARG_REG - state->ngrn;
}




static unsigned
available_v (struct arg_state *state)
{
  return N_V_ARG_REG - state->nsrn;
}

static void *
allocate_to_x (struct call_context *context, struct arg_state *state)
{
  FFI_ASSERT (state->ngrn < N_X_ARG_REG);
  return get_x_addr (context, (state->ngrn)++);
}

static void *
allocate_to_s (struct call_context *context, struct arg_state *state)
{
  FFI_ASSERT (state->nsrn < N_V_ARG_REG);
  return get_s_addr (context, (state->nsrn)++);
}

static void *
allocate_to_d (struct call_context *context, struct arg_state *state)
{
  FFI_ASSERT (state->nsrn < N_V_ARG_REG);
  return get_d_addr (context, (state->nsrn)++);
}

static void *
allocate_to_v (struct call_context *context, struct arg_state *state)
{
  FFI_ASSERT (state->nsrn < N_V_ARG_REG);
  return get_v_addr (context, (state->nsrn)++);
}


static void *
allocate_to_stack (struct arg_state *state, void *stack, size_t alignment,
		   size_t size)
{
  void *allocation;

  

  state->nsaa = ALIGN (state->nsaa, alignment);
  state->nsaa = ALIGN (state->nsaa, alignment);
#if defined (__APPLE__)
  if (state->allocating_variadic)
    state->nsaa = ALIGN (state->nsaa, 8);
#else
  state->nsaa = ALIGN (state->nsaa, 8);
#endif

  allocation = stack + state->nsaa;

  state->nsaa += size;
  return allocation;
}

static void
copy_basic_type (void *dest, void *source, unsigned short type)
{
  

  switch (type)
    {
    case FFI_TYPE_FLOAT:
      *(float *) dest = *(float *) source;
      break;
    case FFI_TYPE_DOUBLE:
      *(double *) dest = *(double *) source;
      break;
#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
    case FFI_TYPE_LONGDOUBLE:
      *(long double *) dest = *(long double *) source;
      break;
#endif
    case FFI_TYPE_UINT8:
      *(ffi_arg *) dest = *(UINT8 *) source;
      break;
    case FFI_TYPE_SINT8:
      *(ffi_sarg *) dest = *(SINT8 *) source;
      break;
    case FFI_TYPE_UINT16:
      *(ffi_arg *) dest = *(UINT16 *) source;
      break;
    case FFI_TYPE_SINT16:
      *(ffi_sarg *) dest = *(SINT16 *) source;
      break;
    case FFI_TYPE_UINT32:
      *(ffi_arg *) dest = *(UINT32 *) source;
      break;
    case FFI_TYPE_INT:
    case FFI_TYPE_SINT32:
      *(ffi_sarg *) dest = *(SINT32 *) source;
      break;
    case FFI_TYPE_POINTER:
    case FFI_TYPE_UINT64:
      *(ffi_arg *) dest = *(UINT64 *) source;
      break;
    case FFI_TYPE_SINT64:
      *(ffi_sarg *) dest = *(SINT64 *) source;
      break;
    case FFI_TYPE_VOID:
      break;

    default:
      FFI_ASSERT (0);
    }
}

static void
copy_hfa_to_reg_or_stack (void *memory,
			  ffi_type *ty,
			  struct call_context *context,
			  unsigned char *stack,
			  struct arg_state *state)
{
  unsigned elems = element_count (ty);
  if (available_v (state) < elems)
    {
      


      state->nsrn = N_V_ARG_REG;
      memcpy (allocate_to_stack (state, stack, ty->alignment, ty->size),
	      memory,
	      ty->size);
    }
  else
    {
      int i;
      unsigned short type = get_homogeneous_type (ty);
      for (i = 0; i < elems; i++)
	{
	  void *reg = allocate_to_v (context, state);
	  copy_basic_type (reg, memory, type);
	  memory += get_basic_type_size (type);
	}
    }
}





static void *
allocate_to_register_or_stack (struct call_context *context,
			       unsigned char *stack,
			       struct arg_state *state,
			       unsigned short type)
{
  size_t alignment = get_basic_type_alignment (type);
  size_t size = alignment;
  switch (type)
    {
    case FFI_TYPE_FLOAT:
      

      size = sizeof (UINT32);
      
    case FFI_TYPE_DOUBLE:
      if (state->nsrn < N_V_ARG_REG)
	return allocate_to_d (context, state);
      state->nsrn = N_V_ARG_REG;
      break;
#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
    case FFI_TYPE_LONGDOUBLE:
      if (state->nsrn < N_V_ARG_REG)
	return allocate_to_v (context, state);
      state->nsrn = N_V_ARG_REG;
      break;
#endif
    case FFI_TYPE_UINT8:
    case FFI_TYPE_SINT8:
    case FFI_TYPE_UINT16:
    case FFI_TYPE_SINT16:
    case FFI_TYPE_UINT32:
    case FFI_TYPE_SINT32:
    case FFI_TYPE_INT:
    case FFI_TYPE_POINTER:
    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
      if (state->ngrn < N_X_ARG_REG)
	return allocate_to_x (context, state);
      state->ngrn = N_X_ARG_REG;
      break;
    default:
      FFI_ASSERT (0);
    }

    return allocate_to_stack (state, stack, alignment, size);
}




static void
copy_to_register_or_stack (struct call_context *context,
			   unsigned char *stack,
			   struct arg_state *state,
			   void *value,
			   unsigned short type)
{
  copy_basic_type (
	  allocate_to_register_or_stack (context, stack, state, type),
	  value,
	  type);
}




static unsigned
aarch64_prep_args (struct call_context *context, unsigned char *stack,
		   extended_cif *ecif)
{
  int i;
  struct arg_state state;

  arg_init (&state, ALIGN(ecif->cif->bytes, 16));

  for (i = 0; i < ecif->cif->nargs; i++)
    {
      ffi_type *ty = ecif->cif->arg_types[i];
      switch (ty->type)
	{
	case FFI_TYPE_VOID:
	  FFI_ASSERT (0);
	  break;

	

	case FFI_TYPE_FLOAT:
	case FFI_TYPE_DOUBLE:
#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
	case FFI_TYPE_LONGDOUBLE:
#endif
	case FFI_TYPE_UINT8:
	case FFI_TYPE_SINT8:
	case FFI_TYPE_UINT16:
	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT32:
	case FFI_TYPE_INT:
	case FFI_TYPE_SINT32:
	case FFI_TYPE_POINTER:
	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	  copy_to_register_or_stack (context, stack, &state,
				     ecif->avalue[i], ty->type);
	  break;

	case FFI_TYPE_STRUCT:
	  if (is_hfa (ty))
	    {
	      copy_hfa_to_reg_or_stack (ecif->avalue[i], ty, context,
					stack, &state);
	    }
	  else if (ty->size > 16)
	    {
	      



	      copy_to_register_or_stack (context, stack, &state,
					 &(ecif->avalue[i]), FFI_TYPE_POINTER);
	    }
	  else if (available_x (&state) >= (ty->size + 7) / 8)
	    {
	      



	      int j;
	      for (j = 0; j < (ty->size + 7) / 8; j++)
		{
		  memcpy (allocate_to_x (context, &state),
			  &(((UINT64 *) ecif->avalue[i])[j]),
			  sizeof (UINT64));
		}
	    }
	  else
	    {
	      



	      state.ngrn = N_X_ARG_REG;

	      memcpy (allocate_to_stack (&state, stack, ty->alignment,
					 ty->size), ecif->avalue + i, ty->size);
	    }
	  break;

	default:
	  FFI_ASSERT (0);
	  break;
	}

#if defined (__APPLE__)
      if (i + 1 == ecif->cif->aarch64_nfixedargs)
	{
	  state.ngrn = N_X_ARG_REG;
	  state.nsrn = N_V_ARG_REG;

	  state.allocating_variadic = 1;
	}
#endif
    }

  return ecif->cif->aarch64_flags;
}

ffi_status
ffi_prep_cif_machdep (ffi_cif *cif)
{
  
  cif->bytes =
    (cif->bytes + (AARCH64_STACK_ALIGN - 1)) & ~ (AARCH64_STACK_ALIGN - 1);

  



  cif->aarch64_flags = 0;

  if (is_v_register_candidate (cif->rtype))
    {
      cif->aarch64_flags |= AARCH64_FFI_WITH_V;
    }
  else
    {
      int i;
      for (i = 0; i < cif->nargs; i++)
        if (is_v_register_candidate (cif->arg_types[i]))
          {
            cif->aarch64_flags |= AARCH64_FFI_WITH_V;
            break;
          }
    }

  return FFI_OK;
}

#if defined (__APPLE__)


ffi_status ffi_prep_cif_machdep_var(ffi_cif *cif,
				    unsigned int nfixedargs,
				    unsigned int ntotalargs)
{
  cif->aarch64_nfixedargs = nfixedargs;

  return ffi_prep_cif_machdep(cif);
}

#endif



void
ffi_call (ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;
  ecif.rvalue = rvalue;

  switch (cif->abi)
    {
    case FFI_SYSV:
      {
        struct call_context context;
	size_t stack_bytes;

	



	stack_bytes = ALIGN(cif->bytes, 16);

	memset (&context, 0, sizeof (context));
        if (is_register_candidate (cif->rtype))
          {
            ffi_call_SYSV (aarch64_prep_args, &context, &ecif, stack_bytes, fn);
            switch (cif->rtype->type)
              {
              case FFI_TYPE_VOID:
              case FFI_TYPE_FLOAT:
              case FFI_TYPE_DOUBLE:
#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
              case FFI_TYPE_LONGDOUBLE:
#endif
              case FFI_TYPE_UINT8:
              case FFI_TYPE_SINT8:
              case FFI_TYPE_UINT16:
              case FFI_TYPE_SINT16:
              case FFI_TYPE_UINT32:
              case FFI_TYPE_SINT32:
              case FFI_TYPE_POINTER:
              case FFI_TYPE_UINT64:
              case FFI_TYPE_INT:
              case FFI_TYPE_SINT64:
		{
		  void *addr = get_basic_type_addr (cif->rtype->type,
						    &context, 0);
		  copy_basic_type (rvalue, addr, cif->rtype->type);
		  break;
		}

              case FFI_TYPE_STRUCT:
                if (is_hfa (cif->rtype))
		  {
		    int j;
		    unsigned short type = get_homogeneous_type (cif->rtype);
		    unsigned elems = element_count (cif->rtype);
		    for (j = 0; j < elems; j++)
		      {
			void *reg = get_basic_type_addr (type, &context, j);
			copy_basic_type (rvalue, reg, type);
			rvalue += get_basic_type_size (type);
		      }
		  }
                else if ((cif->rtype->size + 7) / 8 < N_X_ARG_REG)
                  {
                    size_t size = ALIGN (cif->rtype->size, sizeof (UINT64));
                    memcpy (rvalue, get_x_addr (&context, 0), size);
                  }
                else
                  {
                    FFI_ASSERT (0);
                  }
                break;

              default:
                FFI_ASSERT (0);
                break;
              }
          }
        else
          {
            memcpy (get_x_addr (&context, 8), &rvalue, sizeof (UINT64));
            ffi_call_SYSV (aarch64_prep_args, &context, &ecif,
			   stack_bytes, fn);
          }
        break;
      }

    default:
      FFI_ASSERT (0);
      break;
    }
}

static unsigned char trampoline [] =
{ 0x70, 0x00, 0x00, 0x58,	
  0x91, 0x00, 0x00, 0x10,	
  0x00, 0x02, 0x1f, 0xd6	
};



#define FFI_INIT_TRAMPOLINE(TRAMP,FUN,CTX,FLAGS)			\
  ({unsigned char *__tramp = (unsigned char*)(TRAMP);			\
    UINT64  __fun = (UINT64)(FUN);					\
    UINT64  __ctx = (UINT64)(CTX);					\
    UINT64  __flags = (UINT64)(FLAGS);					\
    memcpy (__tramp, trampoline, sizeof (trampoline));			\
    memcpy (__tramp + 12, &__fun, sizeof (__fun));			\
    memcpy (__tramp + 20, &__ctx, sizeof (__ctx));			\
    memcpy (__tramp + 28, &__flags, sizeof (__flags));			\
    ffi_clear_cache(__tramp, __tramp + FFI_TRAMPOLINE_SIZE);		\
  })

ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
                      ffi_cif* cif,
                      void (*fun)(ffi_cif*,void*,void**,void*),
                      void *user_data,
                      void *codeloc)
{
  if (cif->abi != FFI_SYSV)
    return FFI_BAD_ABI;

  FFI_INIT_TRAMPOLINE (&closure->tramp[0], &ffi_closure_SYSV, codeloc,
		       cif->aarch64_flags);

  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}

















void FFI_HIDDEN
ffi_closure_SYSV_inner (ffi_closure *closure, struct call_context *context,
			void *stack)
{
  ffi_cif *cif = closure->cif;
  void **avalue = (void**) alloca (cif->nargs * sizeof (void*));
  void *rvalue = NULL;
  int i;
  struct arg_state state;

  arg_init (&state, ALIGN(cif->bytes, 16));

  for (i = 0; i < cif->nargs; i++)
    {
      ffi_type *ty = cif->arg_types[i];

      switch (ty->type)
	{
	case FFI_TYPE_VOID:
	  FFI_ASSERT (0);
	  break;

	case FFI_TYPE_UINT8:
	case FFI_TYPE_SINT8:
	case FFI_TYPE_UINT16:
	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT32:
	case FFI_TYPE_SINT32:
	case FFI_TYPE_INT:
	case FFI_TYPE_POINTER:
	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	case  FFI_TYPE_FLOAT:
	case  FFI_TYPE_DOUBLE:
#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
	case  FFI_TYPE_LONGDOUBLE:
	  avalue[i] = allocate_to_register_or_stack (context, stack,
						     &state, ty->type);
	  break;
#endif

	case FFI_TYPE_STRUCT:
	  if (is_hfa (ty))
	    {
	      unsigned n = element_count (ty);
	      if (available_v (&state) < n)
		{
		  state.nsrn = N_V_ARG_REG;
		  avalue[i] = allocate_to_stack (&state, stack, ty->alignment,
						 ty->size);
		}
	      else
		{
		  switch (get_homogeneous_type (ty))
		    {
		    case FFI_TYPE_FLOAT:
		      {
			









			int j;
			UINT32 *p = avalue[i] = alloca (ty->size);
			for (j = 0; j < element_count (ty); j++)
			  memcpy (&p[j],
				  allocate_to_s (context, &state),
				  sizeof (*p));
			break;
		      }

		    case FFI_TYPE_DOUBLE:
		      {
			









			int j;
			UINT64 *p = avalue[i] = alloca (ty->size);
			for (j = 0; j < element_count (ty); j++)
			  memcpy (&p[j],
				  allocate_to_d (context, &state),
				  sizeof (*p));
			break;
		      }

#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
		    case FFI_TYPE_LONGDOUBLE:
			  memcpy (&avalue[i],
				  allocate_to_v (context, &state),
				  sizeof (*avalue));
		      break;
#endif

		    default:
		      FFI_ASSERT (0);
		      break;
		    }
		}
	    }
	  else if (ty->size > 16)
	    {
	      

	      memcpy (&avalue[i],
		      allocate_to_register_or_stack (context, stack,
						     &state, FFI_TYPE_POINTER),
		      sizeof (avalue[i]));
	    }
	  else if (available_x (&state) >= (ty->size + 7) / 8)
	    {
	      avalue[i] = get_x_addr (context, state.ngrn);
	      state.ngrn += (ty->size + 7) / 8;
	    }
	  else
	    {
	      state.ngrn = N_X_ARG_REG;

	      avalue[i] = allocate_to_stack (&state, stack, ty->alignment,
					     ty->size);
	    }
	  break;

	default:
	  FFI_ASSERT (0);
	  break;
	}
    }

  



  if (is_register_candidate (cif->rtype))
    {
      

      



      rvalue = alloca (cif->rtype->size);
      (closure->fun) (cif, rvalue, avalue, closure->user_data);

      

      switch (cif->rtype->type)
        {
        case FFI_TYPE_VOID:
          break;

        case FFI_TYPE_UINT8:
        case FFI_TYPE_UINT16:
        case FFI_TYPE_UINT32:
        case FFI_TYPE_POINTER:
        case FFI_TYPE_UINT64:
        case FFI_TYPE_SINT8:
        case FFI_TYPE_SINT16:
        case FFI_TYPE_INT:
        case FFI_TYPE_SINT32:
        case FFI_TYPE_SINT64:
        case FFI_TYPE_FLOAT:
        case FFI_TYPE_DOUBLE:
#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
        case FFI_TYPE_LONGDOUBLE:
#endif
	  {
	    void *addr = get_basic_type_addr (cif->rtype->type, context, 0);
	    copy_basic_type (addr, rvalue, cif->rtype->type);
            break;
	  }
        case FFI_TYPE_STRUCT:
          if (is_hfa (cif->rtype))
	    {
	      int j;
	      unsigned short type = get_homogeneous_type (cif->rtype);
	      unsigned elems = element_count (cif->rtype);
	      for (j = 0; j < elems; j++)
		{
		  void *reg = get_basic_type_addr (type, context, j);
		  copy_basic_type (reg, rvalue, type);
		  rvalue += get_basic_type_size (type);
		}
	    }
          else if ((cif->rtype->size + 7) / 8 < N_X_ARG_REG)
            {
              size_t size = ALIGN (cif->rtype->size, sizeof (UINT64)) ;
              memcpy (get_x_addr (context, 0), rvalue, size);
            }
          else
            {
              FFI_ASSERT (0);
            }
          break;
        default:
          FFI_ASSERT (0);
          break;
        }
    }
  else
    {
      memcpy (&rvalue, get_x_addr (context, 8), sizeof (UINT64));
      (closure->fun) (cif, rvalue, avalue, closure->user_data);
    }
}




























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>


static int vfp_type_p (ffi_type *);
static void layout_vfp_args (ffi_cif *);








int ffi_prep_args(char *stack, extended_cif *ecif, float *vfp_space)
{
  register unsigned int i, vi = 0;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;

  argp = stack;

  if ( ecif->cif->flags == FFI_TYPE_STRUCT ) {
    *(void **) argp = ecif->rvalue;
    argp += 4;
  }

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       (i != 0);
       i--, p_arg++)
    {
      size_t z;

      
      if (ecif->cif->abi == FFI_VFP
	  && vi < ecif->cif->vfp_nargs && vfp_type_p (*p_arg))
	{
	  float* vfp_slot = vfp_space + ecif->cif->vfp_args[vi++];
	  if ((*p_arg)->type == FFI_TYPE_FLOAT)
	    *((float*)vfp_slot) = *((float*)*p_argv);
	  else if ((*p_arg)->type == FFI_TYPE_DOUBLE)
	    *((double*)vfp_slot) = *((double*)*p_argv);
	  else
	    memcpy(vfp_slot, *p_argv, (*p_arg)->size);
	  p_argv++;
	  continue;
	}

      
      if (((*p_arg)->alignment - 1) & (unsigned) argp) {
	argp = (char *) ALIGN(argp, (*p_arg)->alignment);
      }

      if ((*p_arg)->type == FFI_TYPE_STRUCT)
	argp = (char *) ALIGN(argp, 4);

	  z = (*p_arg)->size;
	  if (z < sizeof(int))
	    {
	      z = sizeof(int);
	      switch ((*p_arg)->type)
		{
		case FFI_TYPE_SINT8:
		  *(signed int *) argp = (signed int)*(SINT8 *)(* p_argv);
		  break;
		  
		case FFI_TYPE_UINT8:
		  *(unsigned int *) argp = (unsigned int)*(UINT8 *)(* p_argv);
		  break;
		  
		case FFI_TYPE_SINT16:
		  *(signed int *) argp = (signed int)*(SINT16 *)(* p_argv);
		  break;
		  
		case FFI_TYPE_UINT16:
		  *(unsigned int *) argp = (unsigned int)*(UINT16 *)(* p_argv);
		  break;
		  
		case FFI_TYPE_STRUCT:
		  memcpy(argp, *p_argv, (*p_arg)->size);
		  break;

		default:
		  FFI_ASSERT(0);
		}
	    }
	  else if (z == sizeof(int))
	    {
	      *(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
	    }
	  else
	    {
	      memcpy(argp, *p_argv, z);
	    }
	  p_argv++;
	  argp += z;
    }

  
  return ecif->cif->vfp_used;
}


ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  int type_code;
  


  cif->bytes = (cif->bytes + 7) & ~7;

  
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
      cif->flags = (unsigned) cif->rtype->type;
      break;

    case FFI_TYPE_SINT64:
    case FFI_TYPE_UINT64:
      cif->flags = (unsigned) FFI_TYPE_SINT64;
      break;

    case FFI_TYPE_STRUCT:
      if (cif->abi == FFI_VFP
	  && (type_code = vfp_type_p (cif->rtype)) != 0)
	{
	  

	  cif->flags = (unsigned) type_code;
	}
      else if (cif->rtype->size <= 4)
	
	cif->flags = (unsigned)FFI_TYPE_INT;
      else
	


	cif->flags = (unsigned)FFI_TYPE_STRUCT;
      break;

    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }

  


  if (cif->abi == FFI_VFP)
    layout_vfp_args (cif);

  return FFI_OK;
}


extern void ffi_call_SYSV (void (*fn)(void), extended_cif *, unsigned, unsigned, unsigned *);
extern void ffi_call_VFP (void (*fn)(void), extended_cif *, unsigned, unsigned, unsigned *);

void ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;

  int small_struct = (cif->flags == FFI_TYPE_INT 
		      && cif->rtype->type == FFI_TYPE_STRUCT);
  int vfp_struct = (cif->flags == FFI_TYPE_STRUCT_VFP_FLOAT
		    || cif->flags == FFI_TYPE_STRUCT_VFP_DOUBLE);

  ecif.cif = cif;
  ecif.avalue = avalue;

  unsigned int temp;
  
  
  

  if ((rvalue == NULL) && 
      (cif->flags == FFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
  else if (small_struct)
    ecif.rvalue = &temp;
  else if (vfp_struct)
    {
      
      ecif.rvalue = alloca(32);
    }
  else
    ecif.rvalue = rvalue;

  switch (cif->abi) 
    {
    case FFI_SYSV:
      ffi_call_SYSV (fn, &ecif, cif->bytes, cif->flags, ecif.rvalue);
      break;

    case FFI_VFP:
      ffi_call_VFP (fn, &ecif, cif->bytes, cif->flags, ecif.rvalue);
      break;

    default:
      FFI_ASSERT(0);
      break;
    }
  if (small_struct)
    memcpy (rvalue, &temp, cif->rtype->size);
  else if (vfp_struct)
    memcpy (rvalue, ecif.rvalue, cif->rtype->size);
}



static void ffi_prep_incoming_args_SYSV (char *stack, void **ret,
					 void** args, ffi_cif* cif, float *vfp_stack);

void ffi_closure_SYSV (ffi_closure *);

void ffi_closure_VFP (ffi_closure *);



unsigned int
ffi_closure_SYSV_inner (closure, respp, args, vfp_args)
     ffi_closure *closure;
     void **respp;
     void *args;
     void *vfp_args;
{
  
  ffi_cif       *cif;
  void         **arg_area;

  cif         = closure->cif;
  arg_area    = (void**) alloca (cif->nargs * sizeof (void*));  

  





  ffi_prep_incoming_args_SYSV(args, respp, arg_area, cif, vfp_args);

  (closure->fun) (cif, *respp, arg_area, closure->user_data);

  return cif->flags;
}


static void 
ffi_prep_incoming_args_SYSV(char *stack, void **rvalue,
			    void **avalue, ffi_cif *cif,
			    
			    float *vfp_stack)

{
  register unsigned int i, vi = 0;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;

  argp = stack;

  if ( cif->flags == FFI_TYPE_STRUCT ) {
    *rvalue = *(void **) argp;
    argp += 4;
  }

  p_argv = avalue;

  for (i = cif->nargs, p_arg = cif->arg_types; (i != 0); i--, p_arg++)
    {
      size_t z;
      size_t alignment;
  
      if (cif->abi == FFI_VFP
	  && vi < cif->vfp_nargs && vfp_type_p (*p_arg))
	{
	  *p_argv++ = (void*)(vfp_stack + cif->vfp_args[vi++]);
	  continue;
	}

      alignment = (*p_arg)->alignment;
      if (alignment < 4)
	alignment = 4;
      
      if ((alignment - 1) & (unsigned) argp) {
	argp = (char *) ALIGN(argp, alignment);
      }

      z = (*p_arg)->size;

      

      *p_argv = (void*) argp;

      p_argv++;
      argp += z;
    }
  
  return;
}



#define FFI_INIT_TRAMPOLINE(TRAMP,FUN,CTX)				\
({ unsigned char *__tramp = (unsigned char*)(TRAMP);			\
   unsigned int  __fun = (unsigned int)(FUN);				\
   unsigned int  __ctx = (unsigned int)(CTX);				\
   *(unsigned int*) &__tramp[0] = 0xe92d000f; /* stmfd sp!, {r0-r3} */	\
   *(unsigned int*) &__tramp[4] = 0xe59f0000; /* ldr r0, [pc] */	\
   *(unsigned int*) &__tramp[8] = 0xe59ff000; /* ldr pc, [pc] */	\
   *(unsigned int*) &__tramp[12] = __ctx;				\
   *(unsigned int*) &__tramp[16] = __fun;				\
   __clear_cache((&__tramp[0]), (&__tramp[19]));			\
 })




ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*,void*,void**,void*),
		      void *user_data,
		      void *codeloc)
{
  void (*closure_func)(ffi_closure*) = NULL;

  if (cif->abi == FFI_SYSV)
    closure_func = &ffi_closure_SYSV;
  else if (cif->abi == FFI_VFP)
    closure_func = &ffi_closure_VFP;
  else
    FFI_ASSERT (0);
    
  FFI_INIT_TRAMPOLINE (&closure->tramp[0], \
		       closure_func,  \
		       codeloc);
    
  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}



static int rec_vfp_type_p (ffi_type *t, int *elt, int *elnum)
{
  switch (t->type)
    {
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
      *elt = (int) t->type;
      *elnum = 1;
      return 1;

    case FFI_TYPE_STRUCT_VFP_FLOAT:
      *elt = FFI_TYPE_FLOAT;
      *elnum = t->size / sizeof (float);
      return 1;

    case FFI_TYPE_STRUCT_VFP_DOUBLE:
      *elt = FFI_TYPE_DOUBLE;
      *elnum = t->size / sizeof (double);
      return 1;

    case FFI_TYPE_STRUCT:;
      {
	int base_elt = 0, total_elnum = 0;
	ffi_type **el = t->elements;
	while (*el)
	  {
	    int el_elt = 0, el_elnum = 0;
	    if (! rec_vfp_type_p (*el, &el_elt, &el_elnum)
		|| (base_elt && base_elt != el_elt)
		|| total_elnum + el_elnum > 4)
	      return 0;
	    base_elt = el_elt;
	    total_elnum += el_elnum;
	    el++;
	  }
	*elnum = total_elnum;
	*elt = base_elt;
	return 1;
      }
    default: ;
    }
  return 0;
}

static int vfp_type_p (ffi_type *t)
{
  int elt, elnum;
  if (rec_vfp_type_p (t, &elt, &elnum))
    {
      if (t->type == FFI_TYPE_STRUCT)
	{
	  if (elnum == 1)
	    t->type = elt;
	  else
	    t->type = (elt == FFI_TYPE_FLOAT
		       ? FFI_TYPE_STRUCT_VFP_FLOAT
		       : FFI_TYPE_STRUCT_VFP_DOUBLE);
	}
      return (int) t->type;
    }
  return 0;
}

static void place_vfp_arg (ffi_cif *cif, ffi_type *t)
{
  int reg = cif->vfp_reg_free;
  int nregs = t->size / sizeof (float);
  int align = ((t->type == FFI_TYPE_STRUCT_VFP_FLOAT
		|| t->type == FFI_TYPE_FLOAT) ? 1 : 2);
  
  if ((reg & 1) && align == 2)
    reg++;
  while (reg + nregs <= 16)
    {
      int s, new_used = 0;
      for (s = reg; s < reg + nregs; s++)
	{
	  new_used |= (1 << s);
	  if (cif->vfp_used & (1 << s))
	    {
	      reg += align;
	      goto next_reg;
	    }
	}
      
      cif->vfp_used |= new_used;
      cif->vfp_args[cif->vfp_nargs++] = reg;

      
      if (cif->vfp_used & (1 << cif->vfp_reg_free))
	{
	  reg += nregs;
	  while (cif->vfp_used & (1 << reg))
	    reg += 1;
	  cif->vfp_reg_free = reg;
	}
      return;
    next_reg: ;
    }
}

static void layout_vfp_args (ffi_cif *cif)
{
  int i;
  
  cif->vfp_used = 0;
  cif->vfp_nargs = 0;
  cif->vfp_reg_free = 0;
  memset (cif->vfp_args, -1, 16); 

  for (i = 0; i < cif->nargs; i++)
    {
      ffi_type *t = cif->arg_types[i];
      if (vfp_type_p (t))
	place_vfp_arg (cif, t);
    }
}

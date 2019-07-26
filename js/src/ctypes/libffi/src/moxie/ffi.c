

























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>




void *ffi_prep_args(char *stack, extended_cif *ecif)
{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;
  register int count = 0;

  p_argv = ecif->avalue;
  argp = stack;

  if (ecif->cif->rtype->type == FFI_TYPE_STRUCT)
    {
      *(void **) argp = ecif->rvalue;
      argp += 4;
    }

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       (i != 0);
       i--, p_arg++)
    {
      size_t z;
      
      z = (*p_arg)->size;

      if ((*p_arg)->type == FFI_TYPE_STRUCT)
	{
	  z = sizeof(void*);
	  *(void **) argp = *p_argv;
	} 
      else if (z < sizeof(int))
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
      count += z;
    }

  return (stack + ((count > 24) ? 24 : ALIGN_DOWN(count, 8)));
}


ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  if (cif->rtype->type == FFI_TYPE_STRUCT)
    cif->flags = -1;
  else
    cif->flags = cif->rtype->size;

  cif->bytes = ALIGN (cif->bytes, 8);

  return FFI_OK;
}

extern void ffi_call_EABI(void *(*)(char *, extended_cif *), 
			  extended_cif *, 
			  unsigned, unsigned, 
			  unsigned *, 
			  void (*fn)(void));

void ffi_call(ffi_cif *cif, 
	      void (*fn)(void), 
	      void *rvalue, 
	      void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;
  
  
  

  if ((rvalue == NULL) && 
      (cif->rtype->type == FFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;

  switch (cif->abi) 
    {
    case FFI_EABI:
      ffi_call_EABI(ffi_prep_args, &ecif, cif->bytes, 
		    cif->flags, ecif.rvalue, fn);
      break;
    default:
      FFI_ASSERT(0);
      break;
    }
}

void ffi_closure_eabi (unsigned arg1, unsigned arg2, unsigned arg3,
		       unsigned arg4, unsigned arg5, unsigned arg6)
{
  


  register ffi_closure *creg __asm__ ("$r12");
  ffi_closure *closure = creg;

  

  register char *frame_pointer __asm__ ("$fp");

  
  void *struct_rvalue = (void *) arg1;

  
  char *stack_args = frame_pointer + 9*4; 

  
  unsigned register_args[6] =
    { arg1, arg2, arg3, arg4, arg5, arg6 };
  char *register_args_ptr = (char *) register_args;

  ffi_cif *cif = closure->cif;
  ffi_type **arg_types = cif->arg_types;
  void **avalue = alloca (cif->nargs * sizeof(void *));
  char *ptr = (char *) register_args;
  int i;

  
  if ((cif->rtype != NULL) && (cif->rtype->type == FFI_TYPE_STRUCT)) {
    ptr += 4;
    register_args_ptr = (char *)&register_args[1];
  }

  
  for (i = 0; i < cif->nargs; i++)
    {
      switch (arg_types[i]->type)
	{
	case FFI_TYPE_SINT8:
	case FFI_TYPE_UINT8:
	  avalue[i] = ptr + 3;
	  break;
	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT16:
	  avalue[i] = ptr + 2;
	  break;
	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT32:
	case FFI_TYPE_FLOAT:
	case FFI_TYPE_POINTER:
	  avalue[i] = ptr;
	  break;
	case FFI_TYPE_STRUCT:
	  avalue[i] = *(void**)ptr;
	  break;
	default:
	  
	  avalue[i] = ptr;
	  ptr += 4;
	  break;
	}
      ptr += 4;

      

      if (ptr == &register_args[6])
	ptr = stack_args;
    }

  
  if (cif->rtype && (cif->rtype->type == FFI_TYPE_STRUCT))
    {
      (closure->fun) (cif, struct_rvalue, avalue, closure->user_data);
    }
  else
    {
      
      long long rvalue;
      (closure->fun) (cif, &rvalue, avalue, closure->user_data);
      asm ("mov $r12, %0\n ld.l $r0, ($r12)\n ldo.l $r1, 4($r12)" : : "r" (&rvalue));
    }
}

ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned short *tramp = (unsigned short *) &closure->tramp[0];
  unsigned long fn = (long) ffi_closure_eabi;
  unsigned long cls = (long) codeloc;

  if (cif->abi != FFI_EABI)
    return FFI_BAD_ABI;

  fn = (unsigned long) ffi_closure_eabi;

  tramp[0] = 0x01e0; 
  tramp[1] = cls >> 16;
  tramp[2] = cls & 0xffff;
  tramp[3] = 0x1a00; 
  tramp[4] = fn >> 16;
  tramp[5] = fn & 0xffff;

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return FFI_OK;
}





























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
  


  register ffi_closure *creg __asm__ ("gr7");
  ffi_closure *closure = creg;

  

  register char *frame_pointer __asm__ ("fp");
  char *stack_args = frame_pointer + 16;

  
  unsigned register_args[6] =
    { arg1, arg2, arg3, arg4, arg5, arg6 };

  ffi_cif *cif = closure->cif;
  ffi_type **arg_types = cif->arg_types;
  void **avalue = alloca (cif->nargs * sizeof(void *));
  char *ptr = (char *) register_args;
  int i;

  
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

      

      if (ptr == ((char *)register_args + (6*4)))
	ptr = stack_args;
    }

  
  if (cif->rtype->type == FFI_TYPE_STRUCT)
    {
      


      register void *return_struct_ptr __asm__("gr3");
      (closure->fun) (cif, return_struct_ptr, avalue, closure->user_data);
    }
  else
    {
      
      long long rvalue;
      (closure->fun) (cif, &rvalue, avalue, closure->user_data);

      

 
      asm ("ldi  @(%0, #0), gr8" : : "r" (&rvalue));
      asm ("ldi  @(%0, #0), gr9" : : "r" (&((int *) &rvalue)[1]));
    }
}

ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned int *tramp = (unsigned int *) &closure->tramp[0];
  unsigned long fn = (long) ffi_closure_eabi;
  unsigned long cls = (long) codeloc;
#ifdef __FRV_FDPIC__
  register void *got __asm__("gr15");
#endif
  int i;

  fn = (unsigned long) ffi_closure_eabi;

#ifdef __FRV_FDPIC__
  tramp[0] = &((unsigned int *)codeloc)[2];
  tramp[1] = got;
  tramp[2] = 0x8cfc0000 + (fn  & 0xffff); 
  tramp[3] = 0x8efc0000 + (cls & 0xffff); 
  tramp[4] = 0x8cf80000 + (fn  >> 16);	  
  tramp[5] = 0x8ef80000 + (cls >> 16);    
  tramp[6] = 0x9cc86000;                  
  tramp[7] = 0x8030e000;                  
#else
  tramp[0] = 0x8cfc0000 + (fn  & 0xffff); 
  tramp[1] = 0x8efc0000 + (cls & 0xffff); 
  tramp[2] = 0x8cf80000 + (fn  >> 16);	  
  tramp[3] = 0x8ef80000 + (cls >> 16);    
  tramp[4] = 0x80300006;                  
#endif

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  
  for (i = 0; i < FFI_TRAMPOLINE_SIZE; i++)
    __asm__ volatile ("dcf @(%0,%1)\n\tici @(%2,%1)" :: "r" (tramp), "r" (i),
		      "r" (codeloc));

  return FFI_OK;
}

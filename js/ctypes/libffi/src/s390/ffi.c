




























 
#include <ffi.h>
#include <ffi_common.h>
 
#include <stdlib.h>
#include <stdio.h>
 

 





 
#define MAX_GPRARGS 5

 
#ifdef __s390x__
#define MAX_FPRARGS 4
#else
#define MAX_FPRARGS 2
#endif


#define ROUND_SIZE(size) (((size) + 15) & ~15)


#define FFI390_RET_VOID		0
#define FFI390_RET_STRUCT	1
#define FFI390_RET_FLOAT	2
#define FFI390_RET_DOUBLE	3
#define FFI390_RET_INT32	4
#define FFI390_RET_INT64	5


 




 
static void ffi_prep_args (unsigned char *, extended_cif *);
void
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 2)
__attribute__ ((visibility ("hidden")))
#endif
ffi_closure_helper_SYSV (ffi_closure *, unsigned long *, 
			 unsigned long long *, unsigned long *);


 




 
extern void ffi_call_SYSV(unsigned,
			  extended_cif *,
			  void (*)(unsigned char *, extended_cif *),
			  unsigned,
			  void *,
			  void (*fn)(void));

extern void ffi_closure_SYSV(void);
 

 








 
static int
ffi_check_struct_type (ffi_type *arg)
{
  size_t size = arg->size;

  

  while (arg->type == FFI_TYPE_STRUCT 
         && arg->elements[0] && !arg->elements[1])
    arg = arg->elements[0];

  

  switch (size)
    {
      case 1:
        return FFI_TYPE_UINT8;

      case 2:
        return FFI_TYPE_UINT16;

      case 4:
	if (arg->type == FFI_TYPE_FLOAT)
          return FFI_TYPE_FLOAT;
	else
	  return FFI_TYPE_UINT32;

      case 8:
	if (arg->type == FFI_TYPE_DOUBLE)
          return FFI_TYPE_DOUBLE;
	else
	  return FFI_TYPE_UINT64;

      default:
	break;
    }

  
  return FFI_TYPE_POINTER;
}
 

 










 
static void
ffi_prep_args (unsigned char *stack, extended_cif *ecif)
{
  















  int gpr_off = ecif->cif->bytes;
  int fpr_off = gpr_off + ROUND_SIZE (MAX_GPRARGS * sizeof (long));

  unsigned long long *p_fpr = (unsigned long long *)(stack + fpr_off);
  unsigned long *p_gpr = (unsigned long *)(stack + gpr_off);
  unsigned char *p_struct = (unsigned char *)p_gpr;
  unsigned long *p_ov = (unsigned long *)stack;

  int n_fpr = 0;
  int n_gpr = 0;
  int n_ov = 0;

  ffi_type **ptr;
  void **p_argv = ecif->avalue;
  int i;
 
  


  if (ecif->cif->flags == FFI390_RET_STRUCT)
    p_gpr[n_gpr++] = (unsigned long) ecif->rvalue;

  
 
  for (ptr = ecif->cif->arg_types, i = ecif->cif->nargs;
       i > 0;
       i--, ptr++, p_argv++)
    {
      void *arg = *p_argv;
      int type = (*ptr)->type;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
      
      if (type == FFI_TYPE_LONGDOUBLE)
	type = FFI_TYPE_STRUCT;
#endif

      
      if (type == FFI_TYPE_STRUCT)
	{
	  type = ffi_check_struct_type (*ptr);

	  
	  if (type == FFI_TYPE_POINTER)
	    {
	      p_struct -= ROUND_SIZE ((*ptr)->size);
	      memcpy (p_struct, (char *)arg, (*ptr)->size);
	      arg = &p_struct;
	    }
	}

      
      switch (type) 
	{
	  case FFI_TYPE_DOUBLE:
	    if (n_fpr < MAX_FPRARGS)
	      p_fpr[n_fpr++] = *(unsigned long long *) arg;
	    else
#ifdef __s390x__
	      p_ov[n_ov++] = *(unsigned long *) arg;
#else
	      p_ov[n_ov++] = ((unsigned long *) arg)[0],
	      p_ov[n_ov++] = ((unsigned long *) arg)[1];
#endif
	    break;
	
	  case FFI_TYPE_FLOAT:
	    if (n_fpr < MAX_FPRARGS)
	      p_fpr[n_fpr++] = (long long) *(unsigned int *) arg << 32;
	    else
	      p_ov[n_ov++] = *(unsigned int *) arg;
	    break;

	  case FFI_TYPE_POINTER:
	    if (n_gpr < MAX_GPRARGS)
	      p_gpr[n_gpr++] = (unsigned long)*(unsigned char **) arg;
	    else
	      p_ov[n_ov++] = (unsigned long)*(unsigned char **) arg;
	    break;
 
	  case FFI_TYPE_UINT64:
	  case FFI_TYPE_SINT64:
#ifdef __s390x__
	    if (n_gpr < MAX_GPRARGS)
	      p_gpr[n_gpr++] = *(unsigned long *) arg;
	    else
	      p_ov[n_ov++] = *(unsigned long *) arg;
#else
	    if (n_gpr == MAX_GPRARGS-1)
	      n_gpr = MAX_GPRARGS;
	    if (n_gpr < MAX_GPRARGS)
	      p_gpr[n_gpr++] = ((unsigned long *) arg)[0],
	      p_gpr[n_gpr++] = ((unsigned long *) arg)[1];
	    else
	      p_ov[n_ov++] = ((unsigned long *) arg)[0],
	      p_ov[n_ov++] = ((unsigned long *) arg)[1];
#endif
	    break;
 
	  case FFI_TYPE_UINT32:
	    if (n_gpr < MAX_GPRARGS)
	      p_gpr[n_gpr++] = *(unsigned int *) arg;
	    else
	      p_ov[n_ov++] = *(unsigned int *) arg;
	    break;
 
	  case FFI_TYPE_INT:
	  case FFI_TYPE_SINT32:
	    if (n_gpr < MAX_GPRARGS)
	      p_gpr[n_gpr++] = *(signed int *) arg;
	    else
	      p_ov[n_ov++] = *(signed int *) arg;
	    break;
 
	  case FFI_TYPE_UINT16:
	    if (n_gpr < MAX_GPRARGS)
	      p_gpr[n_gpr++] = *(unsigned short *) arg;
	    else
	      p_ov[n_ov++] = *(unsigned short *) arg;
	    break;
 
	  case FFI_TYPE_SINT16:
	    if (n_gpr < MAX_GPRARGS)
	      p_gpr[n_gpr++] = *(signed short *) arg;
	    else
	      p_ov[n_ov++] = *(signed short *) arg;
	    break;

	  case FFI_TYPE_UINT8:
	    if (n_gpr < MAX_GPRARGS)
	      p_gpr[n_gpr++] = *(unsigned char *) arg;
	    else
	      p_ov[n_ov++] = *(unsigned char *) arg;
	    break;
 
	  case FFI_TYPE_SINT8:
	    if (n_gpr < MAX_GPRARGS)
	      p_gpr[n_gpr++] = *(signed char *) arg;
	    else
	      p_ov[n_ov++] = *(signed char *) arg;
	    break;
 
	  default:
	    FFI_ASSERT (0);
	    break;
        }
    }
}


 







 
ffi_status
ffi_prep_cif_machdep(ffi_cif *cif)
{
  size_t struct_size = 0;
  int n_gpr = 0;
  int n_fpr = 0;
  int n_ov = 0;

  ffi_type **ptr;
  int i;

   

  switch (cif->rtype->type)
    {
      
      case FFI_TYPE_VOID:
	cif->flags = FFI390_RET_VOID;
	break;

      
      case FFI_TYPE_STRUCT:
	cif->flags = FFI390_RET_STRUCT;
	n_gpr++;  
	break; 

      
      case FFI_TYPE_FLOAT:
	cif->flags = FFI390_RET_FLOAT;
	break;

      case FFI_TYPE_DOUBLE:
	cif->flags = FFI390_RET_DOUBLE;
	break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
      case FFI_TYPE_LONGDOUBLE:
	cif->flags = FFI390_RET_STRUCT;
	n_gpr++;
	break;
#endif
      

      case FFI_TYPE_UINT64:
      case FFI_TYPE_SINT64:
	cif->flags = FFI390_RET_INT64;
	break;

      case FFI_TYPE_POINTER:
      case FFI_TYPE_INT:
      case FFI_TYPE_UINT32:
      case FFI_TYPE_SINT32:
      case FFI_TYPE_UINT16:
      case FFI_TYPE_SINT16:
      case FFI_TYPE_UINT8:
      case FFI_TYPE_SINT8:
	
#ifdef __s390x__
	cif->flags = FFI390_RET_INT64;
#else
	cif->flags = FFI390_RET_INT32;
#endif
	break;
 
      default:
        FFI_ASSERT (0);
        break;
    }

  
 
  for (ptr = cif->arg_types, i = cif->nargs;
       i > 0;
       i--, ptr++)
    {
      int type = (*ptr)->type;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
      
      if (type == FFI_TYPE_LONGDOUBLE)
	type = FFI_TYPE_STRUCT;
#endif

      
      if (type == FFI_TYPE_STRUCT)
	{
	  type = ffi_check_struct_type (*ptr);

	  

	  if (type == FFI_TYPE_POINTER)
	    struct_size += ROUND_SIZE ((*ptr)->size);
	}

      
      switch (type) 
	{
	  


	  case FFI_TYPE_DOUBLE:
	    if (n_fpr < MAX_FPRARGS)
	      n_fpr++;
	    else
	      n_ov += sizeof (double) / sizeof (long);
	    break;
	
	  case FFI_TYPE_FLOAT:
	    if (n_fpr < MAX_FPRARGS)
	      n_fpr++;
	    else
	      n_ov++;
	    break;

	  



	      
#ifndef __s390x__
	  case FFI_TYPE_UINT64:
	  case FFI_TYPE_SINT64:
	    if (n_gpr == MAX_GPRARGS-1)
	      n_gpr = MAX_GPRARGS;
	    if (n_gpr < MAX_GPRARGS)
	      n_gpr += 2;
	    else
	      n_ov += 2;
	    break;
#endif

	  


	  default: 
	    if (n_gpr < MAX_GPRARGS)
	      n_gpr++;
	    else
	      n_ov++;
	    break;
        }
    }

  


  cif->bytes = ROUND_SIZE (n_ov * sizeof (long)) + struct_size;
 
  return FFI_OK;
}
 

 







 
void
ffi_call(ffi_cif *cif,
	 void (*fn)(void),
	 void *rvalue,
	 void **avalue)
{
  int ret_type = cif->flags;
  extended_cif ecif;
 
  ecif.cif    = cif;
  ecif.avalue = avalue;
  ecif.rvalue = rvalue;

  
  if (rvalue == NULL)
    {
      if (ret_type == FFI390_RET_STRUCT)
	ecif.rvalue = alloca (cif->rtype->size);
      else
	ret_type = FFI390_RET_VOID;
    } 

  switch (cif->abi)
    {
      case FFI_SYSV:
        ffi_call_SYSV (cif->bytes, &ecif, ffi_prep_args,
		       ret_type, ecif.rvalue, fn);
        break;
 
      default:
        FFI_ASSERT (0);
        break;
    }
}
 









 
void
ffi_closure_helper_SYSV (ffi_closure *closure,
			 unsigned long *p_gpr,
			 unsigned long long *p_fpr,
			 unsigned long *p_ov)
{
  unsigned long long ret_buffer;

  void *rvalue = &ret_buffer;
  void **avalue;
  void **p_arg;

  int n_gpr = 0;
  int n_fpr = 0;
  int n_ov = 0;

  ffi_type **ptr;
  int i;

  

  p_arg = avalue = alloca (closure->cif->nargs * sizeof (void *));

  



  if (closure->cif->flags == FFI390_RET_STRUCT)
    rvalue = (void *) p_gpr[n_gpr++];

  

  for (ptr = closure->cif->arg_types, i = closure->cif->nargs;
       i > 0;
       i--, p_arg++, ptr++)
    {
      int deref_struct_pointer = 0;
      int type = (*ptr)->type;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
      
      if (type == FFI_TYPE_LONGDOUBLE)
	type = FFI_TYPE_STRUCT;
#endif

      
      if (type == FFI_TYPE_STRUCT)
	{
	  type = ffi_check_struct_type (*ptr);

	  

	  if (type == FFI_TYPE_POINTER)
	    deref_struct_pointer = 1;
	}

      
      if (type == FFI_TYPE_POINTER)
#ifdef __s390x__
	type = FFI_TYPE_UINT64;
#else
	type = FFI_TYPE_UINT32;
#endif

      
      switch (type) 
	{
	  case FFI_TYPE_DOUBLE:
	    if (n_fpr < MAX_FPRARGS)
	      *p_arg = &p_fpr[n_fpr++];
	    else
	      *p_arg = &p_ov[n_ov], 
	      n_ov += sizeof (double) / sizeof (long);
	    break;
	
	  case FFI_TYPE_FLOAT:
	    if (n_fpr < MAX_FPRARGS)
	      *p_arg = &p_fpr[n_fpr++];
	    else
	      *p_arg = (char *)&p_ov[n_ov++] + sizeof (long) - 4;
	    break;
 
	  case FFI_TYPE_UINT64:
	  case FFI_TYPE_SINT64:
#ifdef __s390x__
	    if (n_gpr < MAX_GPRARGS)
	      *p_arg = &p_gpr[n_gpr++];
	    else
	      *p_arg = &p_ov[n_ov++];
#else
	    if (n_gpr == MAX_GPRARGS-1)
	      n_gpr = MAX_GPRARGS;
	    if (n_gpr < MAX_GPRARGS)
	      *p_arg = &p_gpr[n_gpr], n_gpr += 2;
	    else
	      *p_arg = &p_ov[n_ov], n_ov += 2;
#endif
	    break;
 
	  case FFI_TYPE_INT:
	  case FFI_TYPE_UINT32:
	  case FFI_TYPE_SINT32:
	    if (n_gpr < MAX_GPRARGS)
	      *p_arg = (char *)&p_gpr[n_gpr++] + sizeof (long) - 4;
	    else
	      *p_arg = (char *)&p_ov[n_ov++] + sizeof (long) - 4;
	    break;
 
	  case FFI_TYPE_UINT16:
	  case FFI_TYPE_SINT16:
	    if (n_gpr < MAX_GPRARGS)
	      *p_arg = (char *)&p_gpr[n_gpr++] + sizeof (long) - 2;
	    else
	      *p_arg = (char *)&p_ov[n_ov++] + sizeof (long) - 2;
	    break;

	  case FFI_TYPE_UINT8:
	  case FFI_TYPE_SINT8:
	    if (n_gpr < MAX_GPRARGS)
	      *p_arg = (char *)&p_gpr[n_gpr++] + sizeof (long) - 1;
	    else
	      *p_arg = (char *)&p_ov[n_ov++] + sizeof (long) - 1;
	    break;
 
	  default:
	    FFI_ASSERT (0);
	    break;
        }

      

      if (deref_struct_pointer)
	*p_arg = *(void **)*p_arg;
    }


  
  (closure->fun) (closure->cif, rvalue, avalue, closure->user_data);

  
  switch (closure->cif->rtype->type)
    {
      
      case FFI_TYPE_VOID:
      case FFI_TYPE_STRUCT:
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
      case FFI_TYPE_LONGDOUBLE:
#endif
	break;

      
      case FFI_TYPE_FLOAT:
	p_fpr[0] = (long long) *(unsigned int *) rvalue << 32;
	break;

      case FFI_TYPE_DOUBLE:
	p_fpr[0] = *(unsigned long long *) rvalue;
	break;

      

      case FFI_TYPE_UINT64:
      case FFI_TYPE_SINT64:
#ifdef __s390x__
	p_gpr[0] = *(unsigned long *) rvalue;
#else
	p_gpr[0] = ((unsigned long *) rvalue)[0],
	p_gpr[1] = ((unsigned long *) rvalue)[1];
#endif
	break;

      case FFI_TYPE_POINTER:
      case FFI_TYPE_UINT32:
      case FFI_TYPE_UINT16:
      case FFI_TYPE_UINT8:
	p_gpr[0] = *(unsigned long *) rvalue;
	break;

      case FFI_TYPE_INT:
      case FFI_TYPE_SINT32:
      case FFI_TYPE_SINT16:
      case FFI_TYPE_SINT8:
	p_gpr[0] = *(signed long *) rvalue;
	break;

      default:
        FFI_ASSERT (0);
        break;
    }
}
 









 
ffi_status
ffi_prep_closure_loc (ffi_closure *closure,
		      ffi_cif *cif,
		      void (*fun) (ffi_cif *, void *, void **, void *),
		      void *user_data,
		      void *codeloc)
{
  FFI_ASSERT (cif->abi == FFI_SYSV);

#ifndef __s390x__
  *(short *)&closure->tramp [0] = 0x0d10;   
  *(short *)&closure->tramp [2] = 0x9801;   
  *(short *)&closure->tramp [4] = 0x1006;
  *(short *)&closure->tramp [6] = 0x07f1;   
  *(long  *)&closure->tramp [8] = (long)codeloc;
  *(long  *)&closure->tramp[12] = (long)&ffi_closure_SYSV;
#else
  *(short *)&closure->tramp [0] = 0x0d10;   
  *(short *)&closure->tramp [2] = 0xeb01;   
  *(short *)&closure->tramp [4] = 0x100e;
  *(short *)&closure->tramp [6] = 0x0004;
  *(short *)&closure->tramp [8] = 0x07f1;   
  *(long  *)&closure->tramp[16] = (long)codeloc;
  *(long  *)&closure->tramp[24] = (long)&ffi_closure_SYSV;
#endif 
 
  closure->cif = cif;
  closure->user_data = user_data;
  closure->fun = fun;
 
  return FFI_OK;
}


 




























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>

#define NGREGARG 4
#if defined(__SH4__)
#define NFREGARG 8
#endif

#if defined(__HITACHI__)
#define STRUCT_VALUE_ADDRESS_WITH_ARG 1
#else
#define STRUCT_VALUE_ADDRESS_WITH_ARG 0
#endif


static int
simple_type (ffi_type *arg)
{
  if (arg->type != FFI_TYPE_STRUCT)
    return arg->type;
  else if (arg->elements[1])
    return FFI_TYPE_STRUCT;

  return simple_type (arg->elements[0]);
}

static int
return_type (ffi_type *arg)
{
  unsigned short type;

  if (arg->type != FFI_TYPE_STRUCT)
    return arg->type;

  type = simple_type (arg->elements[0]);
  if (! arg->elements[1])
    {
      switch (type)
	{
	case FFI_TYPE_SINT8:
	case FFI_TYPE_UINT8:
	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT16:
	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT32:
	  return FFI_TYPE_INT;

	default:
	  return type;
	}
    }

  
  if (arg->size <= 2 * sizeof (int))
    {
      int i = 0;
      ffi_type *e;

      while ((e = arg->elements[i++]))
	{
	  type = simple_type (e);
	  switch (type)
	    {
	    case FFI_TYPE_SINT32:
	    case FFI_TYPE_UINT32:
	    case FFI_TYPE_INT:
	    case FFI_TYPE_FLOAT:
	      return FFI_TYPE_UINT64;

	    default:
	      break;
	    }
	}
    }

  return FFI_TYPE_STRUCT;
}




void ffi_prep_args(char *stack, extended_cif *ecif)
{
  register unsigned int i;
  register int tmp;
  register unsigned int avn;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;
  int greg, ireg;
#if defined(__SH4__)
  int freg = 0;
#endif

  tmp = 0;
  argp = stack;

  if (return_type (ecif->cif->rtype) == FFI_TYPE_STRUCT)
    {
      *(void **) argp = ecif->rvalue;
      argp += 4;
      ireg = STRUCT_VALUE_ADDRESS_WITH_ARG ? 1 : 0;
    }
  else
    ireg = 0;

  
  greg = ireg;
  avn = ecif->cif->nargs;
  p_argv = ecif->avalue;

  for (i = 0, p_arg = ecif->cif->arg_types; i < avn; i++, p_arg++, p_argv++)
    {
      size_t z;

      z = (*p_arg)->size;
      if (z < sizeof(int))
	{
	  if (greg++ >= NGREGARG)
	    continue;

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
	      *(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
	      break;

	    default:
	      FFI_ASSERT(0);
	    }
	  argp += z;
	}
      else if (z == sizeof(int))
	{
#if defined(__SH4__)
	  if ((*p_arg)->type == FFI_TYPE_FLOAT)
	    {
	      if (freg++ >= NFREGARG)
		continue;
	    }
	  else
#endif
	    {
	      if (greg++ >= NGREGARG)
		continue;
	    }
	  *(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
	  argp += z;
	}
#if defined(__SH4__)
      else if ((*p_arg)->type == FFI_TYPE_DOUBLE)
	{
	  if (freg + 1 >= NFREGARG)
	    continue;
	  freg = (freg + 1) & ~1;
	  freg += 2;
	  memcpy (argp, *p_argv, z);
	  argp += z;
	}
#endif
      else
	{
	  int n = (z + sizeof (int) - 1) / sizeof (int);
#if defined(__SH4__)
	  if (greg + n - 1 >= NGREGARG)
	    continue;
#else
	  if (greg >= NGREGARG)
	    continue;
#endif
	  greg += n;
	  memcpy (argp, *p_argv, z);
	  argp += n * sizeof (int);
	}
    }

  
  greg = ireg;
#if defined(__SH4__)
  freg = 0;
#endif
  p_argv = ecif->avalue;

  for (i = 0, p_arg = ecif->cif->arg_types; i < avn; i++, p_arg++, p_argv++)
    {
      size_t z;

      z = (*p_arg)->size;
      if (z < sizeof(int))
	{
	  if (greg++ < NGREGARG)
	    continue;

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
	      *(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
	      break;

	    default:
	      FFI_ASSERT(0);
	    }
	  argp += z;
	}
      else if (z == sizeof(int))
	{
#if defined(__SH4__)
	  if ((*p_arg)->type == FFI_TYPE_FLOAT)
	    {
	      if (freg++ < NFREGARG)
		continue;
	    }
	  else
#endif
	    {
	      if (greg++ < NGREGARG)
		continue;
	    }
	  *(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
	  argp += z;
	}
#if defined(__SH4__)
      else if ((*p_arg)->type == FFI_TYPE_DOUBLE)
	{
	  if (freg + 1 < NFREGARG)
	    {
	      freg = (freg + 1) & ~1;
	      freg += 2;
	      continue;
	    }
	  memcpy (argp, *p_argv, z);
	  argp += z;
	}
#endif
      else
	{
	  int n = (z + sizeof (int) - 1) / sizeof (int);
	  if (greg + n - 1 < NGREGARG)
	    {
	      greg += n;
	      continue;
	    }
#if (! defined(__SH4__))
	  else if (greg < NGREGARG)
	    {
	      greg = NGREGARG;
	      continue;
	    }
#endif
	  memcpy (argp, *p_argv, z);
	  argp += n * sizeof (int);
	}
    }

  return;
}


ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  int i, j;
  int size, type;
  int n, m;
  int greg;
#if defined(__SH4__)
  int freg = 0;
#endif

  cif->flags = 0;

  greg = ((return_type (cif->rtype) == FFI_TYPE_STRUCT) &&
	  STRUCT_VALUE_ADDRESS_WITH_ARG) ? 1 : 0;

#if defined(__SH4__)
  for (i = j = 0; i < cif->nargs && j < 12; i++)
    {
      type = (cif->arg_types)[i]->type;
      switch (type)
	{
	case FFI_TYPE_FLOAT:
	  if (freg >= NFREGARG)
	    continue;
	  freg++;
	  cif->flags += ((cif->arg_types)[i]->type) << (2 * j);
	  j++;
	  break;

	case FFI_TYPE_DOUBLE:
	  if ((freg + 1) >= NFREGARG)
	    continue;
	  freg = (freg + 1) & ~1;
	  freg += 2;
	  cif->flags += ((cif->arg_types)[i]->type) << (2 * j);
	  j++;
	  break;
	      
	default:
	  size = (cif->arg_types)[i]->size;
	  n = (size + sizeof (int) - 1) / sizeof (int);
	  if (greg + n - 1 >= NGREGARG)
		continue;
	  greg += n;
	  for (m = 0; m < n; m++)
	    cif->flags += FFI_TYPE_INT << (2 * j++);
	  break;
	}
    }
#else
  for (i = j = 0; i < cif->nargs && j < 4; i++)
    {
      size = (cif->arg_types)[i]->size;
      n = (size + sizeof (int) - 1) / sizeof (int);
      if (greg >= NGREGARG)
	continue;
      else if (greg + n - 1 >= NGREGARG)
	n = NGREGARG - greg;
      greg += n;
      for (m = 0; m < n; m++)
        cif->flags += FFI_TYPE_INT << (2 * j++);
    }
#endif

  
  switch (cif->rtype->type)
    {
    case FFI_TYPE_STRUCT:
      cif->flags += (unsigned) (return_type (cif->rtype)) << 24;
      break;

    case FFI_TYPE_VOID:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
    case FFI_TYPE_SINT64:
    case FFI_TYPE_UINT64:
      cif->flags += (unsigned) cif->rtype->type << 24;
      break;

    default:
      cif->flags += FFI_TYPE_INT << 24;
      break;
    }

  return FFI_OK;
}

extern void ffi_call_SYSV(void (*)(char *, extended_cif *), extended_cif *,
			  unsigned, unsigned, unsigned *, void (*fn)(void));

void ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;
  UINT64 trvalue;

  ecif.cif = cif;
  ecif.avalue = avalue;
  
  
  

  if (cif->rtype->type == FFI_TYPE_STRUCT
      && return_type (cif->rtype) != FFI_TYPE_STRUCT)
    ecif.rvalue = &trvalue;
  else if ((rvalue == NULL) && 
      (cif->rtype->type == FFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;

  switch (cif->abi) 
    {
    case FFI_SYSV:
      ffi_call_SYSV(ffi_prep_args, &ecif, cif->bytes, cif->flags, ecif.rvalue,
		    fn);
      break;
    default:
      FFI_ASSERT(0);
      break;
    }

  if (rvalue
      && cif->rtype->type == FFI_TYPE_STRUCT
      && return_type (cif->rtype) != FFI_TYPE_STRUCT)
    memcpy (rvalue, &trvalue, cif->rtype->size);
}

extern void ffi_closure_SYSV (void);
#if defined(__SH4__)
extern void __ic_invalidate (void *line);
#endif

ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned int *tramp;
  unsigned int insn;

  FFI_ASSERT (cif->abi == FFI_GCC_SYSV);

  tramp = (unsigned int *) &closure->tramp[0];
  
  insn = (return_type (cif->rtype) == FFI_TYPE_STRUCT
	  ? 0x0018 
	  : 0x0008 );

#ifdef __LITTLE_ENDIAN__
  tramp[0] = 0xd301d102;
  tramp[1] = 0x0000412b | (insn << 16);
#else
  tramp[0] = 0xd102d301;
  tramp[1] = 0x412b0000 | insn;
#endif
  *(void **) &tramp[2] = (void *)codeloc;          
  *(void **) &tramp[3] = (void *)ffi_closure_SYSV; 

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

#if defined(__SH4__)
  
  __ic_invalidate(codeloc);
#endif

  return FFI_OK;
}









#ifdef __LITTLE_ENDIAN__
#define OFS_INT8	0
#define OFS_INT16	0
#else
#define OFS_INT8	3
#define OFS_INT16	2
#endif

int
ffi_closure_helper_SYSV (ffi_closure *closure, void *rvalue, 
			 unsigned long *pgr, unsigned long *pfr, 
			 unsigned long *pst)
{
  void **avalue;
  ffi_type **p_arg;
  int i, avn;
  int ireg, greg = 0;
#if defined(__SH4__)
  int freg = 0;
#endif
  ffi_cif *cif; 

  cif = closure->cif;
  avalue = alloca(cif->nargs * sizeof(void *));

  

  if (cif->rtype->type == FFI_TYPE_STRUCT && STRUCT_VALUE_ADDRESS_WITH_ARG)
    {
      rvalue = (void *) *pgr++;
      ireg = 1;
    }
  else
    ireg = 0;

  cif = closure->cif;
  greg = ireg;
  avn = cif->nargs;

  
  for (i = 0, p_arg = cif->arg_types; i < avn; i++, p_arg++)
    {
      size_t z;

      z = (*p_arg)->size;
      if (z < sizeof(int))
	{
	  if (greg++ >= NGREGARG)
	    continue;

	  z = sizeof(int);
	  switch ((*p_arg)->type)
	    {
	    case FFI_TYPE_SINT8:
	    case FFI_TYPE_UINT8:
	      avalue[i] = (((char *)pgr) + OFS_INT8);
	      break;
  
	    case FFI_TYPE_SINT16:
	    case FFI_TYPE_UINT16:
	      avalue[i] = (((char *)pgr) + OFS_INT16);
	      break;
  
	    case FFI_TYPE_STRUCT:
	      avalue[i] = pgr;
	      break;

	    default:
	      FFI_ASSERT(0);
	    }
	  pgr++;
	}
      else if (z == sizeof(int))
	{
#if defined(__SH4__)
	  if ((*p_arg)->type == FFI_TYPE_FLOAT)
	    {
	      if (freg++ >= NFREGARG)
		continue;
	      avalue[i] = pfr;
	      pfr++;
	    }
	  else
#endif
	    {
	      if (greg++ >= NGREGARG)
		continue;
	      avalue[i] = pgr;
	      pgr++;
	    }
	}
#if defined(__SH4__)
      else if ((*p_arg)->type == FFI_TYPE_DOUBLE)
	{
	  if (freg + 1 >= NFREGARG)
	    continue;
	  if (freg & 1)
	    pfr++;
	  freg = (freg + 1) & ~1;
	  freg += 2;
	  avalue[i] = pfr;
	  pfr += 2;
	}
#endif
      else
	{
	  int n = (z + sizeof (int) - 1) / sizeof (int);
#if defined(__SH4__)
	  if (greg + n - 1 >= NGREGARG)
	    continue;
#else
	  if (greg >= NGREGARG)
	    continue;
#endif
	  greg += n;
	  avalue[i] = pgr;
	  pgr += n;
	}
    }

  greg = ireg;
#if defined(__SH4__)
  freg = 0;
#endif

  for (i = 0, p_arg = cif->arg_types; i < avn; i++, p_arg++)
    {
      size_t z;

      z = (*p_arg)->size;
      if (z < sizeof(int))
	{
	  if (greg++ < NGREGARG)
	    continue;

	  z = sizeof(int);
	  switch ((*p_arg)->type)
	    {
	    case FFI_TYPE_SINT8:
	    case FFI_TYPE_UINT8:
	      avalue[i] = (((char *)pst) + OFS_INT8);
	      break;
  
	    case FFI_TYPE_SINT16:
	    case FFI_TYPE_UINT16:
	      avalue[i] = (((char *)pst) + OFS_INT16);
	      break;
  
	    case FFI_TYPE_STRUCT:
	      avalue[i] = pst;
	      break;

	    default:
	      FFI_ASSERT(0);
	    }
	  pst++;
	}
      else if (z == sizeof(int))
	{
#if defined(__SH4__)
	  if ((*p_arg)->type == FFI_TYPE_FLOAT)
	    {
	      if (freg++ < NFREGARG)
		continue;
	    }
	  else
#endif
	    {
	      if (greg++ < NGREGARG)
		continue;
	    }
	  avalue[i] = pst;
	  pst++;
	}
#if defined(__SH4__)
      else if ((*p_arg)->type == FFI_TYPE_DOUBLE)
	{
	  if (freg + 1 < NFREGARG)
	    {
	      freg = (freg + 1) & ~1;
	      freg += 2;
	      continue;
	    }
	  avalue[i] = pst;
	  pst += 2;
	}
#endif
      else
	{
	  int n = (z + sizeof (int) - 1) / sizeof (int);
	  if (greg + n - 1 < NGREGARG)
	    {
	      greg += n;
	      continue;
	    }
#if (! defined(__SH4__))
	  else if (greg < NGREGARG)
	    {
	      greg += n;
	      pst += greg - NGREGARG;
	      continue;
	    }
#endif
	  avalue[i] = pst;
	  pst += n;
	}
    }

  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  
  return return_type (cif->rtype);
}

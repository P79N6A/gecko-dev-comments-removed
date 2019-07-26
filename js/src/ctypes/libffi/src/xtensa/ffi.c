

























#include <ffi.h>
#include <ffi_common.h>



















#define FFI_TYPE_STRUCT_REGS FFI_TYPE_LAST


extern void ffi_call_SYSV(void *rvalue, unsigned rsize, unsigned flags,
			  void(*fn)(void), unsigned nbytes, extended_cif*);
extern void ffi_closure_SYSV(void) FFI_HIDDEN;

ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  switch(cif->rtype->type) {
    case FFI_TYPE_SINT8:
    case FFI_TYPE_UINT8:
    case FFI_TYPE_SINT16:
    case FFI_TYPE_UINT16:
      cif->flags = cif->rtype->type;
      break;
    case FFI_TYPE_VOID:
    case FFI_TYPE_FLOAT:
      cif->flags = FFI_TYPE_UINT32;
      break;
    case FFI_TYPE_DOUBLE:
    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
      cif->flags = FFI_TYPE_UINT64; 
      break;
    case FFI_TYPE_STRUCT:
      cif->flags = FFI_TYPE_STRUCT; 
      
      if (cif->rtype->size > 4 * 4) {
        

        cif->flags = FFI_TYPE_STRUCT;	
        cif->bytes += 8;
      }
      break;

    default:
      cif->flags = FFI_TYPE_UINT32;
      break;
  }

  


  cif->bytes = ALIGN(cif->bytes, 16);

  return FFI_OK;
}

void ffi_prep_args(extended_cif *ecif, unsigned char* stack)
{
  unsigned int i;
  unsigned long *addr;
  ffi_type **ptr;

  union {
    void **v;
    char **c;
    signed char **sc;
    unsigned char **uc;
    signed short **ss;
    unsigned short **us;
    unsigned int **i;
    long long **ll;
    float **f;
    double **d;
  } p_argv;

  
  FFI_ASSERT (((unsigned long) stack & 0x7) == 0);

  p_argv.v = ecif->avalue;
  addr = (unsigned long*)stack;

  
  if (ecif->cif->rtype->type == FFI_TYPE_STRUCT && ecif->cif->rtype->size > 16)
  {
    *addr++ = (unsigned long)ecif->rvalue;
  }

  for (i = ecif->cif->nargs, ptr = ecif->cif->arg_types;
       i > 0;
       i--, ptr++, p_argv.v++)
  {
    switch ((*ptr)->type)
    {
      case FFI_TYPE_SINT8:
        *addr++ = **p_argv.sc;
        break;
      case FFI_TYPE_UINT8:
        *addr++ = **p_argv.uc;
        break;
      case FFI_TYPE_SINT16:
        *addr++ = **p_argv.ss;
        break;
      case FFI_TYPE_UINT16:
        *addr++ = **p_argv.us;
        break;
      case FFI_TYPE_FLOAT:
      case FFI_TYPE_INT:
      case FFI_TYPE_UINT32:
      case FFI_TYPE_SINT32:
      case FFI_TYPE_POINTER:
        *addr++ = **p_argv.i;
        break;
      case FFI_TYPE_DOUBLE:
      case FFI_TYPE_UINT64:
      case FFI_TYPE_SINT64:
        if (((unsigned long)addr & 4) != 0)
          addr++;
        *(unsigned long long*)addr = **p_argv.ll;
	addr += sizeof(unsigned long long) / sizeof (addr);
        break;

      case FFI_TYPE_STRUCT:
      {
        unsigned long offs;
        unsigned long size;

        if (((unsigned long)addr & 4) != 0 && (*ptr)->alignment > 4)
          addr++;

        offs = (unsigned long) addr - (unsigned long) stack;
        size = (*ptr)->size;

        
        if (offs < FFI_REGISTER_NARGS * 4
            && offs + size > FFI_REGISTER_NARGS * 4)
          addr = (unsigned long*) (stack + FFI_REGISTER_NARGS * 4);

        memcpy((char*) addr, *p_argv.c, size);
        addr += (size + 3) / 4;
        break;
      }

      default:
        FFI_ASSERT(0);
    }
  }
}


void ffi_call(ffi_cif* cif, void(*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;
  unsigned long rsize = cif->rtype->size;
  int flags = cif->flags;
  void *alloc = NULL;

  ecif.cif = cif;
  ecif.avalue = avalue;

  





  if (flags == FFI_TYPE_STRUCT && (rsize <= 16 || rvalue == NULL))
  {
    alloc = alloca(ALIGN(rsize, 4));
    ecif.rvalue = alloc;
  }
  else
  {
    ecif.rvalue = rvalue;
  }

  if (cif->abi != FFI_SYSV)
    FFI_ASSERT(0);

  ffi_call_SYSV (ecif.rvalue, rsize, cif->flags, fn, cif->bytes, &ecif);

  if (alloc != NULL && rvalue != NULL)
    memcpy(rvalue, alloc, rsize);
}

extern void ffi_trampoline();
extern void ffi_cacheflush(void* start, void* end);

ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
                      ffi_cif* cif,
                      void (*fun)(ffi_cif*, void*, void**, void*),
                      void *user_data,
                      void *codeloc)
{
  
  memcpy(closure->tramp, ffi_trampoline, FFI_TRAMPOLINE_SIZE);
  *(unsigned int*)(&closure->tramp[8]) = (unsigned int)ffi_closure_SYSV;

  
  
  ffi_cacheflush(closure->tramp, closure->tramp + FFI_TRAMPOLINE_SIZE);

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;
  return FFI_OK; 
}


long FFI_HIDDEN
ffi_closure_SYSV_inner(ffi_closure *closure, void **values, void *rvalue)
{
  ffi_cif *cif;
  ffi_type **arg_types;
  void **avalue;
  int i, areg;

  cif = closure->cif;
  if (cif->abi != FFI_SYSV)
    return FFI_BAD_ABI;

  areg = 0;

  int rtype = cif->rtype->type;
  if (rtype == FFI_TYPE_STRUCT && cif->rtype->size > 4 * 4)
  {
    rvalue = *values;
    areg++;
  }

  cif = closure->cif; 
  arg_types = cif->arg_types;
  avalue = alloca(cif->nargs * sizeof(void *));

  for (i = 0; i < cif->nargs; i++)
  {
    if (arg_types[i]->alignment == 8 && (areg & 1) != 0)
      areg++;

    
    if (areg == FFI_REGISTER_NARGS)
      areg += 4;

    if (arg_types[i]->type == FFI_TYPE_STRUCT)
    {
      int numregs = ((arg_types[i]->size + 3) & ~3) / 4;
      if (areg < FFI_REGISTER_NARGS && areg + numregs > FFI_REGISTER_NARGS)
        areg = FFI_REGISTER_NARGS + 4;
    }

    avalue[i] = &values[areg];
    areg += (arg_types[i]->size + 3) / 4;
  }

  (closure->fun)(cif, rvalue, avalue, closure->user_data);

  return rtype;
}

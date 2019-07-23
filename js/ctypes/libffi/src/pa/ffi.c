



























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>
#include <stdio.h>

#define ROUND_UP(v, a)  (((size_t)(v) + (a) - 1) & ~((a) - 1))

#define MIN_STACK_SIZE  64
#define FIRST_ARG_SLOT  9
#define DEBUG_LEVEL   0

#define fldw(addr, fpreg) \
  __asm__ volatile ("fldw 0(%0), %%" #fpreg "L" : : "r"(addr) : #fpreg)
#define fstw(fpreg, addr) \
  __asm__ volatile ("fstw %%" #fpreg "L, 0(%0)" : : "r"(addr))
#define fldd(addr, fpreg) \
  __asm__ volatile ("fldd 0(%0), %%" #fpreg : : "r"(addr) : #fpreg)
#define fstd(fpreg, addr) \
  __asm__ volatile ("fstd %%" #fpreg "L, 0(%0)" : : "r"(addr))

#define debug(lvl, x...) do { if (lvl <= DEBUG_LEVEL) { printf(x); } } while (0)

static inline int ffi_struct_type(ffi_type *t)
{
  size_t sz = t->size;

  





  if (sz <= 1)
    return FFI_TYPE_UINT8;
  else if (sz == 2)
    return FFI_TYPE_SMALL_STRUCT2;
  else if (sz == 3)
    return FFI_TYPE_SMALL_STRUCT3;
  else if (sz == 4)
    return FFI_TYPE_SMALL_STRUCT4;
  else if (sz == 5)
    return FFI_TYPE_SMALL_STRUCT5;
  else if (sz == 6)
    return FFI_TYPE_SMALL_STRUCT6;
  else if (sz == 7)
    return FFI_TYPE_SMALL_STRUCT7;
  else if (sz <= 8)
    return FFI_TYPE_SMALL_STRUCT8;
  else
    return FFI_TYPE_STRUCT; 
}



























































void ffi_prep_args_pa32(UINT32 *stack, extended_cif *ecif, unsigned bytes)
{
  register unsigned int i;
  register ffi_type **p_arg;
  register void **p_argv;
  unsigned int slot = FIRST_ARG_SLOT;
  char *dest_cpy;
  size_t len;

  debug(1, "%s: stack = %p, ecif = %p, bytes = %u\n", __FUNCTION__, stack,
	ecif, bytes);

  p_arg = ecif->cif->arg_types;
  p_argv = ecif->avalue;

  for (i = 0; i < ecif->cif->nargs; i++)
    {
      int type = (*p_arg)->type;

      switch (type)
	{
	case FFI_TYPE_SINT8:
	  *(SINT32 *)(stack - slot) = *(SINT8 *)(*p_argv);
	  break;

	case FFI_TYPE_UINT8:
	  *(UINT32 *)(stack - slot) = *(UINT8 *)(*p_argv);
	  break;

	case FFI_TYPE_SINT16:
	  *(SINT32 *)(stack - slot) = *(SINT16 *)(*p_argv);
	  break;

	case FFI_TYPE_UINT16:
	  *(UINT32 *)(stack - slot) = *(UINT16 *)(*p_argv);
	  break;

	case FFI_TYPE_UINT32:
	case FFI_TYPE_SINT32:
	case FFI_TYPE_POINTER:
	  debug(3, "Storing UINT32 %u in slot %u\n", *(UINT32 *)(*p_argv),
		slot);
	  *(UINT32 *)(stack - slot) = *(UINT32 *)(*p_argv);
	  break;

	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	  
	  slot += (slot & 1) ? 1 : 2;
	  *(UINT64 *)(stack - slot) = *(UINT64 *)(*p_argv);
	  break;

	case FFI_TYPE_FLOAT:
	  
	  debug(3, "Storing UINT32(float) in slot %u\n", slot);
	  *(UINT32 *)(stack - slot) = *(UINT32 *)(*p_argv);
	  switch (slot - FIRST_ARG_SLOT)
	    {
	    
	    case 0: fldw(stack - slot, fr4); break;
	    case 1: fldw(stack - slot, fr5); break;
	    case 2: fldw(stack - slot, fr6); break;
	    case 3: fldw(stack - slot, fr7); break;
	    }
	  break;

	case FFI_TYPE_DOUBLE:
	  
	  slot += (slot & 1) ? 1 : 2;
	  debug(3, "Storing UINT64(double) at slot %u\n", slot);
	  *(UINT64 *)(stack - slot) = *(UINT64 *)(*p_argv);
	  switch (slot - FIRST_ARG_SLOT)
	    {
	      
	      case 1: fldd(stack - slot, fr5); break;
	      case 3: fldd(stack - slot, fr7); break;
	    }
	  break;

#ifdef PA_HPUX
	case FFI_TYPE_LONGDOUBLE:
	  

	  *(UINT32 *)(stack - slot) = (UINT32)(*p_argv);
	  break;
#endif

	case FFI_TYPE_STRUCT:

	  



	  len = (*p_arg)->size;
	  if (len <= 4)
	    {
	      dest_cpy = (char *)(stack - slot) + 4 - len;
	      memcpy(dest_cpy, (char *)*p_argv, len);
	    }
	  else if (len <= 8)
	    {
	      slot += (slot & 1) ? 1 : 2;
	      dest_cpy = (char *)(stack - slot) + 8 - len;
	      memcpy(dest_cpy, (char *)*p_argv, len);
	    }
	  else
	    *(UINT32 *)(stack - slot) = (UINT32)(*p_argv);
	  break;

	default:
	  FFI_ASSERT(0);
	}

      slot++;
      p_arg++;
      p_argv++;
    }

  
  {
    unsigned int n;

    debug(5, "Stack setup:\n");
    for (n = 0; n < (bytes + 3) / 4; n++)
      {
	if ((n%4) == 0) { debug(5, "\n%08x: ", (unsigned int)(stack - n)); }
	debug(5, "%08x ", *(stack - n));
      }
    debug(5, "\n");
  }

  FFI_ASSERT(slot * 4 <= bytes);

  return;
}

static void ffi_size_stack_pa32(ffi_cif *cif)
{
  ffi_type **ptr;
  int i;
  int z = 0; 

  for (ptr = cif->arg_types, i = 0; i < cif->nargs; ptr++, i++)
    {
      int type = (*ptr)->type;

      switch (type)
	{
	case FFI_TYPE_DOUBLE:
	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	  z += 2 + (z & 1); 
	  break;

#ifdef PA_HPUX
	case FFI_TYPE_LONGDOUBLE:
#endif
	case FFI_TYPE_STRUCT:
	  z += 1; 
	  break;

	default: 
	  z++;
	}
    }

  

  if (z <= 6)
    cif->bytes = MIN_STACK_SIZE; 
  else
    cif->bytes = 64 + ROUND_UP((z - 6) * sizeof(UINT32), MIN_STACK_SIZE);

  debug(3, "Calculated stack size is %u bytes\n", cif->bytes);
}


ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
      cif->flags = (unsigned) cif->rtype->type;
      break;

#ifdef PA_HPUX
    case FFI_TYPE_LONGDOUBLE:
      
      cif->flags = FFI_TYPE_STRUCT;
      break;
#endif

    case FFI_TYPE_STRUCT:
      




      cif->flags = ffi_struct_type(cif->rtype);
      break;

    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
      cif->flags = FFI_TYPE_UINT64;
      break;

    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }

  

  switch (cif->abi)
    {
    case FFI_PA32:
      ffi_size_stack_pa32(cif);
      break;

    default:
      FFI_ASSERT(0);
      break;
    }

  return FFI_OK;
}

extern void ffi_call_pa32(void (*)(UINT32 *, extended_cif *, unsigned),
			  extended_cif *, unsigned, unsigned, unsigned *,
			  void (*fn)(void));

void ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  


  if (rvalue == NULL
#ifdef PA_HPUX
      && (cif->rtype->type == FFI_TYPE_STRUCT
	  || cif->rtype->type == FFI_TYPE_LONGDOUBLE))
#else
      && cif->rtype->type == FFI_TYPE_STRUCT)
#endif
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;


  switch (cif->abi)
    {
    case FFI_PA32:
      debug(3, "Calling ffi_call_pa32: ecif=%p, bytes=%u, flags=%u, rvalue=%p, fn=%p\n", &ecif, cif->bytes, cif->flags, ecif.rvalue, (void *)fn);
      ffi_call_pa32(ffi_prep_args_pa32, &ecif, cif->bytes,
		     cif->flags, ecif.rvalue, fn);
      break;

    default:
      FFI_ASSERT(0);
      break;
    }
}

#if FFI_CLOSURES




ffi_status ffi_closure_inner_pa32(ffi_closure *closure, UINT32 *stack)
{
  ffi_cif *cif;
  void **avalue;
  void *rvalue;
  UINT32 ret[2]; 
  ffi_type **p_arg;
  char *tmp;
  int i, avn;
  unsigned int slot = FIRST_ARG_SLOT;
  register UINT32 r28 asm("r28");

  cif = closure->cif;

  
  if (cif->flags == FFI_TYPE_STRUCT)
    rvalue = (void *)r28;
  else
    rvalue = &ret[0];

  avalue = (void **)alloca(cif->nargs * FFI_SIZEOF_ARG);
  avn = cif->nargs;
  p_arg = cif->arg_types;

  for (i = 0; i < avn; i++)
    {
      int type = (*p_arg)->type;

      switch (type)
	{
	case FFI_TYPE_SINT8:
	case FFI_TYPE_UINT8:
	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT16:
	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT32:
	case FFI_TYPE_POINTER:
	  avalue[i] = (char *)(stack - slot) + sizeof(UINT32) - (*p_arg)->size;
	  break;

	case FFI_TYPE_SINT64:
	case FFI_TYPE_UINT64:
	  slot += (slot & 1) ? 1 : 2;
	  avalue[i] = (void *)(stack - slot);
	  break;

	case FFI_TYPE_FLOAT:
#ifdef PA_LINUX
	  





	  switch (slot - FIRST_ARG_SLOT)
	    {
	    case 0: fstw(fr4, (void *)(stack - slot)); break;
	    case 1: fstw(fr5, (void *)(stack - slot)); break;
	    case 2: fstw(fr6, (void *)(stack - slot)); break;
	    case 3: fstw(fr7, (void *)(stack - slot)); break;
	    }
#endif
	  avalue[i] = (void *)(stack - slot);
	  break;

	case FFI_TYPE_DOUBLE:
	  slot += (slot & 1) ? 1 : 2;
#ifdef PA_LINUX
	  
	  switch (slot - FIRST_ARG_SLOT)
	    {
	    case 1: fstd(fr5, (void *)(stack - slot)); break;
	    case 3: fstd(fr7, (void *)(stack - slot)); break;
	    }
#endif
	  avalue[i] = (void *)(stack - slot);
	  break;

	case FFI_TYPE_STRUCT:
	  


	  if((*p_arg)->size <= 4)
	    {
	      avalue[i] = (void *)(stack - slot) + sizeof(UINT32) -
		(*p_arg)->size;
	    }
	  else if ((*p_arg)->size <= 8)
	    {
	      slot += (slot & 1) ? 1 : 2;
	      avalue[i] = (void *)(stack - slot) + sizeof(UINT64) -
		(*p_arg)->size;
	    }
	  else
	    avalue[i] = (void *) *(stack - slot);
	  break;

	default:
	  FFI_ASSERT(0);
	}

      slot++;
      p_arg++;
    }

  
  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  debug(3, "after calling function, ret[0] = %08x, ret[1] = %08x\n", ret[0],
	ret[1]);

  
  switch (cif->flags)
    {
    case FFI_TYPE_UINT8:
      *(stack - FIRST_ARG_SLOT) = (UINT8)(ret[0] >> 24);
      break;
    case FFI_TYPE_SINT8:
      *(stack - FIRST_ARG_SLOT) = (SINT8)(ret[0] >> 24);
      break;
    case FFI_TYPE_UINT16:
      *(stack - FIRST_ARG_SLOT) = (UINT16)(ret[0] >> 16);
      break;
    case FFI_TYPE_SINT16:
      *(stack - FIRST_ARG_SLOT) = (SINT16)(ret[0] >> 16);
      break;
    case FFI_TYPE_INT:
    case FFI_TYPE_SINT32:
    case FFI_TYPE_UINT32:
      *(stack - FIRST_ARG_SLOT) = ret[0];
      break;
    case FFI_TYPE_SINT64:
    case FFI_TYPE_UINT64:
      *(stack - FIRST_ARG_SLOT) = ret[0];
      *(stack - FIRST_ARG_SLOT - 1) = ret[1];
      break;

    case FFI_TYPE_DOUBLE:
      fldd(rvalue, fr4);
      break;

    case FFI_TYPE_FLOAT:
      fldw(rvalue, fr4);
      break;

    case FFI_TYPE_STRUCT:
      
      break;

    case FFI_TYPE_SMALL_STRUCT2:
    case FFI_TYPE_SMALL_STRUCT3:
    case FFI_TYPE_SMALL_STRUCT4:
      tmp = (void*)(stack -  FIRST_ARG_SLOT);
      tmp += 4 - cif->rtype->size;
      memcpy((void*)tmp, &ret[0], cif->rtype->size);
      break;

    case FFI_TYPE_SMALL_STRUCT5:
    case FFI_TYPE_SMALL_STRUCT6:
    case FFI_TYPE_SMALL_STRUCT7:
    case FFI_TYPE_SMALL_STRUCT8:
      {
	unsigned int ret2[2];
	int off;

	
	switch (cif->flags)
	  {
	    case FFI_TYPE_SMALL_STRUCT5: off = 3; break;
	    case FFI_TYPE_SMALL_STRUCT6: off = 2; break;
	    case FFI_TYPE_SMALL_STRUCT7: off = 1; break;
	    default: off = 0; break;
	  }

	memset (ret2, 0, sizeof (ret2));
	memcpy ((char *)ret2 + off, ret, 8 - off);

	*(stack - FIRST_ARG_SLOT) = ret2[0];
	*(stack - FIRST_ARG_SLOT - 1) = ret2[1];
      }
      break;

    case FFI_TYPE_POINTER:
    case FFI_TYPE_VOID:
      break;

    default:
      debug(0, "assert with cif->flags: %d\n",cif->flags);
      FFI_ASSERT(0);
      break;
    }
  return FFI_OK;
}





extern void ffi_closure_pa32(void);

ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*,void*,void**,void*),
		      void *user_data,
		      void *codeloc)
{
  UINT32 *tramp = (UINT32 *)(closure->tramp);
#ifdef PA_HPUX
  UINT32 *tmp;
#endif

  FFI_ASSERT (cif->abi == FFI_PA32);

  


#ifdef PA_LINUX
  tramp[0] = 0xeaa00000; 
  tramp[1] = 0xd6a01c1e; 
  tramp[2] = 0x4aa10028; 
  tramp[3] = 0x36b53ff1; 
  tramp[4] = 0x0c201096; 
  tramp[5] = 0xeac0c000; 
  tramp[6] = 0x0c281093; 
  tramp[7] = ((UINT32)(ffi_closure_pa32) & ~2);

  

  __asm__ volatile(
		   "fdc 0(%0)\n\t"
		   "fdc %1(%0)\n\t"
		   "fic 0(%%sr4, %0)\n\t"
		   "fic %1(%%sr4, %0)\n\t"
		   "sync\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   "nop\n"
		   :
		   : "r"((unsigned long)tramp & ~31),
		     "r"(32 )
		   : "memory");
#endif

#ifdef PA_HPUX
  tramp[0] = 0xeaa00000; 
  tramp[1] = 0xd6a01c1e; 
  tramp[2] = 0x4aa10038; 
  tramp[3] = 0x36b53ff1; 
  tramp[4] = 0x0c201096; 
  tramp[5] = 0x02c010b4; 
  tramp[6] = 0x00141820; 
  tramp[7] = 0xe2c00000; 
  tramp[8] = 0x0c281093; 
  tramp[9] = ((UINT32)(ffi_closure_pa32) & ~2);

  
  __asm__ volatile(
		   "copy %1,%0\n\t"
		   "fdc,m %2(%0)\n\t"
		   "fdc,m %2(%0)\n\t"
		   "fdc,m %2(%0)\n\t"
		   "ldsid (%1),%0\n\t"
		   "mtsp %0,%%sr0\n\t"
		   "copy %1,%0\n\t"
		   "fic,m %2(%%sr0,%0)\n\t"
		   "fic,m %2(%%sr0,%0)\n\t"
		   "fic,m %2(%%sr0,%0)\n\t"
		   "sync\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   "nop\n"
		   : "=&r" ((unsigned long)tmp)
		   : "r" ((unsigned long)tramp & ~31),
		     "r" (32/* stride */)
		   : "memory");
#endif

  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}
#endif

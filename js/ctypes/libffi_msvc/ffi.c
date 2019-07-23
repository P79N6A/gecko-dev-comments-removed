



























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>





void ffi_prep_args(char *stack, extended_cif *ecif)

{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;

  argp = stack;
  if (ecif->cif->rtype->type == FFI_TYPE_STRUCT)
    {
      *(void **) argp = ecif->rvalue;
      argp += sizeof(void *);
    }

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       i != 0;
       i--, p_arg++)
    {
      size_t z;

      
      if ((sizeof(void *) - 1) & (size_t) argp)
	argp = (char *) ALIGN(argp, sizeof(void *));

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

	    case FFI_TYPE_SINT32:
	      *(signed int *) argp = (signed int)*(SINT32 *)(* p_argv);
	      break;

	    case FFI_TYPE_UINT32:
	      *(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
	      break;

	    case FFI_TYPE_STRUCT:
	      *(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
	      break;

	    default:
	      FFI_ASSERT(0);
	    }
	}
      else
	{
	  memcpy(argp, *p_argv, z);
	}
      p_argv++;
      argp += z;
    }

  FFI_ASSERT(argp - stack <= ecif->cif->bytes);

  return;
}


ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
    case FFI_TYPE_STRUCT:
    case FFI_TYPE_SINT64:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
    case FFI_TYPE_LONGDOUBLE:
      cif->flags = (unsigned) cif->rtype->type;
      break;

    case FFI_TYPE_UINT64:
#ifdef _WIN64
    case FFI_TYPE_POINTER:
#endif
      cif->flags = FFI_TYPE_SINT64;
      break;

    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }

  return FFI_OK;
}

#ifdef _WIN32
extern int
ffi_call_x86(void (*)(char *, extended_cif *), 
	      extended_cif *, 
	     unsigned, unsigned, 
	      unsigned *, 
	     void (*fn)());
#endif

#ifdef _WIN64
extern int
ffi_call_AMD64(void (*)(char *, extended_cif *),
		  extended_cif *,
		 unsigned, unsigned,
		  unsigned *,
		 void (*fn)());
#endif

int
ffi_call( ffi_cif *cif, 
	 void (*fn)(), 
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
#if !defined(_WIN64)
    case FFI_SYSV:
    case FFI_STDCALL:
      return ffi_call_x86(ffi_prep_args, &ecif, cif->bytes, 
			  cif->flags, ecif.rvalue, fn);
      break;
#else
    case FFI_SYSV:
      
      
      return ffi_call_AMD64(ffi_prep_args, &ecif, cif->bytes ? cif->bytes : 40,
			   cif->flags, ecif.rvalue, fn);
      
      break;
#endif

    default:
      FFI_ASSERT(0);
      break;
    }
  return -1; 
}




static void ffi_prep_incoming_args_SYSV (char *stack, void **ret,
					  void** args, ffi_cif* cif);


#ifdef _WIN64
void *
#else
static void __fastcall
#endif
ffi_closure_SYSV (ffi_closure *closure, int *argp)
{
  
  long double    res;

  
  ffi_cif       *cif;
  void         **arg_area;
  unsigned short rtype;
  void          *resp = (void*)&res;
  void *args = &argp[1];

  cif         = closure->cif;
  arg_area    = (void**) alloca (cif->nargs * sizeof (void*));  

  





  ffi_prep_incoming_args_SYSV(args, (void**)&resp, arg_area, cif);
  
  (closure->fun) (cif, resp, arg_area, closure->user_data);

  rtype = cif->flags;

#if defined(_WIN32) && !defined(_WIN64)
#ifdef _MSC_VER
  
  if (rtype == FFI_TYPE_INT)
    {
	    _asm mov eax, resp ;
	    _asm mov eax, [eax] ;
    }
  else if (rtype == FFI_TYPE_FLOAT)
    {
	    _asm mov eax, resp ;
	    _asm fld DWORD PTR [eax] ;

    }
  else if (rtype == FFI_TYPE_DOUBLE)
    {
	    _asm mov eax, resp ;
	    _asm fld QWORD PTR [eax] ;

    }
  else if (rtype == FFI_TYPE_LONGDOUBLE)
    {

    }
  else if (rtype == FFI_TYPE_SINT64)
    {
	    _asm mov edx, resp ;
	    _asm mov eax, [edx] ;
	    _asm mov edx, [edx + 4] ;




    }
#else
  
  if (rtype == FFI_TYPE_INT)
    {
      asm ("movl (%0),%%eax" : : "r" (resp) : "eax");
    }
  else if (rtype == FFI_TYPE_FLOAT)
    {
      asm ("flds (%0)" : : "r" (resp) : "st" );
    }
  else if (rtype == FFI_TYPE_DOUBLE)
    {
      asm ("fldl (%0)" : : "r" (resp) : "st", "st(1)" );
    }
  else if (rtype == FFI_TYPE_LONGDOUBLE)
    {
      asm ("fldt (%0)" : : "r" (resp) : "st", "st(1)" );
    }
  else if (rtype == FFI_TYPE_SINT64)
    {
      asm ("movl 0(%0),%%eax;"
	   "movl 4(%0),%%edx" 
	   : : "r"(resp)
	   : "eax", "edx");
    }
#endif
#endif

#ifdef _WIN64
  



  return *(void **)resp;
#endif
}


static void 
ffi_prep_incoming_args_SYSV(char *stack, void **rvalue,
			    void **avalue, ffi_cif *cif)

{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;

  argp = stack;

  if ( cif->rtype->type == FFI_TYPE_STRUCT ) {
    *rvalue = *(void **) argp;
    argp += 4;
  }

  p_argv = avalue;

  for (i = cif->nargs, p_arg = cif->arg_types; (i != 0); i--, p_arg++)
    {
      size_t z;

      
      if ((sizeof(char *) - 1) & (size_t) argp) {
	argp = (char *) ALIGN(argp, sizeof(char*));
      }

      z = (*p_arg)->size;

      

      *p_argv = (void*) argp;

      p_argv++;
      argp += z;
    }
  
  return;
}


extern void ffi_closure_OUTER();

ffi_status
ffi_prep_closure (ffi_closure* closure,
		  ffi_cif* cif,
		  void (*fun)(ffi_cif*,void*,void**,void*),
		  void *user_data)
{
  short bytes;
  char *tramp;
#ifdef _WIN64
  int mask;
#endif
  FFI_ASSERT (cif->abi == FFI_SYSV);
  
  if (cif->abi == FFI_SYSV)
    bytes = 0;
#if !defined(_WIN64)
  else if (cif->abi == FFI_STDCALL)
    bytes = cif->bytes;
#endif
  else
    return FFI_BAD_ABI;

  tramp = &closure->tramp[0];

#define BYTES(text) memcpy(tramp, text, sizeof(text)), tramp += sizeof(text)-1
#define POINTER(x) *(void**)tramp = (void*)(x), tramp += sizeof(void*)
#define SHORT(x) *(short*)tramp = x, tramp += sizeof(short)
#define INT(x) *(int*)tramp = x, tramp += sizeof(int)

#ifdef _WIN64
  if (cif->nargs >= 1 &&
      (cif->arg_types[0]->type == FFI_TYPE_FLOAT
       || cif->arg_types[0]->type == FFI_TYPE_DOUBLE))
    mask |= 1;
  if (cif->nargs >= 2 &&
      (cif->arg_types[1]->type == FFI_TYPE_FLOAT
       || cif->arg_types[1]->type == FFI_TYPE_DOUBLE))
    mask |= 2;
  if (cif->nargs >= 3 &&
      (cif->arg_types[2]->type == FFI_TYPE_FLOAT
       || cif->arg_types[2]->type == FFI_TYPE_DOUBLE))
    mask |= 4;
  if (cif->nargs >= 4 &&
      (cif->arg_types[3]->type == FFI_TYPE_FLOAT
       || cif->arg_types[3]->type == FFI_TYPE_DOUBLE))
    mask |= 8;

  
  BYTES("\x41\xBB"); INT(mask);

  
  BYTES("\x48\xB8"); POINTER(closure);

  
  BYTES("\x49\xBA"); POINTER(ffi_closure_OUTER);

  
  BYTES("\x41\xFF\xE2");

#else

  
  BYTES("\xb9"); POINTER(closure);

  
  BYTES("\x8b\xd4");

  
  BYTES("\xe8"); POINTER((char*)&ffi_closure_SYSV - (tramp + 4));

  
  BYTES("\xc2");
  SHORT(bytes);
  
#endif

  FFI_ASSERT(tramp - &closure->tramp[0] <= FFI_TRAMPOLINE_SIZE);

  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}

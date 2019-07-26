

























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>
#include <stdio.h>


#define MAX_GPRARGS 3




#define FFIBFIN_RET_VOID 0
#define FFIBFIN_RET_BYTE 1
#define FFIBFIN_RET_HALFWORD 2
#define FFIBFIN_RET_INT64 3
#define FFIBFIN_RET_INT32 4




void ffi_prep_args(unsigned char *, extended_cif *);






extern void ffi_call_SYSV(unsigned, extended_cif *, void(*)(unsigned char *, extended_cif *), unsigned, void *, void(*fn)(void));











ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
   


   switch (cif->rtype->type) {
      case FFI_TYPE_VOID:
         cif->flags = FFIBFIN_RET_VOID;
         break;
      case FFI_TYPE_UINT16:
      case FFI_TYPE_SINT16:
         cif->flags = FFIBFIN_RET_HALFWORD;
         break;
      case FFI_TYPE_UINT8:
         cif->flags = FFIBFIN_RET_BYTE;
         break;
      case FFI_TYPE_INT:
      case FFI_TYPE_UINT32:
      case FFI_TYPE_SINT32:
      case FFI_TYPE_FLOAT:
      case FFI_TYPE_POINTER:
      case FFI_TYPE_SINT8:
         cif->flags = FFIBFIN_RET_INT32;
         break;
      case FFI_TYPE_SINT64:
      case FFI_TYPE_UINT64:
      case FFI_TYPE_DOUBLE:
          cif->flags = FFIBFIN_RET_INT64;
          break;
      case FFI_TYPE_STRUCT:
         if (cif->rtype->size <= 4){
        	 cif->flags = FFIBFIN_RET_INT32;
         }else if (cif->rtype->size == 8){
        	 cif->flags = FFIBFIN_RET_INT64;
         }else{
        	 
        	 cif->flags = FFIBFIN_RET_VOID;
         }
         break;
      default:
         FFI_ASSERT(0);
         break;
   }
   return FFI_OK;
}








void ffi_call(ffi_cif *cif, void(*fn)(void), void *rvalue, void **avalue)
{
   int ret_type = cif->flags;
   extended_cif ecif;
   ecif.cif = cif;
   ecif.avalue = avalue;
   ecif.rvalue = rvalue;

   switch (cif->abi) {
      case FFI_SYSV:
         ffi_call_SYSV(cif->bytes, &ecif, ffi_prep_args, ret_type, ecif.rvalue, fn);
         break;
      default:
         FFI_ASSERT(0);
         break;
   }
}







void ffi_prep_args(unsigned char *stack, extended_cif *ecif)
{
   register unsigned int i = 0;
   void **p_argv;
   unsigned char *argp;
   ffi_type **p_arg;
   argp = stack;
   p_argv = ecif->avalue;
   for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
        (i != 0);
        i--, p_arg++) {
      size_t z;
      z = (*p_arg)->size;
      if (z < sizeof(int)) {
         z = sizeof(int);
         switch ((*p_arg)->type) {
            case FFI_TYPE_SINT8: {
                  signed char v = *(SINT8 *)(* p_argv);
                  signed int t = v;
                  *(signed int *) argp = t;
               }
               break;
            case FFI_TYPE_UINT8: {
                  unsigned char v = *(UINT8 *)(* p_argv);
                  unsigned int t = v;
                  *(unsigned int *) argp = t;
               }
               break;
            case FFI_TYPE_SINT16:
               *(signed int *) argp = (signed int) * (SINT16 *)(* p_argv);
               break;
            case FFI_TYPE_UINT16:
               *(unsigned int *) argp = (unsigned int) * (UINT16 *)(* p_argv);
               break;
            case FFI_TYPE_STRUCT:
               memcpy(argp, *p_argv, (*p_arg)->size);
               break;
            default:
               FFI_ASSERT(0);
               break;
         }
      } else if (z == sizeof(int)) {
         *(unsigned int *) argp = (unsigned int) * (UINT32 *)(* p_argv);
      } else {
         memcpy(argp, *p_argv, z);
      }
      p_argv++;
      argp += z;
   }
}




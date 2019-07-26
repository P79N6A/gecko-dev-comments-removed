







#include "ffitest.h"

static void cls_ret_ulonglong_fn(ffi_cif* cif __UNUSED__, void* resp,
				 void** args, void* userdata __UNUSED__)
{
  *(unsigned long long *)resp= 0xfffffffffffffffLL ^ *(unsigned long long *)args[0];

  printf("%" PRIuLL ": %" PRIuLL "\n",*(unsigned long long *)args[0],
	 *(unsigned long long *)(resp));
}
typedef unsigned long long (*cls_ret_ulonglong)(unsigned long long);

int main (void)
{
  ffi_cif cif;
  void *code;
  ffi_closure *pcl = ffi_closure_alloc(sizeof(ffi_closure), &code);
  ffi_type * cl_arg_types[2];
  unsigned long long res;

  cl_arg_types[0] = &ffi_type_uint64;
  cl_arg_types[1] = NULL;

  
  CHECK(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1,
		     &ffi_type_uint64, cl_arg_types) == FFI_OK);
  CHECK(ffi_prep_closure_loc(pcl, &cif, cls_ret_ulonglong_fn, NULL, code)  == FFI_OK);
  res = (*((cls_ret_ulonglong)code))(214LL);
  
  printf("res: %" PRIdLL "\n", res);
  

  res = (*((cls_ret_ulonglong)code))(9223372035854775808LL);
  
  printf("res: %" PRIdLL "\n", res);
  

  exit(0);
}

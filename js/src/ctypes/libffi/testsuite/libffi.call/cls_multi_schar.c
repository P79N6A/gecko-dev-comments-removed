






#include "ffitest.h"

signed char test_func_fn(signed char a1, signed char a2)
{
  signed char result;

  result = a1 + a2;

  printf("%d %d: %d\n", a1, a2, result);

  return result;

}

static void test_func_gn(ffi_cif *cif __UNUSED__, void *rval, void **avals,
			 void *data __UNUSED__)
{
  signed char a1, a2;

  a1 = *(signed char *)avals[0];
  a2 = *(signed char *)avals[1];

  *(ffi_arg *)rval = test_func_fn(a1, a2);

}

typedef signed char (*test_type)(signed char, signed char);

int main (void)
{
  ffi_cif cif;
  void *code;
  ffi_closure *pcl = ffi_closure_alloc(sizeof(ffi_closure), &code);
  void * args_dbl[3];
  ffi_type * cl_arg_types[3];
  ffi_arg res_call;
  signed char a, b, res_closure;

  a = 2;
  b = 125;

  args_dbl[0] = &a;
  args_dbl[1] = &b;
  args_dbl[2] = NULL;

  cl_arg_types[0] = &ffi_type_schar;
  cl_arg_types[1] = &ffi_type_schar;
  cl_arg_types[2] = NULL;

  
  CHECK(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 2,
		     &ffi_type_schar, cl_arg_types) == FFI_OK);

  ffi_call(&cif, FFI_FN(test_func_fn), &res_call, args_dbl);
  
  printf("res: %d\n", (signed char)res_call);
  

  CHECK(ffi_prep_closure_loc(pcl, &cif, test_func_gn, NULL, code)  == FFI_OK);

  res_closure = (*((test_type)code))(2, 125);
  
  printf("res: %d\n", res_closure);
  

  exit(0);
}

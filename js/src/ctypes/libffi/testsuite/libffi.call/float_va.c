










#include <stdarg.h>

#include "ffitest.h"




double float_va_fn(unsigned int x, double y,...)
{
  double total=0.0;
  va_list ap;
  unsigned int i;

  total+=(double)x;
  total+=y;

  printf("%u: %.1f :", x, y);

  va_start(ap, y);
  for(i=0;i<x;i++)
  {
    double arg=va_arg(ap, double);
    total+=arg;
    printf(" %d:%.1f ", i, arg);
  }
  va_end(ap);

  printf(" total: %.1f\n", total);

  return total;
}

int main (void)
{
  ffi_cif    cif;

  ffi_type    *arg_types[5];
  void        *values[5];
  double        doubles[5];
  unsigned int firstarg;
  double        resfp;

  


  
  resfp=float_va_fn(0,2.0);
  
  printf("compiled: %.1f\n", resfp);
  

  arg_types[0] = &ffi_type_uint;
  arg_types[1] = &ffi_type_double;
  arg_types[2] = NULL;
  CHECK(ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, 2, 2,
        &ffi_type_double, arg_types) == FFI_OK);

  firstarg = 0;
  doubles[0] = 2.0;
  values[0] = &firstarg;
  values[1] = &doubles[0];
  ffi_call(&cif, FFI_FN(float_va_fn), &resfp, values);
  
  printf("ffi: %.1f\n", resfp);
  

  
  
  resfp=float_va_fn(2,2.0,3.0,4.0);
  
  printf("compiled: %.1f\n", resfp);
  

  arg_types[0] = &ffi_type_uint;
  arg_types[1] = &ffi_type_double;
  arg_types[2] = &ffi_type_double;
  arg_types[3] = &ffi_type_double;
  arg_types[4] = NULL;
  CHECK(ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, 2, 4,
        &ffi_type_double, arg_types) == FFI_OK);

  firstarg = 2;
  doubles[0] = 2.0;
  doubles[1] = 3.0;
  doubles[2] = 4.0;
  values[0] = &firstarg;
  values[1] = &doubles[0];
  values[2] = &doubles[1];
  values[3] = &doubles[2];
  ffi_call(&cif, FFI_FN(float_va_fn), &resfp, values);
  
  printf("ffi: %.1f\n", resfp);
  

  exit(0);
}

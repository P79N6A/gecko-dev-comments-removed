







#include "ffitest.h"

static size_t ABI_ATTR my_f(char *s, float a)
{
  return (size_t) ((int) strlen(s) + (int) a);
}

int main (void)
{
  ffi_cif cif;
  ffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  ffi_arg rint;
  char *s;
  float v2;
  args[0] = &ffi_type_pointer;
  args[1] = &ffi_type_float;
  values[0] = (void*) &s;
  values[1] = (void*) &v2;
  
  
  CHECK(ffi_prep_cif(&cif, ABI_NUM, 2,
		       &ffi_type_sint, args) == FFI_OK);
  
  s = "a";
  v2 = 0.0;
  ffi_call(&cif, FFI_FN(my_f), &rint, values);
  CHECK(rint == 1);
  
  s = "1234567";
  v2 = -1.0;
  ffi_call(&cif, FFI_FN(my_f), &rint, values);
  CHECK(rint == 6);
  
  s = "1234567890123456789012345";
  v2 = 1.0;
  ffi_call(&cif, FFI_FN(my_f), &rint, values);
  CHECK(rint == 26);
  
  exit(0);
}








#include "ffitest.h"
static long long return_ll(int ll0, long long ll1, int ll2)
{
  return ll0 + ll1 + ll2;
}

int main (void)
{
  ffi_cif cif;
  ffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  long long rlonglong;
  long long ll1;
  unsigned ll0, ll2;

  args[0] = &ffi_type_sint;
  args[1] = &ffi_type_sint64;
  args[2] = &ffi_type_sint;
  values[0] = &ll0;
  values[1] = &ll1;
  values[2] = &ll2;

  
  CHECK(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 3,
		     &ffi_type_sint64, args) == FFI_OK);

  ll0 = 11111111;
  ll1 = 11111111111000LL;
  ll2 = 11111111;

  ffi_call(&cif, FFI_FN(return_ll), &rlonglong, values);
  printf("res: %lld, %lld\n", rlonglong, ll0 + ll1 + ll2);
  
  exit(0);
}

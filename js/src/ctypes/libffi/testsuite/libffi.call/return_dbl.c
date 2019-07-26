






#include "ffitest.h"

static double return_dbl(double dbl)
{
  printf ("%f\n", dbl);
  return 2 * dbl;
}
int main (void)
{
  ffi_cif cif;
  ffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  double dbl, rdbl;

  args[0] = &ffi_type_double;
  values[0] = &dbl;

  
  CHECK(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1,
		     &ffi_type_double, args) == FFI_OK);

  for (dbl = -127.3; dbl <  127; dbl++)
    {
      ffi_call(&cif, FFI_FN(return_dbl), &rdbl, values);
      printf ("%f vs %f\n", rdbl, return_dbl(dbl));
      CHECK(rdbl == 2 * dbl);
    }
  exit(0);
}

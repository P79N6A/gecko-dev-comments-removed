






#include "ffitest.h"

static float return_fl(float fl)
{
  return 2 * fl;
}
int main (void)
{
  ffi_cif cif;
  ffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  float fl, rfl;

  args[0] = &ffi_type_float;
  values[0] = &fl;

  
  CHECK(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1,
		     &ffi_type_float, args) == FFI_OK);

  for (fl = -127.0; fl <  127; fl++)
    {
      ffi_call(&cif, FFI_FN(return_fl), &rfl, values);
      printf ("%f vs %f\n", rfl, return_fl(fl));
      CHECK(rfl ==  2 * fl);
    }
  exit(0);
}

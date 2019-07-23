






#include "ffitestcxx.h"

static int checking(int a __UNUSED__, short b __UNUSED__,
		    signed char c __UNUSED__)
{
  throw 9;
}

int main (void)
{
  ffi_cif cif;
  ffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  ffi_arg rint;

  signed int si;
  signed short ss;
  signed char sc;

  args[0] = &ffi_type_sint;
  values[0] = &si;
  args[1] = &ffi_type_sshort;
  values[1] = &ss;
  args[2] = &ffi_type_schar;
  values[2] = &sc;

  
  CHECK(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 3,
		     &ffi_type_sint, args) == FFI_OK);

  si = -6;
  ss = -12;
  sc = -1;
  {
    try
      {
	ffi_call(&cif, FFI_FN(checking), &rint, values);
      } catch (int exception_code)
      {
	CHECK(exception_code == 9);
      }
    printf("part one OK\n");
    
  }
  exit(0);
}

























#include <ffi.h>
#include <ffi_common.h>
#include <stdlib.h>
#include <stdio.h>



void ffi_stop_here(void)
{
  


}



void ffi_assert(char *expr, char *file, int line)
{
  fprintf(stderr, "ASSERTION FAILURE: %s at %s:%d\n", expr, file, line);
  ffi_stop_here();
  abort();
}



void ffi_type_test(ffi_type *a, char *file, int line)
{
  FFI_ASSERT_AT(a != NULL, file, line);

  FFI_ASSERT_AT(a->type <= FFI_TYPE_LAST, file, line);
  FFI_ASSERT_AT(a->type == FFI_TYPE_VOID || a->size > 0, file, line);
  FFI_ASSERT_AT(a->type == FFI_TYPE_VOID || a->alignment > 0, file, line);
  FFI_ASSERT_AT(a->type != FFI_TYPE_STRUCT || a->elements != NULL, file, line);

}

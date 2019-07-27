


















#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"            

#ifndef HAVE_STDLIB_H           
extern void * malloc (size_t size);
extern void free (void *ptr);
#endif







GLOBAL(void *)
jpeg_get_small (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void *) malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_small (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
  free(object);
}






GLOBAL(void *)
jpeg_get_large (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void *) malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_large (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
  free(object);
}







GLOBAL(size_t)
jpeg_mem_available (j_common_ptr cinfo, size_t min_bytes_needed,
                    size_t max_bytes_needed, size_t already_allocated)
{
  return max_bytes_needed;
}








GLOBAL(void)
jpeg_open_backing_store (j_common_ptr cinfo, backing_store_ptr info,
                         long total_bytes_needed)
{
  ERREXIT(cinfo, JERR_NO_BACKING_STORE);
}







GLOBAL(long)
jpeg_mem_init (j_common_ptr cinfo)
{
  return 0;                     
}

GLOBAL(void)
jpeg_mem_term (j_common_ptr cinfo)
{
  
}

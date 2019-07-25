










#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"













GLOBAL(void)
jpeg_abort (j_common_ptr cinfo)
{
  int pool;

  
  if (cinfo->mem == NULL)
    return;

  


  for (pool = JPOOL_NUMPOOLS-1; pool > JPOOL_PERMANENT; pool--) {
    (*cinfo->mem->free_pool) (cinfo, pool);
  }

  
  if (cinfo->is_decompressor) {
    cinfo->global_state = DSTATE_START;
    


    ((j_decompress_ptr) cinfo)->marker_list = NULL;
  } else {
    cinfo->global_state = CSTATE_START;
  }
}













GLOBAL(void)
jpeg_destroy (j_common_ptr cinfo)
{
  
  
  if (cinfo->mem != NULL)
    (*cinfo->mem->self_destruct) (cinfo);
  cinfo->mem = NULL;		
  cinfo->global_state = 0;	
}







GLOBAL(JQUANT_TBL *)
jpeg_alloc_quant_table (j_common_ptr cinfo)
{
  JQUANT_TBL *tbl;

  tbl = (JQUANT_TBL *)
    (*cinfo->mem->alloc_small) (cinfo, JPOOL_PERMANENT, SIZEOF(JQUANT_TBL));
  tbl->sent_table = FALSE;	
  return tbl;
}


GLOBAL(JHUFF_TBL *)
jpeg_alloc_huff_table (j_common_ptr cinfo)
{
  JHUFF_TBL *tbl;

  tbl = (JHUFF_TBL *)
    (*cinfo->mem->alloc_small) (cinfo, JPOOL_PERMANENT, SIZEOF(JHUFF_TBL));
  tbl->sent_table = FALSE;	
  return tbl;
}

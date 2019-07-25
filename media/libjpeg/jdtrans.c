











#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"



LOCAL(void) transdecode_master_selection JPP((j_decompress_ptr cinfo));
























GLOBAL(jvirt_barray_ptr *)
jpeg_read_coefficients (j_decompress_ptr cinfo)
{
  if (cinfo->global_state == DSTATE_READY) {
    
    transdecode_master_selection(cinfo);
    cinfo->global_state = DSTATE_RDCOEFS;
  }
  if (cinfo->global_state == DSTATE_RDCOEFS) {
    
    for (;;) {
      int retcode;
      
      if (cinfo->progress != NULL)
	(*cinfo->progress->progress_monitor) ((j_common_ptr) cinfo);
      
      retcode = (*cinfo->inputctl->consume_input) (cinfo);
      if (retcode == JPEG_SUSPENDED)
	return NULL;
      if (retcode == JPEG_REACHED_EOI)
	break;
      
      if (cinfo->progress != NULL &&
	  (retcode == JPEG_ROW_COMPLETED || retcode == JPEG_REACHED_SOS)) {
	if (++cinfo->progress->pass_counter >= cinfo->progress->pass_limit) {
	  
	  cinfo->progress->pass_limit += (long) cinfo->total_iMCU_rows;
	}
      }
    }
    
    cinfo->global_state = DSTATE_STOPPING;
  }
  



  if ((cinfo->global_state == DSTATE_STOPPING ||
       cinfo->global_state == DSTATE_BUFIMAGE) && cinfo->buffered_image) {
    return cinfo->coef->coef_arrays;
  }
  
  ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  return NULL;			
}







LOCAL(void)
transdecode_master_selection (j_decompress_ptr cinfo)
{
  
  cinfo->buffered_image = TRUE;

#if JPEG_LIB_VERSION >= 80
  
  jpeg_core_output_dimensions(cinfo);
#endif

  
  if (cinfo->arith_code) {
#ifdef D_ARITH_CODING_SUPPORTED
    jinit_arith_decoder(cinfo);
#else
    ERREXIT(cinfo, JERR_ARITH_NOTIMPL);
#endif
  } else {
    if (cinfo->progressive_mode) {
#ifdef D_PROGRESSIVE_SUPPORTED
      jinit_phuff_decoder(cinfo);
#else
      ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
    } else
      jinit_huff_decoder(cinfo);
  }

  
  jinit_d_coef_controller(cinfo, TRUE);

  
  (*cinfo->mem->realize_virt_arrays) ((j_common_ptr) cinfo);

  
  (*cinfo->inputctl->start_input_pass) (cinfo);

  
  if (cinfo->progress != NULL) {
    int nscans;
    
    if (cinfo->progressive_mode) {
      
      nscans = 2 + 3 * cinfo->num_components;
    } else if (cinfo->inputctl->has_multiple_scans) {
      
      nscans = cinfo->num_components;
    } else {
      nscans = 1;
    }
    cinfo->progress->pass_counter = 0L;
    cinfo->progress->pass_limit = (long) cinfo->total_iMCU_rows * nscans;
    cinfo->progress->completed_passes = 0;
    cinfo->progress->total_passes = 1;
  }
}

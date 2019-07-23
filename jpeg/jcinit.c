
















#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"








GLOBAL(void)
jinit_compress_master (j_compress_ptr cinfo)
{
  
  jinit_c_master_control(cinfo, FALSE );

  
  if (! cinfo->raw_data_in) {
    jinit_color_converter(cinfo);
    jinit_downsampler(cinfo);
    jinit_c_prep_controller(cinfo, FALSE );
  }
  
  jinit_forward_dct(cinfo);
  
  if (cinfo->arith_code) {
    ERREXIT(cinfo, JERR_ARITH_NOTIMPL);
  } else {
    if (cinfo->progressive_mode) {
#ifdef C_PROGRESSIVE_SUPPORTED
      jinit_phuff_encoder(cinfo);
#else
      ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
    } else
      jinit_huff_encoder(cinfo);
  }

  
  jinit_c_coef_controller(cinfo,
		(boolean) (cinfo->num_scans > 1 || cinfo->optimize_coding));
  jinit_c_main_controller(cinfo, FALSE );

  jinit_marker_writer(cinfo);

  
  (*cinfo->mem->realize_virt_arrays) ((j_common_ptr) cinfo);

  



  (*cinfo->marker->write_file_header) (cinfo);
}

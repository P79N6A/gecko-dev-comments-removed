












#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"



LOCAL(void) transencode_master_selection
	JPP((j_compress_ptr cinfo, jvirt_barray_ptr * coef_arrays));
LOCAL(void) transencode_coef_controller
	JPP((j_compress_ptr cinfo, jvirt_barray_ptr * coef_arrays));














GLOBAL(void)
jpeg_write_coefficients (j_compress_ptr cinfo, jvirt_barray_ptr * coef_arrays)
{
  if (cinfo->global_state != CSTATE_START)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  
  jpeg_suppress_tables(cinfo, FALSE);
  
  (*cinfo->err->reset_error_mgr) ((j_common_ptr) cinfo);
  (*cinfo->dest->init_destination) (cinfo);
  
  transencode_master_selection(cinfo, coef_arrays);
  
  cinfo->next_scanline = 0;	
  cinfo->global_state = CSTATE_WRCOEFS;
}









GLOBAL(void)
jpeg_copy_critical_parameters (j_decompress_ptr srcinfo,
			       j_compress_ptr dstinfo)
{
  JQUANT_TBL ** qtblptr;
  jpeg_component_info *incomp, *outcomp;
  JQUANT_TBL *c_quant, *slot_quant;
  int tblno, ci, coefi;

  
  if (dstinfo->global_state != CSTATE_START)
    ERREXIT1(dstinfo, JERR_BAD_STATE, dstinfo->global_state);
  
  dstinfo->image_width = srcinfo->image_width;
  dstinfo->image_height = srcinfo->image_height;
  dstinfo->input_components = srcinfo->num_components;
  dstinfo->in_color_space = srcinfo->jpeg_color_space;
#if JPEG_LIB_VERSION >= 70
  dstinfo->jpeg_width = srcinfo->output_width;
  dstinfo->jpeg_height = srcinfo->output_height;
  dstinfo->min_DCT_h_scaled_size = srcinfo->min_DCT_h_scaled_size;
  dstinfo->min_DCT_v_scaled_size = srcinfo->min_DCT_v_scaled_size;
#endif
  
  jpeg_set_defaults(dstinfo);
  


  jpeg_set_colorspace(dstinfo, srcinfo->jpeg_color_space);
  dstinfo->data_precision = srcinfo->data_precision;
  dstinfo->CCIR601_sampling = srcinfo->CCIR601_sampling;
  
  for (tblno = 0; tblno < NUM_QUANT_TBLS; tblno++) {
    if (srcinfo->quant_tbl_ptrs[tblno] != NULL) {
      qtblptr = & dstinfo->quant_tbl_ptrs[tblno];
      if (*qtblptr == NULL)
	*qtblptr = jpeg_alloc_quant_table((j_common_ptr) dstinfo);
      MEMCOPY((*qtblptr)->quantval,
	      srcinfo->quant_tbl_ptrs[tblno]->quantval,
	      SIZEOF((*qtblptr)->quantval));
      (*qtblptr)->sent_table = FALSE;
    }
  }
  


  dstinfo->num_components = srcinfo->num_components;
  if (dstinfo->num_components < 1 || dstinfo->num_components > MAX_COMPONENTS)
    ERREXIT2(dstinfo, JERR_COMPONENT_COUNT, dstinfo->num_components,
	     MAX_COMPONENTS);
  for (ci = 0, incomp = srcinfo->comp_info, outcomp = dstinfo->comp_info;
       ci < dstinfo->num_components; ci++, incomp++, outcomp++) {
    outcomp->component_id = incomp->component_id;
    outcomp->h_samp_factor = incomp->h_samp_factor;
    outcomp->v_samp_factor = incomp->v_samp_factor;
    outcomp->quant_tbl_no = incomp->quant_tbl_no;
    



    tblno = outcomp->quant_tbl_no;
    if (tblno < 0 || tblno >= NUM_QUANT_TBLS ||
	srcinfo->quant_tbl_ptrs[tblno] == NULL)
      ERREXIT1(dstinfo, JERR_NO_QUANT_TABLE, tblno);
    slot_quant = srcinfo->quant_tbl_ptrs[tblno];
    c_quant = incomp->quant_table;
    if (c_quant != NULL) {
      for (coefi = 0; coefi < DCTSIZE2; coefi++) {
	if (c_quant->quantval[coefi] != slot_quant->quantval[coefi])
	  ERREXIT1(dstinfo, JERR_MISMATCHED_QUANT_TABLE, tblno);
      }
    }
    


  }
  







  if (srcinfo->saw_JFIF_marker) {
    if (srcinfo->JFIF_major_version == 1) {
      dstinfo->JFIF_major_version = srcinfo->JFIF_major_version;
      dstinfo->JFIF_minor_version = srcinfo->JFIF_minor_version;
    }
    dstinfo->density_unit = srcinfo->density_unit;
    dstinfo->X_density = srcinfo->X_density;
    dstinfo->Y_density = srcinfo->Y_density;
  }
}







LOCAL(void)
transencode_master_selection (j_compress_ptr cinfo,
			      jvirt_barray_ptr * coef_arrays)
{
  


  cinfo->input_components = 1;
  
  jinit_c_master_control(cinfo, TRUE );

  
  if (cinfo->arith_code) {
#ifdef C_ARITH_CODING_SUPPORTED
    jinit_arith_encoder(cinfo);
#else
    ERREXIT(cinfo, JERR_ARITH_NOTIMPL);
#endif
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

  
  transencode_coef_controller(cinfo, coef_arrays);

  jinit_marker_writer(cinfo);

  
  (*cinfo->mem->realize_virt_arrays) ((j_common_ptr) cinfo);

  



  (*cinfo->marker->write_file_header) (cinfo);
}












typedef struct {
  struct jpeg_c_coef_controller pub; 

  JDIMENSION iMCU_row_num;	
  JDIMENSION mcu_ctr;		
  int MCU_vert_offset;		
  int MCU_rows_per_iMCU_row;	

  
  jvirt_barray_ptr * whole_image;

  
  JBLOCKROW dummy_buffer[C_MAX_BLOCKS_IN_MCU];
} my_coef_controller;

typedef my_coef_controller * my_coef_ptr;


LOCAL(void)
start_iMCU_row (j_compress_ptr cinfo)

{
  my_coef_ptr coef = (my_coef_ptr) cinfo->coef;

  



  if (cinfo->comps_in_scan > 1) {
    coef->MCU_rows_per_iMCU_row = 1;
  } else {
    if (coef->iMCU_row_num < (cinfo->total_iMCU_rows-1))
      coef->MCU_rows_per_iMCU_row = cinfo->cur_comp_info[0]->v_samp_factor;
    else
      coef->MCU_rows_per_iMCU_row = cinfo->cur_comp_info[0]->last_row_height;
  }

  coef->mcu_ctr = 0;
  coef->MCU_vert_offset = 0;
}






METHODDEF(void)
start_pass_coef (j_compress_ptr cinfo, J_BUF_MODE pass_mode)
{
  my_coef_ptr coef = (my_coef_ptr) cinfo->coef;

  if (pass_mode != JBUF_CRANK_DEST)
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);

  coef->iMCU_row_num = 0;
  start_iMCU_row(cinfo);
}












METHODDEF(boolean)
compress_output (j_compress_ptr cinfo, JSAMPIMAGE input_buf)
{
  my_coef_ptr coef = (my_coef_ptr) cinfo->coef;
  JDIMENSION MCU_col_num;	
  JDIMENSION last_MCU_col = cinfo->MCUs_per_row - 1;
  JDIMENSION last_iMCU_row = cinfo->total_iMCU_rows - 1;
  int blkn, ci, xindex, yindex, yoffset, blockcnt;
  JDIMENSION start_col;
  JBLOCKARRAY buffer[MAX_COMPS_IN_SCAN];
  JBLOCKROW MCU_buffer[C_MAX_BLOCKS_IN_MCU];
  JBLOCKROW buffer_ptr;
  jpeg_component_info *compptr;

  
  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    buffer[ci] = (*cinfo->mem->access_virt_barray)
      ((j_common_ptr) cinfo, coef->whole_image[compptr->component_index],
       coef->iMCU_row_num * compptr->v_samp_factor,
       (JDIMENSION) compptr->v_samp_factor, FALSE);
  }

  
  for (yoffset = coef->MCU_vert_offset; yoffset < coef->MCU_rows_per_iMCU_row;
       yoffset++) {
    for (MCU_col_num = coef->mcu_ctr; MCU_col_num < cinfo->MCUs_per_row;
	 MCU_col_num++) {
      
      blkn = 0;			
      for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
	compptr = cinfo->cur_comp_info[ci];
	start_col = MCU_col_num * compptr->MCU_width;
	blockcnt = (MCU_col_num < last_MCU_col) ? compptr->MCU_width
						: compptr->last_col_width;
	for (yindex = 0; yindex < compptr->MCU_height; yindex++) {
	  if (coef->iMCU_row_num < last_iMCU_row ||
	      yindex+yoffset < compptr->last_row_height) {
	    
	    buffer_ptr = buffer[ci][yindex+yoffset] + start_col;
	    for (xindex = 0; xindex < blockcnt; xindex++)
	      MCU_buffer[blkn++] = buffer_ptr++;
	  } else {
	    
	    xindex = 0;
	  }
	  





	  for (; xindex < compptr->MCU_width; xindex++) {
	    MCU_buffer[blkn] = coef->dummy_buffer[blkn];
	    MCU_buffer[blkn][0][0] = MCU_buffer[blkn-1][0][0];
	    blkn++;
	  }
	}
      }
      
      if (! (*cinfo->entropy->encode_mcu) (cinfo, MCU_buffer)) {
	
	coef->MCU_vert_offset = yoffset;
	coef->mcu_ctr = MCU_col_num;
	return FALSE;
      }
    }
    
    coef->mcu_ctr = 0;
  }
  
  coef->iMCU_row_num++;
  start_iMCU_row(cinfo);
  return TRUE;
}










LOCAL(void)
transencode_coef_controller (j_compress_ptr cinfo,
			     jvirt_barray_ptr * coef_arrays)
{
  my_coef_ptr coef;
  JBLOCKROW buffer;
  int i;

  coef = (my_coef_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_coef_controller));
  cinfo->coef = (struct jpeg_c_coef_controller *) coef;
  coef->pub.start_pass = start_pass_coef;
  coef->pub.compress_data = compress_output;

  
  coef->whole_image = coef_arrays;

  
  buffer = (JBLOCKROW)
    (*cinfo->mem->alloc_large) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				C_MAX_BLOCKS_IN_MCU * SIZEOF(JBLOCK));
  jzero_far((void FAR *) buffer, C_MAX_BLOCKS_IN_MCU * SIZEOF(JBLOCK));
  for (i = 0; i < C_MAX_BLOCKS_IN_MCU; i++) {
    coef->dummy_buffer[i] = buffer + i;
  }
}

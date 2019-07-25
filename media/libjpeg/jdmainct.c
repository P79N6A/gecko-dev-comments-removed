















#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jpegcomp.h"































































































typedef struct {
  struct jpeg_d_main_controller pub; 

  
  JSAMPARRAY buffer[MAX_COMPONENTS];

  boolean buffer_full;		
  JDIMENSION rowgroup_ctr;	

  

  
  JSAMPIMAGE xbuffer[2];	

  int whichptr;			
  int context_state;		
  JDIMENSION rowgroups_avail;	
  JDIMENSION iMCU_row_ctr;	
} my_main_controller;

typedef my_main_controller * my_main_ptr;


#define CTX_PREPARE_FOR_IMCU	0	/* need to prepare for MCU row */
#define CTX_PROCESS_IMCU	1	/* feeding iMCU to postprocessor */
#define CTX_POSTPONED_ROW	2	/* feeding postponed row group */



METHODDEF(void) process_data_simple_main
	JPP((j_decompress_ptr cinfo, JSAMPARRAY output_buf,
	     JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail));
METHODDEF(void) process_data_context_main
	JPP((j_decompress_ptr cinfo, JSAMPARRAY output_buf,
	     JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail));
#ifdef QUANT_2PASS_SUPPORTED
METHODDEF(void) process_data_crank_post
	JPP((j_decompress_ptr cinfo, JSAMPARRAY output_buf,
	     JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail));
#endif


LOCAL(void)
alloc_funny_pointers (j_decompress_ptr cinfo)



{
  my_main_ptr main_ptr = (my_main_ptr) cinfo->main;
  int ci, rgroup;
  int M = cinfo->_min_DCT_scaled_size;
  jpeg_component_info *compptr;
  JSAMPARRAY xbuf;

  


  main_ptr->xbuffer[0] = (JSAMPIMAGE)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				cinfo->num_components * 2 * SIZEOF(JSAMPARRAY));
  main_ptr->xbuffer[1] = main_ptr->xbuffer[0] + cinfo->num_components;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    rgroup = (compptr->v_samp_factor * compptr->_DCT_scaled_size) /
      cinfo->_min_DCT_scaled_size; 
    


    xbuf = (JSAMPARRAY)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  2 * (rgroup * (M + 4)) * SIZEOF(JSAMPROW));
    xbuf += rgroup;		
    main_ptr->xbuffer[0][ci] = xbuf;
    xbuf += rgroup * (M + 4);
    main_ptr->xbuffer[1][ci] = xbuf;
  }
}


LOCAL(void)
make_funny_pointers (j_decompress_ptr cinfo)






{
  my_main_ptr main_ptr = (my_main_ptr) cinfo->main;
  int ci, i, rgroup;
  int M = cinfo->_min_DCT_scaled_size;
  jpeg_component_info *compptr;
  JSAMPARRAY buf, xbuf0, xbuf1;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    rgroup = (compptr->v_samp_factor * compptr->_DCT_scaled_size) /
      cinfo->_min_DCT_scaled_size; 
    xbuf0 = main_ptr->xbuffer[0][ci];
    xbuf1 = main_ptr->xbuffer[1][ci];
    
    buf = main_ptr->buffer[ci];
    for (i = 0; i < rgroup * (M + 2); i++) {
      xbuf0[i] = xbuf1[i] = buf[i];
    }
    
    for (i = 0; i < rgroup * 2; i++) {
      xbuf1[rgroup*(M-2) + i] = buf[rgroup*M + i];
      xbuf1[rgroup*M + i] = buf[rgroup*(M-2) + i];
    }
    




    for (i = 0; i < rgroup; i++) {
      xbuf0[i - rgroup] = xbuf0[0];
    }
  }
}


LOCAL(void)
set_wraparound_pointers (j_decompress_ptr cinfo)



{
  my_main_ptr main_ptr = (my_main_ptr) cinfo->main;
  int ci, i, rgroup;
  int M = cinfo->_min_DCT_scaled_size;
  jpeg_component_info *compptr;
  JSAMPARRAY xbuf0, xbuf1;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    rgroup = (compptr->v_samp_factor * compptr->_DCT_scaled_size) /
      cinfo->_min_DCT_scaled_size; 
    xbuf0 = main_ptr->xbuffer[0][ci];
    xbuf1 = main_ptr->xbuffer[1][ci];
    for (i = 0; i < rgroup; i++) {
      xbuf0[i - rgroup] = xbuf0[rgroup*(M+1) + i];
      xbuf1[i - rgroup] = xbuf1[rgroup*(M+1) + i];
      xbuf0[rgroup*(M+2) + i] = xbuf0[i];
      xbuf1[rgroup*(M+2) + i] = xbuf1[i];
    }
  }
}


LOCAL(void)
set_bottom_pointers (j_decompress_ptr cinfo)




{
  my_main_ptr main_ptr = (my_main_ptr) cinfo->main;
  int ci, i, rgroup, iMCUheight, rows_left;
  jpeg_component_info *compptr;
  JSAMPARRAY xbuf;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    
    iMCUheight = compptr->v_samp_factor * compptr->_DCT_scaled_size;
    rgroup = iMCUheight / cinfo->_min_DCT_scaled_size;
    
    rows_left = (int) (compptr->downsampled_height % (JDIMENSION) iMCUheight);
    if (rows_left == 0) rows_left = iMCUheight;
    


    if (ci == 0) {
      main_ptr->rowgroups_avail = (JDIMENSION) ((rows_left-1) / rgroup + 1);
    }
    


    xbuf = main_ptr->xbuffer[main_ptr->whichptr][ci];
    for (i = 0; i < rgroup * 2; i++) {
      xbuf[rows_left + i] = xbuf[rows_left-1];
    }
  }
}






METHODDEF(void)
start_pass_main (j_decompress_ptr cinfo, J_BUF_MODE pass_mode)
{
  my_main_ptr main_ptr = (my_main_ptr) cinfo->main;

  switch (pass_mode) {
  case JBUF_PASS_THRU:
    if (cinfo->upsample->need_context_rows) {
      main_ptr->pub.process_data = process_data_context_main;
      make_funny_pointers(cinfo); 
      main_ptr->whichptr = 0;	
      main_ptr->context_state = CTX_PREPARE_FOR_IMCU;
      main_ptr->iMCU_row_ctr = 0;
    } else {
      
      main_ptr->pub.process_data = process_data_simple_main;
    }
    main_ptr->buffer_full = FALSE;	
    main_ptr->rowgroup_ctr = 0;
    break;
#ifdef QUANT_2PASS_SUPPORTED
  case JBUF_CRANK_DEST:
    
    main_ptr->pub.process_data = process_data_crank_post;
    break;
#endif
  default:
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    break;
  }
}







METHODDEF(void)
process_data_simple_main (j_decompress_ptr cinfo,
			  JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
			  JDIMENSION out_rows_avail)
{
  my_main_ptr main_ptr = (my_main_ptr) cinfo->main;
  JDIMENSION rowgroups_avail;

  
  if (! main_ptr->buffer_full) {
    if (! (*cinfo->coef->decompress_data) (cinfo, main_ptr->buffer))
      return;			
    main_ptr->buffer_full = TRUE;	
  }

  
  rowgroups_avail = (JDIMENSION) cinfo->_min_DCT_scaled_size;
  




  
  (*cinfo->post->post_process_data) (cinfo, main_ptr->buffer,
				     &main_ptr->rowgroup_ctr, rowgroups_avail,
				     output_buf, out_row_ctr, out_rows_avail);

  
  if (main_ptr->rowgroup_ctr >= rowgroups_avail) {
    main_ptr->buffer_full = FALSE;
    main_ptr->rowgroup_ctr = 0;
  }
}







METHODDEF(void)
process_data_context_main (j_decompress_ptr cinfo,
			   JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
			   JDIMENSION out_rows_avail)
{
  my_main_ptr main_ptr = (my_main_ptr) cinfo->main;

  
  if (! main_ptr->buffer_full) {
    if (! (*cinfo->coef->decompress_data) (cinfo,
					   main_ptr->xbuffer[main_ptr->whichptr]))
      return;			
    main_ptr->buffer_full = TRUE;	
    main_ptr->iMCU_row_ctr++;	
  }

  




  switch (main_ptr->context_state) {
  case CTX_POSTPONED_ROW:
    
    (*cinfo->post->post_process_data) (cinfo, main_ptr->xbuffer[main_ptr->whichptr],
			&main_ptr->rowgroup_ctr, main_ptr->rowgroups_avail,
			output_buf, out_row_ctr, out_rows_avail);
    if (main_ptr->rowgroup_ctr < main_ptr->rowgroups_avail)
      return;			
    main_ptr->context_state = CTX_PREPARE_FOR_IMCU;
    if (*out_row_ctr >= out_rows_avail)
      return;			
    
  case CTX_PREPARE_FOR_IMCU:
    
    main_ptr->rowgroup_ctr = 0;
    main_ptr->rowgroups_avail = (JDIMENSION) (cinfo->_min_DCT_scaled_size - 1);
    


    if (main_ptr->iMCU_row_ctr == cinfo->total_iMCU_rows)
      set_bottom_pointers(cinfo);
    main_ptr->context_state = CTX_PROCESS_IMCU;
    
  case CTX_PROCESS_IMCU:
    
    (*cinfo->post->post_process_data) (cinfo, main_ptr->xbuffer[main_ptr->whichptr],
			&main_ptr->rowgroup_ctr, main_ptr->rowgroups_avail,
			output_buf, out_row_ctr, out_rows_avail);
    if (main_ptr->rowgroup_ctr < main_ptr->rowgroups_avail)
      return;			
    
    if (main_ptr->iMCU_row_ctr == 1)
      set_wraparound_pointers(cinfo);
    
    main_ptr->whichptr ^= 1;	
    main_ptr->buffer_full = FALSE;
    
    
    main_ptr->rowgroup_ctr = (JDIMENSION) (cinfo->_min_DCT_scaled_size + 1);
    main_ptr->rowgroups_avail = (JDIMENSION) (cinfo->_min_DCT_scaled_size + 2);
    main_ptr->context_state = CTX_POSTPONED_ROW;
  }
}








#ifdef QUANT_2PASS_SUPPORTED

METHODDEF(void)
process_data_crank_post (j_decompress_ptr cinfo,
			 JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
			 JDIMENSION out_rows_avail)
{
  (*cinfo->post->post_process_data) (cinfo, (JSAMPIMAGE) NULL,
				     (JDIMENSION *) NULL, (JDIMENSION) 0,
				     output_buf, out_row_ctr, out_rows_avail);
}

#endif 






GLOBAL(void)
jinit_d_main_controller (j_decompress_ptr cinfo, boolean need_full_buffer)
{
  my_main_ptr main_ptr;
  int ci, rgroup, ngroups;
  jpeg_component_info *compptr;

  main_ptr = (my_main_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_main_controller));
  cinfo->main = (struct jpeg_d_main_controller *) main_ptr;
  main_ptr->pub.start_pass = start_pass_main;

  if (need_full_buffer)		
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);

  


  if (cinfo->upsample->need_context_rows) {
    if (cinfo->_min_DCT_scaled_size < 2) 
      ERREXIT(cinfo, JERR_NOTIMPL);
    alloc_funny_pointers(cinfo); 
    ngroups = cinfo->_min_DCT_scaled_size + 2;
  } else {
    ngroups = cinfo->_min_DCT_scaled_size;
  }

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    rgroup = (compptr->v_samp_factor * compptr->_DCT_scaled_size) /
      cinfo->_min_DCT_scaled_size; 
    main_ptr->buffer[ci] = (*cinfo->mem->alloc_sarray)
			((j_common_ptr) cinfo, JPOOL_IMAGE,
			 compptr->width_in_blocks * compptr->_DCT_scaled_size,
			 (JDIMENSION) (rgroup * ngroups));
  }
}













#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"







#undef FULL_MAIN_BUFFER_SUPPORTED




typedef struct {
  struct jpeg_c_main_controller pub; 

  JDIMENSION cur_iMCU_row;	
  JDIMENSION rowgroup_ctr;	
  boolean suspended;		
  J_BUF_MODE pass_mode;		

  



  JSAMPARRAY buffer[MAX_COMPONENTS];

#ifdef FULL_MAIN_BUFFER_SUPPORTED
  


  jvirt_sarray_ptr whole_image[MAX_COMPONENTS];
#endif
} my_main_controller;

typedef my_main_controller * my_main_ptr;



METHODDEF(void) process_data_simple_main
	JPP((j_compress_ptr cinfo, JSAMPARRAY input_buf,
	     JDIMENSION *in_row_ctr, JDIMENSION in_rows_avail));
#ifdef FULL_MAIN_BUFFER_SUPPORTED
METHODDEF(void) process_data_buffer_main
	JPP((j_compress_ptr cinfo, JSAMPARRAY input_buf,
	     JDIMENSION *in_row_ctr, JDIMENSION in_rows_avail));
#endif






METHODDEF(void)
start_pass_main (j_compress_ptr cinfo, J_BUF_MODE pass_mode)
{
  my_main_ptr main_ptr = (my_main_ptr) cinfo->main;

  
  if (cinfo->raw_data_in)
    return;

  main_ptr->cur_iMCU_row = 0;	
  main_ptr->rowgroup_ctr = 0;
  main_ptr->suspended = FALSE;
  main_ptr->pass_mode = pass_mode;	

  switch (pass_mode) {
  case JBUF_PASS_THRU:
#ifdef FULL_MAIN_BUFFER_SUPPORTED
    if (main_ptr->whole_image[0] != NULL)
      ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
#endif
    main_ptr->pub.process_data = process_data_simple_main;
    break;
#ifdef FULL_MAIN_BUFFER_SUPPORTED
  case JBUF_SAVE_SOURCE:
  case JBUF_CRANK_DEST:
  case JBUF_SAVE_AND_PASS:
    if (main_ptr->whole_image[0] == NULL)
      ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    main_ptr->pub.process_data = process_data_buffer_main;
    break;
#endif
  default:
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    break;
  }
}








METHODDEF(void)
process_data_simple_main (j_compress_ptr cinfo,
			  JSAMPARRAY input_buf, JDIMENSION *in_row_ctr,
			  JDIMENSION in_rows_avail)
{
  my_main_ptr main_ptr = (my_main_ptr) cinfo->main;

  while (main_ptr->cur_iMCU_row < cinfo->total_iMCU_rows) {
    
    if (main_ptr->rowgroup_ctr < DCTSIZE)
      (*cinfo->prep->pre_process_data) (cinfo,
					input_buf, in_row_ctr, in_rows_avail,
					main_ptr->buffer, &main_ptr->rowgroup_ctr,
					(JDIMENSION) DCTSIZE);

    



    if (main_ptr->rowgroup_ctr != DCTSIZE)
      return;

    
    if (! (*cinfo->coef->compress_data) (cinfo, main_ptr->buffer)) {
      





      if (! main_ptr->suspended) {
	(*in_row_ctr)--;
	main_ptr->suspended = TRUE;
      }
      return;
    }
    


    if (main_ptr->suspended) {
      (*in_row_ctr)++;
      main_ptr->suspended = FALSE;
    }
    main_ptr->rowgroup_ctr = 0;
    main_ptr->cur_iMCU_row++;
  }
}


#ifdef FULL_MAIN_BUFFER_SUPPORTED






METHODDEF(void)
process_data_buffer_main (j_compress_ptr cinfo,
			  JSAMPARRAY input_buf, JDIMENSION *in_row_ctr,
			  JDIMENSION in_rows_avail)
{
  my_main_ptr main_ptr = (my_main_ptr) cinfo->main;
  int ci;
  jpeg_component_info *compptr;
  boolean writing = (main_ptr->pass_mode != JBUF_CRANK_DEST);

  while (main_ptr->cur_iMCU_row < cinfo->total_iMCU_rows) {
    
    if (main_ptr->rowgroup_ctr == 0) {
      for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
	   ci++, compptr++) {
	main_ptr->buffer[ci] = (*cinfo->mem->access_virt_sarray)
	  ((j_common_ptr) cinfo, main_ptr->whole_image[ci],
	   main_ptr->cur_iMCU_row * (compptr->v_samp_factor * DCTSIZE),
	   (JDIMENSION) (compptr->v_samp_factor * DCTSIZE), writing);
      }
      
      if (! writing) {
	*in_row_ctr += cinfo->max_v_samp_factor * DCTSIZE;
	main_ptr->rowgroup_ctr = DCTSIZE;
      }
    }

    
    
    if (writing) {
      (*cinfo->prep->pre_process_data) (cinfo,
					input_buf, in_row_ctr, in_rows_avail,
					main_ptr->buffer, &main_ptr->rowgroup_ctr,
					(JDIMENSION) DCTSIZE);
      
      if (main_ptr->rowgroup_ctr < DCTSIZE)
	return;
    }

    
    if (main_ptr->pass_mode != JBUF_SAVE_SOURCE) {
      if (! (*cinfo->coef->compress_data) (cinfo, main_ptr->buffer)) {
	





	if (! main_ptr->suspended) {
	  (*in_row_ctr)--;
	  main_ptr->suspended = TRUE;
	}
	return;
      }
      


      if (main_ptr->suspended) {
	(*in_row_ctr)++;
	main_ptr->suspended = FALSE;
      }
    }

    
    main_ptr->rowgroup_ctr = 0;
    main_ptr->cur_iMCU_row++;
  }
}

#endif 






GLOBAL(void)
jinit_c_main_controller (j_compress_ptr cinfo, boolean need_full_buffer)
{
  my_main_ptr main_ptr;
  int ci;
  jpeg_component_info *compptr;

  main_ptr = (my_main_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_main_controller));
  cinfo->main = (struct jpeg_c_main_controller *) main_ptr;
  main_ptr->pub.start_pass = start_pass_main;

  
  if (cinfo->raw_data_in)
    return;

  


  if (need_full_buffer) {
#ifdef FULL_MAIN_BUFFER_SUPPORTED
    
    
    for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
	 ci++, compptr++) {
      main_ptr->whole_image[ci] = (*cinfo->mem->request_virt_sarray)
	((j_common_ptr) cinfo, JPOOL_IMAGE, FALSE,
	 compptr->width_in_blocks * DCTSIZE,
	 (JDIMENSION) jround_up((long) compptr->height_in_blocks,
				(long) compptr->v_samp_factor) * DCTSIZE,
	 (JDIMENSION) (compptr->v_samp_factor * DCTSIZE));
    }
#else
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
#endif
  } else {
#ifdef FULL_MAIN_BUFFER_SUPPORTED
    main_ptr->whole_image[0] = NULL; 
#endif
    
    for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
	 ci++, compptr++) {
      main_ptr->buffer[ci] = (*cinfo->mem->alloc_sarray)
	((j_common_ptr) cinfo, JPOOL_IMAGE,
	 compptr->width_in_blocks * DCTSIZE,
	 (JDIMENSION) (compptr->v_samp_factor * DCTSIZE));
    }
  }
}

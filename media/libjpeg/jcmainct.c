













#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"




typedef struct {
  struct jpeg_c_main_controller pub; 

  JDIMENSION cur_iMCU_row;      
  JDIMENSION rowgroup_ctr;      
  boolean suspended;            
  J_BUF_MODE pass_mode;         

  



  JSAMPARRAY buffer[MAX_COMPONENTS];
} my_main_controller;

typedef my_main_controller * my_main_ptr;



METHODDEF(void) process_data_simple_main
        (j_compress_ptr cinfo, JSAMPARRAY input_buf, JDIMENSION *in_row_ctr,
         JDIMENSION in_rows_avail);






METHODDEF(void)
start_pass_main (j_compress_ptr cinfo, J_BUF_MODE pass_mode)
{
  my_main_ptr main_ptr = (my_main_ptr) cinfo->main;

  
  if (cinfo->raw_data_in)
    return;

  if (pass_mode != JBUF_PASS_THRU)
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);

  main_ptr->cur_iMCU_row = 0;   
  main_ptr->rowgroup_ctr = 0;
  main_ptr->suspended = FALSE;
  main_ptr->pass_mode = pass_mode;      
  main_ptr->pub.process_data = process_data_simple_main;
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






GLOBAL(void)
jinit_c_main_controller (j_compress_ptr cinfo, boolean need_full_buffer)
{
  my_main_ptr main_ptr;
  int ci;
  jpeg_component_info *compptr;

  main_ptr = (my_main_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                sizeof(my_main_controller));
  cinfo->main = (struct jpeg_c_main_controller *) main_ptr;
  main_ptr->pub.start_pass = start_pass_main;

  
  if (cinfo->raw_data_in)
    return;

  


  if (need_full_buffer) {
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
  } else {
    
    for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
         ci++, compptr++) {
      main_ptr->buffer[ci] = (*cinfo->mem->alloc_sarray)
        ((j_common_ptr) cinfo, JPOOL_IMAGE,
         compptr->width_in_blocks * DCTSIZE,
         (JDIMENSION) (compptr->v_samp_factor * DCTSIZE));
    }
  }
}

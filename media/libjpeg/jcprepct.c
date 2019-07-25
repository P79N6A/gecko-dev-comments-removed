















#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"







#ifdef INPUT_SMOOTHING_SUPPORTED
#define CONTEXT_ROWS_SUPPORTED
#endif























typedef struct {
  struct jpeg_c_prep_controller pub; 

  


  JSAMPARRAY color_buf[MAX_COMPONENTS];

  JDIMENSION rows_to_go;	
  int next_buf_row;		

#ifdef CONTEXT_ROWS_SUPPORTED	
  int this_row_group;		
  int next_buf_stop;		
#endif
} my_prep_controller;

typedef my_prep_controller * my_prep_ptr;






METHODDEF(void)
start_pass_prep (j_compress_ptr cinfo, J_BUF_MODE pass_mode)
{
  my_prep_ptr prep = (my_prep_ptr) cinfo->prep;

  if (pass_mode != JBUF_PASS_THRU)
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);

  
  prep->rows_to_go = cinfo->image_height;
  
  prep->next_buf_row = 0;
#ifdef CONTEXT_ROWS_SUPPORTED
  


  prep->this_row_group = 0;
  
  prep->next_buf_stop = 2 * cinfo->max_v_samp_factor;
#endif
}







LOCAL(void)
expand_bottom_edge (JSAMPARRAY image_data, JDIMENSION num_cols,
		    int input_rows, int output_rows)
{
  register int row;

  for (row = input_rows; row < output_rows; row++) {
    jcopy_sample_rows(image_data, input_rows-1, image_data, row,
		      1, num_cols);
  }
}











METHODDEF(void)
pre_process_data (j_compress_ptr cinfo,
		  JSAMPARRAY input_buf, JDIMENSION *in_row_ctr,
		  JDIMENSION in_rows_avail,
		  JSAMPIMAGE output_buf, JDIMENSION *out_row_group_ctr,
		  JDIMENSION out_row_groups_avail)
{
  my_prep_ptr prep = (my_prep_ptr) cinfo->prep;
  int numrows, ci;
  JDIMENSION inrows;
  jpeg_component_info * compptr;

  while (*in_row_ctr < in_rows_avail &&
	 *out_row_group_ctr < out_row_groups_avail) {
    
    inrows = in_rows_avail - *in_row_ctr;
    numrows = cinfo->max_v_samp_factor - prep->next_buf_row;
    numrows = (int) MIN((JDIMENSION) numrows, inrows);
    (*cinfo->cconvert->color_convert) (cinfo, input_buf + *in_row_ctr,
				       prep->color_buf,
				       (JDIMENSION) prep->next_buf_row,
				       numrows);
    *in_row_ctr += numrows;
    prep->next_buf_row += numrows;
    prep->rows_to_go -= numrows;
    
    if (prep->rows_to_go == 0 &&
	prep->next_buf_row < cinfo->max_v_samp_factor) {
      for (ci = 0; ci < cinfo->num_components; ci++) {
	expand_bottom_edge(prep->color_buf[ci], cinfo->image_width,
			   prep->next_buf_row, cinfo->max_v_samp_factor);
      }
      prep->next_buf_row = cinfo->max_v_samp_factor;
    }
    
    if (prep->next_buf_row == cinfo->max_v_samp_factor) {
      (*cinfo->downsample->downsample) (cinfo,
					prep->color_buf, (JDIMENSION) 0,
					output_buf, *out_row_group_ctr);
      prep->next_buf_row = 0;
      (*out_row_group_ctr)++;
    }
    


    if (prep->rows_to_go == 0 &&
	*out_row_group_ctr < out_row_groups_avail) {
      for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
	   ci++, compptr++) {
	expand_bottom_edge(output_buf[ci],
			   compptr->width_in_blocks * DCTSIZE,
			   (int) (*out_row_group_ctr * compptr->v_samp_factor),
			   (int) (out_row_groups_avail * compptr->v_samp_factor));
      }
      *out_row_group_ctr = out_row_groups_avail;
      break;			
    }
  }
}


#ifdef CONTEXT_ROWS_SUPPORTED





METHODDEF(void)
pre_process_context (j_compress_ptr cinfo,
		     JSAMPARRAY input_buf, JDIMENSION *in_row_ctr,
		     JDIMENSION in_rows_avail,
		     JSAMPIMAGE output_buf, JDIMENSION *out_row_group_ctr,
		     JDIMENSION out_row_groups_avail)
{
  my_prep_ptr prep = (my_prep_ptr) cinfo->prep;
  int numrows, ci;
  int buf_height = cinfo->max_v_samp_factor * 3;
  JDIMENSION inrows;

  while (*out_row_group_ctr < out_row_groups_avail) {
    if (*in_row_ctr < in_rows_avail) {
      
      inrows = in_rows_avail - *in_row_ctr;
      numrows = prep->next_buf_stop - prep->next_buf_row;
      numrows = (int) MIN((JDIMENSION) numrows, inrows);
      (*cinfo->cconvert->color_convert) (cinfo, input_buf + *in_row_ctr,
					 prep->color_buf,
					 (JDIMENSION) prep->next_buf_row,
					 numrows);
      
      if (prep->rows_to_go == cinfo->image_height) {
	for (ci = 0; ci < cinfo->num_components; ci++) {
	  int row;
	  for (row = 1; row <= cinfo->max_v_samp_factor; row++) {
	    jcopy_sample_rows(prep->color_buf[ci], 0,
			      prep->color_buf[ci], -row,
			      1, cinfo->image_width);
	  }
	}
      }
      *in_row_ctr += numrows;
      prep->next_buf_row += numrows;
      prep->rows_to_go -= numrows;
    } else {
      
      if (prep->rows_to_go != 0)
	break;
      
      if (prep->next_buf_row < prep->next_buf_stop) {
	for (ci = 0; ci < cinfo->num_components; ci++) {
	  expand_bottom_edge(prep->color_buf[ci], cinfo->image_width,
			     prep->next_buf_row, prep->next_buf_stop);
	}
	prep->next_buf_row = prep->next_buf_stop;
      }
    }
    
    if (prep->next_buf_row == prep->next_buf_stop) {
      (*cinfo->downsample->downsample) (cinfo,
					prep->color_buf,
					(JDIMENSION) prep->this_row_group,
					output_buf, *out_row_group_ctr);
      (*out_row_group_ctr)++;
      
      prep->this_row_group += cinfo->max_v_samp_factor;
      if (prep->this_row_group >= buf_height)
	prep->this_row_group = 0;
      if (prep->next_buf_row >= buf_height)
	prep->next_buf_row = 0;
      prep->next_buf_stop = prep->next_buf_row + cinfo->max_v_samp_factor;
    }
  }
}






LOCAL(void)
create_context_buffer (j_compress_ptr cinfo)
{
  my_prep_ptr prep = (my_prep_ptr) cinfo->prep;
  int rgroup_height = cinfo->max_v_samp_factor;
  int ci, i;
  jpeg_component_info * compptr;
  JSAMPARRAY true_buffer, fake_buffer;

  


  fake_buffer = (JSAMPARRAY)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(cinfo->num_components * 5 * rgroup_height) *
				SIZEOF(JSAMPROW));

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    



    true_buffer = (*cinfo->mem->alloc_sarray)
      ((j_common_ptr) cinfo, JPOOL_IMAGE,
       (JDIMENSION) (((long) compptr->width_in_blocks * DCTSIZE *
		      cinfo->max_h_samp_factor) / compptr->h_samp_factor),
       (JDIMENSION) (3 * rgroup_height));
    
    MEMCOPY(fake_buffer + rgroup_height, true_buffer,
	    3 * rgroup_height * SIZEOF(JSAMPROW));
    
    for (i = 0; i < rgroup_height; i++) {
      fake_buffer[i] = true_buffer[2 * rgroup_height + i];
      fake_buffer[4 * rgroup_height + i] = true_buffer[i];
    }
    prep->color_buf[ci] = fake_buffer + rgroup_height;
    fake_buffer += 5 * rgroup_height; 
  }
}

#endif 






GLOBAL(void)
jinit_c_prep_controller (j_compress_ptr cinfo, boolean need_full_buffer)
{
  my_prep_ptr prep;
  int ci;
  jpeg_component_info * compptr;

  if (need_full_buffer)		
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);

  prep = (my_prep_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_prep_controller));
  cinfo->prep = (struct jpeg_c_prep_controller *) prep;
  prep->pub.start_pass = start_pass_prep;

  



  if (cinfo->downsample->need_context_rows) {
    
#ifdef CONTEXT_ROWS_SUPPORTED
    prep->pub.pre_process_data = pre_process_context;
    create_context_buffer(cinfo);
#else
    ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
  } else {
    
    prep->pub.pre_process_data = pre_process_data;
    for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
	 ci++, compptr++) {
      prep->color_buf[ci] = (*cinfo->mem->alloc_sarray)
	((j_common_ptr) cinfo, JPOOL_IMAGE,
	 (JDIMENSION) (((long) compptr->width_in_blocks * DCTSIZE *
			cinfo->max_h_samp_factor) / compptr->h_samp_factor),
	 (JDIMENSION) cinfo->max_v_samp_factor);
    }
  }
}

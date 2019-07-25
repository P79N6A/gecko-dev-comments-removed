



































#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jsimd.h"

#ifdef UPSAMPLE_MERGING_SUPPORTED




typedef struct {
  struct jpeg_upsampler pub;	

  
  JMETHOD(void, upmethod, (j_decompress_ptr cinfo,
			   JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
			   JSAMPARRAY output_buf));

  
  int * Cr_r_tab;		
  int * Cb_b_tab;		
  INT32 * Cr_g_tab;		
  INT32 * Cb_g_tab;		

  




  JSAMPROW spare_row;
  boolean spare_full;		

  JDIMENSION out_row_width;	
  JDIMENSION rows_to_go;	
} my_upsampler;

typedef my_upsampler * my_upsample_ptr;

#define SCALEBITS	16	/* speediest right-shift on some machines */
#define ONE_HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))







LOCAL(void)
build_ycc_rgb_table (j_decompress_ptr cinfo)
{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;
  int i;
  INT32 x;
  SHIFT_TEMPS

  upsample->Cr_r_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  upsample->Cb_b_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  upsample->Cr_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));
  upsample->Cb_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));

  for (i = 0, x = -CENTERJSAMPLE; i <= MAXJSAMPLE; i++, x++) {
    
    
    
    upsample->Cr_r_tab[i] = (int)
		    RIGHT_SHIFT(FIX(1.40200) * x + ONE_HALF, SCALEBITS);
    
    upsample->Cb_b_tab[i] = (int)
		    RIGHT_SHIFT(FIX(1.77200) * x + ONE_HALF, SCALEBITS);
    
    upsample->Cr_g_tab[i] = (- FIX(0.71414)) * x;
    
    
    upsample->Cb_g_tab[i] = (- FIX(0.34414)) * x + ONE_HALF;
  }
}






METHODDEF(void)
start_pass_merged_upsample (j_decompress_ptr cinfo)
{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;

  
  upsample->spare_full = FALSE;
  
  upsample->rows_to_go = cinfo->output_height;
}








METHODDEF(void)
merged_2v_upsample (j_decompress_ptr cinfo,
		    JSAMPIMAGE input_buf, JDIMENSION *in_row_group_ctr,
		    JDIMENSION in_row_groups_avail,
		    JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
		    JDIMENSION out_rows_avail)

{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;
  JSAMPROW work_ptrs[2];
  JDIMENSION num_rows;		

  if (upsample->spare_full) {
    
    jcopy_sample_rows(& upsample->spare_row, 0, output_buf + *out_row_ctr, 0,
		      1, upsample->out_row_width);
    num_rows = 1;
    upsample->spare_full = FALSE;
  } else {
    
    num_rows = 2;
    
    if (num_rows > upsample->rows_to_go)
      num_rows = upsample->rows_to_go;
    
    out_rows_avail -= *out_row_ctr;
    if (num_rows > out_rows_avail)
      num_rows = out_rows_avail;
    
    work_ptrs[0] = output_buf[*out_row_ctr];
    if (num_rows > 1) {
      work_ptrs[1] = output_buf[*out_row_ctr + 1];
    } else {
      work_ptrs[1] = upsample->spare_row;
      upsample->spare_full = TRUE;
    }
    
    (*upsample->upmethod) (cinfo, input_buf, *in_row_group_ctr, work_ptrs);
  }

  
  *out_row_ctr += num_rows;
  upsample->rows_to_go -= num_rows;
  
  if (! upsample->spare_full)
    (*in_row_group_ctr)++;
}


METHODDEF(void)
merged_1v_upsample (j_decompress_ptr cinfo,
		    JSAMPIMAGE input_buf, JDIMENSION *in_row_group_ctr,
		    JDIMENSION in_row_groups_avail,
		    JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
		    JDIMENSION out_rows_avail)

{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;

  
  (*upsample->upmethod) (cinfo, input_buf, *in_row_group_ctr,
			 output_buf + *out_row_ctr);
  
  (*out_row_ctr)++;
  (*in_row_group_ctr)++;
}
















METHODDEF(void)
h2v1_merged_upsample (j_decompress_ptr cinfo,
		      JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
		      JSAMPARRAY output_buf)
{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;
  register int y, cred, cgreen, cblue;
  int cb, cr;
  register JSAMPROW outptr;
  JSAMPROW inptr0, inptr1, inptr2;
  JDIMENSION col;
  
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  int * Crrtab = upsample->Cr_r_tab;
  int * Cbbtab = upsample->Cb_b_tab;
  INT32 * Crgtab = upsample->Cr_g_tab;
  INT32 * Cbgtab = upsample->Cb_g_tab;
  SHIFT_TEMPS

  inptr0 = input_buf[0][in_row_group_ctr];
  inptr1 = input_buf[1][in_row_group_ctr];
  inptr2 = input_buf[2][in_row_group_ctr];
  outptr = output_buf[0];
  
  for (col = cinfo->output_width >> 1; col > 0; col--) {
    
    cb = GETJSAMPLE(*inptr1++);
    cr = GETJSAMPLE(*inptr2++);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    
    y  = GETJSAMPLE(*inptr0++);
    outptr[rgb_red[cinfo->out_color_space]] =   range_limit[y + cred];
    outptr[rgb_green[cinfo->out_color_space]] = range_limit[y + cgreen];
    outptr[rgb_blue[cinfo->out_color_space]] =  range_limit[y + cblue];
    outptr += rgb_pixelsize[cinfo->out_color_space];
    y  = GETJSAMPLE(*inptr0++);
    outptr[rgb_red[cinfo->out_color_space]] =   range_limit[y + cred];
    outptr[rgb_green[cinfo->out_color_space]] = range_limit[y + cgreen];
    outptr[rgb_blue[cinfo->out_color_space]] =  range_limit[y + cblue];
    outptr += rgb_pixelsize[cinfo->out_color_space];
  }
  
  if (cinfo->output_width & 1) {
    cb = GETJSAMPLE(*inptr1);
    cr = GETJSAMPLE(*inptr2);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    y  = GETJSAMPLE(*inptr0);
    outptr[rgb_red[cinfo->out_color_space]] =   range_limit[y + cred];
    outptr[rgb_green[cinfo->out_color_space]] = range_limit[y + cgreen];
    outptr[rgb_blue[cinfo->out_color_space]] =  range_limit[y + cblue];
  }
}






METHODDEF(void)
h2v2_merged_upsample (j_decompress_ptr cinfo,
		      JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
		      JSAMPARRAY output_buf)
{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;
  register int y, cred, cgreen, cblue;
  int cb, cr;
  register JSAMPROW outptr0, outptr1;
  JSAMPROW inptr00, inptr01, inptr1, inptr2;
  JDIMENSION col;
  
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  int * Crrtab = upsample->Cr_r_tab;
  int * Cbbtab = upsample->Cb_b_tab;
  INT32 * Crgtab = upsample->Cr_g_tab;
  INT32 * Cbgtab = upsample->Cb_g_tab;
  SHIFT_TEMPS

  inptr00 = input_buf[0][in_row_group_ctr*2];
  inptr01 = input_buf[0][in_row_group_ctr*2 + 1];
  inptr1 = input_buf[1][in_row_group_ctr];
  inptr2 = input_buf[2][in_row_group_ctr];
  outptr0 = output_buf[0];
  outptr1 = output_buf[1];
  
  for (col = cinfo->output_width >> 1; col > 0; col--) {
    
    cb = GETJSAMPLE(*inptr1++);
    cr = GETJSAMPLE(*inptr2++);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    
    y  = GETJSAMPLE(*inptr00++);
    outptr0[rgb_red[cinfo->out_color_space]] =   range_limit[y + cred];
    outptr0[rgb_green[cinfo->out_color_space]] = range_limit[y + cgreen];
    outptr0[rgb_blue[cinfo->out_color_space]] =  range_limit[y + cblue];
    outptr0 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr00++);
    outptr0[rgb_red[cinfo->out_color_space]] =   range_limit[y + cred];
    outptr0[rgb_green[cinfo->out_color_space]] = range_limit[y + cgreen];
    outptr0[rgb_blue[cinfo->out_color_space]] =  range_limit[y + cblue];
    outptr0 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr01++);
    outptr1[rgb_red[cinfo->out_color_space]] =   range_limit[y + cred];
    outptr1[rgb_green[cinfo->out_color_space]] = range_limit[y + cgreen];
    outptr1[rgb_blue[cinfo->out_color_space]] =  range_limit[y + cblue];
    outptr1 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr01++);
    outptr1[rgb_red[cinfo->out_color_space]] =   range_limit[y + cred];
    outptr1[rgb_green[cinfo->out_color_space]] = range_limit[y + cgreen];
    outptr1[rgb_blue[cinfo->out_color_space]] =  range_limit[y + cblue];
    outptr1 += RGB_PIXELSIZE;
  }
  
  if (cinfo->output_width & 1) {
    cb = GETJSAMPLE(*inptr1);
    cr = GETJSAMPLE(*inptr2);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    y  = GETJSAMPLE(*inptr00);
    outptr0[rgb_red[cinfo->out_color_space]] =   range_limit[y + cred];
    outptr0[rgb_green[cinfo->out_color_space]] = range_limit[y + cgreen];
    outptr0[rgb_blue[cinfo->out_color_space]] =  range_limit[y + cblue];
    y  = GETJSAMPLE(*inptr01);
    outptr1[rgb_red[cinfo->out_color_space]] =   range_limit[y + cred];
    outptr1[rgb_green[cinfo->out_color_space]] = range_limit[y + cgreen];
    outptr1[rgb_blue[cinfo->out_color_space]] =  range_limit[y + cblue];
  }
}










GLOBAL(void)
jinit_merged_upsampler (j_decompress_ptr cinfo)
{
  my_upsample_ptr upsample;

  upsample = (my_upsample_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_upsampler));
  cinfo->upsample = (struct jpeg_upsampler *) upsample;
  upsample->pub.start_pass = start_pass_merged_upsample;
  upsample->pub.need_context_rows = FALSE;

  upsample->out_row_width = cinfo->output_width * cinfo->out_color_components;

  if (cinfo->max_v_samp_factor == 2) {
    upsample->pub.upsample = merged_2v_upsample;
    if (jsimd_can_h2v2_merged_upsample())
      upsample->upmethod = jsimd_h2v2_merged_upsample;
    else
      upsample->upmethod = h2v2_merged_upsample;
    
    upsample->spare_row = (JSAMPROW)
      (*cinfo->mem->alloc_large) ((j_common_ptr) cinfo, JPOOL_IMAGE,
		(size_t) (upsample->out_row_width * SIZEOF(JSAMPLE)));
  } else {
    upsample->pub.upsample = merged_1v_upsample;
    if (jsimd_can_h2v1_merged_upsample())
      upsample->upmethod = jsimd_h2v1_merged_upsample;
    else
      upsample->upmethod = h2v1_merged_upsample;
    
    upsample->spare_row = NULL;
  }

  build_ycc_rgb_table(cinfo);
}

#endif 





































#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jsimd.h"
#include "config.h"

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




#include "jdmrgext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE

#define RGB_RED EXT_RGB_RED
#define RGB_GREEN EXT_RGB_GREEN
#define RGB_BLUE EXT_RGB_BLUE
#define RGB_PIXELSIZE EXT_RGB_PIXELSIZE
#define h2v1_merged_upsample_internal extrgb_h2v1_merged_upsample_internal
#define h2v2_merged_upsample_internal extrgb_h2v2_merged_upsample_internal
#include "jdmrgext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef h2v1_merged_upsample_internal
#undef h2v2_merged_upsample_internal

#define RGB_RED EXT_RGBX_RED
#define RGB_GREEN EXT_RGBX_GREEN
#define RGB_BLUE EXT_RGBX_BLUE
#define RGB_PIXELSIZE EXT_RGBX_PIXELSIZE
#define h2v1_merged_upsample_internal extrgbx_h2v1_merged_upsample_internal
#define h2v2_merged_upsample_internal extrgbx_h2v2_merged_upsample_internal
#include "jdmrgext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef h2v1_merged_upsample_internal
#undef h2v2_merged_upsample_internal

#define RGB_RED EXT_BGR_RED
#define RGB_GREEN EXT_BGR_GREEN
#define RGB_BLUE EXT_BGR_BLUE
#define RGB_PIXELSIZE EXT_BGR_PIXELSIZE
#define h2v1_merged_upsample_internal extbgr_h2v1_merged_upsample_internal
#define h2v2_merged_upsample_internal extbgr_h2v2_merged_upsample_internal
#include "jdmrgext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef h2v1_merged_upsample_internal
#undef h2v2_merged_upsample_internal

#define RGB_RED EXT_BGRX_RED
#define RGB_GREEN EXT_BGRX_GREEN
#define RGB_BLUE EXT_BGRX_BLUE
#define RGB_PIXELSIZE EXT_BGRX_PIXELSIZE
#define h2v1_merged_upsample_internal extbgrx_h2v1_merged_upsample_internal
#define h2v2_merged_upsample_internal extbgrx_h2v2_merged_upsample_internal
#include "jdmrgext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef h2v1_merged_upsample_internal
#undef h2v2_merged_upsample_internal

#define RGB_RED EXT_XBGR_RED
#define RGB_GREEN EXT_XBGR_GREEN
#define RGB_BLUE EXT_XBGR_BLUE
#define RGB_PIXELSIZE EXT_XBGR_PIXELSIZE
#define h2v1_merged_upsample_internal extxbgr_h2v1_merged_upsample_internal
#define h2v2_merged_upsample_internal extxbgr_h2v2_merged_upsample_internal
#include "jdmrgext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef h2v1_merged_upsample_internal
#undef h2v2_merged_upsample_internal

#define RGB_RED EXT_XRGB_RED
#define RGB_GREEN EXT_XRGB_GREEN
#define RGB_BLUE EXT_XRGB_BLUE
#define RGB_PIXELSIZE EXT_XRGB_PIXELSIZE
#define h2v1_merged_upsample_internal extxrgb_h2v1_merged_upsample_internal
#define h2v2_merged_upsample_internal extxrgb_h2v2_merged_upsample_internal
#include "jdmrgext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef h2v1_merged_upsample_internal
#undef h2v2_merged_upsample_internal







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
  switch (cinfo->out_color_space) {
    case JCS_EXT_RGB:
      extrgb_h2v1_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                           output_buf);
      break;
    case JCS_EXT_RGBX:
    case JCS_EXT_RGBA:
      extrgbx_h2v1_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                            output_buf);
      break;
    case JCS_EXT_BGR:
      extbgr_h2v1_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                           output_buf);
      break;
    case JCS_EXT_BGRX:
    case JCS_EXT_BGRA:
      extbgrx_h2v1_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                            output_buf);
      break;
    case JCS_EXT_XBGR:
    case JCS_EXT_ABGR:
      extxbgr_h2v1_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                            output_buf);
      break;
    case JCS_EXT_XRGB:
    case JCS_EXT_ARGB:
      extxrgb_h2v1_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                            output_buf);
      break;
    default:
      h2v1_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                    output_buf);
      break;
  }
}






METHODDEF(void)
h2v2_merged_upsample (j_decompress_ptr cinfo,
		      JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
		      JSAMPARRAY output_buf)
{
  switch (cinfo->out_color_space) {
    case JCS_EXT_RGB:
      extrgb_h2v2_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                           output_buf);
      break;
    case JCS_EXT_RGBX:
    case JCS_EXT_RGBA:
      extrgbx_h2v2_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                            output_buf);
      break;
    case JCS_EXT_BGR:
      extbgr_h2v2_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                           output_buf);
      break;
    case JCS_EXT_BGRX:
    case JCS_EXT_BGRA:
      extbgrx_h2v2_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                            output_buf);
      break;
    case JCS_EXT_XBGR:
    case JCS_EXT_ABGR:
      extxbgr_h2v2_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                            output_buf);
      break;
    case JCS_EXT_XRGB:
    case JCS_EXT_ARGB:
      extxrgb_h2v2_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                            output_buf);
      break;
    default:
      h2v2_merged_upsample_internal(cinfo, input_buf, in_row_group_ctr,
                                    output_buf);
      break;
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

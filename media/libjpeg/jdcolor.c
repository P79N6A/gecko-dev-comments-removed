











#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jsimd.h"
#include "config.h"




typedef struct {
  struct jpeg_color_deconverter pub; 

  
  int * Cr_r_tab;		
  int * Cb_b_tab;		
  INT32 * Cr_g_tab;		
  INT32 * Cb_g_tab;		
} my_color_deconverter;

typedef my_color_deconverter * my_cconvert_ptr;































#define SCALEBITS	16	/* speediest right-shift on some machines */
#define ONE_HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))




#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE

#define RGB_RED EXT_RGB_RED
#define RGB_GREEN EXT_RGB_GREEN
#define RGB_BLUE EXT_RGB_BLUE
#define RGB_PIXELSIZE EXT_RGB_PIXELSIZE
#define ycc_rgb_convert_internal ycc_extrgb_convert_internal
#define gray_rgb_convert_internal gray_extrgb_convert_internal
#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef ycc_rgb_convert_internal
#undef gray_rgb_convert_internal

#define RGB_RED EXT_RGBX_RED
#define RGB_GREEN EXT_RGBX_GREEN
#define RGB_BLUE EXT_RGBX_BLUE
#define RGB_ALPHA 3
#define RGB_PIXELSIZE EXT_RGBX_PIXELSIZE
#define ycc_rgb_convert_internal ycc_extrgbx_convert_internal
#define gray_rgb_convert_internal gray_extrgbx_convert_internal
#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef ycc_rgb_convert_internal
#undef gray_rgb_convert_internal

#define RGB_RED EXT_BGR_RED
#define RGB_GREEN EXT_BGR_GREEN
#define RGB_BLUE EXT_BGR_BLUE
#define RGB_PIXELSIZE EXT_BGR_PIXELSIZE
#define ycc_rgb_convert_internal ycc_extbgr_convert_internal
#define gray_rgb_convert_internal gray_extbgr_convert_internal
#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef ycc_rgb_convert_internal
#undef gray_rgb_convert_internal

#define RGB_RED EXT_BGRX_RED
#define RGB_GREEN EXT_BGRX_GREEN
#define RGB_BLUE EXT_BGRX_BLUE
#define RGB_ALPHA 3
#define RGB_PIXELSIZE EXT_BGRX_PIXELSIZE
#define ycc_rgb_convert_internal ycc_extbgrx_convert_internal
#define gray_rgb_convert_internal gray_extbgrx_convert_internal
#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef ycc_rgb_convert_internal
#undef gray_rgb_convert_internal

#define RGB_RED EXT_XBGR_RED
#define RGB_GREEN EXT_XBGR_GREEN
#define RGB_BLUE EXT_XBGR_BLUE
#define RGB_ALPHA 0
#define RGB_PIXELSIZE EXT_XBGR_PIXELSIZE
#define ycc_rgb_convert_internal ycc_extxbgr_convert_internal
#define gray_rgb_convert_internal gray_extxbgr_convert_internal
#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef ycc_rgb_convert_internal
#undef gray_rgb_convert_internal

#define RGB_RED EXT_XRGB_RED
#define RGB_GREEN EXT_XRGB_GREEN
#define RGB_BLUE EXT_XRGB_BLUE
#define RGB_ALPHA 0
#define RGB_PIXELSIZE EXT_XRGB_PIXELSIZE
#define ycc_rgb_convert_internal ycc_extxrgb_convert_internal
#define gray_rgb_convert_internal gray_extxrgb_convert_internal
#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef ycc_rgb_convert_internal
#undef gray_rgb_convert_internal






LOCAL(void)
build_ycc_rgb_table (j_decompress_ptr cinfo)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  int i;
  INT32 x;
  SHIFT_TEMPS

  cconvert->Cr_r_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  cconvert->Cb_b_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  cconvert->Cr_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));
  cconvert->Cb_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));

  for (i = 0, x = -CENTERJSAMPLE; i <= MAXJSAMPLE; i++, x++) {
    
    
    
    cconvert->Cr_r_tab[i] = (int)
		    RIGHT_SHIFT(FIX(1.40200) * x + ONE_HALF, SCALEBITS);
    
    cconvert->Cb_b_tab[i] = (int)
		    RIGHT_SHIFT(FIX(1.77200) * x + ONE_HALF, SCALEBITS);
    
    cconvert->Cr_g_tab[i] = (- FIX(0.71414)) * x;
    
    
    cconvert->Cb_g_tab[i] = (- FIX(0.34414)) * x + ONE_HALF;
  }
}






METHODDEF(void)
ycc_rgb_convert (j_decompress_ptr cinfo,
		 JSAMPIMAGE input_buf, JDIMENSION input_row,
		 JSAMPARRAY output_buf, int num_rows)
{
  switch (cinfo->out_color_space) {
    case JCS_EXT_RGB:
      ycc_extrgb_convert_internal(cinfo, input_buf, input_row, output_buf,
                                  num_rows);
      break;
    case JCS_EXT_RGBX:
    case JCS_EXT_RGBA:
      ycc_extrgbx_convert_internal(cinfo, input_buf, input_row, output_buf,
                                   num_rows);
      break;
    case JCS_EXT_BGR:
      ycc_extbgr_convert_internal(cinfo, input_buf, input_row, output_buf,
                                  num_rows);
      break;
    case JCS_EXT_BGRX:
    case JCS_EXT_BGRA:
      ycc_extbgrx_convert_internal(cinfo, input_buf, input_row, output_buf,
                                   num_rows);
      break;
    case JCS_EXT_XBGR:
    case JCS_EXT_ABGR:
      ycc_extxbgr_convert_internal(cinfo, input_buf, input_row, output_buf,
                                   num_rows);
      break;
    case JCS_EXT_XRGB:
    case JCS_EXT_ARGB:
      ycc_extxrgb_convert_internal(cinfo, input_buf, input_row, output_buf,
                                   num_rows);
      break;
    default:
      ycc_rgb_convert_internal(cinfo, input_buf, input_row, output_buf,
                               num_rows);
      break;
  }
}










METHODDEF(void)
null_convert (j_decompress_ptr cinfo,
	      JSAMPIMAGE input_buf, JDIMENSION input_row,
	      JSAMPARRAY output_buf, int num_rows)
{
  register JSAMPROW inptr, outptr;
  register JDIMENSION count;
  register int num_components = cinfo->num_components;
  JDIMENSION num_cols = cinfo->output_width;
  int ci;

  while (--num_rows >= 0) {
    for (ci = 0; ci < num_components; ci++) {
      inptr = input_buf[ci][input_row];
      outptr = output_buf[0] + ci;
      for (count = num_cols; count > 0; count--) {
	*outptr = *inptr++;	
	outptr += num_components;
      }
    }
    input_row++;
    output_buf++;
  }
}








METHODDEF(void)
grayscale_convert (j_decompress_ptr cinfo,
		   JSAMPIMAGE input_buf, JDIMENSION input_row,
		   JSAMPARRAY output_buf, int num_rows)
{
  jcopy_sample_rows(input_buf[0], (int) input_row, output_buf, 0,
		    num_rows, cinfo->output_width);
}






METHODDEF(void)
gray_rgb_convert (j_decompress_ptr cinfo,
		  JSAMPIMAGE input_buf, JDIMENSION input_row,
		  JSAMPARRAY output_buf, int num_rows)
{
  switch (cinfo->out_color_space) {
    case JCS_EXT_RGB:
      gray_extrgb_convert_internal(cinfo, input_buf, input_row, output_buf,
                                   num_rows);
      break;
    case JCS_EXT_RGBX:
    case JCS_EXT_RGBA:
      gray_extrgbx_convert_internal(cinfo, input_buf, input_row, output_buf,
                                    num_rows);
      break;
    case JCS_EXT_BGR:
      gray_extbgr_convert_internal(cinfo, input_buf, input_row, output_buf,
                                   num_rows);
      break;
    case JCS_EXT_BGRX:
    case JCS_EXT_BGRA:
      gray_extbgrx_convert_internal(cinfo, input_buf, input_row, output_buf,
                                    num_rows);
      break;
    case JCS_EXT_XBGR:
    case JCS_EXT_ABGR:
      gray_extxbgr_convert_internal(cinfo, input_buf, input_row, output_buf,
                                    num_rows);
      break;
    case JCS_EXT_XRGB:
    case JCS_EXT_ARGB:
      gray_extxrgb_convert_internal(cinfo, input_buf, input_row, output_buf,
                                    num_rows);
      break;
    default:
      gray_rgb_convert_internal(cinfo, input_buf, input_row, output_buf,
                                num_rows);
      break;
  }
}









METHODDEF(void)
ycck_cmyk_convert (j_decompress_ptr cinfo,
		   JSAMPIMAGE input_buf, JDIMENSION input_row,
		   JSAMPARRAY output_buf, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int y, cb, cr;
  register JSAMPROW outptr;
  register JSAMPROW inptr0, inptr1, inptr2, inptr3;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;
  
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  register int * Crrtab = cconvert->Cr_r_tab;
  register int * Cbbtab = cconvert->Cb_b_tab;
  register INT32 * Crgtab = cconvert->Cr_g_tab;
  register INT32 * Cbgtab = cconvert->Cb_g_tab;
  SHIFT_TEMPS

  while (--num_rows >= 0) {
    inptr0 = input_buf[0][input_row];
    inptr1 = input_buf[1][input_row];
    inptr2 = input_buf[2][input_row];
    inptr3 = input_buf[3][input_row];
    input_row++;
    outptr = *output_buf++;
    for (col = 0; col < num_cols; col++) {
      y  = GETJSAMPLE(inptr0[col]);
      cb = GETJSAMPLE(inptr1[col]);
      cr = GETJSAMPLE(inptr2[col]);
      
      outptr[0] = range_limit[MAXJSAMPLE - (y + Crrtab[cr])];	
      outptr[1] = range_limit[MAXJSAMPLE - (y +			
			      ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
						 SCALEBITS)))];
      outptr[2] = range_limit[MAXJSAMPLE - (y + Cbbtab[cb])];	
      
      outptr[3] = inptr3[col];	
      outptr += 4;
    }
  }
}






METHODDEF(void)
start_pass_dcolor (j_decompress_ptr cinfo)
{
  
}






GLOBAL(void)
jinit_color_deconverter (j_decompress_ptr cinfo)
{
  my_cconvert_ptr cconvert;
  int ci;

  cconvert = (my_cconvert_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_color_deconverter));
  cinfo->cconvert = (struct jpeg_color_deconverter *) cconvert;
  cconvert->pub.start_pass = start_pass_dcolor;

  
  switch (cinfo->jpeg_color_space) {
  case JCS_GRAYSCALE:
    if (cinfo->num_components != 1)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;

  case JCS_RGB:
  case JCS_YCbCr:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;

  case JCS_CMYK:
  case JCS_YCCK:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;

  default:			
    if (cinfo->num_components < 1)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;
  }

  




  switch (cinfo->out_color_space) {
  case JCS_GRAYSCALE:
    cinfo->out_color_components = 1;
    if (cinfo->jpeg_color_space == JCS_GRAYSCALE ||
	cinfo->jpeg_color_space == JCS_YCbCr) {
      cconvert->pub.color_convert = grayscale_convert;
      
      for (ci = 1; ci < cinfo->num_components; ci++)
	cinfo->comp_info[ci].component_needed = FALSE;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_RGB:
  case JCS_EXT_RGB:
  case JCS_EXT_RGBX:
  case JCS_EXT_BGR:
  case JCS_EXT_BGRX:
  case JCS_EXT_XBGR:
  case JCS_EXT_XRGB:
  case JCS_EXT_RGBA:
  case JCS_EXT_BGRA:
  case JCS_EXT_ABGR:
  case JCS_EXT_ARGB:
    cinfo->out_color_components = rgb_pixelsize[cinfo->out_color_space];
    if (cinfo->jpeg_color_space == JCS_YCbCr) {
      if (jsimd_can_ycc_rgb())
        cconvert->pub.color_convert = jsimd_ycc_rgb_convert;
      else {
        cconvert->pub.color_convert = ycc_rgb_convert;
        build_ycc_rgb_table(cinfo);
      }
    } else if (cinfo->jpeg_color_space == JCS_GRAYSCALE) {
      cconvert->pub.color_convert = gray_rgb_convert;
    } else if (cinfo->jpeg_color_space == cinfo->out_color_space &&
      rgb_pixelsize[cinfo->out_color_space] == 3) {
      cconvert->pub.color_convert = null_convert;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_CMYK:
    cinfo->out_color_components = 4;
    if (cinfo->jpeg_color_space == JCS_YCCK) {
      cconvert->pub.color_convert = ycck_cmyk_convert;
      build_ycc_rgb_table(cinfo);
    } else if (cinfo->jpeg_color_space == JCS_CMYK) {
      cconvert->pub.color_convert = null_convert;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  default:
    
    if (cinfo->out_color_space == cinfo->jpeg_color_space) {
      cinfo->out_color_components = cinfo->num_components;
      cconvert->pub.color_convert = null_convert;
    } else			
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;
  }

  if (cinfo->quantize_colors)
    cinfo->output_components = 1; 
  else
    cinfo->output_components = cinfo->out_color_components;
}

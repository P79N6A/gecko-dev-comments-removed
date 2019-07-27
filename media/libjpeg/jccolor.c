













#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jsimd.h"
#include "jconfigint.h"




typedef struct {
  struct jpeg_color_converter pub; 

  
  INT32 * rgb_ycc_tab;          
} my_color_converter;

typedef my_color_converter * my_cconvert_ptr;
































#define SCALEBITS       16      /* speediest right-shift on some machines */
#define CBCR_OFFSET     ((INT32) CENTERJSAMPLE << SCALEBITS)
#define ONE_HALF        ((INT32) 1 << (SCALEBITS-1))
#define FIX(x)          ((INT32) ((x) * (1L<<SCALEBITS) + 0.5))







#define R_Y_OFF         0                       /* offset to R => Y section */
#define G_Y_OFF         (1*(MAXJSAMPLE+1))      /* offset to G => Y section */
#define B_Y_OFF         (2*(MAXJSAMPLE+1))      /* etc. */
#define R_CB_OFF        (3*(MAXJSAMPLE+1))
#define G_CB_OFF        (4*(MAXJSAMPLE+1))
#define B_CB_OFF        (5*(MAXJSAMPLE+1))
#define R_CR_OFF        B_CB_OFF                /* B=>Cb, R=>Cr are the same */
#define G_CR_OFF        (6*(MAXJSAMPLE+1))
#define B_CR_OFF        (7*(MAXJSAMPLE+1))
#define TABLE_SIZE      (8*(MAXJSAMPLE+1))




#include "jccolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE

#define RGB_RED EXT_RGB_RED
#define RGB_GREEN EXT_RGB_GREEN
#define RGB_BLUE EXT_RGB_BLUE
#define RGB_PIXELSIZE EXT_RGB_PIXELSIZE
#define rgb_ycc_convert_internal extrgb_ycc_convert_internal
#define rgb_gray_convert_internal extrgb_gray_convert_internal
#define rgb_rgb_convert_internal extrgb_rgb_convert_internal
#include "jccolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef rgb_ycc_convert_internal
#undef rgb_gray_convert_internal
#undef rgb_rgb_convert_internal

#define RGB_RED EXT_RGBX_RED
#define RGB_GREEN EXT_RGBX_GREEN
#define RGB_BLUE EXT_RGBX_BLUE
#define RGB_PIXELSIZE EXT_RGBX_PIXELSIZE
#define rgb_ycc_convert_internal extrgbx_ycc_convert_internal
#define rgb_gray_convert_internal extrgbx_gray_convert_internal
#define rgb_rgb_convert_internal extrgbx_rgb_convert_internal
#include "jccolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef rgb_ycc_convert_internal
#undef rgb_gray_convert_internal
#undef rgb_rgb_convert_internal

#define RGB_RED EXT_BGR_RED
#define RGB_GREEN EXT_BGR_GREEN
#define RGB_BLUE EXT_BGR_BLUE
#define RGB_PIXELSIZE EXT_BGR_PIXELSIZE
#define rgb_ycc_convert_internal extbgr_ycc_convert_internal
#define rgb_gray_convert_internal extbgr_gray_convert_internal
#define rgb_rgb_convert_internal extbgr_rgb_convert_internal
#include "jccolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef rgb_ycc_convert_internal
#undef rgb_gray_convert_internal
#undef rgb_rgb_convert_internal

#define RGB_RED EXT_BGRX_RED
#define RGB_GREEN EXT_BGRX_GREEN
#define RGB_BLUE EXT_BGRX_BLUE
#define RGB_PIXELSIZE EXT_BGRX_PIXELSIZE
#define rgb_ycc_convert_internal extbgrx_ycc_convert_internal
#define rgb_gray_convert_internal extbgrx_gray_convert_internal
#define rgb_rgb_convert_internal extbgrx_rgb_convert_internal
#include "jccolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef rgb_ycc_convert_internal
#undef rgb_gray_convert_internal
#undef rgb_rgb_convert_internal

#define RGB_RED EXT_XBGR_RED
#define RGB_GREEN EXT_XBGR_GREEN
#define RGB_BLUE EXT_XBGR_BLUE
#define RGB_PIXELSIZE EXT_XBGR_PIXELSIZE
#define rgb_ycc_convert_internal extxbgr_ycc_convert_internal
#define rgb_gray_convert_internal extxbgr_gray_convert_internal
#define rgb_rgb_convert_internal extxbgr_rgb_convert_internal
#include "jccolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef rgb_ycc_convert_internal
#undef rgb_gray_convert_internal
#undef rgb_rgb_convert_internal

#define RGB_RED EXT_XRGB_RED
#define RGB_GREEN EXT_XRGB_GREEN
#define RGB_BLUE EXT_XRGB_BLUE
#define RGB_PIXELSIZE EXT_XRGB_PIXELSIZE
#define rgb_ycc_convert_internal extxrgb_ycc_convert_internal
#define rgb_gray_convert_internal extxrgb_gray_convert_internal
#define rgb_rgb_convert_internal extxrgb_rgb_convert_internal
#include "jccolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef rgb_ycc_convert_internal
#undef rgb_gray_convert_internal
#undef rgb_rgb_convert_internal






METHODDEF(void)
rgb_ycc_start (j_compress_ptr cinfo)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  INT32 * rgb_ycc_tab;
  INT32 i;

  
  cconvert->rgb_ycc_tab = rgb_ycc_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                (TABLE_SIZE * sizeof(INT32)));

  for (i = 0; i <= MAXJSAMPLE; i++) {
    rgb_ycc_tab[i+R_Y_OFF] = FIX(0.29900) * i;
    rgb_ycc_tab[i+G_Y_OFF] = FIX(0.58700) * i;
    rgb_ycc_tab[i+B_Y_OFF] = FIX(0.11400) * i     + ONE_HALF;
    rgb_ycc_tab[i+R_CB_OFF] = (-FIX(0.16874)) * i;
    rgb_ycc_tab[i+G_CB_OFF] = (-FIX(0.33126)) * i;
    



    rgb_ycc_tab[i+B_CB_OFF] = FIX(0.50000) * i    + CBCR_OFFSET + ONE_HALF-1;



    rgb_ycc_tab[i+G_CR_OFF] = (-FIX(0.41869)) * i;
    rgb_ycc_tab[i+B_CR_OFF] = (-FIX(0.08131)) * i;
  }
}






METHODDEF(void)
rgb_ycc_convert (j_compress_ptr cinfo,
                 JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
                 JDIMENSION output_row, int num_rows)
{
  switch (cinfo->in_color_space) {
    case JCS_EXT_RGB:
      extrgb_ycc_convert_internal(cinfo, input_buf, output_buf, output_row,
                                  num_rows);
      break;
    case JCS_EXT_RGBX:
    case JCS_EXT_RGBA:
      extrgbx_ycc_convert_internal(cinfo, input_buf, output_buf, output_row,
                                   num_rows);
      break;
    case JCS_EXT_BGR:
      extbgr_ycc_convert_internal(cinfo, input_buf, output_buf, output_row,
                                  num_rows);
      break;
    case JCS_EXT_BGRX:
    case JCS_EXT_BGRA:
      extbgrx_ycc_convert_internal(cinfo, input_buf, output_buf, output_row,
                                   num_rows);
      break;
    case JCS_EXT_XBGR:
    case JCS_EXT_ABGR:
      extxbgr_ycc_convert_internal(cinfo, input_buf, output_buf, output_row,
                                   num_rows);
      break;
    case JCS_EXT_XRGB:
    case JCS_EXT_ARGB:
      extxrgb_ycc_convert_internal(cinfo, input_buf, output_buf, output_row,
                                   num_rows);
      break;
    default:
      rgb_ycc_convert_internal(cinfo, input_buf, output_buf, output_row,
                               num_rows);
      break;
  }
}









METHODDEF(void)
rgb_gray_convert (j_compress_ptr cinfo,
                  JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
                  JDIMENSION output_row, int num_rows)
{
  switch (cinfo->in_color_space) {
    case JCS_EXT_RGB:
      extrgb_gray_convert_internal(cinfo, input_buf, output_buf, output_row,
                                   num_rows);
      break;
    case JCS_EXT_RGBX:
    case JCS_EXT_RGBA:
      extrgbx_gray_convert_internal(cinfo, input_buf, output_buf, output_row,
                                    num_rows);
      break;
    case JCS_EXT_BGR:
      extbgr_gray_convert_internal(cinfo, input_buf, output_buf, output_row,
                                   num_rows);
      break;
    case JCS_EXT_BGRX:
    case JCS_EXT_BGRA:
      extbgrx_gray_convert_internal(cinfo, input_buf, output_buf, output_row,
                                    num_rows);
      break;
    case JCS_EXT_XBGR:
    case JCS_EXT_ABGR:
      extxbgr_gray_convert_internal(cinfo, input_buf, output_buf, output_row,
                                    num_rows);
      break;
    case JCS_EXT_XRGB:
    case JCS_EXT_ARGB:
      extxrgb_gray_convert_internal(cinfo, input_buf, output_buf, output_row,
                                    num_rows);
      break;
    default:
      rgb_gray_convert_internal(cinfo, input_buf, output_buf, output_row,
                                num_rows);
      break;
  }
}






METHODDEF(void)
rgb_rgb_convert (j_compress_ptr cinfo,
                  JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
                  JDIMENSION output_row, int num_rows)
{
  switch (cinfo->in_color_space) {
    case JCS_EXT_RGB:
      extrgb_rgb_convert_internal(cinfo, input_buf, output_buf, output_row,
                                  num_rows);
      break;
    case JCS_EXT_RGBX:
    case JCS_EXT_RGBA:
      extrgbx_rgb_convert_internal(cinfo, input_buf, output_buf, output_row,
                                   num_rows);
      break;
    case JCS_EXT_BGR:
      extbgr_rgb_convert_internal(cinfo, input_buf, output_buf, output_row,
                                  num_rows);
      break;
    case JCS_EXT_BGRX:
    case JCS_EXT_BGRA:
      extbgrx_rgb_convert_internal(cinfo, input_buf, output_buf, output_row,
                                   num_rows);
      break;
    case JCS_EXT_XBGR:
    case JCS_EXT_ABGR:
      extxbgr_rgb_convert_internal(cinfo, input_buf, output_buf, output_row,
                                   num_rows);
      break;
    case JCS_EXT_XRGB:
    case JCS_EXT_ARGB:
      extxrgb_rgb_convert_internal(cinfo, input_buf, output_buf, output_row,
                                   num_rows);
      break;
    default:
      rgb_rgb_convert_internal(cinfo, input_buf, output_buf, output_row,
                               num_rows);
      break;
  }
}










METHODDEF(void)
cmyk_ycck_convert (j_compress_ptr cinfo,
                   JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
                   JDIMENSION output_row, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb_ycc_tab;
  register JSAMPROW inptr;
  register JSAMPROW outptr0, outptr1, outptr2, outptr3;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;

  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr0 = output_buf[0][output_row];
    outptr1 = output_buf[1][output_row];
    outptr2 = output_buf[2][output_row];
    outptr3 = output_buf[3][output_row];
    output_row++;
    for (col = 0; col < num_cols; col++) {
      r = MAXJSAMPLE - GETJSAMPLE(inptr[0]);
      g = MAXJSAMPLE - GETJSAMPLE(inptr[1]);
      b = MAXJSAMPLE - GETJSAMPLE(inptr[2]);
      
      outptr3[col] = inptr[3];  
      inptr += 4;
      




      
      outptr0[col] = (JSAMPLE)
                ((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
                 >> SCALEBITS);
      
      outptr1[col] = (JSAMPLE)
                ((ctab[r+R_CB_OFF] + ctab[g+G_CB_OFF] + ctab[b+B_CB_OFF])
                 >> SCALEBITS);
      
      outptr2[col] = (JSAMPLE)
                ((ctab[r+R_CR_OFF] + ctab[g+G_CR_OFF] + ctab[b+B_CR_OFF])
                 >> SCALEBITS);
    }
  }
}








METHODDEF(void)
grayscale_convert (j_compress_ptr cinfo,
                   JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
                   JDIMENSION output_row, int num_rows)
{
  register JSAMPROW inptr;
  register JSAMPROW outptr;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;
  int instride = cinfo->input_components;

  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr = output_buf[0][output_row];
    output_row++;
    for (col = 0; col < num_cols; col++) {
      outptr[col] = inptr[0];   
      inptr += instride;
    }
  }
}








METHODDEF(void)
null_convert (j_compress_ptr cinfo,
              JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
              JDIMENSION output_row, int num_rows)
{
  register JSAMPROW inptr;
  register JSAMPROW outptr;
  register JDIMENSION col;
  register int ci;
  int nc = cinfo->num_components;
  JDIMENSION num_cols = cinfo->image_width;

  while (--num_rows >= 0) {
    
    for (ci = 0; ci < nc; ci++) {
      inptr = *input_buf;
      outptr = output_buf[ci][output_row];
      for (col = 0; col < num_cols; col++) {
        outptr[col] = inptr[ci]; 
        inptr += nc;
      }
    }
    input_buf++;
    output_row++;
  }
}






METHODDEF(void)
null_method (j_compress_ptr cinfo)
{
  
}






GLOBAL(void)
jinit_color_converter (j_compress_ptr cinfo)
{
  my_cconvert_ptr cconvert;

  cconvert = (my_cconvert_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                sizeof(my_color_converter));
  cinfo->cconvert = (struct jpeg_color_converter *) cconvert;
  
  cconvert->pub.start_pass = null_method;

  
  switch (cinfo->in_color_space) {
  case JCS_GRAYSCALE:
    if (cinfo->input_components != 1)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
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
    if (cinfo->input_components != rgb_pixelsize[cinfo->in_color_space])
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;

  case JCS_YCbCr:
    if (cinfo->input_components != 3)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;

  case JCS_CMYK:
  case JCS_YCCK:
    if (cinfo->input_components != 4)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;

  default:                      
    if (cinfo->input_components < 1)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;
  }

  
  switch (cinfo->jpeg_color_space) {
  case JCS_GRAYSCALE:
    if (cinfo->num_components != 1)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_GRAYSCALE)
      cconvert->pub.color_convert = grayscale_convert;
    else if (cinfo->in_color_space == JCS_RGB ||
             cinfo->in_color_space == JCS_EXT_RGB ||
             cinfo->in_color_space == JCS_EXT_RGBX ||
             cinfo->in_color_space == JCS_EXT_BGR ||
             cinfo->in_color_space == JCS_EXT_BGRX ||
             cinfo->in_color_space == JCS_EXT_XBGR ||
             cinfo->in_color_space == JCS_EXT_XRGB ||
             cinfo->in_color_space == JCS_EXT_RGBA ||
             cinfo->in_color_space == JCS_EXT_BGRA ||
             cinfo->in_color_space == JCS_EXT_ABGR ||
             cinfo->in_color_space == JCS_EXT_ARGB) {
      if (jsimd_can_rgb_gray())
        cconvert->pub.color_convert = jsimd_rgb_gray_convert;
      else {
        cconvert->pub.start_pass = rgb_ycc_start;
        cconvert->pub.color_convert = rgb_gray_convert;
      }
    } else if (cinfo->in_color_space == JCS_YCbCr)
      cconvert->pub.color_convert = grayscale_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_RGB:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (rgb_red[cinfo->in_color_space] == 0 &&
        rgb_green[cinfo->in_color_space] == 1 &&
        rgb_blue[cinfo->in_color_space] == 2 &&
        rgb_pixelsize[cinfo->in_color_space] == 3) {
#if defined(__mips__)
      if (jsimd_c_can_null_convert())
        cconvert->pub.color_convert = jsimd_c_null_convert;
      else
#endif
        cconvert->pub.color_convert = null_convert;
    } else if (cinfo->in_color_space == JCS_RGB ||
               cinfo->in_color_space == JCS_EXT_RGB ||
               cinfo->in_color_space == JCS_EXT_RGBX ||
               cinfo->in_color_space == JCS_EXT_BGR ||
               cinfo->in_color_space == JCS_EXT_BGRX ||
               cinfo->in_color_space == JCS_EXT_XBGR ||
               cinfo->in_color_space == JCS_EXT_XRGB ||
               cinfo->in_color_space == JCS_EXT_RGBA ||
               cinfo->in_color_space == JCS_EXT_BGRA ||
               cinfo->in_color_space == JCS_EXT_ABGR ||
               cinfo->in_color_space == JCS_EXT_ARGB)
      cconvert->pub.color_convert = rgb_rgb_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_YCbCr:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_RGB ||
        cinfo->in_color_space == JCS_EXT_RGB ||
        cinfo->in_color_space == JCS_EXT_RGBX ||
        cinfo->in_color_space == JCS_EXT_BGR ||
        cinfo->in_color_space == JCS_EXT_BGRX ||
        cinfo->in_color_space == JCS_EXT_XBGR ||
        cinfo->in_color_space == JCS_EXT_XRGB ||
        cinfo->in_color_space == JCS_EXT_RGBA ||
        cinfo->in_color_space == JCS_EXT_BGRA ||
        cinfo->in_color_space == JCS_EXT_ABGR ||
        cinfo->in_color_space == JCS_EXT_ARGB) {
      if (jsimd_can_rgb_ycc())
        cconvert->pub.color_convert = jsimd_rgb_ycc_convert;
      else {
        cconvert->pub.start_pass = rgb_ycc_start;
        cconvert->pub.color_convert = rgb_ycc_convert;
      }
    } else if (cinfo->in_color_space == JCS_YCbCr) {
#if defined(__mips__)
      if (jsimd_c_can_null_convert())
        cconvert->pub.color_convert = jsimd_c_null_convert;
      else
#endif
        cconvert->pub.color_convert = null_convert;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_CMYK:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_CMYK) {
#if defined(__mips__)
      if (jsimd_c_can_null_convert())
        cconvert->pub.color_convert = jsimd_c_null_convert;
      else
#endif
        cconvert->pub.color_convert = null_convert;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_YCCK:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_CMYK) {
      cconvert->pub.start_pass = rgb_ycc_start;
      cconvert->pub.color_convert = cmyk_ycck_convert;
    } else if (cinfo->in_color_space == JCS_YCCK) {
#if defined(__mips__)
      if (jsimd_c_can_null_convert())
        cconvert->pub.color_convert = jsimd_c_null_convert;
      else
#endif
        cconvert->pub.color_convert = null_convert;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  default:                      
    if (cinfo->jpeg_color_space != cinfo->in_color_space ||
        cinfo->num_components != cinfo->input_components)
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
#if defined(__mips__)
    if (jsimd_c_can_null_convert())
      cconvert->pub.color_convert = jsimd_c_null_convert;
    else
#endif
      cconvert->pub.color_convert = null_convert;
    break;
  }
}

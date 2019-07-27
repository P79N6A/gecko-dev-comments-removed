














#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jsimd.h"
#include "jconfigint.h"




typedef struct {
  struct jpeg_color_deconverter pub; 

  
  int * Cr_r_tab;               
  int * Cb_b_tab;               
  INT32 * Cr_g_tab;             
  INT32 * Cb_g_tab;             

  
  INT32 * rgb_y_tab;            
} my_color_deconverter;

typedef my_color_deconverter * my_cconvert_ptr;




































#define SCALEBITS       16      /* speediest right-shift on some machines */
#define ONE_HALF        ((INT32) 1 << (SCALEBITS-1))
#define FIX(x)          ((INT32) ((x) * (1L<<SCALEBITS) + 0.5))








#define R_Y_OFF         0                       /* offset to R => Y section */
#define G_Y_OFF         (1*(MAXJSAMPLE+1))      /* offset to G => Y section */
#define B_Y_OFF         (2*(MAXJSAMPLE+1))      /* etc. */
#define TABLE_SIZE      (3*(MAXJSAMPLE+1))




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
#define rgb_rgb_convert_internal rgb_extrgb_convert_internal
#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef ycc_rgb_convert_internal
#undef gray_rgb_convert_internal
#undef rgb_rgb_convert_internal

#define RGB_RED EXT_RGBX_RED
#define RGB_GREEN EXT_RGBX_GREEN
#define RGB_BLUE EXT_RGBX_BLUE
#define RGB_ALPHA 3
#define RGB_PIXELSIZE EXT_RGBX_PIXELSIZE
#define ycc_rgb_convert_internal ycc_extrgbx_convert_internal
#define gray_rgb_convert_internal gray_extrgbx_convert_internal
#define rgb_rgb_convert_internal rgb_extrgbx_convert_internal
#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef ycc_rgb_convert_internal
#undef gray_rgb_convert_internal
#undef rgb_rgb_convert_internal

#define RGB_RED EXT_BGR_RED
#define RGB_GREEN EXT_BGR_GREEN
#define RGB_BLUE EXT_BGR_BLUE
#define RGB_PIXELSIZE EXT_BGR_PIXELSIZE
#define ycc_rgb_convert_internal ycc_extbgr_convert_internal
#define gray_rgb_convert_internal gray_extbgr_convert_internal
#define rgb_rgb_convert_internal rgb_extbgr_convert_internal
#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef ycc_rgb_convert_internal
#undef gray_rgb_convert_internal
#undef rgb_rgb_convert_internal

#define RGB_RED EXT_BGRX_RED
#define RGB_GREEN EXT_BGRX_GREEN
#define RGB_BLUE EXT_BGRX_BLUE
#define RGB_ALPHA 3
#define RGB_PIXELSIZE EXT_BGRX_PIXELSIZE
#define ycc_rgb_convert_internal ycc_extbgrx_convert_internal
#define gray_rgb_convert_internal gray_extbgrx_convert_internal
#define rgb_rgb_convert_internal rgb_extbgrx_convert_internal
#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef ycc_rgb_convert_internal
#undef gray_rgb_convert_internal
#undef rgb_rgb_convert_internal

#define RGB_RED EXT_XBGR_RED
#define RGB_GREEN EXT_XBGR_GREEN
#define RGB_BLUE EXT_XBGR_BLUE
#define RGB_ALPHA 0
#define RGB_PIXELSIZE EXT_XBGR_PIXELSIZE
#define ycc_rgb_convert_internal ycc_extxbgr_convert_internal
#define gray_rgb_convert_internal gray_extxbgr_convert_internal
#define rgb_rgb_convert_internal rgb_extxbgr_convert_internal
#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef ycc_rgb_convert_internal
#undef gray_rgb_convert_internal
#undef rgb_rgb_convert_internal

#define RGB_RED EXT_XRGB_RED
#define RGB_GREEN EXT_XRGB_GREEN
#define RGB_BLUE EXT_XRGB_BLUE
#define RGB_ALPHA 0
#define RGB_PIXELSIZE EXT_XRGB_PIXELSIZE
#define ycc_rgb_convert_internal ycc_extxrgb_convert_internal
#define gray_rgb_convert_internal gray_extxrgb_convert_internal
#define rgb_rgb_convert_internal rgb_extxrgb_convert_internal
#include "jdcolext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef ycc_rgb_convert_internal
#undef gray_rgb_convert_internal
#undef rgb_rgb_convert_internal






LOCAL(void)
build_ycc_rgb_table (j_decompress_ptr cinfo)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  int i;
  INT32 x;
  SHIFT_TEMPS

  cconvert->Cr_r_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                (MAXJSAMPLE+1) * sizeof(int));
  cconvert->Cb_b_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                (MAXJSAMPLE+1) * sizeof(int));
  cconvert->Cr_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                (MAXJSAMPLE+1) * sizeof(INT32));
  cconvert->Cb_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                (MAXJSAMPLE+1) * sizeof(INT32));

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









LOCAL(void)
build_rgb_y_table (j_decompress_ptr cinfo)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  INT32 * rgb_y_tab;
  INT32 i;

  
  cconvert->rgb_y_tab = rgb_y_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                (TABLE_SIZE * sizeof(INT32)));

  for (i = 0; i <= MAXJSAMPLE; i++) {
    rgb_y_tab[i+R_Y_OFF] = FIX(0.29900) * i;
    rgb_y_tab[i+G_Y_OFF] = FIX(0.58700) * i;
    rgb_y_tab[i+B_Y_OFF] = FIX(0.11400) * i + ONE_HALF;
  }
}






METHODDEF(void)
rgb_gray_convert (j_decompress_ptr cinfo,
                  JSAMPIMAGE input_buf, JDIMENSION input_row,
                  JSAMPARRAY output_buf, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb_y_tab;
  register JSAMPROW outptr;
  register JSAMPROW inptr0, inptr1, inptr2;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;

  while (--num_rows >= 0) {
    inptr0 = input_buf[0][input_row];
    inptr1 = input_buf[1][input_row];
    inptr2 = input_buf[2][input_row];
    input_row++;
    outptr = *output_buf++;
    for (col = 0; col < num_cols; col++) {
      r = GETJSAMPLE(inptr0[col]);
      g = GETJSAMPLE(inptr1[col]);
      b = GETJSAMPLE(inptr2[col]);
      
      outptr[col] = (JSAMPLE)
                ((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
                 >> SCALEBITS);
    }
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
rgb_rgb_convert (j_decompress_ptr cinfo,
                  JSAMPIMAGE input_buf, JDIMENSION input_row,
                  JSAMPARRAY output_buf, int num_rows)
{
  switch (cinfo->out_color_space) {
    case JCS_EXT_RGB:
      rgb_extrgb_convert_internal(cinfo, input_buf, input_row, output_buf,
                                  num_rows);
      break;
    case JCS_EXT_RGBX:
    case JCS_EXT_RGBA:
      rgb_extrgbx_convert_internal(cinfo, input_buf, input_row, output_buf,
                                   num_rows);
      break;
    case JCS_EXT_BGR:
      rgb_extbgr_convert_internal(cinfo, input_buf, input_row, output_buf,
                                  num_rows);
      break;
    case JCS_EXT_BGRX:
    case JCS_EXT_BGRA:
      rgb_extbgrx_convert_internal(cinfo, input_buf, input_row, output_buf,
                                   num_rows);
      break;
    case JCS_EXT_XBGR:
    case JCS_EXT_ABGR:
      rgb_extxbgr_convert_internal(cinfo, input_buf, input_row, output_buf,
                                   num_rows);
      break;
    case JCS_EXT_XRGB:
    case JCS_EXT_ARGB:
      rgb_extxrgb_convert_internal(cinfo, input_buf, input_row, output_buf,
                                   num_rows);
      break;
    default:
      rgb_rgb_convert_internal(cinfo, input_buf, input_row, output_buf,
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






#define PACK_SHORT_565_LE(r, g, b)   ((((r) << 8) & 0xF800) |  \
                                      (((g) << 3) & 0x7E0) | ((b) >> 3))
#define PACK_SHORT_565_BE(r, g, b)   (((r) & 0xF8) | ((g) >> 5) |  \
                                      (((g) << 11) & 0xE000) |  \
                                      (((b) << 5) & 0x1F00))

#define PACK_TWO_PIXELS_LE(l, r)     ((r << 16) | l)
#define PACK_TWO_PIXELS_BE(l, r)     ((l << 16) | r)

#define PACK_NEED_ALIGNMENT(ptr)     (((size_t)(ptr)) & 3)

#define WRITE_TWO_ALIGNED_PIXELS(addr, pixels)  ((*(int *)(addr)) = pixels)

#define DITHER_565_R(r, dither)  ((r) + ((dither) & 0xFF))
#define DITHER_565_G(g, dither)  ((g) + (((dither) & 0xFF) >> 1))
#define DITHER_565_B(b, dither)  ((b) + ((dither) & 0xFF))








#define DITHER_MASK       0x3
#define DITHER_ROTATE(x)  (((x) << 24) | (((x) >> 8) & 0x00FFFFFF))
static const INT32 dither_matrix[4] = {
  0x0008020A,
  0x0C040E06,
  0x030B0109,
  0x0F070D05
};


static INLINE boolean is_big_endian(void)
{
  int test_value = 1;
  if(*(char *)&test_value != 1)
    return TRUE;
  return FALSE;
}




#define PACK_SHORT_565 PACK_SHORT_565_LE
#define PACK_TWO_PIXELS PACK_TWO_PIXELS_LE
#define ycc_rgb565_convert_internal ycc_rgb565_convert_le
#define ycc_rgb565D_convert_internal ycc_rgb565D_convert_le
#define rgb_rgb565_convert_internal rgb_rgb565_convert_le
#define rgb_rgb565D_convert_internal rgb_rgb565D_convert_le
#define gray_rgb565_convert_internal gray_rgb565_convert_le
#define gray_rgb565D_convert_internal gray_rgb565D_convert_le
#include "jdcol565.c"
#undef PACK_SHORT_565
#undef PACK_TWO_PIXELS
#undef ycc_rgb565_convert_internal
#undef ycc_rgb565D_convert_internal
#undef rgb_rgb565_convert_internal
#undef rgb_rgb565D_convert_internal
#undef gray_rgb565_convert_internal
#undef gray_rgb565D_convert_internal

#define PACK_SHORT_565 PACK_SHORT_565_BE
#define PACK_TWO_PIXELS PACK_TWO_PIXELS_BE
#define ycc_rgb565_convert_internal ycc_rgb565_convert_be
#define ycc_rgb565D_convert_internal ycc_rgb565D_convert_be
#define rgb_rgb565_convert_internal rgb_rgb565_convert_be
#define rgb_rgb565D_convert_internal rgb_rgb565D_convert_be
#define gray_rgb565_convert_internal gray_rgb565_convert_be
#define gray_rgb565D_convert_internal gray_rgb565D_convert_be
#include "jdcol565.c"
#undef PACK_SHORT_565
#undef PACK_TWO_PIXELS
#undef ycc_rgb565_convert_internal
#undef ycc_rgb565D_convert_internal
#undef rgb_rgb565_convert_internal
#undef rgb_rgb565D_convert_internal
#undef gray_rgb565_convert_internal
#undef gray_rgb565D_convert_internal


METHODDEF(void)
ycc_rgb565_convert (j_decompress_ptr cinfo,
                    JSAMPIMAGE input_buf, JDIMENSION input_row,
                    JSAMPARRAY output_buf, int num_rows)
{
  if (is_big_endian())
    ycc_rgb565_convert_be(cinfo, input_buf, input_row, output_buf, num_rows);
  else
    ycc_rgb565_convert_le(cinfo, input_buf, input_row, output_buf, num_rows);
}


METHODDEF(void)
ycc_rgb565D_convert (j_decompress_ptr cinfo,
                     JSAMPIMAGE input_buf, JDIMENSION input_row,
                     JSAMPARRAY output_buf, int num_rows)
{
  if (is_big_endian())
    ycc_rgb565D_convert_be(cinfo, input_buf, input_row, output_buf, num_rows);
  else
    ycc_rgb565D_convert_le(cinfo, input_buf, input_row, output_buf, num_rows);
}


METHODDEF(void)
rgb_rgb565_convert (j_decompress_ptr cinfo,
                    JSAMPIMAGE input_buf, JDIMENSION input_row,
                    JSAMPARRAY output_buf, int num_rows)
{
  if (is_big_endian())
    rgb_rgb565_convert_be(cinfo, input_buf, input_row, output_buf, num_rows);
  else
    rgb_rgb565_convert_le(cinfo, input_buf, input_row, output_buf, num_rows);
}


METHODDEF(void)
rgb_rgb565D_convert (j_decompress_ptr cinfo,
                     JSAMPIMAGE input_buf, JDIMENSION input_row,
                     JSAMPARRAY output_buf, int num_rows)
{
  if (is_big_endian())
    rgb_rgb565D_convert_be(cinfo, input_buf, input_row, output_buf, num_rows);
  else
    rgb_rgb565D_convert_le(cinfo, input_buf, input_row, output_buf, num_rows);
}


METHODDEF(void)
gray_rgb565_convert (j_decompress_ptr cinfo,
                     JSAMPIMAGE input_buf, JDIMENSION input_row,
                     JSAMPARRAY output_buf, int num_rows)
{
  if (is_big_endian())
    gray_rgb565_convert_be(cinfo, input_buf, input_row, output_buf, num_rows);
  else
    gray_rgb565_convert_le(cinfo, input_buf, input_row, output_buf, num_rows);
}


METHODDEF(void)
gray_rgb565D_convert (j_decompress_ptr cinfo,
                      JSAMPIMAGE input_buf, JDIMENSION input_row,
                      JSAMPARRAY output_buf, int num_rows)
{
  if (is_big_endian())
    gray_rgb565D_convert_be(cinfo, input_buf, input_row, output_buf, num_rows);
  else
    gray_rgb565D_convert_le(cinfo, input_buf, input_row, output_buf, num_rows);
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
                                sizeof(my_color_deconverter));
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
    } else if (cinfo->jpeg_color_space == JCS_RGB) {
      cconvert->pub.color_convert = rgb_gray_convert;
      build_rgb_y_table(cinfo);
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
    } else if (cinfo->jpeg_color_space == JCS_RGB) {
      if (rgb_red[cinfo->out_color_space] == 0 &&
          rgb_green[cinfo->out_color_space] == 1 &&
          rgb_blue[cinfo->out_color_space] == 2 &&
          rgb_pixelsize[cinfo->out_color_space] == 3)
        cconvert->pub.color_convert = null_convert;
      else
        cconvert->pub.color_convert = rgb_rgb_convert;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_RGB565:
    cinfo->out_color_components = 3;
    if (cinfo->dither_mode == JDITHER_NONE) {
      if (cinfo->jpeg_color_space == JCS_YCbCr) {
         if (jsimd_can_ycc_rgb565())
           cconvert->pub.color_convert = jsimd_ycc_rgb565_convert;
         else {
           cconvert->pub.color_convert = ycc_rgb565_convert;
           build_ycc_rgb_table(cinfo);
        }
      } else if (cinfo->jpeg_color_space == JCS_GRAYSCALE) {
        cconvert->pub.color_convert = gray_rgb565_convert;
      } else if (cinfo->jpeg_color_space == JCS_RGB) {
        cconvert->pub.color_convert = rgb_rgb565_convert;
      } else
        ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    } else {
      
      if (cinfo->jpeg_color_space == JCS_YCbCr) {
        cconvert->pub.color_convert = ycc_rgb565D_convert;
        build_ycc_rgb_table(cinfo);
      } else if (cinfo->jpeg_color_space == JCS_GRAYSCALE) {
        cconvert->pub.color_convert = gray_rgb565D_convert;
      } else if (cinfo->jpeg_color_space == JCS_RGB) {
        cconvert->pub.color_convert = rgb_rgb565D_convert;
      } else
        ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    }
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

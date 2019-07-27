





































#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jsimd.h"
#include "jconfigint.h"

#ifdef UPSAMPLE_MERGING_SUPPORTED




typedef struct {
  struct jpeg_upsampler pub;    

  
  void (*upmethod) (j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                    JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf);

  
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

#define SCALEBITS       16      /* speediest right-shift on some machines */
#define ONE_HALF        ((INT32) 1 << (SCALEBITS-1))
#define FIX(x)          ((INT32) ((x) * (1L<<SCALEBITS) + 0.5))




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
#define RGB_ALPHA 3
#define RGB_PIXELSIZE EXT_RGBX_PIXELSIZE
#define h2v1_merged_upsample_internal extrgbx_h2v1_merged_upsample_internal
#define h2v2_merged_upsample_internal extrgbx_h2v2_merged_upsample_internal
#include "jdmrgext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
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
#define RGB_ALPHA 3
#define RGB_PIXELSIZE EXT_BGRX_PIXELSIZE
#define h2v1_merged_upsample_internal extbgrx_h2v1_merged_upsample_internal
#define h2v2_merged_upsample_internal extbgrx_h2v2_merged_upsample_internal
#include "jdmrgext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef h2v1_merged_upsample_internal
#undef h2v2_merged_upsample_internal

#define RGB_RED EXT_XBGR_RED
#define RGB_GREEN EXT_XBGR_GREEN
#define RGB_BLUE EXT_XBGR_BLUE
#define RGB_ALPHA 0
#define RGB_PIXELSIZE EXT_XBGR_PIXELSIZE
#define h2v1_merged_upsample_internal extxbgr_h2v1_merged_upsample_internal
#define h2v2_merged_upsample_internal extxbgr_h2v2_merged_upsample_internal
#include "jdmrgext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef h2v1_merged_upsample_internal
#undef h2v2_merged_upsample_internal

#define RGB_RED EXT_XRGB_RED
#define RGB_GREEN EXT_XRGB_GREEN
#define RGB_BLUE EXT_XRGB_BLUE
#define RGB_ALPHA 0
#define RGB_PIXELSIZE EXT_XRGB_PIXELSIZE
#define h2v1_merged_upsample_internal extxrgb_h2v1_merged_upsample_internal
#define h2v2_merged_upsample_internal extxrgb_h2v2_merged_upsample_internal
#include "jdmrgext.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
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
                                (MAXJSAMPLE+1) * sizeof(int));
  upsample->Cb_b_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                (MAXJSAMPLE+1) * sizeof(int));
  upsample->Cr_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                (MAXJSAMPLE+1) * sizeof(INT32));
  upsample->Cb_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                (MAXJSAMPLE+1) * sizeof(INT32));

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
    
    JDIMENSION size = upsample->out_row_width;
    if (cinfo->out_color_space == JCS_RGB565)
      size = cinfo->output_width * 2;
    jcopy_sample_rows(& upsample->spare_row, 0, output_buf + *out_row_ctr, 0,
                      1, size);
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






#define PACK_SHORT_565_LE(r, g, b)   ((((r) << 8) & 0xF800) |  \
                                      (((g) << 3) & 0x7E0) | ((b) >> 3))
#define PACK_SHORT_565_BE(r, g, b)   (((r) & 0xF8) | ((g) >> 5) |  \
                                      (((g) << 11) & 0xE000) |  \
                                      (((b) << 5) & 0x1F00))

#define PACK_TWO_PIXELS_LE(l, r)     ((r << 16) | l)
#define PACK_TWO_PIXELS_BE(l, r)     ((l << 16) | r)

#define PACK_NEED_ALIGNMENT(ptr)  (((size_t)(ptr)) & 3)

#define WRITE_TWO_PIXELS_LE(addr, pixels) {  \
  ((INT16*)(addr))[0] = (pixels);  \
  ((INT16*)(addr))[1] = (pixels) >> 16;  \
}
#define WRITE_TWO_PIXELS_BE(addr, pixels) {  \
  ((INT16*)(addr))[1] = (pixels);  \
  ((INT16*)(addr))[0] = (pixels) >> 16;  \
}

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




#define PACK_SHORT_565 PACK_SHORT_565_LE
#define PACK_TWO_PIXELS PACK_TWO_PIXELS_LE
#define WRITE_TWO_PIXELS WRITE_TWO_PIXELS_LE
#define h2v1_merged_upsample_565_internal h2v1_merged_upsample_565_le
#define h2v1_merged_upsample_565D_internal h2v1_merged_upsample_565D_le
#define h2v2_merged_upsample_565_internal h2v2_merged_upsample_565_le
#define h2v2_merged_upsample_565D_internal h2v2_merged_upsample_565D_le
#include "jdmrg565.c"
#undef PACK_SHORT_565
#undef PACK_TWO_PIXELS
#undef WRITE_TWO_PIXELS
#undef h2v1_merged_upsample_565_internal
#undef h2v1_merged_upsample_565D_internal
#undef h2v2_merged_upsample_565_internal
#undef h2v2_merged_upsample_565D_internal

#define PACK_SHORT_565 PACK_SHORT_565_BE
#define PACK_TWO_PIXELS PACK_TWO_PIXELS_BE
#define WRITE_TWO_PIXELS WRITE_TWO_PIXELS_BE
#define h2v1_merged_upsample_565_internal h2v1_merged_upsample_565_be
#define h2v1_merged_upsample_565D_internal h2v1_merged_upsample_565D_be
#define h2v2_merged_upsample_565_internal h2v2_merged_upsample_565_be
#define h2v2_merged_upsample_565D_internal h2v2_merged_upsample_565D_be
#include "jdmrg565.c"
#undef PACK_SHORT_565
#undef PACK_TWO_PIXELS
#undef WRITE_TWO_PIXELS
#undef h2v1_merged_upsample_565_internal
#undef h2v1_merged_upsample_565D_internal
#undef h2v2_merged_upsample_565_internal
#undef h2v2_merged_upsample_565D_internal


static INLINE boolean is_big_endian(void)
{
  int test_value = 1;
  if(*(char *)&test_value != 1)
    return TRUE;
  return FALSE;
}


METHODDEF(void)
h2v1_merged_upsample_565 (j_decompress_ptr cinfo,
                          JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
                          JSAMPARRAY output_buf)
{
  if (is_big_endian())
    h2v1_merged_upsample_565_be(cinfo, input_buf, in_row_group_ctr,
                                output_buf);
  else
    h2v1_merged_upsample_565_le(cinfo, input_buf, in_row_group_ctr,
                                output_buf);
 }


METHODDEF(void)
h2v1_merged_upsample_565D (j_decompress_ptr cinfo,
                           JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
                           JSAMPARRAY output_buf)
{
  if (is_big_endian())
    h2v1_merged_upsample_565D_be(cinfo, input_buf, in_row_group_ctr,
                                 output_buf);
  else
    h2v1_merged_upsample_565D_le(cinfo, input_buf, in_row_group_ctr,
                                 output_buf);
}


METHODDEF(void)
h2v2_merged_upsample_565 (j_decompress_ptr cinfo,
                          JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
                          JSAMPARRAY output_buf)
{
  if (is_big_endian())
    h2v2_merged_upsample_565_be(cinfo, input_buf, in_row_group_ctr,
                                output_buf);
  else
    h2v2_merged_upsample_565_le(cinfo, input_buf, in_row_group_ctr,
                                output_buf);
}


METHODDEF(void)
h2v2_merged_upsample_565D (j_decompress_ptr cinfo,
                           JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
                           JSAMPARRAY output_buf)
{
  if (is_big_endian())
    h2v2_merged_upsample_565D_be(cinfo, input_buf, in_row_group_ctr,
                                 output_buf);
  else
    h2v2_merged_upsample_565D_le(cinfo, input_buf, in_row_group_ctr,
                                 output_buf);
}










GLOBAL(void)
jinit_merged_upsampler (j_decompress_ptr cinfo)
{
  my_upsample_ptr upsample;

  upsample = (my_upsample_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                sizeof(my_upsampler));
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
    if (cinfo->out_color_space == JCS_RGB565) {
      if (cinfo->dither_mode != JDITHER_NONE) {
        upsample->upmethod = h2v2_merged_upsample_565D;
      } else {
        upsample->upmethod = h2v2_merged_upsample_565;
      }
    }
    
    upsample->spare_row = (JSAMPROW)
      (*cinfo->mem->alloc_large) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                (size_t) (upsample->out_row_width * sizeof(JSAMPLE)));
  } else {
    upsample->pub.upsample = merged_1v_upsample;
    if (jsimd_can_h2v1_merged_upsample())
      upsample->upmethod = jsimd_h2v1_merged_upsample;
    else
      upsample->upmethod = h2v1_merged_upsample;
    if (cinfo->out_color_space == JCS_RGB565) {
      if (cinfo->dither_mode != JDITHER_NONE) {
        upsample->upmethod = h2v1_merged_upsample_565D;
      } else {
        upsample->upmethod = h2v1_merged_upsample_565;
      }
    }
    
    upsample->spare_row = NULL;
  }

  build_ycc_rgb_table(cinfo);
}

#endif 

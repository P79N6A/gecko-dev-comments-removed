

















#include "pngpriv.h"

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)

#ifdef PNG_bKGD_SUPPORTED
void PNGAPI
png_set_bKGD(png_const_structrp png_ptr, png_inforp info_ptr,
    png_const_color_16p background)
{
   png_debug1(1, "in %s storage function", "bKGD");

   if (png_ptr == NULL || info_ptr == NULL || background == NULL)
      return;

   info_ptr->background = *background;
   info_ptr->valid |= PNG_INFO_bKGD;
}
#endif

#ifdef PNG_cHRM_SUPPORTED
void PNGFAPI
png_set_cHRM_fixed(png_const_structrp png_ptr, png_inforp info_ptr,
    png_fixed_point white_x, png_fixed_point white_y, png_fixed_point red_x,
    png_fixed_point red_y, png_fixed_point green_x, png_fixed_point green_y,
    png_fixed_point blue_x, png_fixed_point blue_y)
{
   png_xy xy;

   png_debug1(1, "in %s storage function", "cHRM fixed");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   xy.redx = red_x;
   xy.redy = red_y;
   xy.greenx = green_x;
   xy.greeny = green_y;
   xy.bluex = blue_x;
   xy.bluey = blue_y;
   xy.whitex = white_x;
   xy.whitey = white_y;

   if (png_colorspace_set_chromaticities(png_ptr, &info_ptr->colorspace, &xy,
       2) != 0)
      info_ptr->colorspace.flags |= PNG_COLORSPACE_FROM_cHRM;

   png_colorspace_sync_info(png_ptr, info_ptr);
}

void PNGFAPI
png_set_cHRM_XYZ_fixed(png_const_structrp png_ptr, png_inforp info_ptr,
    png_fixed_point int_red_X, png_fixed_point int_red_Y,
    png_fixed_point int_red_Z, png_fixed_point int_green_X,
    png_fixed_point int_green_Y, png_fixed_point int_green_Z,
    png_fixed_point int_blue_X, png_fixed_point int_blue_Y,
    png_fixed_point int_blue_Z)
{
   png_XYZ XYZ;

   png_debug1(1, "in %s storage function", "cHRM XYZ fixed");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   XYZ.red_X = int_red_X;
   XYZ.red_Y = int_red_Y;
   XYZ.red_Z = int_red_Z;
   XYZ.green_X = int_green_X;
   XYZ.green_Y = int_green_Y;
   XYZ.green_Z = int_green_Z;
   XYZ.blue_X = int_blue_X;
   XYZ.blue_Y = int_blue_Y;
   XYZ.blue_Z = int_blue_Z;

   if (png_colorspace_set_endpoints(png_ptr, &info_ptr->colorspace,
       &XYZ, 2) != 0)
      info_ptr->colorspace.flags |= PNG_COLORSPACE_FROM_cHRM;

   png_colorspace_sync_info(png_ptr, info_ptr);
}

#  ifdef PNG_FLOATING_POINT_SUPPORTED
void PNGAPI
png_set_cHRM(png_const_structrp png_ptr, png_inforp info_ptr,
    double white_x, double white_y, double red_x, double red_y,
    double green_x, double green_y, double blue_x, double blue_y)
{
   png_set_cHRM_fixed(png_ptr, info_ptr,
      png_fixed(png_ptr, white_x, "cHRM White X"),
      png_fixed(png_ptr, white_y, "cHRM White Y"),
      png_fixed(png_ptr, red_x, "cHRM Red X"),
      png_fixed(png_ptr, red_y, "cHRM Red Y"),
      png_fixed(png_ptr, green_x, "cHRM Green X"),
      png_fixed(png_ptr, green_y, "cHRM Green Y"),
      png_fixed(png_ptr, blue_x, "cHRM Blue X"),
      png_fixed(png_ptr, blue_y, "cHRM Blue Y"));
}

void PNGAPI
png_set_cHRM_XYZ(png_const_structrp png_ptr, png_inforp info_ptr, double red_X,
    double red_Y, double red_Z, double green_X, double green_Y, double green_Z,
    double blue_X, double blue_Y, double blue_Z)
{
   png_set_cHRM_XYZ_fixed(png_ptr, info_ptr,
      png_fixed(png_ptr, red_X, "cHRM Red X"),
      png_fixed(png_ptr, red_Y, "cHRM Red Y"),
      png_fixed(png_ptr, red_Z, "cHRM Red Z"),
      png_fixed(png_ptr, green_X, "cHRM Red X"),
      png_fixed(png_ptr, green_Y, "cHRM Red Y"),
      png_fixed(png_ptr, green_Z, "cHRM Red Z"),
      png_fixed(png_ptr, blue_X, "cHRM Red X"),
      png_fixed(png_ptr, blue_Y, "cHRM Red Y"),
      png_fixed(png_ptr, blue_Z, "cHRM Red Z"));
}
#  endif 

#endif 

#ifdef PNG_gAMA_SUPPORTED
void PNGFAPI
png_set_gAMA_fixed(png_const_structrp png_ptr, png_inforp info_ptr,
    png_fixed_point file_gamma)
{
   png_debug1(1, "in %s storage function", "gAMA");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   png_colorspace_set_gamma(png_ptr, &info_ptr->colorspace, file_gamma);
   png_colorspace_sync_info(png_ptr, info_ptr);
}

#  ifdef PNG_FLOATING_POINT_SUPPORTED
void PNGAPI
png_set_gAMA(png_const_structrp png_ptr, png_inforp info_ptr, double file_gamma)
{
   png_set_gAMA_fixed(png_ptr, info_ptr, png_fixed(png_ptr, file_gamma,
       "png_set_gAMA"));
}
#  endif
#endif

#ifdef PNG_hIST_SUPPORTED
void PNGAPI
png_set_hIST(png_const_structrp png_ptr, png_inforp info_ptr,
    png_const_uint_16p hist)
{
   int i;

   png_debug1(1, "in %s storage function", "hIST");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   if (info_ptr->num_palette == 0 || info_ptr->num_palette
       > PNG_MAX_PALETTE_LENGTH)
   {
      png_warning(png_ptr,
          "Invalid palette size, hIST allocation skipped");

      return;
   }

   png_free_data(png_ptr, info_ptr, PNG_FREE_HIST, 0);

   


   info_ptr->hist = png_voidcast(png_uint_16p, png_malloc_warn(png_ptr,
       PNG_MAX_PALETTE_LENGTH * (sizeof (png_uint_16))));

   if (info_ptr->hist == NULL)
   {
      png_warning(png_ptr, "Insufficient memory for hIST chunk data");
      return;
   }

   info_ptr->free_me |= PNG_FREE_HIST;

   for (i = 0; i < info_ptr->num_palette; i++)
      info_ptr->hist[i] = hist[i];

   info_ptr->valid |= PNG_INFO_hIST;
}
#endif

void PNGAPI
png_set_IHDR(png_const_structrp png_ptr, png_inforp info_ptr,
    png_uint_32 width, png_uint_32 height, int bit_depth,
    int color_type, int interlace_type, int compression_type,
    int filter_type)
{
   png_debug1(1, "in %s storage function", "IHDR");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->width = width;
   info_ptr->height = height;
   info_ptr->bit_depth = (png_byte)bit_depth;
   info_ptr->color_type = (png_byte)color_type;
   info_ptr->compression_type = (png_byte)compression_type;
   info_ptr->filter_type = (png_byte)filter_type;
   info_ptr->interlace_type = (png_byte)interlace_type;

   png_check_IHDR (png_ptr, info_ptr->width, info_ptr->height,
       info_ptr->bit_depth, info_ptr->color_type, info_ptr->interlace_type,
       info_ptr->compression_type, info_ptr->filter_type);

   if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      info_ptr->channels = 1;

   else if ((info_ptr->color_type & PNG_COLOR_MASK_COLOR) != 0)
      info_ptr->channels = 3;

   else
      info_ptr->channels = 1;

   if ((info_ptr->color_type & PNG_COLOR_MASK_ALPHA) != 0)
      info_ptr->channels++;

   info_ptr->pixel_depth = (png_byte)(info_ptr->channels * info_ptr->bit_depth);

   info_ptr->rowbytes = PNG_ROWBYTES(info_ptr->pixel_depth, width);

#ifdef PNG_APNG_SUPPORTED
   
   info_ptr->num_frames = 1;
#endif
}

#ifdef PNG_oFFs_SUPPORTED
void PNGAPI
png_set_oFFs(png_const_structrp png_ptr, png_inforp info_ptr,
    png_int_32 offset_x, png_int_32 offset_y, int unit_type)
{
   png_debug1(1, "in %s storage function", "oFFs");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->x_offset = offset_x;
   info_ptr->y_offset = offset_y;
   info_ptr->offset_unit_type = (png_byte)unit_type;
   info_ptr->valid |= PNG_INFO_oFFs;
}
#endif

#ifdef PNG_pCAL_SUPPORTED
void PNGAPI
png_set_pCAL(png_const_structrp png_ptr, png_inforp info_ptr,
    png_const_charp purpose, png_int_32 X0, png_int_32 X1, int type,
    int nparams, png_const_charp units, png_charpp params)
{
   png_size_t length;
   int i;

   png_debug1(1, "in %s storage function", "pCAL");

   if (png_ptr == NULL || info_ptr == NULL || purpose == NULL || units == NULL
      || (nparams > 0 && params == NULL))
      return;

   length = strlen(purpose) + 1;
   png_debug1(3, "allocating purpose for info (%lu bytes)",
       (unsigned long)length);

   

   
   if (type < 0 || type > 3)
      png_error(png_ptr, "Invalid pCAL equation type");

   if (nparams < 0 || nparams > 255)
      png_error(png_ptr, "Invalid pCAL parameter count");

   
   for (i=0; i<nparams; ++i)
   {
      if (params[i] == NULL ||
          !png_check_fp_string(params[i], strlen(params[i])))
         png_error(png_ptr, "Invalid format for pCAL parameter");
   }

   info_ptr->pcal_purpose = png_voidcast(png_charp,
       png_malloc_warn(png_ptr, length));

   if (info_ptr->pcal_purpose == NULL)
   {
      png_warning(png_ptr, "Insufficient memory for pCAL purpose");
      return;
   }

   memcpy(info_ptr->pcal_purpose, purpose, length);

   png_debug(3, "storing X0, X1, type, and nparams in info");
   info_ptr->pcal_X0 = X0;
   info_ptr->pcal_X1 = X1;
   info_ptr->pcal_type = (png_byte)type;
   info_ptr->pcal_nparams = (png_byte)nparams;

   length = strlen(units) + 1;
   png_debug1(3, "allocating units for info (%lu bytes)",
     (unsigned long)length);

   info_ptr->pcal_units = png_voidcast(png_charp,
      png_malloc_warn(png_ptr, length));

   if (info_ptr->pcal_units == NULL)
   {
      png_warning(png_ptr, "Insufficient memory for pCAL units");
      return;
   }

   memcpy(info_ptr->pcal_units, units, length);

   info_ptr->pcal_params = png_voidcast(png_charpp, png_malloc_warn(png_ptr,
       (png_size_t)((nparams + 1) * (sizeof (png_charp)))));

   if (info_ptr->pcal_params == NULL)
   {
      png_warning(png_ptr, "Insufficient memory for pCAL params");
      return;
   }

   memset(info_ptr->pcal_params, 0, (nparams + 1) * (sizeof (png_charp)));

   for (i = 0; i < nparams; i++)
   {
      length = strlen(params[i]) + 1;
      png_debug2(3, "allocating parameter %d for info (%lu bytes)", i,
          (unsigned long)length);

      info_ptr->pcal_params[i] = (png_charp)png_malloc_warn(png_ptr, length);

      if (info_ptr->pcal_params[i] == NULL)
      {
         png_warning(png_ptr, "Insufficient memory for pCAL parameter");
         return;
      }

      memcpy(info_ptr->pcal_params[i], params[i], length);
   }

   info_ptr->valid |= PNG_INFO_pCAL;
   info_ptr->free_me |= PNG_FREE_PCAL;
}
#endif

#ifdef PNG_sCAL_SUPPORTED
void PNGAPI
png_set_sCAL_s(png_const_structrp png_ptr, png_inforp info_ptr,
    int unit, png_const_charp swidth, png_const_charp sheight)
{
   png_size_t lengthw = 0, lengthh = 0;

   png_debug1(1, "in %s storage function", "sCAL");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   


   if (unit != 1 && unit != 2)
      png_error(png_ptr, "Invalid sCAL unit");

   if (swidth == NULL || (lengthw = strlen(swidth)) == 0 ||
       swidth[0] == 45  || !png_check_fp_string(swidth, lengthw))
      png_error(png_ptr, "Invalid sCAL width");

   if (sheight == NULL || (lengthh = strlen(sheight)) == 0 ||
       sheight[0] == 45  || !png_check_fp_string(sheight, lengthh))
      png_error(png_ptr, "Invalid sCAL height");

   info_ptr->scal_unit = (png_byte)unit;

   ++lengthw;

   png_debug1(3, "allocating unit for info (%u bytes)", (unsigned int)lengthw);

   info_ptr->scal_s_width = png_voidcast(png_charp,
      png_malloc_warn(png_ptr, lengthw));

   if (info_ptr->scal_s_width == NULL)
   {
      png_warning(png_ptr, "Memory allocation failed while processing sCAL");
      return;
   }

   memcpy(info_ptr->scal_s_width, swidth, lengthw);

   ++lengthh;

   png_debug1(3, "allocating unit for info (%u bytes)", (unsigned int)lengthh);

   info_ptr->scal_s_height = png_voidcast(png_charp,
      png_malloc_warn(png_ptr, lengthh));

   if (info_ptr->scal_s_height == NULL)
   {
      png_free (png_ptr, info_ptr->scal_s_width);
      info_ptr->scal_s_width = NULL;

      png_warning(png_ptr, "Memory allocation failed while processing sCAL");
      return;
   }

   memcpy(info_ptr->scal_s_height, sheight, lengthh);

   info_ptr->valid |= PNG_INFO_sCAL;
   info_ptr->free_me |= PNG_FREE_SCAL;
}

#  ifdef PNG_FLOATING_POINT_SUPPORTED
void PNGAPI
png_set_sCAL(png_const_structrp png_ptr, png_inforp info_ptr, int unit,
    double width, double height)
{
   png_debug1(1, "in %s storage function", "sCAL");

   
   if (width <= 0)
      png_warning(png_ptr, "Invalid sCAL width ignored");

   else if (height <= 0)
      png_warning(png_ptr, "Invalid sCAL height ignored");

   else
   {
      
      char swidth[PNG_sCAL_MAX_DIGITS+1];
      char sheight[PNG_sCAL_MAX_DIGITS+1];

      png_ascii_from_fp(png_ptr, swidth, (sizeof swidth), width,
         PNG_sCAL_PRECISION);
      png_ascii_from_fp(png_ptr, sheight, (sizeof sheight), height,
         PNG_sCAL_PRECISION);

      png_set_sCAL_s(png_ptr, info_ptr, unit, swidth, sheight);
   }
}
#  endif

#  ifdef PNG_FIXED_POINT_SUPPORTED
void PNGAPI
png_set_sCAL_fixed(png_const_structrp png_ptr, png_inforp info_ptr, int unit,
    png_fixed_point width, png_fixed_point height)
{
   png_debug1(1, "in %s storage function", "sCAL");

   
   if (width <= 0)
      png_warning(png_ptr, "Invalid sCAL width ignored");

   else if (height <= 0)
      png_warning(png_ptr, "Invalid sCAL height ignored");

   else
   {
      
      char swidth[PNG_sCAL_MAX_DIGITS+1];
      char sheight[PNG_sCAL_MAX_DIGITS+1];

      png_ascii_from_fixed(png_ptr, swidth, (sizeof swidth), width);
      png_ascii_from_fixed(png_ptr, sheight, (sizeof sheight), height);

      png_set_sCAL_s(png_ptr, info_ptr, unit, swidth, sheight);
   }
}
#  endif
#endif

#ifdef PNG_pHYs_SUPPORTED
void PNGAPI
png_set_pHYs(png_const_structrp png_ptr, png_inforp info_ptr,
    png_uint_32 res_x, png_uint_32 res_y, int unit_type)
{
   png_debug1(1, "in %s storage function", "pHYs");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->x_pixels_per_unit = res_x;
   info_ptr->y_pixels_per_unit = res_y;
   info_ptr->phys_unit_type = (png_byte)unit_type;
   info_ptr->valid |= PNG_INFO_pHYs;
}
#endif

void PNGAPI
png_set_PLTE(png_structrp png_ptr, png_inforp info_ptr,
    png_const_colorp palette, int num_palette)
{

   png_debug1(1, "in %s storage function", "PLTE");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   if (num_palette < 0 || num_palette > PNG_MAX_PALETTE_LENGTH)
   {
      if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
         png_error(png_ptr, "Invalid palette length");

      else
      {
         png_warning(png_ptr, "Invalid palette length");
         return;
      }
   }

   if ((num_palette > 0 && palette == NULL) ||
      (num_palette == 0
#        ifdef PNG_MNG_FEATURES_SUPPORTED
            && (png_ptr->mng_features_permitted & PNG_FLAG_MNG_EMPTY_PLTE) == 0
#        endif
      ))
   {
      png_error(png_ptr, "Invalid palette");
      return;
   }

   






   png_free_data(png_ptr, info_ptr, PNG_FREE_PLTE, 0);

   



   png_ptr->palette = png_voidcast(png_colorp, png_calloc(png_ptr,
       PNG_MAX_PALETTE_LENGTH * (sizeof (png_color))));

   if (num_palette > 0)
      memcpy(png_ptr->palette, palette, num_palette * (sizeof (png_color)));
   info_ptr->palette = png_ptr->palette;
   info_ptr->num_palette = png_ptr->num_palette = (png_uint_16)num_palette;

   info_ptr->free_me |= PNG_FREE_PLTE;

   info_ptr->valid |= PNG_INFO_PLTE;
}

#ifdef PNG_sBIT_SUPPORTED
void PNGAPI
png_set_sBIT(png_const_structrp png_ptr, png_inforp info_ptr,
    png_const_color_8p sig_bit)
{
   png_debug1(1, "in %s storage function", "sBIT");

   if (png_ptr == NULL || info_ptr == NULL || sig_bit == NULL)
      return;

   info_ptr->sig_bit = *sig_bit;
   info_ptr->valid |= PNG_INFO_sBIT;
}
#endif

#ifdef PNG_sRGB_SUPPORTED
void PNGAPI
png_set_sRGB(png_const_structrp png_ptr, png_inforp info_ptr, int srgb_intent)
{
   png_debug1(1, "in %s storage function", "sRGB");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   (void)png_colorspace_set_sRGB(png_ptr, &info_ptr->colorspace, srgb_intent);
   png_colorspace_sync_info(png_ptr, info_ptr);
}

void PNGAPI
png_set_sRGB_gAMA_and_cHRM(png_const_structrp png_ptr, png_inforp info_ptr,
    int srgb_intent)
{
   png_debug1(1, "in %s storage function", "sRGB_gAMA_and_cHRM");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   if (png_colorspace_set_sRGB(png_ptr, &info_ptr->colorspace,
       srgb_intent) != 0)
   {
      
      info_ptr->colorspace.flags |=
         PNG_COLORSPACE_FROM_gAMA|PNG_COLORSPACE_FROM_cHRM;
   }

   png_colorspace_sync_info(png_ptr, info_ptr);
}
#endif 


#ifdef PNG_iCCP_SUPPORTED
void PNGAPI
png_set_iCCP(png_const_structrp png_ptr, png_inforp info_ptr,
    png_const_charp name, int compression_type,
    png_const_bytep profile, png_uint_32 proflen)
{
   png_charp new_iccp_name;
   png_bytep new_iccp_profile;
   png_size_t length;

   png_debug1(1, "in %s storage function", "iCCP");

   if (png_ptr == NULL || info_ptr == NULL || name == NULL || profile == NULL)
      return;

   if (compression_type != PNG_COMPRESSION_TYPE_BASE)
      png_app_error(png_ptr, "Invalid iCCP compression method");

   





   {
      int result = png_colorspace_set_ICC(png_ptr, &info_ptr->colorspace, name,
         proflen, profile, info_ptr->color_type);

      png_colorspace_sync_info(png_ptr, info_ptr);

      
      if (result == 0)
         return;

      
      info_ptr->colorspace.flags |=
         PNG_COLORSPACE_FROM_gAMA|PNG_COLORSPACE_FROM_cHRM;
   }

   length = strlen(name)+1;
   new_iccp_name = png_voidcast(png_charp, png_malloc_warn(png_ptr, length));

   if (new_iccp_name == NULL)
   {
      png_benign_error(png_ptr, "Insufficient memory to process iCCP chunk");
      return;
   }

   memcpy(new_iccp_name, name, length);
   new_iccp_profile = png_voidcast(png_bytep,
      png_malloc_warn(png_ptr, proflen));

   if (new_iccp_profile == NULL)
   {
      png_free(png_ptr, new_iccp_name);
      new_iccp_name = NULL;
      png_benign_error(png_ptr,
          "Insufficient memory to process iCCP profile");
      return;
   }

   memcpy(new_iccp_profile, profile, proflen);

   png_free_data(png_ptr, info_ptr, PNG_FREE_ICCP, 0);

   info_ptr->iccp_proflen = proflen;
   info_ptr->iccp_name = new_iccp_name;
   info_ptr->iccp_profile = new_iccp_profile;
   info_ptr->free_me |= PNG_FREE_ICCP;
   info_ptr->valid |= PNG_INFO_iCCP;
}
#endif

#ifdef PNG_TEXT_SUPPORTED
void PNGAPI
png_set_text(png_const_structrp png_ptr, png_inforp info_ptr,
    png_const_textp text_ptr, int num_text)
{
   int ret;
   ret = png_set_text_2(png_ptr, info_ptr, text_ptr, num_text);

   if (ret != 0)
      png_error(png_ptr, "Insufficient memory to store text");
}

int 
png_set_text_2(png_const_structrp png_ptr, png_inforp info_ptr,
    png_const_textp text_ptr, int num_text)
{
   int i;

   png_debug1(1, "in %lx storage function", png_ptr == NULL ? "unexpected" :
      (unsigned long)png_ptr->chunk_name);

   if (png_ptr == NULL || info_ptr == NULL || num_text <= 0 || text_ptr == NULL)
      return(0);

   




   if (num_text > info_ptr->max_text - info_ptr->num_text)
   {
      int old_num_text = info_ptr->num_text;
      int max_text;
      png_textp new_text = NULL;

      
      max_text = old_num_text;
      if (num_text <= INT_MAX - max_text)
      {
         max_text += num_text;

         
         if (max_text < INT_MAX-8)
            max_text = (max_text + 8) & ~0x7;

         else
            max_text = INT_MAX;

         


         new_text = png_voidcast(png_textp,png_realloc_array(png_ptr,
            info_ptr->text, old_num_text, max_text-old_num_text,
            sizeof *new_text));
      }

      if (new_text == NULL)
      {
         png_chunk_report(png_ptr, "too many text chunks",
            PNG_CHUNK_WRITE_ERROR);
         return 1;
      }

      png_free(png_ptr, info_ptr->text);

      info_ptr->text = new_text;
      info_ptr->free_me |= PNG_FREE_TEXT;
      info_ptr->max_text = max_text;
      

      png_debug1(3, "allocated %d entries for info_ptr->text", max_text);
   }

   for (i = 0; i < num_text; i++)
   {
      size_t text_length, key_len;
      size_t lang_len, lang_key_len;
      png_textp textp = &(info_ptr->text[info_ptr->num_text]);

      if (text_ptr[i].key == NULL)
          continue;

      if (text_ptr[i].compression < PNG_TEXT_COMPRESSION_NONE ||
          text_ptr[i].compression >= PNG_TEXT_COMPRESSION_LAST)
      {
         png_chunk_report(png_ptr, "text compression mode is out of range",
            PNG_CHUNK_WRITE_ERROR);
         continue;
      }

      key_len = strlen(text_ptr[i].key);

      if (text_ptr[i].compression <= 0)
      {
         lang_len = 0;
         lang_key_len = 0;
      }

      else
#  ifdef PNG_iTXt_SUPPORTED
      {
         

         if (text_ptr[i].lang != NULL)
            lang_len = strlen(text_ptr[i].lang);

         else
            lang_len = 0;

         if (text_ptr[i].lang_key != NULL)
            lang_key_len = strlen(text_ptr[i].lang_key);

         else
            lang_key_len = 0;
      }
#  else 
      {
         png_chunk_report(png_ptr, "iTXt chunk not supported",
            PNG_CHUNK_WRITE_ERROR);
         continue;
      }
#  endif

      if (text_ptr[i].text == NULL || text_ptr[i].text[0] == '\0')
      {
         text_length = 0;
#  ifdef PNG_iTXt_SUPPORTED
         if (text_ptr[i].compression > 0)
            textp->compression = PNG_ITXT_COMPRESSION_NONE;

         else
#  endif
            textp->compression = PNG_TEXT_COMPRESSION_NONE;
      }

      else
      {
         text_length = strlen(text_ptr[i].text);
         textp->compression = text_ptr[i].compression;
      }

      textp->key = png_voidcast(png_charp,png_malloc_base(png_ptr,
          key_len + text_length + lang_len + lang_key_len + 4));

      if (textp->key == NULL)
      {
         png_chunk_report(png_ptr, "text chunk: out of memory",
               PNG_CHUNK_WRITE_ERROR);
         return 1;
      }

      png_debug2(2, "Allocated %lu bytes at %p in png_set_text",
          (unsigned long)(png_uint_32)
          (key_len + lang_len + lang_key_len + text_length + 4),
          textp->key);

      memcpy(textp->key, text_ptr[i].key, key_len);
      *(textp->key + key_len) = '\0';

      if (text_ptr[i].compression > 0)
      {
         textp->lang = textp->key + key_len + 1;
         memcpy(textp->lang, text_ptr[i].lang, lang_len);
         *(textp->lang + lang_len) = '\0';
         textp->lang_key = textp->lang + lang_len + 1;
         memcpy(textp->lang_key, text_ptr[i].lang_key, lang_key_len);
         *(textp->lang_key + lang_key_len) = '\0';
         textp->text = textp->lang_key + lang_key_len + 1;
      }

      else
      {
         textp->lang=NULL;
         textp->lang_key=NULL;
         textp->text = textp->key + key_len + 1;
      }

      if (text_length != 0)
         memcpy(textp->text, text_ptr[i].text, text_length);

      *(textp->text + text_length) = '\0';

#  ifdef PNG_iTXt_SUPPORTED
      if (textp->compression > 0)
      {
         textp->text_length = 0;
         textp->itxt_length = text_length;
      }

      else
#  endif
      {
         textp->text_length = text_length;
         textp->itxt_length = 0;
      }

      info_ptr->num_text++;
      png_debug1(3, "transferred text chunk %d", info_ptr->num_text);
   }

   return(0);
}
#endif

#ifdef PNG_tIME_SUPPORTED
void PNGAPI
png_set_tIME(png_const_structrp png_ptr, png_inforp info_ptr,
    png_const_timep mod_time)
{
   png_debug1(1, "in %s storage function", "tIME");

   if (png_ptr == NULL || info_ptr == NULL || mod_time == NULL ||
       (png_ptr->mode & PNG_WROTE_tIME) != 0)
      return;

   if (mod_time->month == 0   || mod_time->month > 12  ||
       mod_time->day   == 0   || mod_time->day   > 31  ||
       mod_time->hour  > 23   || mod_time->minute > 59 ||
       mod_time->second > 60)
   {
      png_warning(png_ptr, "Ignoring invalid time value");
      return;
   }

   info_ptr->mod_time = *mod_time;
   info_ptr->valid |= PNG_INFO_tIME;
}
#endif

#ifdef PNG_tRNS_SUPPORTED
void PNGAPI
png_set_tRNS(png_structrp png_ptr, png_inforp info_ptr,
    png_const_bytep trans_alpha, int num_trans, png_const_color_16p trans_color)
{
   png_debug1(1, "in %s storage function", "tRNS");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   if (trans_alpha != NULL)
   {
       








       png_free_data(png_ptr, info_ptr, PNG_FREE_TRNS, 0);

       
       png_ptr->trans_alpha = info_ptr->trans_alpha = png_voidcast(png_bytep,
         png_malloc(png_ptr, PNG_MAX_PALETTE_LENGTH));

       if (num_trans > 0 && num_trans <= PNG_MAX_PALETTE_LENGTH)
          memcpy(info_ptr->trans_alpha, trans_alpha, (png_size_t)num_trans);
   }

   if (trans_color != NULL)
   {
      int sample_max = (1 << info_ptr->bit_depth);

      if ((info_ptr->color_type == PNG_COLOR_TYPE_GRAY &&
          trans_color->gray > sample_max) ||
          (info_ptr->color_type == PNG_COLOR_TYPE_RGB &&
          (trans_color->red > sample_max ||
          trans_color->green > sample_max ||
          trans_color->blue > sample_max)))
         png_warning(png_ptr,
            "tRNS chunk has out-of-range samples for bit_depth");

      info_ptr->trans_color = *trans_color;

      if (num_trans == 0)
         num_trans = 1;
   }

   info_ptr->num_trans = (png_uint_16)num_trans;

   if (num_trans != 0)
   {
      info_ptr->valid |= PNG_INFO_tRNS;
      info_ptr->free_me |= PNG_FREE_TRNS;
   }
}
#endif

#ifdef PNG_sPLT_SUPPORTED
void PNGAPI
png_set_sPLT(png_const_structrp png_ptr,
    png_inforp info_ptr, png_const_sPLT_tp entries, int nentries)








{
   png_sPLT_tp np;

   if (png_ptr == NULL || info_ptr == NULL || nentries <= 0 || entries == NULL)
      return;

   


   np = png_voidcast(png_sPLT_tp,png_realloc_array(png_ptr,
      info_ptr->splt_palettes, info_ptr->splt_palettes_num, nentries,
      sizeof *np));

   if (np == NULL)
   {
      
      png_chunk_report(png_ptr, "too many sPLT chunks", PNG_CHUNK_WRITE_ERROR);
      return;
   }

   png_free(png_ptr, info_ptr->splt_palettes);
   info_ptr->splt_palettes = np;
   info_ptr->free_me |= PNG_FREE_SPLT;

   np += info_ptr->splt_palettes_num;

   do
   {
      png_size_t length;

      
      if (entries->name == NULL || entries->entries == NULL)
      {
         
         png_app_error(png_ptr, "png_set_sPLT: invalid sPLT");
         
         continue;
      }

      np->depth = entries->depth;

      


      length = strlen(entries->name) + 1;
      np->name = png_voidcast(png_charp, png_malloc_base(png_ptr, length));

      if (np->name == NULL)
         break;

      memcpy(np->name, entries->name, length);

      



      np->entries = png_voidcast(png_sPLT_entryp, png_malloc_array(png_ptr,
          entries->nentries, sizeof (png_sPLT_entry)));

      if (np->entries == NULL)
      {
         png_free(png_ptr, np->name);
         np->name = NULL;
         break;
      }

      np->nentries = entries->nentries;
      


      memcpy(np->entries, entries->entries,
         entries->nentries * sizeof (png_sPLT_entry));

      


      info_ptr->valid |= PNG_INFO_sPLT;
      ++(info_ptr->splt_palettes_num);
      ++np;
   }
   while (++entries, --nentries);

   if (nentries > 0)
      png_chunk_report(png_ptr, "sPLT out of memory", PNG_CHUNK_WRITE_ERROR);
}
#endif 

#ifdef PNG_APNG_SUPPORTED
png_uint_32 PNGAPI
png_set_acTL(png_structp png_ptr, png_infop info_ptr,
    png_uint_32 num_frames, png_uint_32 num_plays)
{
    png_debug1(1, "in %s storage function", "acTL");

    if (png_ptr == NULL || info_ptr == NULL)
    {
        png_warning(png_ptr,
                    "Call to png_set_acTL() with NULL png_ptr "
                    "or info_ptr ignored");
        return (0);
    }
    if (num_frames == 0)
    {
        png_warning(png_ptr,
                    "Ignoring attempt to set acTL with num_frames zero");
        return (0);
    }
    if (num_frames > PNG_UINT_31_MAX)
    {
        png_warning(png_ptr,
                    "Ignoring attempt to set acTL with num_frames > 2^31-1");
        return (0);
    }
    if (num_plays > PNG_UINT_31_MAX)
    {
        png_warning(png_ptr,
                    "Ignoring attempt to set acTL with num_plays "
                    "> 2^31-1");
        return (0);
    }

    info_ptr->num_frames = num_frames;
    info_ptr->num_plays = num_plays;

    info_ptr->valid |= PNG_INFO_acTL;

    return (1);
}


png_uint_32 PNGAPI
png_set_next_frame_fcTL(png_structp png_ptr, png_infop info_ptr,
    png_uint_32 width, png_uint_32 height,
    png_uint_32 x_offset, png_uint_32 y_offset,
    png_uint_16 delay_num, png_uint_16 delay_den,
    png_byte dispose_op, png_byte blend_op)
{
    png_debug1(1, "in %s storage function", "fcTL");

    if (png_ptr == NULL || info_ptr == NULL)
    {
        png_warning(png_ptr,
                    "Call to png_set_fcTL() with NULL png_ptr or info_ptr "
                    "ignored");
        return (0);
    }

    png_ensure_fcTL_is_valid(png_ptr, width, height, x_offset, y_offset,
                             delay_num, delay_den, dispose_op, blend_op);

    if (blend_op == PNG_BLEND_OP_OVER)
    {
        if ((png_ptr->color_type & PNG_COLOR_MASK_ALPHA) == 0 &&
            png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) == 0)
        {
          png_warning(png_ptr, "PNG_BLEND_OP_OVER is meaningless "
                               "and wasteful for opaque images, ignored");
          blend_op = PNG_BLEND_OP_SOURCE;
        }
    }

    info_ptr->next_frame_width = width;
    info_ptr->next_frame_height = height;
    info_ptr->next_frame_x_offset = x_offset;
    info_ptr->next_frame_y_offset = y_offset;
    info_ptr->next_frame_delay_num = delay_num;
    info_ptr->next_frame_delay_den = delay_den;
    info_ptr->next_frame_dispose_op = dispose_op;
    info_ptr->next_frame_blend_op = blend_op;

    info_ptr->valid |= PNG_INFO_fcTL;

    return (1);
}

void 
png_ensure_fcTL_is_valid(png_structp png_ptr,
    png_uint_32 width, png_uint_32 height,
    png_uint_32 x_offset, png_uint_32 y_offset,
    png_uint_16 delay_num, png_uint_16 delay_den,
    png_byte dispose_op, png_byte blend_op)
{
    if (width > PNG_UINT_31_MAX)
        png_error(png_ptr, "invalid width in fcTL (> 2^31-1)");
    if (height > PNG_UINT_31_MAX)
        png_error(png_ptr, "invalid height in fcTL (> 2^31-1)");
    if (x_offset > PNG_UINT_31_MAX)
        png_error(png_ptr, "invalid x_offset in fcTL (> 2^31-1)");
    if (y_offset > PNG_UINT_31_MAX)
        png_error(png_ptr, "invalid y_offset in fcTL (> 2^31-1)");
    if (width + x_offset > png_ptr->first_frame_width ||
        height + y_offset > png_ptr->first_frame_height)
        png_error(png_ptr, "dimensions of a frame are greater than"
                           "the ones in IHDR");

    if (dispose_op != PNG_DISPOSE_OP_NONE &&
        dispose_op != PNG_DISPOSE_OP_BACKGROUND &&
        dispose_op != PNG_DISPOSE_OP_PREVIOUS)
        png_error(png_ptr, "invalid dispose_op in fcTL");

    if (blend_op != PNG_BLEND_OP_SOURCE &&
        blend_op != PNG_BLEND_OP_OVER)
        png_error(png_ptr, "invalid blend_op in fcTL");

    PNG_UNUSED(delay_num)
    PNG_UNUSED(delay_den)
}

png_uint_32 PNGAPI
png_set_first_frame_is_hidden(png_structp png_ptr, png_infop info_ptr,
                              png_byte is_hidden)
{
    png_debug(1, "in png_first_frame_is_hidden()");

    if (png_ptr == NULL)
        return 0;

    if (is_hidden != 0)
        png_ptr->apng_flags |= PNG_FIRST_FRAME_HIDDEN;
    else
        png_ptr->apng_flags &= ~PNG_FIRST_FRAME_HIDDEN;

    PNG_UNUSED(info_ptr)

    return 1;
}
#endif 

#ifdef PNG_STORE_UNKNOWN_CHUNKS_SUPPORTED
static png_byte
check_location(png_const_structrp png_ptr, int location)
{
   location &= (PNG_HAVE_IHDR|PNG_HAVE_PLTE|PNG_AFTER_IDAT);

   



   if (location == 0 && (png_ptr->mode & PNG_IS_READ_STRUCT) == 0)
   {
      
      png_app_warning(png_ptr,
         "png_set_unknown_chunks now expects a valid location");
      
      location = (png_byte)(png_ptr->mode &
         (PNG_HAVE_IHDR|PNG_HAVE_PLTE|PNG_AFTER_IDAT));
   }

   


   if (location == 0)
      png_error(png_ptr, "invalid location in png_set_unknown_chunks");

   


   while (location != (location & -location))
      location &= ~(location & -location);

   


   return (png_byte)location;
}

void PNGAPI
png_set_unknown_chunks(png_const_structrp png_ptr,
   png_inforp info_ptr, png_const_unknown_chunkp unknowns, int num_unknowns)
{
   png_unknown_chunkp np;

   if (png_ptr == NULL || info_ptr == NULL || num_unknowns <= 0 ||
      unknowns == NULL)
      return;

   





#  if !defined(PNG_READ_UNKNOWN_CHUNKS_SUPPORTED) && \
      defined(PNG_READ_SUPPORTED)
      if ((png_ptr->mode & PNG_IS_READ_STRUCT) != 0)
      {
         png_app_error(png_ptr, "no unknown chunk support on read");
         return;
      }
#  endif
#  if !defined(PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED) && \
      defined(PNG_WRITE_SUPPORTED)
      if ((png_ptr->mode & PNG_IS_READ_STRUCT) == 0)
      {
         png_app_error(png_ptr, "no unknown chunk support on write");
         return;
      }
#  endif

   




   np = png_voidcast(png_unknown_chunkp, png_realloc_array(png_ptr,
         info_ptr->unknown_chunks, info_ptr->unknown_chunks_num, num_unknowns,
         sizeof *np));

   if (np == NULL)
   {
      png_chunk_report(png_ptr, "too many unknown chunks",
         PNG_CHUNK_WRITE_ERROR);
      return;
   }

   png_free(png_ptr, info_ptr->unknown_chunks);
   info_ptr->unknown_chunks = np; 
   info_ptr->free_me |= PNG_FREE_UNKN;

   np += info_ptr->unknown_chunks_num;

   


   for (; num_unknowns > 0; --num_unknowns, ++unknowns)
   {
      memcpy(np->name, unknowns->name, (sizeof np->name));
      np->name[(sizeof np->name)-1] = '\0';
      np->location = check_location(png_ptr, unknowns->location);

      if (unknowns->size == 0)
      {
         np->data = NULL;
         np->size = 0;
      }

      else
      {
         np->data = png_voidcast(png_bytep,
            png_malloc_base(png_ptr, unknowns->size));

         if (np->data == NULL)
         {
            png_chunk_report(png_ptr, "unknown chunk: out of memory",
               PNG_CHUNK_WRITE_ERROR);
            
            continue;
         }

         memcpy(np->data, unknowns->data, unknowns->size);
         np->size = unknowns->size;
      }

      



      ++np;
      ++(info_ptr->unknown_chunks_num);
   }
}

void PNGAPI
png_set_unknown_chunk_location(png_const_structrp png_ptr, png_inforp info_ptr,
    int chunk, int location)
{
   




   if (png_ptr != NULL && info_ptr != NULL && chunk >= 0 &&
      chunk < info_ptr->unknown_chunks_num)
   {
      if ((location & (PNG_HAVE_IHDR|PNG_HAVE_PLTE|PNG_AFTER_IDAT)) == 0)
      {
         png_app_error(png_ptr, "invalid unknown chunk location");
         
         if ((location & PNG_HAVE_IDAT) != 0) 
            location = PNG_AFTER_IDAT;

         else
            location = PNG_HAVE_IHDR; 
      }

      info_ptr->unknown_chunks[chunk].location =
         check_location(png_ptr, location);
   }
}
#endif


#ifdef PNG_MNG_FEATURES_SUPPORTED
png_uint_32 PNGAPI
png_permit_mng_features (png_structrp png_ptr, png_uint_32 mng_features)
{
   png_debug(1, "in png_permit_mng_features");

   if (png_ptr == NULL)
      return 0;

   png_ptr->mng_features_permitted = mng_features & PNG_ALL_MNG_FEATURES;

   return png_ptr->mng_features_permitted;
}
#endif

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
static unsigned int
add_one_chunk(png_bytep list, unsigned int count, png_const_bytep add, int keep)
{
   unsigned int i;

   


   for (i=0; i<count; ++i, list += 5)
   {
      if (memcmp(list, add, 4) == 0)
      {
         list[4] = (png_byte)keep;
         return count;
      }
   }

   if (keep != PNG_HANDLE_CHUNK_AS_DEFAULT)
   {
      ++count;
      memcpy(list, add, 4);
      list[4] = (png_byte)keep;
   }

   return count;
}

void PNGAPI
png_set_keep_unknown_chunks(png_structrp png_ptr, int keep,
    png_const_bytep chunk_list, int num_chunks_in)
{
   png_bytep new_list;
   unsigned int num_chunks, old_num_chunks;

   if (png_ptr == NULL)
      return;

   if (keep < 0 || keep >= PNG_HANDLE_CHUNK_LAST)
   {
      png_app_error(png_ptr, "png_set_keep_unknown_chunks: invalid keep");
      return;
   }

   if (num_chunks_in <= 0)
   {
      png_ptr->unknown_default = keep;

      
      if (num_chunks_in == 0)
        return;
   }

   if (num_chunks_in < 0)
   {
      


      static PNG_CONST png_byte chunks_to_ignore[] = {
         98,  75,  71,  68, '\0',  
         99,  72,  82,  77, '\0',  
        103,  65,  77,  65, '\0',  
        104,  73,  83,  84, '\0',  
        105,  67,  67,  80, '\0',  
        105,  84,  88, 116, '\0',  
        111,  70,  70, 115, '\0',  
        112,  67,  65,  76, '\0',  
        112,  72,  89, 115, '\0',  
        115,  66,  73,  84, '\0',  
        115,  67,  65,  76, '\0',  
        115,  80,  76,  84, '\0',  
        115,  84,  69,  82, '\0',  
        115,  82,  71,  66, '\0',  
        116,  69,  88, 116, '\0',  
        116,  73,  77,  69, '\0',  
        122,  84,  88, 116, '\0'   
      };

      chunk_list = chunks_to_ignore;
      num_chunks = (unsigned int)(sizeof chunks_to_ignore)/5U;
   }

   else 
   {
      if (chunk_list == NULL)
      {
         


         png_app_error(png_ptr, "png_set_keep_unknown_chunks: no chunk list");
         return;
      }

      num_chunks = num_chunks_in;
   }

   old_num_chunks = png_ptr->num_chunk_list;
   if (png_ptr->chunk_list == NULL)
      old_num_chunks = 0;

   

   if (num_chunks + old_num_chunks > UINT_MAX/5)
   {
      png_app_error(png_ptr, "png_set_keep_unknown_chunks: too many chunks");
      return;
   }

   



   if (keep != 0)
   {
      new_list = png_voidcast(png_bytep, png_malloc(png_ptr,
          5 * (num_chunks + old_num_chunks)));

      if (old_num_chunks > 0)
         memcpy(new_list, png_ptr->chunk_list, 5*old_num_chunks);
   }

   else if (old_num_chunks > 0)
      new_list = png_ptr->chunk_list;

   else
      new_list = NULL;

   




   if (new_list != NULL)
   {
      png_const_bytep inlist;
      png_bytep outlist;
      unsigned int i;

      for (i=0; i<num_chunks; ++i)
      {
         old_num_chunks = add_one_chunk(new_list, old_num_chunks,
            chunk_list+5*i, keep);
      }

      
      num_chunks = 0;
      for (i=0, inlist=outlist=new_list; i<old_num_chunks; ++i, inlist += 5)
      {
         if (inlist[4])
         {
            if (outlist != inlist)
               memcpy(outlist, inlist, 5);
            outlist += 5;
            ++num_chunks;
         }
      }

      
      if (num_chunks == 0)
      {
         if (png_ptr->chunk_list != new_list)
            png_free(png_ptr, new_list);

         new_list = NULL;
      }
   }

   else
      num_chunks = 0;

   png_ptr->num_chunk_list = num_chunks;

   if (png_ptr->chunk_list != new_list)
   {
      if (png_ptr->chunk_list != NULL)
         png_free(png_ptr, png_ptr->chunk_list);

      png_ptr->chunk_list = new_list;
   }
}
#endif

#ifdef PNG_READ_USER_CHUNKS_SUPPORTED
void PNGAPI
png_set_read_user_chunk_fn(png_structrp png_ptr, png_voidp user_chunk_ptr,
    png_user_chunk_ptr read_user_chunk_fn)
{
   png_debug(1, "in png_set_read_user_chunk_fn");

   if (png_ptr == NULL)
      return;

   png_ptr->read_user_chunk_fn = read_user_chunk_fn;
   png_ptr->user_chunk_ptr = user_chunk_ptr;
}
#endif

#ifdef PNG_INFO_IMAGE_SUPPORTED
void PNGAPI
png_set_rows(png_const_structrp png_ptr, png_inforp info_ptr,
    png_bytepp row_pointers)
{
   png_debug1(1, "in %s storage function", "rows");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   if (info_ptr->row_pointers != NULL &&
       (info_ptr->row_pointers != row_pointers))
      png_free_data(png_ptr, info_ptr, PNG_FREE_ROWS, 0);

   info_ptr->row_pointers = row_pointers;

   if (row_pointers != NULL)
      info_ptr->valid |= PNG_INFO_IDAT;
}
#endif

void PNGAPI
png_set_compression_buffer_size(png_structrp png_ptr, png_size_t size)
{
    if (png_ptr == NULL)
       return;

    if (size == 0 || size > PNG_UINT_31_MAX)
       png_error(png_ptr, "invalid compression buffer size");

#  ifdef PNG_SEQUENTIAL_READ_SUPPORTED
      if ((png_ptr->mode & PNG_IS_READ_STRUCT) != 0)
      {
         png_ptr->IDAT_read_size = (png_uint_32)size; 
         return;
      }
#  endif

#  ifdef PNG_WRITE_SUPPORTED
      if ((png_ptr->mode & PNG_IS_READ_STRUCT) == 0)
      {
         if (png_ptr->zowner != 0)
         {
            png_warning(png_ptr,
              "Compression buffer size cannot be changed because it is in use");
            return;
         }

         if (size > ZLIB_IO_MAX)
         {
            png_warning(png_ptr,
               "Compression buffer size limited to system maximum");
            size = ZLIB_IO_MAX; 
         }

         else if (size < 6)
         {
            


            png_warning(png_ptr,
               "Compression buffer size cannot be reduced below 6");
            return;
         }

         if (png_ptr->zbuffer_size != size)
         {
            png_free_buffer_list(png_ptr, &png_ptr->zbuffer_list);
            png_ptr->zbuffer_size = (uInt)size;
         }
      }
#  endif
}

void PNGAPI
png_set_invalid(png_const_structrp png_ptr, png_inforp info_ptr, int mask)
{
   if (png_ptr != NULL && info_ptr != NULL)
      info_ptr->valid &= ~mask;
}


#ifdef PNG_SET_USER_LIMITS_SUPPORTED

void PNGAPI
png_set_user_limits (png_structrp png_ptr, png_uint_32 user_width_max,
    png_uint_32 user_height_max)
{
   



   if (png_ptr == NULL)
      return;

   png_ptr->user_width_max = user_width_max;
   png_ptr->user_height_max = user_height_max;
}


void PNGAPI
png_set_chunk_cache_max (png_structrp png_ptr, png_uint_32 user_chunk_cache_max)
{
    if (png_ptr != NULL)
       png_ptr->user_chunk_cache_max = user_chunk_cache_max;
}


void PNGAPI
png_set_chunk_malloc_max (png_structrp png_ptr,
    png_alloc_size_t user_chunk_malloc_max)
{
   if (png_ptr != NULL)
      png_ptr->user_chunk_malloc_max = user_chunk_malloc_max;
}
#endif 


#ifdef PNG_BENIGN_ERRORS_SUPPORTED
void PNGAPI
png_set_benign_errors(png_structrp png_ptr, int allowed)
{
   png_debug(1, "in png_set_benign_errors");

   





   if (allowed != 0)
      png_ptr->flags |= PNG_FLAG_BENIGN_ERRORS_WARN |
         PNG_FLAG_APP_WARNINGS_WARN | PNG_FLAG_APP_ERRORS_WARN;

   else
      png_ptr->flags &= ~(PNG_FLAG_BENIGN_ERRORS_WARN |
         PNG_FLAG_APP_WARNINGS_WARN | PNG_FLAG_APP_ERRORS_WARN);
}
#endif 

#ifdef PNG_CHECK_FOR_INVALID_INDEX_SUPPORTED
   







void PNGAPI
png_set_check_for_invalid_index(png_structrp png_ptr, int allowed)
{
   png_debug(1, "in png_set_check_for_invalid_index");

   if (allowed > 0)
      png_ptr->num_palette_max = 0;

   else
      png_ptr->num_palette_max = -1;
}
#endif
#endif 

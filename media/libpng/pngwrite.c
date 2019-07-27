












#include "pngpriv.h"
#if defined(PNG_SIMPLIFIED_WRITE_SUPPORTED) && defined(PNG_STDIO_SUPPORTED)
#  include <errno.h>
#endif

#ifdef PNG_WRITE_SUPPORTED

#ifdef PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED

static void
write_unknown_chunks(png_structrp png_ptr, png_const_inforp info_ptr,
   unsigned int where)
{
   if (info_ptr->unknown_chunks_num != 0)
   {
      png_const_unknown_chunkp up;

      png_debug(5, "writing extra chunks");

      for (up = info_ptr->unknown_chunks;
           up < info_ptr->unknown_chunks + info_ptr->unknown_chunks_num;
           ++up)
         if ((up->location & where) != 0)
      {
         


#ifdef PNG_SET_UNKNOWN_CHUNKS_SUPPORTED
         int keep = png_handle_as_unknown(png_ptr, up->name);

         











         if (keep != PNG_HANDLE_CHUNK_NEVER &&
             ((up->name[3] & 0x20)  ||
              keep == PNG_HANDLE_CHUNK_ALWAYS ||
              (keep == PNG_HANDLE_CHUNK_AS_DEFAULT &&
               png_ptr->unknown_default == PNG_HANDLE_CHUNK_ALWAYS)))
#endif
         {
            
            if (up->size == 0)
               png_warning(png_ptr, "Writing zero-length unknown chunk");

            png_write_chunk(png_ptr, up->name, up->data, up->size);
         }
      }
   }
}
#endif 










void PNGAPI
png_write_info_before_PLTE(png_structrp png_ptr, png_const_inforp info_ptr)
{
   png_debug(1, "in png_write_info_before_PLTE");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   if ((png_ptr->mode & PNG_WROTE_INFO_BEFORE_PLTE) == 0)
   {
   
   png_write_sig(png_ptr);

#ifdef PNG_MNG_FEATURES_SUPPORTED
   if ((png_ptr->mode & PNG_HAVE_PNG_SIGNATURE) != 0 && \
       png_ptr->mng_features_permitted != 0)
   {
      png_warning(png_ptr, "MNG features are not allowed in a PNG datastream");
      png_ptr->mng_features_permitted = 0;
   }
#endif

   
   png_write_IHDR(png_ptr, info_ptr->width, info_ptr->height,
       info_ptr->bit_depth, info_ptr->color_type, info_ptr->compression_type,
       info_ptr->filter_type,
#ifdef PNG_WRITE_INTERLACING_SUPPORTED
       info_ptr->interlace_type
#else
       0
#endif
      );

   













#ifdef PNG_WRITE_APNG_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_acTL) != 0)
      png_write_acTL(png_ptr, info_ptr->num_frames, info_ptr->num_plays);
#endif
#ifdef PNG_GAMMA_SUPPORTED
#  ifdef PNG_WRITE_gAMA_SUPPORTED
      if ((info_ptr->colorspace.flags & PNG_COLORSPACE_INVALID) == 0 &&
          (info_ptr->colorspace.flags & PNG_COLORSPACE_FROM_gAMA) != 0 &&
          (info_ptr->valid & PNG_INFO_gAMA) != 0)
         png_write_gAMA_fixed(png_ptr, info_ptr->colorspace.gamma);
#  endif
#endif

#ifdef PNG_COLORSPACE_SUPPORTED
   


#  ifdef PNG_WRITE_iCCP_SUPPORTED
      if ((info_ptr->colorspace.flags & PNG_COLORSPACE_INVALID) == 0 &&
          (info_ptr->valid & PNG_INFO_iCCP) != 0)
      {
#        ifdef PNG_WRITE_sRGB_SUPPORTED
            if ((info_ptr->valid & PNG_INFO_sRGB) != 0)
               png_app_warning(png_ptr,
                  "profile matches sRGB but writing iCCP instead");
#        endif

         png_write_iCCP(png_ptr, info_ptr->iccp_name,
            info_ptr->iccp_profile);
      }
#     ifdef PNG_WRITE_sRGB_SUPPORTED
         else
#     endif
#  endif

#  ifdef PNG_WRITE_sRGB_SUPPORTED
      if ((info_ptr->colorspace.flags & PNG_COLORSPACE_INVALID) == 0 &&
          (info_ptr->valid & PNG_INFO_sRGB) != 0)
         png_write_sRGB(png_ptr, info_ptr->colorspace.rendering_intent);
#  endif 
#endif 

#ifdef PNG_WRITE_sBIT_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_sBIT) != 0)
      png_write_sBIT(png_ptr, &(info_ptr->sig_bit), info_ptr->color_type);
#endif

#ifdef PNG_COLORSPACE_SUPPORTED
#  ifdef PNG_WRITE_cHRM_SUPPORTED
      if ((info_ptr->colorspace.flags & PNG_COLORSPACE_INVALID) == 0 &&
         (info_ptr->colorspace.flags & PNG_COLORSPACE_FROM_cHRM) != 0 &&
         (info_ptr->valid & PNG_INFO_cHRM) != 0)
         png_write_cHRM_fixed(png_ptr, &info_ptr->colorspace.end_points_xy);
#  endif
#endif

#ifdef PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
      write_unknown_chunks(png_ptr, info_ptr, PNG_HAVE_IHDR);
#endif

      png_ptr->mode |= PNG_WROTE_INFO_BEFORE_PLTE;
   }
}

void PNGAPI
png_write_info(png_structrp png_ptr, png_const_inforp info_ptr)
{
#if defined(PNG_WRITE_TEXT_SUPPORTED) || defined(PNG_WRITE_sPLT_SUPPORTED)
   int i;
#endif

   png_debug(1, "in png_write_info");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   png_write_info_before_PLTE(png_ptr, info_ptr);

   if ((info_ptr->valid & PNG_INFO_PLTE) != 0)
      png_write_PLTE(png_ptr, info_ptr->palette,
          (png_uint_32)info_ptr->num_palette);

   else if ((info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) !=0)
      png_error(png_ptr, "Valid palette required for paletted images");

#ifdef PNG_WRITE_tRNS_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_tRNS) !=0)
   {
#ifdef PNG_WRITE_INVERT_ALPHA_SUPPORTED
      
      if ((png_ptr->transformations & PNG_INVERT_ALPHA) != 0 &&
          info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         int j;
         for (j = 0; j<(int)info_ptr->num_trans; j++)
            info_ptr->trans_alpha[j] =
               (png_byte)(255 - info_ptr->trans_alpha[j]);
      }
#endif
      png_write_tRNS(png_ptr, info_ptr->trans_alpha, &(info_ptr->trans_color),
          info_ptr->num_trans, info_ptr->color_type);
   }
#endif
#ifdef PNG_WRITE_bKGD_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_bKGD) != 0)
      png_write_bKGD(png_ptr, &(info_ptr->background), info_ptr->color_type);
#endif

#ifdef PNG_WRITE_hIST_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_hIST) != 0)
      png_write_hIST(png_ptr, info_ptr->hist, info_ptr->num_palette);
#endif

#ifdef PNG_WRITE_oFFs_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_oFFs) != 0)
      png_write_oFFs(png_ptr, info_ptr->x_offset, info_ptr->y_offset,
          info_ptr->offset_unit_type);
#endif

#ifdef PNG_WRITE_pCAL_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_pCAL) != 0)
      png_write_pCAL(png_ptr, info_ptr->pcal_purpose, info_ptr->pcal_X0,
          info_ptr->pcal_X1, info_ptr->pcal_type, info_ptr->pcal_nparams,
          info_ptr->pcal_units, info_ptr->pcal_params);
#endif

#ifdef PNG_WRITE_sCAL_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_sCAL) != 0)
      png_write_sCAL_s(png_ptr, (int)info_ptr->scal_unit,
          info_ptr->scal_s_width, info_ptr->scal_s_height);
#endif 

#ifdef PNG_WRITE_pHYs_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_pHYs) != 0)
      png_write_pHYs(png_ptr, info_ptr->x_pixels_per_unit,
          info_ptr->y_pixels_per_unit, info_ptr->phys_unit_type);
#endif 

#ifdef PNG_WRITE_tIME_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_tIME) != 0)
   {
      png_write_tIME(png_ptr, &(info_ptr->mod_time));
      png_ptr->mode |= PNG_WROTE_tIME;
   }
#endif 

#ifdef PNG_WRITE_sPLT_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_sPLT) != 0)
      for (i = 0; i < (int)info_ptr->splt_palettes_num; i++)
         png_write_sPLT(png_ptr, info_ptr->splt_palettes + i);
#endif 

#ifdef PNG_WRITE_TEXT_SUPPORTED
   
   for (i = 0; i < info_ptr->num_text; i++)
   {
      png_debug2(2, "Writing header text chunk %d, type %d", i,
          info_ptr->text[i].compression);
      
      if (info_ptr->text[i].compression > 0)
      {
#ifdef PNG_WRITE_iTXt_SUPPORTED
         
         png_write_iTXt(png_ptr,
             info_ptr->text[i].compression,
             info_ptr->text[i].key,
             info_ptr->text[i].lang,
             info_ptr->text[i].lang_key,
             info_ptr->text[i].text);
         
         if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_NONE)
            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
         else
            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_zTXt_WR;
#else
         png_warning(png_ptr, "Unable to write international text");
#endif
      }

      
      else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_zTXt)
      {
#ifdef PNG_WRITE_zTXt_SUPPORTED
         
         png_write_zTXt(png_ptr, info_ptr->text[i].key,
             info_ptr->text[i].text, info_ptr->text[i].compression);
         
         info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_zTXt_WR;
#else
         png_warning(png_ptr, "Unable to write compressed text");
#endif
      }

      else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_NONE)
      {
#ifdef PNG_WRITE_tEXt_SUPPORTED
         
         png_write_tEXt(png_ptr, info_ptr->text[i].key,
             info_ptr->text[i].text,
             0);
         
         info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
#else
         
         png_warning(png_ptr, "Unable to write uncompressed text");
#endif
      }
   }
#endif 

#ifdef PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
   write_unknown_chunks(png_ptr, info_ptr, PNG_HAVE_PLTE);
#endif
}






void PNGAPI
png_write_end(png_structrp png_ptr, png_inforp info_ptr)
{
   png_debug(1, "in png_write_end");

   if (png_ptr == NULL)
      return;

   if ((png_ptr->mode & PNG_HAVE_IDAT) == 0)
      png_error(png_ptr, "No IDATs written into file");

#ifdef PNG_WRITE_APNG_SUPPORTED
   if (png_ptr->num_frames_written != png_ptr->num_frames_to_write)
      png_error(png_ptr, "Not enough frames written");
#endif

#ifdef PNG_WRITE_CHECK_FOR_INVALID_INDEX_SUPPORTED
   if (png_ptr->num_palette_max > png_ptr->num_palette)
      png_benign_error(png_ptr, "Wrote palette index exceeding num_palette");
#endif

   
   if (info_ptr != NULL)
   {
#ifdef PNG_WRITE_TEXT_SUPPORTED
      int i; 
#endif
#ifdef PNG_WRITE_tIME_SUPPORTED
      
      if ((info_ptr->valid & PNG_INFO_tIME) != 0 &&
          (png_ptr->mode & PNG_WROTE_tIME) == 0)
         png_write_tIME(png_ptr, &(info_ptr->mod_time));

#endif
#ifdef PNG_WRITE_TEXT_SUPPORTED
      
      for (i = 0; i < info_ptr->num_text; i++)
      {
         png_debug2(2, "Writing trailer text chunk %d, type %d", i,
            info_ptr->text[i].compression);
         
         if (info_ptr->text[i].compression > 0)
         {
#ifdef PNG_WRITE_iTXt_SUPPORTED
            
            png_write_iTXt(png_ptr,
                info_ptr->text[i].compression,
                info_ptr->text[i].key,
                info_ptr->text[i].lang,
                info_ptr->text[i].lang_key,
                info_ptr->text[i].text);
            
            if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_NONE)
               info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
            else
               info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_zTXt_WR;
#else
            png_warning(png_ptr, "Unable to write international text");
#endif
         }

         else if (info_ptr->text[i].compression >= PNG_TEXT_COMPRESSION_zTXt)
         {
#ifdef PNG_WRITE_zTXt_SUPPORTED
            
            png_write_zTXt(png_ptr, info_ptr->text[i].key,
                info_ptr->text[i].text, info_ptr->text[i].compression);
            
            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_zTXt_WR;
#else
            png_warning(png_ptr, "Unable to write compressed text");
#endif
         }

         else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_NONE)
         {
#ifdef PNG_WRITE_tEXt_SUPPORTED
            
            png_write_tEXt(png_ptr, info_ptr->text[i].key,
                info_ptr->text[i].text, 0);
            
            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
#else
            png_warning(png_ptr, "Unable to write uncompressed text");
#endif
         }
      }
#endif
#ifdef PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
      write_unknown_chunks(png_ptr, info_ptr, PNG_AFTER_IDAT);
#endif
   }

   png_ptr->mode |= PNG_AFTER_IDAT;

   
   png_write_IEND(png_ptr);

   






#ifdef PNG_WRITE_FLUSH_SUPPORTED
#  ifdef PNG_WRITE_FLUSH_AFTER_IEND_SUPPORTED
   png_flush(png_ptr);
#  endif
#endif
}

#ifdef PNG_CONVERT_tIME_SUPPORTED
void PNGAPI
png_convert_from_struct_tm(png_timep ptime, PNG_CONST struct tm * ttime)
{
   png_debug(1, "in png_convert_from_struct_tm");

   ptime->year = (png_uint_16)(1900 + ttime->tm_year);
   ptime->month = (png_byte)(ttime->tm_mon + 1);
   ptime->day = (png_byte)ttime->tm_mday;
   ptime->hour = (png_byte)ttime->tm_hour;
   ptime->minute = (png_byte)ttime->tm_min;
   ptime->second = (png_byte)ttime->tm_sec;
}

void PNGAPI
png_convert_from_time_t(png_timep ptime, time_t ttime)
{
   struct tm *tbuf;

   png_debug(1, "in png_convert_from_time_t");

   tbuf = gmtime(&ttime);
   png_convert_from_struct_tm(ptime, tbuf);
}
#endif


PNG_FUNCTION(png_structp,PNGAPI
png_create_write_struct,(png_const_charp user_png_ver, png_voidp error_ptr,
    png_error_ptr error_fn, png_error_ptr warn_fn),PNG_ALLOCATED)
{
#ifndef PNG_USER_MEM_SUPPORTED
   png_structrp png_ptr = png_create_png_struct(user_png_ver, error_ptr,
       error_fn, warn_fn, NULL, NULL, NULL);
#else
   return png_create_write_struct_2(user_png_ver, error_ptr, error_fn,
       warn_fn, NULL, NULL, NULL);
}


PNG_FUNCTION(png_structp,PNGAPI
png_create_write_struct_2,(png_const_charp user_png_ver, png_voidp error_ptr,
    png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
    png_malloc_ptr malloc_fn, png_free_ptr free_fn),PNG_ALLOCATED)
{
   png_structrp png_ptr = png_create_png_struct(user_png_ver, error_ptr,
       error_fn, warn_fn, mem_ptr, malloc_fn, free_fn);
#endif
   if (png_ptr != NULL)
   {
      


      png_ptr->zbuffer_size = PNG_ZBUF_SIZE;

      



      png_ptr->zlib_strategy = PNG_Z_DEFAULT_STRATEGY;
      png_ptr->zlib_level = PNG_Z_DEFAULT_COMPRESSION;
      png_ptr->zlib_mem_level = 8;
      png_ptr->zlib_window_bits = 15;
      png_ptr->zlib_method = 8;

#ifdef PNG_WRITE_COMPRESSED_TEXT_SUPPORTED
      png_ptr->zlib_text_strategy = PNG_TEXT_Z_DEFAULT_STRATEGY;
      png_ptr->zlib_text_level = PNG_TEXT_Z_DEFAULT_COMPRESSION;
      png_ptr->zlib_text_mem_level = 8;
      png_ptr->zlib_text_window_bits = 15;
      png_ptr->zlib_text_method = 8;
#endif 

      




#ifdef PNG_BENIGN_WRITE_ERRORS_SUPPORTED
      


      png_ptr->flags |= PNG_FLAG_BENIGN_ERRORS_WARN;
#endif

      


#if PNG_LIBPNG_BUILD_BASE_TYPE >= PNG_LIBPNG_BUILD_RC
      png_ptr->flags |= PNG_FLAG_APP_WARNINGS_WARN;
#endif

      



      png_set_write_fn(png_ptr, NULL, NULL, NULL);
   }

   return png_ptr;
}







void PNGAPI
png_write_rows(png_structrp png_ptr, png_bytepp row,
    png_uint_32 num_rows)
{
   png_uint_32 i; 
   png_bytepp rp; 

   png_debug(1, "in png_write_rows");

   if (png_ptr == NULL)
      return;

   
   for (i = 0, rp = row; i < num_rows; i++, rp++)
   {
      png_write_row(png_ptr, *rp);
   }
}




void PNGAPI
png_write_image(png_structrp png_ptr, png_bytepp image)
{
   png_uint_32 i; 
   int pass, num_pass; 
   png_bytepp rp; 

   if (png_ptr == NULL)
      return;

   png_debug(1, "in png_write_image");

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   


   num_pass = png_set_interlace_handling(png_ptr);
#else
   num_pass = 1;
#endif
   
   for (pass = 0; pass < num_pass; pass++)
   {
      
      for (i = 0, rp = image; i < png_ptr->height; i++, rp++)
      {
         png_write_row(png_ptr, *rp);
      }
   }
}

#ifdef PNG_MNG_FEATURES_SUPPORTED

static void
png_do_write_intrapixel(png_row_infop row_info, png_bytep row)
{
   png_debug(1, "in png_do_write_intrapixel");

   if ((row_info->color_type & PNG_COLOR_MASK_COLOR) != 0)
   {
      int bytes_per_pixel;
      png_uint_32 row_width = row_info->width;
      if (row_info->bit_depth == 8)
      {
         png_bytep rp;
         png_uint_32 i;

         if (row_info->color_type == PNG_COLOR_TYPE_RGB)
            bytes_per_pixel = 3;

         else if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
            bytes_per_pixel = 4;

         else
            return;

         for (i = 0, rp = row; i < row_width; i++, rp += bytes_per_pixel)
         {
            *(rp)     = (png_byte)(*rp       - *(rp + 1));
            *(rp + 2) = (png_byte)(*(rp + 2) - *(rp + 1));
         }
      }

#ifdef PNG_WRITE_16BIT_SUPPORTED
      else if (row_info->bit_depth == 16)
      {
         png_bytep rp;
         png_uint_32 i;

         if (row_info->color_type == PNG_COLOR_TYPE_RGB)
            bytes_per_pixel = 6;

         else if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
            bytes_per_pixel = 8;

         else
            return;

         for (i = 0, rp = row; i < row_width; i++, rp += bytes_per_pixel)
         {
            png_uint_32 s0   = (*(rp    ) << 8) | *(rp + 1);
            png_uint_32 s1   = (*(rp + 2) << 8) | *(rp + 3);
            png_uint_32 s2   = (*(rp + 4) << 8) | *(rp + 5);
            png_uint_32 red  = (png_uint_32)((s0 - s1) & 0xffffL);
            png_uint_32 blue = (png_uint_32)((s2 - s1) & 0xffffL);
            *(rp    ) = (png_byte)(red >> 8);
            *(rp + 1) = (png_byte)red;
            *(rp + 4) = (png_byte)(blue >> 8);
            *(rp + 5) = (png_byte)blue;
         }
      }
#endif 
   }
}
#endif 


void PNGAPI
png_write_row(png_structrp png_ptr, png_const_bytep row)
{
   
   png_row_info row_info;

   if (png_ptr == NULL)
      return;

   png_debug2(1, "in png_write_row (row %u, pass %d)",
      png_ptr->row_number, png_ptr->pass);

   
   if (png_ptr->row_number == 0 && png_ptr->pass == 0)
   {
      
      if ((png_ptr->mode & PNG_WROTE_INFO_BEFORE_PLTE) == 0)
         png_error(png_ptr,
             "png_write_info was never called before png_write_row");

      
#if !defined(PNG_WRITE_INVERT_SUPPORTED) && defined(PNG_READ_INVERT_SUPPORTED)
      if ((png_ptr->transformations & PNG_INVERT_MONO) != 0)
         png_warning(png_ptr, "PNG_WRITE_INVERT_SUPPORTED is not defined");
#endif

#if !defined(PNG_WRITE_FILLER_SUPPORTED) && defined(PNG_READ_FILLER_SUPPORTED)
      if ((png_ptr->transformations & PNG_FILLER) != 0)
         png_warning(png_ptr, "PNG_WRITE_FILLER_SUPPORTED is not defined");
#endif
#if !defined(PNG_WRITE_PACKSWAP_SUPPORTED) && \
    defined(PNG_READ_PACKSWAP_SUPPORTED)
      if ((png_ptr->transformations & PNG_PACKSWAP) != 0)
         png_warning(png_ptr,
             "PNG_WRITE_PACKSWAP_SUPPORTED is not defined");
#endif

#if !defined(PNG_WRITE_PACK_SUPPORTED) && defined(PNG_READ_PACK_SUPPORTED)
      if ((png_ptr->transformations & PNG_PACK) != 0)
         png_warning(png_ptr, "PNG_WRITE_PACK_SUPPORTED is not defined");
#endif

#if !defined(PNG_WRITE_SHIFT_SUPPORTED) && defined(PNG_READ_SHIFT_SUPPORTED)
      if ((png_ptr->transformations & PNG_SHIFT) != 0)
         png_warning(png_ptr, "PNG_WRITE_SHIFT_SUPPORTED is not defined");
#endif

#if !defined(PNG_WRITE_BGR_SUPPORTED) && defined(PNG_READ_BGR_SUPPORTED)
      if ((png_ptr->transformations & PNG_BGR) != 0)
         png_warning(png_ptr, "PNG_WRITE_BGR_SUPPORTED is not defined");
#endif

#if !defined(PNG_WRITE_SWAP_SUPPORTED) && defined(PNG_READ_SWAP_SUPPORTED)
      if ((png_ptr->transformations & PNG_SWAP_BYTES) != 0)
         png_warning(png_ptr, "PNG_WRITE_SWAP_SUPPORTED is not defined");
#endif

      png_write_start_row(png_ptr);
   }

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   
   if (png_ptr->interlaced != 0 &&
       (png_ptr->transformations & PNG_INTERLACE) != 0)
   {
      switch (png_ptr->pass)
      {
         case 0:
            if ((png_ptr->row_number & 0x07) != 0)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         case 1:
            if ((png_ptr->row_number & 0x07) != 0 || png_ptr->width < 5)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         case 2:
            if ((png_ptr->row_number & 0x07) != 4)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         case 3:
            if ((png_ptr->row_number & 0x03) != 0 || png_ptr->width < 3)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         case 4:
            if ((png_ptr->row_number & 0x03) != 2)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         case 5:
            if ((png_ptr->row_number & 0x01) != 0 || png_ptr->width < 2)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         case 6:
            if ((png_ptr->row_number & 0x01) == 0)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         default: 
            break;
      }
   }
#endif

   
   row_info.color_type = png_ptr->color_type;
   row_info.width = png_ptr->usr_width;
   row_info.channels = png_ptr->usr_channels;
   row_info.bit_depth = png_ptr->usr_bit_depth;
   row_info.pixel_depth = (png_byte)(row_info.bit_depth * row_info.channels);
   row_info.rowbytes = PNG_ROWBYTES(row_info.pixel_depth, row_info.width);

   png_debug1(3, "row_info->color_type = %d", row_info.color_type);
   png_debug1(3, "row_info->width = %u", row_info.width);
   png_debug1(3, "row_info->channels = %d", row_info.channels);
   png_debug1(3, "row_info->bit_depth = %d", row_info.bit_depth);
   png_debug1(3, "row_info->pixel_depth = %d", row_info.pixel_depth);
   png_debug1(3, "row_info->rowbytes = %lu", (unsigned long)row_info.rowbytes);

   
   memcpy(png_ptr->row_buf + 1, row, row_info.rowbytes);

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   
   if (png_ptr->interlaced && png_ptr->pass < 6 &&
       (png_ptr->transformations & PNG_INTERLACE) != 0)
   {
      png_do_write_interlace(&row_info, png_ptr->row_buf + 1, png_ptr->pass);
      
      if (row_info.width == 0)
      {
         png_write_finish_row(png_ptr);
         return;
      }
   }
#endif

#ifdef PNG_WRITE_TRANSFORMS_SUPPORTED
   
   if (png_ptr->transformations != 0)
      png_do_write_transformations(png_ptr, &row_info);
#endif

   


   if (row_info.pixel_depth != png_ptr->pixel_depth ||
      row_info.pixel_depth != png_ptr->transformed_pixel_depth)
      png_error(png_ptr, "internal write transform logic error");

#ifdef PNG_MNG_FEATURES_SUPPORTED
   








   if ((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) != 0 &&
       (png_ptr->filter_type == PNG_INTRAPIXEL_DIFFERENCING))
   {
      
      png_do_write_intrapixel(&row_info, png_ptr->row_buf + 1);
   }
#endif


#ifdef PNG_WRITE_CHECK_FOR_INVALID_INDEX_SUPPORTED
   
   if (row_info.color_type == PNG_COLOR_TYPE_PALETTE &&
       png_ptr->num_palette_max >= 0)
      png_do_check_palette_indexes(png_ptr, &row_info);
#endif

   
   png_write_find_filter(png_ptr, &row_info);

   if (png_ptr->write_row_fn != NULL)
      (*(png_ptr->write_row_fn))(png_ptr, png_ptr->row_number, png_ptr->pass);
}

#ifdef PNG_WRITE_FLUSH_SUPPORTED

void PNGAPI
png_set_flush(png_structrp png_ptr, int nrows)
{
   png_debug(1, "in png_set_flush");

   if (png_ptr == NULL)
      return;

   png_ptr->flush_dist = (nrows < 0 ? 0 : nrows);
}


void PNGAPI
png_write_flush(png_structrp png_ptr)
{
   png_debug(1, "in png_write_flush");

   if (png_ptr == NULL)
      return;

   
   if (png_ptr->row_number >= png_ptr->num_rows)
      return;

   png_compress_IDAT(png_ptr, NULL, 0, Z_SYNC_FLUSH);
   png_ptr->flush_rows = 0;
   png_flush(png_ptr);
}
#endif 

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
static void png_reset_filter_heuristics(png_structrp png_ptr);
#endif


static void
png_write_destroy(png_structrp png_ptr)
{
   png_debug(1, "in png_write_destroy");

   
   if ((png_ptr->flags & PNG_FLAG_ZSTREAM_INITIALIZED) != 0)
      deflateEnd(&png_ptr->zstream);

   
   png_free_buffer_list(png_ptr, &png_ptr->zbuffer_list);
   png_free(png_ptr, png_ptr->row_buf);
   png_ptr->row_buf = NULL;
#ifdef PNG_WRITE_FILTER_SUPPORTED
   png_free(png_ptr, png_ptr->prev_row);
   png_free(png_ptr, png_ptr->sub_row);
   png_free(png_ptr, png_ptr->up_row);
   png_free(png_ptr, png_ptr->avg_row);
   png_free(png_ptr, png_ptr->paeth_row);
   png_ptr->prev_row = NULL;
   png_ptr->sub_row = NULL;
   png_ptr->up_row = NULL;
   png_ptr->avg_row = NULL;
   png_ptr->paeth_row = NULL;
#endif

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
   
   png_reset_filter_heuristics(png_ptr);
   png_free(png_ptr, png_ptr->filter_costs);
   png_free(png_ptr, png_ptr->inv_filter_costs);
   png_ptr->filter_costs = NULL;
   png_ptr->inv_filter_costs = NULL;
#endif

#ifdef PNG_SET_UNKNOWN_CHUNKS_SUPPORTED
   png_free(png_ptr, png_ptr->chunk_list);
   png_ptr->chunk_list = NULL;
#endif

   



}








void PNGAPI
png_destroy_write_struct(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr)
{
   png_debug(1, "in png_destroy_write_struct");

   if (png_ptr_ptr != NULL)
   {
      png_structrp png_ptr = *png_ptr_ptr;

      if (png_ptr != NULL) 
      {
         png_destroy_info_struct(png_ptr, info_ptr_ptr);

         *png_ptr_ptr = NULL;
         png_write_destroy(png_ptr);
         png_destroy_png_struct(png_ptr);
      }
   }
}


void PNGAPI
png_set_filter(png_structrp png_ptr, int method, int filters)
{
   png_debug(1, "in png_set_filter");

   if (png_ptr == NULL)
      return;

#ifdef PNG_MNG_FEATURES_SUPPORTED
   if ((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) != 0 &&
       (method == PNG_INTRAPIXEL_DIFFERENCING))
      method = PNG_FILTER_TYPE_BASE;

#endif
   if (method == PNG_FILTER_TYPE_BASE)
   {
      switch (filters & (PNG_ALL_FILTERS | 0x07))
      {
#ifdef PNG_WRITE_FILTER_SUPPORTED
         case 5:
         case 6:
         case 7: png_app_error(png_ptr, "Unknown row filter for method 0");
            
#endif 
         case PNG_FILTER_VALUE_NONE:
            png_ptr->do_filter = PNG_FILTER_NONE; break;

#ifdef PNG_WRITE_FILTER_SUPPORTED
         case PNG_FILTER_VALUE_SUB:
            png_ptr->do_filter = PNG_FILTER_SUB; break;

         case PNG_FILTER_VALUE_UP:
            png_ptr->do_filter = PNG_FILTER_UP; break;

         case PNG_FILTER_VALUE_AVG:
            png_ptr->do_filter = PNG_FILTER_AVG; break;

         case PNG_FILTER_VALUE_PAETH:
            png_ptr->do_filter = PNG_FILTER_PAETH; break;

         default:
            png_ptr->do_filter = (png_byte)filters; break;
#else
         default:
            png_app_error(png_ptr, "Unknown row filter for method 0");
#endif 
      }

      








      if (png_ptr->row_buf != NULL)
      {
#ifdef PNG_WRITE_FILTER_SUPPORTED
         if ((png_ptr->do_filter & PNG_FILTER_SUB) != 0 &&
             png_ptr->sub_row == NULL)
         {
            png_ptr->sub_row = (png_bytep)png_malloc(png_ptr,
                (png_ptr->rowbytes + 1));
            png_ptr->sub_row[0] = PNG_FILTER_VALUE_SUB;
         }

         if ((png_ptr->do_filter & PNG_FILTER_UP) != 0 &&
              png_ptr->up_row == NULL)
         {
            if (png_ptr->prev_row == NULL)
            {
               png_warning(png_ptr, "Can't add Up filter after starting");
               png_ptr->do_filter = (png_byte)(png_ptr->do_filter &
                   ~PNG_FILTER_UP);
            }

            else
            {
               png_ptr->up_row = (png_bytep)png_malloc(png_ptr,
                   (png_ptr->rowbytes + 1));
               png_ptr->up_row[0] = PNG_FILTER_VALUE_UP;
            }
         }

         if ((png_ptr->do_filter & PNG_FILTER_AVG) != 0 &&
              png_ptr->avg_row == NULL)
         {
            if (png_ptr->prev_row == NULL)
            {
               png_warning(png_ptr, "Can't add Average filter after starting");
               png_ptr->do_filter = (png_byte)(png_ptr->do_filter &
                   ~PNG_FILTER_AVG);
            }

            else
            {
               png_ptr->avg_row = (png_bytep)png_malloc(png_ptr,
                   (png_ptr->rowbytes + 1));
               png_ptr->avg_row[0] = PNG_FILTER_VALUE_AVG;
            }
         }

         if ((png_ptr->do_filter & PNG_FILTER_PAETH) != 0 &&
             png_ptr->paeth_row == NULL)
         {
            if (png_ptr->prev_row == NULL)
            {
               png_warning(png_ptr, "Can't add Paeth filter after starting");
               png_ptr->do_filter &= (png_byte)(~PNG_FILTER_PAETH);
            }

            else
            {
               png_ptr->paeth_row = (png_bytep)png_malloc(png_ptr,
                   (png_ptr->rowbytes + 1));
               png_ptr->paeth_row[0] = PNG_FILTER_VALUE_PAETH;
            }
         }

         if (png_ptr->do_filter == PNG_NO_FILTERS)
#endif 
            png_ptr->do_filter = PNG_FILTER_NONE;
      }
   }
   else
      png_error(png_ptr, "Unknown custom filter method");
}








#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED      

static void
png_reset_filter_heuristics(png_structrp png_ptr)
{
   




   png_ptr->num_prev_filters = 0;
   png_ptr->heuristic_method = PNG_FILTER_HEURISTIC_UNWEIGHTED;
   if (png_ptr->prev_filters != NULL)
   {
      png_bytep old = png_ptr->prev_filters;
      png_ptr->prev_filters = NULL;
      png_free(png_ptr, old);
   }
   if (png_ptr->filter_weights != NULL)
   {
      png_uint_16p old = png_ptr->filter_weights;
      png_ptr->filter_weights = NULL;
      png_free(png_ptr, old);
   }

   if (png_ptr->inv_filter_weights != NULL)
   {
      png_uint_16p old = png_ptr->inv_filter_weights;
      png_ptr->inv_filter_weights = NULL;
      png_free(png_ptr, old);
   }

   
}

static int
png_init_filter_heuristics(png_structrp png_ptr, int heuristic_method,
   int num_weights)
{
   if (png_ptr == NULL)
      return 0;

   
   png_reset_filter_heuristics(png_ptr);

   



   if (heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
   {
      int i;

      if (num_weights > 0)
      {
         png_ptr->prev_filters = (png_bytep)png_malloc(png_ptr,
             (png_uint_32)((sizeof (png_byte)) * num_weights));

         
         for (i = 0; i < num_weights; i++)
         {
            png_ptr->prev_filters[i] = 255;
         }

         png_ptr->filter_weights = (png_uint_16p)png_malloc(png_ptr,
             (png_uint_32)((sizeof (png_uint_16)) * num_weights));

         png_ptr->inv_filter_weights = (png_uint_16p)png_malloc(png_ptr,
             (png_uint_32)((sizeof (png_uint_16)) * num_weights));

         for (i = 0; i < num_weights; i++)
         {
            png_ptr->inv_filter_weights[i] =
            png_ptr->filter_weights[i] = PNG_WEIGHT_FACTOR;
         }

         
         png_ptr->num_prev_filters = (png_byte)num_weights;
      }

      


      if (png_ptr->filter_costs == NULL)
      {
         png_ptr->filter_costs = (png_uint_16p)png_malloc(png_ptr,
             (png_uint_32)((sizeof (png_uint_16)) * PNG_FILTER_VALUE_LAST));

         png_ptr->inv_filter_costs = (png_uint_16p)png_malloc(png_ptr,
             (png_uint_32)((sizeof (png_uint_16)) * PNG_FILTER_VALUE_LAST));
      }

      for (i = 0; i < PNG_FILTER_VALUE_LAST; i++)
      {
         png_ptr->inv_filter_costs[i] =
         png_ptr->filter_costs[i] = PNG_COST_FACTOR;
      }

      
      png_ptr->heuristic_method = PNG_FILTER_HEURISTIC_WEIGHTED;

      
      return 1;
   }
   else if (heuristic_method == PNG_FILTER_HEURISTIC_DEFAULT ||
      heuristic_method == PNG_FILTER_HEURISTIC_UNWEIGHTED)
   {
      return 1;
   }
   else
   {
      png_warning(png_ptr, "Unknown filter heuristic method");
      return 0;
   }
}


#ifdef PNG_FLOATING_POINT_SUPPORTED
void PNGAPI
png_set_filter_heuristics(png_structrp png_ptr, int heuristic_method,
    int num_weights, png_const_doublep filter_weights,
    png_const_doublep filter_costs)
{
   png_debug(1, "in png_set_filter_heuristics");

   


   if (png_init_filter_heuristics(png_ptr, heuristic_method, num_weights) == 0)
      return;

   
   if (heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
   {
      int i;
      for (i = 0; i < num_weights; i++)
      {
         if (filter_weights[i] <= 0.0)
         {
            png_ptr->inv_filter_weights[i] =
            png_ptr->filter_weights[i] = PNG_WEIGHT_FACTOR;
         }

         else
         {
            png_ptr->inv_filter_weights[i] =
                (png_uint_16)(PNG_WEIGHT_FACTOR*filter_weights[i]+.5);

            png_ptr->filter_weights[i] =
                (png_uint_16)(PNG_WEIGHT_FACTOR/filter_weights[i]+.5);
         }
      }

      






      for (i = 0; i < PNG_FILTER_VALUE_LAST; i++) if (filter_costs[i] >= 1.0)
      {
         png_ptr->inv_filter_costs[i] =
             (png_uint_16)(PNG_COST_FACTOR / filter_costs[i] + .5);

         png_ptr->filter_costs[i] =
             (png_uint_16)(PNG_COST_FACTOR * filter_costs[i] + .5);
      }
   }
}
#endif 

#ifdef PNG_FIXED_POINT_SUPPORTED
void PNGAPI
png_set_filter_heuristics_fixed(png_structrp png_ptr, int heuristic_method,
    int num_weights, png_const_fixed_point_p filter_weights,
    png_const_fixed_point_p filter_costs)
{
   png_debug(1, "in png_set_filter_heuristics_fixed");

   


   if (png_init_filter_heuristics(png_ptr, heuristic_method, num_weights) == 0)
      return;

   
   if (heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
   {
      int i;
      for (i = 0; i < num_weights; i++)
      {
         if (filter_weights[i] <= 0)
         {
            png_ptr->inv_filter_weights[i] =
            png_ptr->filter_weights[i] = PNG_WEIGHT_FACTOR;
         }

         else
         {
            png_ptr->inv_filter_weights[i] = (png_uint_16)
               ((PNG_WEIGHT_FACTOR*filter_weights[i]+PNG_FP_HALF)/PNG_FP_1);

            png_ptr->filter_weights[i] = (png_uint_16)((PNG_WEIGHT_FACTOR*
               PNG_FP_1+(filter_weights[i]/2))/filter_weights[i]);
         }
      }

      






      for (i = 0; i < PNG_FILTER_VALUE_LAST; i++)
         if (filter_costs[i] >= PNG_FP_1)
      {
         png_uint_32 tmp;

         



         tmp = PNG_COST_FACTOR*PNG_FP_1 + (filter_costs[i]/2);
         tmp /= filter_costs[i];

         png_ptr->inv_filter_costs[i] = (png_uint_16)tmp;

         tmp = PNG_COST_FACTOR * filter_costs[i] + PNG_FP_HALF;
         tmp /= PNG_FP_1;

         png_ptr->filter_costs[i] = (png_uint_16)tmp;
      }
   }
}
#endif 
#endif 

#ifdef PNG_WRITE_CUSTOMIZE_COMPRESSION_SUPPORTED
void PNGAPI
png_set_compression_level(png_structrp png_ptr, int level)
{
   png_debug(1, "in png_set_compression_level");

   if (png_ptr == NULL)
      return;

   png_ptr->zlib_level = level;
}

void PNGAPI
png_set_compression_mem_level(png_structrp png_ptr, int mem_level)
{
   png_debug(1, "in png_set_compression_mem_level");

   if (png_ptr == NULL)
      return;

   png_ptr->zlib_mem_level = mem_level;
}

void PNGAPI
png_set_compression_strategy(png_structrp png_ptr, int strategy)
{
   png_debug(1, "in png_set_compression_strategy");

   if (png_ptr == NULL)
      return;

   

   png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_STRATEGY;
   png_ptr->zlib_strategy = strategy;
}




void PNGAPI
png_set_compression_window_bits(png_structrp png_ptr, int window_bits)
{
   if (png_ptr == NULL)
      return;

   





   if (window_bits > 15)
   {
      png_warning(png_ptr, "Only compression windows <= 32k supported by PNG");
      window_bits = 15;
   }

   else if (window_bits < 8)
   {
      png_warning(png_ptr, "Only compression windows >= 256 supported by PNG");
      window_bits = 8;
   }

   png_ptr->zlib_window_bits = window_bits;
}

void PNGAPI
png_set_compression_method(png_structrp png_ptr, int method)
{
   png_debug(1, "in png_set_compression_method");

   if (png_ptr == NULL)
      return;

   


   if (method != 8)
      png_warning(png_ptr, "Only compression method 8 is supported by PNG");

   png_ptr->zlib_method = method;
}
#endif 


#ifdef PNG_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED
void PNGAPI
png_set_text_compression_level(png_structrp png_ptr, int level)
{
   png_debug(1, "in png_set_text_compression_level");

   if (png_ptr == NULL)
      return;

   png_ptr->zlib_text_level = level;
}

void PNGAPI
png_set_text_compression_mem_level(png_structrp png_ptr, int mem_level)
{
   png_debug(1, "in png_set_text_compression_mem_level");

   if (png_ptr == NULL)
      return;

   png_ptr->zlib_text_mem_level = mem_level;
}

void PNGAPI
png_set_text_compression_strategy(png_structrp png_ptr, int strategy)
{
   png_debug(1, "in png_set_text_compression_strategy");

   if (png_ptr == NULL)
      return;

   png_ptr->zlib_text_strategy = strategy;
}




void PNGAPI
png_set_text_compression_window_bits(png_structrp png_ptr, int window_bits)
{
   if (png_ptr == NULL)
      return;

   if (window_bits > 15)
   {
      png_warning(png_ptr, "Only compression windows <= 32k supported by PNG");
      window_bits = 15;
   }

   else if (window_bits < 8)
   {
      png_warning(png_ptr, "Only compression windows >= 256 supported by PNG");
      window_bits = 8;
   }

   png_ptr->zlib_text_window_bits = window_bits;
}

void PNGAPI
png_set_text_compression_method(png_structrp png_ptr, int method)
{
   png_debug(1, "in png_set_text_compression_method");

   if (png_ptr == NULL)
      return;

   if (method != 8)
      png_warning(png_ptr, "Only compression method 8 is supported by PNG");

   png_ptr->zlib_text_method = method;
}
#endif 


void PNGAPI
png_set_write_status_fn(png_structrp png_ptr, png_write_status_ptr write_row_fn)
{
   if (png_ptr == NULL)
      return;

   png_ptr->write_row_fn = write_row_fn;
}

#ifdef PNG_WRITE_USER_TRANSFORM_SUPPORTED
void PNGAPI
png_set_write_user_transform_fn(png_structrp png_ptr, png_user_transform_ptr
    write_user_transform_fn)
{
   png_debug(1, "in png_set_write_user_transform_fn");

   if (png_ptr == NULL)
      return;

   png_ptr->transformations |= PNG_USER_TRANSFORM;
   png_ptr->write_user_transform_fn = write_user_transform_fn;
}
#endif


#ifdef PNG_INFO_IMAGE_SUPPORTED
void PNGAPI
png_write_png(png_structrp png_ptr, png_inforp info_ptr,
    int transforms, voidp params)
{
   if (png_ptr == NULL || info_ptr == NULL)
      return;

   if ((info_ptr->valid & PNG_INFO_IDAT) == 0)
   {
      png_app_error(png_ptr, "no rows for png_write_image to write");
      return;
   }

   
   png_write_info(png_ptr, info_ptr);

   

   
   if ((transforms & PNG_TRANSFORM_INVERT_MONO) != 0)
#ifdef PNG_WRITE_INVERT_SUPPORTED
      png_set_invert_mono(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_INVERT_MONO not supported");
#endif

   


   if ((transforms & PNG_TRANSFORM_SHIFT) != 0)
#ifdef PNG_WRITE_SHIFT_SUPPORTED
      if ((info_ptr->valid & PNG_INFO_sBIT) != 0)
         png_set_shift(png_ptr, &info_ptr->sig_bit);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_SHIFT not supported");
#endif

   
   if ((transforms & PNG_TRANSFORM_PACKING) != 0)
#ifdef PNG_WRITE_PACK_SUPPORTED
      png_set_packing(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_PACKING not supported");
#endif

   
   if ((transforms & PNG_TRANSFORM_SWAP_ALPHA) != 0)
#ifdef PNG_WRITE_SWAP_ALPHA_SUPPORTED
      png_set_swap_alpha(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_SWAP_ALPHA not supported");
#endif

   



   if ((transforms & (PNG_TRANSFORM_STRIP_FILLER_AFTER|
      PNG_TRANSFORM_STRIP_FILLER_BEFORE)) != 0)
   {
#ifdef PNG_WRITE_FILLER_SUPPORTED
      if ((transforms & PNG_TRANSFORM_STRIP_FILLER_AFTER) != 0)
      {
         if ((transforms & PNG_TRANSFORM_STRIP_FILLER_BEFORE) != 0)
            png_app_error(png_ptr,
               "PNG_TRANSFORM_STRIP_FILLER: BEFORE+AFTER not supported");

         
         png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
      }

      else if ((transforms & PNG_TRANSFORM_STRIP_FILLER_BEFORE) != 0)
         png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_STRIP_FILLER not supported");
#endif
   }

   
   if ((transforms & PNG_TRANSFORM_BGR) != 0)
#ifdef PNG_WRITE_BGR_SUPPORTED
      png_set_bgr(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_BGR not supported");
#endif

   
   if ((transforms & PNG_TRANSFORM_SWAP_ENDIAN) != 0)
#ifdef PNG_WRITE_SWAP_SUPPORTED
      png_set_swap(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_SWAP_ENDIAN not supported");
#endif

   
   if ((transforms & PNG_TRANSFORM_PACKSWAP) != 0)
#ifdef PNG_WRITE_PACKSWAP_SUPPORTED
      png_set_packswap(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_PACKSWAP not supported");
#endif

   
   if ((transforms & PNG_TRANSFORM_INVERT_ALPHA) != 0)
#ifdef PNG_WRITE_INVERT_ALPHA_SUPPORTED
      png_set_invert_alpha(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_INVERT_ALPHA not supported");
#endif

   

   
   png_write_image(png_ptr, info_ptr->row_pointers);

   
   png_write_end(png_ptr, info_ptr);

   PNG_UNUSED(params)
}
#endif


#ifdef PNG_SIMPLIFIED_WRITE_SUPPORTED
#ifdef PNG_STDIO_SUPPORTED 

static int
png_image_write_init(png_imagep image)
{
   png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, image,
          png_safe_error, png_safe_warning);

   if (png_ptr != NULL)
   {
      png_infop info_ptr = png_create_info_struct(png_ptr);

      if (info_ptr != NULL)
      {
         png_controlp control = png_voidcast(png_controlp,
            png_malloc_warn(png_ptr, (sizeof *control)));

         if (control != NULL)
         {
            memset(control, 0, (sizeof *control));

            control->png_ptr = png_ptr;
            control->info_ptr = info_ptr;
            control->for_write = 1;

            image->opaque = control;
            return 1;
         }

         
         png_destroy_info_struct(png_ptr, &info_ptr);
      }

      png_destroy_write_struct(&png_ptr, NULL);
   }

   return png_image_error(image, "png_image_write_: out of memory");
}


typedef struct
{
   
   png_imagep      image;
   png_const_voidp buffer;
   png_int_32      row_stride;
   png_const_voidp colormap;
   int             convert_to_8bit;
   
   png_const_voidp first_row;
   ptrdiff_t       row_bytes;
   png_voidp       local_row;
} png_image_write_control;





static int
png_write_image_16bit(png_voidp argument)
{
   png_image_write_control *display = png_voidcast(png_image_write_control*,
      argument);
   png_imagep image = display->image;
   png_structrp png_ptr = image->opaque->png_ptr;

   png_const_uint_16p input_row = png_voidcast(png_const_uint_16p,
      display->first_row);
   png_uint_16p output_row = png_voidcast(png_uint_16p, display->local_row);
   png_uint_16p row_end;
   const int channels = (image->format & PNG_FORMAT_FLAG_COLOR) != 0 ? 3 : 1;
   int aindex = 0;
   png_uint_32 y = image->height;

   if ((image->format & PNG_FORMAT_FLAG_ALPHA) != 0)
   {
#     ifdef PNG_SIMPLIFIED_WRITE_AFIRST_SUPPORTED
         if ((image->format & PNG_FORMAT_FLAG_AFIRST) != 0)
         {
            aindex = -1;
            ++input_row; 
            ++output_row;
         }

         else
#     endif
         aindex = channels;
   }

   else
      png_error(png_ptr, "png_write_image: internal call error");

   



   row_end = output_row + image->width * (channels+1);

   while (y-- > 0)
   {
      png_const_uint_16p in_ptr = input_row;
      png_uint_16p out_ptr = output_row;

      while (out_ptr < row_end)
      {
         const png_uint_16 alpha = in_ptr[aindex];
         png_uint_32 reciprocal = 0;
         int c;

         out_ptr[aindex] = alpha;

         




         if (alpha > 0 && alpha < 65535)
            reciprocal = ((0xffff<<15)+(alpha>>1))/alpha;

         c = channels;
         do 
         {
            png_uint_16 component = *in_ptr++;

            






            if (component >= alpha)
               component = 65535;

            


            else if (component > 0 && alpha < 65535)
            {
               png_uint_32 calc = component * reciprocal;
               calc += 16384; 
               component = (png_uint_16)(calc >> 15);
            }

            *out_ptr++ = component;
         }
         while (--c > 0);

         
         ++in_ptr;
         ++out_ptr;
      }

      png_write_row(png_ptr, png_voidcast(png_const_bytep, display->local_row));
      input_row += display->row_bytes/(sizeof (png_uint_16));
   }

   return 1;
}









#define UNP_RECIPROCAL(alpha) ((((0xffff*0xff)<<7)+(alpha>>1))/alpha)

static png_byte
png_unpremultiply(png_uint_32 component, png_uint_32 alpha,
   png_uint_32 reciprocal)
{
   









   if (component >= alpha || alpha < 128)
      return 255;

   


   else if (component > 0)
   {
      




      if (alpha < 65407)
      {
         component *= reciprocal;
         component += 64; 
         component >>= 7;
      }

      else
         component *= 255;

      
      return (png_byte)PNG_sRGB_FROM_LINEAR(component);
   }

   else
      return 0;
}

static int
png_write_image_8bit(png_voidp argument)
{
   png_image_write_control *display = png_voidcast(png_image_write_control*,
      argument);
   png_imagep image = display->image;
   png_structrp png_ptr = image->opaque->png_ptr;

   png_const_uint_16p input_row = png_voidcast(png_const_uint_16p,
      display->first_row);
   png_bytep output_row = png_voidcast(png_bytep, display->local_row);
   png_uint_32 y = image->height;
   const int channels = (image->format & PNG_FORMAT_FLAG_COLOR) != 0 ? 3 : 1;

   if ((image->format & PNG_FORMAT_FLAG_ALPHA) != 0)
   {
      png_bytep row_end;
      int aindex;

#     ifdef PNG_SIMPLIFIED_WRITE_AFIRST_SUPPORTED
         if ((image->format & PNG_FORMAT_FLAG_AFIRST) != 0)
         {
            aindex = -1;
            ++input_row; 
            ++output_row;
         }

         else
#     endif
         aindex = channels;

      
      row_end = output_row + image->width * (channels+1);

      while (y-- > 0)
      {
         png_const_uint_16p in_ptr = input_row;
         png_bytep out_ptr = output_row;

         while (out_ptr < row_end)
         {
            png_uint_16 alpha = in_ptr[aindex];
            png_byte alphabyte = (png_byte)PNG_DIV257(alpha);
            png_uint_32 reciprocal = 0;
            int c;

            
            out_ptr[aindex] = alphabyte;

            if (alphabyte > 0 && alphabyte < 255)
               reciprocal = UNP_RECIPROCAL(alpha);

            c = channels;
            do 
               *out_ptr++ = png_unpremultiply(*in_ptr++, alpha, reciprocal);
            while (--c > 0);

            
            ++in_ptr;
            ++out_ptr;
         } 

         png_write_row(png_ptr, png_voidcast(png_const_bytep,
            display->local_row));
         input_row += display->row_bytes/(sizeof (png_uint_16));
      } 
   }

   else
   {
      


      png_bytep row_end = output_row + image->width * channels;

      while (y-- > 0)
      {
         png_const_uint_16p in_ptr = input_row;
         png_bytep out_ptr = output_row;

         while (out_ptr < row_end)
         {
            png_uint_32 component = *in_ptr++;

            component *= 255;
            *out_ptr++ = (png_byte)PNG_sRGB_FROM_LINEAR(component);
         }

         png_write_row(png_ptr, output_row);
         input_row += display->row_bytes/(sizeof (png_uint_16));
      }
   }

   return 1;
}

static void
png_image_set_PLTE(png_image_write_control *display)
{
   const png_imagep image = display->image;
   const void *cmap = display->colormap;
   const int entries = image->colormap_entries > 256 ? 256 :
      (int)image->colormap_entries;

   
   const png_uint_32 format = image->format;
   const int channels = PNG_IMAGE_SAMPLE_CHANNELS(format);

#  if defined(PNG_FORMAT_BGR_SUPPORTED) &&\
      defined(PNG_SIMPLIFIED_WRITE_AFIRST_SUPPORTED)
      const int afirst = (format & PNG_FORMAT_FLAG_AFIRST) != 0 &&
         (format & PNG_FORMAT_FLAG_ALPHA) != 0;
#  else
#     define afirst 0
#  endif

#  ifdef PNG_FORMAT_BGR_SUPPORTED
      const int bgr = (format & PNG_FORMAT_FLAG_BGR) != 0 ? 2 : 0;
#  else
#     define bgr 0
#  endif

   int i, num_trans;
   png_color palette[256];
   png_byte tRNS[256];

   memset(tRNS, 255, (sizeof tRNS));
   memset(palette, 0, (sizeof palette));

   for (i=num_trans=0; i<entries; ++i)
   {
      


      if ((format & PNG_FORMAT_FLAG_LINEAR) != 0)
      {
         png_const_uint_16p entry = png_voidcast(png_const_uint_16p, cmap);

         entry += i * channels;

         if ((channels & 1) != 0) 
         {
            if (channels >= 3) 
            {
               palette[i].blue = (png_byte)PNG_sRGB_FROM_LINEAR(255 *
                  entry[(2 ^ bgr)]);
               palette[i].green = (png_byte)PNG_sRGB_FROM_LINEAR(255 *
                  entry[1]);
               palette[i].red = (png_byte)PNG_sRGB_FROM_LINEAR(255 *
                  entry[bgr]);
            }

            else 
               palette[i].blue = palette[i].red = palette[i].green =
                  (png_byte)PNG_sRGB_FROM_LINEAR(255 * *entry);
         }

         else 
         {
            png_uint_16 alpha = entry[afirst ? 0 : channels-1];
            png_byte alphabyte = (png_byte)PNG_DIV257(alpha);
            png_uint_32 reciprocal = 0;

            



            if (alphabyte > 0 && alphabyte < 255)
               reciprocal = (((0xffff*0xff)<<7)+(alpha>>1))/alpha;

            tRNS[i] = alphabyte;
            if (alphabyte < 255)
               num_trans = i+1;

            if (channels >= 3) 
            {
               palette[i].blue = png_unpremultiply(entry[afirst + (2 ^ bgr)],
                  alpha, reciprocal);
               palette[i].green = png_unpremultiply(entry[afirst + 1], alpha,
                  reciprocal);
               palette[i].red = png_unpremultiply(entry[afirst + bgr], alpha,
                  reciprocal);
            }

            else 
               palette[i].blue = palette[i].red = palette[i].green =
                  png_unpremultiply(entry[afirst], alpha, reciprocal);
         }
      }

      else 
      {
         png_const_bytep entry = png_voidcast(png_const_bytep, cmap);

         entry += i * channels;

         switch (channels)
         {
            case 4:
               tRNS[i] = entry[afirst ? 0 : 3];
               if (tRNS[i] < 255)
                  num_trans = i+1;
               
            case 3:
               palette[i].blue = entry[afirst + (2 ^ bgr)];
               palette[i].green = entry[afirst + 1];
               palette[i].red = entry[afirst + bgr];
               break;

            case 2:
               tRNS[i] = entry[1 ^ afirst];
               if (tRNS[i] < 255)
                  num_trans = i+1;
               
            case 1:
               palette[i].blue = palette[i].red = palette[i].green =
                  entry[afirst];
               break;

            default:
               break;
         }
      }
   }

#  ifdef afirst
#     undef afirst
#  endif
#  ifdef bgr
#     undef bgr
#  endif

   png_set_PLTE(image->opaque->png_ptr, image->opaque->info_ptr, palette,
      entries);

   if (num_trans > 0)
      png_set_tRNS(image->opaque->png_ptr, image->opaque->info_ptr, tRNS,
         num_trans, NULL);

   image->colormap_entries = entries;
}

static int
png_image_write_main(png_voidp argument)
{
   png_image_write_control *display = png_voidcast(png_image_write_control*,
      argument);
   png_imagep image = display->image;
   png_structrp png_ptr = image->opaque->png_ptr;
   png_inforp info_ptr = image->opaque->info_ptr;
   png_uint_32 format = image->format;

   
   int colormap = (format & PNG_FORMAT_FLAG_COLORMAP);
   int linear = !colormap && (format & PNG_FORMAT_FLAG_LINEAR); 
   int alpha = !colormap && (format & PNG_FORMAT_FLAG_ALPHA);
   int write_16bit = linear && !colormap && (display->convert_to_8bit == 0);

#  ifdef PNG_BENIGN_ERRORS_SUPPORTED
      
      png_set_benign_errors(png_ptr, 0);
#  endif

   
   if (display->row_stride == 0)
      display->row_stride = PNG_IMAGE_ROW_STRIDE(*image);

   
   if ((format & PNG_FORMAT_FLAG_COLORMAP) != 0)
   {
      if (display->colormap != NULL && image->colormap_entries > 0)
      {
         png_uint_32 entries = image->colormap_entries;

         png_set_IHDR(png_ptr, info_ptr, image->width, image->height,
            entries > 16 ? 8 : (entries > 4 ? 4 : (entries > 2 ? 2 : 1)),
            PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

         png_image_set_PLTE(display);
      }

      else
         png_error(image->opaque->png_ptr,
            "no color-map for color-mapped image");
   }

   else
      png_set_IHDR(png_ptr, info_ptr, image->width, image->height,
         write_16bit ? 16 : 8,
         ((format & PNG_FORMAT_FLAG_COLOR) ? PNG_COLOR_MASK_COLOR : 0) +
         ((format & PNG_FORMAT_FLAG_ALPHA) ? PNG_COLOR_MASK_ALPHA : 0),
         PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   





   if (write_16bit != 0)
   {
      
      png_set_gAMA_fixed(png_ptr, info_ptr, PNG_GAMMA_LINEAR);

      if ((image->flags & PNG_IMAGE_FLAG_COLORSPACE_NOT_sRGB) == 0)
         png_set_cHRM_fixed(png_ptr, info_ptr,
            
             31270, 32900,
             64000, 33000,
             30000, 60000,
             15000,  6000
         );
   }

   else if ((image->flags & PNG_IMAGE_FLAG_COLORSPACE_NOT_sRGB) == 0)
      png_set_sRGB(png_ptr, info_ptr, PNG_sRGB_INTENT_PERCEPTUAL);

   


   else
      png_set_gAMA_fixed(png_ptr, info_ptr, PNG_GAMMA_sRGB_INVERSE);

   
   png_write_info(png_ptr, info_ptr);

   




   if (write_16bit != 0)
   {
      PNG_CONST png_uint_16 le = 0x0001;

      if ((*(png_const_bytep) & le) != 0)
         png_set_swap(png_ptr);
   }

#  ifdef PNG_SIMPLIFIED_WRITE_BGR_SUPPORTED
      if ((format & PNG_FORMAT_FLAG_BGR) != 0)
      {
         if (colormap == 0 && (format & PNG_FORMAT_FLAG_COLOR) != 0)
            png_set_bgr(png_ptr);
         format &= ~PNG_FORMAT_FLAG_BGR;
      }
#  endif

#  ifdef PNG_SIMPLIFIED_WRITE_AFIRST_SUPPORTED
      if ((format & PNG_FORMAT_FLAG_AFIRST) != 0)
      {
         if (colormap == 0 && (format & PNG_FORMAT_FLAG_ALPHA) != 0)
            png_set_swap_alpha(png_ptr);
         format &= ~PNG_FORMAT_FLAG_AFIRST;
      }
#  endif

   


   if (colormap != 0 && image->colormap_entries <= 16)
      png_set_packing(png_ptr);

   
   if ((format & ~(png_uint_32)(PNG_FORMAT_FLAG_COLOR | PNG_FORMAT_FLAG_LINEAR |
         PNG_FORMAT_FLAG_ALPHA | PNG_FORMAT_FLAG_COLORMAP)) != 0)
      png_error(png_ptr, "png_write_image: unsupported transformation");

   {
      png_const_bytep row = png_voidcast(png_const_bytep, display->buffer);
      ptrdiff_t row_bytes = display->row_stride;

      if (linear != 0)
         row_bytes *= (sizeof (png_uint_16));

      if (row_bytes < 0)
         row += (image->height-1) * (-row_bytes);

      display->first_row = row;
      display->row_bytes = row_bytes;
   }

   
   if ((image->flags & PNG_IMAGE_FLAG_FAST) != 0)
   {
      png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_NO_FILTERS);
      




#ifdef PNG_WRITE_CUSTOMIZE_COMPRESSION_SUPPORTED
      png_set_compression_level(png_ptr, 3);
#endif
   }

   



   if ((linear != 0 && alpha != 0 ) ||
       (colormap == 0 && display->convert_to_8bit != 0))
   {
      png_bytep row = png_voidcast(png_bytep, png_malloc(png_ptr,
         png_get_rowbytes(png_ptr, info_ptr)));
      int result;

      display->local_row = row;
      if (write_16bit != 0)
         result = png_safe_execute(image, png_write_image_16bit, display);
      else
         result = png_safe_execute(image, png_write_image_8bit, display);
      display->local_row = NULL;

      png_free(png_ptr, row);

      
      if (result == 0)
         return 0;
   }

   


   else
   {
      png_const_bytep row = png_voidcast(png_const_bytep, display->first_row);
      ptrdiff_t row_bytes = display->row_bytes;
      png_uint_32 y = image->height;

      while (y-- > 0)
      {
         png_write_row(png_ptr, row);
         row += row_bytes;
      }
   }

   png_write_end(png_ptr, info_ptr);
   return 1;
}

int PNGAPI
png_image_write_to_stdio(png_imagep image, FILE *file, int convert_to_8bit,
   const void *buffer, png_int_32 row_stride, const void *colormap)
{
   
   if (image != NULL && image->version == PNG_IMAGE_VERSION)
   {
      if (file != NULL)
      {
         if (png_image_write_init(image) != 0)
         {
            png_image_write_control display;
            int result;

            



            image->opaque->png_ptr->io_ptr = file;

            memset(&display, 0, (sizeof display));
            display.image = image;
            display.buffer = buffer;
            display.row_stride = row_stride;
            display.colormap = colormap;
            display.convert_to_8bit = convert_to_8bit;

            result = png_safe_execute(image, png_image_write_main, &display);
            png_image_free(image);
            return result;
         }

         else
            return 0;
      }

      else
         return png_image_error(image,
            "png_image_write_to_stdio: invalid argument");
   }

   else if (image != NULL)
      return png_image_error(image,
         "png_image_write_to_stdio: incorrect PNG_IMAGE_VERSION");

   else
      return 0;
}

int PNGAPI
png_image_write_to_file(png_imagep image, const char *file_name,
   int convert_to_8bit, const void *buffer, png_int_32 row_stride,
   const void *colormap)
{
   
   if (image != NULL && image->version == PNG_IMAGE_VERSION)
   {
      if (file_name != NULL)
      {
         FILE *fp = fopen(file_name, "wb");

         if (fp != NULL)
         {
            if (png_image_write_to_stdio(image, fp, convert_to_8bit, buffer,
               row_stride, colormap) != 0)
            {
               int error; 

               
               if (fflush(fp) == 0 && ferror(fp) == 0)
               {
                  if (fclose(fp) == 0)
                     return 1;

                  error = errno; 
               }

               else
               {
                  error = errno; 
                  (void)fclose(fp);
               }

               (void)remove(file_name);
               


               return png_image_error(image, strerror(error));
            }

            else
            {
               
               (void)fclose(fp);
               (void)remove(file_name);
               return 0;
            }
         }

         else
            return png_image_error(image, strerror(errno));
      }

      else
         return png_image_error(image,
            "png_image_write_to_file: invalid argument");
   }

   else if (image != NULL)
      return png_image_error(image,
         "png_image_write_to_file: incorrect PNG_IMAGE_VERSION");

   else
      return 0;
}
#endif 
#endif 

#ifdef PNG_WRITE_APNG_SUPPORTED
void PNGAPI
png_write_frame_head(png_structp png_ptr, png_infop info_ptr,
    png_bytepp row_pointers, png_uint_32 width, png_uint_32 height,
    png_uint_32 x_offset, png_uint_32 y_offset,
    png_uint_16 delay_num, png_uint_16 delay_den, png_byte dispose_op,
    png_byte blend_op)
{
    png_debug(1, "in png_write_frame_head");

    

    if ((info_ptr->valid & PNG_INFO_acTL) == 0)
        png_error(png_ptr, "png_write_frame_head(): acTL not set");

    png_write_reset(png_ptr);

    png_write_reinit(png_ptr, info_ptr, width, height);

    if ((png_ptr->apng_flags & PNG_FIRST_FRAME_HIDDEN) == 0 ||
        png_ptr->num_frames_written != 0)
        png_write_fcTL(png_ptr, width, height, x_offset, y_offset,
                       delay_num, delay_den, dispose_op, blend_op);

    PNG_UNUSED(row_pointers)
}

void PNGAPI
png_write_frame_tail(png_structp png_ptr, png_infop info_ptr)
{
    png_debug(1, "in png_write_frame_tail");

    png_ptr->num_frames_written++;

    PNG_UNUSED(info_ptr)
}
#endif 
#endif 

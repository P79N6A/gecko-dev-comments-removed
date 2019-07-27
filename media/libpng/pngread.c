















#include "pngpriv.h"
#if defined(PNG_SIMPLIFIED_READ_SUPPORTED) && defined(PNG_STDIO_SUPPORTED)
#  include <errno.h>
#endif

#ifdef PNG_READ_SUPPORTED


PNG_FUNCTION(png_structp,PNGAPI
png_create_read_struct,(png_const_charp user_png_ver, png_voidp error_ptr,
    png_error_ptr error_fn, png_error_ptr warn_fn),PNG_ALLOCATED)
{
#ifndef PNG_USER_MEM_SUPPORTED
   png_structp png_ptr = png_create_png_struct(user_png_ver, error_ptr,
      error_fn, warn_fn, NULL, NULL, NULL);
#else
   return png_create_read_struct_2(user_png_ver, error_ptr, error_fn,
       warn_fn, NULL, NULL, NULL);
}




PNG_FUNCTION(png_structp,PNGAPI
png_create_read_struct_2,(png_const_charp user_png_ver, png_voidp error_ptr,
    png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
    png_malloc_ptr malloc_fn, png_free_ptr free_fn),PNG_ALLOCATED)
{
   png_structp png_ptr = png_create_png_struct(user_png_ver, error_ptr,
      error_fn, warn_fn, mem_ptr, malloc_fn, free_fn);
#endif

   if (png_ptr != NULL)
   {
      png_ptr->mode = PNG_IS_READ_STRUCT;

      


#     ifdef PNG_SEQUENTIAL_READ_SUPPORTED
         png_ptr->IDAT_read_size = PNG_IDAT_READ_SIZE;
#     endif

#     ifdef PNG_BENIGN_READ_ERRORS_SUPPORTED
         png_ptr->flags |= PNG_FLAG_BENIGN_ERRORS_WARN;

         


#        if PNG_LIBPNG_BUILD_BASE_TYPE >= PNG_LIBPNG_BUILD_RC
            png_ptr->flags |= PNG_FLAG_APP_WARNINGS_WARN;
#        endif
#     endif

      



      png_set_read_fn(png_ptr, NULL, NULL);
   }

   return png_ptr;
}


#ifdef PNG_SEQUENTIAL_READ_SUPPORTED








void PNGAPI
png_read_info(png_structrp png_ptr, png_inforp info_ptr)
{
#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
   int keep;
#endif

   png_debug(1, "in png_read_info");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   
   png_read_sig(png_ptr, info_ptr);

   for (;;)
   {
      png_uint_32 length = png_read_chunk_header(png_ptr);
      png_uint_32 chunk_name = png_ptr->chunk_name;

      


      if (chunk_name == png_IDAT)
      {
         if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
            png_chunk_error(png_ptr, "Missing IHDR before IDAT");

         else if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
             (png_ptr->mode & PNG_HAVE_PLTE) == 0)
            png_chunk_error(png_ptr, "Missing PLTE before IDAT");

         else if ((png_ptr->mode & PNG_AFTER_IDAT) != 0)
            png_chunk_benign_error(png_ptr, "Too many IDATs found");

         png_ptr->mode |= PNG_HAVE_IDAT;
      }

      else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
         png_ptr->mode |= PNG_AFTER_IDAT;

      


      if (chunk_name == png_IHDR)
         png_handle_IHDR(png_ptr, info_ptr, length);

      else if (chunk_name == png_IEND)
         png_handle_IEND(png_ptr, info_ptr, length);

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
      else if ((keep = png_chunk_unknown_handling(png_ptr, chunk_name)) != 0)
      {
         png_handle_unknown(png_ptr, info_ptr, length, keep);

         if (chunk_name == png_PLTE)
            png_ptr->mode |= PNG_HAVE_PLTE;

         else if (chunk_name == png_IDAT)
         {
            png_ptr->idat_size = 0; 
            break;
         }
      }
#endif
      else if (chunk_name == png_PLTE)
         png_handle_PLTE(png_ptr, info_ptr, length);

      else if (chunk_name == png_IDAT)
      {
#ifdef PNG_READ_APNG_SUPPORTED
         png_have_info(png_ptr, info_ptr);
#endif
         png_ptr->idat_size = length;
         break;
      }

#ifdef PNG_READ_bKGD_SUPPORTED
      else if (chunk_name == png_bKGD)
         png_handle_bKGD(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_cHRM_SUPPORTED
      else if (chunk_name == png_cHRM)
         png_handle_cHRM(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_gAMA_SUPPORTED
      else if (chunk_name == png_gAMA)
         png_handle_gAMA(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_hIST_SUPPORTED
      else if (chunk_name == png_hIST)
         png_handle_hIST(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_oFFs_SUPPORTED
      else if (chunk_name == png_oFFs)
         png_handle_oFFs(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_pCAL_SUPPORTED
      else if (chunk_name == png_pCAL)
         png_handle_pCAL(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_sCAL_SUPPORTED
      else if (chunk_name == png_sCAL)
         png_handle_sCAL(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_pHYs_SUPPORTED
      else if (chunk_name == png_pHYs)
         png_handle_pHYs(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_sBIT_SUPPORTED
      else if (chunk_name == png_sBIT)
         png_handle_sBIT(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_sRGB_SUPPORTED
      else if (chunk_name == png_sRGB)
         png_handle_sRGB(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_iCCP_SUPPORTED
      else if (chunk_name == png_iCCP)
         png_handle_iCCP(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_sPLT_SUPPORTED
      else if (chunk_name == png_sPLT)
         png_handle_sPLT(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_tEXt_SUPPORTED
      else if (chunk_name == png_tEXt)
         png_handle_tEXt(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_tIME_SUPPORTED
      else if (chunk_name == png_tIME)
         png_handle_tIME(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_tRNS_SUPPORTED
      else if (chunk_name == png_tRNS)
         png_handle_tRNS(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_zTXt_SUPPORTED
      else if (chunk_name == png_zTXt)
         png_handle_zTXt(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_iTXt_SUPPORTED
      else if (chunk_name == png_iTXt)
         png_handle_iTXt(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_APNG_SUPPORTED
      else if (chunk_name == png_acTL)
         png_handle_acTL(png_ptr, info_ptr, length);

      else if (chunk_name == png_fcTL)
         png_handle_fcTL(png_ptr, info_ptr, length);

      else if (chunk_name == png_fdAT)
         png_handle_fdAT(png_ptr, info_ptr, length);
#endif

      else
         png_handle_unknown(png_ptr, info_ptr, length,
            PNG_HANDLE_CHUNK_AS_DEFAULT);
   }
}
#endif 

#ifdef PNG_READ_APNG_SUPPORTED
void PNGAPI
png_read_frame_head(png_structp png_ptr, png_infop info_ptr)
{
    png_byte have_chunk_after_DAT; 

    png_debug(0, "Reading frame head");

    if ((png_ptr->mode & PNG_HAVE_acTL) == 0)
        png_error(png_ptr, "attempt to png_read_frame_head() but "
                           "no acTL present");

    
    if (png_ptr->num_frames_read == 0)
        return;

    png_read_reset(png_ptr);
    png_ptr->flags &= ~PNG_FLAG_ROW_INIT;
    png_ptr->mode &= ~PNG_HAVE_fcTL;

    have_chunk_after_DAT = 0;
    for (;;)
    {
        png_uint_32 length = png_read_chunk_header(png_ptr);

        if (png_ptr->chunk_name == png_IDAT)
        {
            
            if (have_chunk_after_DAT != 0 || png_ptr->num_frames_read > 1)
                png_error(png_ptr, "png_read_frame_head(): out of place IDAT");
            png_crc_finish(png_ptr, length);
        }

        else if (png_ptr->chunk_name == png_fcTL)
        {
            png_handle_fcTL(png_ptr, info_ptr, length);
            have_chunk_after_DAT = 1;
        }

        else if (png_ptr->chunk_name == png_fdAT)
        {
            png_ensure_sequence_number(png_ptr, length);

            
            if (have_chunk_after_DAT == 0 && png_ptr->num_frames_read > 1)
                png_crc_finish(png_ptr, length - 4);
            else if(png_ptr->mode & PNG_HAVE_fcTL)
            {
                png_ptr->idat_size = length - 4;
                png_ptr->mode |= PNG_HAVE_IDAT;

                break;
            }
            else
                png_error(png_ptr, "png_read_frame_head(): out of place fdAT");
        }
        else
        {
            png_warning(png_ptr, "Skipped (ignored) a chunk "
                                 "between APNG chunks");
            png_crc_finish(png_ptr, length);
        }
    }
}
#endif 


void PNGAPI
png_read_update_info(png_structrp png_ptr, png_inforp info_ptr)
{
   png_debug(1, "in png_read_update_info");

   if (png_ptr != NULL)
   {
      if ((png_ptr->flags & PNG_FLAG_ROW_INIT) == 0)
      {
         png_read_start_row(png_ptr);

#        ifdef PNG_READ_TRANSFORMS_SUPPORTED
            png_read_transform_info(png_ptr, info_ptr);
#        else
            PNG_UNUSED(info_ptr)
#        endif
      }

      
      else
         png_app_error(png_ptr,
            "png_read_update_info/png_start_read_image: duplicate call");
   }
}

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED





void PNGAPI
png_start_read_image(png_structrp png_ptr)
{
   png_debug(1, "in png_start_read_image");

   if (png_ptr != NULL)
   {
      if ((png_ptr->flags & PNG_FLAG_ROW_INIT) == 0)
         png_read_start_row(png_ptr);

      
      else
         png_app_error(png_ptr,
            "png_start_read_image/png_read_update_info: duplicate call");
   }
}
#endif 

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED
#ifdef PNG_MNG_FEATURES_SUPPORTED



static void
png_do_read_intrapixel(png_row_infop row_info, png_bytep row)
{
   png_debug(1, "in png_do_read_intrapixel");

   if (
       (row_info->color_type & PNG_COLOR_MASK_COLOR) != 0)
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
            *(rp) = (png_byte)((256 + *rp + *(rp + 1)) & 0xff);
            *(rp+2) = (png_byte)((256 + *(rp + 2) + *(rp + 1)) & 0xff);
         }
      }
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
            png_uint_32 red  = (s0 + s1 + 65536) & 0xffff;
            png_uint_32 blue = (s2 + s1 + 65536) & 0xffff;
            *(rp    ) = (png_byte)((red >> 8) & 0xff);
            *(rp + 1) = (png_byte)(red & 0xff);
            *(rp + 4) = (png_byte)((blue >> 8) & 0xff);
            *(rp + 5) = (png_byte)(blue & 0xff);
         }
      }
   }
}
#endif 

void PNGAPI
png_read_row(png_structrp png_ptr, png_bytep row, png_bytep dsp_row)
{
   png_row_info row_info;

   if (png_ptr == NULL)
      return;

   png_debug2(1, "in png_read_row (row %lu, pass %d)",
       (unsigned long)png_ptr->row_number, png_ptr->pass);

   


   if ((png_ptr->flags & PNG_FLAG_ROW_INIT) == 0)
      png_read_start_row(png_ptr);

   
   row_info.width = png_ptr->iwidth; 
   row_info.color_type = png_ptr->color_type;
   row_info.bit_depth = png_ptr->bit_depth;
   row_info.channels = png_ptr->channels;
   row_info.pixel_depth = png_ptr->pixel_depth;
   row_info.rowbytes = PNG_ROWBYTES(row_info.pixel_depth, row_info.width);

#ifdef PNG_WARNINGS_SUPPORTED
   if (png_ptr->row_number == 0 && png_ptr->pass == 0)
   {
   
#if defined(PNG_WRITE_INVERT_SUPPORTED) && !defined(PNG_READ_INVERT_SUPPORTED)
   if ((png_ptr->transformations & PNG_INVERT_MONO) != 0)
      png_warning(png_ptr, "PNG_READ_INVERT_SUPPORTED is not defined");
#endif

#if defined(PNG_WRITE_FILLER_SUPPORTED) && !defined(PNG_READ_FILLER_SUPPORTED)
   if ((png_ptr->transformations & PNG_FILLER) != 0)
      png_warning(png_ptr, "PNG_READ_FILLER_SUPPORTED is not defined");
#endif

#if defined(PNG_WRITE_PACKSWAP_SUPPORTED) && \
    !defined(PNG_READ_PACKSWAP_SUPPORTED)
   if ((png_ptr->transformations & PNG_PACKSWAP) != 0)
      png_warning(png_ptr, "PNG_READ_PACKSWAP_SUPPORTED is not defined");
#endif

#if defined(PNG_WRITE_PACK_SUPPORTED) && !defined(PNG_READ_PACK_SUPPORTED)
   if ((png_ptr->transformations & PNG_PACK) != 0)
      png_warning(png_ptr, "PNG_READ_PACK_SUPPORTED is not defined");
#endif

#if defined(PNG_WRITE_SHIFT_SUPPORTED) && !defined(PNG_READ_SHIFT_SUPPORTED)
   if ((png_ptr->transformations & PNG_SHIFT) != 0)
      png_warning(png_ptr, "PNG_READ_SHIFT_SUPPORTED is not defined");
#endif

#if defined(PNG_WRITE_BGR_SUPPORTED) && !defined(PNG_READ_BGR_SUPPORTED)
   if ((png_ptr->transformations & PNG_BGR) != 0)
      png_warning(png_ptr, "PNG_READ_BGR_SUPPORTED is not defined");
#endif

#if defined(PNG_WRITE_SWAP_SUPPORTED) && !defined(PNG_READ_SWAP_SUPPORTED)
   if ((png_ptr->transformations & PNG_SWAP_BYTES) != 0)
      png_warning(png_ptr, "PNG_READ_SWAP_SUPPORTED is not defined");
#endif
   }
#endif 

#ifdef PNG_READ_INTERLACING_SUPPORTED
   





   if (png_ptr->interlaced != 0 &&
       (png_ptr->transformations & PNG_INTERLACE) != 0)
   {
      switch (png_ptr->pass)
      {
         case 0:
            if (png_ptr->row_number & 0x07)
            {
               if (dsp_row != NULL)
                  png_combine_row(png_ptr, dsp_row, 1);
               png_read_finish_row(png_ptr);
               return;
            }
            break;

         case 1:
            if ((png_ptr->row_number & 0x07) || png_ptr->width < 5)
            {
               if (dsp_row != NULL)
                  png_combine_row(png_ptr, dsp_row, 1);

               png_read_finish_row(png_ptr);
               return;
            }
            break;

         case 2:
            if ((png_ptr->row_number & 0x07) != 4)
            {
               if (dsp_row != NULL && (png_ptr->row_number & 4))
                  png_combine_row(png_ptr, dsp_row, 1);

               png_read_finish_row(png_ptr);
               return;
            }
            break;

         case 3:
            if ((png_ptr->row_number & 3) || png_ptr->width < 3)
            {
               if (dsp_row != NULL)
                  png_combine_row(png_ptr, dsp_row, 1);

               png_read_finish_row(png_ptr);
               return;
            }
            break;

         case 4:
            if ((png_ptr->row_number & 3) != 2)
            {
               if (dsp_row != NULL && (png_ptr->row_number & 2))
                  png_combine_row(png_ptr, dsp_row, 1);

               png_read_finish_row(png_ptr);
               return;
            }
            break;

         case 5:
            if ((png_ptr->row_number & 1) || png_ptr->width < 2)
            {
               if (dsp_row != NULL)
                  png_combine_row(png_ptr, dsp_row, 1);

               png_read_finish_row(png_ptr);
               return;
            }
            break;

         default:
         case 6:
            if ((png_ptr->row_number & 1) == 0)
            {
               png_read_finish_row(png_ptr);
               return;
            }
            break;
      }
   }
#endif

   if ((png_ptr->mode & PNG_HAVE_IDAT) == 0)
      png_error(png_ptr, "Invalid attempt to read row data");

   
   png_read_IDAT_data(png_ptr, png_ptr->row_buf, row_info.rowbytes + 1);

   if (png_ptr->row_buf[0] > PNG_FILTER_VALUE_NONE)
   {
      if (png_ptr->row_buf[0] < PNG_FILTER_VALUE_LAST)
         png_read_filter_row(png_ptr, &row_info, png_ptr->row_buf + 1,
            png_ptr->prev_row + 1, png_ptr->row_buf[0]);
      else
         png_error(png_ptr, "bad adaptive filter value");
   }

   




   memcpy(png_ptr->prev_row, png_ptr->row_buf, row_info.rowbytes + 1);

#ifdef PNG_MNG_FEATURES_SUPPORTED
   if ((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) != 0 &&
       (png_ptr->filter_type == PNG_INTRAPIXEL_DIFFERENCING))
   {
      
      png_do_read_intrapixel(&row_info, png_ptr->row_buf + 1);
   }
#endif

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
   if (png_ptr->transformations)
      png_do_read_transformations(png_ptr, &row_info);
#endif

   
   if (png_ptr->transformed_pixel_depth == 0)
   {
      png_ptr->transformed_pixel_depth = row_info.pixel_depth;
      if (row_info.pixel_depth > png_ptr->maximum_pixel_depth)
         png_error(png_ptr, "sequential row overflow");
   }

   else if (png_ptr->transformed_pixel_depth != row_info.pixel_depth)
      png_error(png_ptr, "internal sequential row size calculation error");

#ifdef PNG_READ_INTERLACING_SUPPORTED
   
   if (png_ptr->interlaced != 0 &&
      (png_ptr->transformations & PNG_INTERLACE) != 0)
   {
      if (png_ptr->pass < 6)
         png_do_read_interlace(&row_info, png_ptr->row_buf + 1, png_ptr->pass,
            png_ptr->transformations);

      if (dsp_row != NULL)
         png_combine_row(png_ptr, dsp_row, 1);

      if (row != NULL)
         png_combine_row(png_ptr, row, 0);
   }

   else
#endif
   {
      if (row != NULL)
         png_combine_row(png_ptr, row, -1);

      if (dsp_row != NULL)
         png_combine_row(png_ptr, dsp_row, -1);
   }
   png_read_finish_row(png_ptr);

   if (png_ptr->read_row_fn != NULL)
      (*(png_ptr->read_row_fn))(png_ptr, png_ptr->row_number, png_ptr->pass);

}
#endif 

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED
























void PNGAPI
png_read_rows(png_structrp png_ptr, png_bytepp row,
    png_bytepp display_row, png_uint_32 num_rows)
{
   png_uint_32 i;
   png_bytepp rp;
   png_bytepp dp;

   png_debug(1, "in png_read_rows");

   if (png_ptr == NULL)
      return;

   rp = row;
   dp = display_row;
   if (rp != NULL && dp != NULL)
      for (i = 0; i < num_rows; i++)
      {
         png_bytep rptr = *rp++;
         png_bytep dptr = *dp++;

         png_read_row(png_ptr, rptr, dptr);
      }

   else if (rp != NULL)
      for (i = 0; i < num_rows; i++)
      {
         png_bytep rptr = *rp;
         png_read_row(png_ptr, rptr, NULL);
         rp++;
      }

   else if (dp != NULL)
      for (i = 0; i < num_rows; i++)
      {
         png_bytep dptr = *dp;
         png_read_row(png_ptr, NULL, dptr);
         dp++;
      }
}
#endif 

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED












void PNGAPI
png_read_image(png_structrp png_ptr, png_bytepp image)
{
   png_uint_32 i, image_height;
   int pass, j;
   png_bytepp rp;

   png_debug(1, "in png_read_image");

   if (png_ptr == NULL)
      return;

#ifdef PNG_READ_INTERLACING_SUPPORTED
   if ((png_ptr->flags & PNG_FLAG_ROW_INIT) == 0)
   {
      pass = png_set_interlace_handling(png_ptr);
      
      png_start_read_image(png_ptr);
   }
   else
   {
      if (png_ptr->interlaced != 0 &&
          (png_ptr->transformations & PNG_INTERLACE) == 0)
      {
         



         png_warning(png_ptr, "Interlace handling should be turned on when "
            "using png_read_image");
         
         png_ptr->num_rows = png_ptr->height;
      }

      


      pass = png_set_interlace_handling(png_ptr);
   }
#else
   if (png_ptr->interlaced)
      png_error(png_ptr,
          "Cannot read interlaced image -- interlace handler disabled");

   pass = 1;
#endif

   image_height=png_ptr->height;

   for (j = 0; j < pass; j++)
   {
      rp = image;
      for (i = 0; i < image_height; i++)
      {
         png_read_row(png_ptr, *rp, NULL);
         rp++;
      }
   }
}
#endif 

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED




void PNGAPI
png_read_end(png_structrp png_ptr, png_inforp info_ptr)
{
#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
   int keep;
#endif

   png_debug(1, "in png_read_end");

   if (png_ptr == NULL)
      return;

   


#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
   if (png_chunk_unknown_handling(png_ptr, png_IDAT) == 0)
#endif
      png_read_finish_IDAT(png_ptr);

#ifdef PNG_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED
   
   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
      png_ptr->num_palette_max > png_ptr->num_palette)
     png_benign_error(png_ptr, "Read palette index exceeding num_palette");
#endif

   do
   {
      png_uint_32 length = png_read_chunk_header(png_ptr);
      png_uint_32 chunk_name = png_ptr->chunk_name;

      if (chunk_name == png_IEND)
         png_handle_IEND(png_ptr, info_ptr, length);

      else if (chunk_name == png_IHDR)
         png_handle_IHDR(png_ptr, info_ptr, length);

      else if (info_ptr == NULL)
         png_crc_finish(png_ptr, length);

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
      else if ((keep = png_chunk_unknown_handling(png_ptr, chunk_name)) != 0)
      {
         if (chunk_name == png_IDAT)
         {
            if ((length > 0) ||
                (png_ptr->mode & PNG_HAVE_CHUNK_AFTER_IDAT) != 0)
               png_benign_error(png_ptr, "Too many IDATs found");
         }
         png_handle_unknown(png_ptr, info_ptr, length, keep);
         if (chunk_name == png_PLTE)
            png_ptr->mode |= PNG_HAVE_PLTE;
      }
#endif

      else if (chunk_name == png_IDAT)
      {
         


         if ((length > 0) || (png_ptr->mode & PNG_HAVE_CHUNK_AFTER_IDAT) != 0)
            png_benign_error(png_ptr, "Too many IDATs found");

         png_crc_finish(png_ptr, length);
      }
      else if (chunk_name == png_PLTE)
         png_handle_PLTE(png_ptr, info_ptr, length);

#ifdef PNG_READ_bKGD_SUPPORTED
      else if (chunk_name == png_bKGD)
         png_handle_bKGD(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_cHRM_SUPPORTED
      else if (chunk_name == png_cHRM)
         png_handle_cHRM(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_gAMA_SUPPORTED
      else if (chunk_name == png_gAMA)
         png_handle_gAMA(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_hIST_SUPPORTED
      else if (chunk_name == png_hIST)
         png_handle_hIST(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_oFFs_SUPPORTED
      else if (chunk_name == png_oFFs)
         png_handle_oFFs(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_pCAL_SUPPORTED
      else if (chunk_name == png_pCAL)
         png_handle_pCAL(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_sCAL_SUPPORTED
      else if (chunk_name == png_sCAL)
         png_handle_sCAL(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_pHYs_SUPPORTED
      else if (chunk_name == png_pHYs)
         png_handle_pHYs(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_sBIT_SUPPORTED
      else if (chunk_name == png_sBIT)
         png_handle_sBIT(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_sRGB_SUPPORTED
      else if (chunk_name == png_sRGB)
         png_handle_sRGB(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_iCCP_SUPPORTED
      else if (chunk_name == png_iCCP)
         png_handle_iCCP(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_sPLT_SUPPORTED
      else if (chunk_name == png_sPLT)
         png_handle_sPLT(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_tEXt_SUPPORTED
      else if (chunk_name == png_tEXt)
         png_handle_tEXt(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_tIME_SUPPORTED
      else if (chunk_name == png_tIME)
         png_handle_tIME(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_tRNS_SUPPORTED
      else if (chunk_name == png_tRNS)
         png_handle_tRNS(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_zTXt_SUPPORTED
      else if (chunk_name == png_zTXt)
         png_handle_zTXt(png_ptr, info_ptr, length);
#endif

#ifdef PNG_READ_iTXt_SUPPORTED
      else if (chunk_name == png_iTXt)
         png_handle_iTXt(png_ptr, info_ptr, length);
#endif

      else
         png_handle_unknown(png_ptr, info_ptr, length,
            PNG_HANDLE_CHUNK_AS_DEFAULT);
   } while ((png_ptr->mode & PNG_HAVE_IEND) == 0);
}
#endif 


static void
png_read_destroy(png_structrp png_ptr)
{
   png_debug(1, "in png_read_destroy");

#ifdef PNG_READ_GAMMA_SUPPORTED
   png_destroy_gamma_table(png_ptr);
#endif

   png_free(png_ptr, png_ptr->big_row_buf);
   png_ptr->big_row_buf = NULL;
   png_free(png_ptr, png_ptr->big_prev_row);
   png_ptr->big_prev_row = NULL;
   png_free(png_ptr, png_ptr->read_buffer);
   png_ptr->read_buffer = NULL;

#ifdef PNG_READ_QUANTIZE_SUPPORTED
   png_free(png_ptr, png_ptr->palette_lookup);
   png_ptr->palette_lookup = NULL;
   png_free(png_ptr, png_ptr->quantize_index);
   png_ptr->quantize_index = NULL;
#endif

   if ((png_ptr->free_me & PNG_FREE_PLTE) != 0)
   {
      png_zfree(png_ptr, png_ptr->palette);
      png_ptr->palette = NULL;
   }
   png_ptr->free_me &= ~PNG_FREE_PLTE;

#if defined(PNG_tRNS_SUPPORTED) || \
    defined(PNG_READ_EXPAND_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   if ((png_ptr->free_me & PNG_FREE_TRNS) != 0)
   {
      png_free(png_ptr, png_ptr->trans_alpha);
      png_ptr->trans_alpha = NULL;
   }
   png_ptr->free_me &= ~PNG_FREE_TRNS;
#endif

   inflateEnd(&png_ptr->zstream);

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
   png_free(png_ptr, png_ptr->save_buffer);
   png_ptr->save_buffer = NULL;
#endif

#if defined(PNG_STORE_UNKNOWN_CHUNKS_SUPPORTED) && \
   defined(PNG_READ_UNKNOWN_CHUNKS_SUPPORTED)
   png_free(png_ptr, png_ptr->unknown_chunk.data);
   png_ptr->unknown_chunk.data = NULL;
#endif

#ifdef PNG_SET_UNKNOWN_CHUNKS_SUPPORTED
   png_free(png_ptr, png_ptr->chunk_list);
   png_ptr->chunk_list = NULL;
#endif

   



}


void PNGAPI
png_destroy_read_struct(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr,
    png_infopp end_info_ptr_ptr)
{
   png_structrp png_ptr = NULL;

   png_debug(1, "in png_destroy_read_struct");

   if (png_ptr_ptr != NULL)
      png_ptr = *png_ptr_ptr;

   if (png_ptr == NULL)
      return;

   



   png_destroy_info_struct(png_ptr, end_info_ptr_ptr);
   png_destroy_info_struct(png_ptr, info_ptr_ptr);

   *png_ptr_ptr = NULL;
   png_read_destroy(png_ptr);
   png_destroy_png_struct(png_ptr);
}

void PNGAPI
png_set_read_status_fn(png_structrp png_ptr, png_read_status_ptr read_row_fn)
{
   if (png_ptr == NULL)
      return;

   png_ptr->read_row_fn = read_row_fn;
}


#ifdef PNG_SEQUENTIAL_READ_SUPPORTED
#ifdef PNG_INFO_IMAGE_SUPPORTED
void PNGAPI
png_read_png(png_structrp png_ptr, png_inforp info_ptr,
                           int transforms,
                           voidp params)
{
   if (png_ptr == NULL || info_ptr == NULL)
      return;

   


   png_read_info(png_ptr, info_ptr);
   if (info_ptr->height > PNG_UINT_32_MAX/(sizeof (png_bytep)))
      png_error(png_ptr, "Image is too high to process with png_read_png()");

   
   





   

   if ((transforms & PNG_TRANSFORM_SCALE_16) != 0)
     


#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
      png_set_scale_16(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_SCALE_16 not supported");
#endif

   



   if ((transforms & PNG_TRANSFORM_STRIP_16) != 0)
#ifdef PNG_READ_STRIP_16_TO_8_SUPPORTED
      png_set_strip_16(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_STRIP_16 not supported");
#endif

   


   if ((transforms & PNG_TRANSFORM_STRIP_ALPHA) != 0)
#ifdef PNG_READ_STRIP_ALPHA_SUPPORTED
      png_set_strip_alpha(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_STRIP_ALPHA not supported");
#endif

   


   if ((transforms & PNG_TRANSFORM_PACKING) != 0)
#ifdef PNG_READ_PACK_SUPPORTED
      png_set_packing(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_PACKING not supported");
#endif

   


   if ((transforms & PNG_TRANSFORM_PACKSWAP) != 0)
#ifdef PNG_READ_PACKSWAP_SUPPORTED
      png_set_packswap(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_PACKSWAP not supported");
#endif

   




   if ((transforms & PNG_TRANSFORM_EXPAND) != 0)
#ifdef PNG_READ_EXPAND_SUPPORTED
      png_set_expand(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_EXPAND not supported");
#endif

   


   

   if ((transforms & PNG_TRANSFORM_INVERT_MONO) != 0)
#ifdef PNG_READ_INVERT_SUPPORTED
      png_set_invert_mono(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_INVERT_MONO not supported");
#endif

   



   if ((transforms & PNG_TRANSFORM_SHIFT) != 0)
#ifdef PNG_READ_SHIFT_SUPPORTED
      if ((info_ptr->valid & PNG_INFO_sBIT) != 0)
         png_set_shift(png_ptr, &info_ptr->sig_bit);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_SHIFT not supported");
#endif

   
   if ((transforms & PNG_TRANSFORM_BGR) != 0)
#ifdef PNG_READ_BGR_SUPPORTED
      png_set_bgr(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_BGR not supported");
#endif

   
   if ((transforms & PNG_TRANSFORM_SWAP_ALPHA) != 0)
#ifdef PNG_READ_SWAP_ALPHA_SUPPORTED
      png_set_swap_alpha(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_SWAP_ALPHA not supported");
#endif

   
   if ((transforms & PNG_TRANSFORM_SWAP_ENDIAN) != 0)
#ifdef PNG_READ_SWAP_SUPPORTED
      png_set_swap(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_SWAP_ENDIAN not supported");
#endif


   
   if ((transforms & PNG_TRANSFORM_INVERT_ALPHA) != 0)
#ifdef PNG_READ_INVERT_ALPHA_SUPPORTED
      png_set_invert_alpha(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_INVERT_ALPHA not supported");
#endif


   
   if ((transforms & PNG_TRANSFORM_GRAY_TO_RGB) != 0)
#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED
      png_set_gray_to_rgb(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_GRAY_TO_RGB not supported");
#endif


   if ((transforms & PNG_TRANSFORM_EXPAND_16) != 0)
#ifdef PNG_READ_EXPAND_16_SUPPORTED
      png_set_expand_16(png_ptr);
#else
      png_app_error(png_ptr, "PNG_TRANSFORM_EXPAND_16 not supported");
#endif

   

   


   (void)png_set_interlace_handling(png_ptr);

   



   png_read_update_info(png_ptr, info_ptr);

   

   png_free_data(png_ptr, info_ptr, PNG_FREE_ROWS, 0);
   if (info_ptr->row_pointers == NULL)
   {
      png_uint_32 iptr;

      info_ptr->row_pointers = png_voidcast(png_bytepp, png_malloc(png_ptr,
          info_ptr->height * (sizeof (png_bytep))));

      for (iptr=0; iptr<info_ptr->height; iptr++)
         info_ptr->row_pointers[iptr] = NULL;

      info_ptr->free_me |= PNG_FREE_ROWS;

      for (iptr = 0; iptr < info_ptr->height; iptr++)
         info_ptr->row_pointers[iptr] = png_voidcast(png_bytep,
            png_malloc(png_ptr, info_ptr->rowbytes));
   }

   png_read_image(png_ptr, info_ptr->row_pointers);
   info_ptr->valid |= PNG_INFO_IDAT;

   
   png_read_end(png_ptr, info_ptr);

   PNG_UNUSED(params)
}
#endif 
#endif 

#ifdef PNG_SIMPLIFIED_READ_SUPPORTED








#  define P_NOTSET  0 /* File encoding not yet known */
#  define P_sRGB    1 /* 8-bit encoded to sRGB gamma */
#  define P_LINEAR  2 /* 16-bit linear: not encoded, NOT pre-multiplied! */
#  define P_FILE    3 /* 8-bit encoded to file gamma, not sRGB or linear */
#  define P_LINEAR8 4 /* 8-bit linear: only from a file value */




#define PNG_CMAP_NONE      0
#define PNG_CMAP_GA        1 /* Process GA data to a color-map with alpha */
#define PNG_CMAP_TRANS     2 /* Process GA data to a background index */
#define PNG_CMAP_RGB       3 /* Process RGB data */
#define PNG_CMAP_RGB_ALPHA 4 /* Process RGBA data */


#define PNG_CMAP_NONE_BACKGROUND      256
#define PNG_CMAP_GA_BACKGROUND        231
#define PNG_CMAP_TRANS_BACKGROUND     254
#define PNG_CMAP_RGB_BACKGROUND       256
#define PNG_CMAP_RGB_ALPHA_BACKGROUND 216

typedef struct
{
   
   png_imagep image;
   png_voidp  buffer;
   png_int_32 row_stride;
   png_voidp  colormap;
   png_const_colorp background;
   
   png_voidp       local_row;
   png_voidp       first_row;
   ptrdiff_t       row_bytes;           
   int             file_encoding;       
   png_fixed_point gamma_to_linear;     
   int             colormap_processing; 
} png_image_read_control;






static int
png_image_read_init(png_imagep image)
{
   if (image->opaque == NULL)
   {
      png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, image,
          png_safe_error, png_safe_warning);

      


      memset(image, 0, (sizeof *image));
      image->version = PNG_IMAGE_VERSION;

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
               control->for_write = 0;

               image->opaque = control;
               return 1;
            }

            
            png_destroy_info_struct(png_ptr, &info_ptr);
         }

         png_destroy_read_struct(&png_ptr, NULL, NULL);
      }

      return png_image_error(image, "png_image_read: out of memory");
   }

   return png_image_error(image, "png_image_read: opaque pointer not NULL");
}


static png_uint_32
png_image_format(png_structrp png_ptr)
{
   png_uint_32 format = 0;

   if ((png_ptr->color_type & PNG_COLOR_MASK_COLOR) != 0)
      format |= PNG_FORMAT_FLAG_COLOR;

   if ((png_ptr->color_type & PNG_COLOR_MASK_ALPHA) != 0)
      format |= PNG_FORMAT_FLAG_ALPHA;

   




   else if (png_ptr->num_trans > 0)
      format |= PNG_FORMAT_FLAG_ALPHA;

   if (png_ptr->bit_depth == 16)
      format |= PNG_FORMAT_FLAG_LINEAR;

   if ((png_ptr->color_type & PNG_COLOR_MASK_PALETTE) != 0)
      format |= PNG_FORMAT_FLAG_COLORMAP;

   return format;
}






static int
png_gamma_not_sRGB(png_fixed_point g)
{
   if (g < PNG_FP_1)
   {
      
      if (g == 0)
         return 0;

      return png_gamma_significant((g * 11 + 2)/5 );
   }

   return 1;
}





static int
png_image_read_header(png_voidp argument)
{
   png_imagep image = png_voidcast(png_imagep, argument);
   png_structrp png_ptr = image->opaque->png_ptr;
   png_inforp info_ptr = image->opaque->info_ptr;

   png_set_benign_errors(png_ptr, 1);
   png_read_info(png_ptr, info_ptr);

   
   image->width = png_ptr->width;
   image->height = png_ptr->height;

   {
      png_uint_32 format = png_image_format(png_ptr);

      image->format = format;

#ifdef PNG_COLORSPACE_SUPPORTED
      




      if ((format & PNG_FORMAT_FLAG_COLOR) != 0 && ((png_ptr->colorspace.flags
         & (PNG_COLORSPACE_HAVE_ENDPOINTS|PNG_COLORSPACE_ENDPOINTS_MATCH_sRGB|
            PNG_COLORSPACE_INVALID)) == PNG_COLORSPACE_HAVE_ENDPOINTS))
         image->flags |= PNG_IMAGE_FLAG_COLORSPACE_NOT_sRGB;
#endif
   }

   


   {
      png_uint_32 cmap_entries;

      switch (png_ptr->color_type)
      {
         case PNG_COLOR_TYPE_GRAY:
            cmap_entries = 1U << png_ptr->bit_depth;
            break;

         case PNG_COLOR_TYPE_PALETTE:
            cmap_entries = png_ptr->num_palette;
            break;

         default:
            cmap_entries = 256;
            break;
      }

      if (cmap_entries > 256)
         cmap_entries = 256;

      image->colormap_entries = cmap_entries;
   }

   return 1;
}

#ifdef PNG_STDIO_SUPPORTED
int PNGAPI
png_image_begin_read_from_stdio(png_imagep image, FILE* file)
{
   if (image != NULL && image->version == PNG_IMAGE_VERSION)
   {
      if (file != NULL)
      {
         if (png_image_read_init(image) != 0)
         {
            



            image->opaque->png_ptr->io_ptr = file;
            return png_safe_execute(image, png_image_read_header, image);
         }
      }

      else
         return png_image_error(image,
            "png_image_begin_read_from_stdio: invalid argument");
   }

   else if (image != NULL)
      return png_image_error(image,
         "png_image_begin_read_from_stdio: incorrect PNG_IMAGE_VERSION");

   return 0;
}

int PNGAPI
png_image_begin_read_from_file(png_imagep image, const char *file_name)
{
   if (image != NULL && image->version == PNG_IMAGE_VERSION)
   {
      if (file_name != NULL)
      {
         FILE *fp = fopen(file_name, "rb");

         if (fp != NULL)
         {
            if (png_image_read_init(image) != 0)
            {
               image->opaque->png_ptr->io_ptr = fp;
               image->opaque->owned_file = 1;
               return png_safe_execute(image, png_image_read_header, image);
            }

            
            (void)fclose(fp);
         }

         else
            return png_image_error(image, strerror(errno));
      }

      else
         return png_image_error(image,
            "png_image_begin_read_from_file: invalid argument");
   }

   else if (image != NULL)
      return png_image_error(image,
         "png_image_begin_read_from_file: incorrect PNG_IMAGE_VERSION");

   return 0;
}
#endif 

static void PNGCBAPI
png_image_memory_read(png_structp png_ptr, png_bytep out, png_size_t need)
{
   if (png_ptr != NULL)
   {
      png_imagep image = png_voidcast(png_imagep, png_ptr->io_ptr);
      if (image != NULL)
      {
         png_controlp cp = image->opaque;
         if (cp != NULL)
         {
            png_const_bytep memory = cp->memory;
            png_size_t size = cp->size;

            if (memory != NULL && size >= need)
            {
               memcpy(out, memory, need);
               cp->memory = memory + need;
               cp->size = size - need;
               return;
            }

            png_error(png_ptr, "read beyond end of data");
         }
      }

      png_error(png_ptr, "invalid memory read");
   }
}

int PNGAPI png_image_begin_read_from_memory(png_imagep image,
   png_const_voidp memory, png_size_t size)
{
   if (image != NULL && image->version == PNG_IMAGE_VERSION)
   {
      if (memory != NULL && size > 0)
      {
         if (png_image_read_init(image) != 0)
         {
            



            image->opaque->memory = png_voidcast(png_const_bytep, memory);
            image->opaque->size = size;
            image->opaque->png_ptr->io_ptr = image;
            image->opaque->png_ptr->read_data_fn = png_image_memory_read;

            return png_safe_execute(image, png_image_read_header, image);
         }
      }

      else
         return png_image_error(image,
            "png_image_begin_read_from_memory: invalid argument");
   }

   else if (image != NULL)
      return png_image_error(image,
         "png_image_begin_read_from_memory: incorrect PNG_IMAGE_VERSION");

   return 0;
}




#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
static void
png_image_skip_unused_chunks(png_structrp png_ptr)
{
   

















   {
         static PNG_CONST png_byte chunks_to_process[] = {
            98,  75,  71,  68, '\0',  
            99,  72,  82,  77, '\0',  
           103,  65,  77,  65, '\0',  
#        ifdef PNG_READ_iCCP_SUPPORTED
           105,  67,  67,  80, '\0',  
#        endif
           115,  66,  73,  84, '\0',  
           115,  82,  71,  66, '\0',  
           };

       


       png_set_keep_unknown_chunks(png_ptr, PNG_HANDLE_CHUNK_NEVER,
         NULL, -1);

       
       png_set_keep_unknown_chunks(png_ptr, PNG_HANDLE_CHUNK_AS_DEFAULT,
         chunks_to_process, (int)(sizeof chunks_to_process)/5);
    }
}

#  define PNG_SKIP_CHUNKS(p) png_image_skip_unused_chunks(p)
#else
#  define PNG_SKIP_CHUNKS(p) ((void)0)
#endif 





#define PNG_DIV51(v8) (((v8) * 5 + 130) >> 8)


static void
set_file_encoding(png_image_read_control *display)
{
   png_fixed_point g = display->image->opaque->png_ptr->colorspace.gamma;
   if (png_gamma_significant(g) != 0)
   {
      if (png_gamma_not_sRGB(g) != 0)
      {
         display->file_encoding = P_FILE;
         display->gamma_to_linear = png_reciprocal(g);
      }

      else
         display->file_encoding = P_sRGB;
   }

   else
      display->file_encoding = P_LINEAR8;
}

static unsigned int
decode_gamma(png_image_read_control *display, png_uint_32 value, int encoding)
{
   if (encoding == P_FILE) 
      encoding = display->file_encoding;

   if (encoding == P_NOTSET) 
   {
      set_file_encoding(display);
      encoding = display->file_encoding;
   }

   switch (encoding)
   {
      case P_FILE:
         value = png_gamma_16bit_correct(value*257, display->gamma_to_linear);
         break;

      case P_sRGB:
         value = png_sRGB_table[value];
         break;

      case P_LINEAR:
         break;

      case P_LINEAR8:
         value *= 257;
         break;

      default:
         png_error(display->image->opaque->png_ptr,
            "unexpected encoding (internal error)");
         break;
   }

   return value;
}

static png_uint_32
png_colormap_compose(png_image_read_control *display,
   png_uint_32 foreground, int foreground_encoding, png_uint_32 alpha,
   png_uint_32 background, int encoding)
{
   




   png_uint_32 f = decode_gamma(display, foreground, foreground_encoding);
   png_uint_32 b = decode_gamma(display, background, encoding);

   


   f = f * alpha + b * (255-alpha);

   if (encoding == P_LINEAR)
   {
      


      f *= 257; 
      f += f >> 16;
      f = (f+32768) >> 16;
   }

   else 
      f = PNG_sRGB_FROM_LINEAR(f);

   return f;
}




static void
png_create_colormap_entry(png_image_read_control *display,
   png_uint_32 ip, png_uint_32 red, png_uint_32 green, png_uint_32 blue,
   png_uint_32 alpha, int encoding)
{
   png_imagep image = display->image;
   const int output_encoding = (image->format & PNG_FORMAT_FLAG_LINEAR) != 0 ?
      P_LINEAR : P_sRGB;
   const int convert_to_Y = (image->format & PNG_FORMAT_FLAG_COLOR) == 0 &&
      (red != green || green != blue);

   if (ip > 255)
      png_error(image->opaque->png_ptr, "color-map index out of range");

   


   if (encoding == P_FILE)
   {
      if (display->file_encoding == P_NOTSET)
         set_file_encoding(display);

      


      encoding = display->file_encoding;
   }

   if (encoding == P_FILE)
   {
      png_fixed_point g = display->gamma_to_linear;

      red = png_gamma_16bit_correct(red*257, g);
      green = png_gamma_16bit_correct(green*257, g);
      blue = png_gamma_16bit_correct(blue*257, g);

      if (convert_to_Y != 0 || output_encoding == P_LINEAR)
      {
         alpha *= 257;
         encoding = P_LINEAR;
      }

      else
      {
         red = PNG_sRGB_FROM_LINEAR(red * 255);
         green = PNG_sRGB_FROM_LINEAR(green * 255);
         blue = PNG_sRGB_FROM_LINEAR(blue * 255);
         encoding = P_sRGB;
      }
   }

   else if (encoding == P_LINEAR8)
   {
      


      red *= 257;
      green *= 257;
      blue *= 257;
      alpha *= 257;
      encoding = P_LINEAR;
   }

   else if (encoding == P_sRGB &&
       (convert_to_Y  != 0 || output_encoding == P_LINEAR))
   {
      


      red = png_sRGB_table[red];
      green = png_sRGB_table[green];
      blue = png_sRGB_table[blue];
      alpha *= 257;
      encoding = P_LINEAR;
   }

   
   if (encoding == P_LINEAR)
   {
      if (convert_to_Y != 0)
      {
         
         png_uint_32 y = (png_uint_32)6968 * red  + (png_uint_32)23434 * green +
            (png_uint_32)2366 * blue;

         if (output_encoding == P_LINEAR)
            y = (y + 16384) >> 15;

         else
         {
            
            y = (y + 128) >> 8;
            y *= 255;
            y = PNG_sRGB_FROM_LINEAR((y + 64) >> 7);
            alpha = PNG_DIV257(alpha);
            encoding = P_sRGB;
         }

         blue = red = green = y;
      }

      else if (output_encoding == P_sRGB)
      {
         red = PNG_sRGB_FROM_LINEAR(red * 255);
         green = PNG_sRGB_FROM_LINEAR(green * 255);
         blue = PNG_sRGB_FROM_LINEAR(blue * 255);
         alpha = PNG_DIV257(alpha);
         encoding = P_sRGB;
      }
   }

   if (encoding != output_encoding)
      png_error(image->opaque->png_ptr, "bad encoding (internal error)");

   
   {
#     ifdef PNG_FORMAT_AFIRST_SUPPORTED
         const int afirst = (image->format & PNG_FORMAT_FLAG_AFIRST) != 0 &&
            (image->format & PNG_FORMAT_FLAG_ALPHA) != 0;
#     else
#        define afirst 0
#     endif
#     ifdef PNG_FORMAT_BGR_SUPPORTED
         const int bgr = (image->format & PNG_FORMAT_FLAG_BGR) != 0 ? 2 : 0;
#     else
#        define bgr 0
#     endif

      if (output_encoding == P_LINEAR)
      {
         png_uint_16p entry = png_voidcast(png_uint_16p, display->colormap);

         entry += ip * PNG_IMAGE_SAMPLE_CHANNELS(image->format);

         



         switch (PNG_IMAGE_SAMPLE_CHANNELS(image->format))
         {
            case 4:
               entry[afirst ? 0 : 3] = (png_uint_16)alpha;
               

            case 3:
               if (alpha < 65535)
               {
                  if (alpha > 0)
                  {
                     blue = (blue * alpha + 32767U)/65535U;
                     green = (green * alpha + 32767U)/65535U;
                     red = (red * alpha + 32767U)/65535U;
                  }

                  else
                     red = green = blue = 0;
               }
               entry[afirst + (2 ^ bgr)] = (png_uint_16)blue;
               entry[afirst + 1] = (png_uint_16)green;
               entry[afirst + bgr] = (png_uint_16)red;
               break;

            case 2:
               entry[1 ^ afirst] = (png_uint_16)alpha;
               

            case 1:
               if (alpha < 65535)
               {
                  if (alpha > 0)
                     green = (green * alpha + 32767U)/65535U;

                  else
                     green = 0;
               }
               entry[afirst] = (png_uint_16)green;
               break;

            default:
               break;
         }
      }

      else 
      {
         png_bytep entry = png_voidcast(png_bytep, display->colormap);

         entry += ip * PNG_IMAGE_SAMPLE_CHANNELS(image->format);

         switch (PNG_IMAGE_SAMPLE_CHANNELS(image->format))
         {
            case 4:
               entry[afirst ? 0 : 3] = (png_byte)alpha;
            case 3:
               entry[afirst + (2 ^ bgr)] = (png_byte)blue;
               entry[afirst + 1] = (png_byte)green;
               entry[afirst + bgr] = (png_byte)red;
               break;

            case 2:
               entry[1 ^ afirst] = (png_byte)alpha;
            case 1:
               entry[afirst] = (png_byte)green;
               break;

            default:
               break;
         }
      }

#     ifdef afirst
#        undef afirst
#     endif
#     ifdef bgr
#        undef bgr
#     endif
   }
}

static int
make_gray_file_colormap(png_image_read_control *display)
{
   unsigned int i;

   for (i=0; i<256; ++i)
      png_create_colormap_entry(display, i, i, i, i, 255, P_FILE);

   return i;
}

static int
make_gray_colormap(png_image_read_control *display)
{
   unsigned int i;

   for (i=0; i<256; ++i)
      png_create_colormap_entry(display, i, i, i, i, 255, P_sRGB);

   return i;
}
#define PNG_GRAY_COLORMAP_ENTRIES 256

static int
make_ga_colormap(png_image_read_control *display)
{
   unsigned int i, a;

   























   i = 0;
   while (i < 231)
   {
      unsigned int gray = (i * 256 + 115) / 231;
      png_create_colormap_entry(display, i++, gray, gray, gray, 255, P_sRGB);
   }

   


   png_create_colormap_entry(display, i++, 255, 255, 255, 0, P_sRGB);

   for (a=1; a<5; ++a)
   {
      unsigned int g;

      for (g=0; g<6; ++g)
         png_create_colormap_entry(display, i++, g*51, g*51, g*51, a*51,
            P_sRGB);
   }

   return i;
}

#define PNG_GA_COLORMAP_ENTRIES 256

static int
make_rgb_colormap(png_image_read_control *display)
{
   unsigned int i, r;

   
   for (i=r=0; r<6; ++r)
   {
      unsigned int g;

      for (g=0; g<6; ++g)
      {
         unsigned int b;

         for (b=0; b<6; ++b)
            png_create_colormap_entry(display, i++, r*51, g*51, b*51, 255,
               P_sRGB);
      }
   }

   return i;
}

#define PNG_RGB_COLORMAP_ENTRIES 216


#define PNG_RGB_INDEX(r,g,b) \
   ((png_byte)(6 * (6 * PNG_DIV51(r) + PNG_DIV51(g)) + PNG_DIV51(b)))

static int
png_image_read_colormap(png_voidp argument)
{
   png_image_read_control *display =
      png_voidcast(png_image_read_control*, argument);
   const png_imagep image = display->image;

   const png_structrp png_ptr = image->opaque->png_ptr;
   const png_uint_32 output_format = image->format;
   const int output_encoding = (output_format & PNG_FORMAT_FLAG_LINEAR) != 0 ?
      P_LINEAR : P_sRGB;

   unsigned int cmap_entries;
   unsigned int output_processing;        
   unsigned int data_encoding = P_NOTSET; 

   


   unsigned int background_index = 256;
   png_uint_32 back_r, back_g, back_b;

   
   int expand_tRNS = 0;

   




   if (((png_ptr->color_type & PNG_COLOR_MASK_ALPHA) != 0 ||
         png_ptr->num_trans > 0)  &&
      ((output_format & PNG_FORMAT_FLAG_ALPHA) == 0) )
   {
      if (output_encoding == P_LINEAR) 
         back_b = back_g = back_r = 0;

      else if (display->background == NULL )
         png_error(png_ptr,
            "a background color must be supplied to remove alpha/transparency");

      



      else
      {
         back_g = display->background->green;
         if ((output_format & PNG_FORMAT_FLAG_COLOR) != 0)
         {
            back_r = display->background->red;
            back_b = display->background->blue;
         }
         else
            back_b = back_r = back_g;
      }
   }

   else if (output_encoding == P_LINEAR)
      back_b = back_r = back_g = 65535;

   else
      back_b = back_r = back_g = 255;

   




   if ((png_ptr->colorspace.flags & PNG_COLORSPACE_HAVE_GAMMA) == 0)
   {
      




      if (png_ptr->bit_depth == 16 &&
         (image->flags & PNG_IMAGE_FLAG_16BIT_sRGB) == 0)
         png_ptr->colorspace.gamma = PNG_GAMMA_LINEAR;

      else
         png_ptr->colorspace.gamma = PNG_GAMMA_sRGB_INVERSE;

      png_ptr->colorspace.flags |= PNG_COLORSPACE_HAVE_GAMMA;
   }

   




   switch (png_ptr->color_type)
   {
      case PNG_COLOR_TYPE_GRAY:
         if (png_ptr->bit_depth <= 8)
         {
            


            unsigned int step, i, val, trans = 256, back_alpha = 0;

            cmap_entries = 1U << png_ptr->bit_depth;
            if (cmap_entries > image->colormap_entries)
               png_error(png_ptr, "gray[8] color-map: too few entries");

            step = 255 / (cmap_entries - 1);
            output_processing = PNG_CMAP_NONE;

            


            if (png_ptr->num_trans > 0)
            {
               trans = png_ptr->trans_color.gray;

               if ((output_format & PNG_FORMAT_FLAG_ALPHA) == 0)
                  back_alpha = output_encoding == P_LINEAR ? 65535 : 255;
            }

            






            for (i=val=0; i<cmap_entries; ++i, val += step)
            {
               



               if (i != trans)
                  png_create_colormap_entry(display, i, val, val, val, 255,
                     P_FILE);

               







               else
                  png_create_colormap_entry(display, i, back_r, back_g, back_b,
                     back_alpha, output_encoding);
            }

            
            data_encoding = P_FILE;

            




            if (png_ptr->bit_depth < 8)
               png_set_packing(png_ptr);
         }

         else 
         {
            















            data_encoding = P_sRGB;

            if (PNG_GRAY_COLORMAP_ENTRIES > image->colormap_entries)
               png_error(png_ptr, "gray[16] color-map: too few entries");

            cmap_entries = make_gray_colormap(display);

            if (png_ptr->num_trans > 0)
            {
               unsigned int back_alpha;

               if ((output_format & PNG_FORMAT_FLAG_ALPHA) != 0)
                  back_alpha = 0;

               else
               {
                  if (back_r == back_g && back_g == back_b)
                  {
                     


                     png_color_16 c;
                     png_uint_32 gray = back_g;

                     if (output_encoding == P_LINEAR)
                     {
                        gray = PNG_sRGB_FROM_LINEAR(gray * 255);

                        


                        png_create_colormap_entry(display, gray, back_g, back_g,
                           back_g, 65535, P_LINEAR);
                     }

                     


                     c.index = 0; 
                     c.gray = c.red = c.green = c.blue = (png_uint_16)gray;

                     



                     png_set_background_fixed(png_ptr, &c,
                        PNG_BACKGROUND_GAMMA_SCREEN, 0,
                        0);

                     output_processing = PNG_CMAP_NONE;
                     break;
                  }
#ifdef __COVERITY__
                 


                  back_alpha = 255;
#else
                  back_alpha = output_encoding == P_LINEAR ? 65535 : 255;
#endif
               }

               






               expand_tRNS = 1;
               output_processing = PNG_CMAP_TRANS;
               background_index = 254;

               


               png_create_colormap_entry(display, 254, back_r, back_g, back_b,
                  back_alpha, output_encoding);
            }

            else
               output_processing = PNG_CMAP_NONE;
         }
         break;

      case PNG_COLOR_TYPE_GRAY_ALPHA:
         









         data_encoding = P_sRGB;

         if ((output_format & PNG_FORMAT_FLAG_ALPHA) != 0)
         {
            if (PNG_GA_COLORMAP_ENTRIES > image->colormap_entries)
               png_error(png_ptr, "gray+alpha color-map: too few entries");

            cmap_entries = make_ga_colormap(display);

            background_index = PNG_CMAP_GA_BACKGROUND;
            output_processing = PNG_CMAP_GA;
         }

         else 
         {
            















            if ((output_format & PNG_FORMAT_FLAG_COLOR) == 0 ||
               (back_r == back_g && back_g == back_b))
            {
               
               png_color_16 c;
               png_uint_32 gray = back_g;

               if (PNG_GRAY_COLORMAP_ENTRIES > image->colormap_entries)
                  png_error(png_ptr, "gray-alpha color-map: too few entries");

               cmap_entries = make_gray_colormap(display);

               if (output_encoding == P_LINEAR)
               {
                  gray = PNG_sRGB_FROM_LINEAR(gray * 255);

                  
                  png_create_colormap_entry(display, gray, back_g, back_g,
                     back_g, 65535, P_LINEAR);
               }

               


               c.index = 0; 
               c.gray = c.red = c.green = c.blue = (png_uint_16)gray;

               png_set_background_fixed(png_ptr, &c,
                  PNG_BACKGROUND_GAMMA_SCREEN, 0,
                  0);

               output_processing = PNG_CMAP_NONE;
            }

            else
            {
               png_uint_32 i, a;

               


               if (PNG_GA_COLORMAP_ENTRIES > image->colormap_entries)
                  png_error(png_ptr, "ga-alpha color-map: too few entries");

               i = 0;
               while (i < 231)
               {
                  png_uint_32 gray = (i * 256 + 115) / 231;
                  png_create_colormap_entry(display, i++, gray, gray, gray,
                     255, P_sRGB);
               }

               


               background_index = i;
               png_create_colormap_entry(display, i++, back_r, back_g, back_b,
#ifdef __COVERITY__
                 

 255U,
#else
                  output_encoding == P_LINEAR ? 65535U : 255U,
#endif
                  output_encoding);

               







               if (output_encoding == P_sRGB) 
               {
                  



                  back_r = png_sRGB_table[back_r];
                  back_g = png_sRGB_table[back_g];
                  back_b = png_sRGB_table[back_b];
               }

               for (a=1; a<5; ++a)
               {
                  unsigned int g;

                  


                  png_uint_32 alpha = 51 * a;
                  png_uint_32 back_rx = (255-alpha) * back_r;
                  png_uint_32 back_gx = (255-alpha) * back_g;
                  png_uint_32 back_bx = (255-alpha) * back_b;

                  for (g=0; g<6; ++g)
                  {
                     png_uint_32 gray = png_sRGB_table[g*51] * alpha;

                     png_create_colormap_entry(display, i++,
                        PNG_sRGB_FROM_LINEAR(gray + back_rx),
                        PNG_sRGB_FROM_LINEAR(gray + back_gx),
                        PNG_sRGB_FROM_LINEAR(gray + back_bx), 255, P_sRGB);
                  }
               }

               cmap_entries = i;
               output_processing = PNG_CMAP_GA;
            }
         }
         break;

      case PNG_COLOR_TYPE_RGB:
      case PNG_COLOR_TYPE_RGB_ALPHA:
         


         if ((output_format & PNG_FORMAT_FLAG_COLOR) == 0)
         {
            







            png_set_rgb_to_gray_fixed(png_ptr, PNG_ERROR_ACTION_NONE, -1,
               -1);
            data_encoding = P_sRGB;

            


            if ((png_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
               png_ptr->num_trans > 0) &&
               (output_format & PNG_FORMAT_FLAG_ALPHA) != 0)
            {
               



               expand_tRNS = 1;

               if (PNG_GA_COLORMAP_ENTRIES > image->colormap_entries)
                  png_error(png_ptr, "rgb[ga] color-map: too few entries");

               cmap_entries = make_ga_colormap(display);
               background_index = PNG_CMAP_GA_BACKGROUND;
               output_processing = PNG_CMAP_GA;
            }

            else
            {
               



               if (PNG_GRAY_COLORMAP_ENTRIES > image->colormap_entries)
                  png_error(png_ptr, "rgb[gray] color-map: too few entries");

               







               if ((png_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
                  png_ptr->num_trans > 0) &&
                  png_gamma_not_sRGB(png_ptr->colorspace.gamma) != 0)
               {
                  cmap_entries = make_gray_file_colormap(display);
                  data_encoding = P_FILE;
               }

               else
                  cmap_entries = make_gray_colormap(display);

               

               if (png_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
                  png_ptr->num_trans > 0)
               {
                  png_color_16 c;
                  png_uint_32 gray = back_g;

                  




                  if (data_encoding == P_FILE) 
                  {
                     




                     if (output_encoding == P_sRGB)
                        gray = png_sRGB_table[gray]; 

                     gray = PNG_DIV257(png_gamma_16bit_correct(gray,
                        png_ptr->colorspace.gamma)); 

                     


                     png_create_colormap_entry(display, gray, back_g, back_g,
                        back_g, 0, output_encoding);
                  }

                  else if (output_encoding == P_LINEAR)
                  {
                     gray = PNG_sRGB_FROM_LINEAR(gray * 255);

                     

                     png_create_colormap_entry(display, gray, back_g, back_g,
                        back_g, 0, P_LINEAR);
                  }

                  


                  c.index = 0; 
                  c.gray = c.red = c.green = c.blue = (png_uint_16)gray;

                  



                  expand_tRNS = 1;
                  png_set_background_fixed(png_ptr, &c,
                     PNG_BACKGROUND_GAMMA_SCREEN, 0,
                     0);
               }

               output_processing = PNG_CMAP_NONE;
            }
         }

         else 
         {
            




            data_encoding = P_sRGB;

            
            if (png_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
               png_ptr->num_trans > 0)
            {
               


               if ((output_format & PNG_FORMAT_FLAG_ALPHA) != 0)
               {
                  png_uint_32 r;

                  if (PNG_RGB_COLORMAP_ENTRIES+1+27 > image->colormap_entries)
                     png_error(png_ptr, "rgb+alpha color-map: too few entries");

                  cmap_entries = make_rgb_colormap(display);

                  
                  png_create_colormap_entry(display, cmap_entries, 255, 255,
                     255, 0, P_sRGB);

                  


                  background_index = cmap_entries++;

                  
                  for (r=0; r<256; r = (r << 1) | 0x7f)
                  {
                     png_uint_32 g;

                     for (g=0; g<256; g = (g << 1) | 0x7f)
                     {
                        png_uint_32 b;

                        


                        for (b=0; b<256; b = (b << 1) | 0x7f)
                           png_create_colormap_entry(display, cmap_entries++,
                              r, g, b, 128, P_sRGB);
                     }
                  }

                  expand_tRNS = 1;
                  output_processing = PNG_CMAP_RGB_ALPHA;
               }

               else
               {
                  






                  unsigned int sample_size =
                     PNG_IMAGE_SAMPLE_SIZE(output_format);
                  png_uint_32 r, g, b; 

                  if (PNG_RGB_COLORMAP_ENTRIES+1+27 > image->colormap_entries)
                     png_error(png_ptr, "rgb-alpha color-map: too few entries");

                  cmap_entries = make_rgb_colormap(display);

                  png_create_colormap_entry(display, cmap_entries, back_r,
                        back_g, back_b, 0, output_encoding);

                  if (output_encoding == P_LINEAR)
                  {
                     r = PNG_sRGB_FROM_LINEAR(back_r * 255);
                     g = PNG_sRGB_FROM_LINEAR(back_g * 255);
                     b = PNG_sRGB_FROM_LINEAR(back_b * 255);
                  }

                  else
                  {
                     r = back_r;
                     g = back_g;
                     b = back_g;
                  }

                  




                  if (memcmp((png_const_bytep)display->colormap +
                        sample_size * cmap_entries,
                     (png_const_bytep)display->colormap +
                        sample_size * PNG_RGB_INDEX(r,g,b),
                     sample_size) != 0)
                  {
                     
                     background_index = cmap_entries++;

                     


                     for (r=0; r<256; r = (r << 1) | 0x7f)
                     {
                        for (g=0; g<256; g = (g << 1) | 0x7f)
                        {
                           


                           for (b=0; b<256; b = (b << 1) | 0x7f)
                              png_create_colormap_entry(display, cmap_entries++,
                                 png_colormap_compose(display, r, P_sRGB, 128,
                                    back_r, output_encoding),
                                 png_colormap_compose(display, g, P_sRGB, 128,
                                    back_g, output_encoding),
                                 png_colormap_compose(display, b, P_sRGB, 128,
                                    back_b, output_encoding),
                                 0, output_encoding);
                        }
                     }

                     expand_tRNS = 1;
                     output_processing = PNG_CMAP_RGB_ALPHA;
                  }

                  else 
                  {
                     png_color_16 c;

                     c.index = 0; 
                     c.red = (png_uint_16)back_r;
                     c.gray = c.green = (png_uint_16)back_g;
                     c.blue = (png_uint_16)back_b;

                     png_set_background_fixed(png_ptr, &c,
                        PNG_BACKGROUND_GAMMA_SCREEN, 0,
                        0);

                     output_processing = PNG_CMAP_RGB;
                  }
               }
            }

            else 
            {
               


               if (PNG_RGB_COLORMAP_ENTRIES > image->colormap_entries)
                  png_error(png_ptr, "rgb color-map: too few entries");

               cmap_entries = make_rgb_colormap(display);
               output_processing = PNG_CMAP_RGB;
            }
         }
         break;

      case PNG_COLOR_TYPE_PALETTE:
         


         {
            unsigned int num_trans = png_ptr->num_trans;
            png_const_bytep trans = num_trans > 0 ? png_ptr->trans_alpha : NULL;
            png_const_colorp colormap = png_ptr->palette;
            const int do_background = trans != NULL &&
               (output_format & PNG_FORMAT_FLAG_ALPHA) == 0;
            unsigned int i;

            
            if (trans == NULL)
               num_trans = 0;

            output_processing = PNG_CMAP_NONE;
            data_encoding = P_FILE; 
            cmap_entries = png_ptr->num_palette;
            if (cmap_entries > 256)
               cmap_entries = 256;

            if (cmap_entries > image->colormap_entries)
               png_error(png_ptr, "palette color-map: too few entries");

            for (i=0; i < cmap_entries; ++i)
            {
               if (do_background != 0 && i < num_trans && trans[i] < 255)
               {
                  if (trans[i] == 0)
                     png_create_colormap_entry(display, i, back_r, back_g,
                        back_b, 0, output_encoding);

                  else
                  {
                     


                     png_create_colormap_entry(display, i,
                        png_colormap_compose(display, colormap[i].red, P_FILE,
                           trans[i], back_r, output_encoding),
                        png_colormap_compose(display, colormap[i].green, P_FILE,
                           trans[i], back_g, output_encoding),
                        png_colormap_compose(display, colormap[i].blue, P_FILE,
                           trans[i], back_b, output_encoding),
                        output_encoding == P_LINEAR ? trans[i] * 257U :
                           trans[i],
                        output_encoding);
                  }
               }

               else
                  png_create_colormap_entry(display, i, colormap[i].red,
                     colormap[i].green, colormap[i].blue,
                     i < num_trans ? trans[i] : 255U, P_FILE);
            }

            


            if (png_ptr->bit_depth < 8)
               png_set_packing(png_ptr);
         }
         break;

      default:
         png_error(png_ptr, "invalid PNG color type");
         
         break;
   }

   
   if (expand_tRNS != 0 && png_ptr->num_trans > 0 &&
       (png_ptr->color_type & PNG_COLOR_MASK_ALPHA) == 0)
      png_set_tRNS_to_alpha(png_ptr);

   switch (data_encoding)
   {
      default:
         png_error(png_ptr, "bad data option (internal error)");
         break;

      case P_sRGB:
         
         png_set_alpha_mode_fixed(png_ptr, PNG_ALPHA_PNG, PNG_GAMMA_sRGB);
         

      case P_FILE:
         if (png_ptr->bit_depth > 8)
            png_set_scale_16(png_ptr);
         break;
   }

   if (cmap_entries > 256 || cmap_entries > image->colormap_entries)
      png_error(png_ptr, "color map overflow (BAD internal error)");

   image->colormap_entries = cmap_entries;

   
   switch (output_processing)
   {
      case PNG_CMAP_NONE:
         if (background_index != PNG_CMAP_NONE_BACKGROUND)
            goto bad_background;
         break;

      case PNG_CMAP_GA:
         if (background_index != PNG_CMAP_GA_BACKGROUND)
            goto bad_background;
         break;

      case PNG_CMAP_TRANS:
         if (background_index >= cmap_entries ||
            background_index != PNG_CMAP_TRANS_BACKGROUND)
            goto bad_background;
         break;

      case PNG_CMAP_RGB:
         if (background_index != PNG_CMAP_RGB_BACKGROUND)
            goto bad_background;
         break;

      case PNG_CMAP_RGB_ALPHA:
         if (background_index != PNG_CMAP_RGB_ALPHA_BACKGROUND)
            goto bad_background;
         break;

      default:
         png_error(png_ptr, "bad processing option (internal error)");

      bad_background:
         png_error(png_ptr, "bad background index (internal error)");
   }

   display->colormap_processing = output_processing;

   return 1;
}


static int
png_image_read_and_map(png_voidp argument)
{
   png_image_read_control *display = png_voidcast(png_image_read_control*,
      argument);
   png_imagep image = display->image;
   png_structrp png_ptr = image->opaque->png_ptr;
   int passes;

   



   switch (png_ptr->interlaced)
   {
      case PNG_INTERLACE_NONE:
         passes = 1;
         break;

      case PNG_INTERLACE_ADAM7:
         passes = PNG_INTERLACE_ADAM7_PASSES;
         break;

      default:
         png_error(png_ptr, "unknown interlace type");
   }

   {
      png_uint_32  height = image->height;
      png_uint_32  width = image->width;
      int          proc = display->colormap_processing;
      png_bytep    first_row = png_voidcast(png_bytep, display->first_row);
      ptrdiff_t    step_row = display->row_bytes;
      int pass;

      for (pass = 0; pass < passes; ++pass)
      {
         unsigned int     startx, stepx, stepy;
         png_uint_32      y;

         if (png_ptr->interlaced == PNG_INTERLACE_ADAM7)
         {
            
            if (PNG_PASS_COLS(width, pass) == 0)
               continue;

            startx = PNG_PASS_START_COL(pass);
            stepx = PNG_PASS_COL_OFFSET(pass);
            y = PNG_PASS_START_ROW(pass);
            stepy = PNG_PASS_ROW_OFFSET(pass);
         }

         else
         {
            y = 0;
            startx = 0;
            stepx = stepy = 1;
         }

         for (; y<height; y += stepy)
         {
            png_bytep inrow = png_voidcast(png_bytep, display->local_row);
            png_bytep outrow = first_row + y * step_row;
            png_const_bytep end_row = outrow + width;

            
            png_read_row(png_ptr, inrow, NULL);

            



            outrow += startx;
            switch (proc)
            {
               case PNG_CMAP_GA:
                  for (; outrow < end_row; outrow += stepx)
                  {
                     
                     unsigned int gray = *inrow++;
                     unsigned int alpha = *inrow++;
                     unsigned int entry;

                     



                     if (alpha > 229) 
                     {
                        entry = (231 * gray + 128) >> 8;
                     }
                     else if (alpha < 26) 
                     {
                        entry = 231;
                     }
                     else 
                     {
                        entry = 226 + 6 * PNG_DIV51(alpha) + PNG_DIV51(gray);
                     }

                     *outrow = (png_byte)entry;
                  }
                  break;

               case PNG_CMAP_TRANS:
                  for (; outrow < end_row; outrow += stepx)
                  {
                     png_byte gray = *inrow++;
                     png_byte alpha = *inrow++;

                     if (alpha == 0)
                        *outrow = PNG_CMAP_TRANS_BACKGROUND;

                     else if (gray != PNG_CMAP_TRANS_BACKGROUND)
                        *outrow = gray;

                     else
                        *outrow = (png_byte)(PNG_CMAP_TRANS_BACKGROUND+1);
                  }
                  break;

               case PNG_CMAP_RGB:
                  for (; outrow < end_row; outrow += stepx)
                  {
                     *outrow = PNG_RGB_INDEX(inrow[0], inrow[1], inrow[2]);
                     inrow += 3;
                  }
                  break;

               case PNG_CMAP_RGB_ALPHA:
                  for (; outrow < end_row; outrow += stepx)
                  {
                     unsigned int alpha = inrow[3];

                     




                     if (alpha >= 196)
                        *outrow = PNG_RGB_INDEX(inrow[0], inrow[1],
                           inrow[2]);

                     else if (alpha < 64)
                        *outrow = PNG_CMAP_RGB_ALPHA_BACKGROUND;

                     else
                     {
                        





                        unsigned int back_i = PNG_CMAP_RGB_ALPHA_BACKGROUND+1;

                        








                        if (inrow[0] & 0x80) back_i += 9; 
                        if (inrow[0] & 0x40) back_i += 9;
                        if (inrow[0] & 0x80) back_i += 3; 
                        if (inrow[0] & 0x40) back_i += 3;
                        if (inrow[0] & 0x80) back_i += 1; 
                        if (inrow[0] & 0x40) back_i += 1;

                        *outrow = (png_byte)back_i;
                     }

                     inrow += 4;
                  }
                  break;

               default:
                  break;
            }
         }
      }
   }

   return 1;
}

static int
png_image_read_colormapped(png_voidp argument)
{
   png_image_read_control *display = png_voidcast(png_image_read_control*,
      argument);
   png_imagep image = display->image;
   png_controlp control = image->opaque;
   png_structrp png_ptr = control->png_ptr;
   png_inforp info_ptr = control->info_ptr;

   int passes = 0; 

   PNG_SKIP_CHUNKS(png_ptr);

   



   if (display->colormap_processing == PNG_CMAP_NONE)
      passes = png_set_interlace_handling(png_ptr);

   png_read_update_info(png_ptr, info_ptr);

   
   switch (display->colormap_processing)
   {
      case PNG_CMAP_NONE:
         


         if ((info_ptr->color_type == PNG_COLOR_TYPE_PALETTE ||
            info_ptr->color_type == PNG_COLOR_TYPE_GRAY) &&
            info_ptr->bit_depth == 8)
            break;

         goto bad_output;

      case PNG_CMAP_TRANS:
      case PNG_CMAP_GA:
         



         if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA &&
            info_ptr->bit_depth == 8 &&
            png_ptr->screen_gamma == PNG_GAMMA_sRGB &&
            image->colormap_entries == 256)
            break;

         goto bad_output;

      case PNG_CMAP_RGB:
         
         if (info_ptr->color_type == PNG_COLOR_TYPE_RGB &&
            info_ptr->bit_depth == 8 &&
            png_ptr->screen_gamma == PNG_GAMMA_sRGB &&
            image->colormap_entries == 216)
            break;

         goto bad_output;

      case PNG_CMAP_RGB_ALPHA:
         
         if (info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA &&
            info_ptr->bit_depth == 8 &&
            png_ptr->screen_gamma == PNG_GAMMA_sRGB &&
            image->colormap_entries == 244 )
            break;

         
         

      default:
      bad_output:
         png_error(png_ptr, "bad color-map processing (internal error)");
   }

   



   {
      png_voidp first_row = display->buffer;
      ptrdiff_t row_bytes = display->row_stride;

      


      if (row_bytes < 0)
      {
         char *ptr = png_voidcast(char*, first_row);
         ptr += (image->height-1) * (-row_bytes);
         first_row = png_voidcast(png_voidp, ptr);
      }

      display->first_row = first_row;
      display->row_bytes = row_bytes;
   }

   if (passes == 0)
   {
      int result;
      png_voidp row = png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));

      display->local_row = row;
      result = png_safe_execute(image, png_image_read_and_map, display);
      display->local_row = NULL;
      png_free(png_ptr, row);

      return result;
   }

   else
   {
      png_alloc_size_t row_bytes = display->row_bytes;

      while (--passes >= 0)
      {
         png_uint_32      y = image->height;
         png_bytep        row = png_voidcast(png_bytep, display->first_row);

         while (y-- > 0)
         {
            png_read_row(png_ptr, row, NULL);
            row += row_bytes;
         }
      }

      return 1;
   }
}


static int
png_image_read_composite(png_voidp argument)
{
   png_image_read_control *display = png_voidcast(png_image_read_control*,
      argument);
   png_imagep image = display->image;
   png_structrp png_ptr = image->opaque->png_ptr;
   int passes;

   switch (png_ptr->interlaced)
   {
      case PNG_INTERLACE_NONE:
         passes = 1;
         break;

      case PNG_INTERLACE_ADAM7:
         passes = PNG_INTERLACE_ADAM7_PASSES;
         break;

      default:
         png_error(png_ptr, "unknown interlace type");
   }

   {
      png_uint_32  height = image->height;
      png_uint_32  width = image->width;
      ptrdiff_t    step_row = display->row_bytes;
      unsigned int channels =
          (image->format & PNG_FORMAT_FLAG_COLOR) != 0 ? 3 : 1;
      int pass;

      for (pass = 0; pass < passes; ++pass)
      {
         unsigned int     startx, stepx, stepy;
         png_uint_32      y;

         if (png_ptr->interlaced == PNG_INTERLACE_ADAM7)
         {
            
            if (PNG_PASS_COLS(width, pass) == 0)
               continue;

            startx = PNG_PASS_START_COL(pass) * channels;
            stepx = PNG_PASS_COL_OFFSET(pass) * channels;
            y = PNG_PASS_START_ROW(pass);
            stepy = PNG_PASS_ROW_OFFSET(pass);
         }

         else
         {
            y = 0;
            startx = 0;
            stepx = channels;
            stepy = 1;
         }

         for (; y<height; y += stepy)
         {
            png_bytep inrow = png_voidcast(png_bytep, display->local_row);
            png_bytep outrow;
            png_const_bytep end_row;

            
            png_read_row(png_ptr, inrow, NULL);

            outrow = png_voidcast(png_bytep, display->first_row);
            outrow += y * step_row;
            end_row = outrow + width * channels;

            
            outrow += startx;
            for (; outrow < end_row; outrow += stepx)
            {
               png_byte alpha = inrow[channels];

               if (alpha > 0) 
               {
                  unsigned int c;

                  for (c=0; c<channels; ++c)
                  {
                     png_uint_32 component = inrow[c];

                     if (alpha < 255) 
                     {
                        





                        component *= 257*255; 
                        component += (255-alpha)*png_sRGB_table[outrow[c]];

                        



                        component = PNG_sRGB_FROM_LINEAR(component);
                     }

                     outrow[c] = (png_byte)component;
                  }
               }

               inrow += channels+1; 
            }
         }
      }
   }

   return 1;
}














static int
png_image_read_background(png_voidp argument)
{
   png_image_read_control *display = png_voidcast(png_image_read_control*,
      argument);
   png_imagep image = display->image;
   png_structrp png_ptr = image->opaque->png_ptr;
   png_inforp info_ptr = image->opaque->info_ptr;
   png_uint_32 height = image->height;
   png_uint_32 width = image->width;
   int pass, passes;

   




   if ((png_ptr->transformations & PNG_RGB_TO_GRAY) == 0)
      png_error(png_ptr, "lost rgb to gray");

   if ((png_ptr->transformations & PNG_COMPOSE) != 0)
      png_error(png_ptr, "unexpected compose");

   if (png_get_channels(png_ptr, info_ptr) != 2)
      png_error(png_ptr, "lost/gained channels");

   
   if ((image->format & PNG_FORMAT_FLAG_LINEAR) == 0 &&
      (image->format & PNG_FORMAT_FLAG_ALPHA) != 0)
      png_error(png_ptr, "unexpected 8-bit transformation");

   switch (png_ptr->interlaced)
   {
      case PNG_INTERLACE_NONE:
         passes = 1;
         break;

      case PNG_INTERLACE_ADAM7:
         passes = PNG_INTERLACE_ADAM7_PASSES;
         break;

      default:
         png_error(png_ptr, "unknown interlace type");
   }

   




   switch (info_ptr->bit_depth)
   {
      default:
         png_error(png_ptr, "unexpected bit depth");
         break;

      case 8:
         




         {
            png_bytep first_row = png_voidcast(png_bytep, display->first_row);
            ptrdiff_t step_row = display->row_bytes;

            for (pass = 0; pass < passes; ++pass)
            {
               png_bytep        row = png_voidcast(png_bytep,
                                                   display->first_row);
               unsigned int     startx, stepx, stepy;
               png_uint_32      y;

               if (png_ptr->interlaced == PNG_INTERLACE_ADAM7)
               {
                  
                  if (PNG_PASS_COLS(width, pass) == 0)
                     continue;

                  startx = PNG_PASS_START_COL(pass);
                  stepx = PNG_PASS_COL_OFFSET(pass);
                  y = PNG_PASS_START_ROW(pass);
                  stepy = PNG_PASS_ROW_OFFSET(pass);
               }

               else
               {
                  y = 0;
                  startx = 0;
                  stepx = stepy = 1;
               }

               if (display->background == NULL)
               {
                  for (; y<height; y += stepy)
                  {
                     png_bytep inrow = png_voidcast(png_bytep,
                        display->local_row);
                     png_bytep outrow = first_row + y * step_row;
                     png_const_bytep end_row = outrow + width;

                     
                     png_read_row(png_ptr, inrow, NULL);

                     
                     outrow += startx;
                     for (; outrow < end_row; outrow += stepx)
                     {
                        png_byte alpha = inrow[1];

                        if (alpha > 0) 
                        {
                           png_uint_32 component = inrow[0];

                           if (alpha < 255) 
                           {
                              



                              component = png_sRGB_table[component] * alpha;
                              component += png_sRGB_table[outrow[0]] *
                                 (255-alpha);
                              component = PNG_sRGB_FROM_LINEAR(component);
                           }

                           outrow[0] = (png_byte)component;
                        }

                        inrow += 2; 
                     }
                  }
               }

               else 
               {
                  png_byte background8 = display->background->green;
                  png_uint_16 background = png_sRGB_table[background8];

                  for (; y<height; y += stepy)
                  {
                     png_bytep inrow = png_voidcast(png_bytep,
                        display->local_row);
                     png_bytep outrow = first_row + y * step_row;
                     png_const_bytep end_row = outrow + width;

                     
                     png_read_row(png_ptr, inrow, NULL);

                     
                     outrow += startx;
                     for (; outrow < end_row; outrow += stepx)
                     {
                        png_byte alpha = inrow[1];

                        if (alpha > 0) 
                        {
                           png_uint_32 component = inrow[0];

                           if (alpha < 255) 
                           {
                              component = png_sRGB_table[component] * alpha;
                              component += background * (255-alpha);
                              component = PNG_sRGB_FROM_LINEAR(component);
                           }

                           outrow[0] = (png_byte)component;
                        }

                        else
                           outrow[0] = background8;

                        inrow += 2; 
                     }

                     row += display->row_bytes;
                  }
               }
            }
         }
         break;

      case 16:
         



         {
            png_uint_16p first_row = png_voidcast(png_uint_16p,
               display->first_row);
            


            ptrdiff_t    step_row = display->row_bytes / 2;
            int preserve_alpha = (image->format & PNG_FORMAT_FLAG_ALPHA) != 0;
            unsigned int outchannels = 1+preserve_alpha;
            int swap_alpha = 0;

#           ifdef PNG_SIMPLIFIED_READ_AFIRST_SUPPORTED
               if (preserve_alpha != 0 &&
                   (image->format & PNG_FORMAT_FLAG_AFIRST) != 0)
                  swap_alpha = 1;
#           endif

            for (pass = 0; pass < passes; ++pass)
            {
               unsigned int     startx, stepx, stepy;
               png_uint_32      y;

               

               if (png_ptr->interlaced == PNG_INTERLACE_ADAM7)
               {
                  
                  if (PNG_PASS_COLS(width, pass) == 0)
                     continue;

                  startx = PNG_PASS_START_COL(pass) * outchannels;
                  stepx = PNG_PASS_COL_OFFSET(pass) * outchannels;
                  y = PNG_PASS_START_ROW(pass);
                  stepy = PNG_PASS_ROW_OFFSET(pass);
               }

               else
               {
                  y = 0;
                  startx = 0;
                  stepx = outchannels;
                  stepy = 1;
               }

               for (; y<height; y += stepy)
               {
                  png_const_uint_16p inrow;
                  png_uint_16p outrow = first_row + y*step_row;
                  png_uint_16p end_row = outrow + width * outchannels;

                  
                  png_read_row(png_ptr, png_voidcast(png_bytep,
                     display->local_row), NULL);
                  inrow = png_voidcast(png_const_uint_16p, display->local_row);

                  

                  outrow += startx;
                  for (; outrow < end_row; outrow += stepx)
                  {
                     png_uint_32 component = inrow[0];
                     png_uint_16 alpha = inrow[1];

                     if (alpha > 0) 
                     {
                        if (alpha < 65535) 
                        {
                           component *= alpha;
                           component += 32767;
                           component /= 65535;
                        }
                     }

                     else
                        component = 0;

                     outrow[swap_alpha] = (png_uint_16)component;
                     if (preserve_alpha != 0)
                        outrow[1 ^ swap_alpha] = alpha;

                     inrow += 2; 
                  }
               }
            }
         }
         break;
   }

   return 1;
}


static int
png_image_read_direct(png_voidp argument)
{
   png_image_read_control *display = png_voidcast(png_image_read_control*,
      argument);
   png_imagep image = display->image;
   png_structrp png_ptr = image->opaque->png_ptr;
   png_inforp info_ptr = image->opaque->info_ptr;

   png_uint_32 format = image->format;
   int linear = (format & PNG_FORMAT_FLAG_LINEAR) != 0;
   int do_local_compose = 0;
   int do_local_background = 0; 
   int passes = 0;

   



   png_set_expand(png_ptr);

   
   {
      png_uint_32 base_format = png_image_format(png_ptr) &
         ~PNG_FORMAT_FLAG_COLORMAP ;
      png_uint_32 change = format ^ base_format;
      png_fixed_point output_gamma;
      int mode; 

      
      if ((change & PNG_FORMAT_FLAG_COLOR) != 0)
      {
         
         if ((format & PNG_FORMAT_FLAG_COLOR) != 0)
            png_set_gray_to_rgb(png_ptr);

         else
         {
            












            if ((base_format & PNG_FORMAT_FLAG_ALPHA) != 0)
               do_local_background = 1;

            png_set_rgb_to_gray_fixed(png_ptr, PNG_ERROR_ACTION_NONE,
               PNG_RGB_TO_GRAY_DEFAULT, PNG_RGB_TO_GRAY_DEFAULT);
         }

         change &= ~PNG_FORMAT_FLAG_COLOR;
      }

      

      {
         png_fixed_point input_gamma_default;

         if ((base_format & PNG_FORMAT_FLAG_LINEAR) != 0 &&
             (image->flags & PNG_IMAGE_FLAG_16BIT_sRGB) == 0)
            input_gamma_default = PNG_GAMMA_LINEAR;
         else
            input_gamma_default = PNG_DEFAULT_sRGB;

         


         png_set_alpha_mode_fixed(png_ptr, PNG_ALPHA_PNG, input_gamma_default);
      }

      if (linear != 0)
      {
         


         if ((base_format & PNG_FORMAT_FLAG_ALPHA) != 0)
            mode = PNG_ALPHA_STANDARD; 

         else
            mode = PNG_ALPHA_PNG;

         output_gamma = PNG_GAMMA_LINEAR;
      }

      else
      {
         mode = PNG_ALPHA_PNG;
         output_gamma = PNG_DEFAULT_sRGB;
      }

      





      if (do_local_background != 0)
      {
         png_fixed_point gtest;

         




         if (png_muldiv(&gtest, output_gamma, png_ptr->colorspace.gamma,
               PNG_FP_1) != 0 && png_gamma_significant(gtest) == 0)
            do_local_background = 0;

         else if (mode == PNG_ALPHA_STANDARD)
         {
            do_local_background = 2;
            mode = PNG_ALPHA_PNG; 
         }

         
      }

      
      if ((change & PNG_FORMAT_FLAG_LINEAR) != 0)
      {
         if (linear != 0 )
            png_set_expand_16(png_ptr);

         else 
            png_set_scale_16(png_ptr);

         change &= ~PNG_FORMAT_FLAG_LINEAR;
      }

      
      if ((change & PNG_FORMAT_FLAG_ALPHA) != 0)
      {
         



         if ((base_format & PNG_FORMAT_FLAG_ALPHA) != 0)
         {
            




            if (do_local_background != 0)
               do_local_background = 2;

            
            else if (linear != 0) 
               png_set_strip_alpha(png_ptr);

            
            else if (display->background != NULL)
            {
               png_color_16 c;

               c.index = 0; 
               c.red = display->background->red;
               c.green = display->background->green;
               c.blue = display->background->blue;
               c.gray = display->background->green;

               





               png_set_background_fixed(png_ptr, &c,
                  PNG_BACKGROUND_GAMMA_SCREEN, 0,
                  0);
            }

            else 
            {
               do_local_compose = 1;
               




               mode = PNG_ALPHA_OPTIMIZED;
            }
         }

         else 
         {
            




            png_uint_32 filler; 
            int where;

            if (linear != 0)
               filler = 65535;

            else
               filler = 255;

#           ifdef PNG_FORMAT_AFIRST_SUPPORTED
               if ((format & PNG_FORMAT_FLAG_AFIRST) != 0)
               {
                  where = PNG_FILLER_BEFORE;
                  change &= ~PNG_FORMAT_FLAG_AFIRST;
               }

               else
#           endif
               where = PNG_FILLER_AFTER;

            png_set_add_alpha(png_ptr, filler, where);
         }

         
         change &= ~PNG_FORMAT_FLAG_ALPHA;
      }

      



      png_set_alpha_mode_fixed(png_ptr, mode, output_gamma);

#     ifdef PNG_FORMAT_BGR_SUPPORTED
         if ((change & PNG_FORMAT_FLAG_BGR) != 0)
         {
            


            if ((format & PNG_FORMAT_FLAG_COLOR) != 0)
               png_set_bgr(png_ptr);

            else
               format &= ~PNG_FORMAT_FLAG_BGR;

            change &= ~PNG_FORMAT_FLAG_BGR;
         }
#     endif

#     ifdef PNG_FORMAT_AFIRST_SUPPORTED
         if ((change & PNG_FORMAT_FLAG_AFIRST) != 0)
         {
            




            if ((format & PNG_FORMAT_FLAG_ALPHA) != 0)
            {
               


               if (do_local_background != 2)
                  png_set_swap_alpha(png_ptr);
            }

            else
               format &= ~PNG_FORMAT_FLAG_AFIRST;

            change &= ~PNG_FORMAT_FLAG_AFIRST;
         }
#     endif

      


      if (linear != 0)
      {
         PNG_CONST png_uint_16 le = 0x0001;

         if ((*(png_const_bytep) & le) != 0)
            png_set_swap(png_ptr);
      }

      
      if (change != 0)
         png_error(png_ptr, "png_read_image: unsupported transformation");
   }

   PNG_SKIP_CHUNKS(png_ptr);

   





   if (do_local_compose == 0 && do_local_background != 2)
      passes = png_set_interlace_handling(png_ptr);

   png_read_update_info(png_ptr, info_ptr);

   {
      png_uint_32 info_format = 0;

      if ((info_ptr->color_type & PNG_COLOR_MASK_COLOR) != 0)
         info_format |= PNG_FORMAT_FLAG_COLOR;

      if ((info_ptr->color_type & PNG_COLOR_MASK_ALPHA) != 0)
      {
         
         if (do_local_compose == 0)
         {
            
            if (do_local_background != 2 ||
               (format & PNG_FORMAT_FLAG_ALPHA) != 0)
               info_format |= PNG_FORMAT_FLAG_ALPHA;
         }
      }

      else if (do_local_compose != 0) 
         png_error(png_ptr, "png_image_read: alpha channel lost");

      if (info_ptr->bit_depth == 16)
         info_format |= PNG_FORMAT_FLAG_LINEAR;

#     ifdef PNG_FORMAT_BGR_SUPPORTED
         if ((png_ptr->transformations & PNG_BGR) != 0)
            info_format |= PNG_FORMAT_FLAG_BGR;
#     endif

#     ifdef PNG_FORMAT_AFIRST_SUPPORTED
         if (do_local_background == 2)
         {
            if ((format & PNG_FORMAT_FLAG_AFIRST) != 0)
               info_format |= PNG_FORMAT_FLAG_AFIRST;
         }

         if ((png_ptr->transformations & PNG_SWAP_ALPHA) != 0 ||
            ((png_ptr->transformations & PNG_ADD_ALPHA) != 0 &&
            (png_ptr->flags & PNG_FLAG_FILLER_AFTER) == 0))
         {
            if (do_local_background == 2)
               png_error(png_ptr, "unexpected alpha swap transformation");

            info_format |= PNG_FORMAT_FLAG_AFIRST;
         }
#     endif

      
      if (info_format != format)
         png_error(png_ptr, "png_read_image: invalid transformations");
   }

   




   {
      png_voidp first_row = display->buffer;
      ptrdiff_t row_bytes = display->row_stride;

      if (linear != 0)
         row_bytes *= 2;

      


      if (row_bytes < 0)
      {
         char *ptr = png_voidcast(char*, first_row);
         ptr += (image->height-1) * (-row_bytes);
         first_row = png_voidcast(png_voidp, ptr);
      }

      display->first_row = first_row;
      display->row_bytes = row_bytes;
   }

   if (do_local_compose != 0)
   {
      int result;
      png_voidp row = png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));

      display->local_row = row;
      result = png_safe_execute(image, png_image_read_composite, display);
      display->local_row = NULL;
      png_free(png_ptr, row);

      return result;
   }

   else if (do_local_background == 2)
   {
      int result;
      png_voidp row = png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));

      display->local_row = row;
      result = png_safe_execute(image, png_image_read_background, display);
      display->local_row = NULL;
      png_free(png_ptr, row);

      return result;
   }

   else
   {
      png_alloc_size_t row_bytes = display->row_bytes;

      while (--passes >= 0)
      {
         png_uint_32      y = image->height;
         png_bytep        row = png_voidcast(png_bytep, display->first_row);

         while (y-- > 0)
         {
            png_read_row(png_ptr, row, NULL);
            row += row_bytes;
         }
      }

      return 1;
   }
}

int PNGAPI
png_image_finish_read(png_imagep image, png_const_colorp background,
   void *buffer, png_int_32 row_stride, void *colormap)
{
   if (image != NULL && image->version == PNG_IMAGE_VERSION)
   {
      png_uint_32 check;

      if (row_stride == 0)
         row_stride = PNG_IMAGE_ROW_STRIDE(*image);

      if (row_stride < 0)
         check = -row_stride;

      else
         check = row_stride;

      if (image->opaque != NULL && buffer != NULL &&
         check >= PNG_IMAGE_ROW_STRIDE(*image))
      {
         if ((image->format & PNG_FORMAT_FLAG_COLORMAP) == 0 ||
            (image->colormap_entries > 0 && colormap != NULL))
         {
            int result;
            png_image_read_control display;

            memset(&display, 0, (sizeof display));
            display.image = image;
            display.buffer = buffer;
            display.row_stride = row_stride;
            display.colormap = colormap;
            display.background = background;
            display.local_row = NULL;

            


            if ((image->format & PNG_FORMAT_FLAG_COLORMAP) != 0)
               result =
                  png_safe_execute(image, png_image_read_colormap, &display) &&
                  png_safe_execute(image, png_image_read_colormapped, &display);

            else
               result =
                  png_safe_execute(image, png_image_read_direct, &display);

            png_image_free(image);
            return result;
         }

         else
            return png_image_error(image,
               "png_image_finish_read[color-map]: no color-map");
      }

      else
         return png_image_error(image,
            "png_image_finish_read: invalid argument");
   }

   else if (image != NULL)
      return png_image_error(image,
         "png_image_finish_read: damaged PNG_IMAGE_VERSION");

   return 0;
}

#endif 
#endif

















#include "pngpriv.h"

#ifdef PNG_READ_SUPPORTED


PNG_FUNCTION(png_structp,PNGAPI
png_create_read_struct,(png_const_charp user_png_ver, png_voidp error_ptr,
    png_error_ptr error_fn, png_error_ptr warn_fn),PNG_ALLOCATED)
{

#ifdef PNG_USER_MEM_SUPPORTED
   return (png_create_read_struct_2(user_png_ver, error_ptr, error_fn,
       warn_fn, NULL, NULL, NULL));
}




PNG_FUNCTION(png_structp,PNGAPI
png_create_read_struct_2,(png_const_charp user_png_ver, png_voidp error_ptr,
    png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
    png_malloc_ptr malloc_fn, png_free_ptr free_fn),PNG_ALLOCATED)
{
#endif

#ifdef PNG_SETJMP_SUPPORTED
   volatile
#endif
   png_structp png_ptr;
   volatile int png_cleanup_needed = 0;

#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
   jmp_buf tmp_jmpbuf;
#endif
#endif

   png_debug(1, "in png_create_read_struct");

#ifdef PNG_USER_MEM_SUPPORTED
   png_ptr = (png_structp)png_create_struct_2(PNG_STRUCT_PNG,
       malloc_fn, mem_ptr);
#else
   png_ptr = (png_structp)png_create_struct(PNG_STRUCT_PNG);
#endif
   if (png_ptr == NULL)
      return (NULL);

   
#ifdef PNG_USER_LIMITS_SUPPORTED
   png_ptr->user_width_max = PNG_USER_WIDTH_MAX;
   png_ptr->user_height_max = PNG_USER_HEIGHT_MAX;

#  ifdef PNG_USER_CHUNK_CACHE_MAX
   
   png_ptr->user_chunk_cache_max = PNG_USER_CHUNK_CACHE_MAX;
#  endif

#  ifdef PNG_SET_USER_CHUNK_MALLOC_MAX
   
   png_ptr->user_chunk_malloc_max = PNG_USER_CHUNK_MALLOC_MAX;
#  endif
#endif

#ifdef PNG_SETJMP_SUPPORTED




#ifdef USE_FAR_KEYWORD
   if (setjmp(tmp_jmpbuf))
#else
   if (setjmp(png_jmpbuf(png_ptr))) 
#endif
      PNG_ABORT();
#ifdef USE_FAR_KEYWORD
   png_memcpy(png_jmpbuf(png_ptr), tmp_jmpbuf, png_sizeof(jmp_buf));
#endif
#endif 

#ifdef PNG_USER_MEM_SUPPORTED
   png_set_mem_fn(png_ptr, mem_ptr, malloc_fn, free_fn);
#endif

   png_set_error_fn(png_ptr, error_ptr, error_fn, warn_fn);

   
   if (!png_user_version_check(png_ptr, user_png_ver))
      png_cleanup_needed = 1;

   if (!png_cleanup_needed)
   {
   
   png_ptr->zbuf_size = PNG_ZBUF_SIZE;
   png_ptr->zbuf = (png_bytep)png_malloc_warn(png_ptr, png_ptr->zbuf_size);

   if (png_ptr->zbuf == NULL)
      png_cleanup_needed = 1;
   }

   png_ptr->zstream.zalloc = png_zalloc;
   png_ptr->zstream.zfree = png_zfree;
   png_ptr->zstream.opaque = (voidpf)png_ptr;

   if (!png_cleanup_needed)
   {
      switch (inflateInit(&png_ptr->zstream))
      {
         case Z_OK:
            break; 

         case Z_MEM_ERROR:
            png_warning(png_ptr, "zlib memory error");
            png_cleanup_needed = 1;
            break;

         case Z_STREAM_ERROR:
            png_warning(png_ptr, "zlib stream error");
            png_cleanup_needed = 1;
            break;

         case Z_VERSION_ERROR:
            png_warning(png_ptr, "zlib version error");
            png_cleanup_needed = 1;
            break;

         default: png_warning(png_ptr, "Unknown zlib error");
            png_cleanup_needed = 1;
      }
   }

   if (png_cleanup_needed)
   {
      
      png_free(png_ptr, png_ptr->zbuf);
      png_ptr->zbuf = NULL;
#ifdef PNG_USER_MEM_SUPPORTED
      png_destroy_struct_2((png_voidp)png_ptr,
          (png_free_ptr)free_fn, (png_voidp)mem_ptr);
#else
      png_destroy_struct((png_voidp)png_ptr);
#endif
      return (NULL);
   }

   png_ptr->zstream.next_out = png_ptr->zbuf;
   png_ptr->zstream.avail_out = (uInt)png_ptr->zbuf_size;

   png_set_read_fn(png_ptr, NULL, NULL);


   return (png_ptr);
}


#ifdef PNG_SEQUENTIAL_READ_SUPPORTED








void PNGAPI
png_read_info(png_structp png_ptr, png_infop info_ptr)
{
   png_debug(1, "in png_read_info");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   
   png_read_sig(png_ptr, info_ptr);

   for (;;)
   {
      png_uint_32 length = png_read_chunk_header(png_ptr);
      png_uint_32 chunk_name = png_ptr->chunk_name;

      


      if (chunk_name == png_IDAT)
         if (png_ptr->mode & PNG_AFTER_IDAT)
            png_ptr->mode |= PNG_HAVE_CHUNK_AFTER_IDAT;

      if (chunk_name == png_IHDR)
         png_handle_IHDR(png_ptr, info_ptr, length);

      else if (chunk_name == png_IEND)
         png_handle_IEND(png_ptr, info_ptr, length);

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
      else if (png_chunk_unknown_handling(png_ptr, chunk_name) !=
         PNG_HANDLE_CHUNK_AS_DEFAULT)
      {
         if (chunk_name == png_IDAT)
            png_ptr->mode |= PNG_HAVE_IDAT;

         png_handle_unknown(png_ptr, info_ptr, length);

         if (chunk_name == png_PLTE)
            png_ptr->mode |= PNG_HAVE_PLTE;

         else if (chunk_name == png_IDAT)
         {
            if (!(png_ptr->mode & PNG_HAVE_IHDR))
               png_error(png_ptr, "Missing IHDR before IDAT");

            else if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
                !(png_ptr->mode & PNG_HAVE_PLTE))
               png_error(png_ptr, "Missing PLTE before IDAT");

            break;
         }
      }
#endif
      else if (chunk_name == png_PLTE)
         png_handle_PLTE(png_ptr, info_ptr, length);

      else if (chunk_name == png_IDAT)
      {
         if (!(png_ptr->mode & PNG_HAVE_IHDR))
            png_error(png_ptr, "Missing IHDR before IDAT");

         else if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
             !(png_ptr->mode & PNG_HAVE_PLTE))
            png_error(png_ptr, "Missing PLTE before IDAT");

#ifdef PNG_READ_APNG_SUPPORTED
         png_have_info(png_ptr, info_ptr);
#endif
         png_ptr->idat_size = length;
         png_ptr->mode |= PNG_HAVE_IDAT;
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
         png_handle_unknown(png_ptr, info_ptr, length);
   }
}
#endif 

#ifdef PNG_READ_APNG_SUPPORTED
void PNGAPI
png_read_frame_head(png_structp png_ptr, png_infop info_ptr)
{
    png_byte have_chunk_after_DAT; 

    png_debug(0, "Reading frame head");

    if (!(png_ptr->mode & PNG_HAVE_acTL))
        png_error(png_ptr, "attempt to png_read_frame_head() but "
                           "no acTL present");

    
    if (png_ptr->num_frames_read == 0)
        return;

    png_crc_finish(png_ptr, 0); 

    png_read_reset(png_ptr);
    png_ptr->mode &= ~PNG_HAVE_fcTL;

    have_chunk_after_DAT = 0;
    for (;;)
    {
        png_uint_32 length = png_read_chunk_header(png_ptr);

        if (png_ptr->chunk_name == png_IDAT)
        {
            
            if (have_chunk_after_DAT || png_ptr->num_frames_read > 1)
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

            
            if (!have_chunk_after_DAT && png_ptr->num_frames_read > 1)
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
png_read_update_info(png_structp png_ptr, png_infop info_ptr)
{
   png_debug(1, "in png_read_update_info");

   if (png_ptr == NULL)
      return;

   png_read_start_row(png_ptr);

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
   png_read_transform_info(png_ptr, info_ptr);
#else
   PNG_UNUSED(info_ptr)
#endif
}

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED





void PNGAPI
png_start_read_image(png_structp png_ptr)
{
   png_debug(1, "in png_start_read_image");

   if (png_ptr != NULL)
     png_read_start_row(png_ptr);
}
#endif 

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED
void PNGAPI
png_read_row(png_structp png_ptr, png_bytep row, png_bytep dsp_row)
{
   int ret;

   png_row_info row_info;

   if (png_ptr == NULL)
      return;

   png_debug2(1, "in png_read_row (row %lu, pass %d)",
       (unsigned long)png_ptr->row_number, png_ptr->pass);

   


   if (!(png_ptr->flags & PNG_FLAG_ROW_INIT))
      png_read_start_row(png_ptr);

   
   row_info.width = png_ptr->iwidth; 
   row_info.color_type = png_ptr->color_type;
   row_info.bit_depth = png_ptr->bit_depth;
   row_info.channels = png_ptr->channels;
   row_info.pixel_depth = png_ptr->pixel_depth;
   row_info.rowbytes = PNG_ROWBYTES(row_info.pixel_depth, row_info.width);

   if (png_ptr->row_number == 0 && png_ptr->pass == 0)
   {
   
#if defined(PNG_WRITE_INVERT_SUPPORTED) && !defined(PNG_READ_INVERT_SUPPORTED)
   if (png_ptr->transformations & PNG_INVERT_MONO)
      png_warning(png_ptr, "PNG_READ_INVERT_SUPPORTED is not defined");
#endif

#if defined(PNG_WRITE_FILLER_SUPPORTED) && !defined(PNG_READ_FILLER_SUPPORTED)
   if (png_ptr->transformations & PNG_FILLER)
      png_warning(png_ptr, "PNG_READ_FILLER_SUPPORTED is not defined");
#endif

#if defined(PNG_WRITE_PACKSWAP_SUPPORTED) && \
    !defined(PNG_READ_PACKSWAP_SUPPORTED)
   if (png_ptr->transformations & PNG_PACKSWAP)
      png_warning(png_ptr, "PNG_READ_PACKSWAP_SUPPORTED is not defined");
#endif

#if defined(PNG_WRITE_PACK_SUPPORTED) && !defined(PNG_READ_PACK_SUPPORTED)
   if (png_ptr->transformations & PNG_PACK)
      png_warning(png_ptr, "PNG_READ_PACK_SUPPORTED is not defined");
#endif

#if defined(PNG_WRITE_SHIFT_SUPPORTED) && !defined(PNG_READ_SHIFT_SUPPORTED)
   if (png_ptr->transformations & PNG_SHIFT)
      png_warning(png_ptr, "PNG_READ_SHIFT_SUPPORTED is not defined");
#endif

#if defined(PNG_WRITE_BGR_SUPPORTED) && !defined(PNG_READ_BGR_SUPPORTED)
   if (png_ptr->transformations & PNG_BGR)
      png_warning(png_ptr, "PNG_READ_BGR_SUPPORTED is not defined");
#endif

#if defined(PNG_WRITE_SWAP_SUPPORTED) && !defined(PNG_READ_SWAP_SUPPORTED)
   if (png_ptr->transformations & PNG_SWAP_BYTES)
      png_warning(png_ptr, "PNG_READ_SWAP_SUPPORTED is not defined");
#endif
   }

#ifdef PNG_READ_INTERLACING_SUPPORTED
   





   if (png_ptr->interlaced && (png_ptr->transformations & PNG_INTERLACE))
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
            if (!(png_ptr->row_number & 1))
            {
               png_read_finish_row(png_ptr);
               return;
            }
            break;
      }
   }
#endif

   if (!(png_ptr->mode & PNG_HAVE_IDAT))
      png_error(png_ptr, "Invalid attempt to read row data");

   png_ptr->zstream.next_out = png_ptr->row_buf;
   png_ptr->zstream.avail_out =
       (uInt)(PNG_ROWBYTES(png_ptr->pixel_depth,
       png_ptr->iwidth) + 1);

   do
   {
      if (!(png_ptr->zstream.avail_in))
      {
#ifdef PNG_READ_APNG_SUPPORTED
         png_uint_32 bytes_to_skip = 0;

         while (!png_ptr->idat_size || bytes_to_skip != 0)
         {
            png_crc_finish(png_ptr, bytes_to_skip);
            bytes_to_skip = 0;

            png_ptr->idat_size = png_read_chunk_header(png_ptr);
            if (png_ptr->num_frames_read == 0)
            {
               if (png_ptr->chunk_name != png_IDAT)
                  png_error(png_ptr, "Not enough image data");
            }
            else
            {
               if (png_ptr->chunk_name == png_IEND)
                  png_error(png_ptr, "Not enough image data");
               if (png_ptr->chunk_name != png_fdAT)
               {
                  png_warning(png_ptr, "Skipped (ignored) a chunk "
                                       "between APNG chunks");
                  bytes_to_skip = png_ptr->idat_size;
                  continue;
               }

               png_ensure_sequence_number(png_ptr, png_ptr->idat_size);

               png_ptr->idat_size -= 4;
            }
         }
#else
         while (!png_ptr->idat_size)
         {
            png_crc_finish(png_ptr, 0);

            png_ptr->idat_size = png_read_chunk_header(png_ptr);
            if (png_ptr->chunk_name != png_IDAT)
               png_error(png_ptr, "Not enough image data");
         }
#endif
         png_ptr->zstream.avail_in = (uInt)png_ptr->zbuf_size;
         png_ptr->zstream.next_in = png_ptr->zbuf;
         if (png_ptr->zbuf_size > png_ptr->idat_size)
            png_ptr->zstream.avail_in = (uInt)png_ptr->idat_size;
         png_crc_read(png_ptr, png_ptr->zbuf,
             (png_size_t)png_ptr->zstream.avail_in);
         png_ptr->idat_size -= png_ptr->zstream.avail_in;
      }

      ret = inflate(&png_ptr->zstream, Z_PARTIAL_FLUSH);

      if (ret == Z_STREAM_END)
      {
         if (png_ptr->zstream.avail_out || png_ptr->zstream.avail_in ||
            png_ptr->idat_size)
            png_benign_error(png_ptr, "Extra compressed data");
         png_ptr->mode |= PNG_AFTER_IDAT;
         png_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;
#ifdef PNG_READ_APNG_SUPPORTED
         png_ptr->num_frames_read++;
#endif
         break;
      }

      if (ret != Z_OK)
         png_error(png_ptr, png_ptr->zstream.msg ? png_ptr->zstream.msg :
             "Decompression error");

   } while (png_ptr->zstream.avail_out);

   if (png_ptr->row_buf[0] > PNG_FILTER_VALUE_NONE)
   {
      if (png_ptr->row_buf[0] < PNG_FILTER_VALUE_LAST)
         png_read_filter_row(png_ptr, &row_info, png_ptr->row_buf + 1,
            png_ptr->prev_row + 1, png_ptr->row_buf[0]);
      else
         png_error(png_ptr, "bad adaptive filter value");
   }

   




   png_memcpy(png_ptr->prev_row, png_ptr->row_buf, row_info.rowbytes + 1);

#ifdef PNG_MNG_FEATURES_SUPPORTED
   if ((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
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
   
   if (png_ptr->interlaced &&
      (png_ptr->transformations & PNG_INTERLACE))
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
png_read_rows(png_structp png_ptr, png_bytepp row,
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
png_read_image(png_structp png_ptr, png_bytepp image)
{
   png_uint_32 i, image_height;
   int pass, j;
   png_bytepp rp;

   png_debug(1, "in png_read_image");

   if (png_ptr == NULL)
      return;

#ifdef PNG_READ_INTERLACING_SUPPORTED
   if (!(png_ptr->flags & PNG_FLAG_ROW_INIT))
   {
      pass = png_set_interlace_handling(png_ptr);
      
      png_start_read_image(png_ptr);
   }
   else
   {
      if (png_ptr->interlaced && !(png_ptr->transformations & PNG_INTERLACE))
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
png_read_end(png_structp png_ptr, png_infop info_ptr)
{
   png_debug(1, "in png_read_end");

   if (png_ptr == NULL)
      return;

   png_crc_finish(png_ptr, 0); 

   do
   {
      png_uint_32 length = png_read_chunk_header(png_ptr);
      png_uint_32 chunk_name = png_ptr->chunk_name;

      if (chunk_name == png_IHDR)
         png_handle_IHDR(png_ptr, info_ptr, length);

      else if (chunk_name == png_IEND)
         png_handle_IEND(png_ptr, info_ptr, length);

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
      else if (png_chunk_unknown_handling(png_ptr, chunk_name) !=
         PNG_HANDLE_CHUNK_AS_DEFAULT)
      {
         if (chunk_name == png_IDAT)
         {
            if ((length > 0) || (png_ptr->mode & PNG_HAVE_CHUNK_AFTER_IDAT))
               png_benign_error(png_ptr, "Too many IDATs found");
         }
         png_handle_unknown(png_ptr, info_ptr, length);
         if (chunk_name == png_PLTE)
            png_ptr->mode |= PNG_HAVE_PLTE;
      }
#endif

      else if (chunk_name == png_IDAT)
      {
         


         if ((length > 0) || (png_ptr->mode & PNG_HAVE_CHUNK_AFTER_IDAT))
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

#ifdef PNG_READ_APNG_SUPPORTED
      else if (chunk_name == png_acTL)
         png_handle_acTL(png_ptr, info_ptr, length);
      else if (chunk_name == png_fcTL)
         png_handle_fcTL(png_ptr, info_ptr, length);
      else if (chunk_name == png_fdAT)
         png_handle_fdAT(png_ptr, info_ptr, length);
#endif

      else
         png_handle_unknown(png_ptr, info_ptr, length);
   } while (!(png_ptr->mode & PNG_HAVE_IEND));
}
#endif 


void PNGAPI
png_destroy_read_struct(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr,
    png_infopp end_info_ptr_ptr)
{
   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL, end_info_ptr = NULL;
#ifdef PNG_USER_MEM_SUPPORTED
   png_free_ptr free_fn = NULL;
   png_voidp mem_ptr = NULL;
#endif

   png_debug(1, "in png_destroy_read_struct");

   if (png_ptr_ptr != NULL)
      png_ptr = *png_ptr_ptr;
   if (png_ptr == NULL)
      return;

#ifdef PNG_USER_MEM_SUPPORTED
   free_fn = png_ptr->free_fn;
   mem_ptr = png_ptr->mem_ptr;
#endif

   if (info_ptr_ptr != NULL)
      info_ptr = *info_ptr_ptr;

   if (end_info_ptr_ptr != NULL)
      end_info_ptr = *end_info_ptr_ptr;

   png_read_destroy(png_ptr, info_ptr, end_info_ptr);

   if (info_ptr != NULL)
   {
#ifdef PNG_TEXT_SUPPORTED
      png_free_data(png_ptr, info_ptr, PNG_FREE_TEXT, -1);
#endif

#ifdef PNG_USER_MEM_SUPPORTED
      png_destroy_struct_2((png_voidp)info_ptr, (png_free_ptr)free_fn,
          (png_voidp)mem_ptr);
#else
      png_destroy_struct((png_voidp)info_ptr);
#endif
      *info_ptr_ptr = NULL;
   }

   if (end_info_ptr != NULL)
   {
#ifdef PNG_READ_TEXT_SUPPORTED
      png_free_data(png_ptr, end_info_ptr, PNG_FREE_TEXT, -1);
#endif
#ifdef PNG_USER_MEM_SUPPORTED
      png_destroy_struct_2((png_voidp)end_info_ptr, (png_free_ptr)free_fn,
          (png_voidp)mem_ptr);
#else
      png_destroy_struct((png_voidp)end_info_ptr);
#endif
      *end_info_ptr_ptr = NULL;
   }

   if (png_ptr != NULL)
   {
#ifdef PNG_USER_MEM_SUPPORTED
      png_destroy_struct_2((png_voidp)png_ptr, (png_free_ptr)free_fn,
          (png_voidp)mem_ptr);
#else
      png_destroy_struct((png_voidp)png_ptr);
#endif
      *png_ptr_ptr = NULL;
   }
}


void 
png_read_destroy(png_structp png_ptr, png_infop info_ptr,
    png_infop end_info_ptr)
{
#ifdef PNG_SETJMP_SUPPORTED
   jmp_buf tmp_jmp;
#endif
   png_error_ptr error_fn;
#ifdef PNG_WARNINGS_SUPPORTED
   png_error_ptr warning_fn;
#endif
   png_voidp error_ptr;
#ifdef PNG_USER_MEM_SUPPORTED
   png_free_ptr free_fn;
#endif

   png_debug(1, "in png_read_destroy");

   if (info_ptr != NULL)
      png_info_destroy(png_ptr, info_ptr);

   if (end_info_ptr != NULL)
      png_info_destroy(png_ptr, end_info_ptr);

#ifdef PNG_READ_GAMMA_SUPPORTED
   png_destroy_gamma_table(png_ptr);
#endif

   png_free(png_ptr, png_ptr->zbuf);
   png_free(png_ptr, png_ptr->big_row_buf);
   png_free(png_ptr, png_ptr->big_prev_row);
   png_free(png_ptr, png_ptr->chunkdata);

#ifdef PNG_READ_QUANTIZE_SUPPORTED
   png_free(png_ptr, png_ptr->palette_lookup);
   png_free(png_ptr, png_ptr->quantize_index);
#endif

   if (png_ptr->free_me & PNG_FREE_PLTE)
      png_zfree(png_ptr, png_ptr->palette);
   png_ptr->free_me &= ~PNG_FREE_PLTE;

#if defined(PNG_tRNS_SUPPORTED) || \
    defined(PNG_READ_EXPAND_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   if (png_ptr->free_me & PNG_FREE_TRNS)
      png_free(png_ptr, png_ptr->trans_alpha);
   png_ptr->free_me &= ~PNG_FREE_TRNS;
#endif

#ifdef PNG_READ_hIST_SUPPORTED
   if (png_ptr->free_me & PNG_FREE_HIST)
      png_free(png_ptr, png_ptr->hist);
   png_ptr->free_me &= ~PNG_FREE_HIST;
#endif

   inflateEnd(&png_ptr->zstream);

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
   png_free(png_ptr, png_ptr->save_buffer);
#endif

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
#ifdef PNG_TEXT_SUPPORTED
   png_free(png_ptr, png_ptr->current_text);
#endif 
#endif 

   


#ifdef PNG_SETJMP_SUPPORTED
   png_memcpy(tmp_jmp, png_ptr->longjmp_buffer, png_sizeof(jmp_buf));
#endif

   error_fn = png_ptr->error_fn;
#ifdef PNG_WARNINGS_SUPPORTED
   warning_fn = png_ptr->warning_fn;
#endif
   error_ptr = png_ptr->error_ptr;
#ifdef PNG_USER_MEM_SUPPORTED
   free_fn = png_ptr->free_fn;
#endif

   png_memset(png_ptr, 0, png_sizeof(png_struct));

   png_ptr->error_fn = error_fn;
#ifdef PNG_WARNINGS_SUPPORTED
   png_ptr->warning_fn = warning_fn;
#endif
   png_ptr->error_ptr = error_ptr;
#ifdef PNG_USER_MEM_SUPPORTED
   png_ptr->free_fn = free_fn;
#endif

#ifdef PNG_SETJMP_SUPPORTED
   png_memcpy(png_ptr->longjmp_buffer, tmp_jmp, png_sizeof(jmp_buf));
#endif

}

void PNGAPI
png_set_read_status_fn(png_structp png_ptr, png_read_status_ptr read_row_fn)
{
   if (png_ptr == NULL)
      return;

   png_ptr->read_row_fn = read_row_fn;
}


#ifdef PNG_SEQUENTIAL_READ_SUPPORTED
#ifdef PNG_INFO_IMAGE_SUPPORTED
void PNGAPI
png_read_png(png_structp png_ptr, png_infop info_ptr,
                           int transforms,
                           voidp params)
{
   int row;

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   


   png_read_info(png_ptr, info_ptr);
   if (info_ptr->height > PNG_UINT_32_MAX/png_sizeof(png_bytep))
      png_error(png_ptr, "Image is too high to process with png_read_png()");

   

#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
   

   if (transforms & PNG_TRANSFORM_SCALE_16)
   {
     


      png_set_scale_16(png_ptr);
   }
#endif

#ifdef PNG_READ_STRIP_16_TO_8_SUPPORTED
   



   if (transforms & PNG_TRANSFORM_STRIP_16)
      png_set_strip_16(png_ptr);
#endif

#ifdef PNG_READ_STRIP_ALPHA_SUPPORTED
   


   if (transforms & PNG_TRANSFORM_STRIP_ALPHA)
      png_set_strip_alpha(png_ptr);
#endif

#if defined(PNG_READ_PACK_SUPPORTED) && !defined(PNG_READ_EXPAND_SUPPORTED)
   


   if (transforms & PNG_TRANSFORM_PACKING)
      png_set_packing(png_ptr);
#endif

#ifdef PNG_READ_PACKSWAP_SUPPORTED
   


   if (transforms & PNG_TRANSFORM_PACKSWAP)
      png_set_packswap(png_ptr);
#endif

#ifdef PNG_READ_EXPAND_SUPPORTED
   




   if (transforms & PNG_TRANSFORM_EXPAND)
      if ((png_ptr->bit_depth < 8) ||
          (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE) ||
          (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)))
         png_set_expand(png_ptr);
#endif

   


#ifdef PNG_READ_INVERT_SUPPORTED
   

   if (transforms & PNG_TRANSFORM_INVERT_MONO)
      png_set_invert_mono(png_ptr);
#endif

#ifdef PNG_READ_SHIFT_SUPPORTED
   



   if ((transforms & PNG_TRANSFORM_SHIFT)
       && png_get_valid(png_ptr, info_ptr, PNG_INFO_sBIT))
   {
      png_color_8p sig_bit;

      png_get_sBIT(png_ptr, info_ptr, &sig_bit);
      png_set_shift(png_ptr, sig_bit);
   }
#endif

#ifdef PNG_READ_BGR_SUPPORTED
   
   if (transforms & PNG_TRANSFORM_BGR)
      png_set_bgr(png_ptr);
#endif

#ifdef PNG_READ_SWAP_ALPHA_SUPPORTED
   
   if (transforms & PNG_TRANSFORM_SWAP_ALPHA)
      png_set_swap_alpha(png_ptr);
#endif

#ifdef PNG_READ_SWAP_SUPPORTED
   
   if (transforms & PNG_TRANSFORM_SWAP_ENDIAN)
      png_set_swap(png_ptr);
#endif


#ifdef PNG_READ_INVERT_ALPHA_SUPPORTED
   
   if (transforms & PNG_TRANSFORM_INVERT_ALPHA)
      png_set_invert_alpha(png_ptr);
#endif


#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED
   
   if (transforms & PNG_TRANSFORM_GRAY_TO_RGB)
      png_set_gray_to_rgb(png_ptr);
#endif


#ifdef PNG_READ_EXPAND_16_SUPPORTED
   if (transforms & PNG_TRANSFORM_EXPAND_16)
      png_set_expand_16(png_ptr);
#endif

   

   


   (void)png_set_interlace_handling(png_ptr);

   



   png_read_update_info(png_ptr, info_ptr);

   

   png_free_data(png_ptr, info_ptr, PNG_FREE_ROWS, 0);
   if (info_ptr->row_pointers == NULL)
   {
      png_uint_32 iptr;

      info_ptr->row_pointers = (png_bytepp)png_malloc(png_ptr,
          info_ptr->height * png_sizeof(png_bytep));
      for (iptr=0; iptr<info_ptr->height; iptr++)
         info_ptr->row_pointers[iptr] = NULL;

      info_ptr->free_me |= PNG_FREE_ROWS;

      for (row = 0; row < (int)info_ptr->height; row++)
         info_ptr->row_pointers[row] = (png_bytep)png_malloc(png_ptr,
            png_get_rowbytes(png_ptr, info_ptr));
   }

   png_read_image(png_ptr, info_ptr->row_pointers);
   info_ptr->valid |= PNG_INFO_IDAT;

   
   png_read_end(png_ptr, info_ptr);

   PNG_UNUSED(transforms)   
   PNG_UNUSED(params)

}
#endif 
#endif 
#endif 

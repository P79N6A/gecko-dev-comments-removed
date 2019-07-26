












#include "pngpriv.h"

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED


#define PNG_READ_SIG_MODE   0
#define PNG_READ_CHUNK_MODE 1
#define PNG_READ_IDAT_MODE  2
#define PNG_SKIP_MODE       3
#define PNG_READ_tEXt_MODE  4
#define PNG_READ_zTXt_MODE  5
#define PNG_READ_DONE_MODE  6
#define PNG_READ_iTXt_MODE  7
#define PNG_ERROR_MODE      8

void PNGAPI
png_process_data(png_structp png_ptr, png_infop info_ptr,
    png_bytep buffer, png_size_t buffer_size)
{
   if (png_ptr == NULL || info_ptr == NULL)
      return;

   png_push_restore_buffer(png_ptr, buffer, buffer_size);

   while (png_ptr->buffer_size)
   {
      png_process_some_data(png_ptr, info_ptr);
   }
}

png_size_t PNGAPI
png_process_data_pause(png_structp png_ptr, int save)
{
   if (png_ptr != NULL)
   {
      


      if (save)
         png_push_save_buffer(png_ptr);
      else
      {
         
         png_size_t remaining = png_ptr->buffer_size;
         png_ptr->buffer_size = 0;

         


         if (png_ptr->save_buffer_size < remaining)
            return remaining - png_ptr->save_buffer_size;
      }
   }

   return 0;
}

png_uint_32 PNGAPI
png_process_data_skip(png_structp png_ptr)
{
   png_uint_32 remaining = 0;

   if (png_ptr != NULL && png_ptr->process_mode == PNG_SKIP_MODE &&
      png_ptr->skip_length > 0)
   {
      


      if (png_ptr->buffer_size != 0)
         png_error(png_ptr,
            "png_process_data_skip called inside png_process_data");

      




      if (png_ptr->save_buffer_size != 0)
         png_error(png_ptr, "png_process_data_skip called with saved data");

      remaining = png_ptr->skip_length;
      png_ptr->skip_length = 0;
      png_ptr->process_mode = PNG_READ_CHUNK_MODE;
   }

   return remaining;
}




void 
png_process_some_data(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr == NULL)
      return;

   switch (png_ptr->process_mode)
   {
      case PNG_READ_SIG_MODE:
      {
         png_push_read_sig(png_ptr, info_ptr);
         break;
      }

      case PNG_READ_CHUNK_MODE:
      {
         png_push_read_chunk(png_ptr, info_ptr);
         break;
      }

      case PNG_READ_IDAT_MODE:
      {
         png_push_read_IDAT(png_ptr);
         break;
      }

      case PNG_SKIP_MODE:
      {
         png_push_crc_finish(png_ptr);
         break;
      }

      default:
      {
         png_ptr->buffer_size = 0;
         break;
      }
   }
}







void 
png_push_read_sig(png_structp png_ptr, png_infop info_ptr)
{
   png_size_t num_checked = png_ptr->sig_bytes,
       num_to_check = 8 - num_checked;

   if (png_ptr->buffer_size < num_to_check)
   {
      num_to_check = png_ptr->buffer_size;
   }

   png_push_fill_buffer(png_ptr, &(info_ptr->signature[num_checked]),
       num_to_check);
   png_ptr->sig_bytes = (png_byte)(png_ptr->sig_bytes + num_to_check);

   if (png_sig_cmp(info_ptr->signature, num_checked, num_to_check))
   {
      if (num_checked < 4 &&
          png_sig_cmp(info_ptr->signature, num_checked, num_to_check - 4))
         png_error(png_ptr, "Not a PNG file");

      else
         png_error(png_ptr, "PNG file corrupted by ASCII conversion");
   }

   else
   {
      if (png_ptr->sig_bytes >= 8)
      {
         png_ptr->process_mode = PNG_READ_CHUNK_MODE;
      }
   }
}

void 
png_push_read_chunk(png_structp png_ptr, png_infop info_ptr)
{
   png_uint_32 chunk_name;

   





   if (!(png_ptr->mode & PNG_HAVE_CHUNK_HEADER))
   {
      png_byte chunk_length[4];
      png_byte chunk_tag[4];

      if (png_ptr->buffer_size < 8)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_push_fill_buffer(png_ptr, chunk_length, 4);
      png_ptr->push_length = png_get_uint_31(png_ptr, chunk_length);
      png_reset_crc(png_ptr);
      png_crc_read(png_ptr, chunk_tag, 4);
      png_ptr->chunk_name = PNG_CHUNK_FROM_STRING(chunk_tag);
      png_check_chunk_name(png_ptr, png_ptr->chunk_name);
      png_ptr->mode |= PNG_HAVE_CHUNK_HEADER;
   }

   chunk_name = png_ptr->chunk_name;

#ifdef PNG_READ_APNG_SUPPORTED
   if (png_ptr->num_frames_read > 0 &&
       png_ptr->num_frames_read < info_ptr->num_frames)
   {
      if (chunk_name == png_IDAT)
      {
         
         if (png_ptr->mode & PNG_HAVE_fcTL || png_ptr->num_frames_read > 1)
            png_error(png_ptr, "out of place IDAT");

         if (png_ptr->push_length + 4 > png_ptr->buffer_size)
         {
            png_push_save_buffer(png_ptr);
            return;
         }

         png_push_crc_skip(png_ptr, png_ptr->push_length);
         png_ptr->mode &= ~PNG_HAVE_CHUNK_HEADER;
         return;
      }
      else if (chunk_name == png_fdAT)
      {
         if (png_ptr->buffer_size < 4)
         {
            png_push_save_buffer(png_ptr);
            return;
         }

         png_ensure_sequence_number(png_ptr, 4);

         if (!(png_ptr->mode & PNG_HAVE_fcTL))
         {
            
            if (png_ptr->num_frames_read < 2)
               png_error(png_ptr, "out of place fdAT");

            if (png_ptr->push_length + 4 > png_ptr->buffer_size)
            {
               png_push_save_buffer(png_ptr);
               return;
            }

            png_push_crc_skip(png_ptr, png_ptr->push_length);
            png_ptr->mode &= ~PNG_HAVE_CHUNK_HEADER;
            return;
         }

         else
         {
            
            png_ptr->idat_size = png_ptr->push_length - 4;
            png_ptr->mode |= PNG_HAVE_IDAT;
            png_ptr->process_mode = PNG_READ_IDAT_MODE;

            return;
         }
      }

      else if (chunk_name == png_fcTL)
      {
         if (png_ptr->push_length + 4 > png_ptr->buffer_size)
         {
            png_push_save_buffer(png_ptr);
            return;
         }

         png_read_reset(png_ptr);
         png_ptr->mode &= ~PNG_HAVE_fcTL;

         png_handle_fcTL(png_ptr, info_ptr, png_ptr->push_length);

         if (!(png_ptr->mode & PNG_HAVE_fcTL))
            png_error(png_ptr, "missing required fcTL chunk");

         png_read_reinit(png_ptr, info_ptr);
         png_progressive_read_reset(png_ptr);

         if (png_ptr->frame_info_fn != NULL)
            (*(png_ptr->frame_info_fn))(png_ptr, png_ptr->num_frames_read);

         png_ptr->mode &= ~PNG_HAVE_CHUNK_HEADER;

         return;
      }

      else
      {
         if (png_ptr->push_length + 4 > png_ptr->buffer_size)
         {
            png_push_save_buffer(png_ptr);
            return;
         }
         png_warning(png_ptr, "Skipped (ignored) a chunk "
                              "between APNG chunks");
         png_push_crc_skip(png_ptr, png_ptr->push_length);
         png_ptr->mode &= ~PNG_HAVE_CHUNK_HEADER;
         return;
      }

      return;
   }
#endif 

   if (chunk_name == png_IDAT)
   {
      





      if (png_ptr->mode & PNG_AFTER_IDAT)
         png_ptr->mode |= PNG_HAVE_CHUNK_AFTER_IDAT;
   }

   if (chunk_name == png_IHDR)
   {
      if (png_ptr->push_length != 13)
         png_error(png_ptr, "Invalid IHDR length");

      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_IHDR(png_ptr, info_ptr, png_ptr->push_length);
   }

   else if (chunk_name == png_IEND)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_IEND(png_ptr, info_ptr, png_ptr->push_length);

      png_ptr->process_mode = PNG_READ_DONE_MODE;
      png_push_have_end(png_ptr, info_ptr);
   }

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
   else if (png_chunk_unknown_handling(png_ptr, chunk_name))
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      if (chunk_name == png_IDAT)
         png_ptr->mode |= PNG_HAVE_IDAT;

      png_handle_unknown(png_ptr, info_ptr, png_ptr->push_length);

      if (chunk_name == png_PLTE)
         png_ptr->mode |= PNG_HAVE_PLTE;

      else if (chunk_name == png_IDAT)
      {
         if (!(png_ptr->mode & PNG_HAVE_IHDR))
            png_error(png_ptr, "Missing IHDR before IDAT");

         else if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
             !(png_ptr->mode & PNG_HAVE_PLTE))
            png_error(png_ptr, "Missing PLTE before IDAT");
      }
   }
#endif

   else if (chunk_name == png_PLTE)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }
      png_handle_PLTE(png_ptr, info_ptr, png_ptr->push_length);
   }

   else if (chunk_name == png_IDAT)
   {
      




      if (!(png_ptr->mode & PNG_HAVE_IHDR))
         png_error(png_ptr, "Missing IHDR before IDAT");

      else if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
          !(png_ptr->mode & PNG_HAVE_PLTE))
         png_error(png_ptr, "Missing PLTE before IDAT");

      if (png_ptr->mode & PNG_HAVE_IDAT)
      {
         if (!(png_ptr->mode & PNG_HAVE_CHUNK_AFTER_IDAT))
            if (png_ptr->push_length == 0)
               return;

         if (png_ptr->mode & PNG_AFTER_IDAT)
            png_benign_error(png_ptr, "Too many IDATs found");
      }

#ifdef PNG_READ_APNG_SUPPORTED
      png_have_info(png_ptr, info_ptr);
#endif

      png_ptr->idat_size = png_ptr->push_length;
      png_ptr->mode |= PNG_HAVE_IDAT;
      png_ptr->process_mode = PNG_READ_IDAT_MODE;
      png_push_have_info(png_ptr, info_ptr);
      png_ptr->zstream.avail_out =
          (uInt) PNG_ROWBYTES(png_ptr->pixel_depth,
          png_ptr->iwidth) + 1;
      png_ptr->zstream.next_out = png_ptr->row_buf;
      return;
   }

#ifdef PNG_READ_gAMA_SUPPORTED
   else if (png_ptr->chunk_name == png_gAMA)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_gAMA(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_sBIT_SUPPORTED
   else if (png_ptr->chunk_name == png_sBIT)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_sBIT(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_cHRM_SUPPORTED
   else if (png_ptr->chunk_name == png_cHRM)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_cHRM(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_sRGB_SUPPORTED
   else if (chunk_name == png_sRGB)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_sRGB(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_iCCP_SUPPORTED
   else if (png_ptr->chunk_name == png_iCCP)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_iCCP(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_sPLT_SUPPORTED
   else if (chunk_name == png_sPLT)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_sPLT(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_tRNS_SUPPORTED
   else if (chunk_name == png_tRNS)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_tRNS(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_bKGD_SUPPORTED
   else if (chunk_name == png_bKGD)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_bKGD(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_hIST_SUPPORTED
   else if (chunk_name == png_hIST)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_hIST(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_pHYs_SUPPORTED
   else if (chunk_name == png_pHYs)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_pHYs(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_oFFs_SUPPORTED
   else if (chunk_name == png_oFFs)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_oFFs(png_ptr, info_ptr, png_ptr->push_length);
   }
#endif

#ifdef PNG_READ_pCAL_SUPPORTED
   else if (chunk_name == png_pCAL)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_pCAL(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_sCAL_SUPPORTED
   else if (chunk_name == png_sCAL)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_sCAL(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_tIME_SUPPORTED
   else if (chunk_name == png_tIME)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_tIME(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_tEXt_SUPPORTED
   else if (chunk_name == png_tEXt)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_tEXt(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_zTXt_SUPPORTED
   else if (chunk_name == png_zTXt)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_zTXt(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_iTXt_SUPPORTED
   else if (chunk_name == png_iTXt)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_iTXt(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_APNG_SUPPORTED
   else if (chunk_name == png_acTL)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }
      png_handle_acTL(png_ptr, info_ptr, png_ptr->push_length);
   }

   else if (chunk_name == png_fcTL)
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }
      png_handle_fcTL(png_ptr, info_ptr, png_ptr->push_length);
   }
#endif 

   else
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }
      png_handle_unknown(png_ptr, info_ptr, png_ptr->push_length);
   }

   png_ptr->mode &= ~PNG_HAVE_CHUNK_HEADER;
}

void 
png_push_crc_skip(png_structp png_ptr, png_uint_32 skip)
{
   png_ptr->process_mode = PNG_SKIP_MODE;
   png_ptr->skip_length = skip;
}

void 
png_push_crc_finish(png_structp png_ptr)
{
   if (png_ptr->skip_length && png_ptr->save_buffer_size)
   {
      png_size_t save_size = png_ptr->save_buffer_size;
      png_uint_32 skip_length = png_ptr->skip_length;

      





      if (skip_length < save_size)
         save_size = (png_size_t)skip_length;

      else
         skip_length = (png_uint_32)save_size;

      png_calculate_crc(png_ptr, png_ptr->save_buffer_ptr, save_size);

      png_ptr->skip_length -= skip_length;
      png_ptr->buffer_size -= save_size;
      png_ptr->save_buffer_size -= save_size;
      png_ptr->save_buffer_ptr += save_size;
   }

   if (png_ptr->skip_length && png_ptr->current_buffer_size)
   {
      png_size_t save_size = png_ptr->current_buffer_size;
      png_uint_32 skip_length = png_ptr->skip_length;

      


      if (skip_length < save_size)
         save_size = (png_size_t)skip_length;

      else
         skip_length = (png_uint_32)save_size;

      png_calculate_crc(png_ptr, png_ptr->current_buffer_ptr, save_size);

      png_ptr->skip_length -= skip_length;
      png_ptr->buffer_size -= save_size;
      png_ptr->current_buffer_size -= save_size;
      png_ptr->current_buffer_ptr += save_size;
   }

   if (!png_ptr->skip_length)
   {
      if (png_ptr->buffer_size < 4)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_crc_finish(png_ptr, 0);
      png_ptr->process_mode = PNG_READ_CHUNK_MODE;
   }
}

void PNGCBAPI
png_push_fill_buffer(png_structp png_ptr, png_bytep buffer, png_size_t length)
{
   png_bytep ptr;

   if (png_ptr == NULL)
      return;

   ptr = buffer;

   if (png_ptr->save_buffer_size)
   {
      png_size_t save_size;

      if (length < png_ptr->save_buffer_size)
         save_size = length;

      else
         save_size = png_ptr->save_buffer_size;

      png_memcpy(ptr, png_ptr->save_buffer_ptr, save_size);
      length -= save_size;
      ptr += save_size;
      png_ptr->buffer_size -= save_size;
      png_ptr->save_buffer_size -= save_size;
      png_ptr->save_buffer_ptr += save_size;
   }

   if (length && png_ptr->current_buffer_size)
   {
      png_size_t save_size;

      if (length < png_ptr->current_buffer_size)
         save_size = length;

      else
         save_size = png_ptr->current_buffer_size;

      png_memcpy(ptr, png_ptr->current_buffer_ptr, save_size);
      png_ptr->buffer_size -= save_size;
      png_ptr->current_buffer_size -= save_size;
      png_ptr->current_buffer_ptr += save_size;
   }
}

void 
png_push_save_buffer(png_structp png_ptr)
{
   if (png_ptr->save_buffer_size)
   {
      if (png_ptr->save_buffer_ptr != png_ptr->save_buffer)
      {
         png_size_t i, istop;
         png_bytep sp;
         png_bytep dp;

         istop = png_ptr->save_buffer_size;

         for (i = 0, sp = png_ptr->save_buffer_ptr, dp = png_ptr->save_buffer;
             i < istop; i++, sp++, dp++)
         {
            *dp = *sp;
         }
      }
   }

   if (png_ptr->save_buffer_size + png_ptr->current_buffer_size >
       png_ptr->save_buffer_max)
   {
      png_size_t new_max;
      png_bytep old_buffer;

      if (png_ptr->save_buffer_size > PNG_SIZE_MAX -
          (png_ptr->current_buffer_size + 256))
      {
         png_error(png_ptr, "Potential overflow of save_buffer");
      }

      new_max = png_ptr->save_buffer_size + png_ptr->current_buffer_size + 256;
      old_buffer = png_ptr->save_buffer;
      png_ptr->save_buffer = (png_bytep)png_malloc_warn(png_ptr, new_max);

      if (png_ptr->save_buffer == NULL)
      {
         png_free(png_ptr, old_buffer);
         png_error(png_ptr, "Insufficient memory for save_buffer");
      }

      png_memcpy(png_ptr->save_buffer, old_buffer, png_ptr->save_buffer_size);
      png_free(png_ptr, old_buffer);
      png_ptr->save_buffer_max = new_max;
   }

   if (png_ptr->current_buffer_size)
   {
      png_memcpy(png_ptr->save_buffer + png_ptr->save_buffer_size,
         png_ptr->current_buffer_ptr, png_ptr->current_buffer_size);
      png_ptr->save_buffer_size += png_ptr->current_buffer_size;
      png_ptr->current_buffer_size = 0;
   }

   png_ptr->save_buffer_ptr = png_ptr->save_buffer;
   png_ptr->buffer_size = 0;
}

void 
png_push_restore_buffer(png_structp png_ptr, png_bytep buffer,
   png_size_t buffer_length)
{
   png_ptr->current_buffer = buffer;
   png_ptr->current_buffer_size = buffer_length;
   png_ptr->buffer_size = buffer_length + png_ptr->save_buffer_size;
   png_ptr->current_buffer_ptr = png_ptr->current_buffer;
}

void 
png_push_read_IDAT(png_structp png_ptr)
{
   if (!(png_ptr->mode & PNG_HAVE_CHUNK_HEADER))
   {
      png_byte chunk_length[4];
      png_byte chunk_tag[4];

      
#ifdef PNG_READ_APNG_SUPPORTED
      if (png_ptr->buffer_size < 12)
#else
      if (png_ptr->buffer_size < 8)
#endif
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_push_fill_buffer(png_ptr, chunk_length, 4);
      png_ptr->push_length = png_get_uint_31(png_ptr, chunk_length);
      png_reset_crc(png_ptr);
      png_crc_read(png_ptr, chunk_tag, 4);
      png_ptr->chunk_name = PNG_CHUNK_FROM_STRING(chunk_tag);
      png_ptr->mode |= PNG_HAVE_CHUNK_HEADER;

#ifdef PNG_READ_APNG_SUPPORTED
      if (png_ptr->chunk_name != png_fdAT && png_ptr->num_frames_read > 0)
      {
          if (png_ptr->flags & PNG_FLAG_ZLIB_FINISHED)
          {
              png_ptr->process_mode = PNG_READ_CHUNK_MODE;
              if (png_ptr->frame_end_fn != NULL)
                 (*(png_ptr->frame_end_fn))(png_ptr, png_ptr->num_frames_read);
              png_ptr->num_frames_read++;
              return;
          }
          else
          {
              if (png_ptr->chunk_name == png_IEND)
                  png_error(png_ptr, "Not enough image data");
              if (png_ptr->push_length + 4 > png_ptr->buffer_size)
              {
                 png_push_save_buffer(png_ptr);
                 return;
              }
              png_warning(png_ptr, "Skipping (ignoring) a chunk between "
                                   "APNG chunks");
              png_crc_finish(png_ptr, png_ptr->push_length);
              png_ptr->mode &= ~PNG_HAVE_CHUNK_HEADER;
              return;
          }
      }
      else
#endif
#ifdef PNG_READ_APNG_SUPPORTED
      if (png_ptr->chunk_name != png_IDAT && png_ptr->num_frames_read == 0)
#else
      if (png_ptr->chunk_name != png_IDAT)
#endif
      {
         png_ptr->process_mode = PNG_READ_CHUNK_MODE;

         if (!(png_ptr->flags & PNG_FLAG_ZLIB_FINISHED))
            png_error(png_ptr, "Not enough compressed data");

#ifdef PNG_READ_APNG_SUPPORTED
         if (png_ptr->frame_end_fn != NULL)
            (*(png_ptr->frame_end_fn))(png_ptr, png_ptr->num_frames_read);
         png_ptr->num_frames_read++;
#endif

         return;
      }

      png_ptr->idat_size = png_ptr->push_length;

#ifdef PNG_READ_APNG_SUPPORTED
      if (png_ptr->num_frames_read > 0)
      {
         png_ensure_sequence_number(png_ptr, 4);
         png_ptr->idat_size -= 4;
      }
#endif
   }

   if (png_ptr->idat_size && png_ptr->save_buffer_size)
   {
      png_size_t save_size = png_ptr->save_buffer_size;
      png_uint_32 idat_size = png_ptr->idat_size;

      





      if (idat_size < save_size)
         save_size = (png_size_t)idat_size;

      else
         idat_size = (png_uint_32)save_size;

      png_calculate_crc(png_ptr, png_ptr->save_buffer_ptr, save_size);

      png_process_IDAT_data(png_ptr, png_ptr->save_buffer_ptr, save_size);

      png_ptr->idat_size -= idat_size;
      png_ptr->buffer_size -= save_size;
      png_ptr->save_buffer_size -= save_size;
      png_ptr->save_buffer_ptr += save_size;
   }

   if (png_ptr->idat_size && png_ptr->current_buffer_size)
   {
      png_size_t save_size = png_ptr->current_buffer_size;
      png_uint_32 idat_size = png_ptr->idat_size;

      




      if (idat_size < save_size)
         save_size = (png_size_t)idat_size;

      else
         idat_size = (png_uint_32)save_size;

      png_calculate_crc(png_ptr, png_ptr->current_buffer_ptr, save_size);

      png_process_IDAT_data(png_ptr, png_ptr->current_buffer_ptr, save_size);

      png_ptr->idat_size -= idat_size;
      png_ptr->buffer_size -= save_size;
      png_ptr->current_buffer_size -= save_size;
      png_ptr->current_buffer_ptr += save_size;
   }

   if (!png_ptr->idat_size)
   {
      if (png_ptr->buffer_size < 4)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_crc_finish(png_ptr, 0);
      png_ptr->mode &= ~PNG_HAVE_CHUNK_HEADER;
      png_ptr->mode |= PNG_AFTER_IDAT;
   }
}

void 
png_process_IDAT_data(png_structp png_ptr, png_bytep buffer,
   png_size_t buffer_length)
{
   
   if (!(buffer_length > 0) || buffer == NULL)
      png_error(png_ptr, "No IDAT data (internal error)");

#ifdef PNG_READ_APNG_SUPPORTED
   
   if (!(png_ptr->apng_flags & PNG_APNG_APP) && png_ptr->num_frames_read > 0)
   {
     png_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;
     return;
   }
#endif

   



   png_ptr->zstream.next_in = buffer;
   png_ptr->zstream.avail_in = (uInt)buffer_length;

   


   while (png_ptr->zstream.avail_in > 0 &&
          !(png_ptr->flags & PNG_FLAG_ZLIB_FINISHED))
   {
      int ret;

      




      if (!(png_ptr->zstream.avail_out > 0))
      {
         png_ptr->zstream.avail_out =
             (uInt) PNG_ROWBYTES(png_ptr->pixel_depth,
             png_ptr->iwidth) + 1;

         png_ptr->zstream.next_out = png_ptr->row_buf;
      }

      






      ret = inflate(&png_ptr->zstream, Z_SYNC_FLUSH);

      
      if (ret != Z_OK && ret != Z_STREAM_END)
      {
         
         png_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;

         


         if (png_ptr->row_number >= png_ptr->num_rows ||
             png_ptr->pass > 6)
            png_warning(png_ptr, "Truncated compressed data in IDAT");

         else
            png_error(png_ptr, "Decompression error in IDAT");

         
         return;
      }

      
      if (png_ptr->zstream.next_out != png_ptr->row_buf)
      {
         



         if (png_ptr->row_number >= png_ptr->num_rows ||
             png_ptr->pass > 6)
         {
            
            png_warning(png_ptr, "Extra compressed data in IDAT");
            png_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;

            


            return;
         }

         
         if (png_ptr->zstream.avail_out == 0)
            png_push_process_row(png_ptr);
      }

      
      if (ret == Z_STREAM_END)
         png_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;
   }

   



   if (png_ptr->zstream.avail_in > 0)
      png_warning(png_ptr, "Extra compression data in IDAT");
}

void 
png_push_process_row(png_structp png_ptr)
{
   
   png_row_info row_info;

   row_info.width = png_ptr->iwidth; 
   row_info.color_type = png_ptr->color_type;
   row_info.bit_depth = png_ptr->bit_depth;
   row_info.channels = png_ptr->channels;
   row_info.pixel_depth = png_ptr->pixel_depth;
   row_info.rowbytes = PNG_ROWBYTES(row_info.pixel_depth, row_info.width);

   if (png_ptr->row_buf[0] > PNG_FILTER_VALUE_NONE)
   {
      if (png_ptr->row_buf[0] < PNG_FILTER_VALUE_LAST)
         png_read_filter_row(png_ptr, &row_info, png_ptr->row_buf + 1,
            png_ptr->prev_row + 1, png_ptr->row_buf[0]);
      else
         png_error(png_ptr, "bad adaptive filter value");
   }

   




   png_memcpy(png_ptr->prev_row, png_ptr->row_buf, row_info.rowbytes + 1);

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
   if (png_ptr->transformations)
      png_do_read_transformations(png_ptr, &row_info);
#endif

   
   if (png_ptr->transformed_pixel_depth == 0)
   {
      png_ptr->transformed_pixel_depth = row_info.pixel_depth;
      if (row_info.pixel_depth > png_ptr->maximum_pixel_depth)
         png_error(png_ptr, "progressive row overflow");
   }

   else if (png_ptr->transformed_pixel_depth != row_info.pixel_depth)
      png_error(png_ptr, "internal progressive row size calculation error");


#ifdef PNG_READ_INTERLACING_SUPPORTED
   
   if (png_ptr->interlaced && (png_ptr->transformations & PNG_INTERLACE))
   {
      if (png_ptr->pass < 6)
         png_do_read_interlace(&row_info, png_ptr->row_buf + 1, png_ptr->pass,
            png_ptr->transformations);

    switch (png_ptr->pass)
    {
         case 0:
         {
            int i;
            for (i = 0; i < 8 && png_ptr->pass == 0; i++)
            {
               png_push_have_row(png_ptr, png_ptr->row_buf + 1);
               png_read_push_finish_row(png_ptr); 
            }

            if (png_ptr->pass == 2) 
            {
               for (i = 0; i < 4 && png_ptr->pass == 2; i++)
               {
                  png_push_have_row(png_ptr, NULL);
                  png_read_push_finish_row(png_ptr);
               }
            }

            if (png_ptr->pass == 4 && png_ptr->height <= 4)
            {
               for (i = 0; i < 2 && png_ptr->pass == 4; i++)
               {
                  png_push_have_row(png_ptr, NULL);
                  png_read_push_finish_row(png_ptr);
               }
            }

            if (png_ptr->pass == 6 && png_ptr->height <= 4)
            {
                png_push_have_row(png_ptr, NULL);
                png_read_push_finish_row(png_ptr);
            }

            break;
         }

         case 1:
         {
            int i;
            for (i = 0; i < 8 && png_ptr->pass == 1; i++)
            {
               png_push_have_row(png_ptr, png_ptr->row_buf + 1);
               png_read_push_finish_row(png_ptr);
            }

            if (png_ptr->pass == 2) 
            {
               for (i = 0; i < 4 && png_ptr->pass == 2; i++)
               {
                  png_push_have_row(png_ptr, NULL);
                  png_read_push_finish_row(png_ptr);
               }
            }

            break;
         }

         case 2:
         {
            int i;

            for (i = 0; i < 4 && png_ptr->pass == 2; i++)
            {
               png_push_have_row(png_ptr, png_ptr->row_buf + 1);
               png_read_push_finish_row(png_ptr);
            }

            for (i = 0; i < 4 && png_ptr->pass == 2; i++)
            {
               png_push_have_row(png_ptr, NULL);
               png_read_push_finish_row(png_ptr);
            }

            if (png_ptr->pass == 4) 
            {
               for (i = 0; i < 2 && png_ptr->pass == 4; i++)
               {
                  png_push_have_row(png_ptr, NULL);
                  png_read_push_finish_row(png_ptr);
               }
            }

            break;
         }

         case 3:
         {
            int i;

            for (i = 0; i < 4 && png_ptr->pass == 3; i++)
            {
               png_push_have_row(png_ptr, png_ptr->row_buf + 1);
               png_read_push_finish_row(png_ptr);
            }

            if (png_ptr->pass == 4) 
            {
               for (i = 0; i < 2 && png_ptr->pass == 4; i++)
               {
                  png_push_have_row(png_ptr, NULL);
                  png_read_push_finish_row(png_ptr);
               }
            }

            break;
         }

         case 4:
         {
            int i;

            for (i = 0; i < 2 && png_ptr->pass == 4; i++)
            {
               png_push_have_row(png_ptr, png_ptr->row_buf + 1);
               png_read_push_finish_row(png_ptr);
            }

            for (i = 0; i < 2 && png_ptr->pass == 4; i++)
            {
               png_push_have_row(png_ptr, NULL);
               png_read_push_finish_row(png_ptr);
            }

            if (png_ptr->pass == 6) 
            {
               png_push_have_row(png_ptr, NULL);
               png_read_push_finish_row(png_ptr);
            }

            break;
         }

         case 5:
         {
            int i;

            for (i = 0; i < 2 && png_ptr->pass == 5; i++)
            {
               png_push_have_row(png_ptr, png_ptr->row_buf + 1);
               png_read_push_finish_row(png_ptr);
            }

            if (png_ptr->pass == 6) 
            {
               png_push_have_row(png_ptr, NULL);
               png_read_push_finish_row(png_ptr);
            }

            break;
         }

         default:
         case 6:
         {
            png_push_have_row(png_ptr, png_ptr->row_buf + 1);
            png_read_push_finish_row(png_ptr);

            if (png_ptr->pass != 6)
               break;

            png_push_have_row(png_ptr, NULL);
            png_read_push_finish_row(png_ptr);
         }
      }
   }
   else
#endif
   {
      png_push_have_row(png_ptr, png_ptr->row_buf + 1);
      png_read_push_finish_row(png_ptr);
   }
}

void 
png_read_push_finish_row(png_structp png_ptr)
{
#ifdef PNG_READ_INTERLACING_SUPPORTED
   

   
   static PNG_CONST png_byte FARDATA png_pass_start[] = {0, 4, 0, 2, 0, 1, 0};

   
   static PNG_CONST png_byte FARDATA png_pass_inc[] = {8, 8, 4, 4, 2, 2, 1};

   
   static PNG_CONST png_byte FARDATA png_pass_ystart[] = {0, 0, 4, 0, 2, 0, 1};

   
   static PNG_CONST png_byte FARDATA png_pass_yinc[] = {8, 8, 8, 4, 4, 2, 2};

   



#endif

   png_ptr->row_number++;
   if (png_ptr->row_number < png_ptr->num_rows)
      return;

#ifdef PNG_READ_INTERLACING_SUPPORTED
   if (png_ptr->interlaced)
   {
      png_ptr->row_number = 0;
      png_memset(png_ptr->prev_row, 0, png_ptr->rowbytes + 1);

      do
      {
         png_ptr->pass++;
         if ((png_ptr->pass == 1 && png_ptr->width < 5) ||
             (png_ptr->pass == 3 && png_ptr->width < 3) ||
             (png_ptr->pass == 5 && png_ptr->width < 2))
            png_ptr->pass++;

         if (png_ptr->pass > 7)
            png_ptr->pass--;

         if (png_ptr->pass >= 7)
            break;

         png_ptr->iwidth = (png_ptr->width +
             png_pass_inc[png_ptr->pass] - 1 -
             png_pass_start[png_ptr->pass]) /
             png_pass_inc[png_ptr->pass];

         if (png_ptr->transformations & PNG_INTERLACE)
            break;

         png_ptr->num_rows = (png_ptr->height +
             png_pass_yinc[png_ptr->pass] - 1 -
             png_pass_ystart[png_ptr->pass]) /
             png_pass_yinc[png_ptr->pass];

      } while (png_ptr->iwidth == 0 || png_ptr->num_rows == 0);
   }
#endif 
}

void 
png_push_have_info(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr->info_fn != NULL)
      (*(png_ptr->info_fn))(png_ptr, info_ptr);
}

void 
png_push_have_end(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr->end_fn != NULL)
      (*(png_ptr->end_fn))(png_ptr, info_ptr);
}

void 
png_push_have_row(png_structp png_ptr, png_bytep row)
{
   if (png_ptr->row_fn != NULL)
      (*(png_ptr->row_fn))(png_ptr, row, png_ptr->row_number,
         (int)png_ptr->pass);
}

#ifdef PNG_READ_INTERLACING_SUPPORTED
void PNGAPI
png_progressive_combine_row (png_structp png_ptr, png_bytep old_row,
    png_const_bytep new_row)
{
   if (png_ptr == NULL)
      return;

   



   if (new_row != NULL)
      png_combine_row(png_ptr, old_row, 1);
}
#endif 

void PNGAPI
png_set_progressive_read_fn(png_structp png_ptr, png_voidp progressive_ptr,
    png_progressive_info_ptr info_fn, png_progressive_row_ptr row_fn,
    png_progressive_end_ptr end_fn)
{
   if (png_ptr == NULL)
      return;

   png_ptr->info_fn = info_fn;
   png_ptr->row_fn = row_fn;
   png_ptr->end_fn = end_fn;

   png_set_read_fn(png_ptr, progressive_ptr, png_push_fill_buffer);
}

#ifdef PNG_READ_APNG_SUPPORTED
void PNGAPI
png_set_progressive_frame_fn(png_structp png_ptr,
   png_progressive_frame_ptr frame_info_fn,
   png_progressive_frame_ptr frame_end_fn)
{
   png_ptr->frame_info_fn = frame_info_fn;
   png_ptr->frame_end_fn = frame_end_fn;
   png_ptr->apng_flags |= PNG_APNG_APP;
}
#endif

png_voidp PNGAPI
png_get_progressive_ptr(png_const_structp png_ptr)
{
   if (png_ptr == NULL)
      return (NULL);

   return png_ptr->io_ptr;
}
#endif 

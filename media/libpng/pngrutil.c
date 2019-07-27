















#include "pngpriv.h"

#ifdef PNG_READ_SUPPORTED

png_uint_32 PNGAPI
png_get_uint_31(png_const_structrp png_ptr, png_const_bytep buf)
{
   png_uint_32 uval = png_get_uint_32(buf);

   if (uval > PNG_UINT_31_MAX)
      png_error(png_ptr, "PNG unsigned integer out of range");

   return (uval);
}

#if defined(PNG_READ_gAMA_SUPPORTED) || defined(PNG_READ_cHRM_SUPPORTED)





#define PNG_FIXED_ERROR (-1)

static png_fixed_point 
png_get_fixed_point(png_structrp png_ptr, png_const_bytep buf)
{
   png_uint_32 uval = png_get_uint_32(buf);

   if (uval <= PNG_UINT_31_MAX)
      return (png_fixed_point)uval; 

   
   if (png_ptr != NULL)
      png_warning(png_ptr, "PNG fixed point integer out of range");

   return PNG_FIXED_ERROR;
}
#endif

#ifdef PNG_READ_INT_FUNCTIONS_SUPPORTED










png_uint_32 (PNGAPI
png_get_uint_32)(png_const_bytep buf)
{
   png_uint_32 uval =
       ((png_uint_32)(*(buf    )) << 24) +
       ((png_uint_32)(*(buf + 1)) << 16) +
       ((png_uint_32)(*(buf + 2)) <<  8) +
       ((png_uint_32)(*(buf + 3))      ) ;

   return uval;
}






png_int_32 (PNGAPI
png_get_int_32)(png_const_bytep buf)
{
   png_uint_32 uval = png_get_uint_32(buf);
   if ((uval & 0x80000000) == 0) 
      return uval;

   uval = (uval ^ 0xffffffff) + 1;  
   return -(png_int_32)uval;
}


png_uint_16 (PNGAPI
png_get_uint_16)(png_const_bytep buf)
{
   




   unsigned int val =
       ((unsigned int)(*buf) << 8) +
       ((unsigned int)(*(buf + 1)));

   return (png_uint_16)val;
}

#endif 


void 
png_read_sig(png_structrp png_ptr, png_inforp info_ptr)
{
   png_size_t num_checked, num_to_check;

   
   if (png_ptr->sig_bytes >= 8)
      return;

   num_checked = png_ptr->sig_bytes;
   num_to_check = 8 - num_checked;

#ifdef PNG_IO_STATE_SUPPORTED
   png_ptr->io_state = PNG_IO_READING | PNG_IO_SIGNATURE;
#endif

   
   png_read_data(png_ptr, &(info_ptr->signature[num_checked]), num_to_check);
   png_ptr->sig_bytes = 8;

   if (png_sig_cmp(info_ptr->signature, num_checked, num_to_check) != 0)
   {
      if (num_checked < 4 &&
          png_sig_cmp(info_ptr->signature, num_checked, num_to_check - 4))
         png_error(png_ptr, "Not a PNG file");
      else
         png_error(png_ptr, "PNG file corrupted by ASCII conversion");
   }
   if (num_checked < 3)
      png_ptr->mode |= PNG_HAVE_PNG_SIGNATURE;
}




png_uint_32 
png_read_chunk_header(png_structrp png_ptr)
{
   png_byte buf[8];
   png_uint_32 length;

#ifdef PNG_IO_STATE_SUPPORTED
   png_ptr->io_state = PNG_IO_READING | PNG_IO_CHUNK_HDR;
#endif

   


   png_read_data(png_ptr, buf, 8);
   length = png_get_uint_31(png_ptr, buf);

   
   png_ptr->chunk_name = PNG_CHUNK_FROM_STRING(buf+4);

   png_debug2(0, "Reading %lx chunk, length = %lu",
       (unsigned long)png_ptr->chunk_name, (unsigned long)length);

   
   png_reset_crc(png_ptr);
   png_calculate_crc(png_ptr, buf + 4, 4);

   
   png_check_chunk_name(png_ptr, png_ptr->chunk_name);

#ifdef PNG_IO_STATE_SUPPORTED
   png_ptr->io_state = PNG_IO_READING | PNG_IO_CHUNK_DATA;
#endif

   return length;
}


void 
png_crc_read(png_structrp png_ptr, png_bytep buf, png_uint_32 length)
{
   if (png_ptr == NULL)
      return;

   png_read_data(png_ptr, buf, length);
   png_calculate_crc(png_ptr, buf, length);
}






int 
png_crc_finish(png_structrp png_ptr, png_uint_32 skip)
{
   


   while (skip > 0)
   {
      png_uint_32 len;
      png_byte tmpbuf[PNG_INFLATE_BUF_SIZE];

      len = (sizeof tmpbuf);
      if (len > skip)
         len = skip;
      skip -= len;

      png_crc_read(png_ptr, tmpbuf, len);
   }

   if (png_crc_error(png_ptr) != 0)
   {
      if (PNG_CHUNK_ANCILLARY(png_ptr->chunk_name) != 0 ?
          (png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_NOWARN) == 0 :
          (png_ptr->flags & PNG_FLAG_CRC_CRITICAL_USE) != 0)
      {
         png_chunk_warning(png_ptr, "CRC error");
      }

      else
         png_chunk_error(png_ptr, "CRC error");

      return (1);
   }

   return (0);
}




int 
png_crc_error(png_structrp png_ptr)
{
   png_byte crc_bytes[4];
   png_uint_32 crc;
   int need_crc = 1;

   if (PNG_CHUNK_ANCILLARY(png_ptr->chunk_name) != 0)
   {
      if ((png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_MASK) ==
          (PNG_FLAG_CRC_ANCILLARY_USE | PNG_FLAG_CRC_ANCILLARY_NOWARN))
         need_crc = 0;
   }

   else 
   {
      if ((png_ptr->flags & PNG_FLAG_CRC_CRITICAL_IGNORE) != 0)
         need_crc = 0;
   }

#ifdef PNG_IO_STATE_SUPPORTED
   png_ptr->io_state = PNG_IO_READING | PNG_IO_CHUNK_CRC;
#endif

   
   png_read_data(png_ptr, crc_bytes, 4);

   if (need_crc != 0)
   {
      crc = png_get_uint_32(crc_bytes);
      return ((int)(crc != png_ptr->crc));
   }

   else
      return (0);
}

#if defined(PNG_READ_iCCP_SUPPORTED) || defined(PNG_READ_iTXt_SUPPORTED) ||\
    defined(PNG_READ_pCAL_SUPPORTED) || defined(PNG_READ_sCAL_SUPPORTED) ||\
    defined(PNG_READ_sPLT_SUPPORTED) || defined(PNG_READ_tEXt_SUPPORTED) ||\
    defined(PNG_READ_zTXt_SUPPORTED) || defined(PNG_SEQUENTIAL_READ_SUPPORTED)






static png_bytep
png_read_buffer(png_structrp png_ptr, png_alloc_size_t new_size, int warn)
{
   png_bytep buffer = png_ptr->read_buffer;

   if (buffer != NULL && new_size > png_ptr->read_buffer_size)
   {
      png_ptr->read_buffer = NULL;
      png_ptr->read_buffer = NULL;
      png_ptr->read_buffer_size = 0;
      png_free(png_ptr, buffer);
      buffer = NULL;
   }

   if (buffer == NULL)
   {
      buffer = png_voidcast(png_bytep, png_malloc_base(png_ptr, new_size));

      if (buffer != NULL)
      {
         png_ptr->read_buffer = buffer;
         png_ptr->read_buffer_size = new_size;
      }

      else if (warn < 2) 
      {
         if (warn != 0)
             png_chunk_warning(png_ptr, "insufficient memory to read chunk");

         else
             png_chunk_error(png_ptr, "insufficient memory to read chunk");
      }
   }

   return buffer;
}
#endif 






static int
png_inflate_claim(png_structrp png_ptr, png_uint_32 owner)
{
   if (png_ptr->zowner != 0)
   {
      char msg[64];

      PNG_STRING_FROM_CHUNK(msg, png_ptr->zowner);
      



      (void)png_safecat(msg, (sizeof msg), 4, " using zstream");
#if PNG_LIBPNG_BUILD_BASE_TYPE >= PNG_LIBPNG_BUILD_RC
      png_chunk_warning(png_ptr, msg);
      png_ptr->zowner = 0;
#else
      png_chunk_error(png_ptr, msg);
#endif
   }

   












   {
      int ret; 
#if PNG_ZLIB_VERNUM >= 0x1240

# if defined(PNG_SET_OPTION_SUPPORTED) && defined(PNG_MAXIMUM_INFLATE_WINDOW)
      int window_bits;

      if (((png_ptr->options >> PNG_MAXIMUM_INFLATE_WINDOW) & 3) ==
          PNG_OPTION_ON)
         window_bits = 15;

      else
         window_bits = 0;
# else
#   define window_bits 0
# endif
#endif

      


      png_ptr->zstream.next_in = NULL;
      png_ptr->zstream.avail_in = 0;
      png_ptr->zstream.next_out = NULL;
      png_ptr->zstream.avail_out = 0;

      if ((png_ptr->flags & PNG_FLAG_ZSTREAM_INITIALIZED) != 0)
      {
#if PNG_ZLIB_VERNUM < 0x1240
         ret = inflateReset(&png_ptr->zstream);
#else
         ret = inflateReset2(&png_ptr->zstream, window_bits);
#endif
      }

      else
      {
#if PNG_ZLIB_VERNUM < 0x1240
         ret = inflateInit(&png_ptr->zstream);
#else
         ret = inflateInit2(&png_ptr->zstream, window_bits);
#endif

         if (ret == Z_OK)
            png_ptr->flags |= PNG_FLAG_ZSTREAM_INITIALIZED;
      }

      if (ret == Z_OK)
         png_ptr->zowner = owner;

      else
         png_zstream_error(png_ptr, ret);

      return ret;
   }

#ifdef window_bits
# undef window_bits
#endif
}

#ifdef PNG_READ_COMPRESSED_TEXT_SUPPORTED











static int
png_inflate(png_structrp png_ptr, png_uint_32 owner, int finish,
     png_const_bytep input, png_uint_32p input_size_ptr,
     png_bytep output, png_alloc_size_t *output_size_ptr)
{
   if (png_ptr->zowner == owner) 
   {
      int ret;
      png_alloc_size_t avail_out = *output_size_ptr;
      png_uint_32 avail_in = *input_size_ptr;

      








      png_ptr->zstream.next_in = PNGZ_INPUT_CAST(input);
      
      png_ptr->zstream.avail_in = 0;
      png_ptr->zstream.avail_out = 0;

      


      if (output != NULL)
         png_ptr->zstream.next_out = output;

      do
      {
         uInt avail;
         Byte local_buffer[PNG_INFLATE_BUF_SIZE];

         
         






         avail_in += png_ptr->zstream.avail_in; 

         avail = ZLIB_IO_MAX;

         if (avail_in < avail)
            avail = (uInt)avail_in; 

         avail_in -= avail;
         png_ptr->zstream.avail_in = avail;

         
         avail_out += png_ptr->zstream.avail_out; 

         avail = ZLIB_IO_MAX; 

         if (output == NULL)
         {
            


            png_ptr->zstream.next_out = local_buffer;
            if ((sizeof local_buffer) < avail)
               avail = (sizeof local_buffer);
         }

         if (avail_out < avail)
            avail = (uInt)avail_out; 

         png_ptr->zstream.avail_out = avail;
         avail_out -= avail;

         
         




         ret = inflate(&png_ptr->zstream, avail_out > 0 ? Z_NO_FLUSH :
             (finish ? Z_FINISH : Z_SYNC_FLUSH));
      } while (ret == Z_OK);

      
      if (output == NULL)
         png_ptr->zstream.next_out = NULL;

      
      avail_in += png_ptr->zstream.avail_in;
      avail_out += png_ptr->zstream.avail_out;

      


      if (avail_out > 0)
         *output_size_ptr -= avail_out;

      if (avail_in > 0)
         *input_size_ptr -= avail_in;

      
      png_zstream_error(png_ptr, ret);
      return ret;
   }

   else
   {
      



      png_ptr->zstream.msg = PNGZ_MSG_CAST("zstream unclaimed");
      return Z_STREAM_ERROR;
   }
}








static int
png_decompress_chunk(png_structrp png_ptr,
   png_uint_32 chunklength, png_uint_32 prefix_size,
   png_alloc_size_t *newlength ,
   int terminate )
{
   






   png_alloc_size_t limit = PNG_SIZE_MAX;

# ifdef PNG_SET_CHUNK_MALLOC_LIMIT_SUPPORTED
   if (png_ptr->user_chunk_malloc_max > 0 &&
       png_ptr->user_chunk_malloc_max < limit)
      limit = png_ptr->user_chunk_malloc_max;
# elif PNG_USER_CHUNK_MALLOC_MAX > 0
   if (PNG_USER_CHUNK_MALLOC_MAX < limit)
      limit = PNG_USER_CHUNK_MALLOC_MAX;
# endif

   if (limit >= prefix_size + (terminate != 0))
   {
      int ret;

      limit -= prefix_size + (terminate != 0);

      if (limit < *newlength)
         *newlength = limit;

      
      ret = png_inflate_claim(png_ptr, png_ptr->chunk_name);

      if (ret == Z_OK)
      {
         png_uint_32 lzsize = chunklength - prefix_size;

         ret = png_inflate(png_ptr, png_ptr->chunk_name, 1,
             png_ptr->read_buffer + prefix_size, &lzsize,
             NULL, newlength);

         if (ret == Z_STREAM_END)
         {
            






            if (inflateReset(&png_ptr->zstream) == Z_OK)
            {
               




               png_alloc_size_t new_size = *newlength;
               png_alloc_size_t buffer_size = prefix_size + new_size +
                  (terminate != 0);
               png_bytep text = png_voidcast(png_bytep, png_malloc_base(png_ptr,
                  buffer_size));

               if (text != NULL)
               {
                  ret = png_inflate(png_ptr, png_ptr->chunk_name, 1,
                     png_ptr->read_buffer + prefix_size, &lzsize,
                     text + prefix_size, newlength);

                  if (ret == Z_STREAM_END)
                  {
                     if (new_size == *newlength)
                     {
                        if (terminate != 0)
                           text[prefix_size + *newlength] = 0;

                        if (prefix_size > 0)
                           memcpy(text, png_ptr->read_buffer, prefix_size);

                        {
                           png_bytep old_ptr = png_ptr->read_buffer;

                           png_ptr->read_buffer = text;
                           png_ptr->read_buffer_size = buffer_size;
                           text = old_ptr; 
                        }
                     }

                     else
                     {
                        





                        ret = PNG_UNEXPECTED_ZLIB_RETURN;
                     }
                  }

                  else if (ret == Z_OK)
                     ret = PNG_UNEXPECTED_ZLIB_RETURN; 

                  


                  png_free(png_ptr, text);
                  text = NULL;

                  


                  if (ret == Z_STREAM_END &&
                     chunklength - prefix_size != lzsize)
                     png_chunk_benign_error(png_ptr, "extra compressed data");
               }

               else
               {
                  
                  ret = Z_MEM_ERROR;
                  png_zstream_error(png_ptr, Z_MEM_ERROR);
               }
            }

            else
            {
               
               png_zstream_error(png_ptr, ret);

               if (ret == Z_STREAM_END)
                  ret = PNG_UNEXPECTED_ZLIB_RETURN;
            }
         }

         else if (ret == Z_OK)
            ret = PNG_UNEXPECTED_ZLIB_RETURN;

         
         png_ptr->zowner = 0;
      }

      else  if (ret == Z_STREAM_END) 
         ret = PNG_UNEXPECTED_ZLIB_RETURN;

      return ret;
   }

   else
   {
      
      png_zstream_error(png_ptr, Z_MEM_ERROR);
      return Z_MEM_ERROR;
   }
}
#endif 

#ifdef PNG_READ_iCCP_SUPPORTED



static int
png_inflate_read(png_structrp png_ptr, png_bytep read_buffer, uInt read_size,
   png_uint_32p chunk_bytes, png_bytep next_out, png_alloc_size_t *out_size,
   int finish)
{
   if (png_ptr->zowner == png_ptr->chunk_name)
   {
      int ret;

      
      png_ptr->zstream.next_out = next_out;
      png_ptr->zstream.avail_out = 0; 

      do
      {
         if (png_ptr->zstream.avail_in == 0)
         {
            if (read_size > *chunk_bytes)
               read_size = (uInt)*chunk_bytes;
            *chunk_bytes -= read_size;

            if (read_size > 0)
               png_crc_read(png_ptr, read_buffer, read_size);

            png_ptr->zstream.next_in = read_buffer;
            png_ptr->zstream.avail_in = read_size;
         }

         if (png_ptr->zstream.avail_out == 0)
         {
            uInt avail = ZLIB_IO_MAX;
            if (avail > *out_size)
               avail = (uInt)*out_size;
            *out_size -= avail;

            png_ptr->zstream.avail_out = avail;
         }

         



         ret = inflate(&png_ptr->zstream,
            *chunk_bytes > 0 ? Z_NO_FLUSH : (finish ? Z_FINISH : Z_SYNC_FLUSH));
      }
      while (ret == Z_OK && (*out_size > 0 || png_ptr->zstream.avail_out > 0));

      *out_size += png_ptr->zstream.avail_out;
      png_ptr->zstream.avail_out = 0; 

      
      png_zstream_error(png_ptr, ret);
      return ret;
   }

   else
   {
      png_ptr->zstream.msg = PNGZ_MSG_CAST("zstream unclaimed");
      return Z_STREAM_ERROR;
   }
}
#endif



void 
png_handle_IHDR(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_byte buf[13];
   png_uint_32 width, height;
   int bit_depth, color_type, compression_type, filter_type;
   int interlace_type;

   png_debug(1, "in png_handle_IHDR");

   if ((png_ptr->mode & PNG_HAVE_IHDR) != 0)
      png_chunk_error(png_ptr, "out of place");

   
   if (length != 13)
      png_chunk_error(png_ptr, "invalid");

   png_ptr->mode |= PNG_HAVE_IHDR;

   png_crc_read(png_ptr, buf, 13);
   png_crc_finish(png_ptr, 0);

   width = png_get_uint_31(png_ptr, buf);
   height = png_get_uint_31(png_ptr, buf + 4);
   bit_depth = buf[8];
   color_type = buf[9];
   compression_type = buf[10];
   filter_type = buf[11];
   interlace_type = buf[12];

#ifdef PNG_READ_APNG_SUPPORTED
   png_ptr->first_frame_width = width;
   png_ptr->first_frame_height = height;
#endif

   
   png_ptr->width = width;
   png_ptr->height = height;
   png_ptr->bit_depth = (png_byte)bit_depth;
   png_ptr->interlaced = (png_byte)interlace_type;
   png_ptr->color_type = (png_byte)color_type;
#ifdef PNG_MNG_FEATURES_SUPPORTED
   png_ptr->filter_type = (png_byte)filter_type;
#endif
   png_ptr->compression_type = (png_byte)compression_type;

   
   switch (png_ptr->color_type)
   {
      default: 
      case PNG_COLOR_TYPE_GRAY:
      case PNG_COLOR_TYPE_PALETTE:
         png_ptr->channels = 1;
         break;

      case PNG_COLOR_TYPE_RGB:
         png_ptr->channels = 3;
         break;

      case PNG_COLOR_TYPE_GRAY_ALPHA:
         png_ptr->channels = 2;
         break;

      case PNG_COLOR_TYPE_RGB_ALPHA:
         png_ptr->channels = 4;
         break;
   }

   
   png_ptr->pixel_depth = (png_byte)(png_ptr->bit_depth * png_ptr->channels);
   png_ptr->rowbytes = PNG_ROWBYTES(png_ptr->pixel_depth, png_ptr->width);
   png_debug1(3, "bit_depth = %d", png_ptr->bit_depth);
   png_debug1(3, "channels = %d", png_ptr->channels);
   png_debug1(3, "rowbytes = %lu", (unsigned long)png_ptr->rowbytes);
   png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth,
       color_type, interlace_type, compression_type, filter_type);
}


void 
png_handle_PLTE(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_color palette[PNG_MAX_PALETTE_LENGTH];
   int num, i;
#ifdef PNG_POINTER_INDEXING_SUPPORTED
   png_colorp pal_ptr;
#endif

   png_debug(1, "in png_handle_PLTE");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   




   else if ((png_ptr->mode & PNG_HAVE_PLTE) != 0)
      png_chunk_error(png_ptr, "duplicate");

   else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
   {
      


      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   png_ptr->mode |= PNG_HAVE_PLTE;

   if ((png_ptr->color_type & PNG_COLOR_MASK_COLOR) == 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "ignored in grayscale PNG");
      return;
   }

#ifndef PNG_READ_OPT_PLTE_SUPPORTED
   if (png_ptr->color_type != PNG_COLOR_TYPE_PALETTE)
   {
      png_crc_finish(png_ptr, length);
      return;
   }
#endif

   if (length > 3*PNG_MAX_PALETTE_LENGTH || length % 3)
   {
      png_crc_finish(png_ptr, length);

      if (png_ptr->color_type != PNG_COLOR_TYPE_PALETTE)
         png_chunk_benign_error(png_ptr, "invalid");

      else
         png_chunk_error(png_ptr, "invalid");

      return;
   }

   
   num = (int)length / 3;

#ifdef PNG_POINTER_INDEXING_SUPPORTED
   for (i = 0, pal_ptr = palette; i < num; i++, pal_ptr++)
   {
      png_byte buf[3];

      png_crc_read(png_ptr, buf, 3);
      pal_ptr->red = buf[0];
      pal_ptr->green = buf[1];
      pal_ptr->blue = buf[2];
   }
#else
   for (i = 0; i < num; i++)
   {
      png_byte buf[3];

      png_crc_read(png_ptr, buf, 3);
      
      palette[i].red = buf[0];
      palette[i].green = buf[1];
      palette[i].blue = buf[2];
   }
#endif

   




#ifndef PNG_READ_OPT_PLTE_SUPPORTED
   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
#endif
   {
      png_crc_finish(png_ptr, 0);
   }

#ifndef PNG_READ_OPT_PLTE_SUPPORTED
   else if (png_crc_error(png_ptr) != 0)  
   {
      








      if ((png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_USE) == 0)
      {
         if ((png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_NOWARN) != 0)
            return;

         else
            png_chunk_error(png_ptr, "CRC error");
      }

      
      else if ((png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_NOWARN) == 0)
         png_chunk_warning(png_ptr, "CRC error");
   }
#endif

   







   png_set_PLTE(png_ptr, info_ptr, palette, num);

   








#ifdef PNG_READ_tRNS_SUPPORTED
   if (png_ptr->num_trans > 0 ||
       (info_ptr != NULL && (info_ptr->valid & PNG_INFO_tRNS) != 0))
   {
      



      png_ptr->num_trans = 0;

      if (info_ptr != NULL)
         info_ptr->num_trans = 0;

      png_chunk_benign_error(png_ptr, "tRNS must be after");
   }
#endif

#ifdef PNG_READ_hIST_SUPPORTED
   if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_hIST) != 0)
      png_chunk_benign_error(png_ptr, "hIST must be after");
#endif

#ifdef PNG_READ_bKGD_SUPPORTED
   if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_bKGD) != 0)
      png_chunk_benign_error(png_ptr, "bKGD must be after");
#endif
}

void 
png_handle_IEND(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_debug(1, "in png_handle_IEND");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0 ||
       (png_ptr->mode & PNG_HAVE_IDAT) == 0)
      png_chunk_error(png_ptr, "out of place");

   png_ptr->mode |= (PNG_AFTER_IDAT | PNG_HAVE_IEND);

   png_crc_finish(png_ptr, length);

   if (length != 0)
      png_chunk_benign_error(png_ptr, "invalid");

   PNG_UNUSED(info_ptr)
}

#ifdef PNG_READ_gAMA_SUPPORTED
void 
png_handle_gAMA(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_fixed_point igamma;
   png_byte buf[4];

   png_debug(1, "in png_handle_gAMA");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & (PNG_HAVE_IDAT|PNG_HAVE_PLTE)) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   if (length != 4)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "invalid");
      return;
   }

   png_crc_read(png_ptr, buf, 4);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   igamma = png_get_fixed_point(NULL, buf);

   png_colorspace_set_gamma(png_ptr, &png_ptr->colorspace, igamma);
   png_colorspace_sync(png_ptr, info_ptr);
}
#endif

#ifdef PNG_READ_sBIT_SUPPORTED
void 
png_handle_sBIT(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   unsigned int truelen, i;
   png_byte sample_depth;
   png_byte buf[4];

   png_debug(1, "in png_handle_sBIT");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & (PNG_HAVE_IDAT|PNG_HAVE_PLTE)) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_sBIT) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "duplicate");
      return;
   }

   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      truelen = 3;
      sample_depth = 8;
   }

   else
   {
      truelen = png_ptr->channels;
      sample_depth = png_ptr->bit_depth;
   }

   if (length != truelen || length > 4)
   {
      png_chunk_benign_error(png_ptr, "invalid");
      png_crc_finish(png_ptr, length);
      return;
   }

   buf[0] = buf[1] = buf[2] = buf[3] = sample_depth;
   png_crc_read(png_ptr, buf, truelen);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   for (i=0; i<truelen; ++i)
      if (buf[i] == 0 || buf[i] > sample_depth)
      {
         png_chunk_benign_error(png_ptr, "invalid");
         return;
      }

   if ((png_ptr->color_type & PNG_COLOR_MASK_COLOR) != 0)
   {
      png_ptr->sig_bit.red = buf[0];
      png_ptr->sig_bit.green = buf[1];
      png_ptr->sig_bit.blue = buf[2];
      png_ptr->sig_bit.alpha = buf[3];
   }

   else
   {
      png_ptr->sig_bit.gray = buf[0];
      png_ptr->sig_bit.red = buf[0];
      png_ptr->sig_bit.green = buf[0];
      png_ptr->sig_bit.blue = buf[0];
      png_ptr->sig_bit.alpha = buf[1];
   }

   png_set_sBIT(png_ptr, info_ptr, &(png_ptr->sig_bit));
}
#endif

#ifdef PNG_READ_cHRM_SUPPORTED
void 
png_handle_cHRM(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_byte buf[32];
   png_xy xy;

   png_debug(1, "in png_handle_cHRM");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & (PNG_HAVE_IDAT|PNG_HAVE_PLTE)) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   if (length != 32)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "invalid");
      return;
   }

   png_crc_read(png_ptr, buf, 32);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   xy.whitex = png_get_fixed_point(NULL, buf);
   xy.whitey = png_get_fixed_point(NULL, buf + 4);
   xy.redx   = png_get_fixed_point(NULL, buf + 8);
   xy.redy   = png_get_fixed_point(NULL, buf + 12);
   xy.greenx = png_get_fixed_point(NULL, buf + 16);
   xy.greeny = png_get_fixed_point(NULL, buf + 20);
   xy.bluex  = png_get_fixed_point(NULL, buf + 24);
   xy.bluey  = png_get_fixed_point(NULL, buf + 28);

   if (xy.whitex == PNG_FIXED_ERROR ||
       xy.whitey == PNG_FIXED_ERROR ||
       xy.redx   == PNG_FIXED_ERROR ||
       xy.redy   == PNG_FIXED_ERROR ||
       xy.greenx == PNG_FIXED_ERROR ||
       xy.greeny == PNG_FIXED_ERROR ||
       xy.bluex  == PNG_FIXED_ERROR ||
       xy.bluey  == PNG_FIXED_ERROR)
   {
      png_chunk_benign_error(png_ptr, "invalid values");
      return;
   }

   
   if ((png_ptr->colorspace.flags & PNG_COLORSPACE_INVALID) != 0)
      return;

   if ((png_ptr->colorspace.flags & PNG_COLORSPACE_FROM_cHRM) != 0)
   {
      png_ptr->colorspace.flags |= PNG_COLORSPACE_INVALID;
      png_colorspace_sync(png_ptr, info_ptr);
      png_chunk_benign_error(png_ptr, "duplicate");
      return;
   }

   png_ptr->colorspace.flags |= PNG_COLORSPACE_FROM_cHRM;
   (void)png_colorspace_set_chromaticities(png_ptr, &png_ptr->colorspace, &xy,
      1);
   png_colorspace_sync(png_ptr, info_ptr);
}
#endif

#ifdef PNG_READ_sRGB_SUPPORTED
void 
png_handle_sRGB(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_byte intent;

   png_debug(1, "in png_handle_sRGB");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & (PNG_HAVE_IDAT|PNG_HAVE_PLTE)) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   if (length != 1)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "invalid");
      return;
   }

   png_crc_read(png_ptr, &intent, 1);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   
   if ((png_ptr->colorspace.flags & PNG_COLORSPACE_INVALID) != 0)
      return;

   


   if ((png_ptr->colorspace.flags & PNG_COLORSPACE_HAVE_INTENT) != 0)
   {
      png_ptr->colorspace.flags |= PNG_COLORSPACE_INVALID;
      png_colorspace_sync(png_ptr, info_ptr);
      png_chunk_benign_error(png_ptr, "too many profiles");
      return;
   }

   (void)png_colorspace_set_sRGB(png_ptr, &png_ptr->colorspace, intent);
   png_colorspace_sync(png_ptr, info_ptr);
}
#endif 

#ifdef PNG_READ_iCCP_SUPPORTED
void 
png_handle_iCCP(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)

{
   png_const_charp errmsg = NULL; 
   int finished = 0; 

   png_debug(1, "in png_handle_iCCP");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & (PNG_HAVE_IDAT|PNG_HAVE_PLTE)) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   







   if (length < 9)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "too short");
      return;
   }

   
   if ((png_ptr->colorspace.flags & PNG_COLORSPACE_INVALID) != 0)
   {
      png_crc_finish(png_ptr, length);
      return;
   }

   


   if ((png_ptr->colorspace.flags & PNG_COLORSPACE_HAVE_INTENT) == 0)
   {
      uInt read_length, keyword_length;
      char keyword[81];

      


      read_length = 81; 
      if (read_length > length)
         read_length = (uInt)length;

      png_crc_read(png_ptr, (png_bytep)keyword, read_length);
      length -= read_length;

      keyword_length = 0;
      while (keyword_length < 80 && keyword_length < read_length &&
         keyword[keyword_length] != 0)
         ++keyword_length;

      
      if (keyword_length >= 1 && keyword_length <= 79)
      {
         


         if (keyword_length+1 < read_length &&
            keyword[keyword_length+1] == PNG_COMPRESSION_TYPE_BASE)
         {
            read_length -= keyword_length+2;

            if (png_inflate_claim(png_ptr, png_iCCP) == Z_OK)
            {
               Byte profile_header[132];
               Byte local_buffer[PNG_INFLATE_BUF_SIZE];
               png_alloc_size_t size = (sizeof profile_header);

               png_ptr->zstream.next_in = (Bytef*)keyword + (keyword_length+2);
               png_ptr->zstream.avail_in = read_length;
               (void)png_inflate_read(png_ptr, local_buffer,
                  (sizeof local_buffer), &length, profile_header, &size,
                  0);

               if (size == 0)
               {
                  

                  const png_uint_32 profile_length =
                     png_get_uint_32(profile_header);

                  if (png_icc_check_length(png_ptr, &png_ptr->colorspace,
                     keyword, profile_length) != 0)
                  {
                     


                     if (png_icc_check_header(png_ptr, &png_ptr->colorspace,
                        keyword, profile_length, profile_header,
                        png_ptr->color_type) != 0)
                     {
                        




                        const png_uint_32 tag_count = png_get_uint_32(
                           profile_header+128);
                        png_bytep profile = png_read_buffer(png_ptr,
                           profile_length, 2);

                        if (profile != NULL)
                        {
                           memcpy(profile, profile_header,
                              (sizeof profile_header));

                           size = 12 * tag_count;

                           (void)png_inflate_read(png_ptr, local_buffer,
                              (sizeof local_buffer), &length,
                              profile + (sizeof profile_header), &size, 0);

                           


                           if (size == 0)
                           {
                              if (png_icc_check_tag_table(png_ptr,
                                 &png_ptr->colorspace, keyword, profile_length,
                                 profile) != 0)
                              {
                                 


                                 size = profile_length - (sizeof profile_header)
                                    - 12 * tag_count;

                                 (void)png_inflate_read(png_ptr, local_buffer,
                                    (sizeof local_buffer), &length,
                                    profile + (sizeof profile_header) +
                                    12 * tag_count, &size, 1);

                                 if (length > 0 && !(png_ptr->flags &
                                       PNG_FLAG_BENIGN_ERRORS_WARN))
                                    errmsg = "extra compressed data";

                                 
                                 else if (size == 0)
                                 {
                                    if (length > 0)
                                    {
                                       


                                       png_chunk_warning(png_ptr,
                                          "extra compressed data");
                                    }

                                    png_crc_finish(png_ptr, length);
                                    finished = 1;

#                                   ifdef PNG_sRGB_SUPPORTED
                                       
                                       png_icc_set_sRGB(png_ptr,
                                          &png_ptr->colorspace, profile,
                                          png_ptr->zstream.adler);
#                                   endif

                                    
                                    if (info_ptr != NULL)
                                    {
                                       png_free_data(png_ptr, info_ptr,
                                          PNG_FREE_ICCP, 0);

                                       info_ptr->iccp_name = png_voidcast(char*,
                                          png_malloc_base(png_ptr,
                                          keyword_length+1));
                                       if (info_ptr->iccp_name != NULL)
                                       {
                                          memcpy(info_ptr->iccp_name, keyword,
                                             keyword_length+1);
                                          info_ptr->iccp_proflen =
                                             profile_length;
                                          info_ptr->iccp_profile = profile;
                                          png_ptr->read_buffer = NULL; 
                                          info_ptr->free_me |= PNG_FREE_ICCP;
                                          info_ptr->valid |= PNG_INFO_iCCP;
                                       }

                                       else
                                       {
                                          png_ptr->colorspace.flags |=
                                             PNG_COLORSPACE_INVALID;
                                          errmsg = "out of memory";
                                       }
                                    }

                                    




                                    if (info_ptr != NULL)
                                       png_colorspace_sync(png_ptr, info_ptr);

                                    if (errmsg == NULL)
                                    {
                                       png_ptr->zowner = 0;
                                       return;
                                    }
                                 }

                                 else if (size > 0)
                                    errmsg = "truncated";

#ifndef __COVERITY__
                                 else
                                    errmsg = png_ptr->zstream.msg;
#endif
                              }

                              
                           }

                           else 
                              errmsg = png_ptr->zstream.msg;
                        }

                        else
                           errmsg = "out of memory";
                     }

                     
                  }

                  
               }

               else 
                  errmsg = png_ptr->zstream.msg;

               
               png_ptr->zowner = 0;
            }

            else 
               errmsg = png_ptr->zstream.msg;
         }

         else
            errmsg = "bad compression method"; 
      }

      else
         errmsg = "bad keyword";
   }

   else
      errmsg = "too many profiles";

   
   if (finished == 0)
      png_crc_finish(png_ptr, length);

   png_ptr->colorspace.flags |= PNG_COLORSPACE_INVALID;
   png_colorspace_sync(png_ptr, info_ptr);
   if (errmsg != NULL) 
      png_chunk_benign_error(png_ptr, errmsg);
}
#endif 

#ifdef PNG_READ_sPLT_SUPPORTED
void 
png_handle_sPLT(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)

{
   png_bytep entry_start, buffer;
   png_sPLT_t new_palette;
   png_sPLT_entryp pp;
   png_uint_32 data_length;
   int entry_size, i;
   png_uint_32 skip = 0;
   png_uint_32 dl;
   png_size_t max_dl;

   png_debug(1, "in png_handle_sPLT");

#ifdef PNG_USER_LIMITS_SUPPORTED
   if (png_ptr->user_chunk_cache_max != 0)
   {
      if (png_ptr->user_chunk_cache_max == 1)
      {
         png_crc_finish(png_ptr, length);
         return;
      }

      if (--png_ptr->user_chunk_cache_max == 1)
      {
         png_warning(png_ptr, "No space in chunk cache for sPLT");
         png_crc_finish(png_ptr, length);
         return;
      }
   }
#endif

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

#ifdef PNG_MAX_MALLOC_64K
   if (length > 65535U)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "too large to fit in memory");
      return;
   }
#endif

   buffer = png_read_buffer(png_ptr, length+1, 2);
   if (buffer == NULL)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of memory");
      return;
   }


   



   png_crc_read(png_ptr, buffer, length);

   if (png_crc_finish(png_ptr, skip) != 0)
      return;

   buffer[length] = 0;

   for (entry_start = buffer; *entry_start; entry_start++)
       ;

   ++entry_start;

   
   if (entry_start > buffer + length - 2)
   {
      png_warning(png_ptr, "malformed sPLT chunk");
      return;
   }

   new_palette.depth = *entry_start++;
   entry_size = (new_palette.depth == 8 ? 6 : 10);
   


   data_length = length - (png_uint_32)(entry_start - buffer);

   
   if ((data_length % entry_size) != 0)
   {
      png_warning(png_ptr, "sPLT chunk has bad length");
      return;
   }

   dl = (png_int_32)(data_length / entry_size);
   max_dl = PNG_SIZE_MAX / (sizeof (png_sPLT_entry));

   if (dl > max_dl)
   {
       png_warning(png_ptr, "sPLT chunk too long");
       return;
   }

   new_palette.nentries = (png_int_32)(data_length / entry_size);

   new_palette.entries = (png_sPLT_entryp)png_malloc_warn(
       png_ptr, new_palette.nentries * (sizeof (png_sPLT_entry)));

   if (new_palette.entries == NULL)
   {
       png_warning(png_ptr, "sPLT chunk requires too much memory");
       return;
   }

#ifdef PNG_POINTER_INDEXING_SUPPORTED
   for (i = 0; i < new_palette.nentries; i++)
   {
      pp = new_palette.entries + i;

      if (new_palette.depth == 8)
      {
         pp->red = *entry_start++;
         pp->green = *entry_start++;
         pp->blue = *entry_start++;
         pp->alpha = *entry_start++;
      }

      else
      {
         pp->red   = png_get_uint_16(entry_start); entry_start += 2;
         pp->green = png_get_uint_16(entry_start); entry_start += 2;
         pp->blue  = png_get_uint_16(entry_start); entry_start += 2;
         pp->alpha = png_get_uint_16(entry_start); entry_start += 2;
      }

      pp->frequency = png_get_uint_16(entry_start); entry_start += 2;
   }
#else
   pp = new_palette.entries;

   for (i = 0; i < new_palette.nentries; i++)
   {

      if (new_palette.depth == 8)
      {
         pp[i].red   = *entry_start++;
         pp[i].green = *entry_start++;
         pp[i].blue  = *entry_start++;
         pp[i].alpha = *entry_start++;
      }

      else
      {
         pp[i].red   = png_get_uint_16(entry_start); entry_start += 2;
         pp[i].green = png_get_uint_16(entry_start); entry_start += 2;
         pp[i].blue  = png_get_uint_16(entry_start); entry_start += 2;
         pp[i].alpha = png_get_uint_16(entry_start); entry_start += 2;
      }

      pp[i].frequency = png_get_uint_16(entry_start); entry_start += 2;
   }
#endif

   
   new_palette.name = (png_charp)buffer;

   png_set_sPLT(png_ptr, info_ptr, &new_palette, 1);

   png_free(png_ptr, new_palette.entries);
}
#endif 

#ifdef PNG_READ_tRNS_SUPPORTED
void 
png_handle_tRNS(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_byte readbuf[PNG_MAX_PALETTE_LENGTH];

   png_debug(1, "in png_handle_tRNS");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_tRNS) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "duplicate");
      return;
   }

   if (png_ptr->color_type == PNG_COLOR_TYPE_GRAY)
   {
      png_byte buf[2];

      if (length != 2)
      {
         png_crc_finish(png_ptr, length);
         png_chunk_benign_error(png_ptr, "invalid");
         return;
      }

      png_crc_read(png_ptr, buf, 2);
      png_ptr->num_trans = 1;
      png_ptr->trans_color.gray = png_get_uint_16(buf);
   }

   else if (png_ptr->color_type == PNG_COLOR_TYPE_RGB)
   {
      png_byte buf[6];

      if (length != 6)
      {
         png_crc_finish(png_ptr, length);
         png_chunk_benign_error(png_ptr, "invalid");
         return;
      }

      png_crc_read(png_ptr, buf, length);
      png_ptr->num_trans = 1;
      png_ptr->trans_color.red = png_get_uint_16(buf);
      png_ptr->trans_color.green = png_get_uint_16(buf + 2);
      png_ptr->trans_color.blue = png_get_uint_16(buf + 4);
   }

   else if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if ((png_ptr->mode & PNG_HAVE_PLTE) == 0)
      {
         
         png_crc_finish(png_ptr, length);
         png_chunk_benign_error(png_ptr, "out of place");
         return;
      }

      if (length > png_ptr->num_palette || length > PNG_MAX_PALETTE_LENGTH ||
         length == 0)
      {
         png_crc_finish(png_ptr, length);
         png_chunk_benign_error(png_ptr, "invalid");
         return;
      }

      png_crc_read(png_ptr, readbuf, length);
      png_ptr->num_trans = (png_uint_16)length;
   }

   else
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "invalid with alpha channel");
      return;
   }

   if (png_crc_finish(png_ptr, 0) != 0)
   {
      png_ptr->num_trans = 0;
      return;
   }

   



   png_set_tRNS(png_ptr, info_ptr, readbuf, png_ptr->num_trans,
       &(png_ptr->trans_color));
}
#endif

#ifdef PNG_READ_bKGD_SUPPORTED
void 
png_handle_bKGD(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   unsigned int truelen;
   png_byte buf[6];
   png_color_16 background;

   png_debug(1, "in png_handle_bKGD");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0 ||
       (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
       (png_ptr->mode & PNG_HAVE_PLTE) == 0))
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_bKGD) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "duplicate");
      return;
   }

   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      truelen = 1;

   else if ((png_ptr->color_type & PNG_COLOR_MASK_COLOR) != 0)
      truelen = 6;

   else
      truelen = 2;

   if (length != truelen)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "invalid");
      return;
   }

   png_crc_read(png_ptr, buf, truelen);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   




   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      background.index = buf[0];

      if (info_ptr != NULL && info_ptr->num_palette != 0)
      {
         if (buf[0] >= info_ptr->num_palette)
         {
            png_chunk_benign_error(png_ptr, "invalid index");
            return;
         }

         background.red = (png_uint_16)png_ptr->palette[buf[0]].red;
         background.green = (png_uint_16)png_ptr->palette[buf[0]].green;
         background.blue = (png_uint_16)png_ptr->palette[buf[0]].blue;
      }

      else
         background.red = background.green = background.blue = 0;

      background.gray = 0;
   }

   else if ((png_ptr->color_type & PNG_COLOR_MASK_COLOR) == 0) 
   {
      background.index = 0;
      background.red =
      background.green =
      background.blue =
      background.gray = png_get_uint_16(buf);
   }

   else
   {
      background.index = 0;
      background.red = png_get_uint_16(buf);
      background.green = png_get_uint_16(buf + 2);
      background.blue = png_get_uint_16(buf + 4);
      background.gray = 0;
   }

   png_set_bKGD(png_ptr, info_ptr, &background);
}
#endif

#ifdef PNG_READ_hIST_SUPPORTED
void 
png_handle_hIST(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   unsigned int num, i;
   png_uint_16 readbuf[PNG_MAX_PALETTE_LENGTH];

   png_debug(1, "in png_handle_hIST");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0 ||
       (png_ptr->mode & PNG_HAVE_PLTE) == 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_hIST) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "duplicate");
      return;
   }

   num = length / 2 ;

   if (num != png_ptr->num_palette || num > PNG_MAX_PALETTE_LENGTH)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "invalid");
      return;
   }

   for (i = 0; i < num; i++)
   {
      png_byte buf[2];

      png_crc_read(png_ptr, buf, 2);
      readbuf[i] = png_get_uint_16(buf);
   }

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   png_set_hIST(png_ptr, info_ptr, readbuf);
}
#endif

#ifdef PNG_READ_pHYs_SUPPORTED
void 
png_handle_pHYs(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_byte buf[9];
   png_uint_32 res_x, res_y;
   int unit_type;

   png_debug(1, "in png_handle_pHYs");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_pHYs) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "duplicate");
      return;
   }

   if (length != 9)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "invalid");
      return;
   }

   png_crc_read(png_ptr, buf, 9);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   res_x = png_get_uint_32(buf);
   res_y = png_get_uint_32(buf + 4);
   unit_type = buf[8];
   png_set_pHYs(png_ptr, info_ptr, res_x, res_y, unit_type);
}
#endif

#ifdef PNG_READ_oFFs_SUPPORTED
void 
png_handle_oFFs(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_byte buf[9];
   png_int_32 offset_x, offset_y;
   int unit_type;

   png_debug(1, "in png_handle_oFFs");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_oFFs) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "duplicate");
      return;
   }

   if (length != 9)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "invalid");
      return;
   }

   png_crc_read(png_ptr, buf, 9);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   offset_x = png_get_int_32(buf);
   offset_y = png_get_int_32(buf + 4);
   unit_type = buf[8];
   png_set_oFFs(png_ptr, info_ptr, offset_x, offset_y, unit_type);
}
#endif

#ifdef PNG_READ_pCAL_SUPPORTED

void 
png_handle_pCAL(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_int_32 X0, X1;
   png_byte type, nparams;
   png_bytep buffer, buf, units, endptr;
   png_charpp params;
   int i;

   png_debug(1, "in png_handle_pCAL");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_pCAL) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "duplicate");
      return;
   }

   png_debug1(2, "Allocating and reading pCAL chunk data (%u bytes)",
       length + 1);

   buffer = png_read_buffer(png_ptr, length+1, 2);

   if (buffer == NULL)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of memory");
      return;
   }

   png_crc_read(png_ptr, buffer, length);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   buffer[length] = 0; 

   png_debug(3, "Finding end of pCAL purpose string");
   for (buf = buffer; *buf; buf++)
       ;

   endptr = buffer + length;

   


   if (endptr <= buf + 12)
   {
      png_chunk_benign_error(png_ptr, "invalid");
      return;
   }

   png_debug(3, "Reading pCAL X0, X1, type, nparams, and units");
   X0 = png_get_int_32((png_bytep)buf+1);
   X1 = png_get_int_32((png_bytep)buf+5);
   type = buf[9];
   nparams = buf[10];
   units = buf + 11;

   png_debug(3, "Checking pCAL equation type and number of parameters");
   


   if ((type == PNG_EQUATION_LINEAR && nparams != 2) ||
       (type == PNG_EQUATION_BASE_E && nparams != 3) ||
       (type == PNG_EQUATION_ARBITRARY && nparams != 3) ||
       (type == PNG_EQUATION_HYPERBOLIC && nparams != 4))
   {
      png_chunk_benign_error(png_ptr, "invalid parameter count");
      return;
   }

   else if (type >= PNG_EQUATION_LAST)
   {
      png_chunk_benign_error(png_ptr, "unrecognized equation type");
   }

   for (buf = units; *buf; buf++)
       ;

   png_debug(3, "Allocating pCAL parameters array");

   params = png_voidcast(png_charpp, png_malloc_warn(png_ptr,
       nparams * (sizeof (png_charp))));

   if (params == NULL)
   {
      png_chunk_benign_error(png_ptr, "out of memory");
      return;
   }

   
   for (i = 0; i < nparams; i++)
   {
      buf++; 

      png_debug1(3, "Reading pCAL parameter %d", i);

      for (params[i] = (png_charp)buf; buf <= endptr && *buf != 0; buf++)
          ;

      
      if (buf > endptr)
      {
         png_free(png_ptr, params);
         png_chunk_benign_error(png_ptr, "invalid data");
         return;
      }
   }

   png_set_pCAL(png_ptr, info_ptr, (png_charp)buffer, X0, X1, type, nparams,
      (png_charp)units, params);

   png_free(png_ptr, params);
}
#endif

#ifdef PNG_READ_sCAL_SUPPORTED

void 
png_handle_sCAL(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_bytep buffer;
   png_size_t i;
   int state;

   png_debug(1, "in png_handle_sCAL");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_sCAL) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "duplicate");
      return;
   }

   
   else if (length < 4)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "invalid");
      return;
   }

   png_debug1(2, "Allocating and reading sCAL chunk data (%u bytes)",
      length + 1);

   buffer = png_read_buffer(png_ptr, length+1, 2);

   if (buffer == NULL)
   {
      png_chunk_benign_error(png_ptr, "out of memory");
      png_crc_finish(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buffer, length);
   buffer[length] = 0; 

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   
   if (buffer[0] != 1 && buffer[0] != 2)
   {
      png_chunk_benign_error(png_ptr, "invalid unit");
      return;
   }

   


   i = 1;
   state = 0;

   if (png_check_fp_number((png_const_charp)buffer, length, &state, &i) == 0 ||
       i >= length || buffer[i++] != 0)
      png_chunk_benign_error(png_ptr, "bad width format");

   else if (PNG_FP_IS_POSITIVE(state) == 0)
      png_chunk_benign_error(png_ptr, "non-positive width");

   else
   {
      png_size_t heighti = i;

      state = 0;
      if (png_check_fp_number((png_const_charp)buffer, length,
          &state, &i) == 0 || i != length)
         png_chunk_benign_error(png_ptr, "bad height format");

      else if (PNG_FP_IS_POSITIVE(state) == 0)
         png_chunk_benign_error(png_ptr, "non-positive height");

      else
         
         png_set_sCAL_s(png_ptr, info_ptr, buffer[0],
            (png_charp)buffer+1, (png_charp)buffer+heighti);
   }
}
#endif

#ifdef PNG_READ_tIME_SUPPORTED
void 
png_handle_tIME(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_byte buf[7];
   png_time mod_time;

   png_debug(1, "in png_handle_tIME");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_tIME) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "duplicate");
      return;
   }

   if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
      png_ptr->mode |= PNG_AFTER_IDAT;

   if (length != 7)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "invalid");
      return;
   }

   png_crc_read(png_ptr, buf, 7);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   mod_time.second = buf[6];
   mod_time.minute = buf[5];
   mod_time.hour = buf[4];
   mod_time.day = buf[3];
   mod_time.month = buf[2];
   mod_time.year = png_get_uint_16(buf);

   png_set_tIME(png_ptr, info_ptr, &mod_time);
}
#endif

#ifdef PNG_READ_tEXt_SUPPORTED

void 
png_handle_tEXt(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_text  text_info;
   png_bytep buffer;
   png_charp key;
   png_charp text;
   png_uint_32 skip = 0;

   png_debug(1, "in png_handle_tEXt");

#ifdef PNG_USER_LIMITS_SUPPORTED
   if (png_ptr->user_chunk_cache_max != 0)
   {
      if (png_ptr->user_chunk_cache_max == 1)
      {
         png_crc_finish(png_ptr, length);
         return;
      }

      if (--png_ptr->user_chunk_cache_max == 1)
      {
         png_crc_finish(png_ptr, length);
         png_chunk_benign_error(png_ptr, "no space in chunk cache");
         return;
      }
   }
#endif

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
      png_ptr->mode |= PNG_AFTER_IDAT;

#ifdef PNG_MAX_MALLOC_64K
   if (length > 65535U)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "too large to fit in memory");
      return;
   }
#endif

   buffer = png_read_buffer(png_ptr, length+1, 1);

   if (buffer == NULL)
   {
     png_chunk_benign_error(png_ptr, "out of memory");
     return;
   }

   png_crc_read(png_ptr, buffer, length);

   if (png_crc_finish(png_ptr, skip) != 0)
      return;

   key = (png_charp)buffer;
   key[length] = 0;

   for (text = key; *text; text++)
       ;

   if (text != key + length)
      text++;

   text_info.compression = PNG_TEXT_COMPRESSION_NONE;
   text_info.key = key;
   text_info.lang = NULL;
   text_info.lang_key = NULL;
   text_info.itxt_length = 0;
   text_info.text = text;
   text_info.text_length = strlen(text);

   if (png_set_text_2(png_ptr, info_ptr, &text_info, 1) != 0)
      png_warning(png_ptr, "Insufficient memory to process text chunk");
}
#endif

#ifdef PNG_READ_zTXt_SUPPORTED

void 
png_handle_zTXt(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_const_charp errmsg = NULL;
   png_bytep       buffer;
   png_uint_32     keyword_length;

   png_debug(1, "in png_handle_zTXt");

#ifdef PNG_USER_LIMITS_SUPPORTED
   if (png_ptr->user_chunk_cache_max != 0)
   {
      if (png_ptr->user_chunk_cache_max == 1)
      {
         png_crc_finish(png_ptr, length);
         return;
      }

      if (--png_ptr->user_chunk_cache_max == 1)
      {
         png_crc_finish(png_ptr, length);
         png_chunk_benign_error(png_ptr, "no space in chunk cache");
         return;
      }
   }
#endif

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
      png_ptr->mode |= PNG_AFTER_IDAT;

   buffer = png_read_buffer(png_ptr, length, 2);

   if (buffer == NULL)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of memory");
      return;
   }

   png_crc_read(png_ptr, buffer, length);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   
   for (keyword_length = 0;
      keyword_length < length && buffer[keyword_length] != 0;
      ++keyword_length)
       ;

   if (keyword_length > 79 || keyword_length < 1)
      errmsg = "bad keyword";

   



   else if (keyword_length + 3 > length)
      errmsg = "truncated";

   else if (buffer[keyword_length+1] != PNG_COMPRESSION_TYPE_BASE)
      errmsg = "unknown compression type";

   else
   {
      png_alloc_size_t uncompressed_length = PNG_SIZE_MAX;

      



      if (png_decompress_chunk(png_ptr, length, keyword_length+2,
         &uncompressed_length, 1) == Z_STREAM_END)
      {
         png_text text;

         



         buffer = png_ptr->read_buffer;
         buffer[uncompressed_length+(keyword_length+2)] = 0;

         text.compression = PNG_TEXT_COMPRESSION_zTXt;
         text.key = (png_charp)buffer;
         text.text = (png_charp)(buffer + keyword_length+2);
         text.text_length = uncompressed_length;
         text.itxt_length = 0;
         text.lang = NULL;
         text.lang_key = NULL;

         if (png_set_text_2(png_ptr, info_ptr, &text, 1) != 0)
            errmsg = "insufficient memory";
      }

      else
         errmsg = png_ptr->zstream.msg;
   }

   if (errmsg != NULL)
      png_chunk_benign_error(png_ptr, errmsg);
}
#endif

#ifdef PNG_READ_iTXt_SUPPORTED

void 
png_handle_iTXt(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_const_charp errmsg = NULL;
   png_bytep buffer;
   png_uint_32 prefix_length;

   png_debug(1, "in png_handle_iTXt");

#ifdef PNG_USER_LIMITS_SUPPORTED
   if (png_ptr->user_chunk_cache_max != 0)
   {
      if (png_ptr->user_chunk_cache_max == 1)
      {
         png_crc_finish(png_ptr, length);
         return;
      }

      if (--png_ptr->user_chunk_cache_max == 1)
      {
         png_crc_finish(png_ptr, length);
         png_chunk_benign_error(png_ptr, "no space in chunk cache");
         return;
      }
   }
#endif

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
      png_ptr->mode |= PNG_AFTER_IDAT;

   buffer = png_read_buffer(png_ptr, length+1, 1);

   if (buffer == NULL)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of memory");
      return;
   }

   png_crc_read(png_ptr, buffer, length);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   
   for (prefix_length=0;
      prefix_length < length && buffer[prefix_length] != 0;
      ++prefix_length)
       ;

   
   if (prefix_length > 79 || prefix_length < 1)
      errmsg = "bad keyword";

   



   else if (prefix_length + 5 > length)
      errmsg = "truncated";

   else if (buffer[prefix_length+1] == 0 ||
      (buffer[prefix_length+1] == 1 &&
      buffer[prefix_length+2] == PNG_COMPRESSION_TYPE_BASE))
   {
      int compressed = buffer[prefix_length+1] != 0;
      png_uint_32 language_offset, translated_keyword_offset;
      png_alloc_size_t uncompressed_length = 0;

      
      prefix_length += 3;
      language_offset = prefix_length;

      for (; prefix_length < length && buffer[prefix_length] != 0;
         ++prefix_length)
          ;

      
      translated_keyword_offset = ++prefix_length;

      for (; prefix_length < length && buffer[prefix_length] != 0;
         ++prefix_length)
          ;

      




      ++prefix_length;

      if (compressed == 0 && prefix_length <= length)
         uncompressed_length = length - prefix_length;

      else if (compressed != 0 && prefix_length < length)
      {
         uncompressed_length = PNG_SIZE_MAX;

         



         if (png_decompress_chunk(png_ptr, length, prefix_length,
            &uncompressed_length, 1) == Z_STREAM_END)
            buffer = png_ptr->read_buffer;

         else
            errmsg = png_ptr->zstream.msg;
      }

      else
         errmsg = "truncated";

      if (errmsg == NULL)
      {
         png_text text;

         buffer[uncompressed_length+prefix_length] = 0;

         if (compressed == 0)
            text.compression = PNG_ITXT_COMPRESSION_NONE;

         else
            text.compression = PNG_ITXT_COMPRESSION_zTXt;

         text.key = (png_charp)buffer;
         text.lang = (png_charp)buffer + language_offset;
         text.lang_key = (png_charp)buffer + translated_keyword_offset;
         text.text = (png_charp)buffer + prefix_length;
         text.text_length = 0;
         text.itxt_length = uncompressed_length;

         if (png_set_text_2(png_ptr, info_ptr, &text, 1) != 0)
            errmsg = "insufficient memory";
      }
   }

   else
      errmsg = "bad compression info";

   if (errmsg != NULL)
      png_chunk_benign_error(png_ptr, errmsg);
}
#endif

#ifdef PNG_READ_APNG_SUPPORTED
void 
png_handle_acTL(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
    png_byte data[8];
    png_uint_32 num_frames;
    png_uint_32 num_plays;
    png_uint_32 didSet;

    png_debug(1, "in png_handle_acTL");

    if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
    {
        png_error(png_ptr, "Missing IHDR before acTL");
    }
    else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
    {
        png_warning(png_ptr, "Invalid acTL after IDAT skipped");
        png_crc_finish(png_ptr, length);
        return;
    }
    else if ((png_ptr->mode & PNG_HAVE_acTL) != 0)
    {
        png_warning(png_ptr, "Duplicate acTL skipped");
        png_crc_finish(png_ptr, length);
        return;
    }
    else if (length != 8)
    {
        png_warning(png_ptr, "acTL with invalid length skipped");
        png_crc_finish(png_ptr, length);
        return;
    }

    png_crc_read(png_ptr, data, 8);
    png_crc_finish(png_ptr, 0);

    num_frames = png_get_uint_31(png_ptr, data);
    num_plays = png_get_uint_31(png_ptr, data + 4);

    
    didSet = png_set_acTL(png_ptr, info_ptr, num_frames, num_plays);
    if (didSet != 0)
        png_ptr->mode |= PNG_HAVE_acTL;
}

void 
png_handle_fcTL(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
    png_byte data[22];
    png_uint_32 width;
    png_uint_32 height;
    png_uint_32 x_offset;
    png_uint_32 y_offset;
    png_uint_16 delay_num;
    png_uint_16 delay_den;
    png_byte dispose_op;
    png_byte blend_op;

    png_debug(1, "in png_handle_fcTL");

    png_ensure_sequence_number(png_ptr, length);

    if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
    {
        png_error(png_ptr, "Missing IHDR before fcTL");
    }
    else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
    {
        


        png_warning(png_ptr, "Invalid fcTL after IDAT skipped");
        png_crc_finish(png_ptr, length-4);
        return;
    }
    else if ((png_ptr->mode & PNG_HAVE_fcTL) != 0)
    {
        png_warning(png_ptr, "Duplicate fcTL within one frame skipped");
        png_crc_finish(png_ptr, length-4);
        return;
    }
    else if (length != 26)
    {
        png_warning(png_ptr, "fcTL with invalid length skipped");
        png_crc_finish(png_ptr, length-4);
        return;
    }

    png_crc_read(png_ptr, data, 22);
    png_crc_finish(png_ptr, 0);

    width = png_get_uint_31(png_ptr, data);
    height = png_get_uint_31(png_ptr, data + 4);
    x_offset = png_get_uint_31(png_ptr, data + 8);
    y_offset = png_get_uint_31(png_ptr, data + 12);
    delay_num = png_get_uint_16(data + 16);
    delay_den = png_get_uint_16(data + 18);
    dispose_op = data[20];
    blend_op = data[21];

    if (png_ptr->num_frames_read == 0 && (x_offset != 0 || y_offset != 0))
    {
        png_warning(png_ptr, "fcTL for the first frame must have zero offset");
        return;
    }

    if (info_ptr != NULL)
    {
        if (png_ptr->num_frames_read == 0 &&
            (width != info_ptr->width || height != info_ptr->height))
        {
            png_warning(png_ptr, "size in first frame's fcTL must match "
                               "the size in IHDR");
            return;
        }

        
        png_set_next_frame_fcTL(png_ptr, info_ptr, width, height,
                                x_offset, y_offset, delay_num, delay_den,
                                dispose_op, blend_op);

        png_read_reinit(png_ptr, info_ptr);

        png_ptr->mode |= PNG_HAVE_fcTL;
    }
}

void 
png_have_info(png_structp png_ptr, png_infop info_ptr)
{
    if ((info_ptr->valid & PNG_INFO_acTL) != 0 &&
        (info_ptr->valid & PNG_INFO_fcTL) == 0)
    {
        png_ptr->apng_flags |= PNG_FIRST_FRAME_HIDDEN;
        info_ptr->num_frames++;
    }
}

void 
png_handle_fdAT(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
    png_ensure_sequence_number(png_ptr, length);

    




    png_warning(png_ptr, "ignoring fdAT chunk");
    png_crc_finish(png_ptr, length - 4);
    PNG_UNUSED(info_ptr)
}

void 
png_ensure_sequence_number(png_structp png_ptr, png_uint_32 length)
{
    png_byte data[4];
    png_uint_32 sequence_number;

    if (length < 4)
        png_error(png_ptr, "invalid fcTL or fdAT chunk found");

    png_crc_read(png_ptr, data, 4);
    sequence_number = png_get_uint_31(png_ptr, data);

    if (sequence_number != png_ptr->next_seq_num)
        png_error(png_ptr, "fcTL or fdAT chunk with out-of-order sequence "
                           "number found");

    png_ptr->next_seq_num++;
}
#endif 

#ifdef PNG_READ_UNKNOWN_CHUNKS_SUPPORTED

static int
png_cache_unknown_chunk(png_structrp png_ptr, png_uint_32 length)
{
   png_alloc_size_t limit = PNG_SIZE_MAX;

   if (png_ptr->unknown_chunk.data != NULL)
   {
      png_free(png_ptr, png_ptr->unknown_chunk.data);
      png_ptr->unknown_chunk.data = NULL;
   }

#  ifdef PNG_SET_CHUNK_MALLOC_LIMIT_SUPPORTED
      if (png_ptr->user_chunk_malloc_max > 0 &&
          png_ptr->user_chunk_malloc_max < limit)
         limit = png_ptr->user_chunk_malloc_max;

#  elif PNG_USER_CHUNK_MALLOC_MAX > 0
      if (PNG_USER_CHUNK_MALLOC_MAX < limit)
         limit = PNG_USER_CHUNK_MALLOC_MAX;
#  endif

   if (length <= limit)
   {
      PNG_CSTRING_FROM_CHUNK(png_ptr->unknown_chunk.name, png_ptr->chunk_name);
      
      png_ptr->unknown_chunk.size = (png_size_t)length;
      
      png_ptr->unknown_chunk.location = (png_byte)png_ptr->mode;

      if (length == 0)
         png_ptr->unknown_chunk.data = NULL;

      else
      {
         
         png_ptr->unknown_chunk.data = png_voidcast(png_bytep,
            png_malloc_warn(png_ptr, length));
      }
   }

   if (png_ptr->unknown_chunk.data == NULL && length > 0)
   {
      
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "unknown chunk exceeds memory limits");
      return 0;
   }

   else
   {
      if (length > 0)
         png_crc_read(png_ptr, png_ptr->unknown_chunk.data, length);
      png_crc_finish(png_ptr, 0);
      return 1;
   }
}
#endif 


void 
png_handle_unknown(png_structrp png_ptr, png_inforp info_ptr,
   png_uint_32 length, int keep)
{
   int handled = 0; 

   png_debug(1, "in png_handle_unknown");

#ifdef PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
   










#  ifndef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#     ifdef PNG_SET_UNKNOWN_CHUNKS_SUPPORTED
         keep = png_chunk_unknown_handling(png_ptr, png_ptr->chunk_name);
#     endif
#  endif

   



#  ifdef PNG_READ_USER_CHUNKS_SUPPORTED
      


      if (png_ptr->read_user_chunk_fn != NULL)
      {
         if (png_cache_unknown_chunk(png_ptr, length) != 0)
         {
            
            int ret = (*(png_ptr->read_user_chunk_fn))(png_ptr,
               &png_ptr->unknown_chunk);

            








            if (ret < 0)
               png_chunk_error(png_ptr, "error in user chunk");

            else if (ret == 0)
            {
               










               if (keep < PNG_HANDLE_CHUNK_IF_SAFE)
               {
#                 ifdef PNG_SET_UNKNOWN_CHUNKS_SUPPORTED
                     if (png_ptr->unknown_default < PNG_HANDLE_CHUNK_IF_SAFE)
                     {
                        png_chunk_warning(png_ptr, "Saving unknown chunk:");
                        png_app_warning(png_ptr,
                           "forcing save of an unhandled chunk;"
                           " please call png_set_keep_unknown_chunks");
                           
                     }
#                 endif
                  keep = PNG_HANDLE_CHUNK_IF_SAFE;
               }
            }

            else 
            {
               handled = 1;
               
               keep = PNG_HANDLE_CHUNK_NEVER;
            }
         }

         else
            keep = PNG_HANDLE_CHUNK_NEVER; 
      }

      else
         
#  endif 

#  ifdef PNG_SAVE_UNKNOWN_CHUNKS_SUPPORTED
      {
         




         if (keep == PNG_HANDLE_CHUNK_AS_DEFAULT)
            keep = png_ptr->unknown_default;

         if (keep == PNG_HANDLE_CHUNK_ALWAYS ||
            (keep == PNG_HANDLE_CHUNK_IF_SAFE &&
             PNG_CHUNK_ANCILLARY(png_ptr->chunk_name)))
         {
            if (png_cache_unknown_chunk(png_ptr, length) == 0)
               keep = PNG_HANDLE_CHUNK_NEVER;
         }

         else
            png_crc_finish(png_ptr, length);
      }
#  else
#     ifndef PNG_READ_USER_CHUNKS_SUPPORTED
#        error no method to support READ_UNKNOWN_CHUNKS
#     endif

      {
         





         if (keep > PNG_HANDLE_CHUNK_NEVER)
            png_app_error(png_ptr, "no unknown chunk support available");

         png_crc_finish(png_ptr, length);
      }
#  endif

#  ifdef PNG_STORE_UNKNOWN_CHUNKS_SUPPORTED
      


      if (keep == PNG_HANDLE_CHUNK_ALWAYS ||
         (keep == PNG_HANDLE_CHUNK_IF_SAFE &&
          PNG_CHUNK_ANCILLARY(png_ptr->chunk_name)))
      {
#     ifdef PNG_USER_LIMITS_SUPPORTED
         switch (png_ptr->user_chunk_cache_max)
         {
            case 2:
               png_ptr->user_chunk_cache_max = 1;
               png_chunk_benign_error(png_ptr, "no space in chunk cache");
               
            case 1:
               


               break;

            default: 
               --(png_ptr->user_chunk_cache_max);
               
            case 0: 
#     endif 
               


               png_set_unknown_chunks(png_ptr, info_ptr,
                  &png_ptr->unknown_chunk, 1);
               handled = 1;
#     ifdef PNG_USER_LIMITS_SUPPORTED
               break;
         }
#     endif
      }
#  else 
      PNG_UNUSED(info_ptr)
#  endif

   



   if (png_ptr->unknown_chunk.data != NULL)
      png_free(png_ptr, png_ptr->unknown_chunk.data);
   png_ptr->unknown_chunk.data = NULL;

#else 
   
   png_crc_finish(png_ptr, length);
   PNG_UNUSED(info_ptr)
   PNG_UNUSED(keep)
#endif 

   
   if (handled == 0 && PNG_CHUNK_CRITICAL(png_ptr->chunk_name))
      png_chunk_error(png_ptr, "unhandled critical chunk");
}













void 
png_check_chunk_name(png_structrp png_ptr, png_uint_32 chunk_name)
{
   int i;

   png_debug(1, "in png_check_chunk_name");

   for (i=1; i<=4; ++i)
   {
      int c = chunk_name & 0xff;

      if (c < 65 || c > 122 || (c > 90 && c < 97))
         png_chunk_error(png_ptr, "invalid chunk type");

      chunk_name >>= 8;
   }
}








void 
png_combine_row(png_const_structrp png_ptr, png_bytep dp, int display)
{
   unsigned int pixel_depth = png_ptr->transformed_pixel_depth;
   png_const_bytep sp = png_ptr->row_buf + 1;
   png_alloc_size_t row_width = png_ptr->width;
   unsigned int pass = png_ptr->pass;
   png_bytep end_ptr = 0;
   png_byte end_byte = 0;
   unsigned int end_mask;

   png_debug(1, "in png_combine_row");

   


   if (pixel_depth == 0)
      png_error(png_ptr, "internal row logic error");

   



   if (png_ptr->info_rowbytes != 0 && png_ptr->info_rowbytes !=
          PNG_ROWBYTES(pixel_depth, row_width))
      png_error(png_ptr, "internal row size calculation error");

   
   if (row_width == 0)
      png_error(png_ptr, "internal row width error");

   



   end_mask = (pixel_depth * row_width) & 7;
   if (end_mask != 0)
   {
      
      end_ptr = dp + PNG_ROWBYTES(pixel_depth, row_width) - 1;
      end_byte = *end_ptr;
#     ifdef PNG_READ_PACKSWAP_SUPPORTED
         if ((png_ptr->transformations & PNG_PACKSWAP) != 0)
            
            end_mask = 0xff << end_mask;

         else 
#     endif
         end_mask = 0xff >> end_mask;
      
   }

   





#ifdef PNG_READ_INTERLACING_SUPPORTED
   if (png_ptr->interlaced != 0 &&
       (png_ptr->transformations & PNG_INTERLACE) != 0 &&
       pass < 6 && (display == 0 ||
       
       (display == 1 && (pass & 1) != 0)))
   {
      


      if (row_width <= PNG_PASS_START_COL(pass))
         return;

      if (pixel_depth < 8)
      {
         




































#        if PNG_USE_COMPILE_TIME_MASKS
#           define PNG_LSR(x,s) ((x)>>((s) & 0x1f))
#           define PNG_LSL(x,s) ((x)<<((s) & 0x1f))
#        else
#           define PNG_LSR(x,s) ((x)>>(s))
#           define PNG_LSL(x,s) ((x)<<(s))
#        endif
#        define S_COPY(p,x) (((p)<4 ? PNG_LSR(0x80088822,(3-(p))*8+(7-(x))) :\
           PNG_LSR(0xaa55ff00,(7-(p))*8+(7-(x)))) & 1)
#        define B_COPY(p,x) (((p)<4 ? PNG_LSR(0xff0fff33,(3-(p))*8+(7-(x))) :\
           PNG_LSR(0xff55ff00,(7-(p))*8+(7-(x)))) & 1)

         





#        define PIXEL_MASK(p,x,d,s) \
            (PNG_LSL(((PNG_LSL(1U,(d)))-1),(((x)*(d))^((s)?8-(d):0))))

         

#        define S_MASKx(p,x,d,s) (S_COPY(p,x)?PIXEL_MASK(p,x,d,s):0)
#        define B_MASKx(p,x,d,s) (B_COPY(p,x)?PIXEL_MASK(p,x,d,s):0)

         



#        define MASK_EXPAND(m,d) ((m)*((d)==1?0x01010101:((d)==2?0x00010001:1)))

#        define S_MASK(p,d,s) MASK_EXPAND(S_MASKx(p,0,d,s) + S_MASKx(p,1,d,s) +\
            S_MASKx(p,2,d,s) + S_MASKx(p,3,d,s) + S_MASKx(p,4,d,s) +\
            S_MASKx(p,5,d,s) + S_MASKx(p,6,d,s) + S_MASKx(p,7,d,s), d)

#        define B_MASK(p,d,s) MASK_EXPAND(B_MASKx(p,0,d,s) + B_MASKx(p,1,d,s) +\
            B_MASKx(p,2,d,s) + B_MASKx(p,3,d,s) + B_MASKx(p,4,d,s) +\
            B_MASKx(p,5,d,s) + B_MASKx(p,6,d,s) + B_MASKx(p,7,d,s), d)

#if PNG_USE_COMPILE_TIME_MASKS
         




#        define S_MASKS(d,s) { S_MASK(0,d,s), S_MASK(1,d,s), S_MASK(2,d,s),\
            S_MASK(3,d,s), S_MASK(4,d,s), S_MASK(5,d,s) }

#        define B_MASKS(d,s) { B_MASK(1,d,s), B_MASK(3,d,s), B_MASK(5,d,s) }

#        define DEPTH_INDEX(d) ((d)==1?0:((d)==2?1:2))

         


         static PNG_CONST png_uint_32 row_mask[2][3][6] =
         {
            
            { S_MASKS(1,0), S_MASKS(2,0), S_MASKS(4,0) },
            
            { S_MASKS(1,1), S_MASKS(2,1), S_MASKS(4,1) }
         };

         


         static PNG_CONST png_uint_32 display_mask[2][3][3] =
         {
            
            { B_MASKS(1,0), B_MASKS(2,0), B_MASKS(4,0) },
            
            { B_MASKS(1,1), B_MASKS(2,1), B_MASKS(4,1) }
         };

#        define MASK(pass,depth,display,png)\
            ((display)?display_mask[png][DEPTH_INDEX(depth)][pass>>1]:\
               row_mask[png][DEPTH_INDEX(depth)][pass])

#else 
         


#        define MASK(pass,depth,display,png)\
            ((display)?B_MASK(pass,depth,png):S_MASK(pass,depth,png))
#endif 

         




         png_uint_32 pixels_per_byte = 8 / pixel_depth;
         png_uint_32 mask;

#        ifdef PNG_READ_PACKSWAP_SUPPORTED
            if ((png_ptr->transformations & PNG_PACKSWAP) != 0)
               mask = MASK(pass, pixel_depth, display, 0);

            else
#        endif
            mask = MASK(pass, pixel_depth, display, 1);

         for (;;)
         {
            png_uint_32 m;

            



            m = mask;
            mask = (m >> 8) | (m << 24); 
            m &= 0xff;

            if (m != 0) 
            {
               if (m != 0xff)
                  *dp = (png_byte)((*dp & ~m) | (*sp & m));
               else
                  *dp = *sp;
            }

            



            if (row_width <= pixels_per_byte)
               break; 

            row_width -= pixels_per_byte;
            ++dp;
            ++sp;
         }
      }

      else 
      {
         unsigned int bytes_to_copy, bytes_to_jump;

         
         if (pixel_depth & 7)
            png_error(png_ptr, "invalid user transform pixel depth");

         pixel_depth >>= 3; 
         row_width *= pixel_depth;

         



         {
            unsigned int offset = PNG_PASS_START_COL(pass) * pixel_depth;

            row_width -= offset;
            dp += offset;
            sp += offset;
         }

         
         if (display != 0)
         {
            



            bytes_to_copy = (1<<((6-pass)>>1)) * pixel_depth;

            
            if (bytes_to_copy > row_width)
               bytes_to_copy = (unsigned int)row_width;
         }

         else 
            bytes_to_copy = pixel_depth;

         
         bytes_to_jump = PNG_PASS_COL_OFFSET(pass) * pixel_depth;

         







         switch (bytes_to_copy)
         {
            case 1:
               for (;;)
               {
                  *dp = *sp;

                  if (row_width <= bytes_to_jump)
                     return;

                  dp += bytes_to_jump;
                  sp += bytes_to_jump;
                  row_width -= bytes_to_jump;
               }

            case 2:
               


               do
               {
                  dp[0] = sp[0], dp[1] = sp[1];

                  if (row_width <= bytes_to_jump)
                     return;

                  sp += bytes_to_jump;
                  dp += bytes_to_jump;
                  row_width -= bytes_to_jump;
               }
               while (row_width > 1);

               
               *dp = *sp;
               return;

            case 3:
               


               for (;;)
               {
                  dp[0] = sp[0], dp[1] = sp[1], dp[2] = sp[2];

                  if (row_width <= bytes_to_jump)
                     return;

                  sp += bytes_to_jump;
                  dp += bytes_to_jump;
                  row_width -= bytes_to_jump;
               }

            default:
#if PNG_ALIGN_TYPE != PNG_ALIGN_NONE
               




               if (bytes_to_copy < 16  &&
                   png_isaligned(dp, png_uint_16) &&
                   png_isaligned(sp, png_uint_16) &&
                   bytes_to_copy % (sizeof (png_uint_16)) == 0 &&
                   bytes_to_jump % (sizeof (png_uint_16)) == 0)
               {
                  


                  if (png_isaligned(dp, png_uint_32) != 0 &&
                      png_isaligned(sp, png_uint_32) != 0 &&
                      bytes_to_copy % (sizeof (png_uint_32)) == 0 &&
                      bytes_to_jump % (sizeof (png_uint_32)) == 0)
                  {
                     png_uint_32p dp32 = png_aligncast(png_uint_32p,dp);
                     png_const_uint_32p sp32 = png_aligncastconst(
                         png_const_uint_32p, sp);
                     size_t skip = (bytes_to_jump-bytes_to_copy) /
                         (sizeof (png_uint_32));

                     do
                     {
                        size_t c = bytes_to_copy;
                        do
                        {
                           *dp32++ = *sp32++;
                           c -= (sizeof (png_uint_32));
                        }
                        while (c > 0);

                        if (row_width <= bytes_to_jump)
                           return;

                        dp32 += skip;
                        sp32 += skip;
                        row_width -= bytes_to_jump;
                     }
                     while (bytes_to_copy <= row_width);

                     



                     dp = (png_bytep)dp32;
                     sp = (png_const_bytep)sp32;
                     do
                        *dp++ = *sp++;
                     while (--row_width > 0);
                     return;
                  }

                  


                  else
                  {
                     png_uint_16p dp16 = png_aligncast(png_uint_16p, dp);
                     png_const_uint_16p sp16 = png_aligncastconst(
                        png_const_uint_16p, sp);
                     size_t skip = (bytes_to_jump-bytes_to_copy) /
                        (sizeof (png_uint_16));

                     do
                     {
                        size_t c = bytes_to_copy;
                        do
                        {
                           *dp16++ = *sp16++;
                           c -= (sizeof (png_uint_16));
                        }
                        while (c > 0);

                        if (row_width <= bytes_to_jump)
                           return;

                        dp16 += skip;
                        sp16 += skip;
                        row_width -= bytes_to_jump;
                     }
                     while (bytes_to_copy <= row_width);

                     
                     dp = (png_bytep)dp16;
                     sp = (png_const_bytep)sp16;
                     do
                        *dp++ = *sp++;
                     while (--row_width > 0);
                     return;
                  }
               }
#endif 

               
               for (;;)
               {
                  memcpy(dp, sp, bytes_to_copy);

                  if (row_width <= bytes_to_jump)
                     return;

                  sp += bytes_to_jump;
                  dp += bytes_to_jump;
                  row_width -= bytes_to_jump;
                  if (bytes_to_copy > row_width)
                     bytes_to_copy = (unsigned int)row_width;
               }
         }

         
      } 

      
   }
   else
#endif 

   



   memcpy(dp, sp, PNG_ROWBYTES(pixel_depth, row_width));

   
   if (end_ptr != NULL)
      *end_ptr = (png_byte)((end_byte & end_mask) | (*end_ptr & ~end_mask));
}

#ifdef PNG_READ_INTERLACING_SUPPORTED
void 
png_do_read_interlace(png_row_infop row_info, png_bytep row, int pass,
   png_uint_32 transformations )
{
   
   
   static PNG_CONST int png_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};

   png_debug(1, "in png_do_read_interlace");
   if (row != NULL && row_info != NULL)
   {
      png_uint_32 final_width;

      final_width = row_info->width * png_pass_inc[pass];

      switch (row_info->pixel_depth)
      {
         case 1:
         {
            png_bytep sp = row + (png_size_t)((row_info->width - 1) >> 3);
            png_bytep dp = row + (png_size_t)((final_width - 1) >> 3);
            int sshift, dshift;
            int s_start, s_end, s_inc;
            int jstop = png_pass_inc[pass];
            png_byte v;
            png_uint_32 i;
            int j;

#ifdef PNG_READ_PACKSWAP_SUPPORTED
            if ((transformations & PNG_PACKSWAP) != 0)
            {
                sshift = (int)((row_info->width + 7) & 0x07);
                dshift = (int)((final_width + 7) & 0x07);
                s_start = 7;
                s_end = 0;
                s_inc = -1;
            }

            else
#endif
            {
                sshift = 7 - (int)((row_info->width + 7) & 0x07);
                dshift = 7 - (int)((final_width + 7) & 0x07);
                s_start = 0;
                s_end = 7;
                s_inc = 1;
            }

            for (i = 0; i < row_info->width; i++)
            {
               v = (png_byte)((*sp >> sshift) & 0x01);
               for (j = 0; j < jstop; j++)
               {
                  unsigned int tmp = *dp & (0x7f7f >> (7 - dshift));
                  tmp |= v << dshift;
                  *dp = (png_byte)(tmp & 0xff);

                  if (dshift == s_end)
                  {
                     dshift = s_start;
                     dp--;
                  }

                  else
                     dshift += s_inc;
               }

               if (sshift == s_end)
               {
                  sshift = s_start;
                  sp--;
               }

               else
                  sshift += s_inc;
            }
            break;
         }

         case 2:
         {
            png_bytep sp = row + (png_uint_32)((row_info->width - 1) >> 2);
            png_bytep dp = row + (png_uint_32)((final_width - 1) >> 2);
            int sshift, dshift;
            int s_start, s_end, s_inc;
            int jstop = png_pass_inc[pass];
            png_uint_32 i;

#ifdef PNG_READ_PACKSWAP_SUPPORTED
            if ((transformations & PNG_PACKSWAP) != 0)
            {
               sshift = (int)(((row_info->width + 3) & 0x03) << 1);
               dshift = (int)(((final_width + 3) & 0x03) << 1);
               s_start = 6;
               s_end = 0;
               s_inc = -2;
            }

            else
#endif
            {
               sshift = (int)((3 - ((row_info->width + 3) & 0x03)) << 1);
               dshift = (int)((3 - ((final_width + 3) & 0x03)) << 1);
               s_start = 0;
               s_end = 6;
               s_inc = 2;
            }

            for (i = 0; i < row_info->width; i++)
            {
               png_byte v;
               int j;

               v = (png_byte)((*sp >> sshift) & 0x03);
               for (j = 0; j < jstop; j++)
               {
                  unsigned int tmp = *dp & (0x3f3f >> (6 - dshift));
                  tmp |= v << dshift;
                  *dp = (png_byte)(tmp & 0xff);

                  if (dshift == s_end)
                  {
                     dshift = s_start;
                     dp--;
                  }

                  else
                     dshift += s_inc;
               }

               if (sshift == s_end)
               {
                  sshift = s_start;
                  sp--;
               }

               else
                  sshift += s_inc;
            }
            break;
         }

         case 4:
         {
            png_bytep sp = row + (png_size_t)((row_info->width - 1) >> 1);
            png_bytep dp = row + (png_size_t)((final_width - 1) >> 1);
            int sshift, dshift;
            int s_start, s_end, s_inc;
            png_uint_32 i;
            int jstop = png_pass_inc[pass];

#ifdef PNG_READ_PACKSWAP_SUPPORTED
            if ((transformations & PNG_PACKSWAP) != 0)
            {
               sshift = (int)(((row_info->width + 1) & 0x01) << 2);
               dshift = (int)(((final_width + 1) & 0x01) << 2);
               s_start = 4;
               s_end = 0;
               s_inc = -4;
            }

            else
#endif
            {
               sshift = (int)((1 - ((row_info->width + 1) & 0x01)) << 2);
               dshift = (int)((1 - ((final_width + 1) & 0x01)) << 2);
               s_start = 0;
               s_end = 4;
               s_inc = 4;
            }

            for (i = 0; i < row_info->width; i++)
            {
               png_byte v = (png_byte)((*sp >> sshift) & 0x0f);
               int j;

               for (j = 0; j < jstop; j++)
               {
                  unsigned int tmp = *dp & (0xf0f >> (4 - dshift));
                  tmp |= v << dshift;
                  *dp = (png_byte)(tmp & 0xff);

                  if (dshift == s_end)
                  {
                     dshift = s_start;
                     dp--;
                  }

                  else
                     dshift += s_inc;
               }

               if (sshift == s_end)
               {
                  sshift = s_start;
                  sp--;
               }

               else
                  sshift += s_inc;
            }
            break;
         }

         default:
         {
            png_size_t pixel_bytes = (row_info->pixel_depth >> 3);

            png_bytep sp = row + (png_size_t)(row_info->width - 1)
                * pixel_bytes;

            png_bytep dp = row + (png_size_t)(final_width - 1) * pixel_bytes;

            int jstop = png_pass_inc[pass];
            png_uint_32 i;

            for (i = 0; i < row_info->width; i++)
            {
               png_byte v[8]; 
               int j;

               memcpy(v, sp, pixel_bytes);

               for (j = 0; j < jstop; j++)
               {
                  memcpy(dp, v, pixel_bytes);
                  dp -= pixel_bytes;
               }

               sp -= pixel_bytes;
            }
            break;
         }
      }

      row_info->width = final_width;
      row_info->rowbytes = PNG_ROWBYTES(row_info->pixel_depth, final_width);
   }
#ifndef PNG_READ_PACKSWAP_SUPPORTED
   PNG_UNUSED(transformations)  
#endif
}
#endif 

static void
png_read_filter_row_sub(png_row_infop row_info, png_bytep row,
   png_const_bytep prev_row)
{
   png_size_t i;
   png_size_t istop = row_info->rowbytes;
   unsigned int bpp = (row_info->pixel_depth + 7) >> 3;
   png_bytep rp = row + bpp;

   PNG_UNUSED(prev_row)

   for (i = bpp; i < istop; i++)
   {
      *rp = (png_byte)(((int)(*rp) + (int)(*(rp-bpp))) & 0xff);
      rp++;
   }
}

static void
png_read_filter_row_up(png_row_infop row_info, png_bytep row,
   png_const_bytep prev_row)
{
   png_size_t i;
   png_size_t istop = row_info->rowbytes;
   png_bytep rp = row;
   png_const_bytep pp = prev_row;

   for (i = 0; i < istop; i++)
   {
      *rp = (png_byte)(((int)(*rp) + (int)(*pp++)) & 0xff);
      rp++;
   }
}

static void
png_read_filter_row_avg(png_row_infop row_info, png_bytep row,
   png_const_bytep prev_row)
{
   png_size_t i;
   png_bytep rp = row;
   png_const_bytep pp = prev_row;
   unsigned int bpp = (row_info->pixel_depth + 7) >> 3;
   png_size_t istop = row_info->rowbytes - bpp;

   for (i = 0; i < bpp; i++)
   {
      *rp = (png_byte)(((int)(*rp) +
         ((int)(*pp++) / 2 )) & 0xff);

      rp++;
   }

   for (i = 0; i < istop; i++)
   {
      *rp = (png_byte)(((int)(*rp) +
         (int)(*pp++ + *(rp-bpp)) / 2 ) & 0xff);

      rp++;
   }
}

static void
png_read_filter_row_paeth_1byte_pixel(png_row_infop row_info, png_bytep row,
   png_const_bytep prev_row)
{
   png_bytep rp_end = row + row_info->rowbytes;
   int a, c;

   
   c = *prev_row++;
   a = *row + c;
   *row++ = (png_byte)a;

   
   while (row < rp_end)
   {
      int b, pa, pb, pc, p;

      a &= 0xff; 
      b = *prev_row++;

      p = b - c;
      pc = a - c;

#     ifdef PNG_USE_ABS
         pa = abs(p);
         pb = abs(pc);
         pc = abs(p + pc);
#     else
         pa = p < 0 ? -p : p;
         pb = pc < 0 ? -pc : pc;
         pc = (p + pc) < 0 ? -(p + pc) : p + pc;
#     endif

      


      if (pb < pa) pa = pb, a = b;
      if (pc < pa) a = c;

      


      c = b;
      a += *row;
      *row++ = (png_byte)a;
   }
}

static void
png_read_filter_row_paeth_multibyte_pixel(png_row_infop row_info, png_bytep row,
   png_const_bytep prev_row)
{
   int bpp = (row_info->pixel_depth + 7) >> 3;
   png_bytep rp_end = row + bpp;

   


   while (row < rp_end)
   {
      int a = *row + *prev_row++;
      *row++ = (png_byte)a;
   }

   
   rp_end += row_info->rowbytes - bpp;

   while (row < rp_end)
   {
      int a, b, c, pa, pb, pc, p;

      c = *(prev_row - bpp);
      a = *(row - bpp);
      b = *prev_row++;

      p = b - c;
      pc = a - c;

#     ifdef PNG_USE_ABS
         pa = abs(p);
         pb = abs(pc);
         pc = abs(p + pc);
#     else
         pa = p < 0 ? -p : p;
         pb = pc < 0 ? -pc : pc;
         pc = (p + pc) < 0 ? -(p + pc) : p + pc;
#     endif

      if (pb < pa) pa = pb, a = b;
      if (pc < pa) a = c;

      a += *row;
      *row++ = (png_byte)a;
   }
}

static void
png_init_filter_functions(png_structrp pp)
   








{
   unsigned int bpp = (pp->pixel_depth + 7) >> 3;

   pp->read_filter[PNG_FILTER_VALUE_SUB-1] = png_read_filter_row_sub;
   pp->read_filter[PNG_FILTER_VALUE_UP-1] = png_read_filter_row_up;
   pp->read_filter[PNG_FILTER_VALUE_AVG-1] = png_read_filter_row_avg;
   if (bpp == 1)
      pp->read_filter[PNG_FILTER_VALUE_PAETH-1] =
         png_read_filter_row_paeth_1byte_pixel;
   else
      pp->read_filter[PNG_FILTER_VALUE_PAETH-1] =
         png_read_filter_row_paeth_multibyte_pixel;

#ifdef PNG_FILTER_OPTIMIZATIONS
   







   PNG_FILTER_OPTIMIZATIONS(pp, bpp);
#endif
}

void 
png_read_filter_row(png_structrp pp, png_row_infop row_info, png_bytep row,
   png_const_bytep prev_row, int filter)
{
   



   if (filter > PNG_FILTER_VALUE_NONE && filter < PNG_FILTER_VALUE_LAST)
   {
      if (pp->read_filter[0] == NULL)
         png_init_filter_functions(pp);

      pp->read_filter[filter-1](row_info, row, prev_row);
   }
}

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED
void 
png_read_IDAT_data(png_structrp png_ptr, png_bytep output,
   png_alloc_size_t avail_out)
{
   
   png_ptr->zstream.next_out = output;
   png_ptr->zstream.avail_out = 0; 

   if (output == NULL)
      avail_out = 0;

   do
   {
      int ret;
      png_byte tmpbuf[PNG_INFLATE_BUF_SIZE];

      if (png_ptr->zstream.avail_in == 0)
      {
         uInt avail_in;
         png_bytep buffer;

#ifdef PNG_READ_APNG_SUPPORTED
         png_uint_32 bytes_to_skip = 0;

         while (png_ptr->idat_size == 0 || bytes_to_skip != 0)
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
         while (png_ptr->idat_size == 0)
         {
            png_crc_finish(png_ptr, 0);

            png_ptr->idat_size = png_read_chunk_header(png_ptr);
            


            if (png_ptr->chunk_name != png_IDAT)
               png_error(png_ptr, "Not enough image data");
         }
#endif 

         avail_in = png_ptr->IDAT_read_size;

         if (avail_in > png_ptr->idat_size)
            avail_in = (uInt)png_ptr->idat_size;

         




         buffer = png_read_buffer(png_ptr, avail_in, 0);

         png_crc_read(png_ptr, buffer, avail_in);
         png_ptr->idat_size -= avail_in;

         png_ptr->zstream.next_in = buffer;
         png_ptr->zstream.avail_in = avail_in;
      }

      
      if (output != NULL) 
      {
         uInt out = ZLIB_IO_MAX;

         if (out > avail_out)
            out = (uInt)avail_out;

         avail_out -= out;
         png_ptr->zstream.avail_out = out;
      }

      else 
      {
         png_ptr->zstream.next_out = tmpbuf;
         png_ptr->zstream.avail_out = (sizeof tmpbuf);
      }

      






      ret = inflate(&png_ptr->zstream, Z_NO_FLUSH);

      
      if (output != NULL)
         avail_out += png_ptr->zstream.avail_out;

      else 
         avail_out += (sizeof tmpbuf) - png_ptr->zstream.avail_out;

      png_ptr->zstream.avail_out = 0;

      if (ret == Z_STREAM_END)
      {
         
         png_ptr->zstream.next_out = NULL;

         png_ptr->mode |= PNG_AFTER_IDAT;
         png_ptr->flags |= PNG_FLAG_ZSTREAM_ENDED;
#ifdef PNG_READ_APNG_SUPPORTED
         png_ptr->num_frames_read++;
#endif

         if (png_ptr->zstream.avail_in > 0 || png_ptr->idat_size > 0)
            png_chunk_benign_error(png_ptr, "Extra compressed data");
         break;
      }

      if (ret != Z_OK)
      {
         png_zstream_error(png_ptr, ret);

         if (output != NULL)
            png_chunk_error(png_ptr, png_ptr->zstream.msg);

         else 
         {
            png_chunk_benign_error(png_ptr, png_ptr->zstream.msg);
            return;
         }
      }
   } while (avail_out > 0);

   if (avail_out > 0)
   {
      


      if (output != NULL)
         png_error(png_ptr, "Not enough image data");

      else 
         png_chunk_benign_error(png_ptr, "Too much image data");
   }
}

void 
png_read_finish_IDAT(png_structrp png_ptr)
{
   




   if ((png_ptr->flags & PNG_FLAG_ZSTREAM_ENDED) == 0)
   {
      



      png_read_IDAT_data(png_ptr, NULL, 0);
      png_ptr->zstream.next_out = NULL; 

      


      if ((png_ptr->flags & PNG_FLAG_ZSTREAM_ENDED) == 0)
      {
         png_ptr->mode |= PNG_AFTER_IDAT;
         png_ptr->flags |= PNG_FLAG_ZSTREAM_ENDED;
      }
   }

   


   if (png_ptr->zowner == png_IDAT)
   {
      
      png_ptr->zstream.next_in = NULL;
      png_ptr->zstream.avail_in = 0;

      
      png_ptr->zowner = 0;

      




      (void)png_crc_finish(png_ptr, png_ptr->idat_size);
   }
}

void 
png_read_finish_row(png_structrp png_ptr)
{
   

   
   static PNG_CONST png_byte png_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};

   
   static PNG_CONST png_byte png_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};

   
   static PNG_CONST png_byte png_pass_ystart[7] = {0, 0, 4, 0, 2, 0, 1};

   
   static PNG_CONST png_byte png_pass_yinc[7] = {8, 8, 8, 4, 4, 2, 2};

   png_debug(1, "in png_read_finish_row");
   png_ptr->row_number++;
   if (png_ptr->row_number < png_ptr->num_rows)
      return;

   if (png_ptr->interlaced != 0)
   {
      png_ptr->row_number = 0;

      


      memset(png_ptr->prev_row, 0, png_ptr->rowbytes + 1);

      do
      {
         png_ptr->pass++;

         if (png_ptr->pass >= 7)
            break;

         png_ptr->iwidth = (png_ptr->width +
            png_pass_inc[png_ptr->pass] - 1 -
            png_pass_start[png_ptr->pass]) /
            png_pass_inc[png_ptr->pass];

         if ((png_ptr->transformations & PNG_INTERLACE) == 0)
         {
            png_ptr->num_rows = (png_ptr->height +
                png_pass_yinc[png_ptr->pass] - 1 -
                png_pass_ystart[png_ptr->pass]) /
                png_pass_yinc[png_ptr->pass];
         }

         else  
            break; 

      } while (png_ptr->num_rows == 0 || png_ptr->iwidth == 0);

      if (png_ptr->pass < 7)
         return;
   }

   
   png_read_finish_IDAT(png_ptr);
}
#endif 

void 
png_read_start_row(png_structrp png_ptr)
{
   

   
   static PNG_CONST png_byte png_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};

   
   static PNG_CONST png_byte png_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};

   
   static PNG_CONST png_byte png_pass_ystart[7] = {0, 0, 4, 0, 2, 0, 1};

   
   static PNG_CONST png_byte png_pass_yinc[7] = {8, 8, 8, 4, 4, 2, 2};

   int max_pixel_depth;
   png_size_t row_bytes;

   png_debug(1, "in png_read_start_row");

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
   png_init_read_transformations(png_ptr);
#endif
   if (png_ptr->interlaced != 0)
   {
      if ((png_ptr->transformations & PNG_INTERLACE) == 0)
         png_ptr->num_rows = (png_ptr->height + png_pass_yinc[0] - 1 -
             png_pass_ystart[0]) / png_pass_yinc[0];

      else
         png_ptr->num_rows = png_ptr->height;

      png_ptr->iwidth = (png_ptr->width +
          png_pass_inc[png_ptr->pass] - 1 -
          png_pass_start[png_ptr->pass]) /
          png_pass_inc[png_ptr->pass];
   }

   else
   {
      png_ptr->num_rows = png_ptr->height;
      png_ptr->iwidth = png_ptr->width;
   }

   max_pixel_depth = png_ptr->pixel_depth;

   









#ifdef PNG_READ_PACK_SUPPORTED
   if ((png_ptr->transformations & PNG_PACK) != 0 && png_ptr->bit_depth < 8)
      max_pixel_depth = 8;
#endif

#ifdef PNG_READ_EXPAND_SUPPORTED
   if ((png_ptr->transformations & PNG_EXPAND) != 0)
   {
      if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         if (png_ptr->num_trans != 0)
            max_pixel_depth = 32;

         else
            max_pixel_depth = 24;
      }

      else if (png_ptr->color_type == PNG_COLOR_TYPE_GRAY)
      {
         if (max_pixel_depth < 8)
            max_pixel_depth = 8;

         if (png_ptr->num_trans != 0)
            max_pixel_depth *= 2;
      }

      else if (png_ptr->color_type == PNG_COLOR_TYPE_RGB)
      {
         if (png_ptr->num_trans != 0)
         {
            max_pixel_depth *= 4;
            max_pixel_depth /= 3;
         }
      }
   }
#endif

#ifdef PNG_READ_EXPAND_16_SUPPORTED
   if ((png_ptr->transformations & PNG_EXPAND_16) != 0)
   {
#     ifdef PNG_READ_EXPAND_SUPPORTED
         


         if ((png_ptr->transformations & PNG_EXPAND) != 0)
         {
            if (png_ptr->bit_depth < 16)
               max_pixel_depth *= 2;
         }
         else
#     endif
         png_ptr->transformations &= ~PNG_EXPAND_16;
   }
#endif

#ifdef PNG_READ_FILLER_SUPPORTED
   if ((png_ptr->transformations & (PNG_FILLER)) != 0)
   {
      if (png_ptr->color_type == PNG_COLOR_TYPE_GRAY)
      {
         if (max_pixel_depth <= 8)
            max_pixel_depth = 16;

         else
            max_pixel_depth = 32;
      }

      else if (png_ptr->color_type == PNG_COLOR_TYPE_RGB ||
         png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         if (max_pixel_depth <= 32)
            max_pixel_depth = 32;

         else
            max_pixel_depth = 64;
      }
   }
#endif

#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED
   if ((png_ptr->transformations & PNG_GRAY_TO_RGB) != 0)
   {
      if (
#ifdef PNG_READ_EXPAND_SUPPORTED
          (png_ptr->num_trans != 0 &&
          (png_ptr->transformations & PNG_EXPAND) != 0) ||
#endif
#ifdef PNG_READ_FILLER_SUPPORTED
          (png_ptr->transformations & (PNG_FILLER)) != 0 ||
#endif
          png_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      {
         if (max_pixel_depth <= 16)
            max_pixel_depth = 32;

         else
            max_pixel_depth = 64;
      }

      else
      {
         if (max_pixel_depth <= 8)
         {
            if (png_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
               max_pixel_depth = 32;

            else
               max_pixel_depth = 24;
         }

         else if (png_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
            max_pixel_depth = 64;

         else
            max_pixel_depth = 48;
      }
   }
#endif

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) && \
defined(PNG_USER_TRANSFORM_PTR_SUPPORTED)
   if ((png_ptr->transformations & PNG_USER_TRANSFORM) != 0)
   {
      int user_pixel_depth = png_ptr->user_transform_depth *
         png_ptr->user_transform_channels;

      if (user_pixel_depth > max_pixel_depth)
         max_pixel_depth = user_pixel_depth;
   }
#endif

   


   png_ptr->maximum_pixel_depth = (png_byte)max_pixel_depth;
   png_ptr->transformed_pixel_depth = 0; 

   


   row_bytes = ((png_ptr->width + 7) & ~((png_uint_32)7));
   


   row_bytes = PNG_ROWBYTES(max_pixel_depth, row_bytes) +
       1 + ((max_pixel_depth + 7) >> 3);

#ifdef PNG_MAX_MALLOC_64K
   if (row_bytes > (png_uint_32)65536L)
      png_error(png_ptr, "This image requires a row greater than 64KB");
#endif

   if (row_bytes + 48 > png_ptr->old_big_row_buf_size)
   {
     png_free(png_ptr, png_ptr->big_row_buf);
     png_free(png_ptr, png_ptr->big_prev_row);

     if (png_ptr->interlaced != 0)
        png_ptr->big_row_buf = (png_bytep)png_calloc(png_ptr,
            row_bytes + 48);

     else
        png_ptr->big_row_buf = (png_bytep)png_malloc(png_ptr, row_bytes + 48);

     png_ptr->big_prev_row = (png_bytep)png_malloc(png_ptr, row_bytes + 48);

#ifdef PNG_ALIGNED_MEMORY_SUPPORTED
     






     {
        png_bytep temp = png_ptr->big_row_buf + 32;
        int extra = (int)((temp - (png_bytep)0) & 0x0f);
        png_ptr->row_buf = temp - extra - 1;

        temp = png_ptr->big_prev_row + 32;
        extra = (int)((temp - (png_bytep)0) & 0x0f);
        png_ptr->prev_row = temp - extra - 1;
     }

#else
     
     png_ptr->row_buf = png_ptr->big_row_buf + 31;
     png_ptr->prev_row = png_ptr->big_prev_row + 31;
#endif
     png_ptr->old_big_row_buf_size = row_bytes + 48;
   }

#ifdef PNG_MAX_MALLOC_64K
   if (png_ptr->rowbytes > 65535)
      png_error(png_ptr, "This image requires a row greater than 64KB");

#endif
   if (png_ptr->rowbytes > (PNG_SIZE_MAX - 1))
      png_error(png_ptr, "Row has too many bytes to allocate in memory");

   memset(png_ptr->prev_row, 0, png_ptr->rowbytes + 1);

   png_debug1(3, "width = %u,", png_ptr->width);
   png_debug1(3, "height = %u,", png_ptr->height);
   png_debug1(3, "iwidth = %u,", png_ptr->iwidth);
   png_debug1(3, "num_rows = %u,", png_ptr->num_rows);
   png_debug1(3, "rowbytes = %lu,", (unsigned long)png_ptr->rowbytes);
   png_debug1(3, "irowbytes = %lu",
       (unsigned long)PNG_ROWBYTES(png_ptr->pixel_depth, png_ptr->iwidth) + 1);

   



   if (png_ptr->read_buffer != 0)
   {
      png_bytep buffer = png_ptr->read_buffer;

      png_ptr->read_buffer_size = 0;
      png_ptr->read_buffer = NULL;
      png_free(png_ptr, buffer);
   }

   




   if (png_inflate_claim(png_ptr, png_IDAT) != Z_OK)
      png_error(png_ptr, png_ptr->zstream.msg);

   png_ptr->flags |= PNG_FLAG_ROW_INIT;
}

#ifdef PNG_READ_APNG_SUPPORTED



void 
png_read_reset(png_structp png_ptr)
{
    png_ptr->mode &= ~PNG_HAVE_IDAT;
    png_ptr->mode &= ~PNG_AFTER_IDAT;
    png_ptr->row_number = 0;
    png_ptr->pass = 0;
}

void 
png_read_reinit(png_structp png_ptr, png_infop info_ptr)
{
    png_ptr->width = info_ptr->next_frame_width;
    png_ptr->height = info_ptr->next_frame_height;
    png_ptr->rowbytes = PNG_ROWBYTES(png_ptr->pixel_depth,png_ptr->width);
    png_ptr->info_rowbytes = PNG_ROWBYTES(info_ptr->pixel_depth,
        png_ptr->width);
    if (png_ptr->prev_row != NULL)
        memset(png_ptr->prev_row, 0, png_ptr->rowbytes + 1);
}

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED

void 
png_progressive_read_reset(png_structp png_ptr)
{
#ifdef PNG_READ_INTERLACING_SUPPORTED
    

    
    static PNG_CONST png_byte png_pass_start[] = {0, 4, 0, 2, 0, 1, 0};

    
    static PNG_CONST png_byte png_pass_inc[] = {8, 8, 4, 4, 2, 2, 1};

    
    static PNG_CONST png_byte png_pass_ystart[] = {0, 0, 4, 0, 2, 0, 1};

    
    static PNG_CONST png_byte png_pass_yinc[] = {8, 8, 8, 4, 4, 2, 2};

    if (png_ptr->interlaced != 0)
    {
        if ((png_ptr->transformations & PNG_INTERLACE) == 0)
            png_ptr->num_rows = (png_ptr->height + png_pass_yinc[0] - 1 -
                                png_pass_ystart[0]) / png_pass_yinc[0];
        else
            png_ptr->num_rows = png_ptr->height;

        png_ptr->iwidth = (png_ptr->width +
                           png_pass_inc[png_ptr->pass] - 1 -
                           png_pass_start[png_ptr->pass]) /
                           png_pass_inc[png_ptr->pass];
    }
    else
#endif 
    {
        png_ptr->num_rows = png_ptr->height;
        png_ptr->iwidth = png_ptr->width;
    }
    png_ptr->flags &= ~PNG_FLAG_ZSTREAM_ENDED;
    if (inflateReset(&(png_ptr->zstream)) != Z_OK)
        png_error(png_ptr, "inflateReset failed");
    png_ptr->zstream.avail_in = 0;
    png_ptr->zstream.next_in = 0;
    png_ptr->zstream.next_out = png_ptr->row_buf;
    png_ptr->zstream.avail_out = (uInt)PNG_ROWBYTES(png_ptr->pixel_depth,
        png_ptr->iwidth) + 1;
}
#endif 
#endif 
#endif 














#include "pngpriv.h"

#ifdef PNG_WRITE_SUPPORTED

#ifdef PNG_WRITE_INT_FUNCTIONS_SUPPORTED




void PNGAPI
png_save_uint_32(png_bytep buf, png_uint_32 i)
{
   buf[0] = (png_byte)(i >> 24);
   buf[1] = (png_byte)(i >> 16);
   buf[2] = (png_byte)(i >> 8);
   buf[3] = (png_byte)(i     );
}





void PNGAPI
png_save_uint_16(png_bytep buf, unsigned int i)
{
   buf[0] = (png_byte)(i >> 8);
   buf[1] = (png_byte)(i     );
}
#endif







void PNGAPI
png_write_sig(png_structrp png_ptr)
{
   png_byte png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};

#ifdef PNG_IO_STATE_SUPPORTED
   
   png_ptr->io_state = PNG_IO_WRITING | PNG_IO_SIGNATURE;
#endif

   
   png_write_data(png_ptr, &png_signature[png_ptr->sig_bytes],
      (png_size_t)(8 - png_ptr->sig_bytes));

   if (png_ptr->sig_bytes < 3)
      png_ptr->mode |= PNG_HAVE_PNG_SIGNATURE;
}





static void
png_write_chunk_header(png_structrp png_ptr, png_uint_32 chunk_name,
    png_uint_32 length)
{
   png_byte buf[8];

#if defined(PNG_DEBUG) && (PNG_DEBUG > 0)
   PNG_CSTRING_FROM_CHUNK(buf, chunk_name);
   png_debug2(0, "Writing %s chunk, length = %lu", buf, (unsigned long)length);
#endif

   if (png_ptr == NULL)
      return;

#ifdef PNG_IO_STATE_SUPPORTED
   


   png_ptr->io_state = PNG_IO_WRITING | PNG_IO_CHUNK_HDR;
#endif

   
   png_save_uint_32(buf, length);
   png_save_uint_32(buf + 4, chunk_name);
   png_write_data(png_ptr, buf, 8);

   
   png_ptr->chunk_name = chunk_name;

   
   png_reset_crc(png_ptr);

   png_calculate_crc(png_ptr, buf + 4, 4);

#ifdef PNG_IO_STATE_SUPPORTED
   


   png_ptr->io_state = PNG_IO_WRITING | PNG_IO_CHUNK_DATA;
#endif
}

void PNGAPI
png_write_chunk_start(png_structrp png_ptr, png_const_bytep chunk_string,
    png_uint_32 length)
{
   png_write_chunk_header(png_ptr, PNG_CHUNK_FROM_STRING(chunk_string), length);
}






void PNGAPI
png_write_chunk_data(png_structrp png_ptr, png_const_bytep data,
    png_size_t length)
{
   
   if (png_ptr == NULL)
      return;

   if (data != NULL && length > 0)
   {
      png_write_data(png_ptr, data, length);

      


      png_calculate_crc(png_ptr, data, length);
   }
}


void PNGAPI
png_write_chunk_end(png_structrp png_ptr)
{
   png_byte buf[4];

   if (png_ptr == NULL) return;

#ifdef PNG_IO_STATE_SUPPORTED
   


   png_ptr->io_state = PNG_IO_WRITING | PNG_IO_CHUNK_CRC;
#endif

   
   png_save_uint_32(buf, png_ptr->crc);

   png_write_data(png_ptr, buf, (png_size_t)4);
}










static void
png_write_complete_chunk(png_structrp png_ptr, png_uint_32 chunk_name,
   png_const_bytep data, png_size_t length)
{
   if (png_ptr == NULL)
      return;

   
   if (length > PNG_UINT_31_MAX)
      png_error(png_ptr, "length exceeds PNG maximum");

   png_write_chunk_header(png_ptr, chunk_name, (png_uint_32)length);
   png_write_chunk_data(png_ptr, data, length);
   png_write_chunk_end(png_ptr);
}


void PNGAPI
png_write_chunk(png_structrp png_ptr, png_const_bytep chunk_string,
   png_const_bytep data, png_size_t length)
{
   png_write_complete_chunk(png_ptr, PNG_CHUNK_FROM_STRING(chunk_string), data,
      length);
}





static png_alloc_size_t
png_image_size(png_structrp png_ptr)
{
   


   png_uint_32 h = png_ptr->height;

   if (png_ptr->rowbytes < 32768 && h < 32768)
   {
      if (png_ptr->interlaced != 0)
      {
         


         png_uint_32 w = png_ptr->width;
         unsigned int pd = png_ptr->pixel_depth;
         png_alloc_size_t cb_base;
         int pass;

         for (cb_base=0, pass=0; pass<=6; ++pass)
         {
            png_uint_32 pw = PNG_PASS_COLS(w, pass);

            if (pw > 0)
               cb_base += (PNG_ROWBYTES(pd, pw)+1) * PNG_PASS_ROWS(h, pass);
         }

         return cb_base;
      }

      else
         return (png_ptr->rowbytes+1) * h;
   }

   else
      return 0xffffffffU;
}

#ifdef PNG_WRITE_OPTIMIZE_CMF_SUPPORTED
   





static void
optimize_cmf(png_bytep data, png_alloc_size_t data_size)
{
   


   if (data_size <= 16384) 
   {
      unsigned int z_cmf = data[0];  

      if ((z_cmf & 0x0f) == 8 && (z_cmf & 0xf0) <= 0x70)
      {
         unsigned int z_cinfo;
         unsigned int half_z_window_size;

         z_cinfo = z_cmf >> 4;
         half_z_window_size = 1U << (z_cinfo + 7);

         if (data_size <= half_z_window_size) 
         {
            unsigned int tmp;

            do
            {
               half_z_window_size >>= 1;
               --z_cinfo;
            }
            while (z_cinfo > 0 && data_size <= half_z_window_size);

            z_cmf = (z_cmf & 0x0f) | (z_cinfo << 4);

            data[0] = (png_byte)z_cmf;
            tmp = data[1] & 0xe0;
            tmp += 0x1f - ((z_cmf << 8) + tmp) % 0x1f;
            data[1] = (png_byte)tmp;
         }
      }
   }
}
#endif 


static int
png_deflate_claim(png_structrp png_ptr, png_uint_32 owner,
   png_alloc_size_t data_size)
{
   if (png_ptr->zowner != 0)
   {
#if defined(PNG_WARNINGS_SUPPORTED) || defined(PNG_ERROR_TEXT_SUPPORTED)
      char msg[64];

      PNG_STRING_FROM_CHUNK(msg, owner);
      msg[4] = ':';
      msg[5] = ' ';
      PNG_STRING_FROM_CHUNK(msg+6, png_ptr->zowner);
      



      (void)png_safecat(msg, (sizeof msg), 10, " using zstream");
#endif
#if PNG_LIBPNG_BUILD_BASE_TYPE >= PNG_LIBPNG_BUILD_RC
         png_warning(png_ptr, msg);

         
         if (png_ptr->zowner == png_IDAT) 
         {
            png_ptr->zstream.msg = PNGZ_MSG_CAST("in use by IDAT");
            return Z_STREAM_ERROR;
         }

         png_ptr->zowner = 0;
#else
         png_error(png_ptr, msg);
#endif
   }

   {
      int level = png_ptr->zlib_level;
      int method = png_ptr->zlib_method;
      int windowBits = png_ptr->zlib_window_bits;
      int memLevel = png_ptr->zlib_mem_level;
      int strategy; 
      int ret; 

      if (owner == png_IDAT)
      {
         if ((png_ptr->flags & PNG_FLAG_ZLIB_CUSTOM_STRATEGY) != 0)
            strategy = png_ptr->zlib_strategy;

         else if (png_ptr->do_filter != PNG_FILTER_NONE)
            strategy = PNG_Z_DEFAULT_STRATEGY;

         else
            strategy = PNG_Z_DEFAULT_NOFILTER_STRATEGY;
      }

      else
      {
#ifdef PNG_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED
            level = png_ptr->zlib_text_level;
            method = png_ptr->zlib_text_method;
            windowBits = png_ptr->zlib_text_window_bits;
            memLevel = png_ptr->zlib_text_mem_level;
            strategy = png_ptr->zlib_text_strategy;
#else
            




            strategy = Z_DEFAULT_STRATEGY;
#endif
      }

      






      if (data_size <= 16384)
      {
         




         unsigned int half_window_size = 1U << (windowBits-1);

         while (data_size + 262 <= half_window_size)
         {
            half_window_size >>= 1;
            --windowBits;
         }
      }

      
      if ((png_ptr->flags & PNG_FLAG_ZSTREAM_INITIALIZED) != 0 &&
         (png_ptr->zlib_set_level != level ||
         png_ptr->zlib_set_method != method ||
         png_ptr->zlib_set_window_bits != windowBits ||
         png_ptr->zlib_set_mem_level != memLevel ||
         png_ptr->zlib_set_strategy != strategy))
      {
         if (deflateEnd(&png_ptr->zstream) != Z_OK)
            png_warning(png_ptr, "deflateEnd failed (ignored)");

         png_ptr->flags &= ~PNG_FLAG_ZSTREAM_INITIALIZED;
      }

      


      png_ptr->zstream.next_in = NULL;
      png_ptr->zstream.avail_in = 0;
      png_ptr->zstream.next_out = NULL;
      png_ptr->zstream.avail_out = 0;

      


      if ((png_ptr->flags & PNG_FLAG_ZSTREAM_INITIALIZED) != 0)
         ret = deflateReset(&png_ptr->zstream);

      else
      {
         ret = deflateInit2(&png_ptr->zstream, level, method, windowBits,
            memLevel, strategy);

         if (ret == Z_OK)
            png_ptr->flags |= PNG_FLAG_ZSTREAM_INITIALIZED;
      }

      


      if (ret == Z_OK)
         png_ptr->zowner = owner;

      else
         png_zstream_error(png_ptr, ret);

      return ret;
   }
}


void 
png_free_buffer_list(png_structrp png_ptr, png_compression_bufferp *listp)
{
   png_compression_bufferp list = *listp;

   if (list != NULL)
   {
      *listp = NULL;

      do
      {
         png_compression_bufferp next = list->next;

         png_free(png_ptr, list);
         list = next;
      }
      while (list != NULL);
   }
}

#ifdef PNG_WRITE_COMPRESSED_TEXT_SUPPORTED










typedef struct
{
   png_const_bytep      input;        
   png_alloc_size_t     input_len;    
   png_uint_32          output_len;   
   png_byte             output[1024]; 
} compression_state;

static void
png_text_compress_init(compression_state *comp, png_const_bytep input,
   png_alloc_size_t input_len)
{
   comp->input = input;
   comp->input_len = input_len;
   comp->output_len = 0;
}


static int
png_text_compress(png_structrp png_ptr, png_uint_32 chunk_name,
   compression_state *comp, png_uint_32 prefix_len)
{
   int ret;

   









   ret = png_deflate_claim(png_ptr, chunk_name, comp->input_len);

   if (ret != Z_OK)
      return ret;

   





   {
      png_compression_bufferp *end = &png_ptr->zbuffer_list;
      png_alloc_size_t input_len = comp->input_len; 
      png_uint_32 output_len;

      
      png_ptr->zstream.next_in = PNGZ_INPUT_CAST(comp->input);
      png_ptr->zstream.avail_in = 0; 
      png_ptr->zstream.next_out = comp->output;
      png_ptr->zstream.avail_out = (sizeof comp->output);

      output_len = png_ptr->zstream.avail_out;

      do
      {
         uInt avail_in = ZLIB_IO_MAX;

         if (avail_in > input_len)
            avail_in = (uInt)input_len;

         input_len -= avail_in;

         png_ptr->zstream.avail_in = avail_in;

         if (png_ptr->zstream.avail_out == 0)
         {
            png_compression_buffer *next;

            


            if (output_len + prefix_len > PNG_UINT_31_MAX)
            {
               ret = Z_MEM_ERROR;
               break;
            }

            


            next = *end;
            if (next == NULL)
            {
               next = png_voidcast(png_compression_bufferp, png_malloc_base
                  (png_ptr, PNG_COMPRESSION_BUFFER_SIZE(png_ptr)));

               if (next == NULL)
               {
                  ret = Z_MEM_ERROR;
                  break;
               }

               
               next->next = NULL;
               *end = next;
            }

            png_ptr->zstream.next_out = next->output;
            png_ptr->zstream.avail_out = png_ptr->zbuffer_size;
            output_len += png_ptr->zstream.avail_out;

            
            end = &next->next;
         }

         
         ret = deflate(&png_ptr->zstream,
            input_len > 0 ? Z_NO_FLUSH : Z_FINISH);

         


         input_len += png_ptr->zstream.avail_in;
         png_ptr->zstream.avail_in = 0; 
      }
      while (ret == Z_OK);

      


      output_len -= png_ptr->zstream.avail_out;
      png_ptr->zstream.avail_out = 0; 
      comp->output_len = output_len;

      



      if (output_len + prefix_len >= PNG_UINT_31_MAX)
      {
         png_ptr->zstream.msg = PNGZ_MSG_CAST("compressed data too long");
         ret = Z_MEM_ERROR;
      }

      else
         png_zstream_error(png_ptr, ret);

      
      png_ptr->zowner = 0;

      


      if (ret == Z_STREAM_END && input_len == 0)
      {
#ifdef PNG_WRITE_OPTIMIZE_CMF_SUPPORTED
         
         optimize_cmf(comp->output, comp->input_len);
#endif
         



         return Z_OK;
      }

      else
         return ret;
   }
}


static void
png_write_compressed_data_out(png_structrp png_ptr, compression_state *comp)
{
   png_uint_32 output_len = comp->output_len;
   png_const_bytep output = comp->output;
   png_uint_32 avail = (sizeof comp->output);
   png_compression_buffer *next = png_ptr->zbuffer_list;

   for (;;)
   {
      if (avail > output_len)
         avail = output_len;

      png_write_chunk_data(png_ptr, output, avail);

      output_len -= avail;

      if (output_len == 0 || next == NULL)
         break;

      avail = png_ptr->zbuffer_size;
      output = next->output;
      next = next->next;
   }

   
   if (output_len > 0)
      png_error(png_ptr, "error writing ancillary chunked compressed data");
}
#endif 

#if defined(PNG_WRITE_TEXT_SUPPORTED) || defined(PNG_WRITE_pCAL_SUPPORTED) || \
    defined(PNG_WRITE_iCCP_SUPPORTED) || defined(PNG_WRITE_sPLT_SUPPORTED)










static png_uint_32
png_check_keyword(png_structrp png_ptr, png_const_charp key, png_bytep new_key)
{
   png_const_charp orig_key = key;
   png_uint_32 key_len = 0;
   int bad_character = 0;
   int space = 1;

   png_debug(1, "in png_check_keyword");

   if (key == NULL)
   {
      *new_key = 0;
      return 0;
   }

   while (*key && key_len < 79)
   {
      png_byte ch = (png_byte)*key++;

      if ((ch > 32 && ch <= 126) || (ch >= 161 ))
         *new_key++ = ch, ++key_len, space = 0;

      else if (space == 0)
      {
         


         *new_key++ = 32, ++key_len, space = 1;

         
         if (ch != 32)
            bad_character = ch;
      }

      else if (bad_character == 0)
         bad_character = ch; 
   }

   if (key_len > 0 && space != 0) 
   {
      --key_len, --new_key;
      if (bad_character == 0)
         bad_character = 32;
   }

   
   *new_key = 0;

   if (key_len == 0)
      return 0;

#ifdef PNG_WARNINGS_SUPPORTED
   
   if (*key != 0) 
      png_warning(png_ptr, "keyword truncated");

   else if (bad_character != 0)
   {
      PNG_WARNING_PARAMETERS(p)

      png_warning_parameter(p, 1, orig_key);
      png_warning_parameter_signed(p, 2, PNG_NUMBER_FORMAT_02x, bad_character);

      png_formatted_warning(png_ptr, p, "keyword \"@1\": bad character '0x@2'");
   }
#endif 

   return key_len;
}
#endif 





void 
png_write_IHDR(png_structrp png_ptr, png_uint_32 width, png_uint_32 height,
    int bit_depth, int color_type, int compression_type, int filter_type,
    int interlace_type)
{
   png_byte buf[13]; 

   png_debug(1, "in png_write_IHDR");

   
   switch (color_type)
   {
      case PNG_COLOR_TYPE_GRAY:
         switch (bit_depth)
         {
            case 1:
            case 2:
            case 4:
            case 8:
#ifdef PNG_WRITE_16BIT_SUPPORTED
            case 16:
#endif
               png_ptr->channels = 1; break;

            default:
               png_error(png_ptr,
                   "Invalid bit depth for grayscale image");
         }
         break;

      case PNG_COLOR_TYPE_RGB:
#ifdef PNG_WRITE_16BIT_SUPPORTED
         if (bit_depth != 8 && bit_depth != 16)
#else
         if (bit_depth != 8)
#endif
            png_error(png_ptr, "Invalid bit depth for RGB image");

         png_ptr->channels = 3;
         break;

      case PNG_COLOR_TYPE_PALETTE:
         switch (bit_depth)
         {
            case 1:
            case 2:
            case 4:
            case 8:
               png_ptr->channels = 1;
               break;

            default:
               png_error(png_ptr, "Invalid bit depth for paletted image");
         }
         break;

      case PNG_COLOR_TYPE_GRAY_ALPHA:
         if (bit_depth != 8 && bit_depth != 16)
            png_error(png_ptr, "Invalid bit depth for grayscale+alpha image");

         png_ptr->channels = 2;
         break;

      case PNG_COLOR_TYPE_RGB_ALPHA:
#ifdef PNG_WRITE_16BIT_SUPPORTED
         if (bit_depth != 8 && bit_depth != 16)
#else
         if (bit_depth != 8)
#endif
            png_error(png_ptr, "Invalid bit depth for RGBA image");

         png_ptr->channels = 4;
         break;

      default:
         png_error(png_ptr, "Invalid image color type specified");
   }

   if (compression_type != PNG_COMPRESSION_TYPE_BASE)
   {
      png_warning(png_ptr, "Invalid compression type specified");
      compression_type = PNG_COMPRESSION_TYPE_BASE;
   }

   








   if (
#ifdef PNG_MNG_FEATURES_SUPPORTED
       !((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) != 0 &&
       ((png_ptr->mode & PNG_HAVE_PNG_SIGNATURE) == 0) &&
       (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_RGB_ALPHA) &&
       (filter_type == PNG_INTRAPIXEL_DIFFERENCING)) &&
#endif
       filter_type != PNG_FILTER_TYPE_BASE)
   {
      png_warning(png_ptr, "Invalid filter type specified");
      filter_type = PNG_FILTER_TYPE_BASE;
   }

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   if (interlace_type != PNG_INTERLACE_NONE &&
       interlace_type != PNG_INTERLACE_ADAM7)
   {
      png_warning(png_ptr, "Invalid interlace type specified");
      interlace_type = PNG_INTERLACE_ADAM7;
   }
#else
   interlace_type=PNG_INTERLACE_NONE;
#endif

   
   png_ptr->bit_depth = (png_byte)bit_depth;
   png_ptr->color_type = (png_byte)color_type;
   png_ptr->interlaced = (png_byte)interlace_type;
#ifdef PNG_MNG_FEATURES_SUPPORTED
   png_ptr->filter_type = (png_byte)filter_type;
#endif
   png_ptr->compression_type = (png_byte)compression_type;
   png_ptr->width = width;
   png_ptr->height = height;

   png_ptr->pixel_depth = (png_byte)(bit_depth * png_ptr->channels);
   png_ptr->rowbytes = PNG_ROWBYTES(png_ptr->pixel_depth, width);
   
   png_ptr->usr_width = png_ptr->width;
   png_ptr->usr_bit_depth = png_ptr->bit_depth;
   png_ptr->usr_channels = png_ptr->channels;

   
   png_save_uint_32(buf, width);
   png_save_uint_32(buf + 4, height);
   buf[8] = (png_byte)bit_depth;
   buf[9] = (png_byte)color_type;
   buf[10] = (png_byte)compression_type;
   buf[11] = (png_byte)filter_type;
   buf[12] = (png_byte)interlace_type;

   
   png_write_complete_chunk(png_ptr, png_IHDR, buf, (png_size_t)13);

#ifdef PNG_WRITE_APNG_SUPPORTED
   png_ptr->first_frame_width = width;
   png_ptr->first_frame_height = height;
#endif

   if ((png_ptr->do_filter) == PNG_NO_FILTERS)
   {
      if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE ||
          png_ptr->bit_depth < 8)
         png_ptr->do_filter = PNG_FILTER_NONE;

      else
         png_ptr->do_filter = PNG_ALL_FILTERS;
   }

   png_ptr->mode = PNG_HAVE_IHDR; 
}





void 
png_write_PLTE(png_structrp png_ptr, png_const_colorp palette,
    png_uint_32 num_pal)
{
   png_uint_32 i;
   png_const_colorp pal_ptr;
   png_byte buf[3];

   png_debug(1, "in png_write_PLTE");

   if ((
#ifdef PNG_MNG_FEATURES_SUPPORTED
       (png_ptr->mng_features_permitted & PNG_FLAG_MNG_EMPTY_PLTE) == 0 &&
#endif
       num_pal == 0) || num_pal > 256)
   {
      if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         png_error(png_ptr, "Invalid number of colors in palette");
      }

      else
      {
         png_warning(png_ptr, "Invalid number of colors in palette");
         return;
      }
   }

   if ((png_ptr->color_type & PNG_COLOR_MASK_COLOR) == 0)
   {
      png_warning(png_ptr,
          "Ignoring request to write a PLTE chunk in grayscale PNG");

      return;
   }

   png_ptr->num_palette = (png_uint_16)num_pal;
   png_debug1(3, "num_palette = %d", png_ptr->num_palette);

   png_write_chunk_header(png_ptr, png_PLTE, (png_uint_32)(num_pal * 3));
#ifdef PNG_POINTER_INDEXING_SUPPORTED

   for (i = 0, pal_ptr = palette; i < num_pal; i++, pal_ptr++)
   {
      buf[0] = pal_ptr->red;
      buf[1] = pal_ptr->green;
      buf[2] = pal_ptr->blue;
      png_write_chunk_data(png_ptr, buf, (png_size_t)3);
   }

#else
   


   pal_ptr=palette;

   for (i = 0; i < num_pal; i++)
   {
      buf[0] = pal_ptr[i].red;
      buf[1] = pal_ptr[i].green;
      buf[2] = pal_ptr[i].blue;
      png_write_chunk_data(png_ptr, buf, (png_size_t)3);
   }

#endif
   png_write_chunk_end(png_ptr);
   png_ptr->mode |= PNG_HAVE_PLTE;
}


















void 
png_compress_IDAT(png_structrp png_ptr, png_const_bytep input,
   png_alloc_size_t input_len, int flush)
{
   if (png_ptr->zowner != png_IDAT)
   {
      




      if (png_ptr->zbuffer_list == NULL)
      {
         png_ptr->zbuffer_list = png_voidcast(png_compression_bufferp,
            png_malloc(png_ptr, PNG_COMPRESSION_BUFFER_SIZE(png_ptr)));
         png_ptr->zbuffer_list->next = NULL;
      }

      else
         png_free_buffer_list(png_ptr, &png_ptr->zbuffer_list->next);

      
      if (png_deflate_claim(png_ptr, png_IDAT, png_image_size(png_ptr)) != Z_OK)
         png_error(png_ptr, png_ptr->zstream.msg);

      


      png_ptr->zstream.next_out = png_ptr->zbuffer_list->output;
      png_ptr->zstream.avail_out = png_ptr->zbuffer_size;
   }

   



   png_ptr->zstream.next_in = PNGZ_INPUT_CAST(input);
   png_ptr->zstream.avail_in = 0; 
   for (;;)
   {
      int ret;

      
      uInt avail = ZLIB_IO_MAX;

      if (avail > input_len)
         avail = (uInt)input_len; 

      png_ptr->zstream.avail_in = avail;
      input_len -= avail;

      ret = deflate(&png_ptr->zstream, input_len > 0 ? Z_NO_FLUSH : flush);

      
      input_len += png_ptr->zstream.avail_in;
      png_ptr->zstream.avail_in = 0;

      



      if (png_ptr->zstream.avail_out == 0)
      {
         png_bytep data = png_ptr->zbuffer_list->output;
         uInt size = png_ptr->zbuffer_size;

         


#ifdef PNG_WRITE_OPTIMIZE_CMF_SUPPORTED
            if ((png_ptr->mode & PNG_HAVE_IDAT) == 0 &&
                png_ptr->compression_type == PNG_COMPRESSION_TYPE_BASE)
               optimize_cmf(data, png_image_size(png_ptr));
#endif

#ifdef PNG_WRITE_APNG_SUPPORTED
         if (png_ptr->num_frames_written == 0)
#endif
         png_write_complete_chunk(png_ptr, png_IDAT, data, size);
#ifdef PNG_WRITE_APNG_SUPPORTED
         else
            png_write_fdAT(png_ptr, data, size);
#endif 

         png_ptr->mode |= PNG_HAVE_IDAT;

         png_ptr->zstream.next_out = data;
         png_ptr->zstream.avail_out = size;

         



         if (ret == Z_OK && flush != Z_NO_FLUSH)
            continue;
      }

      


      if (ret == Z_OK) 
      {
         



         if (input_len == 0)
         {
            if (flush == Z_FINISH)
               png_error(png_ptr, "Z_OK on Z_FINISH with output space");

            return;
         }
      }

      else if (ret == Z_STREAM_END && flush == Z_FINISH)
      {
         


         png_bytep data = png_ptr->zbuffer_list->output;
         uInt size = png_ptr->zbuffer_size - png_ptr->zstream.avail_out;

#ifdef PNG_WRITE_OPTIMIZE_CMF_SUPPORTED
         if ((png_ptr->mode & PNG_HAVE_IDAT) == 0 &&
             png_ptr->compression_type == PNG_COMPRESSION_TYPE_BASE)
            optimize_cmf(data, png_image_size(png_ptr));
#endif

#ifdef PNG_WRITE_APNG_SUPPORTED
         if (png_ptr->num_frames_written == 0)
#endif
         png_write_complete_chunk(png_ptr, png_IDAT, data, size);
#ifdef PNG_WRITE_APNG_SUPPORTED
         else
            png_write_fdAT(png_ptr, data, size);
#endif 

         png_ptr->zstream.avail_out = 0;
         png_ptr->zstream.next_out = NULL;
         png_ptr->mode |= PNG_HAVE_IDAT | PNG_AFTER_IDAT;

         png_ptr->zowner = 0; 
         return;
      }

      else
      {
         
         png_zstream_error(png_ptr, ret);
         png_error(png_ptr, png_ptr->zstream.msg);
      }
   }
}


void 
png_write_IEND(png_structrp png_ptr)
{
   png_debug(1, "in png_write_IEND");

   png_write_complete_chunk(png_ptr, png_IEND, NULL, (png_size_t)0);
   png_ptr->mode |= PNG_HAVE_IEND;
}

#ifdef PNG_WRITE_gAMA_SUPPORTED

void 
png_write_gAMA_fixed(png_structrp png_ptr, png_fixed_point file_gamma)
{
   png_byte buf[4];

   png_debug(1, "in png_write_gAMA");

   
   png_save_uint_32(buf, (png_uint_32)file_gamma);
   png_write_complete_chunk(png_ptr, png_gAMA, buf, (png_size_t)4);
}
#endif

#ifdef PNG_WRITE_sRGB_SUPPORTED

void 
png_write_sRGB(png_structrp png_ptr, int srgb_intent)
{
   png_byte buf[1];

   png_debug(1, "in png_write_sRGB");

   if (srgb_intent >= PNG_sRGB_INTENT_LAST)
      png_warning(png_ptr,
          "Invalid sRGB rendering intent specified");

   buf[0]=(png_byte)srgb_intent;
   png_write_complete_chunk(png_ptr, png_sRGB, buf, (png_size_t)1);
}
#endif

#ifdef PNG_WRITE_iCCP_SUPPORTED

void 
png_write_iCCP(png_structrp png_ptr, png_const_charp name,
    png_const_bytep profile)
{
   png_uint_32 name_len;
   png_uint_32 profile_len;
   png_byte new_name[81]; 
   compression_state comp;
   png_uint_32 temp;

   png_debug(1, "in png_write_iCCP");

   


   if (profile == NULL)
      png_error(png_ptr, "No profile for iCCP chunk"); 

   profile_len = png_get_uint_32(profile);

   if (profile_len < 132)
      png_error(png_ptr, "ICC profile too short");

   temp = (png_uint_32) (*(profile+8));
   if (temp > 3 && (profile_len & 0x03))
      png_error(png_ptr, "ICC profile length invalid (not a multiple of 4)");

   {
      png_uint_32 embedded_profile_len = png_get_uint_32(profile);

      if (profile_len != embedded_profile_len)
         png_error(png_ptr, "Profile length does not match profile");
   }

   name_len = png_check_keyword(png_ptr, name, new_name);

   if (name_len == 0)
      png_error(png_ptr, "iCCP: invalid keyword");

   new_name[++name_len] = PNG_COMPRESSION_TYPE_BASE;

   
   ++name_len;

   png_text_compress_init(&comp, profile, profile_len);

   
   if (png_text_compress(png_ptr, png_iCCP, &comp, name_len) != Z_OK)
      png_error(png_ptr, png_ptr->zstream.msg);

   png_write_chunk_header(png_ptr, png_iCCP, name_len + comp.output_len);

   png_write_chunk_data(png_ptr, new_name, name_len);

   png_write_compressed_data_out(png_ptr, &comp);

   png_write_chunk_end(png_ptr);
}
#endif

#ifdef PNG_WRITE_sPLT_SUPPORTED

void 
png_write_sPLT(png_structrp png_ptr, png_const_sPLT_tp spalette)
{
   png_uint_32 name_len;
   png_byte new_name[80];
   png_byte entrybuf[10];
   png_size_t entry_size = (spalette->depth == 8 ? 6 : 10);
   png_size_t palette_size = entry_size * spalette->nentries;
   png_sPLT_entryp ep;
#ifndef PNG_POINTER_INDEXING_SUPPORTED
   int i;
#endif

   png_debug(1, "in png_write_sPLT");

   name_len = png_check_keyword(png_ptr, spalette->name, new_name);

   if (name_len == 0)
      png_error(png_ptr, "sPLT: invalid keyword");

   
   png_write_chunk_header(png_ptr, png_sPLT,
       (png_uint_32)(name_len + 2 + palette_size));

   png_write_chunk_data(png_ptr, (png_bytep)new_name,
       (png_size_t)(name_len + 1));

   png_write_chunk_data(png_ptr, &spalette->depth, (png_size_t)1);

   
#ifdef PNG_POINTER_INDEXING_SUPPORTED
   for (ep = spalette->entries; ep<spalette->entries + spalette->nentries; ep++)
   {
      if (spalette->depth == 8)
      {
         entrybuf[0] = (png_byte)ep->red;
         entrybuf[1] = (png_byte)ep->green;
         entrybuf[2] = (png_byte)ep->blue;
         entrybuf[3] = (png_byte)ep->alpha;
         png_save_uint_16(entrybuf + 4, ep->frequency);
      }

      else
      {
         png_save_uint_16(entrybuf + 0, ep->red);
         png_save_uint_16(entrybuf + 2, ep->green);
         png_save_uint_16(entrybuf + 4, ep->blue);
         png_save_uint_16(entrybuf + 6, ep->alpha);
         png_save_uint_16(entrybuf + 8, ep->frequency);
      }

      png_write_chunk_data(png_ptr, entrybuf, entry_size);
   }
#else
   ep=spalette->entries;
   for (i = 0; i>spalette->nentries; i++)
   {
      if (spalette->depth == 8)
      {
         entrybuf[0] = (png_byte)ep[i].red;
         entrybuf[1] = (png_byte)ep[i].green;
         entrybuf[2] = (png_byte)ep[i].blue;
         entrybuf[3] = (png_byte)ep[i].alpha;
         png_save_uint_16(entrybuf + 4, ep[i].frequency);
      }

      else
      {
         png_save_uint_16(entrybuf + 0, ep[i].red);
         png_save_uint_16(entrybuf + 2, ep[i].green);
         png_save_uint_16(entrybuf + 4, ep[i].blue);
         png_save_uint_16(entrybuf + 6, ep[i].alpha);
         png_save_uint_16(entrybuf + 8, ep[i].frequency);
      }

      png_write_chunk_data(png_ptr, entrybuf, entry_size);
   }
#endif

   png_write_chunk_end(png_ptr);
}
#endif

#ifdef PNG_WRITE_sBIT_SUPPORTED

void 
png_write_sBIT(png_structrp png_ptr, png_const_color_8p sbit, int color_type)
{
   png_byte buf[4];
   png_size_t size;

   png_debug(1, "in png_write_sBIT");

   
   if ((color_type & PNG_COLOR_MASK_COLOR) != 0)
   {
      png_byte maxbits;

      maxbits = (png_byte)(color_type==PNG_COLOR_TYPE_PALETTE ? 8 :
          png_ptr->usr_bit_depth);

      if (sbit->red == 0 || sbit->red > maxbits ||
          sbit->green == 0 || sbit->green > maxbits ||
          sbit->blue == 0 || sbit->blue > maxbits)
      {
         png_warning(png_ptr, "Invalid sBIT depth specified");
         return;
      }

      buf[0] = sbit->red;
      buf[1] = sbit->green;
      buf[2] = sbit->blue;
      size = 3;
   }

   else
   {
      if (sbit->gray == 0 || sbit->gray > png_ptr->usr_bit_depth)
      {
         png_warning(png_ptr, "Invalid sBIT depth specified");
         return;
      }

      buf[0] = sbit->gray;
      size = 1;
   }

   if ((color_type & PNG_COLOR_MASK_ALPHA) != 0)
   {
      if (sbit->alpha == 0 || sbit->alpha > png_ptr->usr_bit_depth)
      {
         png_warning(png_ptr, "Invalid sBIT depth specified");
         return;
      }

      buf[size++] = sbit->alpha;
   }

   png_write_complete_chunk(png_ptr, png_sBIT, buf, size);
}
#endif

#ifdef PNG_WRITE_cHRM_SUPPORTED

void 
png_write_cHRM_fixed(png_structrp png_ptr, const png_xy *xy)
{
   png_byte buf[32];

   png_debug(1, "in png_write_cHRM");

   
   png_save_int_32(buf,      xy->whitex);
   png_save_int_32(buf +  4, xy->whitey);

   png_save_int_32(buf +  8, xy->redx);
   png_save_int_32(buf + 12, xy->redy);

   png_save_int_32(buf + 16, xy->greenx);
   png_save_int_32(buf + 20, xy->greeny);

   png_save_int_32(buf + 24, xy->bluex);
   png_save_int_32(buf + 28, xy->bluey);

   png_write_complete_chunk(png_ptr, png_cHRM, buf, 32);
}
#endif

#ifdef PNG_WRITE_tRNS_SUPPORTED

void 
png_write_tRNS(png_structrp png_ptr, png_const_bytep trans_alpha,
    png_const_color_16p tran, int num_trans, int color_type)
{
   png_byte buf[6];

   png_debug(1, "in png_write_tRNS");

   if (color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (num_trans <= 0 || num_trans > (int)png_ptr->num_palette)
      {
         png_app_warning(png_ptr,
             "Invalid number of transparent colors specified");
         return;
      }

      
      png_write_complete_chunk(png_ptr, png_tRNS, trans_alpha,
         (png_size_t)num_trans);
   }

   else if (color_type == PNG_COLOR_TYPE_GRAY)
   {
      
      if (tran->gray >= (1 << png_ptr->bit_depth))
      {
         png_app_warning(png_ptr,
             "Ignoring attempt to write tRNS chunk out-of-range for bit_depth");

         return;
      }

      png_save_uint_16(buf, tran->gray);
      png_write_complete_chunk(png_ptr, png_tRNS, buf, (png_size_t)2);
   }

   else if (color_type == PNG_COLOR_TYPE_RGB)
   {
      
      png_save_uint_16(buf, tran->red);
      png_save_uint_16(buf + 2, tran->green);
      png_save_uint_16(buf + 4, tran->blue);
#ifdef PNG_WRITE_16BIT_SUPPORTED
      if (png_ptr->bit_depth == 8 && (buf[0] | buf[2] | buf[4]) != 0)
#else
      if ((buf[0] | buf[2] | buf[4]) != 0)
#endif
      {
         png_app_warning(png_ptr,
           "Ignoring attempt to write 16-bit tRNS chunk when bit_depth is 8");
         return;
      }

      png_write_complete_chunk(png_ptr, png_tRNS, buf, (png_size_t)6);
   }

   else
   {
      png_app_warning(png_ptr, "Can't write tRNS with an alpha channel");
   }
}
#endif

#ifdef PNG_WRITE_bKGD_SUPPORTED

void 
png_write_bKGD(png_structrp png_ptr, png_const_color_16p back, int color_type)
{
   png_byte buf[6];

   png_debug(1, "in png_write_bKGD");

   if (color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (
#ifdef PNG_MNG_FEATURES_SUPPORTED
          (png_ptr->num_palette != 0 ||
          (png_ptr->mng_features_permitted & PNG_FLAG_MNG_EMPTY_PLTE) == 0) &&
#endif
         back->index >= png_ptr->num_palette)
      {
         png_warning(png_ptr, "Invalid background palette index");
         return;
      }

      buf[0] = back->index;
      png_write_complete_chunk(png_ptr, png_bKGD, buf, (png_size_t)1);
   }

   else if ((color_type & PNG_COLOR_MASK_COLOR) != 0)
   {
      png_save_uint_16(buf, back->red);
      png_save_uint_16(buf + 2, back->green);
      png_save_uint_16(buf + 4, back->blue);
#ifdef PNG_WRITE_16BIT_SUPPORTED
      if (png_ptr->bit_depth == 8 && (buf[0] | buf[2] | buf[4]) != 0)
#else
      if ((buf[0] | buf[2] | buf[4]) != 0)
#endif
      {
         png_warning(png_ptr,
             "Ignoring attempt to write 16-bit bKGD chunk when bit_depth is 8");

         return;
      }

      png_write_complete_chunk(png_ptr, png_bKGD, buf, (png_size_t)6);
   }

   else
   {
      if (back->gray >= (1 << png_ptr->bit_depth))
      {
         png_warning(png_ptr,
             "Ignoring attempt to write bKGD chunk out-of-range for bit_depth");

         return;
      }

      png_save_uint_16(buf, back->gray);
      png_write_complete_chunk(png_ptr, png_bKGD, buf, (png_size_t)2);
   }
}
#endif

#ifdef PNG_WRITE_hIST_SUPPORTED

void 
png_write_hIST(png_structrp png_ptr, png_const_uint_16p hist, int num_hist)
{
   int i;
   png_byte buf[3];

   png_debug(1, "in png_write_hIST");

   if (num_hist > (int)png_ptr->num_palette)
   {
      png_debug2(3, "num_hist = %d, num_palette = %d", num_hist,
          png_ptr->num_palette);

      png_warning(png_ptr, "Invalid number of histogram entries specified");
      return;
   }

   png_write_chunk_header(png_ptr, png_hIST, (png_uint_32)(num_hist * 2));

   for (i = 0; i < num_hist; i++)
   {
      png_save_uint_16(buf, hist[i]);
      png_write_chunk_data(png_ptr, buf, (png_size_t)2);
   }

   png_write_chunk_end(png_ptr);
}
#endif

#ifdef PNG_WRITE_tEXt_SUPPORTED

void 
png_write_tEXt(png_structrp png_ptr, png_const_charp key, png_const_charp text,
    png_size_t text_len)
{
   png_uint_32 key_len;
   png_byte new_key[80];

   png_debug(1, "in png_write_tEXt");

   key_len = png_check_keyword(png_ptr, key, new_key);

   if (key_len == 0)
      png_error(png_ptr, "tEXt: invalid keyword");

   if (text == NULL || *text == '\0')
      text_len = 0;

   else
      text_len = strlen(text);

   if (text_len > PNG_UINT_31_MAX - (key_len+1))
      png_error(png_ptr, "tEXt: text too long");

   
   png_write_chunk_header(png_ptr, png_tEXt,
       (png_uint_32)(key_len + text_len + 1));
   





   png_write_chunk_data(png_ptr, new_key, key_len + 1);

   if (text_len != 0)
      png_write_chunk_data(png_ptr, (png_const_bytep)text, text_len);

   png_write_chunk_end(png_ptr);
}
#endif

#ifdef PNG_WRITE_zTXt_SUPPORTED

void 
png_write_zTXt(png_structrp png_ptr, png_const_charp key, png_const_charp text,
    int compression)
{
   png_uint_32 key_len;
   png_byte new_key[81];
   compression_state comp;

   png_debug(1, "in png_write_zTXt");

   if (compression == PNG_TEXT_COMPRESSION_NONE)
   {
      png_write_tEXt(png_ptr, key, text, 0);
      return;
   }

   if (compression != PNG_TEXT_COMPRESSION_zTXt)
      png_error(png_ptr, "zTXt: invalid compression type");

   key_len = png_check_keyword(png_ptr, key, new_key);

   if (key_len == 0)
      png_error(png_ptr, "zTXt: invalid keyword");

   
   new_key[++key_len] = PNG_COMPRESSION_TYPE_BASE;
   ++key_len;

   
   png_text_compress_init(&comp, (png_const_bytep)text,
      text == NULL ? 0 : strlen(text));

   if (png_text_compress(png_ptr, png_zTXt, &comp, key_len) != Z_OK)
      png_error(png_ptr, png_ptr->zstream.msg);

   
   png_write_chunk_header(png_ptr, png_zTXt, key_len + comp.output_len);

   
   png_write_chunk_data(png_ptr, new_key, key_len);

   
   png_write_compressed_data_out(png_ptr, &comp);

   
   png_write_chunk_end(png_ptr);
}
#endif

#ifdef PNG_WRITE_iTXt_SUPPORTED

void 
png_write_iTXt(png_structrp png_ptr, int compression, png_const_charp key,
    png_const_charp lang, png_const_charp lang_key, png_const_charp text)
{
   png_uint_32 key_len, prefix_len;
   png_size_t lang_len, lang_key_len;
   png_byte new_key[82];
   compression_state comp;

   png_debug(1, "in png_write_iTXt");

   key_len = png_check_keyword(png_ptr, key, new_key);

   if (key_len == 0)
      png_error(png_ptr, "iTXt: invalid keyword");

   
   switch (compression)
   {
      case PNG_ITXT_COMPRESSION_NONE:
      case PNG_TEXT_COMPRESSION_NONE:
         compression = new_key[++key_len] = 0; 
         break;

      case PNG_TEXT_COMPRESSION_zTXt:
      case PNG_ITXT_COMPRESSION_zTXt:
         compression = new_key[++key_len] = 1; 
         break;

      default:
         png_error(png_ptr, "iTXt: invalid compression");
   }

   new_key[++key_len] = PNG_COMPRESSION_TYPE_BASE;
   ++key_len; 

   









   if (lang == NULL) lang = ""; 
   lang_len = strlen(lang)+1;
   if (lang_key == NULL) lang_key = ""; 
   lang_key_len = strlen(lang_key)+1;
   if (text == NULL) text = ""; 

   prefix_len = key_len;
   if (lang_len > PNG_UINT_31_MAX-prefix_len)
      prefix_len = PNG_UINT_31_MAX;
   else
      prefix_len = (png_uint_32)(prefix_len + lang_len);

   if (lang_key_len > PNG_UINT_31_MAX-prefix_len)
      prefix_len = PNG_UINT_31_MAX;
   else
      prefix_len = (png_uint_32)(prefix_len + lang_key_len);

   png_text_compress_init(&comp, (png_const_bytep)text, strlen(text));

   if (compression != 0)
   {
      if (png_text_compress(png_ptr, png_iTXt, &comp, prefix_len) != Z_OK)
         png_error(png_ptr, png_ptr->zstream.msg);
   }

   else
   {
      if (comp.input_len > PNG_UINT_31_MAX-prefix_len)
         png_error(png_ptr, "iTXt: uncompressed text too long");

      
      comp.output_len = (png_uint_32)comp.input_len;
   }

   png_write_chunk_header(png_ptr, png_iTXt, comp.output_len + prefix_len);

   png_write_chunk_data(png_ptr, new_key, key_len);

   png_write_chunk_data(png_ptr, (png_const_bytep)lang, lang_len);

   png_write_chunk_data(png_ptr, (png_const_bytep)lang_key, lang_key_len);

   if (compression != 0)
      png_write_compressed_data_out(png_ptr, &comp);

   else
      png_write_chunk_data(png_ptr, (png_const_bytep)text, comp.output_len);

   png_write_chunk_end(png_ptr);
}
#endif

#ifdef PNG_WRITE_oFFs_SUPPORTED

void 
png_write_oFFs(png_structrp png_ptr, png_int_32 x_offset, png_int_32 y_offset,
    int unit_type)
{
   png_byte buf[9];

   png_debug(1, "in png_write_oFFs");

   if (unit_type >= PNG_OFFSET_LAST)
      png_warning(png_ptr, "Unrecognized unit type for oFFs chunk");

   png_save_int_32(buf, x_offset);
   png_save_int_32(buf + 4, y_offset);
   buf[8] = (png_byte)unit_type;

   png_write_complete_chunk(png_ptr, png_oFFs, buf, (png_size_t)9);
}
#endif
#ifdef PNG_WRITE_pCAL_SUPPORTED

void 
png_write_pCAL(png_structrp png_ptr, png_charp purpose, png_int_32 X0,
    png_int_32 X1, int type, int nparams, png_const_charp units,
    png_charpp params)
{
   png_uint_32 purpose_len;
   png_size_t units_len, total_len;
   png_size_tp params_len;
   png_byte buf[10];
   png_byte new_purpose[80];
   int i;

   png_debug1(1, "in png_write_pCAL (%d parameters)", nparams);

   if (type >= PNG_EQUATION_LAST)
      png_error(png_ptr, "Unrecognized equation type for pCAL chunk");

   purpose_len = png_check_keyword(png_ptr, purpose, new_purpose);

   if (purpose_len == 0)
      png_error(png_ptr, "pCAL: invalid keyword");

   ++purpose_len; 

   png_debug1(3, "pCAL purpose length = %d", (int)purpose_len);
   units_len = strlen(units) + (nparams == 0 ? 0 : 1);
   png_debug1(3, "pCAL units length = %d", (int)units_len);
   total_len = purpose_len + units_len + 10;

   params_len = (png_size_tp)png_malloc(png_ptr,
       (png_alloc_size_t)(nparams * (sizeof (png_size_t))));

   


   for (i = 0; i < nparams; i++)
   {
      params_len[i] = strlen(params[i]) + (i == nparams - 1 ? 0 : 1);
      png_debug2(3, "pCAL parameter %d length = %lu", i,
          (unsigned long)params_len[i]);
      total_len += params_len[i];
   }

   png_debug1(3, "pCAL total length = %d", (int)total_len);
   png_write_chunk_header(png_ptr, png_pCAL, (png_uint_32)total_len);
   png_write_chunk_data(png_ptr, new_purpose, purpose_len);
   png_save_int_32(buf, X0);
   png_save_int_32(buf + 4, X1);
   buf[8] = (png_byte)type;
   buf[9] = (png_byte)nparams;
   png_write_chunk_data(png_ptr, buf, (png_size_t)10);
   png_write_chunk_data(png_ptr, (png_const_bytep)units, (png_size_t)units_len);

   for (i = 0; i < nparams; i++)
   {
      png_write_chunk_data(png_ptr, (png_const_bytep)params[i], params_len[i]);
   }

   png_free(png_ptr, params_len);
   png_write_chunk_end(png_ptr);
}
#endif

#ifdef PNG_WRITE_sCAL_SUPPORTED

void 
png_write_sCAL_s(png_structrp png_ptr, int unit, png_const_charp width,
    png_const_charp height)
{
   png_byte buf[64];
   png_size_t wlen, hlen, total_len;

   png_debug(1, "in png_write_sCAL_s");

   wlen = strlen(width);
   hlen = strlen(height);
   total_len = wlen + hlen + 2;

   if (total_len > 64)
   {
      png_warning(png_ptr, "Can't write sCAL (buffer too small)");
      return;
   }

   buf[0] = (png_byte)unit;
   memcpy(buf + 1, width, wlen + 1);      
   memcpy(buf + wlen + 2, height, hlen);  

   png_debug1(3, "sCAL total length = %u", (unsigned int)total_len);
   png_write_complete_chunk(png_ptr, png_sCAL, buf, total_len);
}
#endif

#ifdef PNG_WRITE_pHYs_SUPPORTED

void 
png_write_pHYs(png_structrp png_ptr, png_uint_32 x_pixels_per_unit,
    png_uint_32 y_pixels_per_unit,
    int unit_type)
{
   png_byte buf[9];

   png_debug(1, "in png_write_pHYs");

   if (unit_type >= PNG_RESOLUTION_LAST)
      png_warning(png_ptr, "Unrecognized unit type for pHYs chunk");

   png_save_uint_32(buf, x_pixels_per_unit);
   png_save_uint_32(buf + 4, y_pixels_per_unit);
   buf[8] = (png_byte)unit_type;

   png_write_complete_chunk(png_ptr, png_pHYs, buf, (png_size_t)9);
}
#endif

#ifdef PNG_WRITE_tIME_SUPPORTED



void 
png_write_tIME(png_structrp png_ptr, png_const_timep mod_time)
{
   png_byte buf[7];

   png_debug(1, "in png_write_tIME");

   if (mod_time->month  > 12 || mod_time->month  < 1 ||
       mod_time->day    > 31 || mod_time->day    < 1 ||
       mod_time->hour   > 23 || mod_time->second > 60)
   {
      png_warning(png_ptr, "Invalid time specified for tIME chunk");
      return;
   }

   png_save_uint_16(buf, mod_time->year);
   buf[2] = mod_time->month;
   buf[3] = mod_time->day;
   buf[4] = mod_time->hour;
   buf[5] = mod_time->minute;
   buf[6] = mod_time->second;

   png_write_complete_chunk(png_ptr, png_tIME, buf, (png_size_t)7);
}
#endif

#ifdef PNG_WRITE_APNG_SUPPORTED
void 
png_write_acTL(png_structp png_ptr,
    png_uint_32 num_frames, png_uint_32 num_plays)
{
    png_byte buf[8];

    png_debug(1, "in png_write_acTL");

    png_ptr->num_frames_to_write = num_frames;

    if ((png_ptr->apng_flags & PNG_FIRST_FRAME_HIDDEN) != 0)
        num_frames--;

    png_save_uint_32(buf, num_frames);
    png_save_uint_32(buf + 4, num_plays);

    png_write_complete_chunk(png_ptr, png_acTL, buf, (png_size_t)8);
}

void 
png_write_fcTL(png_structp png_ptr, png_uint_32 width, png_uint_32 height,
    png_uint_32 x_offset, png_uint_32 y_offset,
    png_uint_16 delay_num, png_uint_16 delay_den, png_byte dispose_op,
    png_byte blend_op)
{
    png_byte buf[26];

    png_debug(1, "in png_write_fcTL");

    if (png_ptr->num_frames_written == 0 && (x_offset != 0 || y_offset != 0))
        png_error(png_ptr, "x and/or y offset for the first frame aren't 0");
    if (png_ptr->num_frames_written == 0 &&
        (width != png_ptr->first_frame_width ||
         height != png_ptr->first_frame_height))
        png_error(png_ptr, "width and/or height in the first frame's fcTL "
                           "don't match the ones in IHDR");

    
    png_ensure_fcTL_is_valid(png_ptr, width, height, x_offset, y_offset,
                             delay_num, delay_den, dispose_op, blend_op);

    png_save_uint_32(buf, png_ptr->next_seq_num);
    png_save_uint_32(buf + 4, width);
    png_save_uint_32(buf + 8, height);
    png_save_uint_32(buf + 12, x_offset);
    png_save_uint_32(buf + 16, y_offset);
    png_save_uint_16(buf + 20, delay_num);
    png_save_uint_16(buf + 22, delay_den);
    buf[24] = dispose_op;
    buf[25] = blend_op;

    png_write_complete_chunk(png_ptr, png_fcTL, buf, (png_size_t)26);

    png_ptr->next_seq_num++;
}

void 
png_write_fdAT(png_structp png_ptr,
    png_const_bytep data, png_size_t length)
{
    png_byte buf[4];

    png_write_chunk_header(png_ptr, png_fdAT, (png_uint_32)(4 + length));

    png_save_uint_32(buf, png_ptr->next_seq_num);
    png_write_chunk_data(png_ptr, buf, 4);

    png_write_chunk_data(png_ptr, data, length);

    png_write_chunk_end(png_ptr);

    png_ptr->next_seq_num++;
}
#endif 


void 
png_write_start_row(png_structrp png_ptr)
{
#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   

   
   static PNG_CONST png_byte png_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};

   
   static PNG_CONST png_byte png_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};

   
   static PNG_CONST png_byte png_pass_ystart[7] = {0, 0, 4, 0, 2, 0, 1};

   
   static PNG_CONST png_byte png_pass_yinc[7] = {8, 8, 8, 4, 4, 2, 2};
#endif

   png_alloc_size_t buf_size;
   int usr_pixel_depth;

   png_debug(1, "in png_write_start_row");

   usr_pixel_depth = png_ptr->usr_channels * png_ptr->usr_bit_depth;
   buf_size = PNG_ROWBYTES(usr_pixel_depth, png_ptr->width) + 1;

   
   png_ptr->transformed_pixel_depth = png_ptr->pixel_depth;
   png_ptr->maximum_pixel_depth = (png_byte)usr_pixel_depth;

   
   png_ptr->row_buf = (png_bytep)png_malloc(png_ptr, buf_size);

   png_ptr->row_buf[0] = PNG_FILTER_VALUE_NONE;

#ifdef PNG_WRITE_FILTER_SUPPORTED
   
   if (png_ptr->do_filter & PNG_FILTER_SUB)
   {
      png_ptr->sub_row = (png_bytep)png_malloc(png_ptr, png_ptr->rowbytes + 1);

      png_ptr->sub_row[0] = PNG_FILTER_VALUE_SUB;
   }

   
   if ((png_ptr->do_filter &
      (PNG_FILTER_AVG | PNG_FILTER_UP | PNG_FILTER_PAETH)) != 0)
   {
      
      png_ptr->prev_row = (png_bytep)png_calloc(png_ptr, buf_size);

      if ((png_ptr->do_filter & PNG_FILTER_UP) != 0)
      {
         png_ptr->up_row = (png_bytep)png_malloc(png_ptr,
            png_ptr->rowbytes + 1);

         png_ptr->up_row[0] = PNG_FILTER_VALUE_UP;
      }

      if ((png_ptr->do_filter & PNG_FILTER_AVG) != 0)
      {
         png_ptr->avg_row = (png_bytep)png_malloc(png_ptr,
             png_ptr->rowbytes + 1);

         png_ptr->avg_row[0] = PNG_FILTER_VALUE_AVG;
      }

      if ((png_ptr->do_filter & PNG_FILTER_PAETH) != 0)
      {
         png_ptr->paeth_row = (png_bytep)png_malloc(png_ptr,
             png_ptr->rowbytes + 1);

         png_ptr->paeth_row[0] = PNG_FILTER_VALUE_PAETH;
      }
   }
#endif 

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   
   if (png_ptr->interlaced != 0)
   {
      if ((png_ptr->transformations & PNG_INTERLACE) == 0)
      {
         png_ptr->num_rows = (png_ptr->height + png_pass_yinc[0] - 1 -
             png_pass_ystart[0]) / png_pass_yinc[0];

         png_ptr->usr_width = (png_ptr->width + png_pass_inc[0] - 1 -
             png_pass_start[0]) / png_pass_inc[0];
      }

      else
      {
         png_ptr->num_rows = png_ptr->height;
         png_ptr->usr_width = png_ptr->width;
      }
   }

   else
#endif
   {
      png_ptr->num_rows = png_ptr->height;
      png_ptr->usr_width = png_ptr->width;
   }
}


void 
png_write_finish_row(png_structrp png_ptr)
{
#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   

   
   static PNG_CONST png_byte png_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};

   
   static PNG_CONST png_byte png_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};

   
   static PNG_CONST png_byte png_pass_ystart[7] = {0, 0, 4, 0, 2, 0, 1};

   
   static PNG_CONST png_byte png_pass_yinc[7] = {8, 8, 8, 4, 4, 2, 2};
#endif

   png_debug(1, "in png_write_finish_row");

   
   png_ptr->row_number++;

   
   if (png_ptr->row_number < png_ptr->num_rows)
      return;

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   
   if (png_ptr->interlaced != 0)
   {
      png_ptr->row_number = 0;
      if ((png_ptr->transformations & PNG_INTERLACE) != 0)
      {
         png_ptr->pass++;
      }

      else
      {
         
         do
         {
            png_ptr->pass++;

            if (png_ptr->pass >= 7)
               break;

            png_ptr->usr_width = (png_ptr->width +
                png_pass_inc[png_ptr->pass] - 1 -
                png_pass_start[png_ptr->pass]) /
                png_pass_inc[png_ptr->pass];

            png_ptr->num_rows = (png_ptr->height +
                png_pass_yinc[png_ptr->pass] - 1 -
                png_pass_ystart[png_ptr->pass]) /
                png_pass_yinc[png_ptr->pass];

            if ((png_ptr->transformations & PNG_INTERLACE) != 0)
               break;

         } while (png_ptr->usr_width == 0 || png_ptr->num_rows == 0);

      }

      
      if (png_ptr->pass < 7)
      {
         if (png_ptr->prev_row != NULL)
            memset(png_ptr->prev_row, 0,
                (png_size_t)(PNG_ROWBYTES(png_ptr->usr_channels*
                png_ptr->usr_bit_depth, png_ptr->width)) + 1);

         return;
      }
   }
#endif

   

   png_compress_IDAT(png_ptr, NULL, 0, Z_FINISH);
}

#ifdef PNG_WRITE_INTERLACING_SUPPORTED







void 
png_do_write_interlace(png_row_infop row_info, png_bytep row, int pass)
{
   

   
   static PNG_CONST png_byte png_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};

   
   static PNG_CONST png_byte  png_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};

   png_debug(1, "in png_do_write_interlace");

   
   if (pass < 6)
   {
      
      switch (row_info->pixel_depth)
      {
         case 1:
         {
            png_bytep sp;
            png_bytep dp;
            int shift;
            int d;
            int value;
            png_uint_32 i;
            png_uint_32 row_width = row_info->width;

            dp = row;
            d = 0;
            shift = 7;

            for (i = png_pass_start[pass]; i < row_width;
               i += png_pass_inc[pass])
            {
               sp = row + (png_size_t)(i >> 3);
               value = (int)(*sp >> (7 - (int)(i & 0x07))) & 0x01;
               d |= (value << shift);

               if (shift == 0)
               {
                  shift = 7;
                  *dp++ = (png_byte)d;
                  d = 0;
               }

               else
                  shift--;

            }
            if (shift != 7)
               *dp = (png_byte)d;

            break;
         }

         case 2:
         {
            png_bytep sp;
            png_bytep dp;
            int shift;
            int d;
            int value;
            png_uint_32 i;
            png_uint_32 row_width = row_info->width;

            dp = row;
            shift = 6;
            d = 0;

            for (i = png_pass_start[pass]; i < row_width;
               i += png_pass_inc[pass])
            {
               sp = row + (png_size_t)(i >> 2);
               value = (*sp >> ((3 - (int)(i & 0x03)) << 1)) & 0x03;
               d |= (value << shift);

               if (shift == 0)
               {
                  shift = 6;
                  *dp++ = (png_byte)d;
                  d = 0;
               }

               else
                  shift -= 2;
            }
            if (shift != 6)
               *dp = (png_byte)d;

            break;
         }

         case 4:
         {
            png_bytep sp;
            png_bytep dp;
            int shift;
            int d;
            int value;
            png_uint_32 i;
            png_uint_32 row_width = row_info->width;

            dp = row;
            shift = 4;
            d = 0;
            for (i = png_pass_start[pass]; i < row_width;
                i += png_pass_inc[pass])
            {
               sp = row + (png_size_t)(i >> 1);
               value = (*sp >> ((1 - (int)(i & 0x01)) << 2)) & 0x0f;
               d |= (value << shift);

               if (shift == 0)
               {
                  shift = 4;
                  *dp++ = (png_byte)d;
                  d = 0;
               }

               else
                  shift -= 4;
            }
            if (shift != 4)
               *dp = (png_byte)d;

            break;
         }

         default:
         {
            png_bytep sp;
            png_bytep dp;
            png_uint_32 i;
            png_uint_32 row_width = row_info->width;
            png_size_t pixel_bytes;

            
            dp = row;

            
            pixel_bytes = (row_info->pixel_depth >> 3);

            
            for (i = png_pass_start[pass]; i < row_width;
               i += png_pass_inc[pass])
            {
               
               sp = row + (png_size_t)i * pixel_bytes;

               
               if (dp != sp)
                  memcpy(dp, sp, pixel_bytes);

               
               dp += pixel_bytes;
            }
            break;
         }
      }
      
      row_info->width = (row_info->width +
          png_pass_inc[pass] - 1 -
          png_pass_start[pass]) /
          png_pass_inc[pass];

      row_info->rowbytes = PNG_ROWBYTES(row_info->pixel_depth,
          row_info->width);
   }
}
#endif





static void 
png_write_filtered_row(png_structrp png_ptr, png_bytep filtered_row,
   png_size_t row_bytes);

#define PNG_MAXSUM (((png_uint_32)(-1)) >> 1)
#define PNG_HISHIFT 10
#define PNG_LOMASK ((png_uint_32)0xffffL)
#define PNG_HIMASK ((png_uint_32)(~PNG_LOMASK >> PNG_HISHIFT))
void 
png_write_find_filter(png_structrp png_ptr, png_row_infop row_info)
{
   png_bytep best_row;
#ifdef PNG_WRITE_FILTER_SUPPORTED
   png_bytep prev_row, row_buf;
   png_uint_32 mins, bpp;
   png_byte filter_to_do = png_ptr->do_filter;
   png_size_t row_bytes = row_info->rowbytes;
#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
   int num_p_filters = png_ptr->num_prev_filters;
#endif

   png_debug(1, "in png_write_find_filter");

#ifndef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
  if (png_ptr->row_number == 0 && filter_to_do == PNG_ALL_FILTERS)
  {
     
     filter_to_do &= ~(PNG_FILTER_UP | PNG_FILTER_PAETH);
  }
#endif

   
   bpp = (row_info->pixel_depth + 7) >> 3;

   prev_row = png_ptr->prev_row;
#endif
   best_row = png_ptr->row_buf;
#ifdef PNG_WRITE_FILTER_SUPPORTED
   row_buf = best_row;
   mins = PNG_MAXSUM;

   
























   


   if ((filter_to_do & PNG_FILTER_NONE) != 0 && filter_to_do != PNG_FILTER_NONE)
   {
      png_bytep rp;
      png_uint_32 sum = 0;
      png_size_t i;
      int v;

      for (i = 0, rp = row_buf + 1; i < row_bytes; i++, rp++)
      {
         v = *rp;
         sum += (v < 128) ? v : 256 - v;
      }

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         png_uint_32 sumhi, sumlo;
         int j;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK; 

         
         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_NONE)
            {
               sumlo = (sumlo * png_ptr->filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;

               sumhi = (sumhi * png_ptr->filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;
            }
         }

         



         sumlo = (sumlo * png_ptr->filter_costs[PNG_FILTER_VALUE_NONE]) >>
             PNG_COST_SHIFT;

         sumhi = (sumhi * png_ptr->filter_costs[PNG_FILTER_VALUE_NONE]) >>
             PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;

         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }
#endif
      mins = sum;
   }

   
   if (filter_to_do == PNG_FILTER_SUB)
   
   {
      png_bytep rp, lp, dp;
      png_size_t i;

      for (i = 0, rp = row_buf + 1, dp = png_ptr->sub_row + 1; i < bpp;
           i++, rp++, dp++)
      {
         *dp = *rp;
      }

      for (lp = row_buf + 1; i < row_bytes;
         i++, rp++, lp++, dp++)
      {
         *dp = (png_byte)((int)*rp - (int)*lp);
      }

      best_row = png_ptr->sub_row;
   }

   else if ((filter_to_do & PNG_FILTER_SUB) != 0)
   {
      png_bytep rp, dp, lp;
      png_uint_32 sum = 0, lmins = mins;
      png_size_t i;
      int v;

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
      



      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 lmhi, lmlo;
         lmlo = lmins & PNG_LOMASK;
         lmhi = (lmins >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_SUB)
            {
               lmlo = (lmlo * png_ptr->inv_filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;

               lmhi = (lmhi * png_ptr->inv_filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;
            }
         }

         lmlo = (lmlo * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_SUB]) >>
             PNG_COST_SHIFT;

         lmhi = (lmhi * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_SUB]) >>
             PNG_COST_SHIFT;

         if (lmhi > PNG_HIMASK)
            lmins = PNG_MAXSUM;

         else
            lmins = (lmhi << PNG_HISHIFT) + lmlo;
      }
#endif

      for (i = 0, rp = row_buf + 1, dp = png_ptr->sub_row + 1; i < bpp;
           i++, rp++, dp++)
      {
         v = *dp = *rp;

         sum += (v < 128) ? v : 256 - v;
      }

      for (lp = row_buf + 1; i < row_bytes;
         i++, rp++, lp++, dp++)
      {
         v = *dp = (png_byte)((int)*rp - (int)*lp);

         sum += (v < 128) ? v : 256 - v;

         if (sum > lmins)  
            break;
      }

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 sumhi, sumlo;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_SUB)
            {
               sumlo = (sumlo * png_ptr->inv_filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;

               sumhi = (sumhi * png_ptr->inv_filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_SUB]) >>
             PNG_COST_SHIFT;

         sumhi = (sumhi * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_SUB]) >>
             PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;

         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }
#endif

      if (sum < mins)
      {
         mins = sum;
         best_row = png_ptr->sub_row;
      }
   }

   
   if (filter_to_do == PNG_FILTER_UP)
   {
      png_bytep rp, dp, pp;
      png_size_t i;

      for (i = 0, rp = row_buf + 1, dp = png_ptr->up_row + 1,
          pp = prev_row + 1; i < row_bytes;
          i++, rp++, pp++, dp++)
      {
         *dp = (png_byte)((int)*rp - (int)*pp);
      }

      best_row = png_ptr->up_row;
   }

   else if ((filter_to_do & PNG_FILTER_UP) != 0)
   {
      png_bytep rp, dp, pp;
      png_uint_32 sum = 0, lmins = mins;
      png_size_t i;
      int v;


#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 lmhi, lmlo;
         lmlo = lmins & PNG_LOMASK;
         lmhi = (lmins >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_UP)
            {
               lmlo = (lmlo * png_ptr->inv_filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;

               lmhi = (lmhi * png_ptr->inv_filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;
            }
         }

         lmlo = (lmlo * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_UP]) >>
             PNG_COST_SHIFT;

         lmhi = (lmhi * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_UP]) >>
             PNG_COST_SHIFT;

         if (lmhi > PNG_HIMASK)
            lmins = PNG_MAXSUM;

         else
            lmins = (lmhi << PNG_HISHIFT) + lmlo;
      }
#endif

      for (i = 0, rp = row_buf + 1, dp = png_ptr->up_row + 1,
          pp = prev_row + 1; i < row_bytes; i++)
      {
         v = *dp++ = (png_byte)((int)*rp++ - (int)*pp++);

         sum += (v < 128) ? v : 256 - v;

         if (sum > lmins)  
            break;
      }

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 sumhi, sumlo;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_UP)
            {
               sumlo = (sumlo * png_ptr->filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;

               sumhi = (sumhi * png_ptr->filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * png_ptr->filter_costs[PNG_FILTER_VALUE_UP]) >>
             PNG_COST_SHIFT;

         sumhi = (sumhi * png_ptr->filter_costs[PNG_FILTER_VALUE_UP]) >>
             PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;

         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }
#endif

      if (sum < mins)
      {
         mins = sum;
         best_row = png_ptr->up_row;
      }
   }

   
   if (filter_to_do == PNG_FILTER_AVG)
   {
      png_bytep rp, dp, pp, lp;
      png_uint_32 i;

      for (i = 0, rp = row_buf + 1, dp = png_ptr->avg_row + 1,
           pp = prev_row + 1; i < bpp; i++)
      {
         *dp++ = (png_byte)((int)*rp++ - ((int)*pp++ / 2));
      }

      for (lp = row_buf + 1; i < row_bytes; i++)
      {
         *dp++ =
             (png_byte)((int)*rp++ - (((int)*pp++ + (int)*lp++) / 2));
      }
      best_row = png_ptr->avg_row;
   }

   else if ((filter_to_do & PNG_FILTER_AVG) != 0)
   {
      png_bytep rp, dp, pp, lp;
      png_uint_32 sum = 0, lmins = mins;
      png_size_t i;
      int v;

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 lmhi, lmlo;
         lmlo = lmins & PNG_LOMASK;
         lmhi = (lmins >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_AVG)
            {
               lmlo = (lmlo * png_ptr->inv_filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;

               lmhi = (lmhi * png_ptr->inv_filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;
            }
         }

         lmlo = (lmlo * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_AVG]) >>
             PNG_COST_SHIFT;

         lmhi = (lmhi * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_AVG]) >>
             PNG_COST_SHIFT;

         if (lmhi > PNG_HIMASK)
            lmins = PNG_MAXSUM;

         else
            lmins = (lmhi << PNG_HISHIFT) + lmlo;
      }
#endif

      for (i = 0, rp = row_buf + 1, dp = png_ptr->avg_row + 1,
           pp = prev_row + 1; i < bpp; i++)
      {
         v = *dp++ = (png_byte)((int)*rp++ - ((int)*pp++ / 2));

         sum += (v < 128) ? v : 256 - v;
      }

      for (lp = row_buf + 1; i < row_bytes; i++)
      {
         v = *dp++ =
             (png_byte)(((int)*rp++ - ((int)*pp++ + (int)*lp++) / 2));

         sum += (v < 128) ? v : 256 - v;

         if (sum > lmins)  
            break;
      }

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 sumhi, sumlo;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_NONE)
            {
               sumlo = (sumlo * png_ptr->filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;

               sumhi = (sumhi * png_ptr->filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * png_ptr->filter_costs[PNG_FILTER_VALUE_AVG]) >>
             PNG_COST_SHIFT;

         sumhi = (sumhi * png_ptr->filter_costs[PNG_FILTER_VALUE_AVG]) >>
             PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;

         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }
#endif

      if (sum < mins)
      {
         mins = sum;
         best_row = png_ptr->avg_row;
      }
   }

   
   if ((filter_to_do == PNG_FILTER_PAETH) != 0)
   {
      png_bytep rp, dp, pp, cp, lp;
      png_size_t i;

      for (i = 0, rp = row_buf + 1, dp = png_ptr->paeth_row + 1,
          pp = prev_row + 1; i < bpp; i++)
      {
         *dp++ = (png_byte)((int)*rp++ - (int)*pp++);
      }

      for (lp = row_buf + 1, cp = prev_row + 1; i < row_bytes; i++)
      {
         int a, b, c, pa, pb, pc, p;

         b = *pp++;
         c = *cp++;
         a = *lp++;

         p = b - c;
         pc = a - c;

#ifdef PNG_USE_ABS
         pa = abs(p);
         pb = abs(pc);
         pc = abs(p + pc);
#else
         pa = p < 0 ? -p : p;
         pb = pc < 0 ? -pc : pc;
         pc = (p + pc) < 0 ? -(p + pc) : p + pc;
#endif

         p = (pa <= pb && pa <=pc) ? a : (pb <= pc) ? b : c;

         *dp++ = (png_byte)((int)*rp++ - p);
      }
      best_row = png_ptr->paeth_row;
   }

   else if ((filter_to_do & PNG_FILTER_PAETH) != 0)
   {
      png_bytep rp, dp, pp, cp, lp;
      png_uint_32 sum = 0, lmins = mins;
      png_size_t i;
      int v;

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 lmhi, lmlo;
         lmlo = lmins & PNG_LOMASK;
         lmhi = (lmins >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_PAETH)
            {
               lmlo = (lmlo * png_ptr->inv_filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;

               lmhi = (lmhi * png_ptr->inv_filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;
            }
         }

         lmlo = (lmlo * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_PAETH]) >>
             PNG_COST_SHIFT;

         lmhi = (lmhi * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_PAETH]) >>
             PNG_COST_SHIFT;

         if (lmhi > PNG_HIMASK)
            lmins = PNG_MAXSUM;

         else
            lmins = (lmhi << PNG_HISHIFT) + lmlo;
      }
#endif

      for (i = 0, rp = row_buf + 1, dp = png_ptr->paeth_row + 1,
          pp = prev_row + 1; i < bpp; i++)
      {
         v = *dp++ = (png_byte)((int)*rp++ - (int)*pp++);

         sum += (v < 128) ? v : 256 - v;
      }

      for (lp = row_buf + 1, cp = prev_row + 1; i < row_bytes; i++)
      {
         int a, b, c, pa, pb, pc, p;

         b = *pp++;
         c = *cp++;
         a = *lp++;

#ifndef PNG_SLOW_PAETH
         p = b - c;
         pc = a - c;
#ifdef PNG_USE_ABS
         pa = abs(p);
         pb = abs(pc);
         pc = abs(p + pc);
#else
         pa = p < 0 ? -p : p;
         pb = pc < 0 ? -pc : pc;
         pc = (p + pc) < 0 ? -(p + pc) : p + pc;
#endif
         p = (pa <= pb && pa <=pc) ? a : (pb <= pc) ? b : c;
#else 
         p = a + b - c;
         pa = abs(p - a);
         pb = abs(p - b);
         pc = abs(p - c);

         if (pa <= pb && pa <= pc)
            p = a;

         else if (pb <= pc)
            p = b;

         else
            p = c;
#endif 

         v = *dp++ = (png_byte)((int)*rp++ - p);

         sum += (v < 128) ? v : 256 - v;

         if (sum > lmins)  
            break;
      }

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 sumhi, sumlo;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_PAETH)
            {
               sumlo = (sumlo * png_ptr->filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;

               sumhi = (sumhi * png_ptr->filter_weights[j]) >>
                   PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * png_ptr->filter_costs[PNG_FILTER_VALUE_PAETH]) >>
             PNG_COST_SHIFT;

         sumhi = (sumhi * png_ptr->filter_costs[PNG_FILTER_VALUE_PAETH]) >>
             PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;

         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }
#endif

      if (sum < mins)
      {
         best_row = png_ptr->paeth_row;
      }
   }
#endif 

   
   png_write_filtered_row(png_ptr, best_row, row_info->rowbytes+1);

#ifdef PNG_WRITE_FILTER_SUPPORTED
#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
   
   if (png_ptr->num_prev_filters > 0)
   {
      int j;

      for (j = 1; j < num_p_filters; j++)
      {
         png_ptr->prev_filters[j] = png_ptr->prev_filters[j - 1];
      }

      png_ptr->prev_filters[j] = best_row[0];
   }
#endif
#endif 
}



static void
png_write_filtered_row(png_structrp png_ptr, png_bytep filtered_row,
   png_size_t full_row_length)
{
   png_debug(1, "in png_write_filtered_row");

   png_debug1(2, "filter = %d", filtered_row[0]);

   png_compress_IDAT(png_ptr, filtered_row, full_row_length, Z_NO_FLUSH);

#ifdef PNG_WRITE_FILTER_SUPPORTED
   
   if (png_ptr->prev_row != NULL)
   {
      png_bytep tptr;

      tptr = png_ptr->prev_row;
      png_ptr->prev_row = png_ptr->row_buf;
      png_ptr->row_buf = tptr;
   }
#endif 

   
   png_write_finish_row(png_ptr);

#ifdef PNG_WRITE_FLUSH_SUPPORTED
   png_ptr->flush_rows++;

   if (png_ptr->flush_dist > 0 &&
       png_ptr->flush_rows >= png_ptr->flush_dist)
   {
      png_write_flush(png_ptr);
   }
#endif 
}

#ifdef PNG_WRITE_APNG_SUPPORTED
void 
png_write_reset(png_structp png_ptr)
{
    png_ptr->row_number = 0;
    png_ptr->pass = 0;
    png_ptr->mode &= ~PNG_HAVE_IDAT;
}

void 
png_write_reinit(png_structp png_ptr, png_infop info_ptr,
                 png_uint_32 width, png_uint_32 height)
{
    if (png_ptr->num_frames_written == 0 &&
        (width != png_ptr->first_frame_width ||
         height != png_ptr->first_frame_height))
        png_error(png_ptr, "width and/or height in the first frame's fcTL "
                           "don't match the ones in IHDR");
    if (width > png_ptr->first_frame_width ||
        height > png_ptr->first_frame_height)
        png_error(png_ptr, "width and/or height for a frame greater than"
                           "the ones in IHDR");

    png_set_IHDR(png_ptr, info_ptr, width, height,
                 info_ptr->bit_depth, info_ptr->color_type,
                 info_ptr->interlace_type, info_ptr->compression_type,
                 info_ptr->filter_type);

    png_ptr->width = width;
    png_ptr->height = height;
    png_ptr->rowbytes = PNG_ROWBYTES(png_ptr->pixel_depth, width);
    png_ptr->usr_width = png_ptr->width;
}
#endif 
#endif 

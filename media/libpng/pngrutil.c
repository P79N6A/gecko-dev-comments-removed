















#include "pngpriv.h"

#ifdef PNG_READ_SUPPORTED

#define png_strtod(p,a,b) strtod(a,b)

png_uint_32 PNGAPI
png_get_uint_31(png_structp png_ptr, png_const_bytep buf)
{
   png_uint_32 uval = png_get_uint_32(buf);

   if (uval > PNG_UINT_31_MAX)
      png_error(png_ptr, "PNG unsigned integer out of range");

   return (uval);
}

#if defined(PNG_READ_gAMA_SUPPORTED) || defined(PNG_READ_cHRM_SUPPORTED)





#define PNG_FIXED_ERROR (-1)

static png_fixed_point 
png_get_fixed_point(png_structp png_ptr, png_const_bytep buf)
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
png_read_sig(png_structp png_ptr, png_infop info_ptr)
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

   if (png_sig_cmp(info_ptr->signature, num_checked, num_to_check))
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
png_read_chunk_header(png_structp png_ptr)
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
png_crc_read(png_structp png_ptr, png_bytep buf, png_size_t length)
{
   if (png_ptr == NULL)
      return;

   png_read_data(png_ptr, buf, length);
   png_calculate_crc(png_ptr, buf, length);
}






int 
png_crc_finish(png_structp png_ptr, png_uint_32 skip)
{
   png_size_t i;
   png_size_t istop = png_ptr->zbuf_size;

   for (i = (png_size_t)skip; i > istop; i -= istop)
   {
      png_crc_read(png_ptr, png_ptr->zbuf, png_ptr->zbuf_size);
   }

   if (i)
   {
      png_crc_read(png_ptr, png_ptr->zbuf, i);
   }

   if (png_crc_error(png_ptr))
   {
      if (PNG_CHUNK_ANCILLIARY(png_ptr->chunk_name) ?
          !(png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_NOWARN) :
          (png_ptr->flags & PNG_FLAG_CRC_CRITICAL_USE))
      {
         png_chunk_warning(png_ptr, "CRC error");
      }

      else
      {
         png_chunk_benign_error(png_ptr, "CRC error");
         return (0);
      }

      return (1);
   }

   return (0);
}




int 
png_crc_error(png_structp png_ptr)
{
   png_byte crc_bytes[4];
   png_uint_32 crc;
   int need_crc = 1;

   if (PNG_CHUNK_ANCILLIARY(png_ptr->chunk_name))
   {
      if ((png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_MASK) ==
          (PNG_FLAG_CRC_ANCILLARY_USE | PNG_FLAG_CRC_ANCILLARY_NOWARN))
         need_crc = 0;
   }

   else 
   {
      if (png_ptr->flags & PNG_FLAG_CRC_CRITICAL_IGNORE)
         need_crc = 0;
   }

#ifdef PNG_IO_STATE_SUPPORTED
   png_ptr->io_state = PNG_IO_READING | PNG_IO_CHUNK_CRC;
#endif

   
   png_read_data(png_ptr, crc_bytes, 4);

   if (need_crc)
   {
      crc = png_get_uint_32(crc_bytes);
      return ((int)(crc != png_ptr->crc));
   }

   else
      return (0);
}

#ifdef PNG_READ_COMPRESSED_TEXT_SUPPORTED
static png_size_t
png_inflate(png_structp png_ptr, png_bytep data, png_size_t size,
    png_bytep output, png_size_t output_size)
{
   png_size_t count = 0;

   








   png_ptr->zstream.next_in = data;
   
   png_ptr->zstream.avail_in = 0;

   while (1)
   {
      int ret, avail;

      






      if (png_ptr->zstream.avail_in == 0 && size > 0)
      {
         if (size <= ZLIB_IO_MAX)
         {
            
            png_ptr->zstream.avail_in = (uInt)size;
            size = 0;
         }

         else
         {
            png_ptr->zstream.avail_in = ZLIB_IO_MAX;
            size -= ZLIB_IO_MAX;
         }
      }

      


      png_ptr->zstream.next_out = png_ptr->zbuf;
      png_ptr->zstream.avail_out = png_ptr->zbuf_size;

      ret = inflate(&png_ptr->zstream, Z_NO_FLUSH);
      avail = png_ptr->zbuf_size - png_ptr->zstream.avail_out;

      


      if ((ret == Z_OK || ret == Z_STREAM_END) && avail > 0)
      {
         png_size_t space = avail; 

         if (output != 0 && output_size > count)
         {
            png_size_t copy = output_size - count;

            if (space < copy)
               copy = space;

            png_memcpy(output + count, png_ptr->zbuf, copy);
         }
         count += space;
      }

      if (ret == Z_OK)
         continue;

      


      png_ptr->zstream.avail_in = 0;
      inflateReset(&png_ptr->zstream);

      if (ret == Z_STREAM_END)
         return count; 

      



#     ifdef PNG_WARNINGS_SUPPORTED
      {
         png_const_charp msg;

         if (png_ptr->zstream.msg != 0)
            msg = png_ptr->zstream.msg;

         else switch (ret)
         {
            case Z_BUF_ERROR:
               msg = "Buffer error in compressed datastream";
               break;

            case Z_DATA_ERROR:
               msg = "Data error in compressed datastream";
               break;

            default:
               msg = "Incomplete compressed datastream";
               break;
         }

         png_chunk_warning(png_ptr, msg);
      }
#     endif

      


      return 0;
   }
}








void 
png_decompress_chunk(png_structp png_ptr, int comp_type,
    png_size_t chunklength,
    png_size_t prefix_size, png_size_t *newlength)
{
   
   if (prefix_size > chunklength)
   {
      
      png_warning(png_ptr, "invalid chunklength");
      prefix_size = 0; 
   }

   else if (comp_type == PNG_COMPRESSION_TYPE_BASE)
   {
      png_size_t expanded_size = png_inflate(png_ptr,
          (png_bytep)(png_ptr->chunkdata + prefix_size),
          chunklength - prefix_size,
          0,            
          0);           

      


      if (prefix_size >= (~(png_size_t)0) - 1 ||
         expanded_size >= (~(png_size_t)0) - 1 - prefix_size
#ifdef PNG_USER_LIMITS_SUPPORTED
         || (png_ptr->user_chunk_malloc_max &&
          (prefix_size + expanded_size >= png_ptr->user_chunk_malloc_max - 1))
#else
         || ((PNG_USER_CHUNK_MALLOC_MAX > 0) &&
          prefix_size + expanded_size >= PNG_USER_CHUNK_MALLOC_MAX - 1)
#endif
         )
         png_warning(png_ptr, "Exceeded size limit while expanding chunk");

      




      else if (expanded_size > 0)
      {
         
         png_size_t new_size = 0;
         png_charp text = (png_charp)png_malloc_warn(png_ptr,
             prefix_size + expanded_size + 1);

         if (text != NULL)
         {
            png_memcpy(text, png_ptr->chunkdata, prefix_size);
            new_size = png_inflate(png_ptr,
                (png_bytep)(png_ptr->chunkdata + prefix_size),
                chunklength - prefix_size,
                (png_bytep)(text + prefix_size), expanded_size);
            text[prefix_size + expanded_size] = 0; 

            if (new_size == expanded_size)
            {
               png_free(png_ptr, png_ptr->chunkdata);
               png_ptr->chunkdata = text;
               *newlength = prefix_size + expanded_size;
               return; 
            }

            png_warning(png_ptr, "png_inflate logic error");
            png_free(png_ptr, text);
         }

         else
            png_warning(png_ptr, "Not enough memory to decompress chunk");
      }
   }

   else 
   {
      PNG_WARNING_PARAMETERS(p)
      png_warning_parameter_signed(p, 1, PNG_NUMBER_FORMAT_d, comp_type);
      png_formatted_warning(png_ptr, p, "Unknown compression type @1");

      
   }

   



   {
      png_charp text = (png_charp)png_malloc_warn(png_ptr, prefix_size + 1);

      if (text != NULL)
      {
         if (prefix_size > 0)
            png_memcpy(text, png_ptr->chunkdata, prefix_size);

         png_free(png_ptr, png_ptr->chunkdata);
         png_ptr->chunkdata = text;

         
         *(png_ptr->chunkdata + prefix_size) = 0x00;
      }
      
   }

   *newlength = prefix_size;
}
#endif 


void 
png_handle_IHDR(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_byte buf[13];
   png_uint_32 width, height;
   int bit_depth, color_type, compression_type, filter_type;
   int interlace_type;

   png_debug(1, "in png_handle_IHDR");

   if (png_ptr->mode & PNG_HAVE_IHDR)
      png_error(png_ptr, "Out of place IHDR");

   
   if (length != 13)
      png_error(png_ptr, "Invalid IHDR chunk");

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

   
   png_ptr->pixel_depth = (png_byte)(png_ptr->bit_depth *
   png_ptr->channels);
   png_ptr->rowbytes = PNG_ROWBYTES(png_ptr->pixel_depth, png_ptr->width);
   png_debug1(3, "bit_depth = %d", png_ptr->bit_depth);
   png_debug1(3, "channels = %d", png_ptr->channels);
   png_debug1(3, "rowbytes = %lu", (unsigned long)png_ptr->rowbytes);
   png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth,
       color_type, interlace_type, compression_type, filter_type);
}


void 
png_handle_PLTE(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_color palette[PNG_MAX_PALETTE_LENGTH];
   int num, i;
#ifdef PNG_POINTER_INDEXING_SUPPORTED
   png_colorp pal_ptr;
#endif

   png_debug(1, "in png_handle_PLTE");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before PLTE");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid PLTE after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (png_ptr->mode & PNG_HAVE_PLTE)
      png_error(png_ptr, "Duplicate PLTE chunk");

   png_ptr->mode |= PNG_HAVE_PLTE;

   if (!(png_ptr->color_type&PNG_COLOR_MASK_COLOR))
   {
      png_warning(png_ptr,
          "Ignoring PLTE chunk in grayscale PNG");
      png_crc_finish(png_ptr, length);
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
      if (png_ptr->color_type != PNG_COLOR_TYPE_PALETTE)
      {
         png_warning(png_ptr, "Invalid palette chunk");
         png_crc_finish(png_ptr, length);
         return;
      }

      else
      {
         png_error(png_ptr, "Invalid palette chunk");
      }
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
   else if (png_crc_error(png_ptr))  
   {
      




      if (!(png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_USE))
      {
         if (png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_NOWARN)
         {
            png_chunk_benign_error(png_ptr, "CRC error");
         }

         else
         {
            png_chunk_warning(png_ptr, "CRC error");
            return;
         }
      }

      
      else if (!(png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_NOWARN))
      {
         png_chunk_warning(png_ptr, "CRC error");
      }
   }
#endif

   png_set_PLTE(png_ptr, info_ptr, palette, num);

#ifdef PNG_READ_tRNS_SUPPORTED
   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_tRNS))
      {
         if (png_ptr->num_trans > (png_uint_16)num)
         {
            png_warning(png_ptr, "Truncating incorrect tRNS chunk length");
            png_ptr->num_trans = (png_uint_16)num;
         }

         if (info_ptr->num_trans > (png_uint_16)num)
         {
            png_warning(png_ptr, "Truncating incorrect info tRNS chunk length");
            info_ptr->num_trans = (png_uint_16)num;
         }
      }
   }
#endif

}

void 
png_handle_IEND(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_debug(1, "in png_handle_IEND");

   if (!(png_ptr->mode & PNG_HAVE_IHDR) || !(png_ptr->mode & PNG_HAVE_IDAT))
   {
      png_error(png_ptr, "No image in file");
   }

   png_ptr->mode |= (PNG_AFTER_IDAT | PNG_HAVE_IEND);

   if (length != 0)
   {
      png_warning(png_ptr, "Incorrect IEND chunk length");
   }

   png_crc_finish(png_ptr, length);

   PNG_UNUSED(info_ptr) 
}

#ifdef PNG_READ_gAMA_SUPPORTED
void 
png_handle_gAMA(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_fixed_point igamma;
   png_byte buf[4];

   png_debug(1, "in png_handle_gAMA");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before gAMA");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid gAMA after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (png_ptr->mode & PNG_HAVE_PLTE)
      
      png_warning(png_ptr, "Out of place gAMA chunk");

   if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_gAMA)
#ifdef PNG_READ_sRGB_SUPPORTED
       && !(info_ptr->valid & PNG_INFO_sRGB)
#endif
       )
   {
      png_warning(png_ptr, "Duplicate gAMA chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   if (length != 4)
   {
      png_warning(png_ptr, "Incorrect gAMA chunk length");
      png_crc_finish(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, 4);

   if (png_crc_finish(png_ptr, 0))
      return;

   igamma = png_get_fixed_point(NULL, buf);

   
   if (igamma <= 0)
   {
      png_warning(png_ptr,
          "Ignoring gAMA chunk with out of range gamma");

      return;
   }

#  ifdef PNG_READ_sRGB_SUPPORTED
   if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_sRGB))
   {
      if (PNG_OUT_OF_RANGE(igamma, 45500, 500))
      {
         PNG_WARNING_PARAMETERS(p)
         png_warning_parameter_signed(p, 1, PNG_NUMBER_FORMAT_fixed, igamma);
         png_formatted_warning(png_ptr, p,
             "Ignoring incorrect gAMA value @1 when sRGB is also present");
         return;
      }
   }
#  endif 

#  ifdef PNG_READ_GAMMA_SUPPORTED
   
   png_ptr->gamma = igamma;
#  endif
   
   png_set_gAMA_fixed(png_ptr, info_ptr, igamma);
}
#endif

#ifdef PNG_READ_sBIT_SUPPORTED
void 
png_handle_sBIT(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_size_t truelen;
   png_byte buf[4];

   png_debug(1, "in png_handle_sBIT");

   buf[0] = buf[1] = buf[2] = buf[3] = 0;

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before sBIT");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid sBIT after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (png_ptr->mode & PNG_HAVE_PLTE)
   {
      
      png_warning(png_ptr, "Out of place sBIT chunk");
   }

   if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_sBIT))
   {
      png_warning(png_ptr, "Duplicate sBIT chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      truelen = 3;

   else
      truelen = (png_size_t)png_ptr->channels;

   if (length != truelen || length > 4)
   {
      png_warning(png_ptr, "Incorrect sBIT chunk length");
      png_crc_finish(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, truelen);

   if (png_crc_finish(png_ptr, 0))
      return;

   if (png_ptr->color_type & PNG_COLOR_MASK_COLOR)
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
png_handle_cHRM(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_byte buf[32];
   png_fixed_point x_white, y_white, x_red, y_red, x_green, y_green, x_blue,
      y_blue;

   png_debug(1, "in png_handle_cHRM");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before cHRM");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid cHRM after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (png_ptr->mode & PNG_HAVE_PLTE)
      
      png_warning(png_ptr, "Out of place cHRM chunk");

   if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_cHRM)
#  ifdef PNG_READ_sRGB_SUPPORTED
       && !(info_ptr->valid & PNG_INFO_sRGB)
#  endif
      )
   {
      png_warning(png_ptr, "Duplicate cHRM chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   if (length != 32)
   {
      png_warning(png_ptr, "Incorrect cHRM chunk length");
      png_crc_finish(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, 32);

   if (png_crc_finish(png_ptr, 0))
      return;

   x_white = png_get_fixed_point(NULL, buf);
   y_white = png_get_fixed_point(NULL, buf + 4);
   x_red   = png_get_fixed_point(NULL, buf + 8);
   y_red   = png_get_fixed_point(NULL, buf + 12);
   x_green = png_get_fixed_point(NULL, buf + 16);
   y_green = png_get_fixed_point(NULL, buf + 20);
   x_blue  = png_get_fixed_point(NULL, buf + 24);
   y_blue  = png_get_fixed_point(NULL, buf + 28);

   if (x_white == PNG_FIXED_ERROR ||
       y_white == PNG_FIXED_ERROR ||
       x_red   == PNG_FIXED_ERROR ||
       y_red   == PNG_FIXED_ERROR ||
       x_green == PNG_FIXED_ERROR ||
       y_green == PNG_FIXED_ERROR ||
       x_blue  == PNG_FIXED_ERROR ||
       y_blue  == PNG_FIXED_ERROR)
   {
      png_warning(png_ptr, "Ignoring cHRM chunk with negative chromaticities");
      return;
   }

#ifdef PNG_READ_sRGB_SUPPORTED
   if ((info_ptr != NULL) && (info_ptr->valid & PNG_INFO_sRGB))
   {
      if (PNG_OUT_OF_RANGE(x_white, 31270,  1000) ||
          PNG_OUT_OF_RANGE(y_white, 32900,  1000) ||
          PNG_OUT_OF_RANGE(x_red,   64000,  1000) ||
          PNG_OUT_OF_RANGE(y_red,   33000,  1000) ||
          PNG_OUT_OF_RANGE(x_green, 30000,  1000) ||
          PNG_OUT_OF_RANGE(y_green, 60000,  1000) ||
          PNG_OUT_OF_RANGE(x_blue,  15000,  1000) ||
          PNG_OUT_OF_RANGE(y_blue,   6000,  1000))
      {
         PNG_WARNING_PARAMETERS(p)

         png_warning_parameter_signed(p, 1, PNG_NUMBER_FORMAT_fixed, x_white);
         png_warning_parameter_signed(p, 2, PNG_NUMBER_FORMAT_fixed, y_white);
         png_warning_parameter_signed(p, 3, PNG_NUMBER_FORMAT_fixed, x_red);
         png_warning_parameter_signed(p, 4, PNG_NUMBER_FORMAT_fixed, y_red);
         png_warning_parameter_signed(p, 5, PNG_NUMBER_FORMAT_fixed, x_green);
         png_warning_parameter_signed(p, 6, PNG_NUMBER_FORMAT_fixed, y_green);
         png_warning_parameter_signed(p, 7, PNG_NUMBER_FORMAT_fixed, x_blue);
         png_warning_parameter_signed(p, 8, PNG_NUMBER_FORMAT_fixed, y_blue);

         png_formatted_warning(png_ptr, p,
             "Ignoring incorrect cHRM white(@1,@2) r(@3,@4)g(@5,@6)b(@7,@8) "
             "when sRGB is also present");
      }
      return;
   }
#endif 

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
   



   if (!png_ptr->rgb_to_gray_coefficients_set)
   {
      


      png_XYZ XYZ;
      png_xy xy;

      xy.redx = x_red;
      xy.redy = y_red;
      xy.greenx = x_green;
      xy.greeny = y_green;
      xy.bluex = x_blue;
      xy.bluey = y_blue;
      xy.whitex = x_white;
      xy.whitey = y_white;

      if (png_XYZ_from_xy_checked(png_ptr, &XYZ, xy))
      {
         



         {
            png_fixed_point r, g, b;
            if (png_muldiv(&r, XYZ.redY, 32768, PNG_FP_1) &&
               r >= 0 && r <= 32768 &&
               png_muldiv(&g, XYZ.greenY, 32768, PNG_FP_1) &&
               g >= 0 && g <= 32768 &&
               png_muldiv(&b, XYZ.blueY, 32768, PNG_FP_1) &&
               b >= 0 && b <= 32768 &&
               r+g+b <= 32769)
            {
               




               int add = 0;

               if (r+g+b > 32768)
                  add = -1;
               else if (r+g+b < 32768)
                  add = 1;

               if (add != 0)
               {
                  if (g >= r && g >= b)
                     g += add;
                  else if (r >= g && r >= b)
                     r += add;
                  else
                     b += add;
               }

               
               if (r+g+b != 32768)
                  png_error(png_ptr,
                     "internal error handling cHRM coefficients");

               png_ptr->rgb_to_gray_red_coeff   = (png_uint_16)r;
               png_ptr->rgb_to_gray_green_coeff = (png_uint_16)g;
            }

            



            else
               png_error(png_ptr, "internal error handling cHRM->XYZ");
         }
      }
   }
#endif

   png_set_cHRM_fixed(png_ptr, info_ptr, x_white, y_white, x_red, y_red,
      x_green, y_green, x_blue, y_blue);
}
#endif

#ifdef PNG_READ_sRGB_SUPPORTED
void 
png_handle_sRGB(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   int intent;
   png_byte buf[1];

   png_debug(1, "in png_handle_sRGB");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before sRGB");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid sRGB after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (png_ptr->mode & PNG_HAVE_PLTE)
      
      png_warning(png_ptr, "Out of place sRGB chunk");

   if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_sRGB))
   {
      png_warning(png_ptr, "Duplicate sRGB chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   if (length != 1)
   {
      png_warning(png_ptr, "Incorrect sRGB chunk length");
      png_crc_finish(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, 1);

   if (png_crc_finish(png_ptr, 0))
      return;

   intent = buf[0];

   
   if (intent >= PNG_sRGB_INTENT_LAST)
   {
      png_warning(png_ptr, "Unknown sRGB intent");
      return;
   }

#if defined(PNG_READ_gAMA_SUPPORTED) && defined(PNG_READ_GAMMA_SUPPORTED)
   if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_gAMA))
   {
      if (PNG_OUT_OF_RANGE(info_ptr->gamma, 45500, 500))
      {
         PNG_WARNING_PARAMETERS(p)

         png_warning_parameter_signed(p, 1, PNG_NUMBER_FORMAT_fixed,
            info_ptr->gamma);

         png_formatted_warning(png_ptr, p,
             "Ignoring incorrect gAMA value @1 when sRGB is also present");
      }
   }
#endif 

#ifdef PNG_READ_cHRM_SUPPORTED
   if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_cHRM))
      if (PNG_OUT_OF_RANGE(info_ptr->x_white, 31270,  1000) ||
          PNG_OUT_OF_RANGE(info_ptr->y_white, 32900,  1000) ||
          PNG_OUT_OF_RANGE(info_ptr->x_red,   64000,  1000) ||
          PNG_OUT_OF_RANGE(info_ptr->y_red,   33000,  1000) ||
          PNG_OUT_OF_RANGE(info_ptr->x_green, 30000,  1000) ||
          PNG_OUT_OF_RANGE(info_ptr->y_green, 60000,  1000) ||
          PNG_OUT_OF_RANGE(info_ptr->x_blue,  15000,  1000) ||
          PNG_OUT_OF_RANGE(info_ptr->y_blue,   6000,  1000))
      {
         png_warning(png_ptr,
             "Ignoring incorrect cHRM value when sRGB is also present");
      }
#endif 

   



   png_ptr->is_sRGB = 1;

#  ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
      
      if (!png_ptr->rgb_to_gray_coefficients_set)
      {
         



















         png_ptr->rgb_to_gray_red_coeff   =  6968; 
         png_ptr->rgb_to_gray_green_coeff = 23434; 
         

         


         png_ptr->rgb_to_gray_coefficients_set = 1;
      }
#  endif

   png_set_sRGB_gAMA_and_cHRM(png_ptr, info_ptr, intent);
}
#endif 

#ifdef PNG_READ_iCCP_SUPPORTED
void 
png_handle_iCCP(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)

{
   png_byte compression_type;
   png_bytep pC;
   png_charp profile;
   png_uint_32 skip = 0;
   png_uint_32 profile_size;
   png_alloc_size_t profile_length;
   png_size_t slength, prefix_length, data_length;

   png_debug(1, "in png_handle_iCCP");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before iCCP");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid iCCP after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (png_ptr->mode & PNG_HAVE_PLTE)
      
      png_warning(png_ptr, "Out of place iCCP chunk");

   if ((png_ptr->mode & PNG_HAVE_iCCP) || (info_ptr != NULL &&
      (info_ptr->valid & (PNG_INFO_iCCP|PNG_INFO_sRGB))))
   {
      png_warning(png_ptr, "Duplicate iCCP chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   png_ptr->mode |= PNG_HAVE_iCCP;

#ifdef PNG_MAX_MALLOC_64K
   if (length > (png_uint_32)65535L)
   {
      png_warning(png_ptr, "iCCP chunk too large to fit in memory");
      skip = length - (png_uint_32)65535L;
      length = (png_uint_32)65535L;
   }
#endif

   png_free(png_ptr, png_ptr->chunkdata);
   png_ptr->chunkdata = (png_charp)png_malloc(png_ptr, length + 1);
   slength = length;
   png_crc_read(png_ptr, (png_bytep)png_ptr->chunkdata, slength);

   if (png_crc_finish(png_ptr, skip))
   {
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   png_ptr->chunkdata[slength] = 0x00;

   for (profile = png_ptr->chunkdata; *profile; profile++)
       ;

   ++profile;

   


   if (profile >= png_ptr->chunkdata + slength - 1)
   {
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      png_warning(png_ptr, "Malformed iCCP chunk");
      return;
   }

   
   compression_type = *profile++;

   if (compression_type)
   {
      png_warning(png_ptr, "Ignoring nonzero compression type in iCCP chunk");
      compression_type = 0x00;  

   }

   prefix_length = profile - png_ptr->chunkdata;
   png_decompress_chunk(png_ptr, compression_type,
       slength, prefix_length, &data_length);

   profile_length = data_length - prefix_length;

   if (prefix_length > data_length || profile_length < 4)
   {
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      png_warning(png_ptr, "Profile size field missing from iCCP chunk");
      return;
   }

   
   pC = (png_bytep)(png_ptr->chunkdata + prefix_length);
   profile_size = ((*(pC    )) << 24) |
                  ((*(pC + 1)) << 16) |
                  ((*(pC + 2)) <<  8) |
                  ((*(pC + 3))      );

   


   if (profile_size < profile_length)
      profile_length = profile_size;

   
   if (profile_size > profile_length)
   {
      PNG_WARNING_PARAMETERS(p)

      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;

      png_warning_parameter_unsigned(p, 1, PNG_NUMBER_FORMAT_u, profile_size);
      png_warning_parameter_unsigned(p, 2, PNG_NUMBER_FORMAT_u, profile_length);
      png_formatted_warning(png_ptr, p,
         "Ignoring iCCP chunk with declared size = @1 and actual length = @2");
      return;
   }

   png_set_iCCP(png_ptr, info_ptr, png_ptr->chunkdata,
       compression_type, (png_bytep)png_ptr->chunkdata + prefix_length,
       profile_size);
   png_free(png_ptr, png_ptr->chunkdata);
   png_ptr->chunkdata = NULL;
}
#endif 

#ifdef PNG_READ_sPLT_SUPPORTED
void 
png_handle_sPLT(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)

{
   png_bytep entry_start;
   png_sPLT_t new_palette;
   png_sPLT_entryp pp;
   png_uint_32 data_length;
   int entry_size, i;
   png_uint_32 skip = 0;
   png_size_t slength;
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

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before sPLT");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid sPLT after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

#ifdef PNG_MAX_MALLOC_64K
   if (length > (png_uint_32)65535L)
   {
      png_warning(png_ptr, "sPLT chunk too large to fit in memory");
      skip = length - (png_uint_32)65535L;
      length = (png_uint_32)65535L;
   }
#endif

   png_free(png_ptr, png_ptr->chunkdata);
   png_ptr->chunkdata = (png_charp)png_malloc(png_ptr, length + 1);

   



   slength = length;
   png_crc_read(png_ptr, (png_bytep)png_ptr->chunkdata, slength);

   if (png_crc_finish(png_ptr, skip))
   {
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   png_ptr->chunkdata[slength] = 0x00;

   for (entry_start = (png_bytep)png_ptr->chunkdata; *entry_start;
       entry_start++)
       ;

   ++entry_start;

   
   if (entry_start > (png_bytep)png_ptr->chunkdata + slength - 2)
   {
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      png_warning(png_ptr, "malformed sPLT chunk");
      return;
   }

   new_palette.depth = *entry_start++;
   entry_size = (new_palette.depth == 8 ? 6 : 10);
   



   data_length = length - (png_uint_32)(entry_start -
      (png_bytep)png_ptr->chunkdata);

   
   if (data_length % entry_size)
   {
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      png_warning(png_ptr, "sPLT chunk has bad length");
      return;
   }

   dl = (png_int_32)(data_length / entry_size);
   max_dl = PNG_SIZE_MAX / png_sizeof(png_sPLT_entry);

   if (dl > max_dl)
   {
       png_warning(png_ptr, "sPLT chunk too long");
       return;
   }

   new_palette.nentries = (png_int_32)(data_length / entry_size);

   new_palette.entries = (png_sPLT_entryp)png_malloc_warn(
       png_ptr, new_palette.nentries * png_sizeof(png_sPLT_entry));

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

   
   new_palette.name = png_ptr->chunkdata;

   png_set_sPLT(png_ptr, info_ptr, &new_palette, 1);

   png_free(png_ptr, png_ptr->chunkdata);
   png_ptr->chunkdata = NULL;
   png_free(png_ptr, new_palette.entries);
}
#endif 

#ifdef PNG_READ_tRNS_SUPPORTED
void 
png_handle_tRNS(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_byte readbuf[PNG_MAX_PALETTE_LENGTH];

   png_debug(1, "in png_handle_tRNS");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before tRNS");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid tRNS after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_tRNS))
   {
      png_warning(png_ptr, "Duplicate tRNS chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   if (png_ptr->color_type == PNG_COLOR_TYPE_GRAY)
   {
      png_byte buf[2];

      if (length != 2)
      {
         png_warning(png_ptr, "Incorrect tRNS chunk length");
         png_crc_finish(png_ptr, length);
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
         png_warning(png_ptr, "Incorrect tRNS chunk length");
         png_crc_finish(png_ptr, length);
         return;
      }

      png_crc_read(png_ptr, buf, (png_size_t)length);
      png_ptr->num_trans = 1;
      png_ptr->trans_color.red = png_get_uint_16(buf);
      png_ptr->trans_color.green = png_get_uint_16(buf + 2);
      png_ptr->trans_color.blue = png_get_uint_16(buf + 4);
   }

   else if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (!(png_ptr->mode & PNG_HAVE_PLTE))
      {
         
         png_warning(png_ptr, "Missing PLTE before tRNS");
      }

      if (length > (png_uint_32)png_ptr->num_palette ||
          length > PNG_MAX_PALETTE_LENGTH)
      {
         png_warning(png_ptr, "Incorrect tRNS chunk length");
         png_crc_finish(png_ptr, length);
         return;
      }

      if (length == 0)
      {
         png_warning(png_ptr, "Zero length tRNS chunk");
         png_crc_finish(png_ptr, length);
         return;
      }

      png_crc_read(png_ptr, readbuf, (png_size_t)length);
      png_ptr->num_trans = (png_uint_16)length;
   }

   else
   {
      png_warning(png_ptr, "tRNS chunk not allowed with alpha channel");
      png_crc_finish(png_ptr, length);
      return;
   }

   if (png_crc_finish(png_ptr, 0))
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
png_handle_bKGD(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_size_t truelen;
   png_byte buf[6];
   png_color_16 background;

   png_debug(1, "in png_handle_bKGD");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before bKGD");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid bKGD after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
       !(png_ptr->mode & PNG_HAVE_PLTE))
   {
      png_warning(png_ptr, "Missing PLTE before bKGD");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_bKGD))
   {
      png_warning(png_ptr, "Duplicate bKGD chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      truelen = 1;

   else if (png_ptr->color_type & PNG_COLOR_MASK_COLOR)
      truelen = 6;

   else
      truelen = 2;

   if (length != truelen)
   {
      png_warning(png_ptr, "Incorrect bKGD chunk length");
      png_crc_finish(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, truelen);

   if (png_crc_finish(png_ptr, 0))
      return;

   




   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      background.index = buf[0];

      if (info_ptr && info_ptr->num_palette)
      {
         if (buf[0] >= info_ptr->num_palette)
         {
            png_warning(png_ptr, "Incorrect bKGD chunk index value");
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

   else if (!(png_ptr->color_type & PNG_COLOR_MASK_COLOR)) 
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
png_handle_hIST(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   unsigned int num, i;
   png_uint_16 readbuf[PNG_MAX_PALETTE_LENGTH];

   png_debug(1, "in png_handle_hIST");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before hIST");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid hIST after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (!(png_ptr->mode & PNG_HAVE_PLTE))
   {
      png_warning(png_ptr, "Missing PLTE before hIST");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_hIST))
   {
      png_warning(png_ptr, "Duplicate hIST chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   if (length > 2*PNG_MAX_PALETTE_LENGTH ||
       length != (unsigned int) (2*png_ptr->num_palette))
   {
      png_warning(png_ptr, "Incorrect hIST chunk length");
      png_crc_finish(png_ptr, length);
      return;
   }

   num = length / 2 ;

   for (i = 0; i < num; i++)
   {
      png_byte buf[2];

      png_crc_read(png_ptr, buf, 2);
      readbuf[i] = png_get_uint_16(buf);
   }

   if (png_crc_finish(png_ptr, 0))
      return;

   png_set_hIST(png_ptr, info_ptr, readbuf);
}
#endif

#ifdef PNG_READ_pHYs_SUPPORTED
void 
png_handle_pHYs(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_byte buf[9];
   png_uint_32 res_x, res_y;
   int unit_type;

   png_debug(1, "in png_handle_pHYs");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before pHYs");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid pHYs after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_pHYs))
   {
      png_warning(png_ptr, "Duplicate pHYs chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   if (length != 9)
   {
      png_warning(png_ptr, "Incorrect pHYs chunk length");
      png_crc_finish(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, 9);

   if (png_crc_finish(png_ptr, 0))
      return;

   res_x = png_get_uint_32(buf);
   res_y = png_get_uint_32(buf + 4);
   unit_type = buf[8];
   png_set_pHYs(png_ptr, info_ptr, res_x, res_y, unit_type);
}
#endif

#ifdef PNG_READ_oFFs_SUPPORTED
void 
png_handle_oFFs(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_byte buf[9];
   png_int_32 offset_x, offset_y;
   int unit_type;

   png_debug(1, "in png_handle_oFFs");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before oFFs");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid oFFs after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_oFFs))
   {
      png_warning(png_ptr, "Duplicate oFFs chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   if (length != 9)
   {
      png_warning(png_ptr, "Incorrect oFFs chunk length");
      png_crc_finish(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, 9);

   if (png_crc_finish(png_ptr, 0))
      return;

   offset_x = png_get_int_32(buf);
   offset_y = png_get_int_32(buf + 4);
   unit_type = buf[8];
   png_set_oFFs(png_ptr, info_ptr, offset_x, offset_y, unit_type);
}
#endif

#ifdef PNG_READ_pCAL_SUPPORTED

void 
png_handle_pCAL(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_int_32 X0, X1;
   png_byte type, nparams;
   png_charp buf, units, endptr;
   png_charpp params;
   png_size_t slength;
   int i;

   png_debug(1, "in png_handle_pCAL");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before pCAL");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid pCAL after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_pCAL))
   {
      png_warning(png_ptr, "Duplicate pCAL chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   png_debug1(2, "Allocating and reading pCAL chunk data (%u bytes)",
       length + 1);
   png_free(png_ptr, png_ptr->chunkdata);
   png_ptr->chunkdata = (png_charp)png_malloc_warn(png_ptr, length + 1);

   if (png_ptr->chunkdata == NULL)
   {
      png_warning(png_ptr, "No memory for pCAL purpose");
      return;
   }

   slength = length;
   png_crc_read(png_ptr, (png_bytep)png_ptr->chunkdata, slength);

   if (png_crc_finish(png_ptr, 0))
   {
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   png_ptr->chunkdata[slength] = 0x00; 

   png_debug(3, "Finding end of pCAL purpose string");
   for (buf = png_ptr->chunkdata; *buf; buf++)
       ;

   endptr = png_ptr->chunkdata + slength;

   


   if (endptr <= buf + 12)
   {
      png_warning(png_ptr, "Invalid pCAL data");
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
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
      png_warning(png_ptr, "Invalid pCAL parameters for equation type");
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   else if (type >= PNG_EQUATION_LAST)
   {
      png_warning(png_ptr, "Unrecognized equation type for pCAL chunk");
   }

   for (buf = units; *buf; buf++)
       ;

   png_debug(3, "Allocating pCAL parameters array");

   params = (png_charpp)png_malloc_warn(png_ptr,
       (png_size_t)(nparams * png_sizeof(png_charp)));

   if (params == NULL)
   {
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      png_warning(png_ptr, "No memory for pCAL params");
      return;
   }

   
   for (i = 0; i < (int)nparams; i++)
   {
      buf++; 

      png_debug1(3, "Reading pCAL parameter %d", i);

      for (params[i] = buf; buf <= endptr && *buf != 0x00; buf++)
          ;

      
      if (buf > endptr)
      {
         png_warning(png_ptr, "Invalid pCAL data");
         png_free(png_ptr, png_ptr->chunkdata);
         png_ptr->chunkdata = NULL;
         png_free(png_ptr, params);
         return;
      }
   }

   png_set_pCAL(png_ptr, info_ptr, png_ptr->chunkdata, X0, X1, type, nparams,
      units, params);

   png_free(png_ptr, png_ptr->chunkdata);
   png_ptr->chunkdata = NULL;
   png_free(png_ptr, params);
}
#endif

#ifdef PNG_READ_sCAL_SUPPORTED

void 
png_handle_sCAL(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_size_t slength, i;
   int state;

   png_debug(1, "in png_handle_sCAL");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before sCAL");

   else if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      png_warning(png_ptr, "Invalid sCAL after IDAT");
      png_crc_finish(png_ptr, length);
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_sCAL))
   {
      png_warning(png_ptr, "Duplicate sCAL chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   
   else if (length < 4)
   {
      png_warning(png_ptr, "sCAL chunk too short");
      png_crc_finish(png_ptr, length);
      return;
   }

   png_debug1(2, "Allocating and reading sCAL chunk data (%u bytes)",
      length + 1);

   png_ptr->chunkdata = (png_charp)png_malloc_warn(png_ptr, length + 1);

   if (png_ptr->chunkdata == NULL)
   {
      png_warning(png_ptr, "Out of memory while processing sCAL chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   slength = length;
   png_crc_read(png_ptr, (png_bytep)png_ptr->chunkdata, slength);
   png_ptr->chunkdata[slength] = 0x00; 

   if (png_crc_finish(png_ptr, 0))
   {
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   
   if (png_ptr->chunkdata[0] != 1 && png_ptr->chunkdata[0] != 2)
   {
      png_warning(png_ptr, "Invalid sCAL ignored: invalid unit");
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   


   i = 1;
   state = 0;

   if (!png_check_fp_number(png_ptr->chunkdata, slength, &state, &i) ||
       i >= slength || png_ptr->chunkdata[i++] != 0)
      png_warning(png_ptr, "Invalid sCAL chunk ignored: bad width format");

   else if (!PNG_FP_IS_POSITIVE(state))
      png_warning(png_ptr, "Invalid sCAL chunk ignored: non-positive width");

   else
   {
      png_size_t heighti = i;

      state = 0;
      if (!png_check_fp_number(png_ptr->chunkdata, slength, &state, &i) ||
          i != slength)
         png_warning(png_ptr, "Invalid sCAL chunk ignored: bad height format");

      else if (!PNG_FP_IS_POSITIVE(state))
         png_warning(png_ptr,
            "Invalid sCAL chunk ignored: non-positive height");

      else
         
         png_set_sCAL_s(png_ptr, info_ptr, png_ptr->chunkdata[0],
            png_ptr->chunkdata+1, png_ptr->chunkdata+heighti);
   }

   
   png_free(png_ptr, png_ptr->chunkdata);
   png_ptr->chunkdata = NULL;
}
#endif

#ifdef PNG_READ_tIME_SUPPORTED
void 
png_handle_tIME(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_byte buf[7];
   png_time mod_time;

   png_debug(1, "in png_handle_tIME");

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Out of place tIME chunk");

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_tIME))
   {
      png_warning(png_ptr, "Duplicate tIME chunk");
      png_crc_finish(png_ptr, length);
      return;
   }

   if (png_ptr->mode & PNG_HAVE_IDAT)
      png_ptr->mode |= PNG_AFTER_IDAT;

   if (length != 7)
   {
      png_warning(png_ptr, "Incorrect tIME chunk length");
      png_crc_finish(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, 7);

   if (png_crc_finish(png_ptr, 0))
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
png_handle_tEXt(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_textp text_ptr;
   png_charp key;
   png_charp text;
   png_uint_32 skip = 0;
   png_size_t slength;
   int ret;

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
         png_warning(png_ptr, "No space in chunk cache for tEXt");
         png_crc_finish(png_ptr, length);
         return;
      }
   }
#endif

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before tEXt");

   if (png_ptr->mode & PNG_HAVE_IDAT)
      png_ptr->mode |= PNG_AFTER_IDAT;

#ifdef PNG_MAX_MALLOC_64K
   if (length > (png_uint_32)65535L)
   {
      png_warning(png_ptr, "tEXt chunk too large to fit in memory");
      skip = length - (png_uint_32)65535L;
      length = (png_uint_32)65535L;
   }
#endif

   png_free(png_ptr, png_ptr->chunkdata);

   png_ptr->chunkdata = (png_charp)png_malloc_warn(png_ptr, length + 1);

   if (png_ptr->chunkdata == NULL)
   {
     png_warning(png_ptr, "No memory to process text chunk");
     return;
   }

   slength = length;
   png_crc_read(png_ptr, (png_bytep)png_ptr->chunkdata, slength);

   if (png_crc_finish(png_ptr, skip))
   {
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   key = png_ptr->chunkdata;

   key[slength] = 0x00;

   for (text = key; *text; text++)
       ;

   if (text != key + slength)
      text++;

   text_ptr = (png_textp)png_malloc_warn(png_ptr,
       png_sizeof(png_text));

   if (text_ptr == NULL)
   {
      png_warning(png_ptr, "Not enough memory to process text chunk");
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   text_ptr->compression = PNG_TEXT_COMPRESSION_NONE;
   text_ptr->key = key;
   text_ptr->lang = NULL;
   text_ptr->lang_key = NULL;
   text_ptr->itxt_length = 0;
   text_ptr->text = text;
   text_ptr->text_length = png_strlen(text);

   ret = png_set_text_2(png_ptr, info_ptr, text_ptr, 1);

   png_free(png_ptr, png_ptr->chunkdata);
   png_ptr->chunkdata = NULL;
   png_free(png_ptr, text_ptr);

   if (ret)
      png_warning(png_ptr, "Insufficient memory to process text chunk");
}
#endif

#ifdef PNG_READ_zTXt_SUPPORTED

void 
png_handle_zTXt(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_textp text_ptr;
   png_charp text;
   int comp_type;
   int ret;
   png_size_t slength, prefix_len, data_len;

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
         png_warning(png_ptr, "No space in chunk cache for zTXt");
         png_crc_finish(png_ptr, length);
         return;
      }
   }
#endif

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before zTXt");

   if (png_ptr->mode & PNG_HAVE_IDAT)
      png_ptr->mode |= PNG_AFTER_IDAT;

#ifdef PNG_MAX_MALLOC_64K
   


   if (length > (png_uint_32)65535L)
   {
      png_warning(png_ptr, "zTXt chunk too large to fit in memory");
      png_crc_finish(png_ptr, length);
      return;
   }
#endif

   png_free(png_ptr, png_ptr->chunkdata);
   png_ptr->chunkdata = (png_charp)png_malloc_warn(png_ptr, length + 1);

   if (png_ptr->chunkdata == NULL)
   {
      png_warning(png_ptr, "Out of memory processing zTXt chunk");
      return;
   }

   slength = length;
   png_crc_read(png_ptr, (png_bytep)png_ptr->chunkdata, slength);

   if (png_crc_finish(png_ptr, 0))
   {
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   png_ptr->chunkdata[slength] = 0x00;

   for (text = png_ptr->chunkdata; *text; text++)
       ;

   
   if (text >= png_ptr->chunkdata + slength - 2)
   {
      png_warning(png_ptr, "Truncated zTXt chunk");
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   else
   {
       comp_type = *(++text);

       if (comp_type != PNG_TEXT_COMPRESSION_zTXt)
       {
          png_warning(png_ptr, "Unknown compression type in zTXt chunk");
          comp_type = PNG_TEXT_COMPRESSION_zTXt;
       }

       text++;        
   }

   prefix_len = text - png_ptr->chunkdata;

   png_decompress_chunk(png_ptr, comp_type,
       (png_size_t)length, prefix_len, &data_len);

   text_ptr = (png_textp)png_malloc_warn(png_ptr,
       png_sizeof(png_text));

   if (text_ptr == NULL)
   {
      png_warning(png_ptr, "Not enough memory to process zTXt chunk");
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   text_ptr->compression = comp_type;
   text_ptr->key = png_ptr->chunkdata;
   text_ptr->lang = NULL;
   text_ptr->lang_key = NULL;
   text_ptr->itxt_length = 0;
   text_ptr->text = png_ptr->chunkdata + prefix_len;
   text_ptr->text_length = data_len;

   ret = png_set_text_2(png_ptr, info_ptr, text_ptr, 1);

   png_free(png_ptr, text_ptr);
   png_free(png_ptr, png_ptr->chunkdata);
   png_ptr->chunkdata = NULL;

   if (ret)
      png_error(png_ptr, "Insufficient memory to store zTXt chunk");
}
#endif

#ifdef PNG_READ_iTXt_SUPPORTED

void 
png_handle_iTXt(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_textp text_ptr;
   png_charp key, lang, text, lang_key;
   int comp_flag;
   int comp_type;
   int ret;
   png_size_t slength, prefix_len, data_len;

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
         png_warning(png_ptr, "No space in chunk cache for iTXt");
         png_crc_finish(png_ptr, length);
         return;
      }
   }
#endif

   if (!(png_ptr->mode & PNG_HAVE_IHDR))
      png_error(png_ptr, "Missing IHDR before iTXt");

   if (png_ptr->mode & PNG_HAVE_IDAT)
      png_ptr->mode |= PNG_AFTER_IDAT;

#ifdef PNG_MAX_MALLOC_64K
   


   if (length > (png_uint_32)65535L)
   {
      png_warning(png_ptr, "iTXt chunk too large to fit in memory");
      png_crc_finish(png_ptr, length);
      return;
   }
#endif

   png_free(png_ptr, png_ptr->chunkdata);
   png_ptr->chunkdata = (png_charp)png_malloc_warn(png_ptr, length + 1);

   if (png_ptr->chunkdata == NULL)
   {
      png_warning(png_ptr, "No memory to process iTXt chunk");
      return;
   }

   slength = length;
   png_crc_read(png_ptr, (png_bytep)png_ptr->chunkdata, slength);

   if (png_crc_finish(png_ptr, 0))
   {
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   png_ptr->chunkdata[slength] = 0x00;

   for (lang = png_ptr->chunkdata; *lang; lang++)
       ;

   lang++;        

   




   if (lang >= png_ptr->chunkdata + slength - 3)
   {
      png_warning(png_ptr, "Truncated iTXt chunk");
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   comp_flag = *lang++;
   comp_type = *lang++;

   



   if (comp_flag != 0 && comp_flag != 1)
   {
      png_warning(png_ptr, "invalid iTXt compression flag");
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   if (comp_flag && comp_type != 0)
   {
      png_warning(png_ptr, "unknown iTXt compression type");
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   for (lang_key = lang; *lang_key; lang_key++)
       ;

   lang_key++;        

   if (lang_key >= png_ptr->chunkdata + slength)
   {
      png_warning(png_ptr, "Truncated iTXt chunk");
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   for (text = lang_key; *text; text++)
       ;

   text++;        

   if (text >= png_ptr->chunkdata + slength)
   {
      png_warning(png_ptr, "Malformed iTXt chunk");
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   prefix_len = text - png_ptr->chunkdata;

   key=png_ptr->chunkdata;

   if (comp_flag)
      png_decompress_chunk(png_ptr, comp_type,
          (size_t)length, prefix_len, &data_len);

   else
      data_len = png_strlen(png_ptr->chunkdata + prefix_len);

   text_ptr = (png_textp)png_malloc_warn(png_ptr,
       png_sizeof(png_text));

   if (text_ptr == NULL)
   {
      png_warning(png_ptr, "Not enough memory to process iTXt chunk");
      png_free(png_ptr, png_ptr->chunkdata);
      png_ptr->chunkdata = NULL;
      return;
   }

   text_ptr->compression =
      (comp_flag ? PNG_ITXT_COMPRESSION_zTXt : PNG_ITXT_COMPRESSION_NONE);
   text_ptr->lang_key = png_ptr->chunkdata + (lang_key - key);
   text_ptr->lang = png_ptr->chunkdata + (lang - key);
   text_ptr->itxt_length = data_len;
   text_ptr->text_length = 0;
   text_ptr->key = png_ptr->chunkdata;
   text_ptr->text = png_ptr->chunkdata + prefix_len;

   ret = png_set_text_2(png_ptr, info_ptr, text_ptr, 1);

   png_free(png_ptr, text_ptr);
   png_free(png_ptr, png_ptr->chunkdata);
   png_ptr->chunkdata = NULL;

   if (ret)
      png_error(png_ptr, "Insufficient memory to store iTXt chunk");
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

    if (!(png_ptr->mode & PNG_HAVE_IHDR))
    {
        png_error(png_ptr, "Missing IHDR before acTL");
    }
    else if (png_ptr->mode & PNG_HAVE_IDAT)
    {
        png_warning(png_ptr, "Invalid acTL after IDAT skipped");
        png_crc_finish(png_ptr, length);
        return;
    }
    else if (png_ptr->mode & PNG_HAVE_acTL)
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
    if(didSet)
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

    if (!(png_ptr->mode & PNG_HAVE_IHDR))
    {
        png_error(png_ptr, "Missing IHDR before fcTL");
    }
    else if (png_ptr->mode & PNG_HAVE_IDAT)
    {
        


        png_warning(png_ptr, "Invalid fcTL after IDAT skipped");
        png_crc_finish(png_ptr, length-4);
        return;
    }
    else if (png_ptr->mode & PNG_HAVE_fcTL)
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
    if((info_ptr->valid & PNG_INFO_acTL) && !(info_ptr->valid & PNG_INFO_fcTL))
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







void 
png_handle_unknown(png_structp png_ptr, png_infop info_ptr, png_uint_32 length)
{
   png_uint_32 skip = 0;

   png_debug(1, "in png_handle_unknown");

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
         png_warning(png_ptr, "No space in chunk cache for unknown chunk");
         png_crc_finish(png_ptr, length);
         return;
      }
   }
#endif

   if (png_ptr->mode & PNG_HAVE_IDAT)
   {
      if (png_ptr->chunk_name != png_IDAT)
         png_ptr->mode |= PNG_AFTER_IDAT;
   }

   if (PNG_CHUNK_CRITICAL(png_ptr->chunk_name))
   {
#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
      if (png_chunk_unknown_handling(png_ptr, png_ptr->chunk_name) !=
          PNG_HANDLE_CHUNK_ALWAYS
#ifdef PNG_READ_USER_CHUNKS_SUPPORTED
          && png_ptr->read_user_chunk_fn == NULL
#endif
          )
#endif
         png_chunk_error(png_ptr, "unknown critical chunk");
   }

#ifdef PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
   if ((png_ptr->flags & PNG_FLAG_KEEP_UNKNOWN_CHUNKS)
#ifdef PNG_READ_USER_CHUNKS_SUPPORTED
       || (png_ptr->read_user_chunk_fn != NULL)
#endif
       )
   {
#ifdef PNG_MAX_MALLOC_64K
      if (length > 65535)
      {
         png_warning(png_ptr, "unknown chunk too large to fit in memory");
         skip = length - 65535;
         length = 65535;
      }
#endif

      




      PNG_CSTRING_FROM_CHUNK(png_ptr->unknown_chunk.name, png_ptr->chunk_name);
      png_ptr->unknown_chunk.size = (png_size_t)length;

      if (length == 0)
         png_ptr->unknown_chunk.data = NULL;

      else
      {
         png_ptr->unknown_chunk.data = (png_bytep)png_malloc(png_ptr, length);
         png_crc_read(png_ptr, png_ptr->unknown_chunk.data, length);
      }

#ifdef PNG_READ_USER_CHUNKS_SUPPORTED
      if (png_ptr->read_user_chunk_fn != NULL)
      {
         
         int ret;

         ret = (*(png_ptr->read_user_chunk_fn))
             (png_ptr, &png_ptr->unknown_chunk);

         if (ret < 0)
            png_chunk_error(png_ptr, "error in user chunk");

         if (ret == 0)
         {
            if (PNG_CHUNK_CRITICAL(png_ptr->chunk_name))
            {
#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
               if (png_chunk_unknown_handling(png_ptr, png_ptr->chunk_name) !=
                   PNG_HANDLE_CHUNK_ALWAYS)
#endif
                  png_chunk_error(png_ptr, "unknown critical chunk");
            }

            png_set_unknown_chunks(png_ptr, info_ptr,
                &png_ptr->unknown_chunk, 1);
         }
      }

      else
#endif
         png_set_unknown_chunks(png_ptr, info_ptr, &png_ptr->unknown_chunk, 1);

      png_free(png_ptr, png_ptr->unknown_chunk.data);
      png_ptr->unknown_chunk.data = NULL;
   }

   else
#endif
      skip = length;

   png_crc_finish(png_ptr, skip);

#ifndef PNG_READ_USER_CHUNKS_SUPPORTED
   PNG_UNUSED(info_ptr) 
#endif
}













void 
png_check_chunk_name(png_structp png_ptr, png_uint_32 chunk_name)
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
png_combine_row(png_structp png_ptr, png_bytep dp, int display)
{
   unsigned int pixel_depth = png_ptr->transformed_pixel_depth;
   png_const_bytep sp = png_ptr->row_buf + 1;
   png_uint_32 row_width = png_ptr->width;
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
         if (png_ptr->transformations & PNG_PACKSWAP) 
            end_mask = 0xff << end_mask;

         else 
#     endif
         end_mask = 0xff >> end_mask;
      
   }

   





#ifdef PNG_READ_INTERLACING_SUPPORTED
   if (png_ptr->interlaced && (png_ptr->transformations & PNG_INTERLACE) &&
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

#        define B_MASKS(d,s) { B_MASK(1,d,s), S_MASK(3,d,s), S_MASK(5,d,s) }

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
            if (png_ptr->transformations & PNG_PACKSWAP)
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

         
         if (display)
         {
            



            bytes_to_copy = (1<<((6-pass)>>1)) * pixel_depth;

            
            if (bytes_to_copy > row_width)
               bytes_to_copy = row_width;
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
               


               for(;;)
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
                  bytes_to_copy % sizeof (png_uint_16) == 0 &&
                  bytes_to_jump % sizeof (png_uint_16) == 0)
               {
                  


                  if (png_isaligned(dp, png_uint_32) &&
                     png_isaligned(sp, png_uint_32) &&
                     bytes_to_copy % sizeof (png_uint_32) == 0 &&
                     bytes_to_jump % sizeof (png_uint_32) == 0)
                  {
                     png_uint_32p dp32 = (png_uint_32p)dp;
                     png_const_uint_32p sp32 = (png_const_uint_32p)sp;
                     unsigned int skip = (bytes_to_jump-bytes_to_copy) /
                        sizeof (png_uint_32);

                     do
                     {
                        size_t c = bytes_to_copy;
                        do
                        {
                           *dp32++ = *sp32++;
                           c -= sizeof (png_uint_32);
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
                     png_uint_16p dp16 = (png_uint_16p)dp;
                     png_const_uint_16p sp16 = (png_const_uint_16p)sp;
                     unsigned int skip = (bytes_to_jump-bytes_to_copy) /
                        sizeof (png_uint_16);

                     do
                     {
                        size_t c = bytes_to_copy;
                        do
                        {
                           *dp16++ = *sp16++;
                           c -= sizeof (png_uint_16);
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
                  png_memcpy(dp, sp, bytes_to_copy);

                  if (row_width <= bytes_to_jump)
                     return;

                  sp += bytes_to_jump;
                  dp += bytes_to_jump;
                  row_width -= bytes_to_jump;
                  if (bytes_to_copy > row_width)
                     bytes_to_copy = row_width;
               }
         }

         
      } 

      
   }
   else
#endif

   



   png_memcpy(dp, sp, PNG_ROWBYTES(pixel_depth, row_width));

   
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
            if (transformations & PNG_PACKSWAP)
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
                  *dp &= (png_byte)((0x7f7f >> (7 - dshift)) & 0xff);
                  *dp |= (png_byte)(v << dshift);

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
            if (transformations & PNG_PACKSWAP)
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
                  *dp &= (png_byte)((0x3f3f >> (6 - dshift)) & 0xff);
                  *dp |= (png_byte)(v << dshift);

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
            if (transformations & PNG_PACKSWAP)
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
                  *dp &= (png_byte)((0xf0f >> (4 - dshift)) & 0xff);
                  *dp |= (png_byte)(v << dshift);

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

               png_memcpy(v, sp, pixel_bytes);

               for (j = 0; j < jstop; j++)
               {
                  png_memcpy(dp, v, pixel_bytes);
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

      c = b;
      a += *row;
      *row++ = (png_byte)a;
   }
}

static void
png_init_filter_functions(png_structp pp)
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
png_read_filter_row(png_structp pp, png_row_infop row_info, png_bytep row,
   png_const_bytep prev_row, int filter)
{
   if (pp->read_filter[0] == NULL)
      png_init_filter_functions(pp);
   if (filter > PNG_FILTER_VALUE_NONE && filter < PNG_FILTER_VALUE_LAST)
      pp->read_filter[filter-1](row_info, row, prev_row);
}

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED
void 
png_read_finish_row(png_structp png_ptr)
{
#ifdef PNG_READ_INTERLACING_SUPPORTED
   

   
   static PNG_CONST png_byte png_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};

   
   static PNG_CONST png_byte png_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};

   
   static PNG_CONST png_byte png_pass_ystart[7] = {0, 0, 4, 0, 2, 0, 1};

   
   static PNG_CONST png_byte png_pass_yinc[7] = {8, 8, 8, 4, 4, 2, 2};
#endif 

   png_debug(1, "in png_read_finish_row");
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

         if (png_ptr->pass >= 7)
            break;

         png_ptr->iwidth = (png_ptr->width +
            png_pass_inc[png_ptr->pass] - 1 -
            png_pass_start[png_ptr->pass]) /
            png_pass_inc[png_ptr->pass];

         if (!(png_ptr->transformations & PNG_INTERLACE))
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
#endif 

   if (!(png_ptr->flags & PNG_FLAG_ZLIB_FINISHED))
   {
      char extra;
      int ret;

      png_ptr->zstream.next_out = (Byte *)&extra;
      png_ptr->zstream.avail_out = (uInt)1;

      for (;;)
      {
         if (!(png_ptr->zstream.avail_in))
         {
            while (!png_ptr->idat_size)
            {
               png_crc_finish(png_ptr, 0);
               png_ptr->idat_size = png_read_chunk_header(png_ptr);
               if (png_ptr->chunk_name != png_IDAT)
                  png_error(png_ptr, "Not enough image data");
            }

            png_ptr->zstream.avail_in = (uInt)png_ptr->zbuf_size;
            png_ptr->zstream.next_in = png_ptr->zbuf;

            if (png_ptr->zbuf_size > png_ptr->idat_size)
               png_ptr->zstream.avail_in = (uInt)png_ptr->idat_size;

            png_crc_read(png_ptr, png_ptr->zbuf, png_ptr->zstream.avail_in);
            png_ptr->idat_size -= png_ptr->zstream.avail_in;
         }

         ret = inflate(&png_ptr->zstream, Z_PARTIAL_FLUSH);

         if (ret == Z_STREAM_END)
         {
            if (!(png_ptr->zstream.avail_out) || png_ptr->zstream.avail_in ||
                png_ptr->idat_size)
               png_warning(png_ptr, "Extra compressed data");

            png_ptr->mode |= PNG_AFTER_IDAT;
            png_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;
            break;
         }

         if (ret != Z_OK)
            png_error(png_ptr, png_ptr->zstream.msg ? png_ptr->zstream.msg :
                "Decompression Error");

         if (!(png_ptr->zstream.avail_out))
         {
            png_warning(png_ptr, "Extra compressed data");
            png_ptr->mode |= PNG_AFTER_IDAT;
            png_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;
            break;
         }

      }
      png_ptr->zstream.avail_out = 0;
   }

   if (png_ptr->idat_size || png_ptr->zstream.avail_in)
      png_warning(png_ptr, "Extra compression data");

   inflateReset(&png_ptr->zstream);

   png_ptr->mode |= PNG_AFTER_IDAT;
}
#endif 

void 
png_read_start_row(png_structp png_ptr)
{
#ifdef PNG_READ_INTERLACING_SUPPORTED
   

   
   static PNG_CONST png_byte png_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};

   
   static PNG_CONST png_byte png_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};

   
   static PNG_CONST png_byte png_pass_ystart[7] = {0, 0, 4, 0, 2, 0, 1};

   
   static PNG_CONST png_byte png_pass_yinc[7] = {8, 8, 8, 4, 4, 2, 2};
#endif

   int max_pixel_depth;
   png_size_t row_bytes;

   png_debug(1, "in png_read_start_row");
   png_ptr->zstream.avail_in = 0;
#ifdef PNG_READ_TRANSFORMS_SUPPORTED
   png_init_read_transformations(png_ptr);
#endif
#ifdef PNG_READ_INTERLACING_SUPPORTED
   if (png_ptr->interlaced)
   {
      if (!(png_ptr->transformations & PNG_INTERLACE))
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

   max_pixel_depth = png_ptr->pixel_depth;

   









#ifdef PNG_READ_PACK_SUPPORTED
   if ((png_ptr->transformations & PNG_PACK) && png_ptr->bit_depth < 8)
      max_pixel_depth = 8;
#endif

#ifdef PNG_READ_EXPAND_SUPPORTED
   if (png_ptr->transformations & PNG_EXPAND)
   {
      if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         if (png_ptr->num_trans)
            max_pixel_depth = 32;

         else
            max_pixel_depth = 24;
      }

      else if (png_ptr->color_type == PNG_COLOR_TYPE_GRAY)
      {
         if (max_pixel_depth < 8)
            max_pixel_depth = 8;

         if (png_ptr->num_trans)
            max_pixel_depth *= 2;
      }

      else if (png_ptr->color_type == PNG_COLOR_TYPE_RGB)
      {
         if (png_ptr->num_trans)
         {
            max_pixel_depth *= 4;
            max_pixel_depth /= 3;
         }
      }
   }
#endif

#ifdef PNG_READ_EXPAND_16_SUPPORTED
   if (png_ptr->transformations & PNG_EXPAND_16)
   {
#     ifdef PNG_READ_EXPAND_SUPPORTED
         


         if (png_ptr->transformations & PNG_EXPAND)
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
   if (png_ptr->transformations & (PNG_FILLER))
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
   if (png_ptr->transformations & PNG_GRAY_TO_RGB)
   {
      if (
#ifdef PNG_READ_EXPAND_SUPPORTED
          (png_ptr->num_trans && (png_ptr->transformations & PNG_EXPAND)) ||
#endif
#ifdef PNG_READ_FILLER_SUPPORTED
          (png_ptr->transformations & (PNG_FILLER)) ||
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
   if (png_ptr->transformations & PNG_USER_TRANSFORM)
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

     if (png_ptr->interlaced)
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

   png_memset(png_ptr->prev_row, 0, png_ptr->rowbytes + 1);

   png_debug1(3, "width = %u,", png_ptr->width);
   png_debug1(3, "height = %u,", png_ptr->height);
   png_debug1(3, "iwidth = %u,", png_ptr->iwidth);
   png_debug1(3, "num_rows = %u,", png_ptr->num_rows);
   png_debug1(3, "rowbytes = %lu,", (unsigned long)png_ptr->rowbytes);
   png_debug1(3, "irowbytes = %lu",
       (unsigned long)PNG_ROWBYTES(png_ptr->pixel_depth, png_ptr->iwidth) + 1);

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
    png_ptr->flags &= ~PNG_FLAG_ROW_INIT;
}

void 
png_read_reinit(png_structp png_ptr, png_infop info_ptr)
{
    png_ptr->width = info_ptr->next_frame_width;
    png_ptr->height = info_ptr->next_frame_height;
    png_ptr->rowbytes = PNG_ROWBYTES(png_ptr->pixel_depth,png_ptr->width);
    png_ptr->info_rowbytes = PNG_ROWBYTES(info_ptr->pixel_depth,
        png_ptr->width);
    if (png_ptr->prev_row)
        png_memset(png_ptr->prev_row, 0, png_ptr->rowbytes + 1);
}

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED

void 
png_progressive_read_reset(png_structp png_ptr)
{
#ifdef PNG_READ_INTERLACING_SUPPORTED
   

   
    const int FARDATA png_pass_start[] = {0, 4, 0, 2, 0, 1, 0};

    
    const int FARDATA png_pass_inc[] = {8, 8, 4, 4, 2, 2, 1};

    
    const int FARDATA png_pass_ystart[] = {0, 0, 4, 0, 2, 0, 1};

    
    const int FARDATA png_pass_yinc[] = {8, 8, 8, 4, 4, 2, 2};

    if (png_ptr->interlaced)
    {
        if (!(png_ptr->transformations & PNG_INTERLACE))
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
    png_ptr->flags &= ~PNG_FLAG_ZLIB_FINISHED;
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

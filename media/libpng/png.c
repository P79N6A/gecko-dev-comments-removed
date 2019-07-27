












#include "pngpriv.h"


typedef png_libpng_version_1_6_17 Your_png_h_is_not_version_1_6_17;







#ifdef PNG_READ_SUPPORTED
void PNGAPI
png_set_sig_bytes(png_structrp png_ptr, int num_bytes)
{
   png_debug(1, "in png_set_sig_bytes");

   if (png_ptr == NULL)
      return;

   if (num_bytes > 8)
      png_error(png_ptr, "Too many bytes for PNG signature");

   png_ptr->sig_bytes = (png_byte)((num_bytes < 0 ? 0 : num_bytes) & 0xff);
}









int PNGAPI
png_sig_cmp(png_const_bytep sig, png_size_t start, png_size_t num_to_check)
{
   png_byte png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};

   if (num_to_check > 8)
      num_to_check = 8;

   else if (num_to_check < 1)
      return (-1);

   if (start > 7)
      return (-1);

   if (start + num_to_check > 8)
      num_to_check = 8 - start;

   return ((int)(memcmp(&sig[start], &png_signature[start], num_to_check)));
}

#endif 

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)

PNG_FUNCTION(voidpf ,
png_zalloc,(voidpf png_ptr, uInt items, uInt size),PNG_ALLOCATED)
{
   png_alloc_size_t num_bytes = size;

   if (png_ptr == NULL)
      return NULL;

   if (items >= (~(png_alloc_size_t)0)/size)
   {
      png_warning (png_voidcast(png_structrp, png_ptr),
         "Potential overflow in png_zalloc()");
      return NULL;
   }

   num_bytes *= items;
   return png_malloc_warn(png_voidcast(png_structrp, png_ptr), num_bytes);
}


void 
png_zfree(voidpf png_ptr, voidpf ptr)
{
   png_free(png_voidcast(png_const_structrp,png_ptr), ptr);
}




void 
png_reset_crc(png_structrp png_ptr)
{
   
   png_ptr->crc = (png_uint_32)crc32(0, Z_NULL, 0);
}






void 
png_calculate_crc(png_structrp png_ptr, png_const_bytep ptr, png_size_t length)
{
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

   




   if (need_crc != 0 && length > 0)
   {
      uLong crc = png_ptr->crc; 

      do
      {
         uInt safe_length = (uInt)length;
#ifndef __COVERITY__
         if (safe_length == 0)
            safe_length = (uInt)-1; 
#endif

         crc = crc32(crc, ptr, safe_length);

         



         ptr += safe_length;
         length -= safe_length;
      }
      while (length > 0);

      
      png_ptr->crc = (png_uint_32)crc;
   }
}




int
png_user_version_check(png_structrp png_ptr, png_const_charp user_png_ver)
{
     




   if (user_png_ver != NULL)
   {
      int i = -1;
      int found_dots = 0;

      do
      {
         i++;
         if (user_png_ver[i] != PNG_LIBPNG_VER_STRING[i])
            png_ptr->flags |= PNG_FLAG_LIBRARY_MISMATCH;
         if (user_png_ver[i] == '.')
            found_dots++;
      } while (found_dots < 2 && user_png_ver[i] != 0 &&
            PNG_LIBPNG_VER_STRING[i] != 0);
   }

   else
      png_ptr->flags |= PNG_FLAG_LIBRARY_MISMATCH;

   if ((png_ptr->flags & PNG_FLAG_LIBRARY_MISMATCH) != 0)
   {
#ifdef PNG_WARNINGS_SUPPORTED
      size_t pos = 0;
      char m[128];

      pos = png_safecat(m, (sizeof m), pos,
          "Application built with libpng-");
      pos = png_safecat(m, (sizeof m), pos, user_png_ver);
      pos = png_safecat(m, (sizeof m), pos, " but running with ");
      pos = png_safecat(m, (sizeof m), pos, PNG_LIBPNG_VER_STRING);
      PNG_UNUSED(pos)

      png_warning(png_ptr, m);
#endif

#ifdef PNG_ERROR_NUMBERS_SUPPORTED
      png_ptr->flags = 0;
#endif

      return 0;
   }

   
   return 1;
}




PNG_FUNCTION(png_structp ,
png_create_png_struct,(png_const_charp user_png_ver, png_voidp error_ptr,
    png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
    png_malloc_ptr malloc_fn, png_free_ptr free_fn),PNG_ALLOCATED)
{
   png_struct create_struct;
#  ifdef PNG_SETJMP_SUPPORTED
      jmp_buf create_jmp_buf;
#  endif

   



   memset(&create_struct, 0, (sizeof create_struct));

   
#  ifdef PNG_USER_LIMITS_SUPPORTED
      create_struct.user_width_max = PNG_USER_WIDTH_MAX;
      create_struct.user_height_max = PNG_USER_HEIGHT_MAX;

#     ifdef PNG_USER_CHUNK_CACHE_MAX
         
         create_struct.user_chunk_cache_max = PNG_USER_CHUNK_CACHE_MAX;
#     endif

#     ifdef PNG_USER_CHUNK_MALLOC_MAX
         


         create_struct.user_chunk_malloc_max = PNG_USER_CHUNK_MALLOC_MAX;
#     endif
#  endif

   


#  ifdef PNG_USER_MEM_SUPPORTED
      png_set_mem_fn(&create_struct, mem_ptr, malloc_fn, free_fn);
#  else
      PNG_UNUSED(mem_ptr)
      PNG_UNUSED(malloc_fn)
      PNG_UNUSED(free_fn)
#  endif

   




   png_set_error_fn(&create_struct, error_ptr, error_fn, warn_fn);

#  ifdef PNG_SETJMP_SUPPORTED
      if (!setjmp(create_jmp_buf))
      {
         




         create_struct.jmp_buf_ptr = &create_jmp_buf;
         create_struct.jmp_buf_size = 0; 
         create_struct.longjmp_fn = longjmp;
#  else
      {
#  endif


         if (png_user_version_check(&create_struct, user_png_ver) != 0)
         {
            png_structrp png_ptr = png_voidcast(png_structrp,
               png_malloc_warn(&create_struct, (sizeof *png_ptr)));

            if (png_ptr != NULL)
            {
               


               create_struct.zstream.zalloc = png_zalloc;
               create_struct.zstream.zfree = png_zfree;
               create_struct.zstream.opaque = png_ptr;

#              ifdef PNG_SETJMP_SUPPORTED
                  
                  create_struct.jmp_buf_ptr = NULL;
                  create_struct.jmp_buf_size = 0;
                  create_struct.longjmp_fn = 0;
#              endif

               *png_ptr = create_struct;

               
               return png_ptr;
            }
         }
      }

   


   return NULL;
}


PNG_FUNCTION(png_infop,PNGAPI
png_create_info_struct,(png_const_structrp png_ptr),PNG_ALLOCATED)
{
   png_inforp info_ptr;

   png_debug(1, "in png_create_info_struct");

   if (png_ptr == NULL)
      return NULL;

   




   info_ptr = png_voidcast(png_inforp, png_malloc_base(png_ptr,
      (sizeof *info_ptr)));

   if (info_ptr != NULL)
      memset(info_ptr, 0, (sizeof *info_ptr));

   return info_ptr;
}









void PNGAPI
png_destroy_info_struct(png_const_structrp png_ptr, png_infopp info_ptr_ptr)
{
   png_inforp info_ptr = NULL;

   png_debug(1, "in png_destroy_info_struct");

   if (png_ptr == NULL)
      return;

   if (info_ptr_ptr != NULL)
      info_ptr = *info_ptr_ptr;

   if (info_ptr != NULL)
   {
      





      *info_ptr_ptr = NULL;

      png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
      memset(info_ptr, 0, (sizeof *info_ptr));
      png_free(png_ptr, info_ptr);
   }
}










PNG_FUNCTION(void,PNGAPI
png_info_init_3,(png_infopp ptr_ptr, png_size_t png_info_struct_size),
   PNG_DEPRECATED)
{
   png_inforp info_ptr = *ptr_ptr;

   png_debug(1, "in png_info_init_3");

   if (info_ptr == NULL)
      return;

   if ((sizeof (png_info)) > png_info_struct_size)
   {
      *ptr_ptr = NULL;
      
      free(info_ptr);
      info_ptr = png_voidcast(png_inforp, png_malloc_base(NULL,
         (sizeof *info_ptr)));
      *ptr_ptr = info_ptr;
   }

   
   memset(info_ptr, 0, (sizeof *info_ptr));
}


void PNGAPI
png_data_freer(png_const_structrp png_ptr, png_inforp info_ptr,
   int freer, png_uint_32 mask)
{
   png_debug(1, "in png_data_freer");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   if (freer == PNG_DESTROY_WILL_FREE_DATA)
      info_ptr->free_me |= mask;

   else if (freer == PNG_USER_WILL_FREE_DATA)
      info_ptr->free_me &= ~mask;

   else
      png_error(png_ptr, "Unknown freer parameter in png_data_freer");
}

void PNGAPI
png_free_data(png_const_structrp png_ptr, png_inforp info_ptr, png_uint_32 mask,
   int num)
{
   png_debug(1, "in png_free_data");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

#ifdef PNG_TEXT_SUPPORTED
   
   if (info_ptr->text != 0 &&
       ((mask & PNG_FREE_TEXT) & info_ptr->free_me) != 0)
   {
      if (num != -1)
      {
         png_free(png_ptr, info_ptr->text[num].key);
         info_ptr->text[num].key = NULL;
      }

      else
      {
         int i;

         for (i = 0; i < info_ptr->num_text; i++)
            png_free(png_ptr, info_ptr->text[i].key);

         png_free(png_ptr, info_ptr->text);
         info_ptr->text = NULL;
         info_ptr->num_text = 0;
      }
   }
#endif

#ifdef PNG_tRNS_SUPPORTED
   
   if (((mask & PNG_FREE_TRNS) & info_ptr->free_me) != 0)
   {
      info_ptr->valid &= ~PNG_INFO_tRNS;
      png_free(png_ptr, info_ptr->trans_alpha);
      info_ptr->trans_alpha = NULL;
      info_ptr->num_trans = 0;
   }
#endif

#ifdef PNG_sCAL_SUPPORTED
   
   if (((mask & PNG_FREE_SCAL) & info_ptr->free_me) != 0)
   {
      png_free(png_ptr, info_ptr->scal_s_width);
      png_free(png_ptr, info_ptr->scal_s_height);
      info_ptr->scal_s_width = NULL;
      info_ptr->scal_s_height = NULL;
      info_ptr->valid &= ~PNG_INFO_sCAL;
   }
#endif

#ifdef PNG_pCAL_SUPPORTED
   
   if (((mask & PNG_FREE_PCAL) & info_ptr->free_me) != 0)
   {
      png_free(png_ptr, info_ptr->pcal_purpose);
      png_free(png_ptr, info_ptr->pcal_units);
      info_ptr->pcal_purpose = NULL;
      info_ptr->pcal_units = NULL;

      if (info_ptr->pcal_params != NULL)
         {
            int i;

            for (i = 0; i < info_ptr->pcal_nparams; i++)
               png_free(png_ptr, info_ptr->pcal_params[i]);

            png_free(png_ptr, info_ptr->pcal_params);
            info_ptr->pcal_params = NULL;
         }
      info_ptr->valid &= ~PNG_INFO_pCAL;
   }
#endif

#ifdef PNG_iCCP_SUPPORTED
   
   if (((mask & PNG_FREE_ICCP) & info_ptr->free_me) != 0)
   {
      png_free(png_ptr, info_ptr->iccp_name);
      png_free(png_ptr, info_ptr->iccp_profile);
      info_ptr->iccp_name = NULL;
      info_ptr->iccp_profile = NULL;
      info_ptr->valid &= ~PNG_INFO_iCCP;
   }
#endif

#ifdef PNG_sPLT_SUPPORTED
   
   if (info_ptr->splt_palettes != 0 &&
       ((mask & PNG_FREE_SPLT) & info_ptr->free_me) != 0)
   {
      if (num != -1)
      {
         png_free(png_ptr, info_ptr->splt_palettes[num].name);
         png_free(png_ptr, info_ptr->splt_palettes[num].entries);
         info_ptr->splt_palettes[num].name = NULL;
         info_ptr->splt_palettes[num].entries = NULL;
      }

      else
      {
         int i;

         for (i = 0; i < info_ptr->splt_palettes_num; i++)
         {
            png_free(png_ptr, info_ptr->splt_palettes[i].name);
            png_free(png_ptr, info_ptr->splt_palettes[i].entries);
         }

         png_free(png_ptr, info_ptr->splt_palettes);
         info_ptr->splt_palettes = NULL;
         info_ptr->splt_palettes_num = 0;
         info_ptr->valid &= ~PNG_INFO_sPLT;
      }
   }
#endif

#ifdef PNG_STORE_UNKNOWN_CHUNKS_SUPPORTED
   if (info_ptr->unknown_chunks != 0 &&
       ((mask & PNG_FREE_UNKN) & info_ptr->free_me) != 0)
   {
      if (num != -1)
      {
          png_free(png_ptr, info_ptr->unknown_chunks[num].data);
          info_ptr->unknown_chunks[num].data = NULL;
      }

      else
      {
         int i;

         for (i = 0; i < info_ptr->unknown_chunks_num; i++)
            png_free(png_ptr, info_ptr->unknown_chunks[i].data);

         png_free(png_ptr, info_ptr->unknown_chunks);
         info_ptr->unknown_chunks = NULL;
         info_ptr->unknown_chunks_num = 0;
      }
   }
#endif

#ifdef PNG_hIST_SUPPORTED
   
   if (((mask & PNG_FREE_HIST) & info_ptr->free_me) != 0)
   {
      png_free(png_ptr, info_ptr->hist);
      info_ptr->hist = NULL;
      info_ptr->valid &= ~PNG_INFO_hIST;
   }
#endif

   
   if (((mask & PNG_FREE_PLTE) & info_ptr->free_me) != 0)
   {
      png_free(png_ptr, info_ptr->palette);
      info_ptr->palette = NULL;
      info_ptr->valid &= ~PNG_INFO_PLTE;
      info_ptr->num_palette = 0;
   }

#ifdef PNG_INFO_IMAGE_SUPPORTED
   
   if (((mask & PNG_FREE_ROWS) & info_ptr->free_me) != 0)
   {
      if (info_ptr->row_pointers != 0)
      {
         png_uint_32 row;
         for (row = 0; row < info_ptr->height; row++)
            png_free(png_ptr, info_ptr->row_pointers[row]);

         png_free(png_ptr, info_ptr->row_pointers);
         info_ptr->row_pointers = NULL;
      }
      info_ptr->valid &= ~PNG_INFO_IDAT;
   }
#endif

   if (num != -1)
      mask &= ~PNG_FREE_MUL;

   info_ptr->free_me &= ~mask;
}
#endif 





png_voidp PNGAPI
png_get_io_ptr(png_const_structrp png_ptr)
{
   if (png_ptr == NULL)
      return (NULL);

   return (png_ptr->io_ptr);
}

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)
#  ifdef PNG_STDIO_SUPPORTED






void PNGAPI
png_init_io(png_structrp png_ptr, png_FILE_p fp)
{
   png_debug(1, "in png_init_io");

   if (png_ptr == NULL)
      return;

   png_ptr->io_ptr = (png_voidp)fp;
}
#  endif

#  ifdef PNG_SAVE_INT_32_SUPPORTED






void PNGAPI
png_save_int_32(png_bytep buf, png_int_32 i)
{
   buf[0] = (png_byte)((i >> 24) & 0xff);
   buf[1] = (png_byte)((i >> 16) & 0xff);
   buf[2] = (png_byte)((i >> 8) & 0xff);
   buf[3] = (png_byte)(i & 0xff);
}
#  endif

#  ifdef PNG_TIME_RFC1123_SUPPORTED



int PNGAPI
png_convert_to_rfc1123_buffer(char out[29], png_const_timep ptime)
{
   static PNG_CONST char short_months[12][4] =
        {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

   if (out == NULL)
      return 0;

   if (ptime->year > 9999  ||
       ptime->month == 0    ||  ptime->month > 12  ||
       ptime->day   == 0    ||  ptime->day   > 31  ||
       ptime->hour  > 23    ||  ptime->minute > 59 ||
       ptime->second > 60)
      return 0;

   {
      size_t pos = 0;
      char number_buf[5]; 

#     define APPEND_STRING(string) pos = png_safecat(out, 29, pos, (string))
#     define APPEND_NUMBER(format, value)\
         APPEND_STRING(PNG_FORMAT_NUMBER(number_buf, format, (value)))
#     define APPEND(ch) if (pos < 28) out[pos++] = (ch)

      APPEND_NUMBER(PNG_NUMBER_FORMAT_u, (unsigned)ptime->day);
      APPEND(' ');
      APPEND_STRING(short_months[(ptime->month - 1)]);
      APPEND(' ');
      APPEND_NUMBER(PNG_NUMBER_FORMAT_u, ptime->year);
      APPEND(' ');
      APPEND_NUMBER(PNG_NUMBER_FORMAT_02u, (unsigned)ptime->hour);
      APPEND(':');
      APPEND_NUMBER(PNG_NUMBER_FORMAT_02u, (unsigned)ptime->minute);
      APPEND(':');
      APPEND_NUMBER(PNG_NUMBER_FORMAT_02u, (unsigned)ptime->second);
      APPEND_STRING(" +0000"); 

#     undef APPEND
#     undef APPEND_NUMBER
#     undef APPEND_STRING
   }

   return 1;
}

#    if PNG_LIBPNG_VER < 10700





png_const_charp PNGAPI
png_convert_to_rfc1123(png_structrp png_ptr, png_const_timep ptime)
{
   if (png_ptr != NULL)
   {
      
      if (png_convert_to_rfc1123_buffer(png_ptr->time_buffer, ptime) == 0)
         png_warning(png_ptr, "Ignoring invalid time value");

      else
         return png_ptr->time_buffer;
   }

   return NULL;
}
#    endif 
#  endif 

#endif 

png_const_charp PNGAPI
png_get_copyright(png_const_structrp png_ptr)
{
   PNG_UNUSED(png_ptr)  
#ifdef PNG_STRING_COPYRIGHT
   return PNG_STRING_COPYRIGHT
#else
#  ifdef __STDC__
   return PNG_STRING_NEWLINE \
     "libpng version 1.6.17 - March 25, 2015" PNG_STRING_NEWLINE \
     "Copyright (c) 1998-2015 Glenn Randers-Pehrson" PNG_STRING_NEWLINE \
     "Copyright (c) 1996-1997 Andreas Dilger" PNG_STRING_NEWLINE \
     "Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc." \
     PNG_STRING_NEWLINE;
#  else
      return "libpng version 1.6.17 - March 25, 2015\
      Copyright (c) 1998-2015 Glenn Randers-Pehrson\
      Copyright (c) 1996-1997 Andreas Dilger\
      Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.";
#  endif
#endif
}









png_const_charp PNGAPI
png_get_libpng_ver(png_const_structrp png_ptr)
{
   
   return png_get_header_ver(png_ptr);
}

png_const_charp PNGAPI
png_get_header_ver(png_const_structrp png_ptr)
{
   
   PNG_UNUSED(png_ptr)  
   return PNG_LIBPNG_VER_STRING;
}

png_const_charp PNGAPI
png_get_header_version(png_const_structrp png_ptr)
{
   
   PNG_UNUSED(png_ptr)  
#ifdef __STDC__
   return PNG_HEADER_VERSION_STRING
#  ifndef PNG_READ_SUPPORTED
   "     (NO READ SUPPORT)"
#  endif
   PNG_STRING_NEWLINE;
#else
   return PNG_HEADER_VERSION_STRING;
#endif
}

#ifdef PNG_BUILD_GRAYSCALE_PALETTE_SUPPORTED






void PNGAPI
png_build_grayscale_palette(int bit_depth, png_colorp palette)
{
   int num_palette;
   int color_inc;
   int i;
   int v;

   png_debug(1, "in png_do_build_grayscale_palette");

   if (palette == NULL)
      return;

   switch (bit_depth)
   {
      case 1:
         num_palette = 2;
         color_inc = 0xff;
         break;

      case 2:
         num_palette = 4;
         color_inc = 0x55;
         break;

      case 4:
         num_palette = 16;
         color_inc = 0x11;
         break;

      case 8:
         num_palette = 256;
         color_inc = 1;
         break;

      default:
         num_palette = 0;
         color_inc = 0;
         break;
   }

   for (i = 0, v = 0; i < num_palette; i++, v += color_inc)
   {
      palette[i].red = (png_byte)(v & 0xff);
      palette[i].green = (png_byte)(v & 0xff);
      palette[i].blue = (png_byte)(v & 0xff);
   }
}
#endif

#ifdef PNG_SET_UNKNOWN_CHUNKS_SUPPORTED
int PNGAPI
png_handle_as_unknown(png_const_structrp png_ptr, png_const_bytep chunk_name)
{
   
   png_const_bytep p, p_end;

   if (png_ptr == NULL || chunk_name == NULL || png_ptr->num_chunk_list == 0)
      return PNG_HANDLE_CHUNK_AS_DEFAULT;

   p_end = png_ptr->chunk_list;
   p = p_end + png_ptr->num_chunk_list*5; 

   



   do 
   {
      p -= 5;

      if (memcmp(chunk_name, p, 4) == 0)
         return p[4];
   }
   while (p > p_end);

   




   return PNG_HANDLE_CHUNK_AS_DEFAULT;
}

#if defined(PNG_READ_UNKNOWN_CHUNKS_SUPPORTED) ||\
   defined(PNG_HANDLE_AS_UNKNOWN_SUPPORTED)
int 
png_chunk_unknown_handling(png_const_structrp png_ptr, png_uint_32 chunk_name)
{
   png_byte chunk_string[5];

   PNG_CSTRING_FROM_CHUNK(chunk_string, chunk_name);
   return png_handle_as_unknown(png_ptr, chunk_string);
}
#endif 
#endif 

#ifdef PNG_READ_SUPPORTED

int PNGAPI
png_reset_zstream(png_structrp png_ptr)
{
   if (png_ptr == NULL)
      return Z_STREAM_ERROR;

   
   return (inflateReset(&png_ptr->zstream));
}
#endif 


png_uint_32 PNGAPI
png_access_version_number(void)
{
   
   return((png_uint_32)PNG_LIBPNG_VER);
}

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)




void 
png_zstream_error(png_structrp png_ptr, int ret)
{
   



   if (png_ptr->zstream.msg == NULL) switch (ret)
   {
      default:
      case Z_OK:
         png_ptr->zstream.msg = PNGZ_MSG_CAST("unexpected zlib return code");
         break;

      case Z_STREAM_END:
         
         png_ptr->zstream.msg = PNGZ_MSG_CAST("unexpected end of LZ stream");
         break;

      case Z_NEED_DICT:
         


         png_ptr->zstream.msg = PNGZ_MSG_CAST("missing LZ dictionary");
         break;

      case Z_ERRNO:
         
         png_ptr->zstream.msg = PNGZ_MSG_CAST("zlib IO error");
         break;

      case Z_STREAM_ERROR:
         
         png_ptr->zstream.msg = PNGZ_MSG_CAST("bad parameters to zlib");
         break;

      case Z_DATA_ERROR:
         png_ptr->zstream.msg = PNGZ_MSG_CAST("damaged LZ stream");
         break;

      case Z_MEM_ERROR:
         png_ptr->zstream.msg = PNGZ_MSG_CAST("insufficient memory");
         break;

      case Z_BUF_ERROR:
         


         png_ptr->zstream.msg = PNGZ_MSG_CAST("truncated");
         break;

      case Z_VERSION_ERROR:
         png_ptr->zstream.msg = PNGZ_MSG_CAST("unsupported zlib version");
         break;

      case PNG_UNEXPECTED_ZLIB_RETURN:
         




         png_ptr->zstream.msg = PNGZ_MSG_CAST("unexpected zlib return");
         break;
   }
}






#ifdef PNG_GAMMA_SUPPORTED 
static int
png_colorspace_check_gamma(png_const_structrp png_ptr,
   png_colorspacerp colorspace, png_fixed_point gAMA, int from)
   








{
   png_fixed_point gtest;

   if ((colorspace->flags & PNG_COLORSPACE_HAVE_GAMMA) != 0 &&
      (png_muldiv(&gtest, colorspace->gamma, PNG_FP_1, gAMA) == 0  ||
      png_gamma_significant(gtest) != 0))
   {
      





      if ((colorspace->flags & PNG_COLORSPACE_FROM_sRGB) != 0 || from == 2)
      {
         png_chunk_report(png_ptr, "gamma value does not match sRGB",
            PNG_CHUNK_ERROR);
         
         return from == 2;
      }

      else 
      {
         png_chunk_report(png_ptr, "gamma value does not match libpng estimate",
            PNG_CHUNK_WARNING);
         return from == 1;
      }
   }

   return 1;
}

void 
png_colorspace_set_gamma(png_const_structrp png_ptr,
   png_colorspacerp colorspace, png_fixed_point gAMA)
{
   










   png_const_charp errmsg;

   if (gAMA < 16 || gAMA > 625000000)
      errmsg = "gamma value out of range";

#  ifdef PNG_READ_gAMA_SUPPORTED
      
      else if ((png_ptr->mode & PNG_IS_READ_STRUCT) != 0 &&
         (colorspace->flags & PNG_COLORSPACE_FROM_gAMA) != 0)
         errmsg = "duplicate";
#  endif

   
   else if ((colorspace->flags & PNG_COLORSPACE_INVALID) != 0)
      return;

   else
   {
      if (png_colorspace_check_gamma(png_ptr, colorspace, gAMA,
          1) != 0)
      {
         
         colorspace->gamma = gAMA;
         colorspace->flags |=
            (PNG_COLORSPACE_HAVE_GAMMA | PNG_COLORSPACE_FROM_gAMA);
      }

      




      return;
   }

   
   colorspace->flags |= PNG_COLORSPACE_INVALID;
   png_chunk_report(png_ptr, errmsg, PNG_CHUNK_WRITE_ERROR);
}

void 
png_colorspace_sync_info(png_const_structrp png_ptr, png_inforp info_ptr)
{
   if ((info_ptr->colorspace.flags & PNG_COLORSPACE_INVALID) != 0)
   {
      
      info_ptr->valid &= ~(PNG_INFO_gAMA|PNG_INFO_cHRM|PNG_INFO_sRGB|
         PNG_INFO_iCCP);

#     ifdef PNG_COLORSPACE_SUPPORTED
         
         png_free_data(png_ptr, info_ptr, PNG_FREE_ICCP, -1);
#     else
         PNG_UNUSED(png_ptr)
#     endif
   }

   else
   {
#     ifdef PNG_COLORSPACE_SUPPORTED
         



         if ((info_ptr->colorspace.flags & PNG_COLORSPACE_MATCHES_sRGB) != 0)
            info_ptr->valid |= PNG_INFO_sRGB;

         else
            info_ptr->valid &= ~PNG_INFO_sRGB;

         if ((info_ptr->colorspace.flags & PNG_COLORSPACE_HAVE_ENDPOINTS) != 0)
            info_ptr->valid |= PNG_INFO_cHRM;

         else
            info_ptr->valid &= ~PNG_INFO_cHRM;
#     endif

      if ((info_ptr->colorspace.flags & PNG_COLORSPACE_HAVE_GAMMA) != 0)
         info_ptr->valid |= PNG_INFO_gAMA;

      else
         info_ptr->valid &= ~PNG_INFO_gAMA;
   }
}

#ifdef PNG_READ_SUPPORTED
void 
png_colorspace_sync(png_const_structrp png_ptr, png_inforp info_ptr)
{
   if (info_ptr == NULL) 
      return;

   info_ptr->colorspace = png_ptr->colorspace;
   png_colorspace_sync_info(png_ptr, info_ptr);
}
#endif
#endif 

#ifdef PNG_COLORSPACE_SUPPORTED





static int
png_xy_from_XYZ(png_xy *xy, const png_XYZ *XYZ)
{
   png_int_32 d, dwhite, whiteX, whiteY;

   d = XYZ->red_X + XYZ->red_Y + XYZ->red_Z;
   if (png_muldiv(&xy->redx, XYZ->red_X, PNG_FP_1, d) == 0)
      return 1;
   if (png_muldiv(&xy->redy, XYZ->red_Y, PNG_FP_1, d) == 0)
      return 1;
   dwhite = d;
   whiteX = XYZ->red_X;
   whiteY = XYZ->red_Y;

   d = XYZ->green_X + XYZ->green_Y + XYZ->green_Z;
   if (png_muldiv(&xy->greenx, XYZ->green_X, PNG_FP_1, d) == 0)
      return 1;
   if (png_muldiv(&xy->greeny, XYZ->green_Y, PNG_FP_1, d) == 0)
      return 1;
   dwhite += d;
   whiteX += XYZ->green_X;
   whiteY += XYZ->green_Y;

   d = XYZ->blue_X + XYZ->blue_Y + XYZ->blue_Z;
   if (png_muldiv(&xy->bluex, XYZ->blue_X, PNG_FP_1, d) == 0)
      return 1;
   if (png_muldiv(&xy->bluey, XYZ->blue_Y, PNG_FP_1, d) == 0)
      return 1;
   dwhite += d;
   whiteX += XYZ->blue_X;
   whiteY += XYZ->blue_Y;

   


   if (png_muldiv(&xy->whitex, whiteX, PNG_FP_1, dwhite) == 0)
      return 1;
   if (png_muldiv(&xy->whitey, whiteY, PNG_FP_1, dwhite) == 0)
      return 1;

   return 0;
}

static int
png_XYZ_from_xy(png_XYZ *XYZ, const png_xy *xy)
{
   png_fixed_point red_inverse, green_inverse, blue_scale;
   png_fixed_point left, right, denominator;

   



   if (xy->redx < 0 || xy->redx > PNG_FP_1) return 1;
   if (xy->redy < 0 || xy->redy > PNG_FP_1-xy->redx) return 1;
   if (xy->greenx < 0 || xy->greenx > PNG_FP_1) return 1;
   if (xy->greeny < 0 || xy->greeny > PNG_FP_1-xy->greenx) return 1;
   if (xy->bluex < 0 || xy->bluex > PNG_FP_1) return 1;
   if (xy->bluey < 0 || xy->bluey > PNG_FP_1-xy->bluex) return 1;
   if (xy->whitex < 0 || xy->whitex > PNG_FP_1) return 1;
   if (xy->whitey < 0 || xy->whitey > PNG_FP_1-xy->whitex) return 1;

   















































































































































































   


   if (png_muldiv(&left, xy->greenx-xy->bluex, xy->redy - xy->bluey, 7) == 0)
      return 2;
   if (png_muldiv(&right, xy->greeny-xy->bluey, xy->redx - xy->bluex, 7) == 0)
      return 2;
   denominator = left - right;

   
   if (png_muldiv(&left, xy->greenx-xy->bluex, xy->whitey-xy->bluey, 7) == 0)
      return 2;
   if (png_muldiv(&right, xy->greeny-xy->bluey, xy->whitex-xy->bluex, 7) == 0)
      return 2;

   




   if (png_muldiv(&red_inverse, xy->whitey, denominator, left-right) == 0 ||
       red_inverse <= xy->whitey )
      return 1;

   
   if (png_muldiv(&left, xy->redy-xy->bluey, xy->whitex-xy->bluex, 7) == 0)
      return 2;
   if (png_muldiv(&right, xy->redx-xy->bluex, xy->whitey-xy->bluey, 7) == 0)
      return 2;
   if (png_muldiv(&green_inverse, xy->whitey, denominator, left-right) == 0 ||
       green_inverse <= xy->whitey)
      return 1;

   


   blue_scale = png_reciprocal(xy->whitey) - png_reciprocal(red_inverse) -
       png_reciprocal(green_inverse);
   if (blue_scale <= 0)
      return 1;


   
   if (png_muldiv(&XYZ->red_X, xy->redx, PNG_FP_1, red_inverse) == 0)
      return 1;
   if (png_muldiv(&XYZ->red_Y, xy->redy, PNG_FP_1, red_inverse) == 0)
      return 1;
   if (png_muldiv(&XYZ->red_Z, PNG_FP_1 - xy->redx - xy->redy, PNG_FP_1,
       red_inverse) == 0)
      return 1;

   if (png_muldiv(&XYZ->green_X, xy->greenx, PNG_FP_1, green_inverse) == 0)
      return 1;
   if (png_muldiv(&XYZ->green_Y, xy->greeny, PNG_FP_1, green_inverse) == 0)
      return 1;
   if (png_muldiv(&XYZ->green_Z, PNG_FP_1 - xy->greenx - xy->greeny, PNG_FP_1,
       green_inverse) == 0)
      return 1;

   if (png_muldiv(&XYZ->blue_X, xy->bluex, blue_scale, PNG_FP_1) == 0)
      return 1;
   if (png_muldiv(&XYZ->blue_Y, xy->bluey, blue_scale, PNG_FP_1) == 0)
      return 1;
   if (png_muldiv(&XYZ->blue_Z, PNG_FP_1 - xy->bluex - xy->bluey, blue_scale,
       PNG_FP_1) == 0)
      return 1;

   return 0; 
}

static int
png_XYZ_normalize(png_XYZ *XYZ)
{
   png_int_32 Y;

   if (XYZ->red_Y < 0 || XYZ->green_Y < 0 || XYZ->blue_Y < 0 ||
      XYZ->red_X < 0 || XYZ->green_X < 0 || XYZ->blue_X < 0 ||
      XYZ->red_Z < 0 || XYZ->green_Z < 0 || XYZ->blue_Z < 0)
      return 1;

   




   Y = XYZ->red_Y;
   if (0x7fffffff - Y < XYZ->green_X)
      return 1;
   Y += XYZ->green_Y;
   if (0x7fffffff - Y < XYZ->blue_X)
      return 1;
   Y += XYZ->blue_Y;

   if (Y != PNG_FP_1)
   {
      if (png_muldiv(&XYZ->red_X, XYZ->red_X, PNG_FP_1, Y) == 0)
         return 1;
      if (png_muldiv(&XYZ->red_Y, XYZ->red_Y, PNG_FP_1, Y) == 0)
         return 1;
      if (png_muldiv(&XYZ->red_Z, XYZ->red_Z, PNG_FP_1, Y) == 0)
         return 1;

      if (png_muldiv(&XYZ->green_X, XYZ->green_X, PNG_FP_1, Y) == 0)
         return 1;
      if (png_muldiv(&XYZ->green_Y, XYZ->green_Y, PNG_FP_1, Y) == 0)
         return 1;
      if (png_muldiv(&XYZ->green_Z, XYZ->green_Z, PNG_FP_1, Y) == 0)
         return 1;

      if (png_muldiv(&XYZ->blue_X, XYZ->blue_X, PNG_FP_1, Y) == 0)
         return 1;
      if (png_muldiv(&XYZ->blue_Y, XYZ->blue_Y, PNG_FP_1, Y) == 0)
         return 1;
      if (png_muldiv(&XYZ->blue_Z, XYZ->blue_Z, PNG_FP_1, Y) == 0)
         return 1;
   }

   return 0;
}

static int
png_colorspace_endpoints_match(const png_xy *xy1, const png_xy *xy2, int delta)
{
   
   if (PNG_OUT_OF_RANGE(xy1->whitex, xy2->whitex,delta) ||
       PNG_OUT_OF_RANGE(xy1->whitey, xy2->whitey,delta) ||
       PNG_OUT_OF_RANGE(xy1->redx,   xy2->redx,  delta) ||
       PNG_OUT_OF_RANGE(xy1->redy,   xy2->redy,  delta) ||
       PNG_OUT_OF_RANGE(xy1->greenx, xy2->greenx,delta) ||
       PNG_OUT_OF_RANGE(xy1->greeny, xy2->greeny,delta) ||
       PNG_OUT_OF_RANGE(xy1->bluex,  xy2->bluex, delta) ||
       PNG_OUT_OF_RANGE(xy1->bluey,  xy2->bluey, delta))
      return 0;
   return 1;
}











static int
png_colorspace_check_xy(png_XYZ *XYZ, const png_xy *xy)
{
   int result;
   png_xy xy_test;

   
   result = png_XYZ_from_xy(XYZ, xy);
   if (result != 0)
      return result;

   result = png_xy_from_XYZ(&xy_test, XYZ);
   if (result != 0)
      return result;

   if (png_colorspace_endpoints_match(xy, &xy_test,
       5) != 0)
      return 0;

   
   return 1;
}




static int
png_colorspace_check_XYZ(png_xy *xy, png_XYZ *XYZ)
{
   int result;
   png_XYZ XYZtemp;

   result = png_XYZ_normalize(XYZ);
   if (result != 0)
      return result;

   result = png_xy_from_XYZ(xy, XYZ);
   if (result != 0)
      return result;

   XYZtemp = *XYZ;
   return png_colorspace_check_xy(&XYZtemp, xy);
}


static const png_xy sRGB_xy = 
{
   
    64000, 33000,
    30000, 60000,
    15000,  6000,
    31270, 32900
};

static int
png_colorspace_set_xy_and_XYZ(png_const_structrp png_ptr,
   png_colorspacerp colorspace, const png_xy *xy, const png_XYZ *XYZ,
   int preferred)
{
   if ((colorspace->flags & PNG_COLORSPACE_INVALID) != 0)
      return 0;

   



   if (preferred < 2 &&
       (colorspace->flags & PNG_COLORSPACE_HAVE_ENDPOINTS) != 0)
   {
      


      if (png_colorspace_endpoints_match(xy, &colorspace->end_points_xy,
          100) == 0)
      {
         colorspace->flags |= PNG_COLORSPACE_INVALID;
         png_benign_error(png_ptr, "inconsistent chromaticities");
         return 0; 
      }

      
      if (preferred == 0)
         return 1; 
   }

   colorspace->end_points_xy = *xy;
   colorspace->end_points_XYZ = *XYZ;
   colorspace->flags |= PNG_COLORSPACE_HAVE_ENDPOINTS;

   


   if (png_colorspace_endpoints_match(xy, &sRGB_xy, 1000) != 0)
      colorspace->flags |= PNG_COLORSPACE_ENDPOINTS_MATCH_sRGB;

   else
      colorspace->flags &= PNG_COLORSPACE_CANCEL(
         PNG_COLORSPACE_ENDPOINTS_MATCH_sRGB);

   return 2; 
}

int 
png_colorspace_set_chromaticities(png_const_structrp png_ptr,
   png_colorspacerp colorspace, const png_xy *xy, int preferred)
{
   





   png_XYZ XYZ;

   switch (png_colorspace_check_xy(&XYZ, xy))
   {
      case 0: 
         return png_colorspace_set_xy_and_XYZ(png_ptr, colorspace, xy, &XYZ,
            preferred);

      case 1:
         


         colorspace->flags |= PNG_COLORSPACE_INVALID;
         png_benign_error(png_ptr, "invalid chromaticities");
         break;

      default:
         


         colorspace->flags |= PNG_COLORSPACE_INVALID;
         png_error(png_ptr, "internal error checking chromaticities");
         break;
   }

   return 0; 
}

int 
png_colorspace_set_endpoints(png_const_structrp png_ptr,
   png_colorspacerp colorspace, const png_XYZ *XYZ_in, int preferred)
{
   png_XYZ XYZ = *XYZ_in;
   png_xy xy;

   switch (png_colorspace_check_XYZ(&xy, &XYZ))
   {
      case 0:
         return png_colorspace_set_xy_and_XYZ(png_ptr, colorspace, &xy, &XYZ,
            preferred);

      case 1:
         
         colorspace->flags |= PNG_COLORSPACE_INVALID;
         png_benign_error(png_ptr, "invalid end points");
         break;

      default:
         colorspace->flags |= PNG_COLORSPACE_INVALID;
         png_error(png_ptr, "internal error checking chromaticities");
         break;
   }

   return 0; 
}

#if defined(PNG_sRGB_SUPPORTED) || defined(PNG_iCCP_SUPPORTED)

static char
png_icc_tag_char(png_uint_32 byte)
{
   byte &= 0xff;
   if (byte >= 32 && byte <= 126)
      return (char)byte;
   else
      return '?';
}

static void
png_icc_tag_name(char *name, png_uint_32 tag)
{
   name[0] = '\'';
   name[1] = png_icc_tag_char(tag >> 24);
   name[2] = png_icc_tag_char(tag >> 16);
   name[3] = png_icc_tag_char(tag >>  8);
   name[4] = png_icc_tag_char(tag      );
   name[5] = '\'';
}

static int
is_ICC_signature_char(png_alloc_size_t it)
{
   return it == 32 || (it >= 48 && it <= 57) || (it >= 65 && it <= 90) ||
      (it >= 97 && it <= 122);
}

static int
is_ICC_signature(png_alloc_size_t it)
{
   return is_ICC_signature_char(it >> 24)  &&
      is_ICC_signature_char((it >> 16) & 0xff) &&
      is_ICC_signature_char((it >> 8) & 0xff) &&
      is_ICC_signature_char(it & 0xff);
}

static int
png_icc_profile_error(png_const_structrp png_ptr, png_colorspacerp colorspace,
   png_const_charp name, png_alloc_size_t value, png_const_charp reason)
{
   size_t pos;
   char message[196]; 

   if (colorspace != NULL)
      colorspace->flags |= PNG_COLORSPACE_INVALID;

   pos = png_safecat(message, (sizeof message), 0, "profile '"); 
   pos = png_safecat(message, pos+79, pos, name); 
   pos = png_safecat(message, (sizeof message), pos, "': "); 
   if (is_ICC_signature(value) != 0)
   {
      
      png_icc_tag_name(message+pos, (png_uint_32)value);
      pos += 6; 
      message[pos++] = ':';
      message[pos++] = ' ';
   }
#  ifdef PNG_WARNINGS_SUPPORTED
   else
      {
         char number[PNG_NUMBER_BUFFER_SIZE]; 

         pos = png_safecat(message, (sizeof message), pos,
            png_format_number(number, number+(sizeof number),
               PNG_NUMBER_FORMAT_x, value));
         pos = png_safecat(message, (sizeof message), pos, "h: "); 
      }
#  endif
   
   pos = png_safecat(message, (sizeof message), pos, reason);
   PNG_UNUSED(pos)

   




   png_chunk_report(png_ptr, message,
      (colorspace != NULL) ? PNG_CHUNK_ERROR : PNG_CHUNK_WRITE_ERROR);

   return 0;
}
#endif 

#ifdef PNG_sRGB_SUPPORTED
int 
png_colorspace_set_sRGB(png_const_structrp png_ptr, png_colorspacerp colorspace,
   int intent)
{
   
   










   static const png_XYZ sRGB_XYZ = 
   {
      
       41239, 21264,  1933,
       35758, 71517, 11919,
       18048,  7219, 95053
   };

   
   if ((colorspace->flags & PNG_COLORSPACE_INVALID) != 0)
      return 0;

   








   if (intent < 0 || intent >= PNG_sRGB_INTENT_LAST)
      return png_icc_profile_error(png_ptr, colorspace, "sRGB",
         (unsigned)intent, "invalid sRGB rendering intent");

   if ((colorspace->flags & PNG_COLORSPACE_HAVE_INTENT) != 0 &&
      colorspace->rendering_intent != intent)
      return png_icc_profile_error(png_ptr, colorspace, "sRGB",
         (unsigned)intent, "inconsistent rendering intents");

   if ((colorspace->flags & PNG_COLORSPACE_FROM_sRGB) != 0)
   {
      png_benign_error(png_ptr, "duplicate sRGB information ignored");
      return 0;
   }

   


   if ((colorspace->flags & PNG_COLORSPACE_HAVE_ENDPOINTS) != 0 &&
      !png_colorspace_endpoints_match(&sRGB_xy, &colorspace->end_points_xy,
         100))
      png_chunk_report(png_ptr, "cHRM chunk does not match sRGB",
         PNG_CHUNK_ERROR);

   


   (void)png_colorspace_check_gamma(png_ptr, colorspace, PNG_GAMMA_sRGB_INVERSE,
      2);

   
   colorspace->rendering_intent = (png_uint_16)intent;
   colorspace->flags |= PNG_COLORSPACE_HAVE_INTENT;

   
   colorspace->end_points_xy = sRGB_xy;
   colorspace->end_points_XYZ = sRGB_XYZ;
   colorspace->flags |=
      (PNG_COLORSPACE_HAVE_ENDPOINTS|PNG_COLORSPACE_ENDPOINTS_MATCH_sRGB);

   
   colorspace->gamma = PNG_GAMMA_sRGB_INVERSE;
   colorspace->flags |= PNG_COLORSPACE_HAVE_GAMMA;

   
   colorspace->flags |=
      (PNG_COLORSPACE_MATCHES_sRGB|PNG_COLORSPACE_FROM_sRGB);

   return 1; 
}
#endif 

#ifdef PNG_iCCP_SUPPORTED





static const png_byte D50_nCIEXYZ[12] =
   { 0x00, 0x00, 0xf6, 0xd6, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xd3, 0x2d };

int 
png_icc_check_length(png_const_structrp png_ptr, png_colorspacerp colorspace,
   png_const_charp name, png_uint_32 profile_length)
{
   if (profile_length < 132)
      return png_icc_profile_error(png_ptr, colorspace, name, profile_length,
         "too short");

   return 1;
}

int 
png_icc_check_header(png_const_structrp png_ptr, png_colorspacerp colorspace,
   png_const_charp name, png_uint_32 profile_length,
   png_const_bytep profile, int color_type)
{
   png_uint_32 temp;

   




   temp = png_get_uint_32(profile);
   if (temp != profile_length)
      return png_icc_profile_error(png_ptr, colorspace, name, temp,
         "length does not match profile");

   temp = (png_uint_32) (*(profile+8));
   if (temp > 3 && (profile_length & 3))
      return png_icc_profile_error(png_ptr, colorspace, name, profile_length,
         "invalid length");

   temp = png_get_uint_32(profile+128); 
   if (temp > 357913930 || 
      profile_length < 132+12*temp) 
      return png_icc_profile_error(png_ptr, colorspace, name, temp,
         "tag count too large");

   


   temp = png_get_uint_32(profile+64);
   if (temp >= 0xffff) 
      return png_icc_profile_error(png_ptr, colorspace, name, temp,
         "invalid rendering intent");

   


   if (temp >= PNG_sRGB_INTENT_LAST)
      (void)png_icc_profile_error(png_ptr, NULL, name, temp,
         "intent outside defined range");

   






   




   temp = png_get_uint_32(profile+36); 
   if (temp != 0x61637370)
      return png_icc_profile_error(png_ptr, colorspace, name, temp,
         "invalid signature");

   






   if (memcmp(profile+68, D50_nCIEXYZ, 12) != 0)
      (void)png_icc_profile_error(png_ptr, NULL, name, 0,
         "PCS illuminant is not D50");

   



















   temp = png_get_uint_32(profile+16); 
   switch (temp)
   {
      case 0x52474220: 
         if ((color_type & PNG_COLOR_MASK_COLOR) == 0)
            return png_icc_profile_error(png_ptr, colorspace, name, temp,
               "RGB color space not permitted on grayscale PNG");
         break;

      case 0x47524159: 
         if ((color_type & PNG_COLOR_MASK_COLOR) != 0)
            return png_icc_profile_error(png_ptr, colorspace, name, temp,
               "Gray color space not permitted on RGB PNG");
         break;

      default:
         return png_icc_profile_error(png_ptr, colorspace, name, temp,
            "invalid ICC profile color space");
   }

   








   temp = png_get_uint_32(profile+12); 
   switch (temp)
   {
      case 0x73636E72: 
      case 0x6D6E7472: 
      case 0x70727472: 
      case 0x73706163: 
         
         break;

      case 0x61627374: 
         
         return png_icc_profile_error(png_ptr, colorspace, name, temp,
            "invalid embedded Abstract ICC profile");

      case 0x6C696E6B: 
         





         return png_icc_profile_error(png_ptr, colorspace, name, temp,
            "unexpected DeviceLink ICC profile class");

      case 0x6E6D636C: 
         



         (void)png_icc_profile_error(png_ptr, NULL, name, temp,
            "unexpected NamedColor ICC profile class");
         break;

      default:
         




         (void)png_icc_profile_error(png_ptr, NULL, name, temp,
            "unrecognized ICC profile class");
         break;
   }

   


   temp = png_get_uint_32(profile+20);
   switch (temp)
   {
      case 0x58595A20: 
      case 0x4C616220: 
         break;

      default:
         return png_icc_profile_error(png_ptr, colorspace, name, temp,
            "unexpected ICC PCS encoding");
   }

   return 1;
}

int 
png_icc_check_tag_table(png_const_structrp png_ptr, png_colorspacerp colorspace,
   png_const_charp name, png_uint_32 profile_length,
   png_const_bytep profile )
{
   png_uint_32 tag_count = png_get_uint_32(profile+128);
   png_uint_32 itag;
   png_const_bytep tag = profile+132; 

   


   for (itag=0; itag < tag_count; ++itag, tag += 12)
   {
      png_uint_32 tag_id = png_get_uint_32(tag+0);
      png_uint_32 tag_start = png_get_uint_32(tag+4); 
      png_uint_32 tag_length = png_get_uint_32(tag+8);

      





      if ((tag_start & 3) != 0)
      {
         



         (void)png_icc_profile_error(png_ptr, NULL, name, tag_id,
            "ICC profile tag start not a multiple of 4");
      }

      


      if (tag_start > profile_length || tag_length > profile_length - tag_start)
         return png_icc_profile_error(png_ptr, colorspace, name, tag_id,
            "ICC profile tag outside profile");
   }

   return 1; 
}

#ifdef PNG_sRGB_SUPPORTED
#if PNG_sRGB_PROFILE_CHECKS >= 0

static const struct
{
   png_uint_32 adler, crc, length;
   png_uint_32 md5[4];
   png_byte    have_md5;
   png_byte    is_broken;
   png_uint_16 intent;

#  define PNG_MD5(a,b,c,d) { a, b, c, d }, (a!=0)||(b!=0)||(c!=0)||(d!=0)
#  define PNG_ICC_CHECKSUM(adler, crc, md5, intent, broke, date, length, fname)\
      { adler, crc, length, md5, broke, intent },

} png_sRGB_checks[] =
{
   


   
   PNG_ICC_CHECKSUM(0x0a3fd9f6, 0x3b8772b9,
      PNG_MD5(0x29f83dde, 0xaff255ae, 0x7842fae4, 0xca83390d), 0, 0,
      "2009/03/27 21:36:31", 3048, "sRGB_IEC61966-2-1_black_scaled.icc")

   
   PNG_ICC_CHECKSUM(0x4909e5e1, 0x427ebb21,
      PNG_MD5(0xc95bd637, 0xe95d8a3b, 0x0df38f99, 0xc1320389), 1, 0,
      "2009/03/27 21:37:45", 3052, "sRGB_IEC61966-2-1_no_black_scaling.icc")

   PNG_ICC_CHECKSUM(0xfd2144a1, 0x306fd8ae,
      PNG_MD5(0xfc663378, 0x37e2886b, 0xfd72e983, 0x8228f1b8), 0, 0,
      "2009/08/10 17:28:01", 60988, "sRGB_v4_ICC_preference_displayclass.icc")

   
   PNG_ICC_CHECKSUM(0x209c35d2, 0xbbef7812,
      PNG_MD5(0x34562abf, 0x994ccd06, 0x6d2c5721, 0xd0d68c5d), 0, 0,
      "2007/07/25 00:05:37", 60960, "sRGB_v4_ICC_preference.icc")

   




   PNG_ICC_CHECKSUM(0xa054d762, 0x5d5129ce,
      PNG_MD5(0x00000000, 0x00000000, 0x00000000, 0x00000000), 1, 0,
      "2004/07/21 18:57:42", 3024, "sRGB_IEC61966-2-1_noBPC.icc")

   






   PNG_ICC_CHECKSUM(0xf784f3fb, 0x182ea552,
      PNG_MD5(0x00000000, 0x00000000, 0x00000000, 0x00000000), 0, 1,
      "1998/02/09 06:49:00", 3144, "HP-Microsoft sRGB v2 perceptual")

   PNG_ICC_CHECKSUM(0x0398f3fc, 0xf29e526d,
      PNG_MD5(0x00000000, 0x00000000, 0x00000000, 0x00000000), 1, 1,
      "1998/02/09 06:49:00", 3144, "HP-Microsoft sRGB v2 media-relative")
};

static int
png_compare_ICC_profile_with_sRGB(png_const_structrp png_ptr,
   png_const_bytep profile, uLong adler)
{
   







   png_uint_32 length = 0;
   png_uint_32 intent = 0x10000; 
#if PNG_sRGB_PROFILE_CHECKS > 1
   uLong crc = 0; 
#endif
   unsigned int i;

#ifdef PNG_SET_OPTION_SUPPORTED
   
   if (((png_ptr->options >> PNG_SKIP_sRGB_CHECK_PROFILE) & 3) ==
               PNG_OPTION_ON)
      return 0;
#endif

   for (i=0; i < (sizeof png_sRGB_checks) / (sizeof png_sRGB_checks[0]); ++i)
   {
      if (png_get_uint_32(profile+84) == png_sRGB_checks[i].md5[0] &&
         png_get_uint_32(profile+88) == png_sRGB_checks[i].md5[1] &&
         png_get_uint_32(profile+92) == png_sRGB_checks[i].md5[2] &&
         png_get_uint_32(profile+96) == png_sRGB_checks[i].md5[3])
      {
         



#        if PNG_sRGB_PROFILE_CHECKS == 0
            if (png_sRGB_checks[i].have_md5 != 0)
               return 1+png_sRGB_checks[i].is_broken;
#        endif

         
         if (length == 0)
         {
            length = png_get_uint_32(profile);
            intent = png_get_uint_32(profile+64);
         }

         
         if (length == png_sRGB_checks[i].length &&
            intent == png_sRGB_checks[i].intent)
         {
            
            if (adler == 0)
            {
               adler = adler32(0, NULL, 0);
               adler = adler32(adler, profile, length);
            }

            if (adler == png_sRGB_checks[i].adler)
            {
               



#              if PNG_sRGB_PROFILE_CHECKS > 1
                  if (crc == 0)
                  {
                     crc = crc32(0, NULL, 0);
                     crc = crc32(crc, profile, length);
                  }

                  

                  if (crc == png_sRGB_checks[i].crc)
#              endif
               {
                  if (png_sRGB_checks[i].is_broken != 0)
                  {
                     




                     png_chunk_report(png_ptr, "known incorrect sRGB profile",
                        PNG_CHUNK_ERROR);
                  }

                  



                  else if (png_sRGB_checks[i].have_md5 == 0)
                  {
                     png_chunk_report(png_ptr,
                        "out-of-date sRGB profile with no signature",
                        PNG_CHUNK_WARNING);
                  }

                  return 1+png_sRGB_checks[i].is_broken;
               }
            }

# if PNG_sRGB_PROFILE_CHECKS > 0
         



         png_chunk_report(png_ptr,
             "Not recognizing known sRGB profile that has been edited", 
             PNG_CHUNK_WARNING);
         break;
# endif
         }
      }
   }

   return 0; 
}
#endif 

void 
png_icc_set_sRGB(png_const_structrp png_ptr,
   png_colorspacerp colorspace, png_const_bytep profile, uLong adler)
{
   


#if PNG_sRGB_PROFILE_CHECKS >= 0
   if (png_compare_ICC_profile_with_sRGB(png_ptr, profile, adler) != 0)
#endif
      (void)png_colorspace_set_sRGB(png_ptr, colorspace,
         (int)png_get_uint_32(profile+64));
}
#endif 

int 
png_colorspace_set_ICC(png_const_structrp png_ptr, png_colorspacerp colorspace,
   png_const_charp name, png_uint_32 profile_length, png_const_bytep profile,
   int color_type)
{
   if ((colorspace->flags & PNG_COLORSPACE_INVALID) != 0)
      return 0;

   if (png_icc_check_length(png_ptr, colorspace, name, profile_length) != 0 &&
       png_icc_check_header(png_ptr, colorspace, name, profile_length, profile,
          color_type) != 0 &&
       png_icc_check_tag_table(png_ptr, colorspace, name, profile_length,
          profile) != 0)
   {
#     ifdef PNG_sRGB_SUPPORTED
         
         png_icc_set_sRGB(png_ptr, colorspace, profile, 0);
#     endif
      return 1;
   }

   
   return 0;
}
#endif 

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
void 
png_colorspace_set_rgb_coefficients(png_structrp png_ptr)
{
   
   if (png_ptr->rgb_to_gray_coefficients_set == 0 &&
      (png_ptr->colorspace.flags & PNG_COLORSPACE_HAVE_ENDPOINTS) != 0)
   {
      


      png_fixed_point r = png_ptr->colorspace.end_points_XYZ.red_Y;
      png_fixed_point g = png_ptr->colorspace.end_points_XYZ.green_Y;
      png_fixed_point b = png_ptr->colorspace.end_points_XYZ.blue_Y;
      png_fixed_point total = r+g+b;

      if (total > 0 &&
         r >= 0 && png_muldiv(&r, r, 32768, total) && r >= 0 && r <= 32768 &&
         g >= 0 && png_muldiv(&g, g, 32768, total) && g >= 0 && g <= 32768 &&
         b >= 0 && png_muldiv(&b, b, 32768, total) && b >= 0 && b <= 32768 &&
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

         else
         {
            png_ptr->rgb_to_gray_red_coeff   = (png_uint_16)r;
            png_ptr->rgb_to_gray_green_coeff = (png_uint_16)g;
         }
      }

      



      else
         png_error(png_ptr, "internal error handling cHRM->XYZ");
   }
}
#endif 

#endif 

#ifdef __GNUC__

static int 
png_gt(size_t a, size_t b)
{
    return a > b;
}
#else
#   define png_gt(a,b) ((a) > (b))
#endif

void 
png_check_IHDR(png_const_structrp png_ptr,
   png_uint_32 width, png_uint_32 height, int bit_depth,
   int color_type, int interlace_type, int compression_type,
   int filter_type)
{
   int error = 0;

   
   if (width == 0)
   {
      png_warning(png_ptr, "Image width is zero in IHDR");
      error = 1;
   }

   if (width > PNG_UINT_31_MAX)
   {
      png_warning(png_ptr, "Invalid image width in IHDR");
      error = 1;
   }

   if (png_gt(((width + 7) & (~7)),
       ((PNG_SIZE_MAX
           - 48        
           - 1)        
           / 8)        
           - 1))       
   {
      










      png_warning(png_ptr, "Image width is too large for this architecture");
      error = 1;
   }

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
   if (width > png_ptr->user_width_max)
#else
   if (width > PNG_USER_WIDTH_MAX)
#endif
   {
      png_warning(png_ptr, "Image width exceeds user limit in IHDR");
      error = 1;
   }

   if (height == 0)
   {
      png_warning(png_ptr, "Image height is zero in IHDR");
      error = 1;
   }

   if (height > PNG_UINT_31_MAX)
   {
      png_warning(png_ptr, "Invalid image height in IHDR");
      error = 1;
   }

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
   if (height > png_ptr->user_height_max)
#else
   if (height > PNG_USER_HEIGHT_MAX)
#endif
   {
      png_warning(png_ptr, "Image height exceeds user limit in IHDR");
      error = 1;
   }

   
   if (bit_depth != 1 && bit_depth != 2 && bit_depth != 4 &&
       bit_depth != 8 && bit_depth != 16)
   {
      png_warning(png_ptr, "Invalid bit depth in IHDR");
      error = 1;
   }

   if (color_type < 0 || color_type == 1 ||
       color_type == 5 || color_type > 6)
   {
      png_warning(png_ptr, "Invalid color type in IHDR");
      error = 1;
   }

   if (((color_type == PNG_COLOR_TYPE_PALETTE) && bit_depth > 8) ||
       ((color_type == PNG_COLOR_TYPE_RGB ||
         color_type == PNG_COLOR_TYPE_GRAY_ALPHA ||
         color_type == PNG_COLOR_TYPE_RGB_ALPHA) && bit_depth < 8))
   {
      png_warning(png_ptr, "Invalid color type/bit depth combination in IHDR");
      error = 1;
   }

   if (interlace_type >= PNG_INTERLACE_LAST)
   {
      png_warning(png_ptr, "Unknown interlace method in IHDR");
      error = 1;
   }

   if (compression_type != PNG_COMPRESSION_TYPE_BASE)
   {
      png_warning(png_ptr, "Unknown compression method in IHDR");
      error = 1;
   }

#ifdef PNG_MNG_FEATURES_SUPPORTED
   








   if ((png_ptr->mode & PNG_HAVE_PNG_SIGNATURE) != 0 &&
       png_ptr->mng_features_permitted != 0)
      png_warning(png_ptr, "MNG features are not allowed in a PNG datastream");

   if (filter_type != PNG_FILTER_TYPE_BASE)
   {
      if (!((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) != 0 &&
          (filter_type == PNG_INTRAPIXEL_DIFFERENCING) &&
          ((png_ptr->mode & PNG_HAVE_PNG_SIGNATURE) == 0) &&
          (color_type == PNG_COLOR_TYPE_RGB ||
          color_type == PNG_COLOR_TYPE_RGB_ALPHA)))
      {
         png_warning(png_ptr, "Unknown filter method in IHDR");
         error = 1;
      }

      if ((png_ptr->mode & PNG_HAVE_PNG_SIGNATURE) != 0)
      {
         png_warning(png_ptr, "Invalid filter method in IHDR");
         error = 1;
      }
   }

#else
   if (filter_type != PNG_FILTER_TYPE_BASE)
   {
      png_warning(png_ptr, "Unknown filter method in IHDR");
      error = 1;
   }
#endif

   if (error == 1)
      png_error(png_ptr, "Invalid IHDR data");
}

#if defined(PNG_sCAL_SUPPORTED) || defined(PNG_pCAL_SUPPORTED)





#define png_fp_add(state, flags) ((state) |= (flags))
#define png_fp_set(state, value) ((state) = (value) | ((state) & PNG_FP_STICKY))

int 
png_check_fp_number(png_const_charp string, png_size_t size, int *statep,
   png_size_tp whereami)
{
   int state = *statep;
   png_size_t i = *whereami;

   while (i < size)
   {
      int type;
      
      switch (string[i])
      {
      case 43:  type = PNG_FP_SAW_SIGN;                   break;
      case 45:  type = PNG_FP_SAW_SIGN + PNG_FP_NEGATIVE; break;
      case 46:  type = PNG_FP_SAW_DOT;                    break;
      case 48:  type = PNG_FP_SAW_DIGIT;                  break;
      case 49: case 50: case 51: case 52:
      case 53: case 54: case 55: case 56:
      case 57:  type = PNG_FP_SAW_DIGIT + PNG_FP_NONZERO; break;
      case 69:
      case 101: type = PNG_FP_SAW_E;                      break;
      default:  goto PNG_FP_End;
      }

      



      switch ((state & PNG_FP_STATE) + (type & PNG_FP_SAW_ANY))
      {
      case PNG_FP_INTEGER + PNG_FP_SAW_SIGN:
         if ((state & PNG_FP_SAW_ANY) != 0)
            goto PNG_FP_End; 

         png_fp_add(state, type);
         break;

      case PNG_FP_INTEGER + PNG_FP_SAW_DOT:
         
         if ((state & PNG_FP_SAW_DOT) != 0) 
            goto PNG_FP_End;

         else if ((state & PNG_FP_SAW_DIGIT) != 0) 
            png_fp_add(state, type);

         else
            png_fp_set(state, PNG_FP_FRACTION | type);

         break;

      case PNG_FP_INTEGER + PNG_FP_SAW_DIGIT:
         if ((state & PNG_FP_SAW_DOT) != 0) 
            png_fp_set(state, PNG_FP_FRACTION | PNG_FP_SAW_DOT);

         png_fp_add(state, type | PNG_FP_WAS_VALID);

         break;

      case PNG_FP_INTEGER + PNG_FP_SAW_E:
         if ((state & PNG_FP_SAW_DIGIT) == 0)
            goto PNG_FP_End;

         png_fp_set(state, PNG_FP_EXPONENT);

         break;

   


   


      case PNG_FP_FRACTION + PNG_FP_SAW_DIGIT:
         png_fp_add(state, type | PNG_FP_WAS_VALID);
         break;

      case PNG_FP_FRACTION + PNG_FP_SAW_E:
         



         if ((state & PNG_FP_SAW_DIGIT) == 0)
            goto PNG_FP_End;

         png_fp_set(state, PNG_FP_EXPONENT);

         break;

      case PNG_FP_EXPONENT + PNG_FP_SAW_SIGN:
         if ((state & PNG_FP_SAW_ANY) != 0)
            goto PNG_FP_End; 

         png_fp_add(state, PNG_FP_SAW_SIGN);

         break;

   


      case PNG_FP_EXPONENT + PNG_FP_SAW_DIGIT:
         png_fp_add(state, PNG_FP_SAW_DIGIT | PNG_FP_WAS_VALID);

         break;

   


      default: goto PNG_FP_End; 
      }

      
      ++i;
   }

PNG_FP_End:
   


   *statep = state;
   *whereami = i;

   return (state & PNG_FP_SAW_DIGIT) != 0;
}



int
png_check_fp_string(png_const_charp string, png_size_t size)
{
   int        state=0;
   png_size_t char_index=0;

   if (png_check_fp_number(string, size, &state, &char_index) != 0 &&
      (char_index == size || string[char_index] == 0))
      return state ;

   return 0; 
}
#endif 

#ifdef PNG_sCAL_SUPPORTED
#  ifdef PNG_FLOATING_POINT_SUPPORTED



static double
png_pow10(int power)
{
   int recip = 0;
   double d = 1;

   


   if (power < 0)
   {
      if (power < DBL_MIN_10_EXP) return 0;
      recip = 1, power = -power;
   }

   if (power > 0)
   {
      
      double mult = 10;
      do
      {
         if (power & 1) d *= mult;
         mult *= mult;
         power >>= 1;
      }
      while (power > 0);

      if (recip != 0) d = 1/d;
   }
   

   return d;
}




void 
png_ascii_from_fp(png_const_structrp png_ptr, png_charp ascii, png_size_t size,
    double fp, unsigned int precision)
{
   




   if (precision < 1)
      precision = DBL_DIG;

   
   if (precision > DBL_DIG+1)
      precision = DBL_DIG+1;

   
   if (size >= precision+5) 
   {
      if (fp < 0)
      {
         fp = -fp;
         *ascii++ = 45; 
         --size;
      }

      if (fp >= DBL_MIN && fp <= DBL_MAX)
      {
         int exp_b10;       
         double base;   

         








         (void)frexp(fp, &exp_b10); 

         exp_b10 = (exp_b10 * 77) >> 8; 

         
         base = png_pow10(exp_b10); 

         while (base < DBL_MIN || base < fp)
         {
            
            double test = png_pow10(exp_b10+1);

            if (test <= DBL_MAX)
               ++exp_b10, base = test;

            else
               break;
         }

         






         fp /= base;
         while (fp >= 1) fp /= 10, ++exp_b10;

         





         {
            int czero, clead, cdigits;
            char exponent[10];

            


            if (exp_b10 < 0 && exp_b10 > -3) 
            {
               czero = -exp_b10; 
               exp_b10 = 0;      
            }
            else
               czero = 0;    

            


            clead = czero; 
            cdigits = 0;   

            do
            {
               double d;

               fp *= 10;
               




               if (cdigits+czero-clead+1 < (int)precision)
                  fp = modf(fp, &d);

               else
               {
                  d = floor(fp + .5);

                  if (d > 9)
                  {
                     
                     if (czero > 0)
                     {
                        --czero, d = 1;
                        if (cdigits == 0) --clead;
                     }
                     else
                     {
                        while (cdigits > 0 && d > 9)
                        {
                           int ch = *--ascii;

                           if (exp_b10 != (-1))
                              ++exp_b10;

                           else if (ch == 46)
                           {
                              ch = *--ascii, ++size;
                              



                              exp_b10 = 1;
                           }

                           --cdigits;
                           d = ch - 47;  
                        }

                        



                        if (d > 9)  
                        {
                           if (exp_b10 == (-1))
                           {
                              



                              int ch = *--ascii;

                              if (ch == 46)
                                 ++size, exp_b10 = 1;

                              


                           }
                           else
                              ++exp_b10;

                           
                           d = 1;
                        }
                     }
                  }
                  fp = 0; 
               }

               if (d == 0)
               {
                  ++czero;
                  if (cdigits == 0) ++clead;
               }
               else
               {
                  
                  cdigits += czero - clead;
                  clead = 0;

                  while (czero > 0)
                  {
                     



                     if (exp_b10 != (-1))
                     {
                        if (exp_b10 == 0) *ascii++ = 46, --size;
                        
                        --exp_b10;
                     }
                     *ascii++ = 48, --czero;
                  }

                  if (exp_b10 != (-1))
                  {
                     if (exp_b10 == 0)
                        *ascii++ = 46, --size; 

                     --exp_b10;
                  }
                  *ascii++ = (char)(48 + (int)d), ++cdigits;
               }
            }
            while (cdigits+czero-clead < (int)precision && fp > DBL_MIN);

            

            






            if (exp_b10 >= (-1) && exp_b10 <= 2)
            {
               






               while (--exp_b10 >= 0) *ascii++ = 48;

               *ascii = 0;

               


               return;
            }

            





            size -= cdigits;

            *ascii++ = 69, --size;    

            



            {
               unsigned int uexp_b10;

               if (exp_b10 < 0)
               {
                  *ascii++ = 45, --size; 
                  uexp_b10 = -exp_b10;
               }

               else
                  uexp_b10 = exp_b10;

               cdigits = 0;

               while (uexp_b10 > 0)
               {
                  exponent[cdigits++] = (char)(48 + uexp_b10 % 10);
                  uexp_b10 /= 10;
               }
            }

            


            if ((int)size > cdigits)
            {
               while (cdigits > 0) *ascii++ = exponent[--cdigits];

               *ascii = 0;

               return;
            }
         }
      }
      else if (!(fp >= DBL_MIN))
      {
         *ascii++ = 48; 
         *ascii = 0;
         return;
      }
      else
      {
         *ascii++ = 105; 
         *ascii++ = 110; 
         *ascii++ = 102; 
         *ascii = 0;
         return;
      }
   }

   
   png_error(png_ptr, "ASCII conversion buffer too small");
}

#  endif 

#  ifdef PNG_FIXED_POINT_SUPPORTED


void 
png_ascii_from_fixed(png_const_structrp png_ptr, png_charp ascii,
    png_size_t size, png_fixed_point fp)
{
   


   if (size > 12)
   {
      png_uint_32 num;

      
      if (fp < 0)
         *ascii++ = 45, --size, num = -fp;
      else
         num = fp;

      if (num <= 0x80000000) 
      {
         unsigned int ndigits = 0, first = 16 ;
         char digits[10];

         while (num)
         {
            
            unsigned int tmp = num/10;
            num -= tmp*10;
            digits[ndigits++] = (char)(48 + num);
            


            if (first == 16 && num > 0)
               first = ndigits;
            num = tmp;
         }

         if (ndigits > 0)
         {
            while (ndigits > 5) *ascii++ = digits[--ndigits];
            



            if (first <= 5)
            {
               unsigned int i;
               *ascii++ = 46; 
               


               i = 5;
               while (ndigits < i) *ascii++ = 48, --i;
               while (ndigits >= first) *ascii++ = digits[--ndigits];
               
            }
         }
         else
            *ascii++ = 48;

         
         *ascii = 0;
         return;
      }
   }

   
   png_error(png_ptr, "ASCII conversion buffer too small");
}
#   endif 
#endif 

#if defined(PNG_FLOATING_POINT_SUPPORTED) && \
   !defined(PNG_FIXED_POINT_MACRO_SUPPORTED) && \
   (defined(PNG_gAMA_SUPPORTED) || defined(PNG_cHRM_SUPPORTED) || \
   defined(PNG_sCAL_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)) || \
   (defined(PNG_sCAL_SUPPORTED) && \
   defined(PNG_FLOATING_ARITHMETIC_SUPPORTED))
png_fixed_point
png_fixed(png_const_structrp png_ptr, double fp, png_const_charp text)
{
   double r = floor(100000 * fp + .5);

   if (r > 2147483647. || r < -2147483648.)
      png_fixed_error(png_ptr, text);

#  ifndef PNG_ERROR_TEXT_SUPPORTED
   PNG_UNUSED(text)
#  endif

   return (png_fixed_point)r;
}
#endif

#if defined(PNG_GAMMA_SUPPORTED) || defined(PNG_COLORSPACE_SUPPORTED) ||\
    defined(PNG_INCH_CONVERSIONS_SUPPORTED) || defined(PNG_READ_pHYs_SUPPORTED)






int
png_muldiv(png_fixed_point_p res, png_fixed_point a, png_int_32 times,
    png_int_32 divisor)
{
   
   if (divisor != 0)
   {
      if (a == 0 || times == 0)
      {
         *res = 0;
         return 1;
      }
      else
      {
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
         double r = a;
         r *= times;
         r /= divisor;
         r = floor(r+.5);

         
         if (r <= 2147483647. && r >= -2147483648.)
         {
            *res = (png_fixed_point)r;
            return 1;
         }
#else
         int negative = 0;
         png_uint_32 A, T, D;
         png_uint_32 s16, s32, s00;

         if (a < 0)
            negative = 1, A = -a;
         else
            A = a;

         if (times < 0)
            negative = !negative, T = -times;
         else
            T = times;

         if (divisor < 0)
            negative = !negative, D = -divisor;
         else
            D = divisor;

         


         s16 = (A >> 16) * (T & 0xffff) +
                           (A & 0xffff) * (T >> 16);
         


         s32 = (A >> 16) * (T >> 16) + (s16 >> 16);
         s00 = (A & 0xffff) * (T & 0xffff);

         s16 = (s16 & 0xffff) << 16;
         s00 += s16;

         if (s00 < s16)
            ++s32; 

         if (s32 < D) 
         {
            



            int bitshift = 32;
            png_fixed_point result = 0; 

            while (--bitshift >= 0)
            {
               png_uint_32 d32, d00;

               if (bitshift > 0)
                  d32 = D >> (32-bitshift), d00 = D << bitshift;

               else
                  d32 = 0, d00 = D;

               if (s32 > d32)
               {
                  if (s00 < d00) --s32; 
                  s32 -= d32, s00 -= d00, result += 1<<bitshift;
               }

               else
                  if (s32 == d32 && s00 >= d00)
                     s32 = 0, s00 -= d00, result += 1<<bitshift;
            }

            
            if (s00 >= (D >> 1))
               ++result;

            if (negative != 0)
               result = -result;

            
            if ((negative != 0 && result <= 0) ||
                (negative == 0 && result >= 0))
            {
               *res = result;
               return 1;
            }
         }
#endif
      }
   }

   return 0;
}
#endif 

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_INCH_CONVERSIONS_SUPPORTED)



png_fixed_point
png_muldiv_warn(png_const_structrp png_ptr, png_fixed_point a, png_int_32 times,
    png_int_32 divisor)
{
   png_fixed_point result;

   if (png_muldiv(&result, a, times, divisor) != 0)
      return result;

   png_warning(png_ptr, "fixed point overflow ignored");
   return 0;
}
#endif

#ifdef PNG_GAMMA_SUPPORTED 

png_fixed_point
png_reciprocal(png_fixed_point a)
{
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
   double r = floor(1E10/a+.5);

   if (r <= 2147483647. && r >= -2147483648.)
      return (png_fixed_point)r;
#else
   png_fixed_point res;

   if (png_muldiv(&res, 100000, 100000, a) != 0)
      return res;
#endif

   return 0; 
}




int 
png_gamma_significant(png_fixed_point gamma_val)
{
   return gamma_val < PNG_FP_1 - PNG_GAMMA_THRESHOLD_FIXED ||
       gamma_val > PNG_FP_1 + PNG_GAMMA_THRESHOLD_FIXED;
}
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED
#ifdef PNG_16BIT_SUPPORTED

static png_fixed_point
png_product2(png_fixed_point a, png_fixed_point b)
{
   
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
   double r = a * 1E-5;
   r *= b;
   r = floor(r+.5);

   if (r <= 2147483647. && r >= -2147483648.)
      return (png_fixed_point)r;
#else
   png_fixed_point res;

   if (png_muldiv(&res, a, b, 100000) != 0)
      return res;
#endif

   return 0; 
}
#endif 


png_fixed_point
png_reciprocal2(png_fixed_point a, png_fixed_point b)
{
   
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
   if (a != 0 && b != 0)
   {
      double r = 1E15/a;
      r /= b;
      r = floor(r+.5);

      if (r <= 2147483647. && r >= -2147483648.)
         return (png_fixed_point)r;
   }
#else
   




   png_fixed_point res = png_product2(a, b);

   if (res != 0)
      return png_reciprocal(res);
#endif

   return 0; 
}
#endif 

#ifdef PNG_READ_GAMMA_SUPPORTED 
#ifndef PNG_FLOATING_ARITHMETIC_SUPPORTED

















static const png_uint_32
png_8bit_l2[128] =
{
   4270715492U, 4222494797U, 4174646467U, 4127164793U, 4080044201U, 4033279239U,
   3986864580U, 3940795015U, 3895065449U, 3849670902U, 3804606499U, 3759867474U,
   3715449162U, 3671346997U, 3627556511U, 3584073329U, 3540893168U, 3498011834U,
   3455425220U, 3413129301U, 3371120137U, 3329393864U, 3287946700U, 3246774933U,
   3205874930U, 3165243125U, 3124876025U, 3084770202U, 3044922296U, 3005329011U,
   2965987113U, 2926893432U, 2888044853U, 2849438323U, 2811070844U, 2772939474U,
   2735041326U, 2697373562U, 2659933400U, 2622718104U, 2585724991U, 2548951424U,
   2512394810U, 2476052606U, 2439922311U, 2404001468U, 2368287663U, 2332778523U,
   2297471715U, 2262364947U, 2227455964U, 2192742551U, 2158222529U, 2123893754U,
   2089754119U, 2055801552U, 2022034013U, 1988449497U, 1955046031U, 1921821672U,
   1888774511U, 1855902668U, 1823204291U, 1790677560U, 1758320682U, 1726131893U,
   1694109454U, 1662251657U, 1630556815U, 1599023271U, 1567649391U, 1536433567U,
   1505374214U, 1474469770U, 1443718700U, 1413119487U, 1382670639U, 1352370686U,
   1322218179U, 1292211689U, 1262349810U, 1232631153U, 1203054352U, 1173618059U,
   1144320946U, 1115161701U, 1086139034U, 1057251672U, 1028498358U, 999877854U,
   971388940U, 943030410U, 914801076U, 886699767U, 858725327U, 830876614U,
   803152505U, 775551890U, 748073672U, 720716771U, 693480120U, 666362667U,
   639363374U, 612481215U, 585715177U, 559064263U, 532527486U, 506103872U,
   479792461U, 453592303U, 427502463U, 401522014U, 375650043U, 349885648U,
   324227938U, 298676034U, 273229066U, 247886176U, 222646516U, 197509248U,
   172473545U, 147538590U, 122703574U, 97967701U, 73330182U, 48790236U,
   24347096U, 0U

#if 0
   




   65166, 64430, 63700, 62976, 62257, 61543, 60835, 60132, 59434, 58741, 58054,
   57371, 56693, 56020, 55352, 54689, 54030, 53375, 52726, 52080, 51439, 50803,
   50170, 49542, 48918, 48298, 47682, 47070, 46462, 45858, 45257, 44661, 44068,
   43479, 42894, 42312, 41733, 41159, 40587, 40020, 39455, 38894, 38336, 37782,
   37230, 36682, 36137, 35595, 35057, 34521, 33988, 33459, 32932, 32408, 31887,
   31369, 30854, 30341, 29832, 29325, 28820, 28319, 27820, 27324, 26830, 26339,
   25850, 25364, 24880, 24399, 23920, 23444, 22970, 22499, 22029, 21562, 21098,
   20636, 20175, 19718, 19262, 18808, 18357, 17908, 17461, 17016, 16573, 16132,
   15694, 15257, 14822, 14390, 13959, 13530, 13103, 12678, 12255, 11834, 11415,
   10997, 10582, 10168, 9756, 9346, 8937, 8531, 8126, 7723, 7321, 6921, 6523,
   6127, 5732, 5339, 4947, 4557, 4169, 3782, 3397, 3014, 2632, 2251, 1872, 1495,
   1119, 744, 372
#endif
};

static png_int_32
png_log8bit(unsigned int x)
{
   unsigned int lg2 = 0;
   





   if ((x &= 0xff) == 0)
      return -1;

   if ((x & 0xf0) == 0)
      lg2  = 4, x <<= 4;

   if ((x & 0xc0) == 0)
      lg2 += 2, x <<= 2;

   if ((x & 0x80) == 0)
      lg2 += 1, x <<= 1;

   
   return (png_int_32)((lg2 << 16) + ((png_8bit_l2[x-128]+32768)>>16));
}































#ifdef PNG_16BIT_SUPPORTED
static png_int_32
png_log16bit(png_uint_32 x)
{
   unsigned int lg2 = 0;

   
   if ((x &= 0xffff) == 0)
      return -1;

   if ((x & 0xff00) == 0)
      lg2  = 8, x <<= 8;

   if ((x & 0xf000) == 0)
      lg2 += 4, x <<= 4;

   if ((x & 0xc000) == 0)
      lg2 += 2, x <<= 2;

   if ((x & 0x8000) == 0)
      lg2 += 1, x <<= 1;

   


   lg2 <<= 28;
   lg2 += (png_8bit_l2[(x>>8)-128]+8) >> 4;

   


   x = ((x << 16) + (x >> 9)) / (x >> 8);

   






   x -= 1U << 24;

   if (x <= 65536U) 
      lg2 += ((23591U * (65536U-x)) + (1U << (16+6-12-1))) >> (16+6-12);

   else
      lg2 -= ((23499U * (x-65536U)) + (1U << (16+6-12-1))) >> (16+6-12);

   
   return (png_int_32)((lg2 + 2048) >> 12);
}
#endif 















static const png_uint_32
png_32bit_exp[16] =
{
   
   4294967295U, 4112874773U, 3938502376U, 3771522796U, 3611622603U, 3458501653U,
   3311872529U, 3171459999U, 3037000500U, 2908241642U, 2784941738U, 2666869345U,
   2553802834U, 2445529972U, 2341847524U, 2242560872U
};


#if 0
for (i=11;i>=0;--i){ print i, " ", (1 - e(-(2^i)/65536*l(2))) * 2^(32-i), "\n"}
   11 44937.64284865548751208448
   10 45180.98734845585101160448
    9 45303.31936980687359311872
    8 45364.65110595323018870784
    7 45395.35850361789624614912
    6 45410.72259715102037508096
    5 45418.40724413220722311168
    4 45422.25021786898173001728
    3 45424.17186732298419044352
    2 45425.13273269940811464704
    1 45425.61317555035558641664
    0 45425.85339951654943850496
#endif

static png_uint_32
png_exp(png_fixed_point x)
{
   if (x > 0 && x <= 0xfffff) 
   {
      
      png_uint_32 e = png_32bit_exp[(x >> 12) & 0xf];

      





      if (x & 0x800)
         e -= (((e >> 16) * 44938U) +  16U) >> 5;

      if (x & 0x400)
         e -= (((e >> 16) * 45181U) +  32U) >> 6;

      if (x & 0x200)
         e -= (((e >> 16) * 45303U) +  64U) >> 7;

      if (x & 0x100)
         e -= (((e >> 16) * 45365U) + 128U) >> 8;

      if (x & 0x080)
         e -= (((e >> 16) * 45395U) + 256U) >> 9;

      if (x & 0x040)
         e -= (((e >> 16) * 45410U) + 512U) >> 10;

      
      e -= (((e >> 16) * 355U * (x & 0x3fU)) + 256U) >> 9;

      
      e >>= x >> 16;
      return e;
   }

   
   if (x <= 0)
      return png_32bit_exp[0];

   
   return 0;
}

static png_byte
png_exp8bit(png_fixed_point lg2)
{
   
   png_uint_32 x = png_exp(lg2);

   



   x -= x >> 8;
   return (png_byte)(((x + 0x7fffffU) >> 24) & 0xff);
}

#ifdef PNG_16BIT_SUPPORTED
static png_uint_16
png_exp16bit(png_fixed_point lg2)
{
   
   png_uint_32 x = png_exp(lg2);

   
   x -= x >> 16;
   return (png_uint_16)((x + 32767U) >> 16);
}
#endif 
#endif 

png_byte
png_gamma_8bit_correct(unsigned int value, png_fixed_point gamma_val)
{
   if (value > 0 && value < 255)
   {
#     ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
         























         double r = floor(255*pow((int)value/255.,gamma_val*.00001)+.5);
         return (png_byte)r;
#     else
         png_int_32 lg2 = png_log8bit(value);
         png_fixed_point res;

         if (png_muldiv(&res, gamma_val, lg2, PNG_FP_1) != 0)
            return png_exp8bit(res);

         
         value = 0;
#     endif
   }

   return (png_byte)(value & 0xff);
}

#ifdef PNG_16BIT_SUPPORTED
png_uint_16
png_gamma_16bit_correct(unsigned int value, png_fixed_point gamma_val)
{
   if (value > 0 && value < 65535)
   {
#     ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
         




         double r = floor(65535*pow((png_int_32)value/65535.,
                     gamma_val*.00001)+.5);
         return (png_uint_16)r;
#     else
         png_int_32 lg2 = png_log16bit(value);
         png_fixed_point res;

         if (png_muldiv(&res, gamma_val, lg2, PNG_FP_1) != 0)
            return png_exp16bit(res);

         
         value = 0;
#     endif
   }

   return (png_uint_16)value;
}
#endif 






png_uint_16 
png_gamma_correct(png_structrp png_ptr, unsigned int value,
    png_fixed_point gamma_val)
{
   if (png_ptr->bit_depth == 8)
      return png_gamma_8bit_correct(value, gamma_val);

#ifdef PNG_16BIT_SUPPORTED
   else
      return png_gamma_16bit_correct(value, gamma_val);
#else
      
      return 0;
#endif 
}

#ifdef PNG_16BIT_SUPPORTED








static void
png_build_16bit_table(png_structrp png_ptr, png_uint_16pp *ptable,
   PNG_CONST unsigned int shift, PNG_CONST png_fixed_point gamma_val)
{
   
   PNG_CONST unsigned int num = 1U << (8U - shift);
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
   


   PNG_CONST double fmax = 1./(((png_int_32)1 << (16U - shift))-1);
#endif
   PNG_CONST unsigned int max = (1U << (16U - shift))-1U;
   PNG_CONST unsigned int max_by_2 = 1U << (15U-shift);
   unsigned int i;

   png_uint_16pp table = *ptable =
       (png_uint_16pp)png_calloc(png_ptr, num * (sizeof (png_uint_16p)));

   for (i = 0; i < num; i++)
   {
      png_uint_16p sub_table = table[i] =
          (png_uint_16p)png_malloc(png_ptr, 256 * (sizeof (png_uint_16)));

      


      if (png_gamma_significant(gamma_val) != 0)
      {
         







         unsigned int j;
         for (j = 0; j < 256; j++)
         {
            png_uint_32 ig = (j << (8-shift)) + i;
#           ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
               
               


               double d = floor(65535.*pow(ig*fmax, gamma_val*.00001)+.5);
               sub_table[j] = (png_uint_16)d;
#           else
               if (shift != 0)
                  ig = (ig * 65535U + max_by_2)/max;

               sub_table[j] = png_gamma_16bit_correct(ig, gamma_val);
#           endif
         }
      }
      else
      {
         
         unsigned int j;

         for (j = 0; j < 256; j++)
         {
            png_uint_32 ig = (j << (8-shift)) + i;

            if (shift != 0)
               ig = (ig * 65535U + max_by_2)/max;

            sub_table[j] = (png_uint_16)ig;
         }
      }
   }
}




static void
png_build_16to8_table(png_structrp png_ptr, png_uint_16pp *ptable,
   PNG_CONST unsigned int shift, PNG_CONST png_fixed_point gamma_val)
{
   PNG_CONST unsigned int num = 1U << (8U - shift);
   PNG_CONST unsigned int max = (1U << (16U - shift))-1U;
   unsigned int i;
   png_uint_32 last;

   png_uint_16pp table = *ptable =
       (png_uint_16pp)png_calloc(png_ptr, num * (sizeof (png_uint_16p)));

   



   for (i = 0; i < num; i++)
      table[i] = (png_uint_16p)png_malloc(png_ptr,
          256 * (sizeof (png_uint_16)));

   















   last = 0;
   for (i = 0; i < 255; ++i) 
   {
      
      png_uint_16 out = (png_uint_16)(i * 257U); 

      
      png_uint_32 bound = png_gamma_16bit_correct(out+128U, gamma_val);

      
      bound = (bound * max + 32768U)/65535U + 1U;

      while (last < bound)
      {
         table[last & (0xffU >> shift)][last >> (8U - shift)] = out;
         last++;
      }
   }

   
   while (last < (num << 8))
   {
      table[last & (0xff >> shift)][last >> (8U - shift)] = 65535U;
      last++;
   }
}
#endif 





static void
png_build_8bit_table(png_structrp png_ptr, png_bytepp ptable,
   PNG_CONST png_fixed_point gamma_val)
{
   unsigned int i;
   png_bytep table = *ptable = (png_bytep)png_malloc(png_ptr, 256);

   if (png_gamma_significant(gamma_val) != 0)
      for (i=0; i<256; i++)
         table[i] = png_gamma_8bit_correct(i, gamma_val);

   else
      for (i=0; i<256; ++i)
         table[i] = (png_byte)(i & 0xff);
}




void 
png_destroy_gamma_table(png_structrp png_ptr)
{
   png_free(png_ptr, png_ptr->gamma_table);
   png_ptr->gamma_table = NULL;

#ifdef PNG_16BIT_SUPPORTED
   if (png_ptr->gamma_16_table != NULL)
   {
      int i;
      int istop = (1 << (8 - png_ptr->gamma_shift));
      for (i = 0; i < istop; i++)
      {
         png_free(png_ptr, png_ptr->gamma_16_table[i]);
      }
   png_free(png_ptr, png_ptr->gamma_16_table);
   png_ptr->gamma_16_table = NULL;
   }
#endif 

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_ALPHA_MODE_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
   png_free(png_ptr, png_ptr->gamma_from_1);
   png_ptr->gamma_from_1 = NULL;
   png_free(png_ptr, png_ptr->gamma_to_1);
   png_ptr->gamma_to_1 = NULL;

#ifdef PNG_16BIT_SUPPORTED
   if (png_ptr->gamma_16_from_1 != NULL)
   {
      int i;
      int istop = (1 << (8 - png_ptr->gamma_shift));
      for (i = 0; i < istop; i++)
      {
         png_free(png_ptr, png_ptr->gamma_16_from_1[i]);
      }
   png_free(png_ptr, png_ptr->gamma_16_from_1);
   png_ptr->gamma_16_from_1 = NULL;
   }
   if (png_ptr->gamma_16_to_1 != NULL)
   {
      int i;
      int istop = (1 << (8 - png_ptr->gamma_shift));
      for (i = 0; i < istop; i++)
      {
         png_free(png_ptr, png_ptr->gamma_16_to_1[i]);
      }
   png_free(png_ptr, png_ptr->gamma_16_to_1);
   png_ptr->gamma_16_to_1 = NULL;
   }
#endif 
#endif 
}






void 
png_build_gamma_table(png_structrp png_ptr, int bit_depth)
{
  png_debug(1, "in png_build_gamma_table");

  





  if (png_ptr->gamma_table != NULL || png_ptr->gamma_16_table != NULL)
  {
    png_warning(png_ptr, "gamma table being rebuilt");
    png_destroy_gamma_table(png_ptr);
  }

  if (bit_depth <= 8)
  {
     png_build_8bit_table(png_ptr, &png_ptr->gamma_table,
         png_ptr->screen_gamma > 0 ?  png_reciprocal2(png_ptr->colorspace.gamma,
         png_ptr->screen_gamma) : PNG_FP_1);

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_ALPHA_MODE_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
     if ((png_ptr->transformations & (PNG_COMPOSE | PNG_RGB_TO_GRAY)) != 0)
     {
        png_build_8bit_table(png_ptr, &png_ptr->gamma_to_1,
            png_reciprocal(png_ptr->colorspace.gamma));

        png_build_8bit_table(png_ptr, &png_ptr->gamma_from_1,
            png_ptr->screen_gamma > 0 ?  png_reciprocal(png_ptr->screen_gamma) :
            png_ptr->colorspace.gamma);
     }
#endif 
  }
#ifdef PNG_16BIT_SUPPORTED
  else
  {
     png_byte shift, sig_bit;

     if ((png_ptr->color_type & PNG_COLOR_MASK_COLOR) != 0)
     {
        sig_bit = png_ptr->sig_bit.red;

        if (png_ptr->sig_bit.green > sig_bit)
           sig_bit = png_ptr->sig_bit.green;

        if (png_ptr->sig_bit.blue > sig_bit)
           sig_bit = png_ptr->sig_bit.blue;
     }
     else
        sig_bit = png_ptr->sig_bit.gray;

     

















     if (sig_bit > 0 && sig_bit < 16U)
        
        shift = (png_byte)((16U - sig_bit) & 0xff);

     else
        shift = 0; 

     if ((png_ptr->transformations & (PNG_16_TO_8 | PNG_SCALE_16_TO_8)) != 0)
     {
        



        if (shift < (16U - PNG_MAX_GAMMA_8))
           shift = (16U - PNG_MAX_GAMMA_8);
     }

     if (shift > 8U)
        shift = 8U; 

     png_ptr->gamma_shift = shift;

     




     if ((png_ptr->transformations & (PNG_16_TO_8 | PNG_SCALE_16_TO_8)) != 0)
         png_build_16to8_table(png_ptr, &png_ptr->gamma_16_table, shift,
         png_ptr->screen_gamma > 0 ? png_product2(png_ptr->colorspace.gamma,
         png_ptr->screen_gamma) : PNG_FP_1);

     else
         png_build_16bit_table(png_ptr, &png_ptr->gamma_16_table, shift,
         png_ptr->screen_gamma > 0 ? png_reciprocal2(png_ptr->colorspace.gamma,
         png_ptr->screen_gamma) : PNG_FP_1);

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_ALPHA_MODE_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
     if ((png_ptr->transformations & (PNG_COMPOSE | PNG_RGB_TO_GRAY)) != 0)
     {
        png_build_16bit_table(png_ptr, &png_ptr->gamma_16_to_1, shift,
            png_reciprocal(png_ptr->colorspace.gamma));

        



        png_build_16bit_table(png_ptr, &png_ptr->gamma_16_from_1, shift,
            png_ptr->screen_gamma > 0 ? png_reciprocal(png_ptr->screen_gamma) :
            png_ptr->colorspace.gamma);
     }
#endif 
  }
#endif 
}
#endif 


#ifdef PNG_SET_OPTION_SUPPORTED
int PNGAPI
png_set_option(png_structrp png_ptr, int option, int onoff)
{
   if (png_ptr != NULL && option >= 0 && option < PNG_OPTION_NEXT &&
      (option & 1) == 0)
   {
      int mask = 3 << option;
      int setting = (2 + (onoff != 0)) << option;
      int current = png_ptr->options;

      png_ptr->options = (png_byte)(((current & ~mask) | setting) & 0xff);

      return (current & mask) >> option;
   }

   return PNG_OPTION_INVALID;
}
#endif


#if defined(PNG_SIMPLIFIED_READ_SUPPORTED) ||\
   defined(PNG_SIMPLIFIED_WRITE_SUPPORTED)


















#ifdef PNG_SIMPLIFIED_READ_SUPPORTED

const png_uint_16 png_sRGB_table[256] =
{
   0,20,40,60,80,99,119,139,
   159,179,199,219,241,264,288,313,
   340,367,396,427,458,491,526,562,
   599,637,677,718,761,805,851,898,
   947,997,1048,1101,1156,1212,1270,1330,
   1391,1453,1517,1583,1651,1720,1790,1863,
   1937,2013,2090,2170,2250,2333,2418,2504,
   2592,2681,2773,2866,2961,3058,3157,3258,
   3360,3464,3570,3678,3788,3900,4014,4129,
   4247,4366,4488,4611,4736,4864,4993,5124,
   5257,5392,5530,5669,5810,5953,6099,6246,
   6395,6547,6700,6856,7014,7174,7335,7500,
   7666,7834,8004,8177,8352,8528,8708,8889,
   9072,9258,9445,9635,9828,10022,10219,10417,
   10619,10822,11028,11235,11446,11658,11873,12090,
   12309,12530,12754,12980,13209,13440,13673,13909,
   14146,14387,14629,14874,15122,15371,15623,15878,
   16135,16394,16656,16920,17187,17456,17727,18001,
   18277,18556,18837,19121,19407,19696,19987,20281,
   20577,20876,21177,21481,21787,22096,22407,22721,
   23038,23357,23678,24002,24329,24658,24990,25325,
   25662,26001,26344,26688,27036,27386,27739,28094,
   28452,28813,29176,29542,29911,30282,30656,31033,
   31412,31794,32179,32567,32957,33350,33745,34143,
   34544,34948,35355,35764,36176,36591,37008,37429,
   37852,38278,38706,39138,39572,40009,40449,40891,
   41337,41785,42236,42690,43147,43606,44069,44534,
   45002,45473,45947,46423,46903,47385,47871,48359,
   48850,49344,49841,50341,50844,51349,51858,52369,
   52884,53401,53921,54445,54971,55500,56032,56567,
   57105,57646,58190,58737,59287,59840,60396,60955,
   61517,62082,62650,63221,63795,64372,64952,65535
};
#endif 




const png_uint_16 png_sRGB_base[512] =
{
   128,1782,3383,4644,5675,6564,7357,8074,
   8732,9346,9921,10463,10977,11466,11935,12384,
   12816,13233,13634,14024,14402,14769,15125,15473,
   15812,16142,16466,16781,17090,17393,17690,17981,
   18266,18546,18822,19093,19359,19621,19879,20133,
   20383,20630,20873,21113,21349,21583,21813,22041,
   22265,22487,22707,22923,23138,23350,23559,23767,
   23972,24175,24376,24575,24772,24967,25160,25352,
   25542,25730,25916,26101,26284,26465,26645,26823,
   27000,27176,27350,27523,27695,27865,28034,28201,
   28368,28533,28697,28860,29021,29182,29341,29500,
   29657,29813,29969,30123,30276,30429,30580,30730,
   30880,31028,31176,31323,31469,31614,31758,31902,
   32045,32186,32327,32468,32607,32746,32884,33021,
   33158,33294,33429,33564,33697,33831,33963,34095,
   34226,34357,34486,34616,34744,34873,35000,35127,
   35253,35379,35504,35629,35753,35876,35999,36122,
   36244,36365,36486,36606,36726,36845,36964,37083,
   37201,37318,37435,37551,37668,37783,37898,38013,
   38127,38241,38354,38467,38580,38692,38803,38915,
   39026,39136,39246,39356,39465,39574,39682,39790,
   39898,40005,40112,40219,40325,40431,40537,40642,
   40747,40851,40955,41059,41163,41266,41369,41471,
   41573,41675,41777,41878,41979,42079,42179,42279,
   42379,42478,42577,42676,42775,42873,42971,43068,
   43165,43262,43359,43456,43552,43648,43743,43839,
   43934,44028,44123,44217,44311,44405,44499,44592,
   44685,44778,44870,44962,45054,45146,45238,45329,
   45420,45511,45601,45692,45782,45872,45961,46051,
   46140,46229,46318,46406,46494,46583,46670,46758,
   46846,46933,47020,47107,47193,47280,47366,47452,
   47538,47623,47709,47794,47879,47964,48048,48133,
   48217,48301,48385,48468,48552,48635,48718,48801,
   48884,48966,49048,49131,49213,49294,49376,49458,
   49539,49620,49701,49782,49862,49943,50023,50103,
   50183,50263,50342,50422,50501,50580,50659,50738,
   50816,50895,50973,51051,51129,51207,51285,51362,
   51439,51517,51594,51671,51747,51824,51900,51977,
   52053,52129,52205,52280,52356,52432,52507,52582,
   52657,52732,52807,52881,52956,53030,53104,53178,
   53252,53326,53400,53473,53546,53620,53693,53766,
   53839,53911,53984,54056,54129,54201,54273,54345,
   54417,54489,54560,54632,54703,54774,54845,54916,
   54987,55058,55129,55199,55269,55340,55410,55480,
   55550,55620,55689,55759,55828,55898,55967,56036,
   56105,56174,56243,56311,56380,56448,56517,56585,
   56653,56721,56789,56857,56924,56992,57059,57127,
   57194,57261,57328,57395,57462,57529,57595,57662,
   57728,57795,57861,57927,57993,58059,58125,58191,
   58256,58322,58387,58453,58518,58583,58648,58713,
   58778,58843,58908,58972,59037,59101,59165,59230,
   59294,59358,59422,59486,59549,59613,59677,59740,
   59804,59867,59930,59993,60056,60119,60182,60245,
   60308,60370,60433,60495,60558,60620,60682,60744,
   60806,60868,60930,60992,61054,61115,61177,61238,
   61300,61361,61422,61483,61544,61605,61666,61727,
   61788,61848,61909,61969,62030,62090,62150,62211,
   62271,62331,62391,62450,62510,62570,62630,62689,
   62749,62808,62867,62927,62986,63045,63104,63163,
   63222,63281,63340,63398,63457,63515,63574,63632,
   63691,63749,63807,63865,63923,63981,64039,64097,
   64155,64212,64270,64328,64385,64443,64500,64557,
   64614,64672,64729,64786,64843,64900,64956,65013,
   65070,65126,65183,65239,65296,65352,65409,65465
};

const png_byte png_sRGB_delta[512] =
{
   207,201,158,129,113,100,90,82,77,72,68,64,61,59,56,54,
   52,50,49,47,46,45,43,42,41,40,39,39,38,37,36,36,
   35,34,34,33,33,32,32,31,31,30,30,30,29,29,28,28,
   28,27,27,27,27,26,26,26,25,25,25,25,24,24,24,24,
   23,23,23,23,23,22,22,22,22,22,22,21,21,21,21,21,
   21,20,20,20,20,20,20,20,20,19,19,19,19,19,19,19,
   19,18,18,18,18,18,18,18,18,18,18,17,17,17,17,17,
   17,17,17,17,17,17,16,16,16,16,16,16,16,16,16,16,
   16,16,16,16,15,15,15,15,15,15,15,15,15,15,15,15,
   15,15,15,15,14,14,14,14,14,14,14,14,14,14,14,14,
   14,14,14,14,14,14,14,13,13,13,13,13,13,13,13,13,
   13,13,13,13,13,13,13,13,13,13,13,13,13,13,12,12,
   12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
   12,12,12,12,12,12,12,12,12,12,12,12,11,11,11,11,
   11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
   11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
   11,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
   10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
   10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
   10,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};
#endif 


#if defined(PNG_SIMPLIFIED_READ_SUPPORTED) ||\
   defined(PNG_SIMPLIFIED_WRITE_SUPPORTED)
static int
png_image_free_function(png_voidp argument)
{
   png_imagep image = png_voidcast(png_imagep, argument);
   png_controlp cp = image->opaque;
   png_control c;

   


   if (cp->png_ptr == NULL)
      return 0;

   
#  ifdef PNG_STDIO_SUPPORTED
      if (cp->owned_file != 0)
      {
         FILE *fp = png_voidcast(FILE*, cp->png_ptr->io_ptr);
         cp->owned_file = 0;

         
         if (fp != NULL)
         {
            cp->png_ptr->io_ptr = NULL;
            (void)fclose(fp);
         }
      }
#  endif

   




   c = *cp;
   image->opaque = &c;
   png_free(c.png_ptr, cp);

   
   if (c.for_write != 0)
   {
#     ifdef PNG_SIMPLIFIED_WRITE_SUPPORTED
         png_destroy_write_struct(&c.png_ptr, &c.info_ptr);
#     else
         png_error(c.png_ptr, "simplified write not supported");
#     endif
   }
   else
   {
#     ifdef PNG_SIMPLIFIED_READ_SUPPORTED
         png_destroy_read_struct(&c.png_ptr, &c.info_ptr, NULL);
#     else
         png_error(c.png_ptr, "simplified read not supported");
#     endif
   }

   
   return 1;
}

void PNGAPI
png_image_free(png_imagep image)
{
   



   if (image != NULL && image->opaque != NULL &&
      image->opaque->error_buf == NULL)
   {
      
      (void)png_safe_execute(image, png_image_free_function, image);
      image->opaque = NULL;
   }
}

int 
png_image_error(png_imagep image, png_const_charp error_message)
{
   
   png_safecat(image->message, (sizeof image->message), 0, error_message);
   image->warning_or_error |= PNG_IMAGE_ERROR;
   png_image_free(image);
   return 0;
}

#endif 
#endif 

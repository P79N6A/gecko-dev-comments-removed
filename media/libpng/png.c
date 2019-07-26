












#include "pngpriv.h"


typedef png_libpng_version_1_5_14 Your_png_h_is_not_version_1_5_14;







#ifdef PNG_READ_SUPPORTED
void PNGAPI
png_set_sig_bytes(png_structp png_ptr, int num_bytes)
{
   png_debug(1, "in png_set_sig_bytes");

   if (png_ptr == NULL)
      return;

   if (num_bytes > 8)
      png_error(png_ptr, "Too many bytes for PNG signature");

   png_ptr->sig_bytes = (png_byte)(num_bytes < 0 ? 0 : num_bytes);
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

   return ((int)(png_memcmp(&sig[start], &png_signature[start], num_to_check)));
}

#endif 

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)

PNG_FUNCTION(voidpf ,
png_zalloc,(voidpf png_ptr, uInt items, uInt size),PNG_ALLOCATED)
{
   png_voidp ptr;
   png_structp p;
   png_uint_32 save_flags;
   png_alloc_size_t num_bytes;

   if (png_ptr == NULL)
      return (NULL);

   p=(png_structp)png_ptr;
   save_flags=p->flags;

   if (items > PNG_UINT_32_MAX/size)
   {
     png_warning (p, "Potential overflow in png_zalloc()");
     return (NULL);
   }
   num_bytes = (png_alloc_size_t)items * size;

   p->flags|=PNG_FLAG_MALLOC_NULL_MEM_OK;
   ptr = (png_voidp)png_malloc((png_structp)png_ptr, num_bytes);
   p->flags=save_flags;

   return ((voidpf)ptr);
}


void 
png_zfree(voidpf png_ptr, voidpf ptr)
{
   png_free((png_structp)png_ptr, (png_voidp)ptr);
}




void 
png_reset_crc(png_structp png_ptr)
{
   
   png_ptr->crc = (png_uint_32)crc32(0, Z_NULL, 0);
}






void 
png_calculate_crc(png_structp png_ptr, png_const_bytep ptr, png_size_t length)
{
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

   




   if (need_crc && length > 0)
   {
      uLong crc = png_ptr->crc; 

      do
      {
         uInt safeLength = (uInt)length;
         if (safeLength == 0)
            safeLength = (uInt)-1; 

         crc = crc32(crc, ptr, safeLength);

         



         ptr += safeLength;
         length -= safeLength;
      }
      while (length > 0);

      
      png_ptr->crc = (png_uint_32)crc;
   }
}




int
png_user_version_check(png_structp png_ptr, png_const_charp user_png_ver)
{
   if (user_png_ver)
   {
      int i = 0;

      do
      {
         if (user_png_ver[i] != png_libpng_ver[i])
            png_ptr->flags |= PNG_FLAG_LIBRARY_MISMATCH;
      } while (png_libpng_ver[i++]);
   }

   else
      png_ptr->flags |= PNG_FLAG_LIBRARY_MISMATCH;

   if (png_ptr->flags & PNG_FLAG_LIBRARY_MISMATCH)
   {
     




      if (user_png_ver == NULL || user_png_ver[0] != png_libpng_ver[0] ||
          (user_png_ver[0] == '1' && user_png_ver[2] != png_libpng_ver[2]) ||
          (user_png_ver[0] == '0' && user_png_ver[2] < '9'))
      {
#ifdef PNG_WARNINGS_SUPPORTED
         size_t pos = 0;
         char m[128];

         pos = png_safecat(m, sizeof m, pos, "Application built with libpng-");
         pos = png_safecat(m, sizeof m, pos, user_png_ver);
         pos = png_safecat(m, sizeof m, pos, " but running with ");
         pos = png_safecat(m, sizeof m, pos, png_libpng_ver);

         png_warning(png_ptr, m);
#endif

#ifdef PNG_ERROR_NUMBERS_SUPPORTED
         png_ptr->flags = 0;
#endif

         return 0;
      }
   }

   
   return 1;
}







PNG_FUNCTION(png_infop,PNGAPI
png_create_info_struct,(png_structp png_ptr),PNG_ALLOCATED)
{
   png_infop info_ptr;

   png_debug(1, "in png_create_info_struct");

   if (png_ptr == NULL)
      return (NULL);

#ifdef PNG_USER_MEM_SUPPORTED
   info_ptr = (png_infop)png_create_struct_2(PNG_STRUCT_INFO,
      png_ptr->malloc_fn, png_ptr->mem_ptr);
#else
   info_ptr = (png_infop)png_create_struct(PNG_STRUCT_INFO);
#endif
   if (info_ptr != NULL)
      png_info_init_3(&info_ptr, png_sizeof(png_info));

   return (info_ptr);
}






void PNGAPI
png_destroy_info_struct(png_structp png_ptr, png_infopp info_ptr_ptr)
{
   png_infop info_ptr = NULL;

   png_debug(1, "in png_destroy_info_struct");

   if (png_ptr == NULL)
      return;

   if (info_ptr_ptr != NULL)
      info_ptr = *info_ptr_ptr;

   if (info_ptr != NULL)
   {
      png_info_destroy(png_ptr, info_ptr);

#ifdef PNG_USER_MEM_SUPPORTED
      png_destroy_struct_2((png_voidp)info_ptr, png_ptr->free_fn,
          png_ptr->mem_ptr);
#else
      png_destroy_struct((png_voidp)info_ptr);
#endif
      *info_ptr_ptr = NULL;
   }
}






void PNGAPI
png_info_init_3(png_infopp ptr_ptr, png_size_t png_info_struct_size)
{
   png_infop info_ptr = *ptr_ptr;

   png_debug(1, "in png_info_init_3");

   if (info_ptr == NULL)
      return;

   if (png_sizeof(png_info) > png_info_struct_size)
   {
      png_destroy_struct(info_ptr);
      info_ptr = (png_infop)png_create_struct(PNG_STRUCT_INFO);
      *ptr_ptr = info_ptr;
   }

   
   png_memset(info_ptr, 0, png_sizeof(png_info));
}

void PNGAPI
png_data_freer(png_structp png_ptr, png_infop info_ptr,
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
      png_warning(png_ptr,
         "Unknown freer parameter in png_data_freer");
}

void PNGAPI
png_free_data(png_structp png_ptr, png_infop info_ptr, png_uint_32 mask,
   int num)
{
   png_debug(1, "in png_free_data");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

#ifdef PNG_TEXT_SUPPORTED
   
   if ((mask & PNG_FREE_TEXT) & info_ptr->free_me)
   {
      if (num != -1)
      {
         if (info_ptr->text && info_ptr->text[num].key)
         {
            png_free(png_ptr, info_ptr->text[num].key);
            info_ptr->text[num].key = NULL;
         }
      }

      else
      {
         int i;
         for (i = 0; i < info_ptr->num_text; i++)
             png_free_data(png_ptr, info_ptr, PNG_FREE_TEXT, i);
         png_free(png_ptr, info_ptr->text);
         info_ptr->text = NULL;
         info_ptr->num_text=0;
      }
   }
#endif

#ifdef PNG_tRNS_SUPPORTED
   
   if ((mask & PNG_FREE_TRNS) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->trans_alpha);
      info_ptr->trans_alpha = NULL;
      info_ptr->valid &= ~PNG_INFO_tRNS;
   }
#endif

#ifdef PNG_sCAL_SUPPORTED
   
   if ((mask & PNG_FREE_SCAL) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->scal_s_width);
      png_free(png_ptr, info_ptr->scal_s_height);
      info_ptr->scal_s_width = NULL;
      info_ptr->scal_s_height = NULL;
      info_ptr->valid &= ~PNG_INFO_sCAL;
   }
#endif

#ifdef PNG_pCAL_SUPPORTED
   
   if ((mask & PNG_FREE_PCAL) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->pcal_purpose);
      png_free(png_ptr, info_ptr->pcal_units);
      info_ptr->pcal_purpose = NULL;
      info_ptr->pcal_units = NULL;
      if (info_ptr->pcal_params != NULL)
         {
            int i;
            for (i = 0; i < (int)info_ptr->pcal_nparams; i++)
            {
               png_free(png_ptr, info_ptr->pcal_params[i]);
               info_ptr->pcal_params[i] = NULL;
            }
            png_free(png_ptr, info_ptr->pcal_params);
            info_ptr->pcal_params = NULL;
         }
      info_ptr->valid &= ~PNG_INFO_pCAL;
   }
#endif

#ifdef PNG_iCCP_SUPPORTED
   
   if ((mask & PNG_FREE_ICCP) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->iccp_name);
      png_free(png_ptr, info_ptr->iccp_profile);
      info_ptr->iccp_name = NULL;
      info_ptr->iccp_profile = NULL;
      info_ptr->valid &= ~PNG_INFO_iCCP;
   }
#endif

#ifdef PNG_sPLT_SUPPORTED
   
   if ((mask & PNG_FREE_SPLT) & info_ptr->free_me)
   {
      if (num != -1)
      {
         if (info_ptr->splt_palettes)
         {
            png_free(png_ptr, info_ptr->splt_palettes[num].name);
            png_free(png_ptr, info_ptr->splt_palettes[num].entries);
            info_ptr->splt_palettes[num].name = NULL;
            info_ptr->splt_palettes[num].entries = NULL;
         }
      }

      else
      {
         if (info_ptr->splt_palettes_num)
         {
            int i;
            for (i = 0; i < (int)info_ptr->splt_palettes_num; i++)
               png_free_data(png_ptr, info_ptr, PNG_FREE_SPLT, i);

            png_free(png_ptr, info_ptr->splt_palettes);
            info_ptr->splt_palettes = NULL;
            info_ptr->splt_palettes_num = 0;
         }
         info_ptr->valid &= ~PNG_INFO_sPLT;
      }
   }
#endif

#ifdef PNG_UNKNOWN_CHUNKS_SUPPORTED
   if (png_ptr->unknown_chunk.data)
   {
      png_free(png_ptr, png_ptr->unknown_chunk.data);
      png_ptr->unknown_chunk.data = NULL;
   }

   if ((mask & PNG_FREE_UNKN) & info_ptr->free_me)
   {
      if (num != -1)
      {
          if (info_ptr->unknown_chunks)
          {
             png_free(png_ptr, info_ptr->unknown_chunks[num].data);
             info_ptr->unknown_chunks[num].data = NULL;
          }
      }

      else
      {
         int i;

         if (info_ptr->unknown_chunks_num)
         {
            for (i = 0; i < info_ptr->unknown_chunks_num; i++)
               png_free_data(png_ptr, info_ptr, PNG_FREE_UNKN, i);

            png_free(png_ptr, info_ptr->unknown_chunks);
            info_ptr->unknown_chunks = NULL;
            info_ptr->unknown_chunks_num = 0;
         }
      }
   }
#endif

#ifdef PNG_hIST_SUPPORTED
   
   if ((mask & PNG_FREE_HIST)  & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->hist);
      info_ptr->hist = NULL;
      info_ptr->valid &= ~PNG_INFO_hIST;
   }
#endif

   
   if ((mask & PNG_FREE_PLTE) & info_ptr->free_me)
   {
      png_zfree(png_ptr, info_ptr->palette);
      info_ptr->palette = NULL;
      info_ptr->valid &= ~PNG_INFO_PLTE;
      info_ptr->num_palette = 0;
   }

#ifdef PNG_INFO_IMAGE_SUPPORTED
   
   if ((mask & PNG_FREE_ROWS) & info_ptr->free_me)
   {
      if (info_ptr->row_pointers)
      {
         int row;
         for (row = 0; row < (int)info_ptr->height; row++)
         {
            png_free(png_ptr, info_ptr->row_pointers[row]);
            info_ptr->row_pointers[row] = NULL;
         }
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





void 
png_info_destroy(png_structp png_ptr, png_infop info_ptr)
{
   png_debug(1, "in png_info_destroy");

   png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
   if (png_ptr->num_chunk_list)
   {
      png_free(png_ptr, png_ptr->chunk_list);
      png_ptr->chunk_list = NULL;
      png_ptr->num_chunk_list = 0;
   }
#endif

   png_info_init_3(&info_ptr, png_sizeof(png_info));
}
#endif 





png_voidp PNGAPI
png_get_io_ptr(png_structp png_ptr)
{
   if (png_ptr == NULL)
      return (NULL);

   return (png_ptr->io_ptr);
}

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)
#  ifdef PNG_STDIO_SUPPORTED






void PNGAPI
png_init_io(png_structp png_ptr, png_FILE_p fp)
{
   png_debug(1, "in png_init_io");

   if (png_ptr == NULL)
      return;

   png_ptr->io_ptr = (png_voidp)fp;
}
#  endif

#  ifdef PNG_TIME_RFC1123_SUPPORTED



png_const_charp PNGAPI
png_convert_to_rfc1123(png_structp png_ptr, png_const_timep ptime)
{
   static PNG_CONST char short_months[12][4] =
        {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

   if (png_ptr == NULL)
      return (NULL);

   if (ptime->year > 9999  ||
       ptime->month == 0    ||  ptime->month > 12  ||
       ptime->day   == 0    ||  ptime->day   > 31  ||
       ptime->hour  > 23    ||  ptime->minute > 59 ||
       ptime->second > 60)
   {
      png_warning(png_ptr, "Ignoring invalid time value");
      return (NULL);
   }

   {
      size_t pos = 0;
      char number_buf[5]; 

#     define APPEND_STRING(string)\
         pos = png_safecat(png_ptr->time_buffer, sizeof png_ptr->time_buffer,\
            pos, (string))
#     define APPEND_NUMBER(format, value)\
         APPEND_STRING(PNG_FORMAT_NUMBER(number_buf, format, (value)))
#     define APPEND(ch)\
         if (pos < (sizeof png_ptr->time_buffer)-1)\
            png_ptr->time_buffer[pos++] = (ch)

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

   return png_ptr->time_buffer;
}
#  endif 

#endif 

png_const_charp PNGAPI
png_get_copyright(png_const_structp png_ptr)
{
   PNG_UNUSED(png_ptr)  
#ifdef PNG_STRING_COPYRIGHT
   return PNG_STRING_COPYRIGHT
#else
#  ifdef __STDC__
   return PNG_STRING_NEWLINE \
     "libpng version 1.5.14 - January 24, 2013" PNG_STRING_NEWLINE \
     "Copyright (c) 1998-2013 Glenn Randers-Pehrson" PNG_STRING_NEWLINE \
     "Copyright (c) 1996-1997 Andreas Dilger" PNG_STRING_NEWLINE \
     "Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc." \
     PNG_STRING_NEWLINE;
#  else
      return "libpng version 1.5.14 - January 24, 2013\
      Copyright (c) 1998-2013 Glenn Randers-Pehrson\
      Copyright (c) 1996-1997 Andreas Dilger\
      Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.";
#  endif
#endif
}









png_const_charp PNGAPI
png_get_libpng_ver(png_const_structp png_ptr)
{
   
   return png_get_header_ver(png_ptr);
}

png_const_charp PNGAPI
png_get_header_ver(png_const_structp png_ptr)
{
   
   PNG_UNUSED(png_ptr)  
   return PNG_LIBPNG_VER_STRING;
}

png_const_charp PNGAPI
png_get_header_version(png_const_structp png_ptr)
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

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
int PNGAPI
png_handle_as_unknown(png_structp png_ptr, png_const_bytep chunk_name)
{
   
   png_const_bytep p, p_end;

   if (png_ptr == NULL || chunk_name == NULL || png_ptr->num_chunk_list <= 0)
      return PNG_HANDLE_CHUNK_AS_DEFAULT;

   p_end = png_ptr->chunk_list;
   p = p_end + png_ptr->num_chunk_list*5; 

   



   do 
   {
      p -= 5;
      if (!png_memcmp(chunk_name, p, 4))
         return p[4];
   }
   while (p > p_end);

   return PNG_HANDLE_CHUNK_AS_DEFAULT;
}

int 
png_chunk_unknown_handling(png_structp png_ptr, png_uint_32 chunk_name)
{
   png_byte chunk_string[5];

   PNG_CSTRING_FROM_CHUNK(chunk_string, chunk_name);
   return png_handle_as_unknown(png_ptr, chunk_string);
}
#endif

#ifdef PNG_READ_SUPPORTED

int PNGAPI
png_reset_zstream(png_structp png_ptr)
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





#  ifdef PNG_CHECK_cHRM_SUPPORTED

int 
png_check_cHRM_fixed(png_structp png_ptr,
   png_fixed_point white_x, png_fixed_point white_y, png_fixed_point red_x,
   png_fixed_point red_y, png_fixed_point green_x, png_fixed_point green_y,
   png_fixed_point blue_x, png_fixed_point blue_y)
{
   int ret = 1;
   unsigned long xy_hi,xy_lo,yx_hi,yx_lo;

   png_debug(1, "in function png_check_cHRM_fixed");

   if (png_ptr == NULL)
      return 0;

   






   if (white_x < 0 || white_y <= 0 ||
         red_x < 0 ||   red_y <  0 ||
       green_x < 0 || green_y <  0 ||
        blue_x < 0 ||  blue_y <  0)
   {
      png_warning(png_ptr,
        "Ignoring attempt to set negative chromaticity value");
      ret = 0;
   }
   
   if (white_x > PNG_FP_1 - white_y)
   {
      png_warning(png_ptr, "Invalid cHRM white point");
      ret = 0;
   }

   if (red_x > PNG_FP_1 - red_y)
   {
      png_warning(png_ptr, "Invalid cHRM red point");
      ret = 0;
   }

   if (green_x > PNG_FP_1 - green_y)
   {
      png_warning(png_ptr, "Invalid cHRM green point");
      ret = 0;
   }

   if (blue_x > PNG_FP_1 - blue_y)
   {
      png_warning(png_ptr, "Invalid cHRM blue point");
      ret = 0;
   }

   png_64bit_product(green_x - red_x, blue_y - red_y, &xy_hi, &xy_lo);
   png_64bit_product(green_y - red_y, blue_x - red_x, &yx_hi, &yx_lo);

   if (xy_hi == yx_hi && xy_lo == yx_lo)
   {
      png_warning(png_ptr,
         "Ignoring attempt to set cHRM RGB triangle with zero area");
      ret = 0;
   }

   return ret;
}
#  endif 

#ifdef PNG_cHRM_SUPPORTED





int png_xy_from_XYZ(png_xy *xy, png_XYZ XYZ)
{
   png_int_32 d, dwhite, whiteX, whiteY;

   d = XYZ.redX + XYZ.redY + XYZ.redZ;
   if (!png_muldiv(&xy->redx, XYZ.redX, PNG_FP_1, d)) return 1;
   if (!png_muldiv(&xy->redy, XYZ.redY, PNG_FP_1, d)) return 1;
   dwhite = d;
   whiteX = XYZ.redX;
   whiteY = XYZ.redY;

   d = XYZ.greenX + XYZ.greenY + XYZ.greenZ;
   if (!png_muldiv(&xy->greenx, XYZ.greenX, PNG_FP_1, d)) return 1;
   if (!png_muldiv(&xy->greeny, XYZ.greenY, PNG_FP_1, d)) return 1;
   dwhite += d;
   whiteX += XYZ.greenX;
   whiteY += XYZ.greenY;

   d = XYZ.blueX + XYZ.blueY + XYZ.blueZ;
   if (!png_muldiv(&xy->bluex, XYZ.blueX, PNG_FP_1, d)) return 1;
   if (!png_muldiv(&xy->bluey, XYZ.blueY, PNG_FP_1, d)) return 1;
   dwhite += d;
   whiteX += XYZ.blueX;
   whiteY += XYZ.blueY;

   


   if (!png_muldiv(&xy->whitex, whiteX, PNG_FP_1, dwhite)) return 1;
   if (!png_muldiv(&xy->whitey, whiteY, PNG_FP_1, dwhite)) return 1;

   return 0;
}

int png_XYZ_from_xy(png_XYZ *XYZ, png_xy xy)
{
   png_fixed_point red_inverse, green_inverse, blue_scale;
   png_fixed_point left, right, denominator;

   



   if (xy.redx < 0 || xy.redx > PNG_FP_1) return 1;
   if (xy.redy < 0 || xy.redy > PNG_FP_1-xy.redx) return 1;
   if (xy.greenx < 0 || xy.greenx > PNG_FP_1) return 1;
   if (xy.greeny < 0 || xy.greeny > PNG_FP_1-xy.greenx) return 1;
   if (xy.bluex < 0 || xy.bluex > PNG_FP_1) return 1;
   if (xy.bluey < 0 || xy.bluey > PNG_FP_1-xy.bluex) return 1;
   if (xy.whitex < 0 || xy.whitex > PNG_FP_1) return 1;
   if (xy.whitey < 0 || xy.whitey > PNG_FP_1-xy.whitex) return 1;

   















































































































































































   


   if (!png_muldiv(&left, xy.greenx-xy.bluex, xy.redy - xy.bluey, 7)) return 2;
   if (!png_muldiv(&right, xy.greeny-xy.bluey, xy.redx - xy.bluex, 7)) return 2;
   denominator = left - right;

   
   if (!png_muldiv(&left, xy.greenx-xy.bluex, xy.whitey-xy.bluey, 7)) return 2;
   if (!png_muldiv(&right, xy.greeny-xy.bluey, xy.whitex-xy.bluex, 7)) return 2;

   




   if (!png_muldiv(&red_inverse, xy.whitey, denominator, left-right) ||
       red_inverse <= xy.whitey )
      return 1;

   
   if (!png_muldiv(&left, xy.redy-xy.bluey, xy.whitex-xy.bluex, 7)) return 2;
   if (!png_muldiv(&right, xy.redx-xy.bluex, xy.whitey-xy.bluey, 7)) return 2;
   if (!png_muldiv(&green_inverse, xy.whitey, denominator, left-right) ||
       green_inverse <= xy.whitey)
      return 1;

   


   blue_scale = png_reciprocal(xy.whitey) - png_reciprocal(red_inverse) -
      png_reciprocal(green_inverse);
   if (blue_scale <= 0) return 1;


   
   if (!png_muldiv(&XYZ->redX, xy.redx, PNG_FP_1, red_inverse)) return 1;
   if (!png_muldiv(&XYZ->redY, xy.redy, PNG_FP_1, red_inverse)) return 1;
   if (!png_muldiv(&XYZ->redZ, PNG_FP_1 - xy.redx - xy.redy, PNG_FP_1,
      red_inverse))
      return 1;

   if (!png_muldiv(&XYZ->greenX, xy.greenx, PNG_FP_1, green_inverse)) return 1;
   if (!png_muldiv(&XYZ->greenY, xy.greeny, PNG_FP_1, green_inverse)) return 1;
   if (!png_muldiv(&XYZ->greenZ, PNG_FP_1 - xy.greenx - xy.greeny, PNG_FP_1,
      green_inverse))
      return 1;

   if (!png_muldiv(&XYZ->blueX, xy.bluex, blue_scale, PNG_FP_1)) return 1;
   if (!png_muldiv(&XYZ->blueY, xy.bluey, blue_scale, PNG_FP_1)) return 1;
   if (!png_muldiv(&XYZ->blueZ, PNG_FP_1 - xy.bluex - xy.bluey, blue_scale,
      PNG_FP_1))
      return 1;

   return 0; 
}

int png_XYZ_from_xy_checked(png_structp png_ptr, png_XYZ *XYZ, png_xy xy)
{
   switch (png_XYZ_from_xy(XYZ, xy))
   {
      case 0: 
         return 1;

      case 1:
         



         png_warning(png_ptr,
            "extreme cHRM chunk cannot be converted to tristimulus values");
         break;

      default:
         


         png_error(png_ptr, "internal error in png_XYZ_from_xy");
         break;
   }

   
   return 0;
}
#endif

void 
png_check_IHDR(png_structp png_ptr,
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

   if (height == 0)
   {
      png_warning(png_ptr, "Image height is zero in IHDR");
      error = 1;
   }

#  ifdef PNG_SET_USER_LIMITS_SUPPORTED
   if (width > png_ptr->user_width_max)

#  else
   if (width > PNG_USER_WIDTH_MAX)
#  endif
   {
      png_warning(png_ptr, "Image width exceeds user limit in IHDR");
      error = 1;
   }

#  ifdef PNG_SET_USER_LIMITS_SUPPORTED
   if (height > png_ptr->user_height_max)
#  else
   if (height > PNG_USER_HEIGHT_MAX)
#  endif
   {
      png_warning(png_ptr, "Image height exceeds user limit in IHDR");
      error = 1;
   }

   if (width > PNG_UINT_31_MAX)
   {
      png_warning(png_ptr, "Invalid image width in IHDR");
      error = 1;
   }

   if (height > PNG_UINT_31_MAX)
   {
      png_warning(png_ptr, "Invalid image height in IHDR");
      error = 1;
   }

   if (width > (PNG_UINT_32_MAX
                 >> 3)      
                 - 48       
                 - 1        
                 - 7*8      
                 - 8)       
      png_warning(png_ptr, "Width is too large for libpng to process pixels");

   
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

#  ifdef PNG_MNG_FEATURES_SUPPORTED
   








   if ((png_ptr->mode & PNG_HAVE_PNG_SIGNATURE) &&
       png_ptr->mng_features_permitted)
      png_warning(png_ptr, "MNG features are not allowed in a PNG datastream");

   if (filter_type != PNG_FILTER_TYPE_BASE)
   {
      if (!((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
          (filter_type == PNG_INTRAPIXEL_DIFFERENCING) &&
          ((png_ptr->mode & PNG_HAVE_PNG_SIGNATURE) == 0) &&
          (color_type == PNG_COLOR_TYPE_RGB ||
          color_type == PNG_COLOR_TYPE_RGB_ALPHA)))
      {
         png_warning(png_ptr, "Unknown filter method in IHDR");
         error = 1;
      }

      if (png_ptr->mode & PNG_HAVE_PNG_SIGNATURE)
      {
         png_warning(png_ptr, "Invalid filter method in IHDR");
         error = 1;
      }
   }

#  else
   if (filter_type != PNG_FILTER_TYPE_BASE)
   {
      png_warning(png_ptr, "Unknown filter method in IHDR");
      error = 1;
   }
#  endif

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
         if (state & PNG_FP_SAW_ANY)
            goto PNG_FP_End; 

         png_fp_add(state, type);
         break;

      case PNG_FP_INTEGER + PNG_FP_SAW_DOT:
         
         if (state & PNG_FP_SAW_DOT) 
            goto PNG_FP_End;

         else if (state & PNG_FP_SAW_DIGIT) 
            png_fp_add(state, type);

         else
            png_fp_set(state, PNG_FP_FRACTION | type);

         break;

      case PNG_FP_INTEGER + PNG_FP_SAW_DIGIT:
         if (state & PNG_FP_SAW_DOT) 
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
         if (state & PNG_FP_SAW_ANY)
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

   if (png_check_fp_number(string, size, &state, &char_index) &&
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
   double d = 1.0;

   


   if (power < 0)
   {
      if (power < DBL_MIN_10_EXP) return 0;
      recip = 1, power = -power;
   }

   if (power > 0)
   {
      
      double mult = 10.0;
      do
      {
         if (power & 1) d *= mult;
         mult *= mult;
         power >>= 1;
      }
      while (power > 0);

      if (recip) d = 1/d;
   }
   

   return d;
}




void 
png_ascii_from_fp(png_structp png_ptr, png_charp ascii, png_size_t size,
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

               fp *= 10.0;

               




               if (cdigits+czero-clead+1 < (int)precision)
                  fp = modf(fp, &d);

               else
               {
                  d = floor(fp + .5);

                  if (d > 9.0)
                  {
                     
                     if (czero > 0)
                     {
                        --czero, d = 1;
                        if (cdigits == 0) --clead;
                     }

                     else
                     {
                        while (cdigits > 0 && d > 9.0)
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

                        



                        if (d > 9.0)  
                        {
                           if (exp_b10 == (-1))
                           {
                              



                              int ch = *--ascii;

                              if (ch == 46)
                                 ++size, exp_b10 = 1;

                              


                           }
                           else
                              ++exp_b10;

                           
                           d = 1.0;
                        }
                     }
                  }
                  fp = 0; 
               }

               if (d == 0.0)
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
                     if (exp_b10 == 0) *ascii++ = 46, --size; 

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
png_ascii_from_fixed(png_structp png_ptr, png_charp ascii, png_size_t size,
    png_fixed_point fp)
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
   !defined(PNG_FIXED_POINT_MACRO_SUPPORTED)
png_fixed_point
png_fixed(png_structp png_ptr, double fp, png_const_charp text)
{
   double r = floor(100000 * fp + .5);

   if (r > 2147483647. || r < -2147483648.)
      png_fixed_error(png_ptr, text);

   return (png_fixed_point)r;
}
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || \
    defined(PNG_INCH_CONVERSIONS_SUPPORTED) || defined(PNG__READ_pHYs_SUPPORTED)






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

            if (negative)
               result = -result;

            
            if ((negative && result <= 0) || (!negative && result >= 0))
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
png_muldiv_warn(png_structp png_ptr, png_fixed_point a, png_int_32 times,
    png_int_32 divisor)
{
   png_fixed_point result;

   if (png_muldiv(&result, a, times, divisor))
      return result;

   png_warning(png_ptr, "fixed point overflow ignored");
   return 0;
}
#endif

#if (defined PNG_READ_GAMMA_SUPPORTED) || (defined PNG_cHRM_SUPPORTED)


png_fixed_point
png_reciprocal(png_fixed_point a)
{
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
   double r = floor(1E10/a+.5);

   if (r <= 2147483647. && r >= -2147483648.)
      return (png_fixed_point)r;
#else
   png_fixed_point res;

   if (png_muldiv(&res, 100000, 100000, a))
      return res;
#endif

   return 0; 
}

#ifdef PNG_READ_GAMMA_SUPPORTED

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

   if (png_muldiv(&res, a, b, 100000))
      return res;
#endif

   return 0; 
}
#endif 


png_fixed_point
png_reciprocal2(png_fixed_point a, png_fixed_point b)
{
   
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
   double r = 1E15/a;
   r /= b;
   r = floor(r+.5);

   if (r <= 2147483647. && r >= -2147483648.)
      return (png_fixed_point)r;
#else
   




   png_fixed_point res = png_product2(a, b);

   if (res != 0)
      return png_reciprocal(res);
#endif

   return 0; 
}
#endif 

#ifdef PNG_CHECK_cHRM_SUPPORTED

















void 
png_64bit_product (long v1, long v2, unsigned long *hi_product,
    unsigned long *lo_product)
{
   int a, b, c, d;
   long lo, hi, x, y;

   a = (v1 >> 16) & 0xffff;
   b = v1 & 0xffff;
   c = (v2 >> 16) & 0xffff;
   d = v2 & 0xffff;

   lo = b * d;                   
   x = a * d + c * b;            
   y = ((lo >> 16) & 0xffff) + x;

   lo = (lo & 0xffff) | ((y & 0xffff) << 16);
   hi = (y >> 16) & 0xffff;

   hi += a * c;                  

   *hi_product = (unsigned long)hi;
   *lo_product = (unsigned long)lo;
}
#endif 

#ifdef PNG_READ_GAMMA_SUPPORTED 
#ifndef PNG_FLOATING_ARITHMETIC_SUPPORTED















static png_uint_32
png_8bit_l2[128] =
{
#  ifdef PNG_DO_BC
      for (i=128;i<256;++i) { .5 - l(i/255)/l(2)*65536*65536; }
#  else
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
#  endif

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

PNG_STATIC png_int_32
png_log8bit(unsigned int x)
{
   unsigned int lg2 = 0;
   





   if ((x &= 0xff) == 0)
      return 0xffffffff;

   if ((x & 0xf0) == 0)
      lg2  = 4, x <<= 4;

   if ((x & 0xc0) == 0)
      lg2 += 2, x <<= 2;

   if ((x & 0x80) == 0)
      lg2 += 1, x <<= 1;

   
   return (png_int_32)((lg2 << 16) + ((png_8bit_l2[x-128]+32768)>>16));
}































PNG_STATIC png_int_32
png_log16bit(png_uint_32 x)
{
   unsigned int lg2 = 0;

   
   if ((x &= 0xffff) == 0)
      return 0xffffffff;

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















static png_uint_32
png_32bit_exp[16] =
{
#  ifdef PNG_DO_BC
      for (i=0;i<16;++i) { .5 + e(-i/16*l(2))*2^32; }
#  else
   
   4294967295U, 4112874773U, 3938502376U, 3771522796U, 3611622603U, 3458501653U,
   3311872529U, 3171459999U, 3037000500U, 2908241642U, 2784941738U, 2666869345U,
   2553802834U, 2445529972U, 2341847524U, 2242560872U
#  endif
};


#ifdef PNG_DO_BC
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

PNG_STATIC png_uint_32
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

PNG_STATIC png_byte
png_exp8bit(png_fixed_point lg2)
{
   
   png_uint_32 x = png_exp(lg2);

   



   x -= x >> 8;
   return (png_byte)((x + 0x7fffffU) >> 24);
}

PNG_STATIC png_uint_16
png_exp16bit(png_fixed_point lg2)
{
   
   png_uint_32 x = png_exp(lg2);

   
   x -= x >> 16;
   return (png_uint_16)((x + 32767U) >> 16);
}
#endif 

png_byte
png_gamma_8bit_correct(unsigned int value, png_fixed_point gamma_val)
{
   if (value > 0 && value < 255)
   {
#     ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
         double r = floor(255*pow(value/255.,gamma_val*.00001)+.5);
         return (png_byte)r;
#     else
         png_int_32 lg2 = png_log8bit(value);
         png_fixed_point res;

         if (png_muldiv(&res, gamma_val, lg2, PNG_FP_1))
            return png_exp8bit(res);

         
         value = 0;
#     endif
   }

   return (png_byte)value;
}

png_uint_16
png_gamma_16bit_correct(unsigned int value, png_fixed_point gamma_val)
{
   if (value > 0 && value < 65535)
   {
#     ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
         double r = floor(65535*pow(value/65535.,gamma_val*.00001)+.5);
         return (png_uint_16)r;
#     else
         png_int_32 lg2 = png_log16bit(value);
         png_fixed_point res;

         if (png_muldiv(&res, gamma_val, lg2, PNG_FP_1))
            return png_exp16bit(res);

         
         value = 0;
#     endif
   }

   return (png_uint_16)value;
}






png_uint_16 
png_gamma_correct(png_structp png_ptr, unsigned int value,
    png_fixed_point gamma_val)
{
   if (png_ptr->bit_depth == 8)
      return png_gamma_8bit_correct(value, gamma_val);

   else
      return png_gamma_16bit_correct(value, gamma_val);
}




int 
png_gamma_significant(png_fixed_point gamma_val)
{
   return gamma_val < PNG_FP_1 - PNG_GAMMA_THRESHOLD_FIXED ||
       gamma_val > PNG_FP_1 + PNG_GAMMA_THRESHOLD_FIXED;
}









static void
png_build_16bit_table(png_structp png_ptr, png_uint_16pp *ptable,
   PNG_CONST unsigned int shift, PNG_CONST png_fixed_point gamma_val)
{
   
   PNG_CONST unsigned int num = 1U << (8U - shift);
   PNG_CONST unsigned int max = (1U << (16U - shift))-1U;
   PNG_CONST unsigned int max_by_2 = 1U << (15U-shift);
   unsigned int i;

   png_uint_16pp table = *ptable =
       (png_uint_16pp)png_calloc(png_ptr, num * png_sizeof(png_uint_16p));

   for (i = 0; i < num; i++)
   {
      png_uint_16p sub_table = table[i] =
          (png_uint_16p)png_malloc(png_ptr, 256 * png_sizeof(png_uint_16));

      


      if (png_gamma_significant(gamma_val))
      {
         







         unsigned int j;
         for (j = 0; j < 256; j++)
         {
            png_uint_32 ig = (j << (8-shift)) + i;
#           ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
               
               double d = floor(65535*pow(ig/(double)max, gamma_val*.00001)+.5);
               sub_table[j] = (png_uint_16)d;
#           else
               if (shift)
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

            if (shift)
               ig = (ig * 65535U + max_by_2)/max;

            sub_table[j] = (png_uint_16)ig;
         }
      }
   }
}




static void
png_build_16to8_table(png_structp png_ptr, png_uint_16pp *ptable,
   PNG_CONST unsigned int shift, PNG_CONST png_fixed_point gamma_val)
{
   PNG_CONST unsigned int num = 1U << (8U - shift);
   PNG_CONST unsigned int max = (1U << (16U - shift))-1U;
   unsigned int i;
   png_uint_32 last;

   png_uint_16pp table = *ptable =
       (png_uint_16pp)png_calloc(png_ptr, num * png_sizeof(png_uint_16p));

   



   for (i = 0; i < num; i++)
      table[i] = (png_uint_16p)png_malloc(png_ptr,
          256 * png_sizeof(png_uint_16));

   















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





static void
png_build_8bit_table(png_structp png_ptr, png_bytepp ptable,
   PNG_CONST png_fixed_point gamma_val)
{
   unsigned int i;
   png_bytep table = *ptable = (png_bytep)png_malloc(png_ptr, 256);

   if (png_gamma_significant(gamma_val)) for (i=0; i<256; i++)
      table[i] = png_gamma_8bit_correct(i, gamma_val);

   else for (i=0; i<256; ++i)
      table[i] = (png_byte)i;
}




void 
png_destroy_gamma_table(png_structp png_ptr)
{
   png_free(png_ptr, png_ptr->gamma_table);
   png_ptr->gamma_table = NULL;

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

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_ALPHA_MODE_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
   png_free(png_ptr, png_ptr->gamma_from_1);
   png_ptr->gamma_from_1 = NULL;
   png_free(png_ptr, png_ptr->gamma_to_1);
   png_ptr->gamma_to_1 = NULL;

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
}






void 
png_build_gamma_table(png_structp png_ptr, int bit_depth)
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
         png_ptr->screen_gamma > 0 ?  png_reciprocal2(png_ptr->gamma,
         png_ptr->screen_gamma) : PNG_FP_1);

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_ALPHA_MODE_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
     if (png_ptr->transformations & (PNG_COMPOSE | PNG_RGB_TO_GRAY))
     {
        png_build_8bit_table(png_ptr, &png_ptr->gamma_to_1,
            png_reciprocal(png_ptr->gamma));

        png_build_8bit_table(png_ptr, &png_ptr->gamma_from_1,
            png_ptr->screen_gamma > 0 ?  png_reciprocal(png_ptr->screen_gamma) :
            png_ptr->gamma);
     }
#endif 
  }
  else
  {
     png_byte shift, sig_bit;

     if (png_ptr->color_type & PNG_COLOR_MASK_COLOR)
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
        shift = (png_byte)(16U - sig_bit); 

     else
        shift = 0; 

     if (png_ptr->transformations & (PNG_16_TO_8 | PNG_SCALE_16_TO_8))
     {
        



        if (shift < (16U - PNG_MAX_GAMMA_8))
           shift = (16U - PNG_MAX_GAMMA_8);
     }

     if (shift > 8U)
        shift = 8U; 

     png_ptr->gamma_shift = shift;

#ifdef PNG_16BIT_SUPPORTED
     




     if (png_ptr->transformations & (PNG_16_TO_8 | PNG_SCALE_16_TO_8))
#endif
         png_build_16to8_table(png_ptr, &png_ptr->gamma_16_table, shift,
         png_ptr->screen_gamma > 0 ? png_product2(png_ptr->gamma,
         png_ptr->screen_gamma) : PNG_FP_1);

#ifdef PNG_16BIT_SUPPORTED
     else
         png_build_16bit_table(png_ptr, &png_ptr->gamma_16_table, shift,
         png_ptr->screen_gamma > 0 ? png_reciprocal2(png_ptr->gamma,
         png_ptr->screen_gamma) : PNG_FP_1);
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_ALPHA_MODE_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
     if (png_ptr->transformations & (PNG_COMPOSE | PNG_RGB_TO_GRAY))
     {
        png_build_16bit_table(png_ptr, &png_ptr->gamma_16_to_1, shift,
            png_reciprocal(png_ptr->gamma));

        



        png_build_16bit_table(png_ptr, &png_ptr->gamma_16_from_1, shift,
            png_ptr->screen_gamma > 0 ? png_reciprocal(png_ptr->screen_gamma) :
            png_ptr->gamma);
     }
#endif 
  }
}
#endif 
#endif 

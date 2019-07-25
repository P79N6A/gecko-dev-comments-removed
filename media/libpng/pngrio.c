



















#define PNG_NO_PEDANTIC_WARNINGS
#include "png.h"
#ifdef PNG_READ_SUPPORTED
#include "pngpriv.h"







void 
png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_debug1(4, "reading %d bytes", (int)length);

   if (png_ptr->read_data_fn != NULL)
      (*(png_ptr->read_data_fn))(png_ptr, data, length);
   else
      png_error(png_ptr, "Call to NULL read function");
}

#ifdef PNG_STDIO_SUPPORTED





#ifndef USE_FAR_KEYWORD
void PNGAPI
png_default_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_size_t check;

   if (png_ptr == NULL)
      return;
   


   check = fread(data, 1, length, (png_FILE_p)png_ptr->io_ptr);

   if (check != length)
      png_error(png_ptr, "Read Error");
}
#else





#define NEAR_BUF_SIZE 1024
#define MIN(a,b) (a <= b ? a : b)

static void PNGAPI
png_default_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_size_t check;
   png_byte *n_data;
   png_FILE_p io_ptr;

   if (png_ptr == NULL)
      return;
   
   n_data = (png_byte *)CVT_PTR_NOCHECK(data);
   io_ptr = (png_FILE_p)CVT_PTR(png_ptr->io_ptr);
   if ((png_bytep)n_data == data)
   {
      check = fread(n_data, 1, length, io_ptr);
   }
   else
   {
      png_byte buf[NEAR_BUF_SIZE];
      png_size_t read, remaining, err;
      check = 0;
      remaining = length;
      do
      {
         read = MIN(NEAR_BUF_SIZE, remaining);
         err = fread(buf, 1, read, io_ptr);
         png_memcpy(data, buf, read); 
         if (err != read)
            break;
         else
            check += err;
         data += read;
         remaining -= read;
      }
      while (remaining != 0);
   }
   if ((png_uint_32)check != (png_uint_32)length)
      png_error(png_ptr, "read Error");
}
#endif
#endif

















void PNGAPI
png_set_read_fn(png_structp png_ptr, png_voidp io_ptr,
   png_rw_ptr read_data_fn)
{
   if (png_ptr == NULL)
      return;
   png_ptr->io_ptr = io_ptr;

#ifdef PNG_STDIO_SUPPORTED
   if (read_data_fn != NULL)
      png_ptr->read_data_fn = read_data_fn;
   else
      png_ptr->read_data_fn = png_default_read_data;
#else
   png_ptr->read_data_fn = read_data_fn;
#endif

   
   if (png_ptr->write_data_fn != NULL)
   {
      png_ptr->write_data_fn = NULL;
      png_warning(png_ptr,
         "It's an error to set both read_data_fn and write_data_fn in the ");
      png_warning(png_ptr,
         "same structure.  Resetting write_data_fn to NULL");
   }

#ifdef PNG_WRITE_FLUSH_SUPPORTED
   png_ptr->output_flush_fn = NULL;
#endif
}
#endif 

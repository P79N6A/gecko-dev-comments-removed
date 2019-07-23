



















#define PNG_NO_PEDANTIC_WARNINGS
#include "png.h"
#ifdef PNG_WRITE_SUPPORTED
#include "pngpriv.h"








void 
png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   if (png_ptr->write_data_fn != NULL )
      (*(png_ptr->write_data_fn))(png_ptr, data, length);
   else
      png_error(png_ptr, "Call to NULL write function");
}

#ifdef PNG_STDIO_SUPPORTED





#ifndef USE_FAR_KEYWORD
void PNGAPI
png_default_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_uint_32 check;

   if (png_ptr == NULL)
      return;
   check = fwrite(data, 1, length, (png_FILE_p)(png_ptr->io_ptr));
   if (check != length)
      png_error(png_ptr, "Write Error");
}
#else





#define NEAR_BUF_SIZE 1024
#define MIN(a,b) (a <= b ? a : b)

void PNGAPI
png_default_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_uint_32 check;
   png_byte *near_data;  
   png_FILE_p io_ptr;

   if (png_ptr == NULL)
      return;
   
   near_data = (png_byte *)CVT_PTR_NOCHECK(data);
   io_ptr = (png_FILE_p)CVT_PTR(png_ptr->io_ptr);
   if ((png_bytep)near_data == data)
   {
      check = fwrite(near_data, 1, length, io_ptr);
   }
   else
   {
      png_byte buf[NEAR_BUF_SIZE];
      png_size_t written, remaining, err;
      check = 0;
      remaining = length;
      do
      {
         written = MIN(NEAR_BUF_SIZE, remaining);
         png_memcpy(buf, data, written); 
         err = fwrite(buf, 1, written, io_ptr);
         if (err != written)
            break;

         else
            check += err;

         data += written;
         remaining -= written;
      }
      while (remaining != 0);
   }
   if (check != length)
      png_error(png_ptr, "Write Error");
}

#endif
#endif





#ifdef PNG_WRITE_FLUSH_SUPPORTED
void 
png_flush(png_structp png_ptr)
{
   if (png_ptr->output_flush_fn != NULL)
      (*(png_ptr->output_flush_fn))(png_ptr);
}

#ifdef PNG_STDIO_SUPPORTED
void PNGAPI
png_default_flush(png_structp png_ptr)
{
   png_FILE_p io_ptr;
   if (png_ptr == NULL)
      return;
   io_ptr = (png_FILE_p)CVT_PTR((png_ptr->io_ptr));
   fflush(io_ptr);
}
#endif
#endif






























void PNGAPI
png_set_write_fn(png_structp png_ptr, png_voidp io_ptr,
   png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn)
{
   if (png_ptr == NULL)
      return;

   png_ptr->io_ptr = io_ptr;

#ifdef PNG_STDIO_SUPPORTED
   if (write_data_fn != NULL)
      png_ptr->write_data_fn = write_data_fn;

   else
      png_ptr->write_data_fn = png_default_write_data;
#else
   png_ptr->write_data_fn = write_data_fn;
#endif

#ifdef PNG_WRITE_FLUSH_SUPPORTED
#ifdef PNG_STDIO_SUPPORTED
   if (output_flush_fn != NULL)
      png_ptr->output_flush_fn = output_flush_fn;

   else
      png_ptr->output_flush_fn = png_default_flush;
#else
   png_ptr->output_flush_fn = output_flush_fn;
#endif
#endif 

   
   if (png_ptr->read_data_fn != NULL)
   {
      png_ptr->read_data_fn = NULL;
      png_warning(png_ptr,
         "Attempted to set both read_data_fn and write_data_fn in");
      png_warning(png_ptr,
         "the same structure.  Resetting read_data_fn to NULL");
   }
}

#ifdef USE_FAR_KEYWORD
#ifdef _MSC_VER
void *png_far_to_near(png_structp png_ptr, png_voidp ptr, int check)
{
   void *near_ptr;
   void FAR *far_ptr;
   FP_OFF(near_ptr) = FP_OFF(ptr);
   far_ptr = (void FAR *)near_ptr;

   if (check != 0)
      if (FP_SEG(ptr) != FP_SEG(far_ptr))
         png_error(png_ptr, "segment lost in conversion");

   return(near_ptr);
}
#  else
void *png_far_to_near(png_structp png_ptr, png_voidp ptr, int check)
{
   void *near_ptr;
   void FAR *far_ptr;
   near_ptr = (void FAR *)ptr;
   far_ptr = (void FAR *)near_ptr;

   if (check != 0)
      if (far_ptr != ptr)
         png_error(png_ptr, "segment lost in conversion");

   return(near_ptr);
}
#   endif
#   endif
#endif 

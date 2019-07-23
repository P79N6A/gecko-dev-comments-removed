
















#define PNG_INTERNAL
#include "png.h"
#ifdef PNG_WRITE_SUPPORTED







void 
png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   if (png_ptr->write_data_fn != NULL )
      (*(png_ptr->write_data_fn))(png_ptr, data, length);
   else
      png_error(png_ptr, "Call to NULL write function");
}

#if !defined(PNG_NO_STDIO)




#ifndef USE_FAR_KEYWORD
void PNGAPI
png_default_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_uint_32 check;

   if (png_ptr == NULL) return;
#if defined(_WIN32_WCE)
   if ( !WriteFile((HANDLE)(png_ptr->io_ptr), data, length, &check, NULL) )
      check = 0;
#else
   check = fwrite(data, 1, length, (png_FILE_p)(png_ptr->io_ptr));
#endif
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

   if (png_ptr == NULL) return;
   
   near_data = (png_byte *)CVT_PTR_NOCHECK(data);
   io_ptr = (png_FILE_p)CVT_PTR(png_ptr->io_ptr);
   if ((png_bytep)near_data == data)
   {
#if defined(_WIN32_WCE)
      if ( !WriteFile(io_ptr, near_data, length, &check, NULL) )
         check = 0;
#else
      check = fwrite(near_data, 1, length, io_ptr);
#endif
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
#if defined(_WIN32_WCE)
         if ( !WriteFile(io_ptr, buf, written, &err, NULL) )
            err = 0;
#else
         err = fwrite(buf, 1, written, io_ptr);
#endif
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




#if defined(PNG_WRITE_FLUSH_SUPPORTED)
void 
png_flush(png_structp png_ptr)
{
   if (png_ptr->output_flush_fn != NULL)
      (*(png_ptr->output_flush_fn))(png_ptr);
}

#if !defined(PNG_NO_STDIO)
void PNGAPI
png_default_flush(png_structp png_ptr)
{
#if !defined(_WIN32_WCE)
   png_FILE_p io_ptr;
#endif
   if (png_ptr == NULL) return;
#if !defined(_WIN32_WCE)
   io_ptr = (png_FILE_p)CVT_PTR((png_ptr->io_ptr));
   if (io_ptr != NULL && fileno(io_ptr) != -1)
      fflush(io_ptr);
#endif
}
#endif
#endif























void PNGAPI
png_set_write_fn(png_structp png_ptr, png_voidp io_ptr,
   png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn)
{
   if (png_ptr == NULL) return;
   png_ptr->io_ptr = io_ptr;

#if !defined(PNG_NO_STDIO)
   if (write_data_fn != NULL)
      png_ptr->write_data_fn = write_data_fn;
   else
      png_ptr->write_data_fn = png_default_write_data;
#else
   png_ptr->write_data_fn = write_data_fn;
#endif

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
#if !defined(PNG_NO_STDIO)
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
         "the same structure.  Resetting read_data_fn to NULL.");
   }
}

#if defined(USE_FAR_KEYWORD)
#if defined(_MSC_VER)
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

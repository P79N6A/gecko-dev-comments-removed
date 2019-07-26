



















#include "pngpriv.h"

#ifdef PNG_WRITE_SUPPORTED








void 
png_write_data(png_structrp png_ptr, png_const_bytep data, png_size_t length)
{
   
   if (png_ptr->write_data_fn != NULL )
      (*(png_ptr->write_data_fn))(png_ptr, png_constcast(png_bytep,data),
         length);

   else
      png_error(png_ptr, "Call to NULL write function");
}

#ifdef PNG_STDIO_SUPPORTED





void PNGCBAPI
png_default_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_size_t check;

   if (png_ptr == NULL)
      return;

   check = fwrite(data, 1, length, (png_FILE_p)(png_ptr->io_ptr));

   if (check != length)
      png_error(png_ptr, "Write Error");
}
#endif





#ifdef PNG_WRITE_FLUSH_SUPPORTED
void 
png_flush(png_structrp png_ptr)
{
   if (png_ptr->output_flush_fn != NULL)
      (*(png_ptr->output_flush_fn))(png_ptr);
}

#  ifdef PNG_STDIO_SUPPORTED
void PNGCBAPI
png_default_flush(png_structp png_ptr)
{
   png_FILE_p io_ptr;

   if (png_ptr == NULL)
      return;

   io_ptr = png_voidcast(png_FILE_p, (png_ptr->io_ptr));
   fflush(io_ptr);
}
#  endif
#endif






























void PNGAPI
png_set_write_fn(png_structrp png_ptr, png_voidp io_ptr,
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
#  ifdef PNG_STDIO_SUPPORTED

   if (output_flush_fn != NULL)
      png_ptr->output_flush_fn = output_flush_fn;

   else
      png_ptr->output_flush_fn = png_default_flush;

#  else
   png_ptr->output_flush_fn = output_flush_fn;
#  endif
#endif 

   
   if (png_ptr->read_data_fn != NULL)
   {
      png_ptr->read_data_fn = NULL;

      png_warning(png_ptr,
          "Can't set both read_data_fn and write_data_fn in the"
          " same structure");
   }
}
#endif 

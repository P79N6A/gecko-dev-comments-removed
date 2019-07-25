

















#define PNG_NO_PEDANTIC_WARNINGS
#include "png.h"
#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)
#include "pngpriv.h"

static void 
png_default_error PNGARG((png_structp png_ptr,
  png_const_charp error_message)) PNG_NORETURN;
#ifdef PNG_WARNINGS_SUPPORTED
static void 
png_default_warning PNGARG((png_structp png_ptr,
  png_const_charp warning_message));
#endif 






#ifdef PNG_ERROR_TEXT_SUPPORTED
void PNGAPI
png_error(png_structp png_ptr, png_const_charp error_message)
{
#ifdef PNG_ERROR_NUMBERS_SUPPORTED
   char msg[16];
   if (png_ptr != NULL)
   {
     if (png_ptr->flags&
       (PNG_FLAG_STRIP_ERROR_NUMBERS|PNG_FLAG_STRIP_ERROR_TEXT))
     {
       if (*error_message == PNG_LITERAL_SHARP)
       {
           
           int offset;
           for (offset = 1; offset<15; offset++)
              if (error_message[offset] == ' ')
                  break;
           if (png_ptr->flags&PNG_FLAG_STRIP_ERROR_TEXT)
           {
              int i;
              for (i = 0; i < offset - 1; i++)
                 msg[i] = error_message[i + 1];
              msg[i - 1] = '\0';
              error_message = msg;
           }
           else
              error_message += offset;
       }
       else
       {
           if (png_ptr->flags&PNG_FLAG_STRIP_ERROR_TEXT)
           {
              msg[0] = '0';
              msg[1] = '\0';
              error_message = msg;
           }
       }
     }
   }
#endif
   if (png_ptr != NULL && png_ptr->error_fn != NULL)
      (*(png_ptr->error_fn))(png_ptr, error_message);

   

   png_default_error(png_ptr, error_message);
}
#else
void PNGAPI
png_err(png_structp png_ptr)
{
   if (png_ptr != NULL && png_ptr->error_fn != NULL)
      (*(png_ptr->error_fn))(png_ptr, '\0');

   

   png_default_error(png_ptr, '\0');
}
#endif 

#ifdef PNG_WARNINGS_SUPPORTED





void PNGAPI
png_warning(png_structp png_ptr, png_const_charp warning_message)
{
   int offset = 0;
   if (png_ptr != NULL)
   {
#ifdef PNG_ERROR_NUMBERS_SUPPORTED
   if (png_ptr->flags&
     (PNG_FLAG_STRIP_ERROR_NUMBERS|PNG_FLAG_STRIP_ERROR_TEXT))
#endif
     {
       if (*warning_message == PNG_LITERAL_SHARP)
       {
           for (offset = 1; offset < 15; offset++)
              if (warning_message[offset] == ' ')
                  break;
       }
     }
   }
   if (png_ptr != NULL && png_ptr->warning_fn != NULL)
      (*(png_ptr->warning_fn))(png_ptr, warning_message + offset);
   else
      png_default_warning(png_ptr, warning_message + offset);
}
#endif 

#ifdef PNG_BENIGN_ERRORS_SUPPORTED
void PNGAPI
png_benign_error(png_structp png_ptr, png_const_charp error_message)
{
  if (png_ptr->flags & PNG_FLAG_BENIGN_ERRORS_WARN)
    png_warning(png_ptr, error_message);
  else
    png_error(png_ptr, error_message);
}
#endif







#define isnonalpha(c) ((c) < 65 || (c) > 122 || ((c) > 90 && (c) < 97))
static PNG_CONST char png_digit[16] = {
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   'A', 'B', 'C', 'D', 'E', 'F'
};

#define PNG_MAX_ERROR_TEXT 64
#if defined(PNG_WARNINGS_SUPPORTED) || defined(PNG_ERROR_TEXT_SUPPORTED)
static void 
png_format_buffer(png_structp png_ptr, png_charp buffer, png_const_charp
   error_message)
{
   int iout = 0, iin = 0;

   while (iin < 4)
   {
      int c = png_ptr->chunk_name[iin++];
      if (isnonalpha(c))
      {
         buffer[iout++] = PNG_LITERAL_LEFT_SQUARE_BRACKET;
         buffer[iout++] = png_digit[(c & 0xf0) >> 4];
         buffer[iout++] = png_digit[c & 0x0f];
         buffer[iout++] = PNG_LITERAL_RIGHT_SQUARE_BRACKET;
      }
      else
      {
         buffer[iout++] = (png_byte)c;
      }
   }

   if (error_message == NULL)
      buffer[iout] = '\0';
   else
   {
      buffer[iout++] = ':';
      buffer[iout++] = ' ';
      png_memcpy(buffer + iout, error_message, PNG_MAX_ERROR_TEXT);
      buffer[iout + PNG_MAX_ERROR_TEXT - 1] = '\0';
   }
}

#ifdef PNG_READ_SUPPORTED
void PNGAPI
png_chunk_error(png_structp png_ptr, png_const_charp error_message)
{
   char msg[18+PNG_MAX_ERROR_TEXT];
   if (png_ptr == NULL)
     png_error(png_ptr, error_message);
   else
   {
     png_format_buffer(png_ptr, msg, error_message);
     png_error(png_ptr, msg);
   }
}
#endif 
#endif 

#ifdef PNG_WARNINGS_SUPPORTED
void PNGAPI
png_chunk_warning(png_structp png_ptr, png_const_charp warning_message)
{
   char msg[18+PNG_MAX_ERROR_TEXT];
   if (png_ptr == NULL)
     png_warning(png_ptr, warning_message);
   else
   {
     png_format_buffer(png_ptr, msg, warning_message);
     png_warning(png_ptr, msg);
   }
}
#endif 

#ifdef PNG_READ_SUPPORTED
#ifdef PNG_BENIGN_ERRORS_SUPPORTED
void PNGAPI
png_chunk_benign_error(png_structp png_ptr, png_const_charp error_message)
{
  if (png_ptr->flags & PNG_FLAG_BENIGN_ERRORS_WARN)
    png_chunk_warning(png_ptr, error_message);
  else
    png_chunk_error(png_ptr, error_message);
}
#endif
#endif 

#ifdef PNG_SETJMP_SUPPORTED



jmp_buf* PNGAPI
png_set_longjmp_fn(png_structp png_ptr, png_longjmp_ptr longjmp_fn,
    size_t jmp_buf_size)
{
   if (png_ptr == NULL || jmp_buf_size != png_sizeof(jmp_buf))
      return NULL;

   png_ptr->longjmp_fn = longjmp_fn;
   return &png_ptr->jmpbuf;
}
#endif






static void 
png_default_error(png_structp png_ptr, png_const_charp error_message)
{
#ifdef PNG_CONSOLE_IO_SUPPORTED
#ifdef PNG_ERROR_NUMBERS_SUPPORTED
   if (*error_message == PNG_LITERAL_SHARP)
   {
     
     int offset;
     char error_number[16];
     for (offset = 0; offset<15; offset++)
     {
         error_number[offset] = error_message[offset + 1];
         if (error_message[offset] == ' ')
             break;
     }
     if ((offset > 1) && (offset < 15))
     {
       error_number[offset - 1] = '\0';
       fprintf(stderr, "libpng error no. %s: %s",
          error_number, error_message + offset + 1);
       fprintf(stderr, PNG_STRING_NEWLINE);
     }
     else
     {
       fprintf(stderr, "libpng error: %s, offset=%d",
          error_message, offset);
       fprintf(stderr, PNG_STRING_NEWLINE);
     }
   }
   else
#endif
   {
      fprintf(stderr, "libpng error: %s", error_message);
      fprintf(stderr, PNG_STRING_NEWLINE);
   }
#endif

#ifdef PNG_SETJMP_SUPPORTED
   if (png_ptr && png_ptr->longjmp_fn)
   {
#  ifdef USE_FAR_KEYWORD
   {
      jmp_buf jmpbuf;
      png_memcpy(jmpbuf, png_ptr->jmpbuf, png_sizeof(jmp_buf));
     png_ptr->longjmp_fn(jmpbuf, 1);
   }
#  else
   png_ptr->longjmp_fn(png_ptr->jmpbuf, 1);
#  endif
   }
#endif
   
   PNG_ABORT();
#ifndef PNG_CONSOLE_IO_SUPPORTED
   PNG_UNUSED(error_message) 
#endif
}

#ifdef PNG_WARNINGS_SUPPORTED





static void 
png_default_warning(png_structp png_ptr, png_const_charp warning_message)
{
#ifdef PNG_CONSOLE_IO_SUPPORTED
#  ifdef PNG_ERROR_NUMBERS_SUPPORTED
   if (*warning_message == PNG_LITERAL_SHARP)
   {
     int offset;
     char warning_number[16];
     for (offset = 0; offset < 15; offset++)
     {
        warning_number[offset] = warning_message[offset + 1];
        if (warning_message[offset] == ' ')
            break;
     }
     if ((offset > 1) && (offset < 15))
     {
       warning_number[offset + 1] = '\0';
       fprintf(stderr, "libpng warning no. %s: %s",
          warning_number, warning_message + offset);
       fprintf(stderr, PNG_STRING_NEWLINE);
     }
     else
     {
       fprintf(stderr, "libpng warning: %s",
          warning_message);
       fprintf(stderr, PNG_STRING_NEWLINE);
     }
   }
   else
#  endif
   {
     fprintf(stderr, "libpng warning: %s", warning_message);
     fprintf(stderr, PNG_STRING_NEWLINE);
   }
#else
   PNG_UNUSED(warning_message) 
#endif
   PNG_UNUSED(png_ptr) 
}
#endif 






void PNGAPI
png_set_error_fn(png_structp png_ptr, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warning_fn)
{
   if (png_ptr == NULL)
      return;
   png_ptr->error_ptr = error_ptr;
   png_ptr->error_fn = error_fn;
   png_ptr->warning_fn = warning_fn;
}






png_voidp PNGAPI
png_get_error_ptr(png_const_structp png_ptr)
{
   if (png_ptr == NULL)
      return NULL;
   return ((png_voidp)png_ptr->error_ptr);
}


#ifdef PNG_ERROR_NUMBERS_SUPPORTED
void PNGAPI
png_set_strip_error_numbers(png_structp png_ptr, png_uint_32 strip_mode)
{
   if (png_ptr != NULL)
   {
     png_ptr->flags &=
       ((~(PNG_FLAG_STRIP_ERROR_NUMBERS|PNG_FLAG_STRIP_ERROR_TEXT))&strip_mode);
   }
}
#endif
#endif 

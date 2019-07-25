

















#include "pngpriv.h"

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)

static PNG_FUNCTION(void, png_default_error,PNGARG((png_structp png_ptr,
    png_const_charp error_message)),PNG_NORETURN);

#ifdef PNG_WARNINGS_SUPPORTED
static void 
png_default_warning PNGARG((png_structp png_ptr,
   png_const_charp warning_message));
#endif 






#ifdef PNG_ERROR_TEXT_SUPPORTED
PNG_FUNCTION(void,PNGAPI
png_error,(png_structp png_ptr, png_const_charp error_message),PNG_NORETURN)
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
PNG_FUNCTION(void,PNGAPI
png_err,(png_structp png_ptr),PNG_NORETURN)
{
   




   if (png_ptr != NULL && png_ptr->error_fn != NULL)
      (*(png_ptr->error_fn))(png_ptr, "");

   

   png_default_error(png_ptr, "");
}
#endif 




size_t
png_safecat(png_charp buffer, size_t bufsize, size_t pos,
   png_const_charp string)
{
   if (buffer != NULL && pos < bufsize)
   {
      if (string != NULL)
         while (*string != '\0' && pos < bufsize-1)
           buffer[pos++] = *string++;

      buffer[pos] = '\0';
   }

   return pos;
}

#if defined(PNG_WARNINGS_SUPPORTED) || defined(PNG_TIME_RFC1123_SUPPORTED)




png_charp
png_format_number(png_const_charp start, png_charp end, int format,
   png_alloc_size_t number)
{
   int count = 0;    
   int mincount = 1; 
   int output = 0;   

   *--end = '\0';

   


   while (end > start && (number != 0 || count < mincount))
   {

      static const char digits[] = "0123456789ABCDEF";

      switch (format)
      {
         case PNG_NUMBER_FORMAT_fixed:
            
            mincount = 5;
            if (output || number % 10 != 0)
            {
               *--end = digits[number % 10];
               output = 1;
            }
            number /= 10;
            break;

         case PNG_NUMBER_FORMAT_02u:
            
            mincount = 2;
            

         case PNG_NUMBER_FORMAT_u:
            *--end = digits[number % 10];
            number /= 10;
            break;

         case PNG_NUMBER_FORMAT_02x:
            
            mincount = 2;
            

         case PNG_NUMBER_FORMAT_x:
            *--end = digits[number & 0xf];
            number >>= 4;
            break;

         default: 
            number = 0;
            break;
      }

      
      ++count;

      
      if (format == PNG_NUMBER_FORMAT_fixed) if (count == 5) if (end > start)
      {
         



         if (output)
            *--end = '.';
         else if (number == 0) 
            *--end = '0';
      }
   }

   return end;
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






void
png_warning_parameter(png_warning_parameters p, int number,
   png_const_charp string)
{
   if (number > 0 && number <= PNG_WARNING_PARAMETER_COUNT)
      (void)png_safecat(p[number-1], (sizeof p[number-1]), 0, string);
}

void
png_warning_parameter_unsigned(png_warning_parameters p, int number, int format,
   png_alloc_size_t value)
{
   char buffer[PNG_NUMBER_BUFFER_SIZE];
   png_warning_parameter(p, number, PNG_FORMAT_NUMBER(buffer, format, value));
}

void
png_warning_parameter_signed(png_warning_parameters p, int number, int format,
   png_int_32 value)
{
   png_alloc_size_t u;
   png_charp str;
   char buffer[PNG_NUMBER_BUFFER_SIZE];

   
   u = (png_alloc_size_t)value;
   if (value < 0)
      u = ~u + 1;

   str = PNG_FORMAT_NUMBER(buffer, format, u);

   if (value < 0 && str > buffer)
      *--str = '-';

   png_warning_parameter(p, number, str);
}

void
png_formatted_warning(png_structp png_ptr, png_warning_parameters p,
   png_const_charp message)
{
   




   size_t i = 0; 
   char msg[192];

   





   while (i<(sizeof msg)-1 && *message != '\0')
   {
      


      if (p != NULL && *message == '@' && message[1] != '\0')
      {
         int parameter_char = *++message; 
         static const char valid_parameters[] = "123456789";
         int parameter = 0;

         


         while (valid_parameters[parameter] != parameter_char &&
            valid_parameters[parameter] != '\0')
            ++parameter;

         
         if (parameter < PNG_WARNING_PARAMETER_COUNT)
         {
            
            png_const_charp parm = p[parameter];
            png_const_charp pend = p[parameter] + (sizeof p[parameter]);

            



            while (i<(sizeof msg)-1 && *parm != '\0' && parm < pend)
               msg[i++] = *parm++;

            
            ++message;
            continue;
         }

         


      }

      


      msg[i++] = *message++;
   }

   
   msg[i] = '\0';

   



   png_warning(png_ptr, msg);
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
   png_uint_32 chunk_name = png_ptr->chunk_name;
   int iout = 0, ishift = 24;

   while (ishift >= 0)
   {
      int c = (int)(chunk_name >> ishift) & 0xff;

      ishift -= 8;
      if (isnonalpha(c))
      {
         buffer[iout++] = PNG_LITERAL_LEFT_SQUARE_BRACKET;
         buffer[iout++] = png_digit[(c & 0xf0) >> 4];
         buffer[iout++] = png_digit[c & 0x0f];
         buffer[iout++] = PNG_LITERAL_RIGHT_SQUARE_BRACKET;
      }

      else
      {
         buffer[iout++] = (char)c;
      }
   }

   if (error_message == NULL)
      buffer[iout] = '\0';

   else
   {
      int iin = 0;

      buffer[iout++] = ':';
      buffer[iout++] = ' ';

      while (iin < PNG_MAX_ERROR_TEXT-1 && error_message[iin] != '\0')
         buffer[iout++] = error_message[iin++];

      
      buffer[iout] = '\0';
   }
}
#endif 

#if defined(PNG_READ_SUPPORTED) && defined(PNG_ERROR_TEXT_SUPPORTED)
PNG_FUNCTION(void,PNGAPI
png_chunk_error,(png_structp png_ptr, png_const_charp error_message),
   PNG_NORETURN)
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

#ifdef PNG_ERROR_TEXT_SUPPORTED
#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_FUNCTION(void,
png_fixed_error,(png_structp png_ptr, png_const_charp name),PNG_NORETURN)
{
#  define fixed_message "fixed point overflow in "
#  define fixed_message_ln ((sizeof fixed_message)-1)
   int  iin;
   char msg[fixed_message_ln+PNG_MAX_ERROR_TEXT];
   png_memcpy(msg, fixed_message, fixed_message_ln);
   iin = 0;
   if (name != NULL) while (iin < (PNG_MAX_ERROR_TEXT-1) && name[iin] != 0)
   {
      msg[fixed_message_ln + iin] = name[iin];
      ++iin;
   }
   msg[fixed_message_ln + iin] = 0;
   png_error(png_ptr, msg);
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
   return &png_ptr->longjmp_buffer;
}
#endif






static PNG_FUNCTION(void ,
png_default_error,(png_structp png_ptr, png_const_charp error_message),
   PNG_NORETURN)
{
#ifdef PNG_CONSOLE_IO_SUPPORTED
#ifdef PNG_ERROR_NUMBERS_SUPPORTED
   
   if (error_message != NULL && *error_message == PNG_LITERAL_SHARP)
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
      fprintf(stderr, "libpng error: %s", error_message ? error_message :
         "undefined");
      fprintf(stderr, PNG_STRING_NEWLINE);
   }
#else
   PNG_UNUSED(error_message) 
#endif
   png_longjmp(png_ptr, 1);
}

PNG_FUNCTION(void,PNGAPI
png_longjmp,(png_structp png_ptr, int val),PNG_NORETURN)
{
#ifdef PNG_SETJMP_SUPPORTED
   if (png_ptr && png_ptr->longjmp_fn)
   {
#  ifdef USE_FAR_KEYWORD
      {
         jmp_buf tmp_jmpbuf;
         png_memcpy(tmp_jmpbuf, png_ptr->longjmp_buffer, png_sizeof(jmp_buf));
         png_ptr->longjmp_fn(tmp_jmpbuf, val);
      }

#  else
   png_ptr->longjmp_fn(png_ptr->longjmp_buffer, val);
#  endif
   }
#endif
   
   PNG_ABORT();
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
#ifdef PNG_WARNINGS_SUPPORTED
   png_ptr->warning_fn = warning_fn;
#else
   PNG_UNUSED(warning_fn)
#endif
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
         ((~(PNG_FLAG_STRIP_ERROR_NUMBERS |
         PNG_FLAG_STRIP_ERROR_TEXT))&strip_mode);
   }
}
#endif
#endif 

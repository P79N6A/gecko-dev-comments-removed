













#define _POSIX_SOURCE 1

#include "../pngpriv.h"

#ifdef PNG_READ_SUPPORTED
#if PNG_ARM_NEON_OPT > 0
#ifdef PNG_ARM_NEON_CHECK_SUPPORTED 
#include <signal.h> 

#ifdef __ANDROID__






#include <cpu-features.h>

static int
png_have_neon(png_structp png_ptr)
{
   



   PNG_UNUSED(png_ptr)
   return android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM &&
      (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0;
}
#elif defined(__linux__)



#include <unistd.h> 
#include <errno.h>  

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <asm/hwcap.h>





static size_t
safe_read(png_structp png_ptr, int fd, void *buffer_in, size_t nbytes)
{
   size_t ntotal = 0;
   char *buffer = png_voidcast(char*, buffer_in);

   while (nbytes > 0)
   {
      unsigned int nread;
      int iread;

      



      if (nbytes > INT_MAX)
         nread = INT_MAX;

      else
         nread = (unsigned int)nbytes;

      iread = read(fd, buffer, nread);

      if (iread == -1)
      {
         



         if (errno != EINTR)
         {
            png_warning(png_ptr, "/proc read failed");
            return 0; 
         }
      }

      else if (iread < 0)
      {
         
         png_warning(png_ptr, "OS /proc read bug");
         return 0;
      }

      else if (iread > 0)
      {
         
         buffer += iread;
         nbytes -= (unsigned int)iread;
         ntotal += (unsigned int)iread;
      }

      else
         return ntotal;
   }

   return ntotal; 
}

static int
png_have_neon(png_structp png_ptr)
{
   int fd = open("/proc/self/auxv", O_RDONLY);
   Elf32_auxv_t aux;

   
   if (fd == -1)
   {
      png_warning(png_ptr, "/proc/self/auxv open failed");
      return 0;
   }

   while (safe_read(png_ptr, fd, &aux, sizeof aux) == sizeof aux)
   {
      if (aux.a_type == AT_HWCAP && (aux.a_un.a_val & HWCAP_NEON) != 0)
      {
         close(fd);
         return 1;
      }
   }

   close(fd);
   return 0;
}
#else
   
#  error "no support for run-time ARM NEON checks"
#endif 
#endif 

#ifndef PNG_ALIGNED_MEMORY_SUPPORTED
#  error "ALIGNED_MEMORY is required; set: -DPNG_ALIGNED_MEMORY_SUPPORTED"
#endif

void
png_init_filter_functions_neon(png_structp pp, unsigned int bpp)
{
#ifdef PNG_ARM_NEON_API_SUPPORTED
   switch ((pp->options >> PNG_ARM_NEON) & 3)
   {
      case PNG_OPTION_UNSET:
         




#endif 
#ifdef PNG_ARM_NEON_CHECK_SUPPORTED
         {
            static volatile sig_atomic_t no_neon = -1; 

            if (no_neon < 0)
               no_neon = !png_have_neon(pp);

            if (no_neon)
               return;
         }
#ifdef PNG_ARM_NEON_API_SUPPORTED
         break;
#endif
#endif 
#ifdef PNG_ARM_NEON_API_SUPPORTED
      case PNG_OPTION_ON:
         
         break;

      default: 
         return;
   }
#endif

   










   pp->read_filter[PNG_FILTER_VALUE_UP-1] = png_read_filter_row_up_neon;

   if (bpp == 3)
   {
      pp->read_filter[PNG_FILTER_VALUE_SUB-1] = png_read_filter_row_sub3_neon;
      pp->read_filter[PNG_FILTER_VALUE_AVG-1] = png_read_filter_row_avg3_neon;
      pp->read_filter[PNG_FILTER_VALUE_PAETH-1] =
         png_read_filter_row_paeth3_neon;
   }

   else if (bpp == 4)
   {
      pp->read_filter[PNG_FILTER_VALUE_SUB-1] = png_read_filter_row_sub4_neon;
      pp->read_filter[PNG_FILTER_VALUE_AVG-1] = png_read_filter_row_avg4_neon;
      pp->read_filter[PNG_FILTER_VALUE_PAETH-1] =
          png_read_filter_row_paeth4_neon;
   }
}
#endif 
#endif 

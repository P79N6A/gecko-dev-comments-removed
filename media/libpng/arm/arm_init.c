










#include "../pngpriv.h"


#if defined __linux__ && defined __arm__
#include <stdio.h>
#include <elf.h>
#include <asm/hwcap.h>

static int png_have_hwcap(unsigned cap)
{
   FILE *f = fopen("/proc/self/auxv", "r");
   Elf32_auxv_t aux;
   int have_cap = 0;

   if (!f)
      return 0;

   while (fread(&aux, sizeof(aux), 1, f) > 0)
   {
      if (aux.a_type == AT_HWCAP &&
          aux.a_un.a_val & cap)
      {
         have_cap = 1;
         break;
      }
   }

   fclose(f);

   return have_cap;
}
#endif 

void
png_init_filter_functions_neon(png_structp pp, unsigned int bpp)
{
#ifdef __arm__
#ifdef __linux__
   if (!png_have_hwcap(HWCAP_NEON))
      return;
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
#else
   PNG_UNUSED(pp)
   PNG_UNUSED(bpp)
#endif
}

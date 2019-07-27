

















#include "pngpriv.h"

#ifdef PNG_READ_SUPPORTED


void PNGAPI
png_set_crc_action(png_structrp png_ptr, int crit_action, int ancil_action)
{
   png_debug(1, "in png_set_crc_action");

   if (png_ptr == NULL)
      return;

   
   switch (crit_action)
   {
      case PNG_CRC_NO_CHANGE:                        
         break;

      case PNG_CRC_WARN_USE:                               
         png_ptr->flags &= ~PNG_FLAG_CRC_CRITICAL_MASK;
         png_ptr->flags |= PNG_FLAG_CRC_CRITICAL_USE;
         break;

      case PNG_CRC_QUIET_USE:                             
         png_ptr->flags &= ~PNG_FLAG_CRC_CRITICAL_MASK;
         png_ptr->flags |= PNG_FLAG_CRC_CRITICAL_USE |
                           PNG_FLAG_CRC_CRITICAL_IGNORE;
         break;

      case PNG_CRC_WARN_DISCARD:    
         png_warning(png_ptr,
            "Can't discard critical data on CRC error");
      case PNG_CRC_ERROR_QUIT:                                

      case PNG_CRC_DEFAULT:
      default:
         png_ptr->flags &= ~PNG_FLAG_CRC_CRITICAL_MASK;
         break;
   }

   
   switch (ancil_action)
   {
      case PNG_CRC_NO_CHANGE:                       
         break;

      case PNG_CRC_WARN_USE:                              
         png_ptr->flags &= ~PNG_FLAG_CRC_ANCILLARY_MASK;
         png_ptr->flags |= PNG_FLAG_CRC_ANCILLARY_USE;
         break;

      case PNG_CRC_QUIET_USE:                            
         png_ptr->flags &= ~PNG_FLAG_CRC_ANCILLARY_MASK;
         png_ptr->flags |= PNG_FLAG_CRC_ANCILLARY_USE |
                           PNG_FLAG_CRC_ANCILLARY_NOWARN;
         break;

      case PNG_CRC_ERROR_QUIT:                               
         png_ptr->flags &= ~PNG_FLAG_CRC_ANCILLARY_MASK;
         png_ptr->flags |= PNG_FLAG_CRC_ANCILLARY_NOWARN;
         break;

      case PNG_CRC_WARN_DISCARD:                      

      case PNG_CRC_DEFAULT:
      default:
         png_ptr->flags &= ~PNG_FLAG_CRC_ANCILLARY_MASK;
         break;
   }
}

#ifdef PNG_READ_TRANSFORMS_SUPPORTED





static int
png_rtran_ok(png_structrp png_ptr, int need_IHDR)
{
   if (png_ptr != NULL)
   {
      if ((png_ptr->flags & PNG_FLAG_ROW_INIT) != 0)
         png_app_error(png_ptr,
            "invalid after png_start_read_image or png_read_update_info");

      else if (need_IHDR && (png_ptr->mode & PNG_HAVE_IHDR) == 0)
         png_app_error(png_ptr, "invalid before the PNG header has been read");

      else
      {
         
         png_ptr->flags |= PNG_FLAG_DETECT_UNINITIALIZED;

         return 1; 
      }
   }

   return 0; 
}
#endif

#ifdef PNG_READ_BACKGROUND_SUPPORTED

void PNGFAPI
png_set_background_fixed(png_structrp png_ptr,
    png_const_color_16p background_color, int background_gamma_code,
    int need_expand, png_fixed_point background_gamma)
{
   png_debug(1, "in png_set_background_fixed");

   if (png_rtran_ok(png_ptr, 0) == 0 || background_color == NULL)
      return;

   if (background_gamma_code == PNG_BACKGROUND_GAMMA_UNKNOWN)
   {
      png_warning(png_ptr, "Application must supply a known background gamma");
      return;
   }

   png_ptr->transformations |= PNG_COMPOSE | PNG_STRIP_ALPHA;
   png_ptr->transformations &= ~PNG_ENCODE_ALPHA;
   png_ptr->flags &= ~PNG_FLAG_OPTIMIZE_ALPHA;

   png_ptr->background = *background_color;
   png_ptr->background_gamma = background_gamma;
   png_ptr->background_gamma_type = (png_byte)(background_gamma_code);
   if (need_expand != 0)
      png_ptr->transformations |= PNG_BACKGROUND_EXPAND;
   else
      png_ptr->transformations &= ~PNG_BACKGROUND_EXPAND;
}

#  ifdef PNG_FLOATING_POINT_SUPPORTED
void PNGAPI
png_set_background(png_structrp png_ptr,
    png_const_color_16p background_color, int background_gamma_code,
    int need_expand, double background_gamma)
{
   png_set_background_fixed(png_ptr, background_color, background_gamma_code,
      need_expand, png_fixed(png_ptr, background_gamma, "png_set_background"));
}
#  endif  
#endif 





#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
void PNGAPI
png_set_scale_16(png_structrp png_ptr)
{
   png_debug(1, "in png_set_scale_16");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   png_ptr->transformations |= PNG_SCALE_16_TO_8;
}
#endif

#ifdef PNG_READ_STRIP_16_TO_8_SUPPORTED

void PNGAPI
png_set_strip_16(png_structrp png_ptr)
{
   png_debug(1, "in png_set_strip_16");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   png_ptr->transformations |= PNG_16_TO_8;
}
#endif

#ifdef PNG_READ_STRIP_ALPHA_SUPPORTED
void PNGAPI
png_set_strip_alpha(png_structrp png_ptr)
{
   png_debug(1, "in png_set_strip_alpha");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   png_ptr->transformations |= PNG_STRIP_ALPHA;
}
#endif

#if defined(PNG_READ_ALPHA_MODE_SUPPORTED) || defined(PNG_READ_GAMMA_SUPPORTED)
static png_fixed_point
translate_gamma_flags(png_structrp png_ptr, png_fixed_point output_gamma,
   int is_screen)
{
   




   if (output_gamma == PNG_DEFAULT_sRGB ||
      output_gamma == PNG_FP_1 / PNG_DEFAULT_sRGB)
   {
      


#     ifdef PNG_READ_sRGB_SUPPORTED
         png_ptr->flags |= PNG_FLAG_ASSUME_sRGB;
#     else
         PNG_UNUSED(png_ptr)
#     endif
      if (is_screen != 0)
         output_gamma = PNG_GAMMA_sRGB;
      else
         output_gamma = PNG_GAMMA_sRGB_INVERSE;
   }

   else if (output_gamma == PNG_GAMMA_MAC_18 ||
      output_gamma == PNG_FP_1 / PNG_GAMMA_MAC_18)
   {
      if (is_screen != 0)
         output_gamma = PNG_GAMMA_MAC_OLD;
      else
         output_gamma = PNG_GAMMA_MAC_INVERSE;
   }

   return output_gamma;
}

#  ifdef PNG_FLOATING_POINT_SUPPORTED
static png_fixed_point
convert_gamma_value(png_structrp png_ptr, double output_gamma)
{
   






   if (output_gamma > 0 && output_gamma < 128)
      output_gamma *= PNG_FP_1;

   
   output_gamma = floor(output_gamma + .5);

   if (output_gamma > PNG_FP_MAX || output_gamma < PNG_FP_MIN)
      png_fixed_error(png_ptr, "gamma value");

   return (png_fixed_point)output_gamma;
}
#  endif
#endif 

#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
void PNGFAPI
png_set_alpha_mode_fixed(png_structrp png_ptr, int mode,
   png_fixed_point output_gamma)
{
   int compose = 0;
   png_fixed_point file_gamma;

   png_debug(1, "in png_set_alpha_mode");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   output_gamma = translate_gamma_flags(png_ptr, output_gamma, 1);

   





   if (output_gamma < 70000 || output_gamma > 300000)
      png_error(png_ptr, "output gamma out of expected range");

   


   file_gamma = png_reciprocal(output_gamma);

   















   switch (mode)
   {
      case PNG_ALPHA_PNG:        
         
         png_ptr->transformations &= ~PNG_ENCODE_ALPHA;
         png_ptr->flags &= ~PNG_FLAG_OPTIMIZE_ALPHA;
         break;

      case PNG_ALPHA_ASSOCIATED: 
         compose = 1;
         png_ptr->transformations &= ~PNG_ENCODE_ALPHA;
         png_ptr->flags &= ~PNG_FLAG_OPTIMIZE_ALPHA;
         
         output_gamma = PNG_FP_1;
         break;

      case PNG_ALPHA_OPTIMIZED:  
         compose = 1;
         png_ptr->transformations &= ~PNG_ENCODE_ALPHA;
         png_ptr->flags |= PNG_FLAG_OPTIMIZE_ALPHA;
         
         break;

      case PNG_ALPHA_BROKEN:     
         compose = 1;
         png_ptr->transformations |= PNG_ENCODE_ALPHA;
         png_ptr->flags &= ~PNG_FLAG_OPTIMIZE_ALPHA;
         break;

      default:
         png_error(png_ptr, "invalid alpha mode");
   }

   



   if (png_ptr->colorspace.gamma == 0)
   {
      png_ptr->colorspace.gamma = file_gamma;
      png_ptr->colorspace.flags |= PNG_COLORSPACE_HAVE_GAMMA;
   }

   
   png_ptr->screen_gamma = output_gamma;

   


   if (compose != 0)
   {
      
      memset(&png_ptr->background, 0, (sizeof png_ptr->background));
      png_ptr->background_gamma = png_ptr->colorspace.gamma; 
      png_ptr->background_gamma_type = PNG_BACKGROUND_GAMMA_FILE;
      png_ptr->transformations &= ~PNG_BACKGROUND_EXPAND;

      if ((png_ptr->transformations & PNG_COMPOSE) != 0)
         png_error(png_ptr,
            "conflicting calls to set alpha mode and background");

      png_ptr->transformations |= PNG_COMPOSE;
   }
}

#  ifdef PNG_FLOATING_POINT_SUPPORTED
void PNGAPI
png_set_alpha_mode(png_structrp png_ptr, int mode, double output_gamma)
{
   png_set_alpha_mode_fixed(png_ptr, mode, convert_gamma_value(png_ptr,
      output_gamma));
}
#  endif
#endif

#ifdef PNG_READ_QUANTIZE_SUPPORTED









typedef struct png_dsort_struct
{
   struct png_dsort_struct * next;
   png_byte left;
   png_byte right;
} png_dsort;
typedef png_dsort *   png_dsortp;
typedef png_dsort * * png_dsortpp;

void PNGAPI
png_set_quantize(png_structrp png_ptr, png_colorp palette,
    int num_palette, int maximum_colors, png_const_uint_16p histogram,
    int full_quantize)
{
   png_debug(1, "in png_set_quantize");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   png_ptr->transformations |= PNG_QUANTIZE;

   if (full_quantize == 0)
   {
      int i;

      png_ptr->quantize_index = (png_bytep)png_malloc(png_ptr,
          (png_uint_32)(num_palette * (sizeof (png_byte))));
      for (i = 0; i < num_palette; i++)
         png_ptr->quantize_index[i] = (png_byte)i;
   }

   if (num_palette > maximum_colors)
   {
      if (histogram != NULL)
      {
         



         int i;

         
         png_ptr->quantize_sort = (png_bytep)png_malloc(png_ptr,
             (png_uint_32)(num_palette * (sizeof (png_byte))));

         
         for (i = 0; i < num_palette; i++)
            png_ptr->quantize_sort[i] = (png_byte)i;

         






         for (i = num_palette - 1; i >= maximum_colors; i--)
         {
            int done; 
            int j;

            done = 1;
            for (j = 0; j < i; j++)
            {
               if (histogram[png_ptr->quantize_sort[j]]
                   < histogram[png_ptr->quantize_sort[j + 1]])
               {
                  png_byte t;

                  t = png_ptr->quantize_sort[j];
                  png_ptr->quantize_sort[j] = png_ptr->quantize_sort[j + 1];
                  png_ptr->quantize_sort[j + 1] = t;
                  done = 0;
               }
            }

            if (done != 0)
               break;
         }

         
         if (full_quantize != 0)
         {
            int j = num_palette;

            


            for (i = 0; i < maximum_colors; i++)
            {
               if ((int)png_ptr->quantize_sort[i] >= maximum_colors)
               {
                  do
                     j--;
                  while ((int)png_ptr->quantize_sort[j] >= maximum_colors);

                  palette[i] = palette[j];
               }
            }
         }
         else
         {
            int j = num_palette;

            


            for (i = 0; i < maximum_colors; i++)
            {
               
               if ((int)png_ptr->quantize_sort[i] >= maximum_colors)
               {
                  png_color tmp_color;

                  do
                     j--;
                  while ((int)png_ptr->quantize_sort[j] >= maximum_colors);

                  tmp_color = palette[j];
                  palette[j] = palette[i];
                  palette[i] = tmp_color;
                  
                  png_ptr->quantize_index[j] = (png_byte)i;
                  png_ptr->quantize_index[i] = (png_byte)j;
               }
            }

            
            for (i = 0; i < num_palette; i++)
            {
               if ((int)png_ptr->quantize_index[i] >= maximum_colors)
               {
                  int min_d, k, min_k, d_index;

                  
                  d_index = png_ptr->quantize_index[i];
                  min_d = PNG_COLOR_DIST(palette[d_index], palette[0]);
                  for (k = 1, min_k = 0; k < maximum_colors; k++)
                  {
                     int d;

                     d = PNG_COLOR_DIST(palette[d_index], palette[k]);

                     if (d < min_d)
                     {
                        min_d = d;
                        min_k = k;
                     }
                  }
                  
                  png_ptr->quantize_index[i] = (png_byte)min_k;
               }
            }
         }
         png_free(png_ptr, png_ptr->quantize_sort);
         png_ptr->quantize_sort = NULL;
      }
      else
      {
         







         int i;
         int max_d;
         int num_new_palette;
         png_dsortp t;
         png_dsortpp hash;

         t = NULL;

         
         png_ptr->index_to_palette = (png_bytep)png_malloc(png_ptr,
             (png_uint_32)(num_palette * (sizeof (png_byte))));
         png_ptr->palette_to_index = (png_bytep)png_malloc(png_ptr,
             (png_uint_32)(num_palette * (sizeof (png_byte))));

         
         for (i = 0; i < num_palette; i++)
         {
            png_ptr->index_to_palette[i] = (png_byte)i;
            png_ptr->palette_to_index[i] = (png_byte)i;
         }

         hash = (png_dsortpp)png_calloc(png_ptr, (png_uint_32)(769 *
             (sizeof (png_dsortp))));

         num_new_palette = num_palette;

         







         max_d = 96;

         while (num_new_palette > maximum_colors)
         {
            for (i = 0; i < num_new_palette - 1; i++)
            {
               int j;

               for (j = i + 1; j < num_new_palette; j++)
               {
                  int d;

                  d = PNG_COLOR_DIST(palette[i], palette[j]);

                  if (d <= max_d)
                  {

                     t = (png_dsortp)png_malloc_warn(png_ptr,
                         (png_uint_32)(sizeof (png_dsort)));

                     if (t == NULL)
                         break;

                     t->next = hash[d];
                     t->left = (png_byte)i;
                     t->right = (png_byte)j;
                     hash[d] = t;
                  }
               }
               if (t == NULL)
                  break;
            }

            if (t != NULL)
            for (i = 0; i <= max_d; i++)
            {
               if (hash[i] != NULL)
               {
                  png_dsortp p;

                  for (p = hash[i]; p; p = p->next)
                  {
                     if ((int)png_ptr->index_to_palette[p->left]
                         < num_new_palette &&
                         (int)png_ptr->index_to_palette[p->right]
                         < num_new_palette)
                     {
                        int j, next_j;

                        if (num_new_palette & 0x01)
                        {
                           j = p->left;
                           next_j = p->right;
                        }
                        else
                        {
                           j = p->right;
                           next_j = p->left;
                        }

                        num_new_palette--;
                        palette[png_ptr->index_to_palette[j]]
                            = palette[num_new_palette];
                        if (full_quantize == 0)
                        {
                           int k;

                           for (k = 0; k < num_palette; k++)
                           {
                              if (png_ptr->quantize_index[k] ==
                                  png_ptr->index_to_palette[j])
                                 png_ptr->quantize_index[k] =
                                     png_ptr->index_to_palette[next_j];

                              if ((int)png_ptr->quantize_index[k] ==
                                  num_new_palette)
                                 png_ptr->quantize_index[k] =
                                     png_ptr->index_to_palette[j];
                           }
                        }

                        png_ptr->index_to_palette[png_ptr->palette_to_index
                            [num_new_palette]] = png_ptr->index_to_palette[j];

                        png_ptr->palette_to_index[png_ptr->index_to_palette[j]]
                            = png_ptr->palette_to_index[num_new_palette];

                        png_ptr->index_to_palette[j] =
                            (png_byte)num_new_palette;

                        png_ptr->palette_to_index[num_new_palette] =
                            (png_byte)j;
                     }
                     if (num_new_palette <= maximum_colors)
                        break;
                  }
                  if (num_new_palette <= maximum_colors)
                     break;
               }
            }

            for (i = 0; i < 769; i++)
            {
               if (hash[i] != NULL)
               {
                  png_dsortp p = hash[i];
                  while (p)
                  {
                     t = p->next;
                     png_free(png_ptr, p);
                     p = t;
                  }
               }
               hash[i] = 0;
            }
            max_d += 96;
         }
         png_free(png_ptr, hash);
         png_free(png_ptr, png_ptr->palette_to_index);
         png_free(png_ptr, png_ptr->index_to_palette);
         png_ptr->palette_to_index = NULL;
         png_ptr->index_to_palette = NULL;
      }
      num_palette = maximum_colors;
   }
   if (png_ptr->palette == NULL)
   {
      png_ptr->palette = palette;
   }
   png_ptr->num_palette = (png_uint_16)num_palette;

   if (full_quantize != 0)
   {
      int i;
      png_bytep distance;
      int total_bits = PNG_QUANTIZE_RED_BITS + PNG_QUANTIZE_GREEN_BITS +
          PNG_QUANTIZE_BLUE_BITS;
      int num_red = (1 << PNG_QUANTIZE_RED_BITS);
      int num_green = (1 << PNG_QUANTIZE_GREEN_BITS);
      int num_blue = (1 << PNG_QUANTIZE_BLUE_BITS);
      png_size_t num_entries = ((png_size_t)1 << total_bits);

      png_ptr->palette_lookup = (png_bytep)png_calloc(png_ptr,
          (png_uint_32)(num_entries * (sizeof (png_byte))));

      distance = (png_bytep)png_malloc(png_ptr, (png_uint_32)(num_entries *
          (sizeof (png_byte))));

      memset(distance, 0xff, num_entries * (sizeof (png_byte)));

      for (i = 0; i < num_palette; i++)
      {
         int ir, ig, ib;
         int r = (palette[i].red >> (8 - PNG_QUANTIZE_RED_BITS));
         int g = (palette[i].green >> (8 - PNG_QUANTIZE_GREEN_BITS));
         int b = (palette[i].blue >> (8 - PNG_QUANTIZE_BLUE_BITS));

         for (ir = 0; ir < num_red; ir++)
         {
            
            int dr = ((ir > r) ? ir - r : r - ir);
            int index_r = (ir << (PNG_QUANTIZE_BLUE_BITS +
                PNG_QUANTIZE_GREEN_BITS));

            for (ig = 0; ig < num_green; ig++)
            {
               
               int dg = ((ig > g) ? ig - g : g - ig);
               int dt = dr + dg;
               int dm = ((dr > dg) ? dr : dg);
               int index_g = index_r | (ig << PNG_QUANTIZE_BLUE_BITS);

               for (ib = 0; ib < num_blue; ib++)
               {
                  int d_index = index_g | ib;
                  
                  int db = ((ib > b) ? ib - b : b - ib);
                  int dmax = ((dm > db) ? dm : db);
                  int d = dmax + dt + db;

                  if (d < (int)distance[d_index])
                  {
                     distance[d_index] = (png_byte)d;
                     png_ptr->palette_lookup[d_index] = (png_byte)i;
                  }
               }
            }
         }
      }

      png_free(png_ptr, distance);
   }
}
#endif 

#ifdef PNG_READ_GAMMA_SUPPORTED
void PNGFAPI
png_set_gamma_fixed(png_structrp png_ptr, png_fixed_point scrn_gamma,
   png_fixed_point file_gamma)
{
   png_debug(1, "in png_set_gamma_fixed");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   
   scrn_gamma = translate_gamma_flags(png_ptr, scrn_gamma, 1);
   file_gamma = translate_gamma_flags(png_ptr, file_gamma, 0);

   










   if (file_gamma <= 0)
      png_error(png_ptr, "invalid file gamma in png_set_gamma");

   if (scrn_gamma <= 0)
      png_error(png_ptr, "invalid screen gamma in png_set_gamma");

   



   png_ptr->colorspace.gamma = file_gamma;
   png_ptr->colorspace.flags |= PNG_COLORSPACE_HAVE_GAMMA;
   png_ptr->screen_gamma = scrn_gamma;
}

#  ifdef PNG_FLOATING_POINT_SUPPORTED
void PNGAPI
png_set_gamma(png_structrp png_ptr, double scrn_gamma, double file_gamma)
{
   png_set_gamma_fixed(png_ptr, convert_gamma_value(png_ptr, scrn_gamma),
      convert_gamma_value(png_ptr, file_gamma));
}
#  endif 
#endif 

#ifdef PNG_READ_EXPAND_SUPPORTED




void PNGAPI
png_set_expand(png_structrp png_ptr)
{
   png_debug(1, "in png_set_expand");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   png_ptr->transformations |= (PNG_EXPAND | PNG_EXPAND_tRNS);
}




















void PNGAPI
png_set_palette_to_rgb(png_structrp png_ptr)
{
   png_debug(1, "in png_set_palette_to_rgb");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   png_ptr->transformations |= (PNG_EXPAND | PNG_EXPAND_tRNS);
}


void PNGAPI
png_set_expand_gray_1_2_4_to_8(png_structrp png_ptr)
{
   png_debug(1, "in png_set_expand_gray_1_2_4_to_8");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   png_ptr->transformations |= PNG_EXPAND;
}


void PNGAPI
png_set_tRNS_to_alpha(png_structrp png_ptr)
{
   png_debug(1, "in png_set_tRNS_to_alpha");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   png_ptr->transformations |= (PNG_EXPAND | PNG_EXPAND_tRNS);
}
#endif 

#ifdef PNG_READ_EXPAND_16_SUPPORTED



void PNGAPI
png_set_expand_16(png_structrp png_ptr)
{
   png_debug(1, "in png_set_expand_16");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   png_ptr->transformations |= (PNG_EXPAND_16 | PNG_EXPAND | PNG_EXPAND_tRNS);
}
#endif

#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED
void PNGAPI
png_set_gray_to_rgb(png_structrp png_ptr)
{
   png_debug(1, "in png_set_gray_to_rgb");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   
   png_set_expand_gray_1_2_4_to_8(png_ptr);
   png_ptr->transformations |= PNG_GRAY_TO_RGB;
}
#endif

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
void PNGFAPI
png_set_rgb_to_gray_fixed(png_structrp png_ptr, int error_action,
    png_fixed_point red, png_fixed_point green)
{
   png_debug(1, "in png_set_rgb_to_gray");

   
   
   if (png_rtran_ok(png_ptr, 1) == 0)
      return;

   switch (error_action)
   {
      case PNG_ERROR_ACTION_NONE:
         png_ptr->transformations |= PNG_RGB_TO_GRAY;
         break;

      case PNG_ERROR_ACTION_WARN:
         png_ptr->transformations |= PNG_RGB_TO_GRAY_WARN;
         break;

      case PNG_ERROR_ACTION_ERROR:
         png_ptr->transformations |= PNG_RGB_TO_GRAY_ERR;
         break;

      default:
         png_error(png_ptr, "invalid error action to rgb_to_gray");
         break;
   }

   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
#ifdef PNG_READ_EXPAND_SUPPORTED
      png_ptr->transformations |= PNG_EXPAND;
#else
   {
      


      png_error(png_ptr,
        "Cannot do RGB_TO_GRAY without EXPAND_SUPPORTED");

      
   }
#endif
   {
      if (red >= 0 && green >= 0 && red + green <= PNG_FP_1)
      {
         png_uint_16 red_int, green_int;

         




         red_int = (png_uint_16)(((png_uint_32)red*32768)/100000);
         green_int = (png_uint_16)(((png_uint_32)green*32768)/100000);

         png_ptr->rgb_to_gray_red_coeff   = red_int;
         png_ptr->rgb_to_gray_green_coeff = green_int;
         png_ptr->rgb_to_gray_coefficients_set = 1;
      }

      else
      {
         if (red >= 0 && green >= 0)
            png_app_warning(png_ptr,
               "ignoring out of range rgb_to_gray coefficients");

         





         if (png_ptr->rgb_to_gray_red_coeff == 0 &&
            png_ptr->rgb_to_gray_green_coeff == 0)
         {
            png_ptr->rgb_to_gray_red_coeff   = 6968;
            png_ptr->rgb_to_gray_green_coeff = 23434;
            
         }
      }
   }
}

#ifdef PNG_FLOATING_POINT_SUPPORTED




void PNGAPI
png_set_rgb_to_gray(png_structrp png_ptr, int error_action, double red,
   double green)
{
   png_set_rgb_to_gray_fixed(png_ptr, error_action,
      png_fixed(png_ptr, red, "rgb to gray red coefficient"),
      png_fixed(png_ptr, green, "rgb to gray green coefficient"));
}
#endif 

#endif 

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED)
void PNGAPI
png_set_read_user_transform_fn(png_structrp png_ptr, png_user_transform_ptr
    read_user_transform_fn)
{
   png_debug(1, "in png_set_read_user_transform_fn");

#ifdef PNG_READ_USER_TRANSFORM_SUPPORTED
   png_ptr->transformations |= PNG_USER_TRANSFORM;
   png_ptr->read_user_transform_fn = read_user_transform_fn;
#endif
}
#endif

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
#ifdef PNG_READ_GAMMA_SUPPORTED




static int 
png_gamma_threshold(png_fixed_point screen_gamma, png_fixed_point file_gamma)
{
   






   png_fixed_point gtest;
   return !png_muldiv(&gtest, screen_gamma, file_gamma, PNG_FP_1) ||
       png_gamma_significant(gtest);
}
#endif










static void 
png_init_palette_transformations(png_structrp png_ptr)
{
   







   int input_has_alpha = 0;
   int input_has_transparency = 0;

   if (png_ptr->num_trans > 0)
   {
      int i;

      
      for (i=0; i<png_ptr->num_trans; ++i)
      {
         if (png_ptr->trans_alpha[i] == 255)
            continue;
         else if (png_ptr->trans_alpha[i] == 0)
            input_has_transparency = 1;
         else
         {
            input_has_transparency = 1;
            input_has_alpha = 1;
            break;
         }
      }
   }

   
   if (input_has_alpha == 0)
   {
      



      png_ptr->transformations &= ~PNG_ENCODE_ALPHA;
      png_ptr->flags &= ~PNG_FLAG_OPTIMIZE_ALPHA;

      if (input_has_transparency == 0)
         png_ptr->transformations &= ~(PNG_COMPOSE | PNG_BACKGROUND_EXPAND);
   }

#if defined(PNG_READ_EXPAND_SUPPORTED) && defined(PNG_READ_BACKGROUND_SUPPORTED)
   




   


   if ((png_ptr->transformations & PNG_BACKGROUND_EXPAND) != 0 &&
       (png_ptr->transformations & PNG_EXPAND) != 0)
   {
      {
         png_ptr->background.red   =
             png_ptr->palette[png_ptr->background.index].red;
         png_ptr->background.green =
             png_ptr->palette[png_ptr->background.index].green;
         png_ptr->background.blue  =
             png_ptr->palette[png_ptr->background.index].blue;

#ifdef PNG_READ_INVERT_ALPHA_SUPPORTED
        if ((png_ptr->transformations & PNG_INVERT_ALPHA) != 0)
        {
           if ((png_ptr->transformations & PNG_EXPAND_tRNS) == 0)
           {
              


              int i, istop = png_ptr->num_trans;

              for (i=0; i<istop; i++)
                 png_ptr->trans_alpha[i] = (png_byte)(255 -
                    png_ptr->trans_alpha[i]);
           }
        }
#endif 
      }
   } 
#endif 
}

static void 
png_init_rgb_transformations(png_structrp png_ptr)
{
   



   int input_has_alpha = (png_ptr->color_type & PNG_COLOR_MASK_ALPHA) != 0;
   int input_has_transparency = png_ptr->num_trans > 0;

   
   if (input_has_alpha == 0)
   {
      



#     ifdef PNG_READ_ALPHA_MODE_SUPPORTED
         png_ptr->transformations &= ~PNG_ENCODE_ALPHA;
         png_ptr->flags &= ~PNG_FLAG_OPTIMIZE_ALPHA;
#     endif

      if (input_has_transparency == 0)
         png_ptr->transformations &= ~(PNG_COMPOSE | PNG_BACKGROUND_EXPAND);
   }

#if defined(PNG_READ_EXPAND_SUPPORTED) && defined(PNG_READ_BACKGROUND_SUPPORTED)
   




   


   if ((png_ptr->transformations & PNG_BACKGROUND_EXPAND) != 0 &&
       (png_ptr->transformations & PNG_EXPAND) != 0 &&
       (png_ptr->color_type & PNG_COLOR_MASK_COLOR) == 0)
       
   {
      {
         
         int gray = png_ptr->background.gray;
         int trans_gray = png_ptr->trans_color.gray;

         switch (png_ptr->bit_depth)
         {
            case 1:
               gray *= 0xff;
               trans_gray *= 0xff;
               break;

            case 2:
               gray *= 0x55;
               trans_gray *= 0x55;
               break;

            case 4:
               gray *= 0x11;
               trans_gray *= 0x11;
               break;

            default:

            case 8:
               

            case 16:
               
               break;
         }

         png_ptr->background.red = png_ptr->background.green =
            png_ptr->background.blue = (png_uint_16)gray;

         if ((png_ptr->transformations & PNG_EXPAND_tRNS) == 0)
         {
            png_ptr->trans_color.red = png_ptr->trans_color.green =
               png_ptr->trans_color.blue = (png_uint_16)trans_gray;
         }
      }
   } 
#endif 
}

void 
png_init_read_transformations(png_structrp png_ptr)
{
   png_debug(1, "in png_init_read_transformations");

   







#ifdef PNG_READ_GAMMA_SUPPORTED
   





   {
      


      int gamma_correction = 0;

      if (png_ptr->colorspace.gamma != 0) 
      {
         if (png_ptr->screen_gamma != 0) 
            gamma_correction = png_gamma_threshold(png_ptr->colorspace.gamma,
               png_ptr->screen_gamma);

         else
            


            png_ptr->screen_gamma = png_reciprocal(png_ptr->colorspace.gamma);
      }

      else if (png_ptr->screen_gamma != 0)
         




         png_ptr->colorspace.gamma = png_reciprocal(png_ptr->screen_gamma);

      else 
         




         png_ptr->screen_gamma = png_ptr->colorspace.gamma = PNG_FP_1;

      
      png_ptr->colorspace.flags |= PNG_COLORSPACE_HAVE_GAMMA;

      







      if (gamma_correction != 0)
         png_ptr->transformations |= PNG_GAMMA;

      else
         png_ptr->transformations &= ~PNG_GAMMA;
   }
#endif

   




























#ifdef PNG_READ_STRIP_ALPHA_SUPPORTED
   if ((png_ptr->transformations & PNG_STRIP_ALPHA) != 0 &&
       (png_ptr->transformations & PNG_COMPOSE) == 0)
   {
      




      png_ptr->transformations &= ~(PNG_BACKGROUND_EXPAND | PNG_ENCODE_ALPHA |
         PNG_EXPAND_tRNS);
      png_ptr->flags &= ~PNG_FLAG_OPTIMIZE_ALPHA;

      






      png_ptr->num_trans = 0;
   }
#endif 

#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
   


   if (png_gamma_significant(png_ptr->screen_gamma) == 0)
   {
      png_ptr->transformations &= ~PNG_ENCODE_ALPHA;
      png_ptr->flags &= ~PNG_FLAG_OPTIMIZE_ALPHA;
   }
#endif

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
   


   if ((png_ptr->transformations & PNG_RGB_TO_GRAY) != 0)
      png_colorspace_set_rgb_coefficients(png_ptr);
#endif

#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED
#if defined(PNG_READ_EXPAND_SUPPORTED) && defined(PNG_READ_BACKGROUND_SUPPORTED)
   














   if ((png_ptr->transformations & PNG_BACKGROUND_EXPAND) != 0)
   {
      


      if ((png_ptr->color_type & PNG_COLOR_MASK_COLOR) == 0)
         png_ptr->mode |= PNG_BACKGROUND_IS_GRAY;
   }

   else if ((png_ptr->transformations & PNG_COMPOSE) != 0)
   {
      




      if ((png_ptr->transformations & PNG_GRAY_TO_RGB) != 0)
      {
         if (png_ptr->background.red == png_ptr->background.green &&
             png_ptr->background.red == png_ptr->background.blue)
         {
            png_ptr->mode |= PNG_BACKGROUND_IS_GRAY;
            png_ptr->background.gray = png_ptr->background.red;
         }
      }
   }
#endif 
#endif 

   










   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      png_init_palette_transformations(png_ptr);

   else
      png_init_rgb_transformations(png_ptr);

#if defined(PNG_READ_BACKGROUND_SUPPORTED) && \
   defined(PNG_READ_EXPAND_16_SUPPORTED)
   if ((png_ptr->transformations & PNG_EXPAND_16) != 0 &&
       (png_ptr->transformations & PNG_COMPOSE) != 0 &&
       (png_ptr->transformations & PNG_BACKGROUND_EXPAND) == 0 &&
       png_ptr->bit_depth != 16)
   {
      









#     define CHOP(x) (x)=((png_uint_16)PNG_DIV257(x))
      CHOP(png_ptr->background.red);
      CHOP(png_ptr->background.green);
      CHOP(png_ptr->background.blue);
      CHOP(png_ptr->background.gray);
#     undef CHOP
   }
#endif 

#if defined(PNG_READ_BACKGROUND_SUPPORTED) && \
   (defined(PNG_READ_SCALE_16_TO_8_SUPPORTED) || \
   defined(PNG_READ_STRIP_16_TO_8_SUPPORTED))
   if ((png_ptr->transformations & (PNG_16_TO_8|PNG_SCALE_16_TO_8)) != 0 &&
       (png_ptr->transformations & PNG_COMPOSE) != 0 &&
       (png_ptr->transformations & PNG_BACKGROUND_EXPAND) == 0 &&
       png_ptr->bit_depth == 16)
   {
      





      png_ptr->background.red = (png_uint_16)(png_ptr->background.red * 257);
      png_ptr->background.green =
         (png_uint_16)(png_ptr->background.green * 257);
      png_ptr->background.blue = (png_uint_16)(png_ptr->background.blue * 257);
      png_ptr->background.gray = (png_uint_16)(png_ptr->background.gray * 257);
   }
#endif

   







#ifdef PNG_READ_GAMMA_SUPPORTED
#  ifdef PNG_READ_BACKGROUND_SUPPORTED
      
      png_ptr->background_1 = png_ptr->background;
#  endif

   












   if ((png_ptr->transformations & PNG_GAMMA) != 0 ||
       ((png_ptr->transformations & PNG_RGB_TO_GRAY) != 0 &&
        (png_gamma_significant(png_ptr->colorspace.gamma) != 0 ||
         png_gamma_significant(png_ptr->screen_gamma) != 0)) ||
        ((png_ptr->transformations & PNG_COMPOSE) != 0 &&
         (png_gamma_significant(png_ptr->colorspace.gamma) != 0 ||
          png_gamma_significant(png_ptr->screen_gamma) != 0
#  ifdef PNG_READ_BACKGROUND_SUPPORTED
         || (png_ptr->background_gamma_type == PNG_BACKGROUND_GAMMA_UNIQUE &&
           png_gamma_significant(png_ptr->background_gamma) != 0)
#  endif
        )) || ((png_ptr->transformations & PNG_ENCODE_ALPHA) != 0 &&
       png_gamma_significant(png_ptr->screen_gamma) != 0))
   {
      png_build_gamma_table(png_ptr, png_ptr->bit_depth);

#ifdef PNG_READ_BACKGROUND_SUPPORTED
      if ((png_ptr->transformations & PNG_COMPOSE) != 0)
      {
         





         if ((png_ptr->transformations & PNG_RGB_TO_GRAY) != 0)
            png_warning(png_ptr,
               "libpng does not support gamma+background+rgb_to_gray");

         if ((png_ptr->color_type == PNG_COLOR_TYPE_PALETTE) != 0)
         {
            


            png_color back, back_1;
            png_colorp palette = png_ptr->palette;
            int num_palette = png_ptr->num_palette;
            int i;
            if (png_ptr->background_gamma_type == PNG_BACKGROUND_GAMMA_FILE)
            {

               back.red = png_ptr->gamma_table[png_ptr->background.red];
               back.green = png_ptr->gamma_table[png_ptr->background.green];
               back.blue = png_ptr->gamma_table[png_ptr->background.blue];

               back_1.red = png_ptr->gamma_to_1[png_ptr->background.red];
               back_1.green = png_ptr->gamma_to_1[png_ptr->background.green];
               back_1.blue = png_ptr->gamma_to_1[png_ptr->background.blue];
            }
            else
            {
               png_fixed_point g, gs;

               switch (png_ptr->background_gamma_type)
               {
                  case PNG_BACKGROUND_GAMMA_SCREEN:
                     g = (png_ptr->screen_gamma);
                     gs = PNG_FP_1;
                     break;

                  case PNG_BACKGROUND_GAMMA_FILE:
                     g = png_reciprocal(png_ptr->colorspace.gamma);
                     gs = png_reciprocal2(png_ptr->colorspace.gamma,
                        png_ptr->screen_gamma);
                     break;

                  case PNG_BACKGROUND_GAMMA_UNIQUE:
                     g = png_reciprocal(png_ptr->background_gamma);
                     gs = png_reciprocal2(png_ptr->background_gamma,
                        png_ptr->screen_gamma);
                     break;
                  default:
                     g = PNG_FP_1;    
                     gs = PNG_FP_1;   
                     break;
               }

               if (png_gamma_significant(gs) != 0)
               {
                  back.red = png_gamma_8bit_correct(png_ptr->background.red,
                      gs);
                  back.green = png_gamma_8bit_correct(png_ptr->background.green,
                      gs);
                  back.blue = png_gamma_8bit_correct(png_ptr->background.blue,
                      gs);
               }

               else
               {
                  back.red   = (png_byte)png_ptr->background.red;
                  back.green = (png_byte)png_ptr->background.green;
                  back.blue  = (png_byte)png_ptr->background.blue;
               }

               if (png_gamma_significant(g) != 0)
               {
                  back_1.red = png_gamma_8bit_correct(png_ptr->background.red,
                     g);
                  back_1.green = png_gamma_8bit_correct(
                     png_ptr->background.green, g);
                  back_1.blue = png_gamma_8bit_correct(png_ptr->background.blue,
                     g);
               }

               else
               {
                  back_1.red   = (png_byte)png_ptr->background.red;
                  back_1.green = (png_byte)png_ptr->background.green;
                  back_1.blue  = (png_byte)png_ptr->background.blue;
               }
            }

            for (i = 0; i < num_palette; i++)
            {
               if (i < (int)png_ptr->num_trans &&
                   png_ptr->trans_alpha[i] != 0xff)
               {
                  if (png_ptr->trans_alpha[i] == 0)
                  {
                     palette[i] = back;
                  }
                  else 
                  {
                     png_byte v, w;

                     v = png_ptr->gamma_to_1[palette[i].red];
                     png_composite(w, v, png_ptr->trans_alpha[i], back_1.red);
                     palette[i].red = png_ptr->gamma_from_1[w];

                     v = png_ptr->gamma_to_1[palette[i].green];
                     png_composite(w, v, png_ptr->trans_alpha[i], back_1.green);
                     palette[i].green = png_ptr->gamma_from_1[w];

                     v = png_ptr->gamma_to_1[palette[i].blue];
                     png_composite(w, v, png_ptr->trans_alpha[i], back_1.blue);
                     palette[i].blue = png_ptr->gamma_from_1[w];
                  }
               }
               else
               {
                  palette[i].red = png_ptr->gamma_table[palette[i].red];
                  palette[i].green = png_ptr->gamma_table[palette[i].green];
                  palette[i].blue = png_ptr->gamma_table[palette[i].blue];
               }
            }

            





            png_ptr->transformations &= ~(PNG_COMPOSE | PNG_GAMMA);
         } 

         
         else 
         {
            int gs_sig, g_sig;
            png_fixed_point g = PNG_FP_1;  
            png_fixed_point gs = PNG_FP_1; 

            switch (png_ptr->background_gamma_type)
            {
               case PNG_BACKGROUND_GAMMA_SCREEN:
                  g = png_ptr->screen_gamma;
                  
                  break;

               case PNG_BACKGROUND_GAMMA_FILE:
                  g = png_reciprocal(png_ptr->colorspace.gamma);
                  gs = png_reciprocal2(png_ptr->colorspace.gamma,
                     png_ptr->screen_gamma);
                  break;

               case PNG_BACKGROUND_GAMMA_UNIQUE:
                  g = png_reciprocal(png_ptr->background_gamma);
                  gs = png_reciprocal2(png_ptr->background_gamma,
                      png_ptr->screen_gamma);
                  break;

               default:
                  png_error(png_ptr, "invalid background gamma type");
            }

            g_sig = png_gamma_significant(g);
            gs_sig = png_gamma_significant(gs);

            if (g_sig != 0)
               png_ptr->background_1.gray = png_gamma_correct(png_ptr,
                   png_ptr->background.gray, g);

            if (gs_sig != 0)
               png_ptr->background.gray = png_gamma_correct(png_ptr,
                   png_ptr->background.gray, gs);

            if ((png_ptr->background.red != png_ptr->background.green) ||
                (png_ptr->background.red != png_ptr->background.blue) ||
                (png_ptr->background.red != png_ptr->background.gray))
            {
               
               if (g_sig != 0)
               {
                  png_ptr->background_1.red = png_gamma_correct(png_ptr,
                      png_ptr->background.red, g);

                  png_ptr->background_1.green = png_gamma_correct(png_ptr,
                      png_ptr->background.green, g);

                  png_ptr->background_1.blue = png_gamma_correct(png_ptr,
                      png_ptr->background.blue, g);
               }

               if (gs_sig != 0)
               {
                  png_ptr->background.red = png_gamma_correct(png_ptr,
                      png_ptr->background.red, gs);

                  png_ptr->background.green = png_gamma_correct(png_ptr,
                      png_ptr->background.green, gs);

                  png_ptr->background.blue = png_gamma_correct(png_ptr,
                      png_ptr->background.blue, gs);
               }
            }

            else
            {
               
               png_ptr->background_1.red = png_ptr->background_1.green
                   = png_ptr->background_1.blue = png_ptr->background_1.gray;

               png_ptr->background.red = png_ptr->background.green
                   = png_ptr->background.blue = png_ptr->background.gray;
            }

            
            png_ptr->background_gamma_type = PNG_BACKGROUND_GAMMA_SCREEN;
         } 
      }

      else
      
#endif 
      if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE
#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
         
         && ((png_ptr->transformations & PNG_EXPAND) == 0 ||
         (png_ptr->transformations & PNG_RGB_TO_GRAY) == 0)
#endif
         )
      {
         png_colorp palette = png_ptr->palette;
         int num_palette = png_ptr->num_palette;
         int i;

         


         for (i = 0; i < num_palette; i++)
         {
            palette[i].red = png_ptr->gamma_table[palette[i].red];
            palette[i].green = png_ptr->gamma_table[palette[i].green];
            palette[i].blue = png_ptr->gamma_table[palette[i].blue];
         }

         
         png_ptr->transformations &= ~PNG_GAMMA;
      } 
   }
#ifdef PNG_READ_BACKGROUND_SUPPORTED
   else
#endif
#endif 

#ifdef PNG_READ_BACKGROUND_SUPPORTED
   
   if ((png_ptr->transformations & PNG_COMPOSE) != 0 &&
       (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE))
   {
      int i;
      int istop = (int)png_ptr->num_trans;
      png_color back;
      png_colorp palette = png_ptr->palette;

      back.red   = (png_byte)png_ptr->background.red;
      back.green = (png_byte)png_ptr->background.green;
      back.blue  = (png_byte)png_ptr->background.blue;

      for (i = 0; i < istop; i++)
      {
         if (png_ptr->trans_alpha[i] == 0)
         {
            palette[i] = back;
         }

         else if (png_ptr->trans_alpha[i] != 0xff)
         {
            
            png_composite(palette[i].red, palette[i].red,
                png_ptr->trans_alpha[i], back.red);

            png_composite(palette[i].green, palette[i].green,
                png_ptr->trans_alpha[i], back.green);

            png_composite(palette[i].blue, palette[i].blue,
                png_ptr->trans_alpha[i], back.blue);
         }
      }

      png_ptr->transformations &= ~PNG_COMPOSE;
   }
#endif 

#ifdef PNG_READ_SHIFT_SUPPORTED
   if ((png_ptr->transformations & PNG_SHIFT) != 0 &&
       (png_ptr->transformations & PNG_EXPAND) == 0 &&
       (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE))
   {
      int i;
      int istop = png_ptr->num_palette;
      int shift = 8 - png_ptr->sig_bit.red;

      png_ptr->transformations &= ~PNG_SHIFT;

      



      if (shift > 0 && shift < 8)
         for (i=0; i<istop; ++i)
         {
            int component = png_ptr->palette[i].red;

            component >>= shift;
            png_ptr->palette[i].red = (png_byte)component;
         }

      shift = 8 - png_ptr->sig_bit.green;
      if (shift > 0 && shift < 8)
         for (i=0; i<istop; ++i)
         {
            int component = png_ptr->palette[i].green;

            component >>= shift;
            png_ptr->palette[i].green = (png_byte)component;
         }

      shift = 8 - png_ptr->sig_bit.blue;
      if (shift > 0 && shift < 8)
         for (i=0; i<istop; ++i)
         {
            int component = png_ptr->palette[i].blue;

            component >>= shift;
            png_ptr->palette[i].blue = (png_byte)component;
         }
   }
#endif  
}





void 
png_read_transform_info(png_structrp png_ptr, png_inforp info_ptr)
{
   png_debug(1, "in png_read_transform_info");

#ifdef PNG_READ_EXPAND_SUPPORTED
   if ((png_ptr->transformations & PNG_EXPAND) != 0)
   {
      if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         



         if (png_ptr->num_trans > 0)
            info_ptr->color_type = PNG_COLOR_TYPE_RGB_ALPHA;

         else
            info_ptr->color_type = PNG_COLOR_TYPE_RGB;

         info_ptr->bit_depth = 8;
         info_ptr->num_trans = 0;

         if (png_ptr->palette == NULL)
            png_error (png_ptr, "Palette is NULL in indexed image");
      }
      else
      {
         if (png_ptr->num_trans != 0)
         {
            if ((png_ptr->transformations & PNG_EXPAND_tRNS) != 0)
               info_ptr->color_type |= PNG_COLOR_MASK_ALPHA;
         }
         if (info_ptr->bit_depth < 8)
            info_ptr->bit_depth = 8;

         info_ptr->num_trans = 0;
      }
   }
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED) ||\
   defined(PNG_READ_ALPHA_MODE_SUPPORTED)
   


   if ((png_ptr->transformations & PNG_COMPOSE) != 0)
      info_ptr->background = png_ptr->background;
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED
   








   info_ptr->colorspace.gamma = png_ptr->colorspace.gamma;
#endif

   if (info_ptr->bit_depth == 16)
   {
#  ifdef PNG_READ_16BIT_SUPPORTED
#     ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
         if ((png_ptr->transformations & PNG_SCALE_16_TO_8) != 0)
            info_ptr->bit_depth = 8;
#     endif

#     ifdef PNG_READ_STRIP_16_TO_8_SUPPORTED
         if ((png_ptr->transformations & PNG_16_TO_8) != 0)
            info_ptr->bit_depth = 8;
#     endif

#  else
      



#     ifdef PNG_READ_STRIP_16_TO_8_SUPPORTED
         



         png_ptr->transformations |= PNG_16_TO_8;
         info_ptr->bit_depth = 8;
#     else

#        ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
            png_ptr->transformations |= PNG_SCALE_16_TO_8;
            info_ptr->bit_depth = 8;
#        else

            CONFIGURATION ERROR: you must enable at least one 16 to 8 method
#        endif
#    endif
#endif 
   }

#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED
   if ((png_ptr->transformations & PNG_GRAY_TO_RGB) != 0)
      info_ptr->color_type = (png_byte)(info_ptr->color_type |
         PNG_COLOR_MASK_COLOR);
#endif

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
   if ((png_ptr->transformations & PNG_RGB_TO_GRAY) != 0)
      info_ptr->color_type = (png_byte)(info_ptr->color_type &
         ~PNG_COLOR_MASK_COLOR);
#endif

#ifdef PNG_READ_QUANTIZE_SUPPORTED
   if ((png_ptr->transformations & PNG_QUANTIZE) != 0)
   {
      if (((info_ptr->color_type == PNG_COLOR_TYPE_RGB) ||
          (info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA)) &&
          png_ptr->palette_lookup != 0 && info_ptr->bit_depth == 8)
      {
         info_ptr->color_type = PNG_COLOR_TYPE_PALETTE;
      }
   }
#endif

#ifdef PNG_READ_EXPAND_16_SUPPORTED
   if ((png_ptr->transformations & PNG_EXPAND_16) != 0 &&
       info_ptr->bit_depth == 8 &&
       info_ptr->color_type != PNG_COLOR_TYPE_PALETTE)
   {
      info_ptr->bit_depth = 16;
   }
#endif

#ifdef PNG_READ_PACK_SUPPORTED
   if ((png_ptr->transformations & PNG_PACK) != 0 &&
       (info_ptr->bit_depth < 8))
      info_ptr->bit_depth = 8;
#endif

   if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      info_ptr->channels = 1;

   else if ((info_ptr->color_type & PNG_COLOR_MASK_COLOR) != 0)
      info_ptr->channels = 3;

   else
      info_ptr->channels = 1;

#ifdef PNG_READ_STRIP_ALPHA_SUPPORTED
   if ((png_ptr->transformations & PNG_STRIP_ALPHA) != 0)
   {
      info_ptr->color_type = (png_byte)(info_ptr->color_type &
         ~PNG_COLOR_MASK_ALPHA);
      info_ptr->num_trans = 0;
   }
#endif

   if ((info_ptr->color_type & PNG_COLOR_MASK_ALPHA) != 0)
      info_ptr->channels++;

#ifdef PNG_READ_FILLER_SUPPORTED
   
   if ((png_ptr->transformations & PNG_FILLER) != 0 &&
       (info_ptr->color_type == PNG_COLOR_TYPE_RGB ||
       info_ptr->color_type == PNG_COLOR_TYPE_GRAY))
   {
      info_ptr->channels++;
      
      if ((png_ptr->transformations & PNG_ADD_ALPHA) != 0)
         info_ptr->color_type |= PNG_COLOR_MASK_ALPHA;
   }
#endif

#if defined(PNG_USER_TRANSFORM_PTR_SUPPORTED) && \
defined(PNG_READ_USER_TRANSFORM_SUPPORTED)
   if ((png_ptr->transformations & PNG_USER_TRANSFORM) != 0)
   {
      if (info_ptr->bit_depth < png_ptr->user_transform_depth)
         info_ptr->bit_depth = png_ptr->user_transform_depth;

      if (info_ptr->channels < png_ptr->user_transform_channels)
         info_ptr->channels = png_ptr->user_transform_channels;
   }
#endif

   info_ptr->pixel_depth = (png_byte)(info_ptr->channels *
       info_ptr->bit_depth);

   info_ptr->rowbytes = PNG_ROWBYTES(info_ptr->pixel_depth, info_ptr->width);

   





   png_ptr->info_rowbytes = info_ptr->rowbytes;

#ifndef PNG_READ_EXPAND_SUPPORTED
   if (png_ptr != NULL)
      return;
#endif
}

#ifdef PNG_READ_PACK_SUPPORTED






static void
png_do_unpack(png_row_infop row_info, png_bytep row)
{
   png_debug(1, "in png_do_unpack");

   if (row_info->bit_depth < 8)
   {
      png_uint_32 i;
      png_uint_32 row_width=row_info->width;

      switch (row_info->bit_depth)
      {
         case 1:
         {
            png_bytep sp = row + (png_size_t)((row_width - 1) >> 3);
            png_bytep dp = row + (png_size_t)row_width - 1;
            png_uint_32 shift = 7 - (int)((row_width + 7) & 0x07);
            for (i = 0; i < row_width; i++)
            {
               *dp = (png_byte)((*sp >> shift) & 0x01);

               if (shift == 7)
               {
                  shift = 0;
                  sp--;
               }

               else
                  shift++;

               dp--;
            }
            break;
         }

         case 2:
         {

            png_bytep sp = row + (png_size_t)((row_width - 1) >> 2);
            png_bytep dp = row + (png_size_t)row_width - 1;
            png_uint_32 shift = (int)((3 - ((row_width + 3) & 0x03)) << 1);
            for (i = 0; i < row_width; i++)
            {
               *dp = (png_byte)((*sp >> shift) & 0x03);

               if (shift == 6)
               {
                  shift = 0;
                  sp--;
               }

               else
                  shift += 2;

               dp--;
            }
            break;
         }

         case 4:
         {
            png_bytep sp = row + (png_size_t)((row_width - 1) >> 1);
            png_bytep dp = row + (png_size_t)row_width - 1;
            png_uint_32 shift = (int)((1 - ((row_width + 1) & 0x01)) << 2);
            for (i = 0; i < row_width; i++)
            {
               *dp = (png_byte)((*sp >> shift) & 0x0f);

               if (shift == 4)
               {
                  shift = 0;
                  sp--;
               }

               else
                  shift = 4;

               dp--;
            }
            break;
         }

         default:
            break;
      }
      row_info->bit_depth = 8;
      row_info->pixel_depth = (png_byte)(8 * row_info->channels);
      row_info->rowbytes = row_width * row_info->channels;
   }
}
#endif

#ifdef PNG_READ_SHIFT_SUPPORTED





static void
png_do_unshift(png_row_infop row_info, png_bytep row,
    png_const_color_8p sig_bits)
{
   int color_type;

   png_debug(1, "in png_do_unshift");

   
   color_type = row_info->color_type;

   if (color_type != PNG_COLOR_TYPE_PALETTE)
   {
      int shift[4];
      int channels = 0;
      int bit_depth = row_info->bit_depth;

      if ((color_type & PNG_COLOR_MASK_COLOR) != 0)
      {
         shift[channels++] = bit_depth - sig_bits->red;
         shift[channels++] = bit_depth - sig_bits->green;
         shift[channels++] = bit_depth - sig_bits->blue;
      }

      else
      {
         shift[channels++] = bit_depth - sig_bits->gray;
      }

      if ((color_type & PNG_COLOR_MASK_ALPHA) != 0)
      {
         shift[channels++] = bit_depth - sig_bits->alpha;
      }

      {
         int c, have_shift;

         for (c = have_shift = 0; c < channels; ++c)
         {
            


            if (shift[c] <= 0 || shift[c] >= bit_depth)
               shift[c] = 0;

            else
               have_shift = 1;
         }

         if (have_shift == 0)
            return;
      }

      switch (bit_depth)
      {
         default:
         
            
            break;

         case 2:
         
         
         {
            png_bytep bp = row;
            png_bytep bp_end = bp + row_info->rowbytes;

            while (bp < bp_end)
            {
               int b = (*bp >> 1) & 0x55;
               *bp++ = (png_byte)b;
            }
            break;
         }

         case 4:
         
         
         {
            png_bytep bp = row;
            png_bytep bp_end = bp + row_info->rowbytes;
            int gray_shift = shift[0];
            int mask =  0xf >> gray_shift;

            mask |= mask << 4;

            while (bp < bp_end)
            {
               int b = (*bp >> gray_shift) & mask;
               *bp++ = (png_byte)b;
            }
            break;
         }

         case 8:
         
         {
            png_bytep bp = row;
            png_bytep bp_end = bp + row_info->rowbytes;
            int channel = 0;

            while (bp < bp_end)
            {
               int b = *bp >> shift[channel];
               if (++channel >= channels)
                  channel = 0;
               *bp++ = (png_byte)b;
            }
            break;
         }

#ifdef PNG_READ_16BIT_SUPPORTED
         case 16:
         
         {
            png_bytep bp = row;
            png_bytep bp_end = bp + row_info->rowbytes;
            int channel = 0;

            while (bp < bp_end)
            {
               int value = (bp[0] << 8) + bp[1];

               value >>= shift[channel];
               if (++channel >= channels)
                  channel = 0;
               *bp++ = (png_byte)(value >> 8);
               *bp++ = (png_byte)value;
            }
            break;
         }
#endif
      }
   }
}
#endif

#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED

static void
png_do_scale_16_to_8(png_row_infop row_info, png_bytep row)
{
   png_debug(1, "in png_do_scale_16_to_8");

   if (row_info->bit_depth == 16)
   {
      png_bytep sp = row; 
      png_bytep dp = row; 
      png_bytep ep = sp + row_info->rowbytes; 

      while (sp < ep)
      {
         































         png_int_32 tmp = *sp++; 
         tmp += (((int)*sp++ - tmp + 128) * 65535) >> 24;
         *dp++ = (png_byte)tmp;
      }

      row_info->bit_depth = 8;
      row_info->pixel_depth = (png_byte)(8 * row_info->channels);
      row_info->rowbytes = row_info->width * row_info->channels;
   }
}
#endif

#ifdef PNG_READ_STRIP_16_TO_8_SUPPORTED
static void



png_do_chop(png_row_infop row_info, png_bytep row)
{
   png_debug(1, "in png_do_chop");

   if (row_info->bit_depth == 16)
   {
      png_bytep sp = row; 
      png_bytep dp = row; 
      png_bytep ep = sp + row_info->rowbytes; 

      while (sp < ep)
      {
         *dp++ = *sp;
         sp += 2; 
      }

      row_info->bit_depth = 8;
      row_info->pixel_depth = (png_byte)(8 * row_info->channels);
      row_info->rowbytes = row_info->width * row_info->channels;
   }
}
#endif

#ifdef PNG_READ_SWAP_ALPHA_SUPPORTED
static void
png_do_read_swap_alpha(png_row_infop row_info, png_bytep row)
{
   png_debug(1, "in png_do_read_swap_alpha");

   {
      png_uint_32 row_width = row_info->width;
      if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
      {
         
         if (row_info->bit_depth == 8)
         {
            png_bytep sp = row + row_info->rowbytes;
            png_bytep dp = sp;
            png_byte save;
            png_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               save = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = save;
            }
         }

#ifdef PNG_READ_16BIT_SUPPORTED
         
         else
         {
            png_bytep sp = row + row_info->rowbytes;
            png_bytep dp = sp;
            png_byte save[2];
            png_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               save[0] = *(--sp);
               save[1] = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = save[0];
               *(--dp) = save[1];
            }
         }
#endif
      }

      else if (row_info->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      {
         
         if (row_info->bit_depth == 8)
         {
            png_bytep sp = row + row_info->rowbytes;
            png_bytep dp = sp;
            png_byte save;
            png_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               save = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = save;
            }
         }

#ifdef PNG_READ_16BIT_SUPPORTED
         
         else
         {
            png_bytep sp = row + row_info->rowbytes;
            png_bytep dp = sp;
            png_byte save[2];
            png_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               save[0] = *(--sp);
               save[1] = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = save[0];
               *(--dp) = save[1];
            }
         }
#endif
      }
   }
}
#endif

#ifdef PNG_READ_INVERT_ALPHA_SUPPORTED
static void
png_do_read_invert_alpha(png_row_infop row_info, png_bytep row)
{
   png_uint_32 row_width;
   png_debug(1, "in png_do_read_invert_alpha");

   row_width = row_info->width;
   if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
   {
      if (row_info->bit_depth == 8)
      {
         
         png_bytep sp = row + row_info->rowbytes;
         png_bytep dp = sp;
         png_uint_32 i;

         for (i = 0; i < row_width; i++)
         {
            *(--dp) = (png_byte)(255 - *(--sp));







            sp-=3;
            dp=sp;
         }
      }

#ifdef PNG_READ_16BIT_SUPPORTED
      
      else
      {
         png_bytep sp = row + row_info->rowbytes;
         png_bytep dp = sp;
         png_uint_32 i;

         for (i = 0; i < row_width; i++)
         {
            *(--dp) = (png_byte)(255 - *(--sp));
            *(--dp) = (png_byte)(255 - *(--sp));










            sp-=6;
            dp=sp;
         }
      }
#endif
   }
   else if (row_info->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
   {
      if (row_info->bit_depth == 8)
      {
         
         png_bytep sp = row + row_info->rowbytes;
         png_bytep dp = sp;
         png_uint_32 i;

         for (i = 0; i < row_width; i++)
         {
            *(--dp) = (png_byte)(255 - *(--sp));
            *(--dp) = *(--sp);
         }
      }

#ifdef PNG_READ_16BIT_SUPPORTED
      else
      {
         
         png_bytep sp  = row + row_info->rowbytes;
         png_bytep dp = sp;
         png_uint_32 i;

         for (i = 0; i < row_width; i++)
         {
            *(--dp) = (png_byte)(255 - *(--sp));
            *(--dp) = (png_byte)(255 - *(--sp));




            sp-=2;
            dp=sp;
         }
      }
#endif
   }
}
#endif

#ifdef PNG_READ_FILLER_SUPPORTED

static void
png_do_read_filler(png_row_infop row_info, png_bytep row,
    png_uint_32 filler, png_uint_32 flags)
{
   png_uint_32 i;
   png_uint_32 row_width = row_info->width;

#ifdef PNG_READ_16BIT_SUPPORTED
   png_byte hi_filler = (png_byte)(filler>>8);
#endif
   png_byte lo_filler = (png_byte)filler;

   png_debug(1, "in png_do_read_filler");

   if (
       row_info->color_type == PNG_COLOR_TYPE_GRAY)
   {
      if (row_info->bit_depth == 8)
      {
         if ((flags & PNG_FLAG_FILLER_AFTER) != 0)
         {
            
            png_bytep sp = row + (png_size_t)row_width;
            png_bytep dp =  sp + (png_size_t)row_width;
            for (i = 1; i < row_width; i++)
            {
               *(--dp) = lo_filler;
               *(--dp) = *(--sp);
            }
            *(--dp) = lo_filler;
            row_info->channels = 2;
            row_info->pixel_depth = 16;
            row_info->rowbytes = row_width * 2;
         }

         else
         {
            
            png_bytep sp = row + (png_size_t)row_width;
            png_bytep dp = sp  + (png_size_t)row_width;
            for (i = 0; i < row_width; i++)
            {
               *(--dp) = *(--sp);
               *(--dp) = lo_filler;
            }
            row_info->channels = 2;
            row_info->pixel_depth = 16;
            row_info->rowbytes = row_width * 2;
         }
      }

#ifdef PNG_READ_16BIT_SUPPORTED
      else if (row_info->bit_depth == 16)
      {
         if ((flags & PNG_FLAG_FILLER_AFTER) != 0)
         {
            
            png_bytep sp = row + (png_size_t)row_width * 2;
            png_bytep dp = sp  + (png_size_t)row_width * 2;
            for (i = 1; i < row_width; i++)
            {
               *(--dp) = lo_filler;
               *(--dp) = hi_filler;
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
            }
            *(--dp) = lo_filler;
            *(--dp) = hi_filler;
            row_info->channels = 2;
            row_info->pixel_depth = 32;
            row_info->rowbytes = row_width * 4;
         }

         else
         {
            
            png_bytep sp = row + (png_size_t)row_width * 2;
            png_bytep dp = sp  + (png_size_t)row_width * 2;
            for (i = 0; i < row_width; i++)
            {
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = lo_filler;
               *(--dp) = hi_filler;
            }
            row_info->channels = 2;
            row_info->pixel_depth = 32;
            row_info->rowbytes = row_width * 4;
         }
      }
#endif
   } 
   else if (row_info->color_type == PNG_COLOR_TYPE_RGB)
   {
      if (row_info->bit_depth == 8)
      {
         if ((flags & PNG_FLAG_FILLER_AFTER) != 0)
         {
            
            png_bytep sp = row + (png_size_t)row_width * 3;
            png_bytep dp = sp  + (png_size_t)row_width;
            for (i = 1; i < row_width; i++)
            {
               *(--dp) = lo_filler;
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
            }
            *(--dp) = lo_filler;
            row_info->channels = 4;
            row_info->pixel_depth = 32;
            row_info->rowbytes = row_width * 4;
         }

         else
         {
            
            png_bytep sp = row + (png_size_t)row_width * 3;
            png_bytep dp = sp + (png_size_t)row_width;
            for (i = 0; i < row_width; i++)
            {
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = lo_filler;
            }
            row_info->channels = 4;
            row_info->pixel_depth = 32;
            row_info->rowbytes = row_width * 4;
         }
      }

#ifdef PNG_READ_16BIT_SUPPORTED
      else if (row_info->bit_depth == 16)
      {
         if ((flags & PNG_FLAG_FILLER_AFTER) != 0)
         {
            
            png_bytep sp = row + (png_size_t)row_width * 6;
            png_bytep dp = sp  + (png_size_t)row_width * 2;
            for (i = 1; i < row_width; i++)
            {
               *(--dp) = lo_filler;
               *(--dp) = hi_filler;
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
            }
            *(--dp) = lo_filler;
            *(--dp) = hi_filler;
            row_info->channels = 4;
            row_info->pixel_depth = 64;
            row_info->rowbytes = row_width * 8;
         }

         else
         {
            
            png_bytep sp = row + (png_size_t)row_width * 6;
            png_bytep dp = sp  + (png_size_t)row_width * 2;
            for (i = 0; i < row_width; i++)
            {
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = lo_filler;
               *(--dp) = hi_filler;
            }

            row_info->channels = 4;
            row_info->pixel_depth = 64;
            row_info->rowbytes = row_width * 8;
         }
      }
#endif
   } 
}
#endif

#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED

static void
png_do_gray_to_rgb(png_row_infop row_info, png_bytep row)
{
   png_uint_32 i;
   png_uint_32 row_width = row_info->width;

   png_debug(1, "in png_do_gray_to_rgb");

   if (row_info->bit_depth >= 8 &&
       (row_info->color_type & PNG_COLOR_MASK_COLOR) == 0)
   {
      if (row_info->color_type == PNG_COLOR_TYPE_GRAY)
      {
         if (row_info->bit_depth == 8)
         {
            
            png_bytep sp = row + (png_size_t)row_width - 1;
            png_bytep dp = sp  + (png_size_t)row_width * 2;
            for (i = 0; i < row_width; i++)
            {
               *(dp--) = *sp;
               *(dp--) = *sp;
               *(dp--) = *(sp--);
            }
         }

         else
         {
            
            png_bytep sp = row + (png_size_t)row_width * 2 - 1;
            png_bytep dp = sp  + (png_size_t)row_width * 4;
            for (i = 0; i < row_width; i++)
            {
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *(sp--);
               *(dp--) = *(sp--);
            }
         }
      }

      else if (row_info->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      {
         if (row_info->bit_depth == 8)
         {
            
            png_bytep sp = row + (png_size_t)row_width * 2 - 1;
            png_bytep dp = sp  + (png_size_t)row_width * 2;
            for (i = 0; i < row_width; i++)
            {
               *(dp--) = *(sp--);
               *(dp--) = *sp;
               *(dp--) = *sp;
               *(dp--) = *(sp--);
            }
         }

         else
         {
            
            png_bytep sp = row + (png_size_t)row_width * 4 - 1;
            png_bytep dp = sp  + (png_size_t)row_width * 4;
            for (i = 0; i < row_width; i++)
            {
               *(dp--) = *(sp--);
               *(dp--) = *(sp--);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *(sp--);
               *(dp--) = *(sp--);
            }
         }
      }
      row_info->channels = (png_byte)(row_info->channels + 2);
      row_info->color_type |= PNG_COLOR_MASK_COLOR;
      row_info->pixel_depth = (png_byte)(row_info->channels *
          row_info->bit_depth);
      row_info->rowbytes = PNG_ROWBYTES(row_info->pixel_depth, row_width);
   }
}
#endif

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED




















































static int
png_do_rgb_to_gray(png_structrp png_ptr, png_row_infop row_info, png_bytep row)

{
   int rgb_error = 0;

   png_debug(1, "in png_do_rgb_to_gray");

   if ((row_info->color_type & PNG_COLOR_MASK_PALETTE) == 0 &&
       (row_info->color_type & PNG_COLOR_MASK_COLOR) != 0)
   {
      PNG_CONST png_uint_32 rc = png_ptr->rgb_to_gray_red_coeff;
      PNG_CONST png_uint_32 gc = png_ptr->rgb_to_gray_green_coeff;
      PNG_CONST png_uint_32 bc = 32768 - rc - gc;
      PNG_CONST png_uint_32 row_width = row_info->width;
      PNG_CONST int have_alpha =
         (row_info->color_type & PNG_COLOR_MASK_ALPHA) != 0;

      if (row_info->bit_depth == 8)
      {
#ifdef PNG_READ_GAMMA_SUPPORTED
         




         if (png_ptr->gamma_from_1 != NULL && png_ptr->gamma_to_1 != NULL)
         {
            png_bytep sp = row;
            png_bytep dp = row;
            png_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               png_byte red   = *(sp++);
               png_byte green = *(sp++);
               png_byte blue  = *(sp++);

               if (red != green || red != blue)
               {
                  red = png_ptr->gamma_to_1[red];
                  green = png_ptr->gamma_to_1[green];
                  blue = png_ptr->gamma_to_1[blue];

                  rgb_error |= 1;
                  *(dp++) = png_ptr->gamma_from_1[
                      (rc*red + gc*green + bc*blue + 16384)>>15];
               }

               else
               {
                  


                  if (png_ptr->gamma_table != NULL)
                     red = png_ptr->gamma_table[red];

                  *(dp++) = red;
               }

               if (have_alpha != 0)
                  *(dp++) = *(sp++);
            }
         }
         else
#endif
         {
            png_bytep sp = row;
            png_bytep dp = row;
            png_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               png_byte red   = *(sp++);
               png_byte green = *(sp++);
               png_byte blue  = *(sp++);

               if (red != green || red != blue)
               {
                  rgb_error |= 1;
                  


                  *(dp++) = (png_byte)((rc*red + gc*green + bc*blue)>>15);
               }

               else
                  *(dp++) = red;

               if (have_alpha != 0)
                  *(dp++) = *(sp++);
            }
         }
      }

      else 
      {
#ifdef PNG_READ_GAMMA_SUPPORTED
         if (png_ptr->gamma_16_to_1 != NULL && png_ptr->gamma_16_from_1 != NULL)
         {
            png_bytep sp = row;
            png_bytep dp = row;
            png_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               png_uint_16 red, green, blue, w;
               png_byte hi,lo;

               hi=*(sp)++; lo=*(sp)++; red   = (png_uint_16)((hi << 8) | (lo));
               hi=*(sp)++; lo=*(sp)++; green = (png_uint_16)((hi << 8) | (lo));
               hi=*(sp)++; lo=*(sp)++; blue  = (png_uint_16)((hi << 8) | (lo));

               if (red == green && red == blue)
               {
                  if (png_ptr->gamma_16_table != NULL)
                     w = png_ptr->gamma_16_table[(red & 0xff)
                         >> png_ptr->gamma_shift][red >> 8];

                  else
                     w = red;
               }

               else
               {
                  png_uint_16 red_1   = png_ptr->gamma_16_to_1[(red & 0xff)
                      >> png_ptr->gamma_shift][red>>8];
                  png_uint_16 green_1 =
                      png_ptr->gamma_16_to_1[(green & 0xff) >>
                      png_ptr->gamma_shift][green>>8];
                  png_uint_16 blue_1  = png_ptr->gamma_16_to_1[(blue & 0xff)
                      >> png_ptr->gamma_shift][blue>>8];
                  png_uint_16 gray16  = (png_uint_16)((rc*red_1 + gc*green_1
                      + bc*blue_1 + 16384)>>15);
                  w = png_ptr->gamma_16_from_1[(gray16 & 0xff) >>
                      png_ptr->gamma_shift][gray16 >> 8];
                  rgb_error |= 1;
               }

               *(dp++) = (png_byte)((w>>8) & 0xff);
               *(dp++) = (png_byte)(w & 0xff);

               if (have_alpha != 0)
               {
                  *(dp++) = *(sp++);
                  *(dp++) = *(sp++);
               }
            }
         }
         else
#endif
         {
            png_bytep sp = row;
            png_bytep dp = row;
            png_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               png_uint_16 red, green, blue, gray16;
               png_byte hi,lo;

               hi=*(sp)++; lo=*(sp)++; red   = (png_uint_16)((hi << 8) | (lo));
               hi=*(sp)++; lo=*(sp)++; green = (png_uint_16)((hi << 8) | (lo));
               hi=*(sp)++; lo=*(sp)++; blue  = (png_uint_16)((hi << 8) | (lo));

               if (red != green || red != blue)
                  rgb_error |= 1;

               



               gray16  = (png_uint_16)((rc*red + gc*green + bc*blue + 16384) >>
                  15);
               *(dp++) = (png_byte)((gray16 >> 8) & 0xff);
               *(dp++) = (png_byte)(gray16 & 0xff);

               if (have_alpha != 0)
               {
                  *(dp++) = *(sp++);
                  *(dp++) = *(sp++);
               }
            }
         }
      }

      row_info->channels = (png_byte)(row_info->channels - 2);
      row_info->color_type = (png_byte)(row_info->color_type &
          ~PNG_COLOR_MASK_COLOR);
      row_info->pixel_depth = (png_byte)(row_info->channels *
          row_info->bit_depth);
      row_info->rowbytes = PNG_ROWBYTES(row_info->pixel_depth, row_width);
   }
   return rgb_error;
}
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED) ||\
   defined(PNG_READ_ALPHA_MODE_SUPPORTED)




static void
png_do_compose(png_row_infop row_info, png_bytep row, png_structrp png_ptr)
{
#ifdef PNG_READ_GAMMA_SUPPORTED
   png_const_bytep gamma_table = png_ptr->gamma_table;
   png_const_bytep gamma_from_1 = png_ptr->gamma_from_1;
   png_const_bytep gamma_to_1 = png_ptr->gamma_to_1;
   png_const_uint_16pp gamma_16 = png_ptr->gamma_16_table;
   png_const_uint_16pp gamma_16_from_1 = png_ptr->gamma_16_from_1;
   png_const_uint_16pp gamma_16_to_1 = png_ptr->gamma_16_to_1;
   int gamma_shift = png_ptr->gamma_shift;
   int optimize = (png_ptr->flags & PNG_FLAG_OPTIMIZE_ALPHA) != 0;
#endif

   png_bytep sp;
   png_uint_32 i;
   png_uint_32 row_width = row_info->width;
   int shift;

   png_debug(1, "in png_do_compose");

   {
      switch (row_info->color_type)
      {
         case PNG_COLOR_TYPE_GRAY:
         {
            switch (row_info->bit_depth)
            {
               case 1:
               {
                  sp = row;
                  shift = 7;
                  for (i = 0; i < row_width; i++)
                  {
                     if ((png_uint_16)((*sp >> shift) & 0x01)
                        == png_ptr->trans_color.gray)
                     {
                        unsigned int tmp = *sp & (0x7f7f >> (7 - shift));
                        tmp |= png_ptr->background.gray << shift;
                        *sp = (png_byte)(tmp & 0xff);
                     }

                     if (shift == 0)
                     {
                        shift = 7;
                        sp++;
                     }

                     else
                        shift--;
                  }
                  break;
               }

               case 2:
               {
#ifdef PNG_READ_GAMMA_SUPPORTED
                  if (gamma_table != NULL)
                  {
                     sp = row;
                     shift = 6;
                     for (i = 0; i < row_width; i++)
                     {
                        if ((png_uint_16)((*sp >> shift) & 0x03)
                            == png_ptr->trans_color.gray)
                        {
                           unsigned int tmp = *sp & (0x3f3f >> (6 - shift));
                           tmp |= png_ptr->background.gray << shift;
                           *sp = (png_byte)(tmp & 0xff);
                        }

                        else
                        {
                           unsigned int p = (*sp >> shift) & 0x03;
                           unsigned int g = (gamma_table [p | (p << 2) |
                               (p << 4) | (p << 6)] >> 6) & 0x03;
                           unsigned int tmp = *sp & (0x3f3f >> (6 - shift));
                           tmp |= g << shift;
                           *sp = (png_byte)(tmp & 0xff);
                        }

                        if (shift == 0)
                        {
                           shift = 6;
                           sp++;
                        }

                        else
                           shift -= 2;
                     }
                  }

                  else
#endif
                  {
                     sp = row;
                     shift = 6;
                     for (i = 0; i < row_width; i++)
                     {
                        if ((png_uint_16)((*sp >> shift) & 0x03)
                            == png_ptr->trans_color.gray)
                        {
                           unsigned int tmp = *sp & (0x3f3f >> (6 - shift));
                           tmp |= png_ptr->background.gray << shift;
                           *sp = (png_byte)(tmp & 0xff);
                        }

                        if (shift == 0)
                        {
                           shift = 6;
                           sp++;
                        }

                        else
                           shift -= 2;
                     }
                  }
                  break;
               }

               case 4:
               {
#ifdef PNG_READ_GAMMA_SUPPORTED
                  if (gamma_table != NULL)
                  {
                     sp = row;
                     shift = 4;
                     for (i = 0; i < row_width; i++)
                     {
                        if ((png_uint_16)((*sp >> shift) & 0x0f)
                            == png_ptr->trans_color.gray)
                        {
                           unsigned int tmp = *sp & (0xf0f >> (4 - shift));
                           tmp |= png_ptr->background.gray << shift;
                           *sp = (png_byte)(tmp & 0xff);
                        }

                        else
                        {
                           unsigned int p = (*sp >> shift) & 0x0f;
                           unsigned int g = (gamma_table[p | (p << 4)] >> 4) &
                              0x0f;
                           unsigned int tmp = *sp & (0xf0f >> (4 - shift));
                           tmp |= g << shift;
                           *sp = (png_byte)(tmp & 0xff);
                        }

                        if (shift == 0)
                        {
                           shift = 4;
                           sp++;
                        }

                        else
                           shift -= 4;
                     }
                  }

                  else
#endif
                  {
                     sp = row;
                     shift = 4;
                     for (i = 0; i < row_width; i++)
                     {
                        if ((png_uint_16)((*sp >> shift) & 0x0f)
                            == png_ptr->trans_color.gray)
                        {
                           unsigned int tmp = *sp & (0xf0f >> (4 - shift));
                           tmp |= png_ptr->background.gray << shift;
                           *sp = (png_byte)(tmp & 0xff);
                        }

                        if (shift == 0)
                        {
                           shift = 4;
                           sp++;
                        }

                        else
                           shift -= 4;
                     }
                  }
                  break;
               }

               case 8:
               {
#ifdef PNG_READ_GAMMA_SUPPORTED
                  if (gamma_table != NULL)
                  {
                     sp = row;
                     for (i = 0; i < row_width; i++, sp++)
                     {
                        if (*sp == png_ptr->trans_color.gray)
                           *sp = (png_byte)png_ptr->background.gray;

                        else
                           *sp = gamma_table[*sp];
                     }
                  }
                  else
#endif
                  {
                     sp = row;
                     for (i = 0; i < row_width; i++, sp++)
                     {
                        if (*sp == png_ptr->trans_color.gray)
                           *sp = (png_byte)png_ptr->background.gray;
                     }
                  }
                  break;
               }

               case 16:
               {
#ifdef PNG_READ_GAMMA_SUPPORTED
                  if (gamma_16 != NULL)
                  {
                     sp = row;
                     for (i = 0; i < row_width; i++, sp += 2)
                     {
                        png_uint_16 v;

                        v = (png_uint_16)(((*sp) << 8) + *(sp + 1));

                        if (v == png_ptr->trans_color.gray)
                        {
                           
                           *sp = (png_byte)((png_ptr->background.gray >> 8)
                                & 0xff);
                           *(sp + 1) = (png_byte)(png_ptr->background.gray
                                & 0xff);
                        }

                        else
                        {
                           v = gamma_16[*(sp + 1) >> gamma_shift][*sp];
                           *sp = (png_byte)((v >> 8) & 0xff);
                           *(sp + 1) = (png_byte)(v & 0xff);
                        }
                     }
                  }
                  else
#endif
                  {
                     sp = row;
                     for (i = 0; i < row_width; i++, sp += 2)
                     {
                        png_uint_16 v;

                        v = (png_uint_16)(((*sp) << 8) + *(sp + 1));

                        if (v == png_ptr->trans_color.gray)
                        {
                           *sp = (png_byte)((png_ptr->background.gray >> 8)
                                & 0xff);
                           *(sp + 1) = (png_byte)(png_ptr->background.gray
                                & 0xff);
                        }
                     }
                  }
                  break;
               }

               default:
                  break;
            }
            break;
         }

         case PNG_COLOR_TYPE_RGB:
         {
            if (row_info->bit_depth == 8)
            {
#ifdef PNG_READ_GAMMA_SUPPORTED
               if (gamma_table != NULL)
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 3)
                  {
                     if (*sp == png_ptr->trans_color.red &&
                         *(sp + 1) == png_ptr->trans_color.green &&
                         *(sp + 2) == png_ptr->trans_color.blue)
                     {
                        *sp = (png_byte)png_ptr->background.red;
                        *(sp + 1) = (png_byte)png_ptr->background.green;
                        *(sp + 2) = (png_byte)png_ptr->background.blue;
                     }

                     else
                     {
                        *sp = gamma_table[*sp];
                        *(sp + 1) = gamma_table[*(sp + 1)];
                        *(sp + 2) = gamma_table[*(sp + 2)];
                     }
                  }
               }
               else
#endif
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 3)
                  {
                     if (*sp == png_ptr->trans_color.red &&
                         *(sp + 1) == png_ptr->trans_color.green &&
                         *(sp + 2) == png_ptr->trans_color.blue)
                     {
                        *sp = (png_byte)png_ptr->background.red;
                        *(sp + 1) = (png_byte)png_ptr->background.green;
                        *(sp + 2) = (png_byte)png_ptr->background.blue;
                     }
                  }
               }
            }
            else 
            {
#ifdef PNG_READ_GAMMA_SUPPORTED
               if (gamma_16 != NULL)
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 6)
                  {
                     png_uint_16 r = (png_uint_16)(((*sp) << 8) + *(sp + 1));

                     png_uint_16 g = (png_uint_16)(((*(sp + 2)) << 8)
                         + *(sp + 3));

                     png_uint_16 b = (png_uint_16)(((*(sp + 4)) << 8)
                         + *(sp + 5));

                     if (r == png_ptr->trans_color.red &&
                         g == png_ptr->trans_color.green &&
                         b == png_ptr->trans_color.blue)
                     {
                        
                        *sp = (png_byte)((png_ptr->background.red >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(png_ptr->background.red & 0xff);
                        *(sp + 2) = (png_byte)((png_ptr->background.green >> 8)
                                & 0xff);
                        *(sp + 3) = (png_byte)(png_ptr->background.green
                                & 0xff);
                        *(sp + 4) = (png_byte)((png_ptr->background.blue >> 8)
                                & 0xff);
                        *(sp + 5) = (png_byte)(png_ptr->background.blue & 0xff);
                     }

                     else
                     {
                        png_uint_16 v = gamma_16[*(sp + 1) >> gamma_shift][*sp];
                        *sp = (png_byte)((v >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(v & 0xff);

                        v = gamma_16[*(sp + 3) >> gamma_shift][*(sp + 2)];
                        *(sp + 2) = (png_byte)((v >> 8) & 0xff);
                        *(sp + 3) = (png_byte)(v & 0xff);

                        v = gamma_16[*(sp + 5) >> gamma_shift][*(sp + 4)];
                        *(sp + 4) = (png_byte)((v >> 8) & 0xff);
                        *(sp + 5) = (png_byte)(v & 0xff);
                     }
                  }
               }

               else
#endif
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 6)
                  {
                     png_uint_16 r = (png_uint_16)(((*sp) << 8) + *(sp + 1));

                     png_uint_16 g = (png_uint_16)(((*(sp + 2)) << 8)
                         + *(sp + 3));

                     png_uint_16 b = (png_uint_16)(((*(sp + 4)) << 8)
                         + *(sp + 5));

                     if (r == png_ptr->trans_color.red &&
                         g == png_ptr->trans_color.green &&
                         b == png_ptr->trans_color.blue)
                     {
                        *sp = (png_byte)((png_ptr->background.red >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(png_ptr->background.red & 0xff);
                        *(sp + 2) = (png_byte)((png_ptr->background.green >> 8)
                                & 0xff);
                        *(sp + 3) = (png_byte)(png_ptr->background.green
                                & 0xff);
                        *(sp + 4) = (png_byte)((png_ptr->background.blue >> 8)
                                & 0xff);
                        *(sp + 5) = (png_byte)(png_ptr->background.blue & 0xff);
                     }
                  }
               }
            }
            break;
         }

         case PNG_COLOR_TYPE_GRAY_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
#ifdef PNG_READ_GAMMA_SUPPORTED
               if (gamma_to_1 != NULL && gamma_from_1 != NULL &&
                   gamma_table != NULL)
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 2)
                  {
                     png_uint_16 a = *(sp + 1);

                     if (a == 0xff)
                        *sp = gamma_table[*sp];

                     else if (a == 0)
                     {
                        
                        *sp = (png_byte)png_ptr->background.gray;
                     }

                     else
                     {
                        png_byte v, w;

                        v = gamma_to_1[*sp];
                        png_composite(w, v, a, png_ptr->background_1.gray);
                        if (optimize == 0)
                           w = gamma_from_1[w];
                        *sp = w;
                     }
                  }
               }
               else
#endif
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 2)
                  {
                     png_byte a = *(sp + 1);

                     if (a == 0)
                        *sp = (png_byte)png_ptr->background.gray;

                     else if (a < 0xff)
                        png_composite(*sp, *sp, a, png_ptr->background.gray);
                  }
               }
            }
            else 
            {
#ifdef PNG_READ_GAMMA_SUPPORTED
               if (gamma_16 != NULL && gamma_16_from_1 != NULL &&
                   gamma_16_to_1 != NULL)
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 4)
                  {
                     png_uint_16 a = (png_uint_16)(((*(sp + 2)) << 8)
                         + *(sp + 3));

                     if (a == (png_uint_16)0xffff)
                     {
                        png_uint_16 v;

                        v = gamma_16[*(sp + 1) >> gamma_shift][*sp];
                        *sp = (png_byte)((v >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(v & 0xff);
                     }

                     else if (a == 0)
                     {
                        
                        *sp = (png_byte)((png_ptr->background.gray >> 8)
                                & 0xff);
                        *(sp + 1) = (png_byte)(png_ptr->background.gray & 0xff);
                     }

                     else
                     {
                        png_uint_16 g, v, w;

                        g = gamma_16_to_1[*(sp + 1) >> gamma_shift][*sp];
                        png_composite_16(v, g, a, png_ptr->background_1.gray);
                        if (optimize != 0)
                           w = v;
                        else
                           w = gamma_16_from_1[(v & 0xff) >>
                               gamma_shift][v >> 8];
                        *sp = (png_byte)((w >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(w & 0xff);
                     }
                  }
               }
               else
#endif
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 4)
                  {
                     png_uint_16 a = (png_uint_16)(((*(sp + 2)) << 8)
                         + *(sp + 3));

                     if (a == 0)
                     {
                        *sp = (png_byte)((png_ptr->background.gray >> 8)
                                & 0xff);
                        *(sp + 1) = (png_byte)(png_ptr->background.gray & 0xff);
                     }

                     else if (a < 0xffff)
                     {
                        png_uint_16 g, v;

                        g = (png_uint_16)(((*sp) << 8) + *(sp + 1));
                        png_composite_16(v, g, a, png_ptr->background.gray);
                        *sp = (png_byte)((v >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(v & 0xff);
                     }
                  }
               }
            }
            break;
         }

         case PNG_COLOR_TYPE_RGB_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
#ifdef PNG_READ_GAMMA_SUPPORTED
               if (gamma_to_1 != NULL && gamma_from_1 != NULL &&
                   gamma_table != NULL)
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 4)
                  {
                     png_byte a = *(sp + 3);

                     if (a == 0xff)
                     {
                        *sp = gamma_table[*sp];
                        *(sp + 1) = gamma_table[*(sp + 1)];
                        *(sp + 2) = gamma_table[*(sp + 2)];
                     }

                     else if (a == 0)
                     {
                        
                        *sp = (png_byte)png_ptr->background.red;
                        *(sp + 1) = (png_byte)png_ptr->background.green;
                        *(sp + 2) = (png_byte)png_ptr->background.blue;
                     }

                     else
                     {
                        png_byte v, w;

                        v = gamma_to_1[*sp];
                        png_composite(w, v, a, png_ptr->background_1.red);
                        if (optimize == 0) w = gamma_from_1[w];
                        *sp = w;

                        v = gamma_to_1[*(sp + 1)];
                        png_composite(w, v, a, png_ptr->background_1.green);
                        if (optimize == 0) w = gamma_from_1[w];
                        *(sp + 1) = w;

                        v = gamma_to_1[*(sp + 2)];
                        png_composite(w, v, a, png_ptr->background_1.blue);
                        if (optimize == 0) w = gamma_from_1[w];
                        *(sp + 2) = w;
                     }
                  }
               }
               else
#endif
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 4)
                  {
                     png_byte a = *(sp + 3);

                     if (a == 0)
                     {
                        *sp = (png_byte)png_ptr->background.red;
                        *(sp + 1) = (png_byte)png_ptr->background.green;
                        *(sp + 2) = (png_byte)png_ptr->background.blue;
                     }

                     else if (a < 0xff)
                     {
                        png_composite(*sp, *sp, a, png_ptr->background.red);

                        png_composite(*(sp + 1), *(sp + 1), a,
                            png_ptr->background.green);

                        png_composite(*(sp + 2), *(sp + 2), a,
                            png_ptr->background.blue);
                     }
                  }
               }
            }
            else 
            {
#ifdef PNG_READ_GAMMA_SUPPORTED
               if (gamma_16 != NULL && gamma_16_from_1 != NULL &&
                   gamma_16_to_1 != NULL)
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 8)
                  {
                     png_uint_16 a = (png_uint_16)(((png_uint_16)(*(sp + 6))
                         << 8) + (png_uint_16)(*(sp + 7)));

                     if (a == (png_uint_16)0xffff)
                     {
                        png_uint_16 v;

                        v = gamma_16[*(sp + 1) >> gamma_shift][*sp];
                        *sp = (png_byte)((v >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(v & 0xff);

                        v = gamma_16[*(sp + 3) >> gamma_shift][*(sp + 2)];
                        *(sp + 2) = (png_byte)((v >> 8) & 0xff);
                        *(sp + 3) = (png_byte)(v & 0xff);

                        v = gamma_16[*(sp + 5) >> gamma_shift][*(sp + 4)];
                        *(sp + 4) = (png_byte)((v >> 8) & 0xff);
                        *(sp + 5) = (png_byte)(v & 0xff);
                     }

                     else if (a == 0)
                     {
                        
                        *sp = (png_byte)((png_ptr->background.red >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(png_ptr->background.red & 0xff);
                        *(sp + 2) = (png_byte)((png_ptr->background.green >> 8)
                                & 0xff);
                        *(sp + 3) = (png_byte)(png_ptr->background.green
                                & 0xff);
                        *(sp + 4) = (png_byte)((png_ptr->background.blue >> 8)
                                & 0xff);
                        *(sp + 5) = (png_byte)(png_ptr->background.blue & 0xff);
                     }

                     else
                     {
                        png_uint_16 v, w;

                        v = gamma_16_to_1[*(sp + 1) >> gamma_shift][*sp];
                        png_composite_16(w, v, a, png_ptr->background_1.red);
                        if (optimize == 0)
                           w = gamma_16_from_1[((w & 0xff) >> gamma_shift)][w >>
                                8];
                        *sp = (png_byte)((w >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(w & 0xff);

                        v = gamma_16_to_1[*(sp + 3) >> gamma_shift][*(sp + 2)];
                        png_composite_16(w, v, a, png_ptr->background_1.green);
                        if (optimize == 0)
                           w = gamma_16_from_1[((w & 0xff) >> gamma_shift)][w >>
                                8];

                        *(sp + 2) = (png_byte)((w >> 8) & 0xff);
                        *(sp + 3) = (png_byte)(w & 0xff);

                        v = gamma_16_to_1[*(sp + 5) >> gamma_shift][*(sp + 4)];
                        png_composite_16(w, v, a, png_ptr->background_1.blue);
                        if (optimize == 0)
                           w = gamma_16_from_1[((w & 0xff) >> gamma_shift)][w >>
                                8];

                        *(sp + 4) = (png_byte)((w >> 8) & 0xff);
                        *(sp + 5) = (png_byte)(w & 0xff);
                     }
                  }
               }

               else
#endif
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 8)
                  {
                     png_uint_16 a = (png_uint_16)(((png_uint_16)(*(sp + 6))
                         << 8) + (png_uint_16)(*(sp + 7)));

                     if (a == 0)
                     {
                        *sp = (png_byte)((png_ptr->background.red >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(png_ptr->background.red & 0xff);
                        *(sp + 2) = (png_byte)((png_ptr->background.green >> 8)
                                & 0xff);
                        *(sp + 3) = (png_byte)(png_ptr->background.green
                                & 0xff);
                        *(sp + 4) = (png_byte)((png_ptr->background.blue >> 8)
                                & 0xff);
                        *(sp + 5) = (png_byte)(png_ptr->background.blue & 0xff);
                     }

                     else if (a < 0xffff)
                     {
                        png_uint_16 v;

                        png_uint_16 r = (png_uint_16)(((*sp) << 8) + *(sp + 1));
                        png_uint_16 g = (png_uint_16)(((*(sp + 2)) << 8)
                            + *(sp + 3));
                        png_uint_16 b = (png_uint_16)(((*(sp + 4)) << 8)
                            + *(sp + 5));

                        png_composite_16(v, r, a, png_ptr->background.red);
                        *sp = (png_byte)((v >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(v & 0xff);

                        png_composite_16(v, g, a, png_ptr->background.green);
                        *(sp + 2) = (png_byte)((v >> 8) & 0xff);
                        *(sp + 3) = (png_byte)(v & 0xff);

                        png_composite_16(v, b, a, png_ptr->background.blue);
                        *(sp + 4) = (png_byte)((v >> 8) & 0xff);
                        *(sp + 5) = (png_byte)(v & 0xff);
                     }
                  }
               }
            }
            break;
         }

         default:
            break;
      }
   }
}
#endif 

#ifdef PNG_READ_GAMMA_SUPPORTED






static void
png_do_gamma(png_row_infop row_info, png_bytep row, png_structrp png_ptr)
{
   png_const_bytep gamma_table = png_ptr->gamma_table;
   png_const_uint_16pp gamma_16_table = png_ptr->gamma_16_table;
   int gamma_shift = png_ptr->gamma_shift;

   png_bytep sp;
   png_uint_32 i;
   png_uint_32 row_width=row_info->width;

   png_debug(1, "in png_do_gamma");

   if (((row_info->bit_depth <= 8 && gamma_table != NULL) ||
       (row_info->bit_depth == 16 && gamma_16_table != NULL)))
   {
      switch (row_info->color_type)
      {
         case PNG_COLOR_TYPE_RGB:
         {
            if (row_info->bit_depth == 8)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
               }
            }

            else 
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  png_uint_16 v;

                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 2;

                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 2;

                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 2;
               }
            }
            break;
         }

         case PNG_COLOR_TYPE_RGB_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;

                  *sp = gamma_table[*sp];
                  sp++;

                  *sp = gamma_table[*sp];
                  sp++;

                  sp++;
               }
            }

            else 
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  png_uint_16 v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 2;

                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 2;

                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 4;
               }
            }
            break;
         }

         case PNG_COLOR_TYPE_GRAY_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp += 2;
               }
            }

            else 
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  png_uint_16 v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 4;
               }
            }
            break;
         }

         case PNG_COLOR_TYPE_GRAY:
         {
            if (row_info->bit_depth == 2)
            {
               sp = row;
               for (i = 0; i < row_width; i += 4)
               {
                  int a = *sp & 0xc0;
                  int b = *sp & 0x30;
                  int c = *sp & 0x0c;
                  int d = *sp & 0x03;

                  *sp = (png_byte)(
                      ((((int)gamma_table[a|(a>>2)|(a>>4)|(a>>6)])   ) & 0xc0)|
                      ((((int)gamma_table[(b<<2)|b|(b>>2)|(b>>4)])>>2) & 0x30)|
                      ((((int)gamma_table[(c<<4)|(c<<2)|c|(c>>2)])>>4) & 0x0c)|
                      ((((int)gamma_table[(d<<6)|(d<<4)|(d<<2)|d])>>6) ));
                  sp++;
               }
            }

            if (row_info->bit_depth == 4)
            {
               sp = row;
               for (i = 0; i < row_width; i += 2)
               {
                  int msb = *sp & 0xf0;
                  int lsb = *sp & 0x0f;

                  *sp = (png_byte)((((int)gamma_table[msb | (msb >> 4)]) & 0xf0)
                      | (((int)gamma_table[(lsb << 4) | lsb]) >> 4));
                  sp++;
               }
            }

            else if (row_info->bit_depth == 8)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
               }
            }

            else if (row_info->bit_depth == 16)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  png_uint_16 v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 2;
               }
            }
            break;
         }

         default:
            break;
      }
   }
}
#endif

#ifdef PNG_READ_ALPHA_MODE_SUPPORTED




static void
png_do_encode_alpha(png_row_infop row_info, png_bytep row, png_structrp png_ptr)
{
   png_uint_32 row_width = row_info->width;

   png_debug(1, "in png_do_encode_alpha");

   if ((row_info->color_type & PNG_COLOR_MASK_ALPHA) != 0)
   {
      if (row_info->bit_depth == 8)
      {
         PNG_CONST png_bytep table = png_ptr->gamma_from_1;

         if (table != NULL)
         {
            PNG_CONST int step =
               (row_info->color_type & PNG_COLOR_MASK_COLOR) ? 4 : 2;

            
            row += step - 1;

            for (; row_width > 0; --row_width, row += step)
               *row = table[*row];

            return;
         }
      }

      else if (row_info->bit_depth == 16)
      {
         PNG_CONST png_uint_16pp table = png_ptr->gamma_16_from_1;
         PNG_CONST int gamma_shift = png_ptr->gamma_shift;

         if (table != NULL)
         {
            PNG_CONST int step =
               (row_info->color_type & PNG_COLOR_MASK_COLOR) ? 8 : 4;

            
            row += step - 2;

            for (; row_width > 0; --row_width, row += step)
            {
               png_uint_16 v;

               v = table[*(row + 1) >> gamma_shift][*row];
               *row = (png_byte)((v >> 8) & 0xff);
               *(row + 1) = (png_byte)(v & 0xff);
            }

            return;
         }
      }
   }

   


   png_warning(png_ptr, "png_do_encode_alpha: unexpected call");
}
#endif

#ifdef PNG_READ_EXPAND_SUPPORTED



static void
png_do_expand_palette(png_row_infop row_info, png_bytep row,
   png_const_colorp palette, png_const_bytep trans_alpha, int num_trans)
{
   int shift, value;
   png_bytep sp, dp;
   png_uint_32 i;
   png_uint_32 row_width=row_info->width;

   png_debug(1, "in png_do_expand_palette");

   if (row_info->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (row_info->bit_depth < 8)
      {
         switch (row_info->bit_depth)
         {
            case 1:
            {
               sp = row + (png_size_t)((row_width - 1) >> 3);
               dp = row + (png_size_t)row_width - 1;
               shift = 7 - (int)((row_width + 7) & 0x07);
               for (i = 0; i < row_width; i++)
               {
                  if ((*sp >> shift) & 0x01)
                     *dp = 1;

                  else
                     *dp = 0;

                  if (shift == 7)
                  {
                     shift = 0;
                     sp--;
                  }

                  else
                     shift++;

                  dp--;
               }
               break;
            }

            case 2:
            {
               sp = row + (png_size_t)((row_width - 1) >> 2);
               dp = row + (png_size_t)row_width - 1;
               shift = (int)((3 - ((row_width + 3) & 0x03)) << 1);
               for (i = 0; i < row_width; i++)
               {
                  value = (*sp >> shift) & 0x03;
                  *dp = (png_byte)value;
                  if (shift == 6)
                  {
                     shift = 0;
                     sp--;
                  }

                  else
                     shift += 2;

                  dp--;
               }
               break;
            }

            case 4:
            {
               sp = row + (png_size_t)((row_width - 1) >> 1);
               dp = row + (png_size_t)row_width - 1;
               shift = (int)((row_width & 0x01) << 2);
               for (i = 0; i < row_width; i++)
               {
                  value = (*sp >> shift) & 0x0f;
                  *dp = (png_byte)value;
                  if (shift == 4)
                  {
                     shift = 0;
                     sp--;
                  }

                  else
                     shift += 4;

                  dp--;
               }
               break;
            }

            default:
               break;
         }
         row_info->bit_depth = 8;
         row_info->pixel_depth = 8;
         row_info->rowbytes = row_width;
      }

      if (row_info->bit_depth == 8)
      {
         {
            if (num_trans > 0)
            {
               sp = row + (png_size_t)row_width - 1;
               dp = row + (png_size_t)(row_width << 2) - 1;

               for (i = 0; i < row_width; i++)
               {
                  if ((int)(*sp) >= num_trans)
                     *dp-- = 0xff;

                  else
                     *dp-- = trans_alpha[*sp];

                  *dp-- = palette[*sp].blue;
                  *dp-- = palette[*sp].green;
                  *dp-- = palette[*sp].red;
                  sp--;
               }
               row_info->bit_depth = 8;
               row_info->pixel_depth = 32;
               row_info->rowbytes = row_width * 4;
               row_info->color_type = 6;
               row_info->channels = 4;
            }

            else
            {
               sp = row + (png_size_t)row_width - 1;
               dp = row + (png_size_t)(row_width * 3) - 1;

               for (i = 0; i < row_width; i++)
               {
                  *dp-- = palette[*sp].blue;
                  *dp-- = palette[*sp].green;
                  *dp-- = palette[*sp].red;
                  sp--;
               }

               row_info->bit_depth = 8;
               row_info->pixel_depth = 24;
               row_info->rowbytes = row_width * 3;
               row_info->color_type = 2;
               row_info->channels = 3;
            }
         }
      }
   }
}




static void
png_do_expand(png_row_infop row_info, png_bytep row,
    png_const_color_16p trans_color)
{
   int shift, value;
   png_bytep sp, dp;
   png_uint_32 i;
   png_uint_32 row_width=row_info->width;

   png_debug(1, "in png_do_expand");

   {
      if (row_info->color_type == PNG_COLOR_TYPE_GRAY)
      {
         unsigned int gray = trans_color != NULL ? trans_color->gray : 0;

         if (row_info->bit_depth < 8)
         {
            switch (row_info->bit_depth)
            {
               case 1:
               {
                  gray = (gray & 0x01) * 0xff;
                  sp = row + (png_size_t)((row_width - 1) >> 3);
                  dp = row + (png_size_t)row_width - 1;
                  shift = 7 - (int)((row_width + 7) & 0x07);
                  for (i = 0; i < row_width; i++)
                  {
                     if ((*sp >> shift) & 0x01)
                        *dp = 0xff;

                     else
                        *dp = 0;

                     if (shift == 7)
                     {
                        shift = 0;
                        sp--;
                     }

                     else
                        shift++;

                     dp--;
                  }
                  break;
               }

               case 2:
               {
                  gray = (gray & 0x03) * 0x55;
                  sp = row + (png_size_t)((row_width - 1) >> 2);
                  dp = row + (png_size_t)row_width - 1;
                  shift = (int)((3 - ((row_width + 3) & 0x03)) << 1);
                  for (i = 0; i < row_width; i++)
                  {
                     value = (*sp >> shift) & 0x03;
                     *dp = (png_byte)(value | (value << 2) | (value << 4) |
                        (value << 6));
                     if (shift == 6)
                     {
                        shift = 0;
                        sp--;
                     }

                     else
                        shift += 2;

                     dp--;
                  }
                  break;
               }

               case 4:
               {
                  gray = (gray & 0x0f) * 0x11;
                  sp = row + (png_size_t)((row_width - 1) >> 1);
                  dp = row + (png_size_t)row_width - 1;
                  shift = (int)((1 - ((row_width + 1) & 0x01)) << 2);
                  for (i = 0; i < row_width; i++)
                  {
                     value = (*sp >> shift) & 0x0f;
                     *dp = (png_byte)(value | (value << 4));
                     if (shift == 4)
                     {
                        shift = 0;
                        sp--;
                     }

                     else
                        shift = 4;

                     dp--;
                  }
                  break;
               }

               default:
                  break;
            }

            row_info->bit_depth = 8;
            row_info->pixel_depth = 8;
            row_info->rowbytes = row_width;
         }

         if (trans_color != NULL)
         {
            if (row_info->bit_depth == 8)
            {
               gray = gray & 0xff;
               sp = row + (png_size_t)row_width - 1;
               dp = row + (png_size_t)(row_width << 1) - 1;

               for (i = 0; i < row_width; i++)
               {
                  if (*sp == gray)
                     *dp-- = 0;

                  else
                     *dp-- = 0xff;

                  *dp-- = *sp--;
               }
            }

            else if (row_info->bit_depth == 16)
            {
               unsigned int gray_high = (gray >> 8) & 0xff;
               unsigned int gray_low = gray & 0xff;
               sp = row + row_info->rowbytes - 1;
               dp = row + (row_info->rowbytes << 1) - 1;
               for (i = 0; i < row_width; i++)
               {
                  if (*(sp - 1) == gray_high && *(sp) == gray_low)
                  {
                     *dp-- = 0;
                     *dp-- = 0;
                  }

                  else
                  {
                     *dp-- = 0xff;
                     *dp-- = 0xff;
                  }

                  *dp-- = *sp--;
                  *dp-- = *sp--;
               }
            }

            row_info->color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
            row_info->channels = 2;
            row_info->pixel_depth = (png_byte)(row_info->bit_depth << 1);
            row_info->rowbytes = PNG_ROWBYTES(row_info->pixel_depth,
               row_width);
         }
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_RGB &&
          trans_color != NULL)
      {
         if (row_info->bit_depth == 8)
         {
            png_byte red = (png_byte)(trans_color->red & 0xff);
            png_byte green = (png_byte)(trans_color->green & 0xff);
            png_byte blue = (png_byte)(trans_color->blue & 0xff);
            sp = row + (png_size_t)row_info->rowbytes - 1;
            dp = row + (png_size_t)(row_width << 2) - 1;
            for (i = 0; i < row_width; i++)
            {
               if (*(sp - 2) == red && *(sp - 1) == green && *(sp) == blue)
                  *dp-- = 0;

               else
                  *dp-- = 0xff;

               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
            }
         }
         else if (row_info->bit_depth == 16)
         {
            png_byte red_high = (png_byte)((trans_color->red >> 8) & 0xff);
            png_byte green_high = (png_byte)((trans_color->green >> 8) & 0xff);
            png_byte blue_high = (png_byte)((trans_color->blue >> 8) & 0xff);
            png_byte red_low = (png_byte)(trans_color->red & 0xff);
            png_byte green_low = (png_byte)(trans_color->green & 0xff);
            png_byte blue_low = (png_byte)(trans_color->blue & 0xff);
            sp = row + row_info->rowbytes - 1;
            dp = row + (png_size_t)(row_width << 3) - 1;
            for (i = 0; i < row_width; i++)
            {
               if (*(sp - 5) == red_high &&
                   *(sp - 4) == red_low &&
                   *(sp - 3) == green_high &&
                   *(sp - 2) == green_low &&
                   *(sp - 1) == blue_high &&
                   *(sp    ) == blue_low)
               {
                  *dp-- = 0;
                  *dp-- = 0;
               }

               else
               {
                  *dp-- = 0xff;
                  *dp-- = 0xff;
               }

               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
            }
         }
         row_info->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
         row_info->channels = 4;
         row_info->pixel_depth = (png_byte)(row_info->bit_depth << 2);
         row_info->rowbytes = PNG_ROWBYTES(row_info->pixel_depth, row_width);
      }
   }
}
#endif

#ifdef PNG_READ_EXPAND_16_SUPPORTED



static void
png_do_expand_16(png_row_infop row_info, png_bytep row)
{
   if (row_info->bit_depth == 8 &&
      row_info->color_type != PNG_COLOR_TYPE_PALETTE)
   {
      








      png_byte *sp = row + row_info->rowbytes; 
      png_byte *dp = sp + row_info->rowbytes;  
      while (dp > sp)
         dp[-2] = dp[-1] = *--sp, dp -= 2;

      row_info->rowbytes *= 2;
      row_info->bit_depth = 16;
      row_info->pixel_depth = (png_byte)(row_info->channels * 16);
   }
}
#endif

#ifdef PNG_READ_QUANTIZE_SUPPORTED
static void
png_do_quantize(png_row_infop row_info, png_bytep row,
    png_const_bytep palette_lookup, png_const_bytep quantize_lookup)
{
   png_bytep sp, dp;
   png_uint_32 i;
   png_uint_32 row_width=row_info->width;

   png_debug(1, "in png_do_quantize");

   if (row_info->bit_depth == 8)
   {
      if (row_info->color_type == PNG_COLOR_TYPE_RGB && palette_lookup)
      {
         int r, g, b, p;
         sp = row;
         dp = row;
         for (i = 0; i < row_width; i++)
         {
            r = *sp++;
            g = *sp++;
            b = *sp++;

            






            p = (((r >> (8 - PNG_QUANTIZE_RED_BITS)) &
                ((1 << PNG_QUANTIZE_RED_BITS) - 1)) <<
                (PNG_QUANTIZE_GREEN_BITS + PNG_QUANTIZE_BLUE_BITS)) |
                (((g >> (8 - PNG_QUANTIZE_GREEN_BITS)) &
                ((1 << PNG_QUANTIZE_GREEN_BITS) - 1)) <<
                (PNG_QUANTIZE_BLUE_BITS)) |
                ((b >> (8 - PNG_QUANTIZE_BLUE_BITS)) &
                ((1 << PNG_QUANTIZE_BLUE_BITS) - 1));

            *dp++ = palette_lookup[p];
         }

         row_info->color_type = PNG_COLOR_TYPE_PALETTE;
         row_info->channels = 1;
         row_info->pixel_depth = row_info->bit_depth;
         row_info->rowbytes = PNG_ROWBYTES(row_info->pixel_depth, row_width);
      }

      else if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA &&
         palette_lookup != NULL)
      {
         int r, g, b, p;
         sp = row;
         dp = row;
         for (i = 0; i < row_width; i++)
         {
            r = *sp++;
            g = *sp++;
            b = *sp++;
            sp++;

            p = (((r >> (8 - PNG_QUANTIZE_RED_BITS)) &
                ((1 << PNG_QUANTIZE_RED_BITS) - 1)) <<
                (PNG_QUANTIZE_GREEN_BITS + PNG_QUANTIZE_BLUE_BITS)) |
                (((g >> (8 - PNG_QUANTIZE_GREEN_BITS)) &
                ((1 << PNG_QUANTIZE_GREEN_BITS) - 1)) <<
                (PNG_QUANTIZE_BLUE_BITS)) |
                ((b >> (8 - PNG_QUANTIZE_BLUE_BITS)) &
                ((1 << PNG_QUANTIZE_BLUE_BITS) - 1));

            *dp++ = palette_lookup[p];
         }

         row_info->color_type = PNG_COLOR_TYPE_PALETTE;
         row_info->channels = 1;
         row_info->pixel_depth = row_info->bit_depth;
         row_info->rowbytes = PNG_ROWBYTES(row_info->pixel_depth, row_width);
      }

      else if (row_info->color_type == PNG_COLOR_TYPE_PALETTE &&
         quantize_lookup)
      {
         sp = row;

         for (i = 0; i < row_width; i++, sp++)
         {
            *sp = quantize_lookup[*sp];
         }
      }
   }
}
#endif 





void 
png_do_read_transformations(png_structrp png_ptr, png_row_infop row_info)
{
   png_debug(1, "in png_do_read_transformations");

   if (png_ptr->row_buf == NULL)
   {
      



      png_error(png_ptr, "NULL row buffer");
   }

   





   if ((png_ptr->flags & PNG_FLAG_DETECT_UNINITIALIZED) != 0 &&
       (png_ptr->flags & PNG_FLAG_ROW_INIT) == 0)
   {
      



      png_error(png_ptr, "Uninitialized row");
   }

#ifdef PNG_READ_EXPAND_SUPPORTED
   if ((png_ptr->transformations & PNG_EXPAND) != 0)
   {
      if (row_info->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         png_do_expand_palette(row_info, png_ptr->row_buf + 1,
             png_ptr->palette, png_ptr->trans_alpha, png_ptr->num_trans);
      }

      else
      {
         if (png_ptr->num_trans != 0 &&
             (png_ptr->transformations & PNG_EXPAND_tRNS) != 0)
            png_do_expand(row_info, png_ptr->row_buf + 1,
                &(png_ptr->trans_color));

         else
            png_do_expand(row_info, png_ptr->row_buf + 1,
                NULL);
      }
   }
#endif

#ifdef PNG_READ_STRIP_ALPHA_SUPPORTED
   if ((png_ptr->transformations & PNG_STRIP_ALPHA) != 0 &&
       (png_ptr->transformations & PNG_COMPOSE) == 0 &&
       (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
       row_info->color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
      png_do_strip_channel(row_info, png_ptr->row_buf + 1,
         0 );
#endif

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
   if ((png_ptr->transformations & PNG_RGB_TO_GRAY) != 0)
   {
      int rgb_error =
          png_do_rgb_to_gray(png_ptr, row_info,
              png_ptr->row_buf + 1);

      if (rgb_error != 0)
      {
         png_ptr->rgb_to_gray_status=1;
         if ((png_ptr->transformations & PNG_RGB_TO_GRAY) ==
             PNG_RGB_TO_GRAY_WARN)
            png_warning(png_ptr, "png_do_rgb_to_gray found nongray pixel");

         if ((png_ptr->transformations & PNG_RGB_TO_GRAY) ==
             PNG_RGB_TO_GRAY_ERR)
            png_error(png_ptr, "png_do_rgb_to_gray found nongray pixel");
      }
   }
#endif
































#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED
   


   if ((png_ptr->transformations & PNG_GRAY_TO_RGB) != 0 &&
       (png_ptr->mode & PNG_BACKGROUND_IS_GRAY) == 0)
      png_do_gray_to_rgb(row_info, png_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED) ||\
   defined(PNG_READ_ALPHA_MODE_SUPPORTED)
   if ((png_ptr->transformations & PNG_COMPOSE) != 0)
      png_do_compose(row_info, png_ptr->row_buf + 1, png_ptr);
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED
   if ((png_ptr->transformations & PNG_GAMMA) != 0 &&
#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
      
      (png_ptr->transformations & PNG_RGB_TO_GRAY) == 0 &&
#endif
#if defined(PNG_READ_BACKGROUND_SUPPORTED) ||\
   defined(PNG_READ_ALPHA_MODE_SUPPORTED)
      


       !((png_ptr->transformations & PNG_COMPOSE) != 0 &&
       ((png_ptr->num_trans != 0) ||
       (png_ptr->color_type & PNG_COLOR_MASK_ALPHA) != 0)) &&
#endif
      


       (png_ptr->color_type != PNG_COLOR_TYPE_PALETTE))
      png_do_gamma(row_info, png_ptr->row_buf + 1, png_ptr);
#endif

#ifdef PNG_READ_STRIP_ALPHA_SUPPORTED
   if ((png_ptr->transformations & PNG_STRIP_ALPHA) != 0 &&
       (png_ptr->transformations & PNG_COMPOSE) != 0 &&
       (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
       row_info->color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
      png_do_strip_channel(row_info, png_ptr->row_buf + 1,
          0 );
#endif

#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
   if ((png_ptr->transformations & PNG_ENCODE_ALPHA) != 0 &&
       (row_info->color_type & PNG_COLOR_MASK_ALPHA) != 0)
      png_do_encode_alpha(row_info, png_ptr->row_buf + 1, png_ptr);
#endif

#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
   if ((png_ptr->transformations & PNG_SCALE_16_TO_8) != 0)
      png_do_scale_16_to_8(row_info, png_ptr->row_buf + 1);
#endif

#ifdef PNG_READ_STRIP_16_TO_8_SUPPORTED
   



   if ((png_ptr->transformations & PNG_16_TO_8) != 0)
      png_do_chop(row_info, png_ptr->row_buf + 1);
#endif

#ifdef PNG_READ_QUANTIZE_SUPPORTED
   if ((png_ptr->transformations & PNG_QUANTIZE) != 0)
   {
      png_do_quantize(row_info, png_ptr->row_buf + 1,
          png_ptr->palette_lookup, png_ptr->quantize_index);

      if (row_info->rowbytes == 0)
         png_error(png_ptr, "png_do_quantize returned rowbytes=0");
   }
#endif 

#ifdef PNG_READ_EXPAND_16_SUPPORTED
   




   if ((png_ptr->transformations & PNG_EXPAND_16) != 0)
      png_do_expand_16(row_info, png_ptr->row_buf + 1);
#endif

#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED
   
   if ((png_ptr->transformations & PNG_GRAY_TO_RGB) != 0 &&
       (png_ptr->mode & PNG_BACKGROUND_IS_GRAY) != 0)
      png_do_gray_to_rgb(row_info, png_ptr->row_buf + 1);
#endif

#ifdef PNG_READ_INVERT_SUPPORTED
   if ((png_ptr->transformations & PNG_INVERT_MONO) != 0)
      png_do_invert(row_info, png_ptr->row_buf + 1);
#endif

#ifdef PNG_READ_INVERT_ALPHA_SUPPORTED
   if ((png_ptr->transformations & PNG_INVERT_ALPHA) != 0)
      png_do_read_invert_alpha(row_info, png_ptr->row_buf + 1);
#endif

#ifdef PNG_READ_SHIFT_SUPPORTED
   if ((png_ptr->transformations & PNG_SHIFT) != 0)
      png_do_unshift(row_info, png_ptr->row_buf + 1,
          &(png_ptr->shift));
#endif

#ifdef PNG_READ_PACK_SUPPORTED
   if ((png_ptr->transformations & PNG_PACK) != 0)
      png_do_unpack(row_info, png_ptr->row_buf + 1);
#endif

#ifdef PNG_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED
   
   if (row_info->color_type == PNG_COLOR_TYPE_PALETTE &&
       png_ptr->num_palette_max >= 0)
      png_do_check_palette_indexes(png_ptr, row_info);
#endif

#ifdef PNG_READ_BGR_SUPPORTED
   if ((png_ptr->transformations & PNG_BGR) != 0)
      png_do_bgr(row_info, png_ptr->row_buf + 1);
#endif

#ifdef PNG_READ_PACKSWAP_SUPPORTED
   if ((png_ptr->transformations & PNG_PACKSWAP) != 0)
      png_do_packswap(row_info, png_ptr->row_buf + 1);
#endif

#ifdef PNG_READ_FILLER_SUPPORTED
   if ((png_ptr->transformations & PNG_FILLER) != 0)
      png_do_read_filler(row_info, png_ptr->row_buf + 1,
          (png_uint_32)png_ptr->filler, png_ptr->flags);
#endif

#ifdef PNG_READ_SWAP_ALPHA_SUPPORTED
   if ((png_ptr->transformations & PNG_SWAP_ALPHA) != 0)
      png_do_read_swap_alpha(row_info, png_ptr->row_buf + 1);
#endif

#ifdef PNG_READ_16BIT_SUPPORTED
#ifdef PNG_READ_SWAP_SUPPORTED
   if ((png_ptr->transformations & PNG_SWAP_BYTES) != 0)
      png_do_swap(row_info, png_ptr->row_buf + 1);
#endif
#endif

#ifdef PNG_READ_USER_TRANSFORM_SUPPORTED
   if ((png_ptr->transformations & PNG_USER_TRANSFORM) != 0)
   {
      if (png_ptr->read_user_transform_fn != NULL)
         (*(png_ptr->read_user_transform_fn)) 
             (png_ptr,     
             row_info,     
                
                
                
                
                
                
             png_ptr->row_buf + 1);    
#ifdef PNG_USER_TRANSFORM_PTR_SUPPORTED
      if (png_ptr->user_transform_depth != 0)
         row_info->bit_depth = png_ptr->user_transform_depth;

      if (png_ptr->user_transform_channels != 0)
         row_info->channels = png_ptr->user_transform_channels;
#endif
      row_info->pixel_depth = (png_byte)(row_info->bit_depth *
          row_info->channels);

      row_info->rowbytes = PNG_ROWBYTES(row_info->pixel_depth, row_info->width);
   }
#endif
}

#endif 
#endif 

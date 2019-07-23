

















#ifndef __FTSMOOTH_H__
#define __FTSMOOTH_H__


#include <ft2build.h>
#include FT_RENDER_H


FT_BEGIN_HEADER


#ifndef FT_CONFIG_OPTION_NO_STD_RASTER
  FT_EXPORT_VAR( const FT_Renderer_Class )  ft_std_renderer_class;
#endif

#ifndef FT_CONFIG_OPTION_NO_SMOOTH_RASTER
  FT_EXPORT_VAR( const FT_Renderer_Class )  ft_smooth_renderer_class;

  FT_EXPORT_VAR( const FT_Renderer_Class )  ft_smooth_lcd_renderer_class;

  FT_EXPORT_VAR( const FT_Renderer_Class )  ft_smooth_lcd_v_renderer_class;
#endif



FT_END_HEADER

#endif 




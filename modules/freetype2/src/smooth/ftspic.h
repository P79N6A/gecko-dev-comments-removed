

















#ifndef __FTSPIC_H__
#define __FTSPIC_H__


FT_BEGIN_HEADER

#include FT_INTERNAL_PIC_H

#ifndef FT_CONFIG_OPTION_PIC
#define FT_GRAYS_RASTER_GET        ft_grays_raster

#else 

  typedef struct SmoothPIC_
  {
    int ref_count;
    FT_Raster_Funcs ft_grays_raster;
  } SmoothPIC;

#define GET_PIC(lib)               ((SmoothPIC*)((lib)->pic_container.smooth))
#define FT_GRAYS_RASTER_GET        (GET_PIC(library)->ft_grays_raster)

  
  void
  ft_smooth_renderer_class_pic_free( FT_Library  library );

  void
  ft_smooth_lcd_renderer_class_pic_free( FT_Library  library );

  void
  ft_smooth_lcdv_renderer_class_pic_free( FT_Library  library );

  FT_Error
  ft_smooth_renderer_class_pic_init( FT_Library  library );

  FT_Error
  ft_smooth_lcd_renderer_class_pic_init( FT_Library  library );

  FT_Error
  ft_smooth_lcdv_renderer_class_pic_init( FT_Library  library );

#endif 

 

FT_END_HEADER

#endif 




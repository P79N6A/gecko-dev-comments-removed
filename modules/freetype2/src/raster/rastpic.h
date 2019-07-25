

















#ifndef __RASTPIC_H__
#define __RASTPIC_H__


FT_BEGIN_HEADER

#include FT_INTERNAL_PIC_H

#ifndef FT_CONFIG_OPTION_PIC
#define FT_STANDARD_RASTER_GET     ft_standard_raster

#else 

  typedef struct RasterPIC_
  {
    int ref_count;
    FT_Raster_Funcs ft_standard_raster;
  } RasterPIC;

#define GET_PIC(lib)               ((RasterPIC*)((lib)->pic_container.raster))
#define FT_STANDARD_RASTER_GET     (GET_PIC(library)->ft_standard_raster)

  
  void
  ft_raster1_renderer_class_pic_free( FT_Library  library );

  void
  ft_raster5_renderer_class_pic_free( FT_Library  library );

  FT_Error
  ft_raster1_renderer_class_pic_init( FT_Library  library );

  FT_Error
  ft_raster5_renderer_class_pic_init( FT_Library  library );

#endif 

 

FT_END_HEADER

#endif 






















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

#endif 

 

FT_END_HEADER

#endif 




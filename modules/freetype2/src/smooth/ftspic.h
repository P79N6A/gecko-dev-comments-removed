

















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

#endif 

 

FT_END_HEADER

#endif 




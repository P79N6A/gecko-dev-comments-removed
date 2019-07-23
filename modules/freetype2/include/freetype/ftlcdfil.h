


















#ifndef __FT_LCD_FILTER_H__
#define __FT_LCD_FILTER_H__

#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER

  























  


































  typedef enum  FT_LcdFilter_
  {
    FT_LCD_FILTER_NONE    = 0,
    FT_LCD_FILTER_DEFAULT = 1,
    FT_LCD_FILTER_LIGHT   = 2,
    FT_LCD_FILTER_LEGACY  = 16,

    FT_LCD_FILTER_MAX   

  } FT_LcdFilter;


  





















































  FT_EXPORT( FT_Error )
  FT_Library_SetLcdFilter( FT_Library    library,
                           FT_LcdFilter  filter );

  


FT_END_HEADER

#endif 




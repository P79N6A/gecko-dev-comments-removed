

















#ifndef __FTGRAYS_H__
#define __FTGRAYS_H__

#ifdef __cplusplus
  extern "C" {
#endif


#ifdef _STANDALONE_
#include "ftimage.h"
#else
#include <ft2build.h>
#include FT_IMAGE_H
#endif


  
  
  
  
  
  
  
  
#ifndef FT_EXPORT_VAR
#define FT_EXPORT_VAR( x )  extern  x
#endif

  FT_EXPORT_VAR( const FT_Raster_Funcs )  ft_grays_raster;


#ifdef __cplusplus
  }
#endif

#endif






















#ifndef _FT_GASP_H_
#define _FT_GASP_H_

#include <ft2build.h>
#include FT_FREETYPE_H

  

















  

































#define FT_GASP_NO_TABLE               -1
#define FT_GASP_DO_GRIDFIT           0x01
#define FT_GASP_DO_GRAY              0x02
#define FT_GASP_SYMMETRIC_SMOOTHING  0x08
#define FT_GASP_SYMMETRIC_GRIDFIT    0x10


  



















  FT_EXPORT( FT_Int )
  FT_Get_Gasp( FT_Face  face,
               FT_UInt  ppem );



#endif 




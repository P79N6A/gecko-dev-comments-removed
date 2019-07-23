

















#ifndef __SVPOSTNM_H__
#define __SVPOSTNM_H__

#include FT_INTERNAL_SERVICE_H


FT_BEGIN_HEADER

  









#define FT_SERVICE_ID_POSTSCRIPT_FONT_NAME  "postscript-font-name"


  typedef const char*
  (*FT_PsName_GetFunc)( FT_Face  face );


  FT_DEFINE_SERVICE( PsFontName )
  {
    FT_PsName_GetFunc  get_ps_font_name;
  };

  


FT_END_HEADER


#endif 




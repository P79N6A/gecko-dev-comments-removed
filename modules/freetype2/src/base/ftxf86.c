

















#include <ft2build.h>
#include FT_XFREE86_H
#include FT_INTERNAL_OBJECTS_H
#include FT_SERVICE_XFREE86_NAME_H


  

  FT_EXPORT_DEF( const char* )
  FT_Get_X11_Font_Format( FT_Face  face )
  {
    const char*  result = NULL;


    if ( face )
      FT_FACE_FIND_SERVICE( face, result, XF86_NAME );

    return result;
  }




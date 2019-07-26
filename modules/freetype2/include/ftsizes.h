

















  
  
  
  
  
  
  


#ifndef __FTSIZES_H__
#define __FTSIZES_H__


#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef FREETYPE_H
#error "freetype.h of FreeType 1 has been loaded!"
#error "Please fix the directory search order for header files"
#error "so that freetype.h of FreeType 2 is found first."
#endif


FT_BEGIN_HEADER


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( FT_Error )
  FT_New_Size( FT_Face   face,
               FT_Size*  size );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( FT_Error )
  FT_Done_Size( FT_Size  size );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( FT_Error )
  FT_Activate_Size( FT_Size  size );

  


FT_END_HEADER

#endif 




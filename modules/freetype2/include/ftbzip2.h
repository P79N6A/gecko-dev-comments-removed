

















#ifndef __FTBZIP2_H__
#define __FTBZIP2_H__

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef FREETYPE_H
#error "freetype.h of FreeType 1 has been loaded!"
#error "Please fix the directory search order for header files"
#error "so that freetype.h of FreeType 2 is found first."
#endif


FT_BEGIN_HEADER

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


 






































  FT_EXPORT( FT_Error )
  FT_Stream_OpenBzip2( FT_Stream  stream,
                       FT_Stream  source );

  


FT_END_HEADER

#endif 




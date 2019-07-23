



















#ifndef __TTUNPAT_H__
#define __TTUNPAT_H__


#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef FREETYPE_H
#error "freetype.h of FreeType 1 has been loaded!"
#error "Please fix the directory search order for header files"
#error "so that freetype.h of FreeType 2 is found first."
#endif


FT_BEGIN_HEADER


 










#define FT_PARAM_TAG_UNPATENTED_HINTING  FT_MAKE_TAG( 'u', 'n', 'p', 'a' )

 

FT_END_HEADER


#endif 




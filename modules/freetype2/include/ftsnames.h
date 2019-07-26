




















#ifndef __FT_SFNT_NAMES_H__
#define __FT_SFNT_NAMES_H__


#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef FREETYPE_H
#error "freetype.h of FreeType 1 has been loaded!"
#error "Please fix the directory search order for header files"
#error "so that freetype.h of FreeType 2 is found first."
#endif


FT_BEGIN_HEADER


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_SfntName_
  {
    FT_UShort  platform_id;
    FT_UShort  encoding_id;
    FT_UShort  language_id;
    FT_UShort  name_id;

    FT_Byte*   string;      
    FT_UInt    string_len;  

  } FT_SfntName;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( FT_UInt )
  FT_Get_Sfnt_Name_Count( FT_Face  face );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( FT_Error )
  FT_Get_Sfnt_Name( FT_Face       face,
                    FT_UInt       idx,
                    FT_SfntName  *aname );


  











#define FT_PARAM_TAG_IGNORE_PREFERRED_FAMILY  FT_MAKE_TAG( 'i', 'g', 'p', 'f' )


  











#define FT_PARAM_TAG_IGNORE_PREFERRED_SUBFAMILY  FT_MAKE_TAG( 'i', 'g', 'p', 's' )

  


FT_END_HEADER

#endif 




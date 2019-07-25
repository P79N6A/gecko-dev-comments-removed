

















#ifndef __FTBASE_H__
#define __FTBASE_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H


FT_BEGIN_HEADER


  
  
  FT_LOCAL( FT_Error )
  open_face_PS_from_sfnt_stream( FT_Library     library,
                                 FT_Stream      stream,
                                 FT_Long        face_index,
                                 FT_Int         num_params,
                                 FT_Parameter  *params,
                                 FT_Face       *aface );


  
  
  FT_LOCAL( FT_Error )
  open_face_from_buffer( FT_Library   library,
                         FT_Byte*     base,
                         FT_ULong     size,
                         FT_Long      face_index,
                         const char*  driver_name,
                         FT_Face     *aface );


#if  defined( FT_CONFIG_OPTION_GUESSING_EMBEDDED_RFORK ) && \
    !defined( FT_MACINTOSH )
  
  
  
  
  
  FT_LOCAL( FT_Bool )
  ft_raccess_rule_by_darwin_vfs( FT_Library library, FT_UInt  rule_index );
#endif


FT_END_HEADER

#endif 




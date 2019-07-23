

















#ifndef __SFOBJS_H__
#define __SFOBJS_H__


#include <ft2build.h>
#include FT_INTERNAL_SFNT_H
#include FT_INTERNAL_OBJECTS_H


FT_BEGIN_HEADER


  FT_LOCAL( FT_Error )
  sfnt_init_face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params );

  FT_LOCAL( FT_Error )
  sfnt_load_face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params );

  FT_LOCAL( void )
  sfnt_done_face( TT_Face  face );


FT_END_HEADER

#endif 




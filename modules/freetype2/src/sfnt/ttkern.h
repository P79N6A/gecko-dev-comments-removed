


















#ifndef __TTKERN_H__
#define __TTKERN_H__


#include <ft2build.h>
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_TRUETYPE_TYPES_H


FT_BEGIN_HEADER


  FT_LOCAL( FT_Error  )
  tt_face_load_kern( TT_Face    face,
                     FT_Stream  stream );

  FT_LOCAL( void )
  tt_face_done_kern( TT_Face  face );

  FT_LOCAL( FT_Int )
  tt_face_get_kerning( TT_Face     face,
                       FT_UInt     left_glyph,
                       FT_UInt     right_glyph );

#define TT_FACE_HAS_KERNING( face )  ( (face)->kern_avail_bits != 0 )


FT_END_HEADER

#endif 




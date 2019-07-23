

















#ifndef __TTPLOAD_H__
#define __TTPLOAD_H__


#include <ft2build.h>
#include FT_INTERNAL_TRUETYPE_TYPES_H


FT_BEGIN_HEADER


  FT_LOCAL( FT_Error )
  tt_face_load_loca( TT_Face    face,
                     FT_Stream  stream );

  FT_LOCAL( FT_ULong )
  tt_face_get_location( TT_Face   face,
                        FT_UInt   gindex,
                        FT_UInt  *asize );

  FT_LOCAL( void )
  tt_face_done_loca( TT_Face  face );

  FT_LOCAL( FT_Error )
  tt_face_load_cvt( TT_Face    face,
                    FT_Stream  stream );

  FT_LOCAL( FT_Error )
  tt_face_load_fpgm( TT_Face    face,
                     FT_Stream  stream );


  FT_LOCAL( FT_Error )
  tt_face_load_prep( TT_Face    face,
                     FT_Stream  stream );


  FT_LOCAL( FT_Error )
  tt_face_load_hdmx( TT_Face    face,
                     FT_Stream  stream );


  FT_LOCAL( void )
  tt_face_free_hdmx( TT_Face  face );


  FT_LOCAL( FT_Byte* )
  tt_face_get_device_metrics( TT_Face    face,
                              FT_UInt    ppem,
                              FT_UInt    gindex );

FT_END_HEADER

#endif 




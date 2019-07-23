


















#ifndef __TTLOAD_H__
#define __TTLOAD_H__


#include <ft2build.h>
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_TRUETYPE_TYPES_H


FT_BEGIN_HEADER


  FT_LOCAL( TT_Table  )
  tt_face_lookup_table( TT_Face   face,
                        FT_ULong  tag );

  FT_LOCAL( FT_Error )
  tt_face_goto_table( TT_Face    face,
                      FT_ULong   tag,
                      FT_Stream  stream,
                      FT_ULong*  length );


  FT_LOCAL( FT_Error )
  tt_face_load_font_dir( TT_Face    face,
                         FT_Stream  stream );


  FT_LOCAL( FT_Error )
  tt_face_load_any( TT_Face    face,
                    FT_ULong   tag,
                    FT_Long    offset,
                    FT_Byte*   buffer,
                    FT_ULong*  length );


  FT_LOCAL( FT_Error )
  tt_face_load_head( TT_Face    face,
                     FT_Stream  stream );


  FT_LOCAL( FT_Error )
  tt_face_load_cmap( TT_Face    face,
                     FT_Stream  stream );


  FT_LOCAL( FT_Error )
  tt_face_load_maxp( TT_Face    face,
                     FT_Stream  stream );


  FT_LOCAL( FT_Error )
  tt_face_load_name( TT_Face    face,
                     FT_Stream  stream );


  FT_LOCAL( FT_Error )
  tt_face_load_os2( TT_Face    face,
                    FT_Stream  stream );


  FT_LOCAL( FT_Error )
  tt_face_load_post( TT_Face    face,
                     FT_Stream  stream );


  FT_LOCAL( FT_Error )
  tt_face_load_pclt( TT_Face    face,
                     FT_Stream  stream );

  FT_LOCAL( void )
  tt_face_free_name( TT_Face  face );


  FT_LOCAL( FT_Error )
  tt_face_load_gasp( TT_Face    face,
                     FT_Stream  stream );

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

  FT_LOCAL( FT_Error )
  tt_face_load_bhed( TT_Face    face,
                     FT_Stream  stream );

#endif 


FT_END_HEADER

#endif 




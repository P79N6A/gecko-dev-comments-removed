


















#ifndef __TTPOST_H__
#define __TTPOST_H__


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_INTERNAL_TRUETYPE_TYPES_H


FT_BEGIN_HEADER


  FT_LOCAL( FT_Error )
  tt_face_get_ps_name( TT_Face      face,
                       FT_UInt      idx,
                       FT_String**  PSname );

  FT_LOCAL( void )
  tt_face_free_ps_names( TT_Face  face );


FT_END_HEADER

#endif 




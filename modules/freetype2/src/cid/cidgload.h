

















#ifndef __CIDGLOAD_H__
#define __CIDGLOAD_H__


#include <ft2build.h>
#include "cidobjs.h"


FT_BEGIN_HEADER


#if 0

  
  FT_LOCAL( FT_Error )
  cid_face_compute_max_advance( CID_Face  face,
                                FT_Int*   max_advance );

#endif 

  FT_LOCAL( FT_Error )
  cid_slot_load_glyph( FT_GlyphSlot  glyph,         
                       FT_Size       size,          
                       FT_UInt       glyph_index,
                       FT_Int32      load_flags );


FT_END_HEADER

#endif 




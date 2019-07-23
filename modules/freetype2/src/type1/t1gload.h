

















#ifndef __T1GLOAD_H__
#define __T1GLOAD_H__


#include <ft2build.h>
#include "t1objs.h"


FT_BEGIN_HEADER


  FT_LOCAL( FT_Error )
  T1_Compute_Max_Advance( T1_Face  face,
                          FT_Pos*  max_advance );

  FT_LOCAL( FT_Error )
  T1_Load_Glyph( T1_GlyphSlot  glyph,
                 T1_Size       size,
                 FT_UInt       glyph_index,
                 FT_Int32      load_flags );


FT_END_HEADER

#endif 




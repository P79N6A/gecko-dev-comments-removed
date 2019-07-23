

















#ifndef __TTGLOAD_H__
#define __TTGLOAD_H__


#include <ft2build.h>
#include "ttobjs.h"

#ifdef TT_USE_BYTECODE_INTERPRETER
#include "ttinterp.h"
#endif


FT_BEGIN_HEADER


  FT_LOCAL( void )
  TT_Init_Glyph_Loading( TT_Face  face );

  FT_LOCAL( FT_Error )
  TT_Load_Glyph( TT_Size       size,
                 TT_GlyphSlot  glyph,
                 FT_UInt       glyph_index,
                 FT_Int32      load_flags );


FT_END_HEADER

#endif 




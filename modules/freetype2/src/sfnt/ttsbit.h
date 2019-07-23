

















#ifndef __TTSBIT_H__
#define __TTSBIT_H__


#include <ft2build.h>
#include "ttload.h"


FT_BEGIN_HEADER


  FT_LOCAL( FT_Error )
  tt_face_load_eblc( TT_Face    face,
                     FT_Stream  stream );

  FT_LOCAL( void )
  tt_face_free_eblc( TT_Face  face );


  FT_LOCAL( FT_Error )
  tt_face_set_sbit_strike( TT_Face          face,
                           FT_Size_Request  req,
                           FT_ULong*        astrike_index );

  FT_LOCAL( FT_Error )
  tt_face_load_strike_metrics( TT_Face           face,
                               FT_ULong          strike_index,
                               FT_Size_Metrics*  metrics );

#if defined FT_CONFIG_OPTION_OLD_INTERNALS
  FT_LOCAL( FT_Error )
  tt_find_sbit_image( TT_Face          face,
                      FT_UInt          glyph_index,
                      FT_ULong         strike_index,
                      TT_SBit_Range   *arange,
                      TT_SBit_Strike  *astrike,
                      FT_ULong        *aglyph_offset );

  FT_LOCAL( FT_Error )
  tt_load_sbit_metrics( FT_Stream        stream,
                        TT_SBit_Range    range,
                        TT_SBit_Metrics  metrics );

#endif 

  FT_LOCAL( FT_Error )
  tt_face_load_sbit_image( TT_Face              face,
                           FT_ULong             strike_index,
                           FT_UInt              glyph_index,
                           FT_UInt              load_flags,
                           FT_Stream            stream,
                           FT_Bitmap           *map,
                           TT_SBit_MetricsRec  *metrics );


FT_END_HEADER

#endif 




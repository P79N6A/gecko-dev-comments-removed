





































#ifndef __CF2FT_H__
#define __CF2FT_H__


#include "cf2types.h"


  
#define CF2_NDEBUG


#include FT_SYSTEM_H

#include "cf2glue.h"
#include "cffgload.h"    


FT_BEGIN_HEADER


  FT_LOCAL( FT_Error )
  cf2_decoder_parse_charstrings( CFF_Decoder*  decoder,
                                 FT_Byte*      charstring_base,
                                 FT_ULong      charstring_len );

  FT_LOCAL( CFF_SubFont )
  cf2_getSubfont( CFF_Decoder*  decoder );


  FT_LOCAL( CF2_Fixed )
  cf2_getPpemY( CFF_Decoder*  decoder );
  FT_LOCAL( CF2_Fixed )
  cf2_getStdVW( CFF_Decoder*  decoder );
  FT_LOCAL( CF2_Fixed )
  cf2_getStdHW( CFF_Decoder*  decoder );

  FT_LOCAL( void )
  cf2_getBlueMetrics( CFF_Decoder*  decoder,
                      CF2_Fixed*    blueScale,
                      CF2_Fixed*    blueShift,
                      CF2_Fixed*    blueFuzz );
  FT_LOCAL( void )
  cf2_getBlueValues( CFF_Decoder*  decoder,
                     size_t*       count,
                     FT_Pos*      *data );
  FT_LOCAL( void )
  cf2_getOtherBlues( CFF_Decoder*  decoder,
                     size_t*       count,
                     FT_Pos*      *data );
  FT_LOCAL( void )
  cf2_getFamilyBlues( CFF_Decoder*  decoder,
                      size_t*       count,
                      FT_Pos*      *data );
  FT_LOCAL( void )
  cf2_getFamilyOtherBlues( CFF_Decoder*  decoder,
                           size_t*       count,
                           FT_Pos*      *data );

  FT_LOCAL( CF2_Int )
  cf2_getLanguageGroup( CFF_Decoder*  decoder );

  FT_LOCAL( CF2_Int )
  cf2_initGlobalRegionBuffer( CFF_Decoder*  decoder,
                              CF2_UInt      idx,
                              CF2_Buffer    buf );
  FT_LOCAL( FT_Error )
  cf2_getSeacComponent( CFF_Decoder*  decoder,
                        CF2_UInt      code,
                        CF2_Buffer    buf );
  FT_LOCAL( void )
  cf2_freeSeacComponent( CFF_Decoder*  decoder,
                         CF2_Buffer    buf );
  FT_LOCAL( CF2_Int )
  cf2_initLocalRegionBuffer( CFF_Decoder*  decoder,
                             CF2_UInt      idx,
                             CF2_Buffer    buf );

  FT_LOCAL( CF2_Fixed )
  cf2_getDefaultWidthX( CFF_Decoder*  decoder );
  FT_LOCAL( CF2_Fixed )
  cf2_getNominalWidthX( CFF_Decoder*  decoder );


  




  typedef struct  CF2_OutlineRec_
  {
    CF2_OutlineCallbacksRec  root;        
    CFF_Decoder*             decoder;

  } CF2_OutlineRec, *CF2_Outline;


  FT_LOCAL( void )
  cf2_outline_reset( CF2_Outline  outline );
  FT_LOCAL( void )
  cf2_outline_close( CF2_Outline  outline );


FT_END_HEADER


#endif 




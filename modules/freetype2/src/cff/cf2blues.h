





































  




























#ifndef __CF2BLUES_H__
#define __CF2BLUES_H__


#include "cf2glue.h"


FT_BEGIN_HEADER


  






  enum
  {
    CF2_GhostBottom = 0x1,  
    CF2_GhostTop    = 0x2,  
    CF2_PairBottom  = 0x4,  
    CF2_PairTop     = 0x8,  
    CF2_Locked      = 0x10, 
                            
    CF2_Synthetic   = 0x20  
  };


  





  enum
  {
    CF2_ICF_Top    = cf2_intToFixed(  880 ),
    CF2_ICF_Bottom = cf2_intToFixed( -120 )
  };


  



#define CF2_MIN_COUNTER  cf2_floatToFixed( 0.5 )


  
  struct  CF2_HintRec_
  {
    CF2_UInt  flags;  
    size_t    index;  
                      
    CF2_Fixed  csCoord;
    CF2_Fixed  dsCoord;
    CF2_Fixed  scale;
  };


  typedef struct  CF2_BlueRec_
  {
    CF2_Fixed  csBottomEdge;
    CF2_Fixed  csTopEdge;
    CF2_Fixed  csFlatEdge; 
    CF2_Fixed  dsFlatEdge; 
                           
    FT_Bool  bottomZone;

  } CF2_BlueRec;


  
  enum
  {
    CF2_MAX_BLUES      = 7,
    CF2_MAX_OTHERBLUES = 5
  };


  typedef struct  CF2_BluesRec_
  {
    CF2_Fixed  scale;
    CF2_UInt   count;
    FT_Bool    suppressOvershoot;
    FT_Bool    doEmBoxHints;

    CF2_Fixed  blueScale;
    CF2_Fixed  blueShift;
    CF2_Fixed  blueFuzz;

    CF2_Fixed  boost;

    CF2_HintRec  emBoxTopEdge;
    CF2_HintRec  emBoxBottomEdge;

    CF2_BlueRec  zone[CF2_MAX_BLUES + CF2_MAX_OTHERBLUES];

  } CF2_BluesRec, *CF2_Blues;


  FT_LOCAL( void )
  cf2_blues_init( CF2_Blues  blues,
                  CF2_Font   font );
  FT_LOCAL( FT_Bool )
  cf2_blues_capture( const CF2_Blues  blues,
                     CF2_Hint         bottomHintEdge,
                     CF2_Hint         topHintEdge );


FT_END_HEADER


#endif 




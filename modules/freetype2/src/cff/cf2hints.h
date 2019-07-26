





































#ifndef __CF2HINTS_H__
#define __CF2HINTS_H__


FT_BEGIN_HEADER


  enum
  {
    CF2_MAX_HINTS = 96    
  };


  


















  typedef struct  CF2_HintMaskRec_
  {
    FT_Error*  error;

    FT_Bool  isValid;
    FT_Bool  isNew;

    size_t  bitCount;
    size_t  byteCount;

    FT_Byte  mask[( CF2_MAX_HINTS + 7 ) / 8];

  } CF2_HintMaskRec, *CF2_HintMask;


  typedef struct  CF2_StemHintRec_
  {
    FT_Bool  used;     

    CF2_Fixed  min;    
    CF2_Fixed  max;

    CF2_Fixed  minDS;  
    CF2_Fixed  maxDS;

  } CF2_StemHintRec, *CF2_StemHint;


  




















  
  enum
  {
    CF2_MAX_HINT_EDGES = CF2_MAX_HINTS * 2
  };


  typedef struct  CF2_HintMapRec_
  {
    CF2_Font  font;

    
    struct CF2_HintMapRec_*  initialHintMap;

    
    CF2_ArrStack  hintMoves;

    FT_Bool  isValid;
    FT_Bool  hinted;

    CF2_Fixed  scale;
    CF2_UInt   count;

    
    CF2_UInt  lastIndex;

    CF2_HintRec  edge[CF2_MAX_HINT_EDGES]; 

  } CF2_HintMapRec, *CF2_HintMap;


  FT_LOCAL( FT_Bool )
  cf2_hint_isValid( const CF2_Hint  hint );
  FT_LOCAL( FT_Bool )
  cf2_hint_isTop( const CF2_Hint  hint );
  FT_LOCAL( FT_Bool )
  cf2_hint_isBottom( const CF2_Hint  hint );
  FT_LOCAL( void )
  cf2_hint_lock( CF2_Hint  hint );


  FT_LOCAL( void )
  cf2_hintmap_init( CF2_HintMap   hintmap,
                    CF2_Font      font,
                    CF2_HintMap   initialMap,
                    CF2_ArrStack  hintMoves,
                    CF2_Fixed     scale );
  FT_LOCAL( void )
  cf2_hintmap_build( CF2_HintMap   hintmap,
                     CF2_ArrStack  hStemHintArray,
                     CF2_ArrStack  vStemHintArray,
                     CF2_HintMask  hintMask,
                     CF2_Fixed     hintOrigin,
                     FT_Bool       initialMap );


  





  typedef struct  CF2_GlyphPathRec_
  {
    

    CF2_Font              font;           
    CF2_OutlineCallbacks  callbacks;      


    CF2_HintMapRec  hintMap;        
    CF2_HintMapRec  firstHintMap;   
    CF2_HintMapRec  initialHintMap; 

    CF2_ArrStackRec  hintMoves;  

    CF2_Fixed  scaleX;         
    CF2_Fixed  scaleC;         
    CF2_Fixed  scaleY;         

    FT_Vector  fractionalTranslation;  
#if 0
    CF2_Fixed  hShift;    
                          
#endif

    FT_Bool  pathIsOpen;     
    FT_Bool  darken;         
    FT_Bool  moveIsPending;  

    
    CF2_ArrStack         hStemHintArray;
    CF2_ArrStack         vStemHintArray;
    CF2_HintMask         hintMask;     
    CF2_Fixed            hintOriginY;  
    const CF2_BluesRec*  blues;

    CF2_Fixed  xOffset;        
    CF2_Fixed  yOffset;

    
    CF2_Fixed  miterLimit;
    
    CF2_Fixed  snapThreshold;

    FT_Vector  offsetStart0;  
    FT_Vector  offsetStart1;  

    
    FT_Vector  currentCS;
    
    FT_Vector  currentDS;
    FT_Vector  start;         

    
    FT_Bool  elemIsQueued;
    CF2_Int  prevElemOp;

    FT_Vector  prevElemP0;
    FT_Vector  prevElemP1;
    FT_Vector  prevElemP2;
    FT_Vector  prevElemP3;

  } CF2_GlyphPathRec, *CF2_GlyphPath;


  FT_LOCAL( void )
  cf2_glyphpath_init( CF2_GlyphPath         glyphpath,
                      CF2_Font              font,
                      CF2_OutlineCallbacks  callbacks,
                      CF2_Fixed             scaleY,
                      
                      CF2_ArrStack          hStemHintArray,
                      CF2_ArrStack          vStemHintArray,
                      CF2_HintMask          hintMask,
                      CF2_Fixed             hintOrigin,
                      const CF2_Blues       blues,
                      const FT_Vector*      fractionalTranslation );
  FT_LOCAL( void )
  cf2_glyphpath_finalize( CF2_GlyphPath  glyphpath );

  FT_LOCAL( void )
  cf2_glyphpath_moveTo( CF2_GlyphPath  glyphpath,
                        CF2_Fixed      x,
                        CF2_Fixed      y );
  FT_LOCAL( void )
  cf2_glyphpath_lineTo( CF2_GlyphPath  glyphpath,
                        CF2_Fixed      x,
                        CF2_Fixed      y );
  FT_LOCAL( void )
  cf2_glyphpath_curveTo( CF2_GlyphPath  glyphpath,
                         CF2_Fixed      x1,
                         CF2_Fixed      y1,
                         CF2_Fixed      x2,
                         CF2_Fixed      y2,
                         CF2_Fixed      x3,
                         CF2_Fixed      y3 );
  FT_LOCAL( void )
  cf2_glyphpath_closeOpenPath( CF2_GlyphPath  glyphpath );


FT_END_HEADER


#endif




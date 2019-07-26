





































#include "cf2ft.h"
#include FT_INTERNAL_DEBUG_H

#include "cf2glue.h"
#include "cf2font.h"
#include "cf2hints.h"
#include "cf2intrp.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cf2hints


  typedef struct  CF2_HintMoveRec_
  {
    size_t     j;          
    CF2_Fixed  moveUp;     

  } CF2_HintMoveRec, *CF2_HintMove;


  
  
  static CF2_Int
  cf2_getWindingMomentum( CF2_Fixed  x1,
                          CF2_Fixed  y1,
                          CF2_Fixed  x2,
                          CF2_Fixed  y2 )
  {
    
    

    return ( x1 >> 16 ) * ( ( y2 - y1 ) >> 16 ) -
           ( y1 >> 16 ) * ( ( x2 - x1 ) >> 16 );
  }


  






  static void
  cf2_hint_init( CF2_Hint            hint,
                 const CF2_ArrStack  stemHintArray,
                 size_t              indexStemHint,
                 const CF2_Font      font,
                 CF2_Fixed           hintOrigin,
                 CF2_Fixed           scale,
                 FT_Bool             bottom )
  {
    CF2_Fixed               width;
    const CF2_StemHintRec*  stemHint;


    FT_ZERO( hint );

    stemHint = (const CF2_StemHintRec*)cf2_arrstack_getPointer(
                                         stemHintArray,
                                         indexStemHint );

    width = stemHint->max - stemHint->min;

    if ( width == cf2_intToFixed( -21 ) )
    {
      

      if ( bottom )
      {
        hint->csCoord = stemHint->max;
        hint->flags   = CF2_GhostBottom;
      }
      else
        hint->flags = 0;
    }

    else if ( width == cf2_intToFixed( -20 ) )
    {
      

      if ( bottom )
        hint->flags = 0;
      else
      {
        hint->csCoord = stemHint->min;
        hint->flags   = CF2_GhostTop;
      }
    }

    else if ( width < 0 )
    {
      

      















      if ( bottom )
      {
        hint->csCoord = stemHint->max;
        hint->flags   = CF2_PairBottom;
      }
      else
      {
        hint->csCoord = stemHint->min;
        hint->flags   = CF2_PairTop;
      }
    }

    else
    {
      

      if ( bottom )
      {
        hint->csCoord = stemHint->min;
        hint->flags   = CF2_PairBottom;
      }
      else
      {
        hint->csCoord = stemHint->max;
        hint->flags   = CF2_PairTop;
      }
    }

    
    
    
    if ( cf2_hint_isTop( hint ) )
      hint->csCoord += 2 * font->darkenY;

    hint->csCoord += hintOrigin;
    hint->scale    = scale;
    hint->index    = indexStemHint;   

    
    if ( hint->flags != 0 && stemHint->used )
    {
      if ( cf2_hint_isTop( hint ) )
        hint->dsCoord = stemHint->maxDS;
      else
        hint->dsCoord = stemHint->minDS;

      cf2_hint_lock( hint );
    }
    else
      hint->dsCoord = FT_MulFix( hint->csCoord, scale );
  }


  
  static void
  cf2_hint_initZero( CF2_Hint  hint )
  {
    FT_ZERO( hint );
  }


  FT_LOCAL_DEF( FT_Bool )
  cf2_hint_isValid( const CF2_Hint  hint )
  {
    return (FT_Bool)( hint->flags != 0 );
  }


  static FT_Bool
  cf2_hint_isPair( const CF2_Hint  hint )
  {
    return (FT_Bool)( ( hint->flags                      &
                        ( CF2_PairBottom | CF2_PairTop ) ) != 0 );
  }


  static FT_Bool
  cf2_hint_isPairTop( const CF2_Hint  hint )
  {
    return (FT_Bool)( ( hint->flags & CF2_PairTop ) != 0 );
  }


  FT_LOCAL_DEF( FT_Bool )
  cf2_hint_isTop( const CF2_Hint  hint )
  {
    return (FT_Bool)( ( hint->flags                    &
                        ( CF2_PairTop | CF2_GhostTop ) ) != 0 );
  }


  FT_LOCAL_DEF( FT_Bool )
  cf2_hint_isBottom( const CF2_Hint  hint )
  {
    return (FT_Bool)( ( hint->flags                          &
                        ( CF2_PairBottom | CF2_GhostBottom ) ) != 0 );
  }


  static FT_Bool
  cf2_hint_isLocked( const CF2_Hint  hint )
  {
    return (FT_Bool)( ( hint->flags & CF2_Locked ) != 0 );
  }


  static FT_Bool
  cf2_hint_isSynthetic( const CF2_Hint  hint )
  {
    return (FT_Bool)( ( hint->flags & CF2_Synthetic ) != 0 );
  }


  FT_LOCAL_DEF( void )
  cf2_hint_lock( CF2_Hint  hint )
  {
    hint->flags |= CF2_Locked;
  }


  FT_LOCAL_DEF( void )
  cf2_hintmap_init( CF2_HintMap   hintmap,
                    CF2_Font      font,
                    CF2_HintMap   initialMap,
                    CF2_ArrStack  hintMoves,
                    CF2_Fixed     scale )
  {
    FT_ZERO( hintmap );

    
    hintmap->hinted         = font->hinted;
    hintmap->scale          = scale;
    hintmap->font           = font;
    hintmap->initialHintMap = initialMap;
    
    hintmap->hintMoves      = hintMoves;
  }


  static FT_Bool
  cf2_hintmap_isValid( const CF2_HintMap  hintmap )
  {
    return hintmap->isValid;
  }


  
  static CF2_Fixed
  cf2_hintmap_map( CF2_HintMap  hintmap,
                   CF2_Fixed    csCoord )
  {
    FT_ASSERT( hintmap->isValid );  
    FT_ASSERT( hintmap->lastIndex < CF2_MAX_HINT_EDGES );

    if ( hintmap->count == 0 || ! hintmap->hinted )
    {
      
      return FT_MulFix( csCoord, hintmap->scale );
    }
    else
    {
      
      CF2_UInt  i = hintmap->lastIndex;


      
      while ( i < hintmap->count - 1                  &&
              csCoord >= hintmap->edge[i + 1].csCoord )
        i += 1;

      
      while ( i > 0 && csCoord < hintmap->edge[i].csCoord )
        i -= 1;

      hintmap->lastIndex = i;

      if ( i == 0 && csCoord < hintmap->edge[0].csCoord )
      {
        
        return FT_MulFix( csCoord - hintmap->edge[0].csCoord,
                          hintmap->scale ) +
                 hintmap->edge[0].dsCoord;
      }
      else
      {
        



        return FT_MulFix( csCoord - hintmap->edge[i].csCoord,
                          hintmap->edge[i].scale ) +
                 hintmap->edge[i].dsCoord;
      }
    }
  }


  













  static void
  cf2_hintmap_adjustHints( CF2_HintMap  hintmap )
  {
    size_t  i, j;


    cf2_arrstack_clear( hintmap->hintMoves );      

    







    for ( i = 0; i < hintmap->count; i++ )
    {
      FT_Bool  isPair = cf2_hint_isPair( &hintmap->edge[i] );


      
      j = isPair ? i + 1 : i;

      FT_ASSERT( j < hintmap->count );
      FT_ASSERT( cf2_hint_isValid( &hintmap->edge[i] ) );
      FT_ASSERT( cf2_hint_isValid( &hintmap->edge[j] ) );
      FT_ASSERT( cf2_hint_isLocked( &hintmap->edge[i] ) ==
                   cf2_hint_isLocked( &hintmap->edge[j] ) );

      if ( !cf2_hint_isLocked( &hintmap->edge[i] ) )
      {
        
        CF2_Fixed  fracDown = cf2_fixedFraction( hintmap->edge[i].dsCoord );
        CF2_Fixed  fracUp   = cf2_fixedFraction( hintmap->edge[j].dsCoord );

        
        CF2_Fixed  downMoveDown = 0 - fracDown;
        CF2_Fixed  upMoveDown   = 0 - fracUp;
        CF2_Fixed  downMoveUp   = fracDown == 0
                                    ? 0
                                    : cf2_intToFixed( 1 ) - fracDown;
        CF2_Fixed  upMoveUp     = fracUp == 0
                                    ? 0
                                    : cf2_intToFixed( 1 ) - fracUp;

        
        CF2_Fixed  moveUp   = FT_MIN( downMoveUp, upMoveUp );
        
        CF2_Fixed  moveDown = FT_MAX( downMoveDown, upMoveDown );

        
        CF2_Fixed  move;

        CF2_Fixed  downMinCounter = CF2_MIN_COUNTER;
        CF2_Fixed  upMinCounter   = CF2_MIN_COUNTER;
        FT_Bool    saveEdge       = FALSE;


        
        
        
#if 0
        if ( i == 0                                        ||
             cf2_hint_isSynthetic( &hintmap->edge[i - 1] ) )
          downMinCounter = 0;

        if ( j >= hintmap->count - 1                       ||
             cf2_hint_isSynthetic( &hintmap->edge[j + 1] ) )
          upMinCounter = 0;
#endif

        
        
        
        if ( j >= hintmap->count - 1                            ||
             hintmap->edge[j + 1].dsCoord >=
               hintmap->edge[j].dsCoord + moveUp + upMinCounter )
        {
          
          if ( i == 0                                                 ||
               hintmap->edge[i - 1].dsCoord <=
                 hintmap->edge[i].dsCoord + moveDown - downMinCounter )
          {
            
            move = ( -moveDown < moveUp ) ? moveDown : moveUp;  
          }
          else
            move = moveUp;
        }
        else
        {
          
          if ( i == 0                                                 ||
               hintmap->edge[i - 1].dsCoord <=
                 hintmap->edge[i].dsCoord + moveDown - downMinCounter )
          {
            move     = moveDown;
            
            saveEdge = (FT_Bool)( moveUp < -moveDown );
          }
          else
          {
            
            
            move     = 0;
            saveEdge = TRUE;
          }
        }

        
        
        
        
        if ( saveEdge                                    &&
             j < hintmap->count - 1                      &&
             !cf2_hint_isLocked( &hintmap->edge[j + 1] ) )
        {
          CF2_HintMoveRec  savedMove;


          savedMove.j      = j;
          
          savedMove.moveUp = moveUp - move;

          cf2_arrstack_push( hintmap->hintMoves, &savedMove );
        }

        
        hintmap->edge[i].dsCoord += move;
        if ( isPair )
          hintmap->edge[j].dsCoord += move;
      }

      
      FT_ASSERT( i == 0                                                   ||
                 hintmap->edge[i - 1].dsCoord <= hintmap->edge[i].dsCoord );
      FT_ASSERT( i < j                                                ||
                 hintmap->edge[i].dsCoord <= hintmap->edge[j].dsCoord );

      
      if ( i > 0 )
      {
        if ( hintmap->edge[i].csCoord != hintmap->edge[i - 1].csCoord )
          hintmap->edge[i - 1].scale =
            FT_DivFix(
              hintmap->edge[i].dsCoord - hintmap->edge[i - 1].dsCoord,
              hintmap->edge[i].csCoord - hintmap->edge[i - 1].csCoord );
      }

      if ( isPair )
      {
        if ( hintmap->edge[j].csCoord != hintmap->edge[j - 1].csCoord )
          hintmap->edge[j - 1].scale =
            FT_DivFix(
              hintmap->edge[j].dsCoord - hintmap->edge[j - 1].dsCoord,
              hintmap->edge[j].csCoord - hintmap->edge[j - 1].csCoord );

        i += 1;     
      }
    }

    
    
    for ( i = cf2_arrstack_size( hintmap->hintMoves ); i > 0; i-- )
    {
      CF2_HintMove  hintMove = (CF2_HintMove)
                      cf2_arrstack_getPointer( hintmap->hintMoves, i - 1 );


      j = hintMove->j;

      
      FT_ASSERT( j < hintmap->count - 1 );

      
      if ( hintmap->edge[j + 1].dsCoord >=
             hintmap->edge[j].dsCoord + hintMove->moveUp + CF2_MIN_COUNTER )
      {
        
        hintmap->edge[j].dsCoord += hintMove->moveUp;

        if ( cf2_hint_isPair( &hintmap->edge[j] ) )
        {
          FT_ASSERT( j > 0 );
          hintmap->edge[j - 1].dsCoord += hintMove->moveUp;
        }
      }
    }
  }


  
  static void
  cf2_hintmap_insertHint( CF2_HintMap  hintmap,
                          CF2_Hint     bottomHintEdge,
                          CF2_Hint     topHintEdge )
  {
    CF2_UInt  indexInsert;

    
    FT_Bool   isPair         = TRUE;
    CF2_Hint  firstHintEdge  = bottomHintEdge;
    CF2_Hint  secondHintEdge = topHintEdge;


    
    
    FT_ASSERT( cf2_hint_isValid( bottomHintEdge ) ||
               cf2_hint_isValid( topHintEdge )    );

    
    if ( !cf2_hint_isValid( bottomHintEdge ) )
    {
      
      firstHintEdge = topHintEdge;
      isPair        = FALSE;
    }
    else if ( !cf2_hint_isValid( topHintEdge ) )
    {
      
      isPair = FALSE;
    }

    
    FT_ASSERT( !isPair                                         ||
               topHintEdge->csCoord >= bottomHintEdge->csCoord );

    
    indexInsert = 0;
    for ( ; indexInsert < hintmap->count; indexInsert++ )
    {
      if ( hintmap->edge[indexInsert].csCoord > firstHintEdge->csCoord )
        break;
    }

    





    if ( indexInsert < hintmap->count )
    {
      
      
      if ( isPair                                                       &&
           hintmap->edge[indexInsert].csCoord < secondHintEdge->csCoord )
        return; 

      
      if ( cf2_hint_isPairTop( &hintmap->edge[indexInsert] ) )
        return; 
    }

    
    if ( cf2_hintmap_isValid( hintmap->initialHintMap ) &&
         !cf2_hint_isLocked( firstHintEdge )            )
    {
      if ( isPair )
      {
        
        
        CF2_Fixed  midpoint  = cf2_hintmap_map(
                                 hintmap->initialHintMap,
                                 ( secondHintEdge->csCoord +
                                   firstHintEdge->csCoord ) / 2 );
        CF2_Fixed  halfWidth = FT_MulFix(
                                 ( secondHintEdge->csCoord -
                                   firstHintEdge->csCoord ) / 2,
                                 hintmap->scale );


        firstHintEdge->dsCoord  = midpoint - halfWidth;
        secondHintEdge->dsCoord = midpoint + halfWidth;
      }
      else
        firstHintEdge->dsCoord = cf2_hintmap_map( hintmap->initialHintMap,
                                                  firstHintEdge->csCoord );
    }

    
    
    if ( indexInsert > 0 )
    {
      
      if ( firstHintEdge->dsCoord < hintmap->edge[indexInsert - 1].dsCoord )
        return;
    }

    if ( indexInsert < hintmap->count )
    {
      
      if ( isPair )
      {
        if ( secondHintEdge->dsCoord > hintmap->edge[indexInsert].dsCoord )
          return;
      }
      else
      {
        if ( firstHintEdge->dsCoord > hintmap->edge[indexInsert].dsCoord )
          return;
      }
    }

    
    {
      CF2_Int  iSrc = hintmap->count - 1;
      CF2_Int  iDst = isPair ? hintmap->count + 1 : hintmap->count;

      CF2_Int  count = hintmap->count - indexInsert;


      if ( iDst >= CF2_MAX_HINT_EDGES )
      {
        FT_TRACE4(( "cf2_hintmap_insertHint: too many hintmaps\n" ));
        return;
      }

      while ( count-- )
        hintmap->edge[iDst--] = hintmap->edge[iSrc--];

      
      hintmap->edge[indexInsert] = *firstHintEdge;         
      hintmap->count += 1;

      if ( isPair )
      {
        
        hintmap->edge[indexInsert + 1] = *secondHintEdge;  
        hintmap->count                += 1;
      }
    }

    return;
  }


  













  FT_LOCAL_DEF( void )
  cf2_hintmap_build( CF2_HintMap   hintmap,
                     CF2_ArrStack  hStemHintArray,
                     CF2_ArrStack  vStemHintArray,
                     CF2_HintMask  hintMask,
                     CF2_Fixed     hintOrigin,
                     FT_Bool       initialMap )
  {
    FT_Byte*  maskPtr;

    CF2_Font         font = hintmap->font;
    CF2_HintMaskRec  tempHintMask;

    size_t   bitCount, i;
    FT_Byte  maskByte;


    
    if ( !initialMap && !cf2_hintmap_isValid( hintmap->initialHintMap ) )
    {
      
      
      cf2_hintmask_init( &tempHintMask, hintMask->error );
      cf2_hintmap_build( hintmap->initialHintMap,
                         hStemHintArray,
                         vStemHintArray,
                         &tempHintMask,
                         hintOrigin,
                         TRUE );
    }

    if ( !cf2_hintmask_isValid( hintMask ) )
    {
      
      cf2_hintmask_setAll( hintMask,
                           cf2_arrstack_size( hStemHintArray ) +
                             cf2_arrstack_size( vStemHintArray ) );
    }

    
    hintmap->count     = 0;
    hintmap->lastIndex = 0;

    
    tempHintMask = *hintMask;
    maskPtr      = cf2_hintmask_getMaskPtr( &tempHintMask );

    
    
    bitCount = cf2_arrstack_size( hStemHintArray );

    
    if ( font->blues.doEmBoxHints )
    {
      CF2_HintRec  dummy;


      cf2_hint_initZero( &dummy );   

      
      cf2_hintmap_insertHint( hintmap,
                              &font->blues.emBoxBottomEdge,
                              &dummy );
      
      cf2_hintmap_insertHint( hintmap,
                              &dummy,
                              &font->blues.emBoxTopEdge );
    }

    
    
    for ( i = 0, maskByte = 0x80; i < bitCount; i++ )
    {
      if ( maskByte & *maskPtr )
      {
        
        CF2_HintRec  bottomHintEdge, topHintEdge;


        cf2_hint_init( &bottomHintEdge,
                       hStemHintArray,
                       i,
                       font,
                       hintOrigin,
                       hintmap->scale,
                       TRUE  );
        cf2_hint_init( &topHintEdge,
                       hStemHintArray,
                       i,
                       font,
                       hintOrigin,
                       hintmap->scale,
                       FALSE  );

        if ( cf2_hint_isLocked( &bottomHintEdge ) ||
             cf2_hint_isLocked( &topHintEdge )    ||
             cf2_blues_capture( &font->blues,
                                &bottomHintEdge,
                                &topHintEdge )   )
        {
          
          cf2_hintmap_insertHint( hintmap, &bottomHintEdge, &topHintEdge );

          *maskPtr &= ~maskByte;      
        }
      }

      if ( ( i & 7 ) == 7 )
      {
        
        maskPtr++;
        maskByte = 0x80;
      }
      else
        maskByte >>= 1;
    }

    

    



















    if ( initialMap )
    {
      
      
      

      if ( hintmap->count == 0                           ||
           hintmap->edge[0].csCoord > 0                  ||
           hintmap->edge[hintmap->count - 1].csCoord < 0 )
      {
        
        

        CF2_HintRec  edge, invalid;


        cf2_hint_initZero( &edge );

        edge.flags = CF2_GhostBottom |
                     CF2_Locked      |
                     CF2_Synthetic;
        edge.scale = hintmap->scale;

        cf2_hint_initZero( &invalid );
        cf2_hintmap_insertHint( hintmap, &edge, &invalid );
      }
    }
    else
    {
      

      maskPtr = cf2_hintmask_getMaskPtr( &tempHintMask );

      for ( i = 0, maskByte = 0x80; i < bitCount; i++ )
      {
        if ( maskByte & *maskPtr )
        {
          CF2_HintRec  bottomHintEdge, topHintEdge;


          cf2_hint_init( &bottomHintEdge,
                         hStemHintArray,
                         i,
                         font,
                         hintOrigin,
                         hintmap->scale,
                         TRUE  );
          cf2_hint_init( &topHintEdge,
                         hStemHintArray,
                         i,
                         font,
                         hintOrigin,
                         hintmap->scale,
                         FALSE  );

          cf2_hintmap_insertHint( hintmap, &bottomHintEdge, &topHintEdge );
        }

        if ( ( i & 7 ) == 7 )
        {
          
          maskPtr++;
          maskByte = 0x80;
        }
        else
          maskByte >>= 1;
      }
    }

    








    
    cf2_hintmap_adjustHints( hintmap );

    
    
    if ( !initialMap )
    {
      for ( i = 0; i < hintmap->count; i++ )
      {
        if ( !cf2_hint_isSynthetic( &hintmap->edge[i] ) )
        {
          
          
          CF2_StemHint  stemhint = (CF2_StemHint)
                          cf2_arrstack_getPointer( hStemHintArray,
                                                   hintmap->edge[i].index );


          if ( cf2_hint_isTop( &hintmap->edge[i] ) )
            stemhint->maxDS = hintmap->edge[i].dsCoord;
          else
            stemhint->minDS = hintmap->edge[i].dsCoord;

          stemhint->used = TRUE;
        }
      }
    }

    
    hintmap->isValid = TRUE;

    
    cf2_hintmask_setNew( hintMask, FALSE );
  }


  FT_LOCAL_DEF( void )
  cf2_glyphpath_init( CF2_GlyphPath         glyphpath,
                      CF2_Font              font,
                      CF2_OutlineCallbacks  callbacks,
                      CF2_Fixed             scaleY,
                      
                      CF2_ArrStack          hStemHintArray,
                      CF2_ArrStack          vStemHintArray,
                      CF2_HintMask          hintMask,
                      CF2_Fixed             hintOriginY,
                      const CF2_Blues       blues,
                      const FT_Vector*      fractionalTranslation )
  {
    FT_ZERO( glyphpath );

    glyphpath->font      = font;
    glyphpath->callbacks = callbacks;

    cf2_arrstack_init( &glyphpath->hintMoves,
                       font->memory,
                       &font->error,
                       sizeof ( CF2_HintMoveRec ) );

    cf2_hintmap_init( &glyphpath->initialHintMap,
                      font,
                      &glyphpath->initialHintMap,
                      &glyphpath->hintMoves,
                      scaleY );
    cf2_hintmap_init( &glyphpath->firstHintMap,
                      font,
                      &glyphpath->initialHintMap,
                      &glyphpath->hintMoves,
                      scaleY );
    cf2_hintmap_init( &glyphpath->hintMap,
                      font,
                      &glyphpath->initialHintMap,
                      &glyphpath->hintMoves,
                      scaleY );

    glyphpath->scaleX = font->innerTransform.a;
    glyphpath->scaleC = font->innerTransform.c;
    glyphpath->scaleY = font->innerTransform.d;

    glyphpath->fractionalTranslation = *fractionalTranslation;

#if 0
    glyphpath->hShift = hShift;       
#endif

    glyphpath->hStemHintArray = hStemHintArray;
    glyphpath->vStemHintArray = vStemHintArray;
    glyphpath->hintMask       = hintMask;      
    glyphpath->hintOriginY    = hintOriginY;
    glyphpath->blues          = blues;
    glyphpath->darken         = font->darkened; 
    glyphpath->xOffset        = font->darkenX;
    glyphpath->yOffset        = font->darkenY;
    glyphpath->miterLimit     = 2 * FT_MAX(
                                     cf2_fixedAbs( glyphpath->xOffset ),
                                     cf2_fixedAbs( glyphpath->yOffset ) );

    
    glyphpath->snapThreshold = cf2_floatToFixed( 0.1f );

    glyphpath->moveIsPending = TRUE;
    glyphpath->pathIsOpen    = FALSE;
    glyphpath->elemIsQueued  = FALSE;
  }


  FT_LOCAL_DEF( void )
  cf2_glyphpath_finalize( CF2_GlyphPath  glyphpath )
  {
    cf2_arrstack_finalize( &glyphpath->hintMoves );
  }


  





  static void
  cf2_glyphpath_hintPoint( CF2_GlyphPath  glyphpath,
                           CF2_HintMap    hintmap,
                           FT_Vector*     ppt,
                           CF2_Fixed      x,
                           CF2_Fixed      y )
  {
    FT_Vector  pt;   


    pt.x = FT_MulFix( glyphpath->scaleX, x ) +
             FT_MulFix( glyphpath->scaleC, y );
    pt.y = cf2_hintmap_map( hintmap, y );

    ppt->x = FT_MulFix( glyphpath->font->outerTransform.a, pt.x )   +
               FT_MulFix( glyphpath->font->outerTransform.c, pt.y ) +
               glyphpath->fractionalTranslation.x;
    ppt->y = FT_MulFix( glyphpath->font->outerTransform.b, pt.x )   +
               FT_MulFix( glyphpath->font->outerTransform.d, pt.y ) +
               glyphpath->fractionalTranslation.y;
  }


  






  static FT_Bool
  cf2_glyphpath_computeIntersection( CF2_GlyphPath     glyphpath,
                                     const FT_Vector*  u1,
                                     const FT_Vector*  u2,
                                     const FT_Vector*  v1,
                                     const FT_Vector*  v2,
                                     FT_Vector*        intersection )
  {
    



















#define cf2_perp( a, b )                                    \
          ( FT_MulFix( a.x, b.y ) - FT_MulFix( a.y, b.x ) )

  
#define CF2_CS_SCALE( x )         \
          ( ( (x) + 0x10 ) >> 5 )

    FT_Vector  u, v, w;      
    CF2_Fixed  denominator, s;


    u.x = CF2_CS_SCALE( u2->x - u1->x );
    u.y = CF2_CS_SCALE( u2->y - u1->y );
    v.x = CF2_CS_SCALE( v2->x - v1->x );
    v.y = CF2_CS_SCALE( v2->y - v1->y );
    w.x = CF2_CS_SCALE( v1->x - u1->x );
    w.y = CF2_CS_SCALE( v1->y - u1->y );

    denominator = cf2_perp( u, v );

    if ( denominator == 0 )
      return FALSE;           

    s = FT_DivFix( cf2_perp( w, v ), denominator );

    intersection->x = u1->x + FT_MulFix( s, u2->x - u1->x );
    intersection->y = u1->y + FT_MulFix( s, u2->y - u1->y );

    








    if ( u1->x == u2->x                                                     &&
         cf2_fixedAbs( intersection->x - u1->x ) < glyphpath->snapThreshold )
      intersection->x = u1->x;
    if ( u1->y == u2->y                                                     &&
         cf2_fixedAbs( intersection->y - u1->y ) < glyphpath->snapThreshold )
      intersection->y = u1->y;

    if ( v1->x == v2->x                                                     &&
         cf2_fixedAbs( intersection->x - v1->x ) < glyphpath->snapThreshold )
      intersection->x = v1->x;
    if ( v1->y == v2->y                                                     &&
         cf2_fixedAbs( intersection->y - v1->y ) < glyphpath->snapThreshold )
      intersection->y = v1->y;

    
    if ( cf2_fixedAbs( intersection->x - ( u2->x + v1->x ) / 2 ) >
           glyphpath->miterLimit                                   ||
         cf2_fixedAbs( intersection->y - ( u2->y + v1->y ) / 2 ) >
           glyphpath->miterLimit                                   )
      return FALSE;

    return TRUE;
  }


  


















  static void
  cf2_glyphpath_pushPrevElem( CF2_GlyphPath  glyphpath,
                              CF2_HintMap    hintmap,
                              FT_Vector*     nextP0,
                              FT_Vector      nextP1,
                              FT_Bool        close )
  {
    CF2_CallbackParamsRec  params;

    FT_Vector*  prevP0;
    FT_Vector*  prevP1;

    FT_Vector  intersection    = { 0, 0 };
    FT_Bool    useIntersection = FALSE;


    FT_ASSERT( glyphpath->prevElemOp == CF2_PathOpLineTo ||
               glyphpath->prevElemOp == CF2_PathOpCubeTo );

    if ( glyphpath->prevElemOp == CF2_PathOpLineTo )
    {
      prevP0 = &glyphpath->prevElemP0;
      prevP1 = &glyphpath->prevElemP1;
    }
    else
    {
      prevP0 = &glyphpath->prevElemP2;
      prevP1 = &glyphpath->prevElemP3;
    }

    
    
    
    if ( prevP1->x != nextP0->x || prevP1->y != nextP0->y )
    {
      
      
      useIntersection = cf2_glyphpath_computeIntersection( glyphpath,
                                                           prevP0,
                                                           prevP1,
                                                           nextP0,
                                                           &nextP1,
                                                           &intersection );
      if ( useIntersection )
      {
        
        
        *prevP1 = intersection;
      }
    }

    params.pt0 = glyphpath->currentDS;

    switch( glyphpath->prevElemOp )
    {
    case CF2_PathOpLineTo:
      params.op = CF2_PathOpLineTo;

      
      cf2_glyphpath_hintPoint( glyphpath,
                               hintmap,
                               &params.pt1,
                               glyphpath->prevElemP1.x,
                               glyphpath->prevElemP1.y );

      glyphpath->callbacks->lineTo( glyphpath->callbacks, &params );

      glyphpath->currentDS = params.pt1;

      break;

    case CF2_PathOpCubeTo:
      params.op = CF2_PathOpCubeTo;

      
      cf2_glyphpath_hintPoint( glyphpath,
                               hintmap,
                               &params.pt1,
                               glyphpath->prevElemP1.x,
                               glyphpath->prevElemP1.y );
      cf2_glyphpath_hintPoint( glyphpath,
                               hintmap,
                               &params.pt2,
                               glyphpath->prevElemP2.x,
                               glyphpath->prevElemP2.y );
      cf2_glyphpath_hintPoint( glyphpath,
                               hintmap,
                               &params.pt3,
                               glyphpath->prevElemP3.x,
                               glyphpath->prevElemP3.y );

      glyphpath->callbacks->cubeTo( glyphpath->callbacks, &params );

      glyphpath->currentDS = params.pt3;

      break;
    }

    if ( !useIntersection || close )
    {
      
      
      
      

      cf2_glyphpath_hintPoint( glyphpath,
                               hintmap,
                               &params.pt1,
                               nextP0->x,
                               nextP0->y );

      if ( params.pt1.x != glyphpath->currentDS.x ||
           params.pt1.y != glyphpath->currentDS.y )
      {
        
        params.op  = CF2_PathOpLineTo;
        params.pt0 = glyphpath->currentDS;

        
        glyphpath->callbacks->lineTo( glyphpath->callbacks, &params );

        glyphpath->currentDS = params.pt1;
      }
    }

    if ( useIntersection )
    {
      
      *nextP0 = intersection;
    }
  }


  
  
  static void
  cf2_glyphpath_pushMove( CF2_GlyphPath  glyphpath,
                          FT_Vector      start )
  {
    CF2_CallbackParamsRec  params;


    params.op  = CF2_PathOpMoveTo;
    params.pt0 = glyphpath->currentDS;

    
    
    if ( !cf2_hintmap_isValid( &glyphpath->hintMap ) )
    {
      
      
      cf2_glyphpath_moveTo( glyphpath,
                            glyphpath->start.x,
                            glyphpath->start.y );
    }

    cf2_glyphpath_hintPoint( glyphpath,
                             &glyphpath->hintMap,
                             &params.pt1,
                             start.x,
                             start.y );

    
    glyphpath->callbacks->moveTo( glyphpath->callbacks, &params );

    glyphpath->currentDS    = params.pt1;
    glyphpath->offsetStart0 = start;
  }


  









  static void
  cf2_glyphpath_computeOffset( CF2_GlyphPath  glyphpath,
                               CF2_Fixed      x1,
                               CF2_Fixed      y1,
                               CF2_Fixed      x2,
                               CF2_Fixed      y2,
                               CF2_Fixed*     x,
                               CF2_Fixed*     y )
  {
    CF2_Fixed  dx = x2 - x1;
    CF2_Fixed  dy = y2 - y1;


    
    
    if ( glyphpath->font->reverseWinding )
    {
      dx = -dx;
      dy = -dy;
    }

    *x = *y = 0;

    if ( !glyphpath->darken )
        return;

    
    glyphpath->callbacks->windingMomentum +=
      cf2_getWindingMomentum( x1, y1, x2, y2 );

    
    if ( dx >= 0 )
    {
      if ( dy >= 0 )
      {
        

        if ( dx > 2 * dy )
        {
          
          *x = 0;
          *y = 0;
        }
        else if ( dy > 2 * dx )
        {
          
          *x = glyphpath->xOffset;
          *y = glyphpath->yOffset;
        }
        else
        {
          
          *x = FT_MulFix( cf2_floatToFixed( 0.7 ),
                          glyphpath->xOffset );
          *y = FT_MulFix( cf2_floatToFixed( 1.0 - 0.7 ),
                          glyphpath->yOffset );
        }
      }
      else
      {
        

        if ( dx > -2 * dy )
        {
          
          *x = 0;
          *y = 0;
        }
        else if ( -dy > 2 * dx )
        {
          
          *x = -glyphpath->xOffset;
          *y = glyphpath->yOffset;
        }
        else
        {
          
          *x = FT_MulFix( cf2_floatToFixed( -0.7 ),
                          glyphpath->xOffset );
          *y = FT_MulFix( cf2_floatToFixed( 1.0 - 0.7 ),
                          glyphpath->yOffset );
        }
      }
    }
    else
    {
      if ( dy >= 0 )
      {
        

        if ( -dx > 2 * dy )
        {
          
          *x = 0;
          *y = 2 * glyphpath->yOffset;
        }
        else if ( dy > -2 * dx )
        {
          
          *x = glyphpath->xOffset;
          *y = glyphpath->yOffset;
        }
        else
        {
          
          *x = FT_MulFix( cf2_floatToFixed( 0.7 ),
                          glyphpath->xOffset );
          *y = FT_MulFix( cf2_floatToFixed( 1.0 + 0.7 ),
                          glyphpath->yOffset );
        }
      }
      else
      {
        

        if ( -dx > -2 * dy )
        {
          
          *x = 0;
          *y = 2 * glyphpath->yOffset;
        }
        else if ( -dy > -2 * dx )
        {
          
          *x = -glyphpath->xOffset;
          *y = glyphpath->xOffset;
        }
        else
        {
          
          *x = FT_MulFix( cf2_floatToFixed( -0.7 ),
                          glyphpath->xOffset );
          *y = FT_MulFix( cf2_floatToFixed( 1.0 + 0.7 ),
                          glyphpath->yOffset );
        }
      }
    }
  }


  FT_LOCAL_DEF( void )
  cf2_glyphpath_moveTo( CF2_GlyphPath  glyphpath,
                        CF2_Fixed      x,
                        CF2_Fixed      y )
  {
    cf2_glyphpath_closeOpenPath( glyphpath );

    
    
    
    glyphpath->currentCS.x = glyphpath->start.x = x;
    glyphpath->currentCS.y = glyphpath->start.y = y;

    glyphpath->moveIsPending = TRUE;

    
    if ( !cf2_hintmap_isValid( &glyphpath->hintMap ) ||
         cf2_hintmask_isNew( glyphpath->hintMask )   )
      cf2_hintmap_build( &glyphpath->hintMap,
                         glyphpath->hStemHintArray,
                         glyphpath->vStemHintArray,
                         glyphpath->hintMask,
                         glyphpath->hintOriginY,
                         FALSE );

    
    glyphpath->firstHintMap = glyphpath->hintMap;     
  }


  FT_LOCAL_DEF( void )
  cf2_glyphpath_lineTo( CF2_GlyphPath  glyphpath,
                        CF2_Fixed      x,
                        CF2_Fixed      y )
  {
    CF2_Fixed  xOffset, yOffset;
    FT_Vector  P0, P1;


    
    if ( glyphpath->currentCS.x == x && glyphpath->currentCS.y == y )
      return;

    cf2_glyphpath_computeOffset( glyphpath,
                                 glyphpath->currentCS.x,
                                 glyphpath->currentCS.y,
                                 x,
                                 y,
                                 &xOffset,
                                 &yOffset );

    
    P0.x = glyphpath->currentCS.x + xOffset;
    P0.y = glyphpath->currentCS.y + yOffset;
    P1.x = x + xOffset;
    P1.y = y + yOffset;

    if ( glyphpath->moveIsPending )
    {
      
      cf2_glyphpath_pushMove( glyphpath, P0 );

      glyphpath->moveIsPending = FALSE;  
      glyphpath->pathIsOpen    = TRUE;

      glyphpath->offsetStart1 = P1;              
    }

    if ( glyphpath->elemIsQueued )
    {
      FT_ASSERT( cf2_hintmap_isValid( &glyphpath->hintMap ) );

      cf2_glyphpath_pushPrevElem( glyphpath,
                                  &glyphpath->hintMap,
                                  &P0,
                                  P1,
                                  FALSE );
    }

    
    glyphpath->elemIsQueued = TRUE;
    glyphpath->prevElemOp   = CF2_PathOpLineTo;
    glyphpath->prevElemP0   = P0;
    glyphpath->prevElemP1   = P1;

    
    if ( cf2_hintmask_isNew( glyphpath->hintMask ) )
      cf2_hintmap_build( &glyphpath->hintMap,
                         glyphpath->hStemHintArray,
                         glyphpath->vStemHintArray,
                         glyphpath->hintMask,
                         glyphpath->hintOriginY,
                         FALSE );

    glyphpath->currentCS.x = x;     
    glyphpath->currentCS.y = y;
  }


  FT_LOCAL_DEF( void )
  cf2_glyphpath_curveTo( CF2_GlyphPath  glyphpath,
                         CF2_Fixed      x1,
                         CF2_Fixed      y1,
                         CF2_Fixed      x2,
                         CF2_Fixed      y2,
                         CF2_Fixed      x3,
                         CF2_Fixed      y3 )
  {
    CF2_Fixed  xOffset1, yOffset1, xOffset3, yOffset3;
    FT_Vector  P0, P1, P2, P3;


    
    cf2_glyphpath_computeOffset( glyphpath,
                                 glyphpath->currentCS.x,
                                 glyphpath->currentCS.y,
                                 x1,
                                 y1,
                                 &xOffset1,
                                 &yOffset1 );
    cf2_glyphpath_computeOffset( glyphpath,
                                 x2,
                                 y2,
                                 x3,
                                 y3,
                                 &xOffset3,
                                 &yOffset3 );

    
    glyphpath->callbacks->windingMomentum +=
      cf2_getWindingMomentum( x1, y1, x2, y2 );

    
    P0.x = glyphpath->currentCS.x + xOffset1;
    P0.y = glyphpath->currentCS.y + yOffset1;
    P1.x = x1 + xOffset1;
    P1.y = y1 + yOffset1;
    
    P2.x = x2 + xOffset3;
    P2.y = y2 + yOffset3;
    P3.x = x3 + xOffset3;
    P3.y = y3 + yOffset3;

    if ( glyphpath->moveIsPending )
    {
      
      cf2_glyphpath_pushMove( glyphpath, P0 );

      glyphpath->moveIsPending = FALSE;
      glyphpath->pathIsOpen    = TRUE;

      glyphpath->offsetStart1 = P1;              
    }

    if ( glyphpath->elemIsQueued )
    {
      FT_ASSERT( cf2_hintmap_isValid( &glyphpath->hintMap ) );

      cf2_glyphpath_pushPrevElem( glyphpath,
                                  &glyphpath->hintMap,
                                  &P0,
                                  P1,
                                  FALSE );
    }

    
    glyphpath->elemIsQueued = TRUE;
    glyphpath->prevElemOp   = CF2_PathOpCubeTo;
    glyphpath->prevElemP0   = P0;
    glyphpath->prevElemP1   = P1;
    glyphpath->prevElemP2   = P2;
    glyphpath->prevElemP3   = P3;

    
    if ( cf2_hintmask_isNew( glyphpath->hintMask ) )
      cf2_hintmap_build( &glyphpath->hintMap,
                         glyphpath->hStemHintArray,
                         glyphpath->vStemHintArray,
                         glyphpath->hintMask,
                         glyphpath->hintOriginY,
                         FALSE );

    glyphpath->currentCS.x = x3;       
    glyphpath->currentCS.y = y3;
  }


  FT_LOCAL_DEF( void )
  cf2_glyphpath_closeOpenPath( CF2_GlyphPath  glyphpath )
  {
    if ( glyphpath->pathIsOpen )
    {
      FT_ASSERT( cf2_hintmap_isValid( &glyphpath->firstHintMap ) );

      
      
      cf2_glyphpath_lineTo( glyphpath,
                            glyphpath->start.x,
                            glyphpath->start.y );

      
      
      
      
      FT_ASSERT( glyphpath->elemIsQueued );

      cf2_glyphpath_pushPrevElem( glyphpath,
                                  &glyphpath->firstHintMap,
                                  &glyphpath->offsetStart0,
                                  glyphpath->offsetStart1,
                                  TRUE );

      
      glyphpath->moveIsPending = TRUE;
      glyphpath->pathIsOpen    = FALSE;
      glyphpath->elemIsQueued  = FALSE;
    }
  }




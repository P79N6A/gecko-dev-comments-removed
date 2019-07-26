





































#include "cf2ft.h"
#include FT_INTERNAL_DEBUG_H

#include "cf2blues.h"
#include "cf2hints.h"
#include "cf2font.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cf2blues


  




#define cf2_blueToFixed( x )  cf2_intToFixed( x )


  FT_LOCAL_DEF( void )
  cf2_blues_init( CF2_Blues  blues,
                  CF2_Font   font )
  {
    
    CFF_Decoder*  decoder = font->decoder;

    CF2_Fixed  zoneHeight;
    CF2_Fixed  maxZoneHeight = 0;
    CF2_Fixed  csUnitsPerPixel;

    size_t  numBlueValues;
    size_t  numOtherBlues;
    size_t  numFamilyBlues;
    size_t  numFamilyOtherBlues;

    FT_Pos*  blueValues;
    FT_Pos*  otherBlues;
    FT_Pos*  familyBlues;
    FT_Pos*  familyOtherBlues;

    size_t     i;
    CF2_Fixed  emBoxBottom, emBoxTop;

    CF2_Int  unitsPerEm = font->unitsPerEm;


    if ( unitsPerEm == 0 )
      unitsPerEm = 1000;

    FT_ZERO( blues );
    blues->scale = font->innerTransform.d;

    cf2_getBlueMetrics( decoder,
                        &blues->blueScale,
                        &blues->blueShift,
                        &blues->blueFuzz );

    cf2_getBlueValues( decoder, &numBlueValues, &blueValues );
    cf2_getOtherBlues( decoder, &numOtherBlues, &otherBlues );
    cf2_getFamilyBlues( decoder, &numFamilyBlues, &familyBlues );
    cf2_getFamilyOtherBlues( decoder, &numFamilyOtherBlues, &familyOtherBlues );

    










    
    
#if 0
    FCM_getHorizontalLineMetrics( &e,
                                  font->font,
                                  &ascender,
                                  &descender,
                                  &linegap );
    if ( ascender - descender == unitsPerEm )
    {
      emBoxBottom = cf2_intToFixed( descender );
      emBoxTop    = cf2_intToFixed( ascender );
    }
    else
#endif
    {
      emBoxBottom = CF2_ICF_Bottom;
      emBoxTop    = CF2_ICF_Top;
    }

    if ( cf2_getLanguageGroup( decoder ) == 1                   &&
         ( numBlueValues == 0                                 ||
           ( numBlueValues == 4                             &&
             cf2_blueToFixed( blueValues[0] ) < emBoxBottom &&
             cf2_blueToFixed( blueValues[1] ) < emBoxBottom &&
             cf2_blueToFixed( blueValues[2] ) > emBoxTop    &&
             cf2_blueToFixed( blueValues[3] ) > emBoxTop    ) ) )
    {
      










      blues->emBoxBottomEdge.csCoord = emBoxBottom - CF2_FIXED_EPSILON;
      blues->emBoxBottomEdge.dsCoord = cf2_fixedRound(
                                         FT_MulFix(
                                           blues->emBoxBottomEdge.csCoord,
                                           blues->scale ) ) -
                                       CF2_MIN_COUNTER;
      blues->emBoxBottomEdge.scale   = blues->scale;
      blues->emBoxBottomEdge.flags   = CF2_GhostBottom |
                                       CF2_Locked |
                                       CF2_Synthetic;

      blues->emBoxTopEdge.csCoord = emBoxTop + CF2_FIXED_EPSILON +
                                    2 * font->darkenY;
      blues->emBoxTopEdge.dsCoord = cf2_fixedRound(
                                      FT_MulFix(
                                        blues->emBoxTopEdge.csCoord,
                                        blues->scale ) ) +
                                    CF2_MIN_COUNTER;
      blues->emBoxTopEdge.scale   = blues->scale;
      blues->emBoxTopEdge.flags   = CF2_GhostTop |
                                    CF2_Locked |
                                    CF2_Synthetic;

      blues->doEmBoxHints = TRUE;    

      return;
    }

    
    
    for ( i = 0; i < numBlueValues; i += 2 )
    {
      blues->zone[blues->count].csBottomEdge =
        cf2_blueToFixed( blueValues[i] );
      blues->zone[blues->count].csTopEdge =
        cf2_blueToFixed( blueValues[i + 1] );

      zoneHeight = blues->zone[blues->count].csTopEdge -
                   blues->zone[blues->count].csBottomEdge;

      if ( zoneHeight < 0 )
      {
        FT_TRACE4(( "cf2_blues_init: ignoring negative zone height\n" ));
        continue;   
      }

      if ( zoneHeight > maxZoneHeight )
      {
        
        
        maxZoneHeight = zoneHeight;
      }

      
      if ( i != 0 )
      {
        blues->zone[blues->count].csTopEdge    += 2 * font->darkenY;
        blues->zone[blues->count].csBottomEdge += 2 * font->darkenY;
      }

      
      if ( i == 0 )
      {
        blues->zone[blues->count].bottomZone =
          TRUE;
        blues->zone[blues->count].csFlatEdge =
          blues->zone[blues->count].csTopEdge;
      }
      else
      {
        blues->zone[blues->count].bottomZone =
          FALSE;
        blues->zone[blues->count].csFlatEdge =
          blues->zone[blues->count].csBottomEdge;
      }

      blues->count += 1;
    }

    for ( i = 0; i < numOtherBlues; i += 2 )
    {
      blues->zone[blues->count].csBottomEdge =
        cf2_blueToFixed( otherBlues[i] );
      blues->zone[blues->count].csTopEdge =
        cf2_blueToFixed( otherBlues[i + 1] );

      zoneHeight = blues->zone[blues->count].csTopEdge -
                   blues->zone[blues->count].csBottomEdge;

      if ( zoneHeight < 0 )
      {
        FT_TRACE4(( "cf2_blues_init: ignoring negative zone height\n" ));
        continue;   
      }

      if ( zoneHeight > maxZoneHeight )
      {
        
        
        maxZoneHeight = zoneHeight;
      }

      

      
      blues->zone[blues->count].bottomZone =
        TRUE;
      blues->zone[blues->count].csFlatEdge =
        blues->zone[blues->count].csTopEdge;

      blues->count += 1;
    }

    

    
    
    

    csUnitsPerPixel = FT_DivFix( cf2_intToFixed( 1 ), blues->scale );

    
    for ( i = 0; i < blues->count; i++ )
    {
      size_t     j;
      CF2_Fixed  minDiff;
      CF2_Fixed  flatFamilyEdge, diff;
      
      CF2_Fixed  flatEdge = blues->zone[i].csFlatEdge;


      if ( blues->zone[i].bottomZone )
      {
        
        
        

        minDiff = CF2_FIXED_MAX;

        for ( j = 0; j < numFamilyOtherBlues; j += 2 )
        {
          
          flatFamilyEdge = cf2_blueToFixed( familyOtherBlues[j + 1] );

          diff = cf2_fixedAbs( flatEdge - flatFamilyEdge );

          if ( diff < minDiff && diff < csUnitsPerPixel )
          {
            blues->zone[i].csFlatEdge = flatFamilyEdge;
            minDiff                   = diff;

            if ( diff == 0 )
              break;
          }
        }

        
        if ( numFamilyBlues >= 2 )
        {
          
          flatFamilyEdge = cf2_blueToFixed( familyBlues[1] );

          diff = cf2_fixedAbs( flatEdge - flatFamilyEdge );

          if ( diff < minDiff && diff < csUnitsPerPixel )
            blues->zone[i].csFlatEdge = flatFamilyEdge;
        }
      }
      else
      {
        
        
        
        

        minDiff = CF2_FIXED_MAX;

        for ( j = 2; j < numFamilyBlues; j += 2 )
        {
          
          flatFamilyEdge = cf2_blueToFixed( familyBlues[j] );

          
          flatFamilyEdge += 2 * font->darkenY;      

          diff = cf2_fixedAbs( flatEdge - flatFamilyEdge );

          if ( diff < minDiff && diff < csUnitsPerPixel )
          {
            blues->zone[i].csFlatEdge = flatFamilyEdge;
            minDiff                   = diff;

            if ( diff == 0 )
              break;
          }
        }
      }
    }

    

    
    

    if ( maxZoneHeight > 0 )
    {
      if ( blues->blueScale > FT_DivFix( cf2_intToFixed( 1 ),
                                         maxZoneHeight ) )
      {
        
        blues->blueScale = FT_DivFix( cf2_intToFixed( 1 ),
                                      maxZoneHeight );
      }

      









#if 0
      if ( blueScale < .4 / maxZoneHeight )
      {
        tetraphilia_assert( 0 );
        
        blueScale = .4 / maxZoneHeight;
      }
#endif

    }

    







    if ( blues->scale < blues->blueScale )
    {
      blues->suppressOvershoot = TRUE;

      
      
      

      blues->boost = FT_MulFix(
                       cf2_floatToFixed( .6 ),
                       ( cf2_intToFixed( 1 ) -
                         FT_DivFix( blues->scale,
                                    blues->blueScale ) ) );
      if ( blues->boost > 0x7FFF )
      {
        
        blues->boost = 0x7FFF;
      }
    }

    
    if ( font->stemDarkened )
      blues->boost = 0;

    
    

    for ( i = 0; i < blues->count; i++ )
    {
      if ( blues->zone[i].bottomZone )
        blues->zone[i].dsFlatEdge = cf2_fixedRound(
                                      FT_MulFix(
                                        blues->zone[i].csFlatEdge,
                                        blues->scale ) -
                                      blues->boost );
      else
        blues->zone[i].dsFlatEdge = cf2_fixedRound(
                                      FT_MulFix(
                                        blues->zone[i].csFlatEdge,
                                        blues->scale ) +
                                      blues->boost );
    }
  }


  
















  FT_LOCAL_DEF( FT_Bool )
  cf2_blues_capture( const CF2_Blues  blues,
                     CF2_Hint         bottomHintEdge,
                     CF2_Hint         topHintEdge )
  {
    
    CF2_Fixed  csFuzz = blues->blueFuzz;

    
    CF2_Fixed  dsNew;

    
    CF2_Fixed  dsMove = 0;

    FT_Bool   captured = FALSE;
    CF2_UInt  i;


    
    FT_ASSERT( !cf2_hint_isTop( bottomHintEdge ) &&
               !cf2_hint_isBottom( topHintEdge ) );

    
    for ( i = 0; i < blues->count; i++ )
    {
      if ( blues->zone[i].bottomZone           &&
           cf2_hint_isBottom( bottomHintEdge ) )
      {
        if ( ( blues->zone[i].csBottomEdge - csFuzz ) <=
               bottomHintEdge->csCoord                   &&
             bottomHintEdge->csCoord <=
               ( blues->zone[i].csTopEdge + csFuzz )     )
        {
          

          if ( blues->suppressOvershoot )
            dsNew = blues->zone[i].dsFlatEdge;

          else if ( ( blues->zone[i].csTopEdge - bottomHintEdge->csCoord ) >=
                      blues->blueShift )
          {
            
            dsNew = FT_MIN(
                      cf2_fixedRound( bottomHintEdge->dsCoord ),
                      blues->zone[i].dsFlatEdge - cf2_intToFixed( 1 ) );
          }

          else
          {
            
            dsNew = cf2_fixedRound( bottomHintEdge->dsCoord );
          }

          dsMove   = dsNew - bottomHintEdge->dsCoord;
          captured = TRUE;

          break;
        }
      }

      if ( !blues->zone[i].bottomZone && cf2_hint_isTop( topHintEdge ) )
      {
        if ( ( blues->zone[i].csBottomEdge - csFuzz ) <=
               topHintEdge->csCoord                      &&
             topHintEdge->csCoord <=
               ( blues->zone[i].csTopEdge + csFuzz )     )
        {
          

          if ( blues->suppressOvershoot )
            dsNew = blues->zone[i].dsFlatEdge;

          else if ( ( topHintEdge->csCoord - blues->zone[i].csBottomEdge ) >=
                      blues->blueShift )
          {
            
            dsNew = FT_MAX(
                      cf2_fixedRound( topHintEdge->dsCoord ),
                      blues->zone[i].dsFlatEdge + cf2_intToFixed( 1 ) );
          }

          else
          {
            
            dsNew = cf2_fixedRound( topHintEdge->dsCoord );
          }

          dsMove   = dsNew - topHintEdge->dsCoord;
          captured = TRUE;

          break;
        }
      }
    }

    if ( captured )
    {
      
      if ( cf2_hint_isValid( bottomHintEdge ) )
      {
        bottomHintEdge->dsCoord += dsMove;
        cf2_hint_lock( bottomHintEdge );
      }

      if ( cf2_hint_isValid( topHintEdge ) )
      {
        topHintEdge->dsCoord += dsMove;
        cf2_hint_lock( topHintEdge );
      }
    }

    return captured;
  }




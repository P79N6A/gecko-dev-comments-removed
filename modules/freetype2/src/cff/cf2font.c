





































#include "cf2ft.h"

#include "cf2glue.h"
#include "cf2font.h"
#include "cf2error.h"
#include "cf2intrp.h"


  
  static void
  cf2_computeDarkening( CF2_Fixed   emRatio,
                        CF2_Fixed   ppem,
                        CF2_Fixed   stemWidth,
                        CF2_Fixed*  darkenAmount,
                        CF2_Fixed   boldenAmount,
                        FT_Bool     stemDarkened )
  {
    
    
    CF2_Fixed  stemWidthPer1000, scaledStem;


    *darkenAmount = 0;

    if ( boldenAmount == 0 && !stemDarkened )
      return;

    
    if ( emRatio < cf2_floatToFixed( .01 ) )
      return;

    if ( stemDarkened )
    {
      
      

      
      

      stemWidthPer1000 = FT_MulFix( stemWidth + boldenAmount, emRatio );

      if ( emRatio > CF2_FIXED_ONE                          &&
           stemWidthPer1000 <= ( stemWidth + boldenAmount ) )
      {
        stemWidthPer1000 = 0;                      
        scaledStem       = cf2_intToFixed( 2333 );
      }
      else
      {
        scaledStem = FT_MulFix( stemWidthPer1000, ppem );

        if ( ppem > CF2_FIXED_ONE           &&
             scaledStem <= stemWidthPer1000 )
          scaledStem = cf2_intToFixed( 2333 );
      }

      














      if ( scaledStem < cf2_intToFixed( 500 ) )
        *darkenAmount = FT_DivFix( cf2_intToFixed( 400 ), ppem );

      else if ( scaledStem < cf2_intToFixed( 1000 ) )
        *darkenAmount = FT_DivFix( cf2_intToFixed( 525 ), ppem ) -
                          FT_MulFix( stemWidthPer1000,
                                     cf2_floatToFixed( .25 ) );

      else if ( scaledStem < cf2_intToFixed( 1667 ) )
        *darkenAmount = FT_DivFix( cf2_intToFixed( 275 ), ppem );

      else if ( scaledStem < cf2_intToFixed( 2333 ) )
        *darkenAmount = FT_DivFix( cf2_intToFixed( 963 ), ppem ) -
                          FT_MulFix( stemWidthPer1000,
                                     cf2_floatToFixed( .413 ) );

      
      
      *darkenAmount = FT_DivFix( *darkenAmount, 2 * emRatio );
    }

    
    *darkenAmount += boldenAmount / 2;
  }


  

  
  static void
  cf2_font_setup( CF2_Font           font,
                  const CF2_Matrix*  transform )
  {
    
    CFF_Decoder*  decoder = font->decoder;

    FT_Bool  needExtraSetup;

    
    CF2_Fixed  boldenX = font->syntheticEmboldeningAmountX;
    CF2_Fixed  boldenY = font->syntheticEmboldeningAmountY;

    CF2_Fixed  ppem;


    
    font->error = FT_Err_Ok;

    
    
    needExtraSetup =
      (FT_Bool)( font->lastSubfont != cf2_getSubfont( decoder ) );

    
    
    
    ppem = cf2_getPpemY( decoder );
    if ( font->ppem != ppem )
    {
      font->ppem     = ppem;
      needExtraSetup = TRUE;
    }

    
    font->hinted = (FT_Bool)( font->renderingFlags & CF2_FlagsHinted );

    
    
    if ( ft_memcmp( transform,
                    &font->currentTransform,
                    4 * sizeof ( CF2_Fixed ) ) != 0 )
    {
      
      
      font->currentTransform    = *transform;
      font->currentTransform.tx =
      font->currentTransform.ty = cf2_intToFixed( 0 );

      
      
      font->innerTransform   = *transform;
      font->outerTransform.a =
      font->outerTransform.d = cf2_intToFixed( 1 );
      font->outerTransform.b =
      font->outerTransform.c = cf2_intToFixed( 0 );

      needExtraSetup = TRUE;
    }

    






    if ( font->stemDarkened != ( font->renderingFlags & CF2_FlagsDarkened ) )
    {
      font->stemDarkened =
        (FT_Bool)( font->renderingFlags & CF2_FlagsDarkened );

      
      needExtraSetup = TRUE;
    }

    
    
    if ( needExtraSetup )
    {
      
      
      
      
      
      
      
      

      CF2_Fixed  emRatio;
      CF2_Fixed  stdHW;
      CF2_Int    unitsPerEm = font->unitsPerEm;


      if ( unitsPerEm == 0 )
        unitsPerEm = 1000;

      ppem = FT_MAX( cf2_intToFixed( 4 ),
                     font->ppem ); 

#if 0
      
      
      emRatio = cf2_fixedFracMul( cf2_intToFixed( 1000 ), fontMatrix->a );
#endif

      
      
      
      emRatio     = cf2_intToFixed( 1000 ) / unitsPerEm;
      font->stdVW = cf2_getStdVW( decoder );

      if ( font->stdVW <= 0 )
        font->stdVW = FT_DivFix( cf2_intToFixed( 75 ), emRatio );

      if ( boldenX > 0 )
      {
        
        
        boldenX = FT_MAX( boldenX,
                          FT_DivFix( cf2_intToFixed( unitsPerEm ), ppem ) );

        
        
        
        
        
        cf2_computeDarkening( emRatio,
                              ppem,
                              font->stdVW,
                              &font->darkenX,
                              boldenX,
                              FALSE );
      }
      else
        cf2_computeDarkening( emRatio,
                              ppem,
                              font->stdVW,
                              &font->darkenX,
                              0,
                              font->stemDarkened );

#if 0
      
      
      
      emRatio = cf2_fixedFracMul( cf2_intToFixed( 1000 ), fontMatrix->d );
#endif

      
      
      
      stdHW = cf2_getStdHW( decoder );

      if ( stdHW > 0 && font->stdVW > 2 * stdHW )
        font->stdHW = FT_DivFix( cf2_intToFixed( 75 ), emRatio );
      else
      {
        
        font->stdHW = FT_DivFix( cf2_intToFixed( 110 ), emRatio );
      }

      cf2_computeDarkening( emRatio,
                            ppem,
                            font->stdHW,
                            &font->darkenY,
                            boldenY,
                            font->stemDarkened );

      if ( font->darkenX != 0 || font->darkenY != 0 )
        font->darkened = TRUE;
      else
        font->darkened = FALSE;

      font->reverseWinding = FALSE; 

      
      cf2_blues_init( &font->blues, font );
    }
  }


  
  FT_LOCAL_DEF( FT_Error )
  cf2_getGlyphOutline( CF2_Font           font,
                       CF2_Buffer         charstring,
                       const CF2_Matrix*  transform,
                       CF2_F16Dot16*      glyphWidth )
  {
    FT_Error  lastError = FT_Err_Ok;

    FT_Vector  translation;

#if 0
    FT_Vector  advancePoint;
#endif

    CF2_Fixed  advWidth = 0;
    FT_Bool    needWinding;


    
    

    translation.x = transform->tx;
    translation.y = transform->ty;

    
    cf2_font_setup( font, transform );
    if ( font->error )
      goto exit;                      

    
    font->reverseWinding = FALSE;

    
    needWinding = font->darkened;

    while ( 1 )
    {
      
      cf2_outline_reset( &font->outline );

      
      cf2_interpT2CharString( font,
                              charstring,
                              (CF2_OutlineCallbacks)&font->outline,
                              &translation,
                              FALSE,
                              0,
                              0,
                              &advWidth );

      if ( font->error )
        goto exit;

      if ( !needWinding )
        break;

      
      if ( font->outline.root.windingMomentum >= 0 ) 
        break;

      
      
      font->reverseWinding = TRUE;

      needWinding = FALSE;    
    }

    
    cf2_outline_close( &font->outline );

  exit:
    
    *glyphWidth = advWidth;

    
    cf2_setError( &font->error, lastError );

    return font->error;
  }




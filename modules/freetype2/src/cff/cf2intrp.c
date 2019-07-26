





































#include "cf2ft.h"
#include FT_INTERNAL_DEBUG_H

#include "cf2glue.h"
#include "cf2font.h"
#include "cf2stack.h"
#include "cf2hints.h"

#include "cf2error.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cf2interp


  
#define CF2_FIXME  FT_TRACE4(( "cf2_interpT2CharString:"            \
                               " operator not implemented yet\n" ))



  FT_LOCAL_DEF( void )
  cf2_hintmask_init( CF2_HintMask  hintmask,
                     FT_Error*     error )
  {
    FT_ZERO( hintmask );

    hintmask->error = error;
  }


  FT_LOCAL_DEF( FT_Bool )
  cf2_hintmask_isValid( const CF2_HintMask  hintmask )
  {
    return hintmask->isValid;
  }


  FT_LOCAL_DEF( FT_Bool )
  cf2_hintmask_isNew( const CF2_HintMask  hintmask )
  {
    return hintmask->isNew;
  }


  FT_LOCAL_DEF( void )
  cf2_hintmask_setNew( CF2_HintMask  hintmask,
                       FT_Bool       val )
  {
    hintmask->isNew = val;
  }


  
  

  FT_LOCAL_DEF( FT_Byte* )
  cf2_hintmask_getMaskPtr( CF2_HintMask  hintmask )
  {
    return hintmask->mask;
  }


  static size_t
  cf2_hintmask_setCounts( CF2_HintMask  hintmask,
                          size_t        bitCount )
  {
    if ( bitCount > CF2_MAX_HINTS )
    {
      
      CF2_SET_ERROR( hintmask->error, Invalid_Glyph_Format );
      return 0;
    }

    hintmask->bitCount  = bitCount;
    hintmask->byteCount = ( hintmask->bitCount + 7 ) / 8;

    hintmask->isValid = TRUE;
    hintmask->isNew   = TRUE;

    return bitCount;
  }


  
  
  static void
  cf2_hintmask_read( CF2_HintMask  hintmask,
                     CF2_Buffer    charstring,
                     size_t        bitCount )
  {
    size_t  i;

#ifndef CF2_NDEBUG
    
    
    
    CF2_UInt  mask = ( 1 << ( -(CF2_Int)bitCount & 7 ) ) - 1;
#endif


    
    if ( cf2_hintmask_setCounts( hintmask, bitCount ) == 0 )
      return;

    FT_ASSERT( hintmask->byteCount > 0 );

    FT_TRACE4(( " (maskbytes:" ));

    
    for ( i = 0; i < hintmask->byteCount; i++ )
    {
      hintmask->mask[i] = (FT_Byte)cf2_buf_readByte( charstring );
      FT_TRACE4(( " 0x%02X", hintmask->mask[i] ));
    }

    FT_TRACE4(( ")\n" ));

    
    
    
#ifndef CF2_NDEBUG
    FT_ASSERT( ( hintmask->mask[hintmask->byteCount - 1] & mask ) == 0 ||
               *hintmask->error                                        );
#endif
  }


  FT_LOCAL_DEF( void )
  cf2_hintmask_setAll( CF2_HintMask  hintmask,
                       size_t        bitCount )
  {
    size_t    i;
    CF2_UInt  mask = ( 1 << ( -(CF2_Int)bitCount & 7 ) ) - 1;


    
    if ( cf2_hintmask_setCounts( hintmask, bitCount ) == 0 )
      return;

    FT_ASSERT( hintmask->byteCount > 0 );
    FT_ASSERT( hintmask->byteCount <
                 sizeof ( hintmask->mask ) / sizeof ( hintmask->mask[0] ) );

    
    for ( i = 0; i < hintmask->byteCount; i++ )
      hintmask->mask[i] = 0xFF;

    
    
    hintmask->mask[hintmask->byteCount - 1] &= ~mask;
  }


  
  enum
  {
    cf2_cmdRESERVED_0,   
    cf2_cmdHSTEM,        
    cf2_cmdRESERVED_2,   
    cf2_cmdVSTEM,        
    cf2_cmdVMOVETO,      
    cf2_cmdRLINETO,      
    cf2_cmdHLINETO,      
    cf2_cmdVLINETO,      
    cf2_cmdRRCURVETO,    
    cf2_cmdRESERVED_9,   
    cf2_cmdCALLSUBR,     
    cf2_cmdRETURN,       
    cf2_cmdESC,          
    cf2_cmdRESERVED_13,  
    cf2_cmdENDCHAR,      
    cf2_cmdRESERVED_15,  
    cf2_cmdRESERVED_16,  
    cf2_cmdRESERVED_17,  
    cf2_cmdHSTEMHM,      
    cf2_cmdHINTMASK,     
    cf2_cmdCNTRMASK,     
    cf2_cmdRMOVETO,      
    cf2_cmdHMOVETO,      
    cf2_cmdVSTEMHM,      
    cf2_cmdRCURVELINE,   
    cf2_cmdRLINECURVE,   
    cf2_cmdVVCURVETO,    
    cf2_cmdHHCURVETO,    
    cf2_cmdEXTENDEDNMBR, 
    cf2_cmdCALLGSUBR,    
    cf2_cmdVHCURVETO,    
    cf2_cmdHVCURVETO     
  };

  enum
  {
    cf2_escDOTSECTION,   
    cf2_escRESERVED_1,   
    cf2_escRESERVED_2,   
    cf2_escAND,          
    cf2_escOR,           
    cf2_escNOT,          
    cf2_escRESERVED_6,   
    cf2_escRESERVED_7,   
    cf2_escRESERVED_8,   
    cf2_escABS,          
    cf2_escADD,          
    cf2_escSUB,          
    cf2_escDIV,          
    cf2_escRESERVED_13,  
    cf2_escNEG,          
    cf2_escEQ,           
    cf2_escRESERVED_16,  
    cf2_escRESERVED_17,  
    cf2_escDROP,         
    cf2_escRESERVED_19,  
    cf2_escPUT,          
    cf2_escGET,          
    cf2_escIFELSE,       
    cf2_escRANDOM,       
    cf2_escMUL,          
    cf2_escRESERVED_25,  
    cf2_escSQRT,         
    cf2_escDUP,          
    cf2_escEXCH,         
    cf2_escINDEX,        
    cf2_escROLL,         
    cf2_escRESERVED_31,  
    cf2_escRESERVED_32,  
    cf2_escRESERVED_33,  
    cf2_escHFLEX,        
    cf2_escFLEX,         
    cf2_escHFLEX1,       
    cf2_escFLEX1         
  };


  
  static void
  cf2_doStems( const CF2_Font  font,
               CF2_Stack       opStack,
               CF2_ArrStack    stemHintArray,
               CF2_Fixed*      width,
               FT_Bool*        haveWidth,
               CF2_Fixed       hintOffset )
  {
    CF2_UInt  i;
    CF2_UInt  count       = cf2_stack_count( opStack );
    FT_Bool   hasWidthArg = (FT_Bool)( count & 1 );

    
    CF2_Fixed  position = hintOffset;

    if ( hasWidthArg && ! *haveWidth )
      *width = cf2_stack_getReal( opStack, 0 ) +
                 cf2_getNominalWidthX( font->decoder );

    if ( font->decoder->width_only )
      goto exit;

    for ( i = hasWidthArg ? 1 : 0; i < count; i += 2 )
    {
      
      CF2_StemHintRec  stemhint;


      stemhint.min  =
        position   += cf2_stack_getReal( opStack, i );
      stemhint.max  =
        position   += cf2_stack_getReal( opStack, i + 1 );

      stemhint.used  = FALSE;
      stemhint.maxDS =
      stemhint.minDS = 0;

      cf2_arrstack_push( stemHintArray, &stemhint ); 
    }

    cf2_stack_clear( opStack );

  exit:
    
    *haveWidth = TRUE;
  }


  static void
  cf2_doFlex( CF2_Stack       opStack,
              CF2_Fixed*      curX,
              CF2_Fixed*      curY,
              CF2_GlyphPath   glyphPath,
              const FT_Bool*  readFromStack,
              FT_Bool         doConditionalLastRead )
  {
    CF2_Fixed  vals[14];
    CF2_UInt   index;
    FT_Bool    isHFlex;
    CF2_Int    top, i, j;


    vals[0] = *curX;
    vals[1] = *curY;
    index   = 0;
    isHFlex = readFromStack[9] == FALSE;
    top     = isHFlex ? 9 : 10;

    for ( i = 0; i < top; i++ )
    {
      vals[i + 2] = vals[i];
      if ( readFromStack[i] )
        vals[i + 2] += cf2_stack_getReal( opStack, index++ );
    }

    if ( isHFlex )
      vals[9 + 2] = *curY;

    if ( doConditionalLastRead )
    {
      FT_Bool    lastIsX = (FT_Bool)( cf2_fixedAbs( vals[10] - *curX ) >
                                        cf2_fixedAbs( vals[11] - *curY ) );
      CF2_Fixed  lastVal = cf2_stack_getReal( opStack, index );


      if ( lastIsX )
      {
        vals[12] = vals[10] + lastVal;
        vals[13] = *curY;
      }
      else
      {
        vals[12] = *curX;
        vals[13] = vals[11] + lastVal;
      }
    }
    else
    {
      if ( readFromStack[10] )
        vals[12] = vals[10] + cf2_stack_getReal( opStack, index++ );
      else
        vals[12] = *curX;

      if ( readFromStack[11] )
        vals[13] = vals[11] + cf2_stack_getReal( opStack, index );
      else
        vals[13] = *curY;
    }

    for ( j = 0; j < 2; j++ )
      cf2_glyphpath_curveTo( glyphPath, vals[j * 6 + 2],
                                        vals[j * 6 + 3],
                                        vals[j * 6 + 4],
                                        vals[j * 6 + 5],
                                        vals[j * 6 + 6],
                                        vals[j * 6 + 7] );

    cf2_stack_clear( opStack );

    *curX = vals[12];
    *curY = vals[13];
  }


  















  FT_LOCAL_DEF( void )
  cf2_interpT2CharString( CF2_Font              font,
                          CF2_Buffer            buf,
                          CF2_OutlineCallbacks  callbacks,
                          const FT_Vector*      translation,
                          FT_Bool               doingSeac,
                          CF2_Fixed             curX,
                          CF2_Fixed             curY,
                          CF2_Fixed*            width )
  {
    
    FT_Error  lastError = FT_Err_Ok;

    
    CFF_Decoder*  decoder = font->decoder;

    FT_Error*  error  = &font->error;
    FT_Memory  memory = font->memory;

    CF2_Fixed  scaleY        = font->innerTransform.d;
    CF2_Fixed  nominalWidthX = cf2_getNominalWidthX( decoder );

    
    CF2_Fixed  hintOriginY = curY;

    CF2_Stack  opStack = NULL;
    FT_Byte    op1;                       

    
    FT_UInt32  instructionLimit = 20000000UL;

    CF2_ArrStackRec  subrStack;

    FT_Bool     haveWidth;
    CF2_Buffer  charstring = NULL;

    CF2_Int  charstringIndex = -1;       

    

    
    CF2_ArrStackRec  hStemHintArray;
    CF2_ArrStackRec  vStemHintArray;

    CF2_HintMaskRec   hintMask;
    CF2_GlyphPathRec  glyphPath;


    
    cf2_arrstack_init( &subrStack,
                       memory,
                       error,
                       sizeof ( CF2_BufferRec ) );
    cf2_arrstack_init( &hStemHintArray,
                       memory,
                       error,
                       sizeof ( CF2_StemHintRec ) );
    cf2_arrstack_init( &vStemHintArray,
                       memory,
                       error,
                       sizeof ( CF2_StemHintRec ) );

    
    cf2_hintmask_init( &hintMask, error );

    

    
    
    
    cf2_glyphpath_init( &glyphPath,
                        font,
                        callbacks,
                        scaleY,
                        
                        &hStemHintArray,
                        &vStemHintArray,
                        &hintMask,
                        hintOriginY,
                        &font->blues,
                        translation );

    















    haveWidth = FALSE;
    *width    = cf2_getDefaultWidthX( decoder );

    






    
    opStack = cf2_stack_init( memory, error );
    if ( !opStack )
    {
      lastError = FT_THROW( Out_Of_Memory );
      goto exit;
    }

    
    
    
    
    cf2_arrstack_setCount( &subrStack, CF2_MAX_SUBR + 1 );

    charstring  = (CF2_Buffer)cf2_arrstack_getBuffer( &subrStack );
    *charstring = *buf;    

    charstringIndex = 0;       

    
    if ( *error )
      goto exit;

    
    while ( 1 )
    {
      if ( cf2_buf_isEnd( charstring ) )
      {
        
        
        if ( charstringIndex )
          op1 = cf2_cmdRETURN;  
        else
          op1 = cf2_cmdENDCHAR; 
      }
      else
        op1 = (FT_Byte)cf2_buf_readByte( charstring );

      
      if ( *error )
        goto exit;

      instructionLimit--;
      if ( instructionLimit == 0 )
      {
        lastError = FT_THROW( Invalid_Glyph_Format );
        goto exit;
      }

      switch( op1 )
      {
      case cf2_cmdRESERVED_0:
      case cf2_cmdRESERVED_2:
      case cf2_cmdRESERVED_9:
      case cf2_cmdRESERVED_13:
      case cf2_cmdRESERVED_15:
      case cf2_cmdRESERVED_16:
      case cf2_cmdRESERVED_17:
        
        FT_TRACE4(( " unknown op (%d)\n", op1 ));
        break;

      case cf2_cmdHSTEMHM:
      case cf2_cmdHSTEM:
        FT_TRACE4(( op1 == cf2_cmdHSTEMHM ? " hstemhm\n" : " hstem\n" ));

        
        if ( cf2_hintmask_isValid( &hintMask ) )
          FT_TRACE4(( "cf2_interpT2CharString:"
                      " invalid horizontal hint mask\n" ));

        cf2_doStems( font,
                     opStack,
                     &hStemHintArray,
                     width,
                     &haveWidth,
                     0 );

        if ( font->decoder->width_only )
            goto exit;

        break;

      case cf2_cmdVSTEMHM:
      case cf2_cmdVSTEM:
        FT_TRACE4(( op1 == cf2_cmdVSTEMHM ? " vstemhm\n" : " vstem\n" ));

        
        if ( cf2_hintmask_isValid( &hintMask ) )
          FT_TRACE4(( "cf2_interpT2CharString:"
                      " invalid vertical hint mask\n" ));

        cf2_doStems( font,
                     opStack,
                     &vStemHintArray,
                     width,
                     &haveWidth,
                     0 );

        if ( font->decoder->width_only )
            goto exit;

        break;

      case cf2_cmdVMOVETO:
        FT_TRACE4(( " vmoveto\n" ));

        if ( cf2_stack_count( opStack ) > 1 && !haveWidth )
          *width = cf2_stack_getReal( opStack, 0 ) + nominalWidthX;

        
        haveWidth = TRUE;

        if ( font->decoder->width_only )
            goto exit;

        curY += cf2_stack_popFixed( opStack );

        cf2_glyphpath_moveTo( &glyphPath, curX, curY );

        break;

      case cf2_cmdRLINETO:
        {
          CF2_UInt  index;
          CF2_UInt  count = cf2_stack_count( opStack );


          FT_TRACE4(( " rlineto\n" ));

          for ( index = 0; index < count; index += 2 )
          {
            curX += cf2_stack_getReal( opStack, index + 0 );
            curY += cf2_stack_getReal( opStack, index + 1 );

            cf2_glyphpath_lineTo( &glyphPath, curX, curY );
          }

          cf2_stack_clear( opStack );
        }
        continue; 

      case cf2_cmdHLINETO:
      case cf2_cmdVLINETO:
        {
          CF2_UInt  index;
          CF2_UInt  count = cf2_stack_count( opStack );

          FT_Bool  isX = op1 == cf2_cmdHLINETO;


          FT_TRACE4(( isX ? " hlineto\n" : " vlineto\n" ));

          for ( index = 0; index < count; index++ )
          {
            CF2_Fixed  v = cf2_stack_getReal( opStack, index );


            if ( isX )
              curX += v;
            else
              curY += v;

            isX = !isX;

            cf2_glyphpath_lineTo( &glyphPath, curX, curY );
          }

          cf2_stack_clear( opStack );
        }
        continue;

      case cf2_cmdRCURVELINE:
      case cf2_cmdRRCURVETO:
        {
          CF2_UInt  count = cf2_stack_count( opStack );
          CF2_UInt  index = 0;


          FT_TRACE4(( op1 == cf2_cmdRCURVELINE ? " rcurveline\n"
                                               : " rrcurveto\n" ));

          while ( index + 6 <= count )
          {
            CF2_Fixed  x1 = cf2_stack_getReal( opStack, index + 0 ) + curX;
            CF2_Fixed  y1 = cf2_stack_getReal( opStack, index + 1 ) + curY;
            CF2_Fixed  x2 = cf2_stack_getReal( opStack, index + 2 ) + x1;
            CF2_Fixed  y2 = cf2_stack_getReal( opStack, index + 3 ) + y1;
            CF2_Fixed  x3 = cf2_stack_getReal( opStack, index + 4 ) + x2;
            CF2_Fixed  y3 = cf2_stack_getReal( opStack, index + 5 ) + y2;


            cf2_glyphpath_curveTo( &glyphPath, x1, y1, x2, y2, x3, y3 );

            curX   = x3;
            curY   = y3;
            index += 6;
          }

          if ( op1 == cf2_cmdRCURVELINE )
          {
            curX += cf2_stack_getReal( opStack, index + 0 );
            curY += cf2_stack_getReal( opStack, index + 1 );

            cf2_glyphpath_lineTo( &glyphPath, curX, curY );
          }

          cf2_stack_clear( opStack );
        }
        continue; 

      case cf2_cmdCALLGSUBR:
      case cf2_cmdCALLSUBR:
        {
          CF2_UInt  subrIndex;


          FT_TRACE4(( op1 == cf2_cmdCALLGSUBR ? " callgsubr"
                                              : " callsubr" ));

          if ( charstringIndex > CF2_MAX_SUBR )
          {
            
            lastError = FT_THROW( Invalid_Glyph_Format );
            goto exit;                      
          }

          
          charstring = (CF2_Buffer)
                         cf2_arrstack_getPointer( &subrStack,
                                                  charstringIndex + 1 );

          
          subrIndex = cf2_stack_popInt( opStack );

          switch ( op1 )
          {
          case cf2_cmdCALLGSUBR:
            FT_TRACE4(( "(%d)\n", subrIndex + decoder->globals_bias ));

            if ( cf2_initGlobalRegionBuffer( decoder,
                                             subrIndex,
                                             charstring ) )
            {
              lastError = FT_THROW( Invalid_Glyph_Format );
              goto exit;  
            }
            break;

          default:
            
            FT_TRACE4(( "(%d)\n", subrIndex + decoder->locals_bias ));

            if ( cf2_initLocalRegionBuffer( decoder,
                                            subrIndex,
                                            charstring ) )
            {
              lastError = FT_THROW( Invalid_Glyph_Format );
              goto exit;  
            }
          }

          charstringIndex += 1;       
        }
        continue; 

      case cf2_cmdRETURN:
        FT_TRACE4(( " return\n" ));

        if ( charstringIndex < 1 )
        {
          
          lastError = FT_THROW( Invalid_Glyph_Format );
          goto exit;                      
        }

        
        charstring = (CF2_Buffer)
                       cf2_arrstack_getPointer( &subrStack,
                                                --charstringIndex );
        continue;     

      case cf2_cmdESC:
        {
          FT_Byte  op2 = (FT_Byte)cf2_buf_readByte( charstring );


          switch ( op2 )
          {
          case cf2_escDOTSECTION:
            
            FT_TRACE4(( " dotsection\n" ));

            break;

          
          case cf2_escAND: 
            FT_TRACE4(( " and\n" ));

            CF2_FIXME;
            break;

          case cf2_escOR: 
            FT_TRACE4(( " or\n" ));

            CF2_FIXME;
            break;

          case cf2_escNOT: 
            FT_TRACE4(( " not\n" ));

            CF2_FIXME;
            break;

          case cf2_escABS: 
            FT_TRACE4(( " abs\n" ));

            CF2_FIXME;
            break;

          case cf2_escADD: 
            FT_TRACE4(( " add\n" ));

            CF2_FIXME;
            break;

          case cf2_escSUB: 
            FT_TRACE4(( " sub\n" ));

            CF2_FIXME;
            break;

          case cf2_escDIV: 
            FT_TRACE4(( " div\n" ));

            CF2_FIXME;
            break;

          case cf2_escNEG: 
            FT_TRACE4(( " neg\n" ));

            CF2_FIXME;
            break;

          case cf2_escEQ: 
            FT_TRACE4(( " eq\n" ));

            CF2_FIXME;
            break;

          case cf2_escDROP: 
            FT_TRACE4(( " drop\n" ));

            CF2_FIXME;
            break;

          case cf2_escPUT: 
            FT_TRACE4(( " put\n" ));

            CF2_FIXME;
            break;

          case cf2_escGET: 
            FT_TRACE4(( " get\n" ));

            CF2_FIXME;
            break;

          case cf2_escIFELSE: 
            FT_TRACE4(( " ifelse\n" ));

            CF2_FIXME;
            break;

          case cf2_escRANDOM: 
            FT_TRACE4(( " random\n" ));

            CF2_FIXME;
            break;

          case cf2_escMUL: 
            FT_TRACE4(( " mul\n" ));

            CF2_FIXME;
            break;

          case cf2_escSQRT: 
            FT_TRACE4(( " sqrt\n" ));

            CF2_FIXME;
            break;

          case cf2_escDUP: 
            FT_TRACE4(( " dup\n" ));

            CF2_FIXME;
            break;

          case cf2_escEXCH: 
            FT_TRACE4(( " exch\n" ));

            CF2_FIXME;
            break;

          case cf2_escINDEX: 
            FT_TRACE4(( " index\n" ));

            CF2_FIXME;
            break;

          case cf2_escROLL: 
            FT_TRACE4(( " roll\n" ));

            CF2_FIXME;
            break;

          case cf2_escHFLEX:
            {
              static const FT_Bool  readFromStack[12] =
              {
                TRUE , FALSE ,
                TRUE , TRUE  ,
                TRUE , FALSE ,
                TRUE , FALSE ,
                TRUE , FALSE ,
                TRUE , FALSE 
              };


              FT_TRACE4(( " hflex\n" ));

              cf2_doFlex( opStack,
                          &curX,
                          &curY,
                          &glyphPath,
                          readFromStack,
                          FALSE  );
            }
            continue;

          case cf2_escFLEX:
            {
              static const FT_Bool  readFromStack[12] =
              {
                TRUE , TRUE ,
                TRUE , TRUE ,
                TRUE , TRUE ,
                TRUE , TRUE ,
                TRUE , TRUE ,
                TRUE , TRUE 
              };


              FT_TRACE4(( " flex\n" ));

              cf2_doFlex( opStack,
                          &curX,
                          &curY,
                          &glyphPath,
                          readFromStack,
                          FALSE  );
            }
            break;      

          case cf2_escHFLEX1:
            {
              static const FT_Bool  readFromStack[12] =
              {
                TRUE , TRUE  ,
                TRUE , TRUE  ,
                TRUE , FALSE ,
                TRUE , FALSE ,
                TRUE , TRUE  ,
                TRUE , FALSE 
              };


              FT_TRACE4(( " hflex1\n" ));

              cf2_doFlex( opStack,
                          &curX,
                          &curY,
                          &glyphPath,
                          readFromStack,
                          FALSE  );
            }
            continue;

          case cf2_escFLEX1:
            {
              static const FT_Bool  readFromStack[12] =
              {
                TRUE  , TRUE  ,
                TRUE  , TRUE  ,
                TRUE  , TRUE  ,
                TRUE  , TRUE  ,
                TRUE  , TRUE  ,
                FALSE , FALSE 
              };


              FT_TRACE4(( " flex1\n" ));

              cf2_doFlex( opStack,
                          &curX,
                          &curY,
                          &glyphPath,
                          readFromStack,
                          TRUE  );
            }
            continue;

          case cf2_escRESERVED_1:
          case cf2_escRESERVED_2:
          case cf2_escRESERVED_6:
          case cf2_escRESERVED_7:
          case cf2_escRESERVED_8:
          case cf2_escRESERVED_13:
          case cf2_escRESERVED_16:
          case cf2_escRESERVED_17:
          case cf2_escRESERVED_19:
          case cf2_escRESERVED_25:
          case cf2_escRESERVED_31:
          case cf2_escRESERVED_32:
          case cf2_escRESERVED_33:
          default:
            FT_TRACE4(( " unknown op (12, %d)\n", op2 ));

          }; 

        } 
        break;

      case cf2_cmdENDCHAR:
        FT_TRACE4(( " endchar\n" ));

        if ( cf2_stack_count( opStack ) == 1 ||
             cf2_stack_count( opStack ) == 5 )
        {
          if ( !haveWidth )
            *width = cf2_stack_getReal( opStack, 0 ) + nominalWidthX;
        }

        
        haveWidth = TRUE;

        if ( font->decoder->width_only )
            goto exit;

        
        cf2_glyphpath_closeOpenPath( &glyphPath );

        if ( cf2_stack_count( opStack ) > 1 )
        {
          
          

          CF2_UInt       achar;
          CF2_UInt       bchar;
          CF2_BufferRec  component;
          CF2_Fixed      dummyWidth;   
          FT_Error       error2;


          if ( doingSeac )
          {
            lastError = FT_THROW( Invalid_Glyph_Format );
            goto exit;      
          }

          achar = cf2_stack_popInt( opStack );
          bchar = cf2_stack_popInt( opStack );

          curY = cf2_stack_popFixed( opStack );
          curX = cf2_stack_popFixed( opStack );

          error2 = cf2_getSeacComponent( decoder, achar, &component );
          if ( error2 )
          {
             lastError = error2;      
             goto exit;
          }
          cf2_interpT2CharString( font,
                                  &component,
                                  callbacks,
                                  translation,
                                  TRUE,
                                  curX,
                                  curY,
                                  &dummyWidth );
          cf2_freeSeacComponent( decoder, &component );

          error2 = cf2_getSeacComponent( decoder, bchar, &component );
          if ( error2 )
          {
            lastError = error2;      
            goto exit;
          }
          cf2_interpT2CharString( font,
                                  &component,
                                  callbacks,
                                  translation,
                                  TRUE,
                                  0,
                                  0,
                                  &dummyWidth );
          cf2_freeSeacComponent( decoder, &component );
        }
        goto exit;

      case cf2_cmdCNTRMASK:
      case cf2_cmdHINTMASK:
        
        
        FT_TRACE4(( op1 == cf2_cmdCNTRMASK ? " cntrmask" : " hintmask" ));

        
        
        if ( cf2_stack_count( opStack ) != 0 )
        {
          
          if ( cf2_hintmask_isValid( &hintMask ) )
            FT_TRACE4(( "cf2_interpT2CharString: invalid hint mask\n" ));
        }

        cf2_doStems( font,
                     opStack,
                     &vStemHintArray,
                     width,
                     &haveWidth,
                     0 );

        if ( font->decoder->width_only )
            goto exit;

        if ( op1 == cf2_cmdHINTMASK )
        {
          
          cf2_hintmask_read( &hintMask,
                             charstring,
                             cf2_arrstack_size( &hStemHintArray ) +
                               cf2_arrstack_size( &vStemHintArray ) );
        }
        else
        {
          












          CF2_HintMapRec   counterHintMap;
          CF2_HintMaskRec  counterMask;


          cf2_hintmap_init( &counterHintMap,
                            font,
                            &glyphPath.initialHintMap,
                            &glyphPath.hintMoves,
                            scaleY );
          cf2_hintmask_init( &counterMask, error );

          cf2_hintmask_read( &counterMask,
                             charstring,
                             cf2_arrstack_size( &hStemHintArray ) +
                               cf2_arrstack_size( &vStemHintArray ) );
          cf2_hintmap_build( &counterHintMap,
                             &hStemHintArray,
                             &vStemHintArray,
                             &counterMask,
                             0,
                             FALSE );
        }
        break;

      case cf2_cmdRMOVETO:
        FT_TRACE4(( " rmoveto\n" ));

        if ( cf2_stack_count( opStack ) > 2 && !haveWidth )
          *width = cf2_stack_getReal( opStack, 0 ) + nominalWidthX;

        
        haveWidth = TRUE;

        if ( font->decoder->width_only )
            goto exit;

        curY += cf2_stack_popFixed( opStack );
        curX += cf2_stack_popFixed( opStack );

        cf2_glyphpath_moveTo( &glyphPath, curX, curY );

        break;

      case cf2_cmdHMOVETO:
        FT_TRACE4(( " hmoveto\n" ));

        if ( cf2_stack_count( opStack ) > 1 && !haveWidth )
          *width = cf2_stack_getReal( opStack, 0 ) + nominalWidthX;

        
        haveWidth = TRUE;

        if ( font->decoder->width_only )
            goto exit;

        curX += cf2_stack_popFixed( opStack );

        cf2_glyphpath_moveTo( &glyphPath, curX, curY );

        break;

      case cf2_cmdRLINECURVE:
        {
          CF2_UInt  count = cf2_stack_count( opStack );
          CF2_UInt  index = 0;


          FT_TRACE4(( " rlinecurve\n" ));

          while ( index + 6 < count )
          {
            curX += cf2_stack_getReal( opStack, index + 0 );
            curY += cf2_stack_getReal( opStack, index + 1 );

            cf2_glyphpath_lineTo( &glyphPath, curX, curY );
            index += 2;
          }

          while ( index < count )
          {
            CF2_Fixed  x1 = cf2_stack_getReal( opStack, index + 0 ) + curX;
            CF2_Fixed  y1 = cf2_stack_getReal( opStack, index + 1 ) + curY;
            CF2_Fixed  x2 = cf2_stack_getReal( opStack, index + 2 ) + x1;
            CF2_Fixed  y2 = cf2_stack_getReal( opStack, index + 3 ) + y1;
            CF2_Fixed  x3 = cf2_stack_getReal( opStack, index + 4 ) + x2;
            CF2_Fixed  y3 = cf2_stack_getReal( opStack, index + 5 ) + y2;


            cf2_glyphpath_curveTo( &glyphPath, x1, y1, x2, y2, x3, y3 );

            curX   = x3;
            curY   = y3;
            index += 6;
          }

          cf2_stack_clear( opStack );
        }
        continue; 

      case cf2_cmdVVCURVETO:
        {
          CF2_UInt  count = cf2_stack_count( opStack );
          CF2_UInt  index = 0;


          FT_TRACE4(( " vvcurveto\n" ));

          while ( index < count )
          {
            CF2_Fixed  x1, y1, x2, y2, x3, y3;


            if ( ( count - index ) & 1 )
            {
              x1 = cf2_stack_getReal( opStack, index ) + curX;

              ++index;
            }
            else
              x1 = curX;

            y1 = cf2_stack_getReal( opStack, index + 0 ) + curY;
            x2 = cf2_stack_getReal( opStack, index + 1 ) + x1;
            y2 = cf2_stack_getReal( opStack, index + 2 ) + y1;
            x3 = x2;
            y3 = cf2_stack_getReal( opStack, index + 3 ) + y2;

            cf2_glyphpath_curveTo( &glyphPath, x1, y1, x2, y2, x3, y3 );

            curX   = x3;
            curY   = y3;
            index += 4;
          }

          cf2_stack_clear( opStack );
        }
        continue; 

      case cf2_cmdHHCURVETO:
        {
          CF2_UInt  count = cf2_stack_count( opStack );
          CF2_UInt  index = 0;


          FT_TRACE4(( " hhcurveto\n" ));

          while ( index < count )
          {
            CF2_Fixed  x1, y1, x2, y2, x3, y3;


            if ( ( count - index ) & 1 )
            {
              y1 = cf2_stack_getReal( opStack, index ) + curY;

              ++index;
            }
            else
              y1 = curY;

            x1 = cf2_stack_getReal( opStack, index + 0 ) + curX;
            x2 = cf2_stack_getReal( opStack, index + 1 ) + x1;
            y2 = cf2_stack_getReal( opStack, index + 2 ) + y1;
            x3 = cf2_stack_getReal( opStack, index + 3 ) + x2;
            y3 = y2;

            cf2_glyphpath_curveTo( &glyphPath, x1, y1, x2, y2, x3, y3 );

            curX   = x3;
            curY   = y3;
            index += 4;
          }

          cf2_stack_clear( opStack );
        }
        continue; 

      case cf2_cmdVHCURVETO:
      case cf2_cmdHVCURVETO:
        {
          CF2_UInt  count = cf2_stack_count( opStack );
          CF2_UInt  index = 0;

          FT_Bool  alternate = op1 == cf2_cmdHVCURVETO;


          FT_TRACE4(( alternate ? " hvcurveto\n" : " vhcurveto\n" ));

          while ( index < count )
          {
            CF2_Fixed x1, x2, x3, y1, y2, y3;


            if ( alternate )
            {
              x1 = cf2_stack_getReal( opStack, index + 0 ) + curX;
              y1 = curY;
              x2 = cf2_stack_getReal( opStack, index + 1 ) + x1;
              y2 = cf2_stack_getReal( opStack, index + 2 ) + y1;
              y3 = cf2_stack_getReal( opStack, index + 3 ) + y2;

              if ( count - index == 5 )
              {
                x3 = cf2_stack_getReal( opStack, index + 4 ) + x2;

                ++index;
              }
              else
                x3 = x2;

              alternate = FALSE;
            }
            else
            {
              x1 = curX;
              y1 = cf2_stack_getReal( opStack, index + 0 ) + curY;
              x2 = cf2_stack_getReal( opStack, index + 1 ) + x1;
              y2 = cf2_stack_getReal( opStack, index + 2 ) + y1;
              x3 = cf2_stack_getReal( opStack, index + 3 ) + x2;

              if ( count - index == 5 )
              {
                y3 = cf2_stack_getReal( opStack, index + 4 ) + y2;

                ++index;
              }
              else
                y3 = y2;

              alternate = TRUE;
            }

            cf2_glyphpath_curveTo( &glyphPath, x1, y1, x2, y2, x3, y3 );

            curX   = x3;
            curY   = y3;
            index += 4;
          }

          cf2_stack_clear( opStack );
        }
        continue;     

      case cf2_cmdEXTENDEDNMBR:
        {
          CF2_Int  v;


          v = (FT_Short)( ( cf2_buf_readByte( charstring ) << 8 ) |
                            cf2_buf_readByte( charstring )        );

          FT_TRACE4(( " %d", v ));

          cf2_stack_pushInt( opStack, v );
        }
        continue;

      default:
        
        {
          if (  op1 <= 246 )
          {
            CF2_Int  v;


            v = op1 - 139;

            FT_TRACE4(( " %d", v ));

            
            cf2_stack_pushInt( opStack, v );
          }

          else if (  op1 <= 250 )
          {
            CF2_Int  v;


            v  = op1;
            v -= 247;
            v *= 256;
            v += cf2_buf_readByte( charstring );
            v += 108;

            FT_TRACE4(( " %d", v ));

            
            cf2_stack_pushInt( opStack, v );
          }

          else if (  op1 <= 254 )
          {
            CF2_Int  v;


            v  = op1;
            v -= 251;
            v *= 256;
            v += cf2_buf_readByte( charstring );
            v  = -v - 108;

            FT_TRACE4(( " %d", v ));

            
            cf2_stack_pushInt( opStack, v );
          }

          else 
          {
            CF2_Fixed  v;


            v = (CF2_Fixed)
                  ( ( (FT_UInt32)cf2_buf_readByte( charstring ) << 24 ) |
                    ( (FT_UInt32)cf2_buf_readByte( charstring ) << 16 ) |
                    ( (FT_UInt32)cf2_buf_readByte( charstring ) <<  8 ) |
                      (FT_UInt32)cf2_buf_readByte( charstring )         );

            FT_TRACE4(( " %.2f", v / 65536.0 ));

            cf2_stack_pushFixed( opStack, v );
          }
        }
        continue;   

      } 

      cf2_stack_clear( opStack );

    } 

    
    FT_TRACE4(( "cf2_interpT2CharString:"
                "  charstring ends without ENDCHAR\n" ));

  exit:
    
    cf2_setError( error, lastError );

    
    cf2_glyphpath_finalize( &glyphPath );
    cf2_arrstack_finalize( &vStemHintArray );
    cf2_arrstack_finalize( &hStemHintArray );
    cf2_arrstack_finalize( &subrStack );
    cf2_stack_free( opStack );

    FT_TRACE4(( "\n" ));

    return;
  }




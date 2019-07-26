





































#ifndef __CF2FONT_H__
#define __CF2FONT_H__


#include "cf2ft.h"
#include "cf2blues.h"


FT_BEGIN_HEADER


#define CF2_OPERAND_STACK_SIZE  48
#define CF2_MAX_SUBR            10 /* maximum subroutine nesting */


  
  struct  CF2_FontRec_
  {
    FT_Memory  memory;
    FT_Error   error;     

    CF2_RenderingFlags  renderingFlags;

    
    
    

    CF2_Matrix  currentTransform;  
    CF2_Matrix  innerTransform;    
    CF2_Matrix  outerTransform;    
    CF2_Fixed   ppem;              

    CF2_Int  unitsPerEm;

    CF2_Fixed  syntheticEmboldeningAmountX;   
    CF2_Fixed  syntheticEmboldeningAmountY;   

    
    CF2_OutlineRec  outline;       
    CFF_Decoder*    decoder;
    CFF_SubFont     lastSubfont;              
                                              

    
    FT_Bool  hinted;
    FT_Bool  darkened;       
                             
    FT_Bool  stemDarkened;

    
    CF2_Fixed  stdVW;     
    CF2_Fixed  stdHW;     
    CF2_Fixed  darkenX;                    
    CF2_Fixed  darkenY;                    
                                           
    FT_Bool  reverseWinding;               
                                           

    CF2_BluesRec  blues;                         
  };


  FT_LOCAL( FT_Error )
  cf2_getGlyphOutline( CF2_Font           font,
                       CF2_Buffer         charstring,
                       const CF2_Matrix*  transform,
                       CF2_F16Dot16*      glyphWidth );


FT_END_HEADER


#endif 




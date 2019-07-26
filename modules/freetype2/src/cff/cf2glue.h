





































#ifndef __CF2GLUE_H__
#define __CF2GLUE_H__



#include "cf2error.h"
#include "cf2fixed.h"
#include "cf2arrst.h"
#include "cf2read.h"


FT_BEGIN_HEADER


  

  
#define CF2_FlagsHinted    1
  
#define CF2_FlagsDarkened  2

  
  typedef CF2_Int  CF2_RenderingFlags;


  
  typedef enum  CF2_PathOp_
  {
    CF2_PathOpMoveTo = 1,     
    CF2_PathOpLineTo = 2,     
    CF2_PathOpQuadTo = 3,     
    CF2_PathOpCubeTo = 4      

  } CF2_PathOp;


  
  typedef struct  CF2_Matrix_
  {
    CF2_F16Dot16  a;
    CF2_F16Dot16  b;
    CF2_F16Dot16  c;
    CF2_F16Dot16  d;
    CF2_F16Dot16  tx;
    CF2_F16Dot16  ty;

  } CF2_Matrix;


  
  
  typedef struct CF2_FontRec_  CF2_FontRec, *CF2_Font;
  typedef struct CF2_HintRec_  CF2_HintRec, *CF2_Hint;


  
  
  
  
  
  
  

  typedef struct  CF2_CallbackParamsRec_
  {
    FT_Vector  pt0;
    FT_Vector  pt1;
    FT_Vector  pt2;
    FT_Vector  pt3;

    CF2_Int  op;

  } CF2_CallbackParamsRec, *CF2_CallbackParams;


  
  typedef struct CF2_OutlineCallbacksRec_  CF2_OutlineCallbacksRec,
                                           *CF2_OutlineCallbacks;

  
  typedef void
  (*CF2_Callback_Type)( CF2_OutlineCallbacks      callbacks,
                        const CF2_CallbackParams  params );


  struct  CF2_OutlineCallbacksRec_
  {
    CF2_Callback_Type  moveTo;
    CF2_Callback_Type  lineTo;
    CF2_Callback_Type  quadTo;
    CF2_Callback_Type  cubeTo;

    CF2_Int  windingMomentum;    

    FT_Memory  memory;
    FT_Error*  error;
  };


FT_END_HEADER


#endif 




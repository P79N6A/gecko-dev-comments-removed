



















#ifndef __PSHINTS_H__
#define __PSHINTS_H__


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TYPE1_TABLES_H


FT_BEGIN_HEADER


  
  
  
  
  
  
  

  typedef struct PSH_GlobalsRec_*  PSH_Globals;

  typedef FT_Error
  (*PSH_Globals_NewFunc)( FT_Memory     memory,
                          T1_Private*   private_dict,
                          PSH_Globals*  aglobals );

  typedef FT_Error
  (*PSH_Globals_SetScaleFunc)( PSH_Globals  globals,
                               FT_Fixed     x_scale,
                               FT_Fixed     y_scale,
                               FT_Fixed     x_delta,
                               FT_Fixed     y_delta );

  typedef void
  (*PSH_Globals_DestroyFunc)( PSH_Globals  globals );


  typedef struct  PSH_Globals_FuncsRec_
  {
    PSH_Globals_NewFunc       create;
    PSH_Globals_SetScaleFunc  set_scale;
    PSH_Globals_DestroyFunc   destroy;

  } PSH_Globals_FuncsRec, *PSH_Globals_Funcs;


  
  
  
  
  
  
  

  



























  typedef struct T1_HintsRec_*  T1_Hints;


  









  typedef const struct T1_Hints_FuncsRec_*  T1_Hints_Funcs;


  

















  typedef void
  (*T1_Hints_OpenFunc)( T1_Hints  hints );


  


































  typedef void
  (*T1_Hints_SetStemFunc)( T1_Hints  hints,
                           FT_UInt   dimension,
                           FT_Long*  coords );


  



























  typedef void
  (*T1_Hints_SetStem3Func)( T1_Hints  hints,
                            FT_UInt   dimension,
                            FT_Long*  coords );


  

















  typedef void
  (*T1_Hints_ResetFunc)( T1_Hints  hints,
                         FT_UInt   end_point );


  























  typedef FT_Error
  (*T1_Hints_CloseFunc)( T1_Hints  hints,
                         FT_UInt   end_point );


  

































  typedef FT_Error
  (*T1_Hints_ApplyFunc)( T1_Hints        hints,
                         FT_Outline*     outline,
                         PSH_Globals     globals,
                         FT_Render_Mode  hint_mode );


  






























  typedef struct  T1_Hints_FuncsRec_
  {
    T1_Hints               hints;
    T1_Hints_OpenFunc      open;
    T1_Hints_CloseFunc     close;
    T1_Hints_SetStemFunc   stem;
    T1_Hints_SetStem3Func  stem3;
    T1_Hints_ResetFunc     reset;
    T1_Hints_ApplyFunc     apply;

  } T1_Hints_FuncsRec;


  
  
  
  
  
  
  

  



























  typedef struct T2_HintsRec_*  T2_Hints;


  









  typedef const struct T2_Hints_FuncsRec_*  T2_Hints_Funcs;


  

















  typedef void
  (*T2_Hints_OpenFunc)( T2_Hints  hints );


  



































  typedef void
  (*T2_Hints_StemsFunc)( T2_Hints   hints,
                         FT_UInt    dimension,
                         FT_UInt    count,
                         FT_Fixed*  coordinates );


  


































  typedef void
  (*T2_Hints_MaskFunc)( T2_Hints        hints,
                        FT_UInt         end_point,
                        FT_UInt         bit_count,
                        const FT_Byte*  bytes );


  


































  typedef void
  (*T2_Hints_CounterFunc)( T2_Hints        hints,
                           FT_UInt         bit_count,
                           const FT_Byte*  bytes );


  























  typedef FT_Error
  (*T2_Hints_CloseFunc)( T2_Hints  hints,
                         FT_UInt   end_point );


  

































  typedef FT_Error
  (*T2_Hints_ApplyFunc)( T2_Hints        hints,
                         FT_Outline*     outline,
                         PSH_Globals     globals,
                         FT_Render_Mode  hint_mode );


  






























  typedef struct  T2_Hints_FuncsRec_
  {
    T2_Hints              hints;
    T2_Hints_OpenFunc     open;
    T2_Hints_CloseFunc    close;
    T2_Hints_StemsFunc    stems;
    T2_Hints_MaskFunc     hintmask;
    T2_Hints_CounterFunc  counter;
    T2_Hints_ApplyFunc    apply;

  } T2_Hints_FuncsRec;


  


  typedef struct  PSHinter_Interface_
  {
    PSH_Globals_Funcs  (*get_globals_funcs)( FT_Module  module );
    T1_Hints_Funcs     (*get_t1_funcs)     ( FT_Module  module );
    T2_Hints_Funcs     (*get_t2_funcs)     ( FT_Module  module );

  } PSHinter_Interface;

  typedef PSHinter_Interface*  PSHinter_Service;


FT_END_HEADER

#endif 




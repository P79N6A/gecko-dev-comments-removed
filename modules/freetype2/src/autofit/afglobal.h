


















#ifndef __AFGLOBAL_H__
#define __AFGLOBAL_H__


#include "aftypes.h"
#include "afmodule.h"


FT_BEGIN_HEADER


  




  
#define AF_SCRIPT_FALLBACK  2
  
#define AF_SCRIPT_NONE      0x7F
  
#define AF_DIGIT            0x80

  
#define AF_PROP_INCREASE_X_HEIGHT_MIN  6
#define AF_PROP_INCREASE_X_HEIGHT_MAX  0


  
  
  
  
  
  
  


  




  typedef struct  AF_FaceGlobalsRec_
  {
    FT_Face           face;
    FT_Long           glyph_count;    
    FT_Byte*          glyph_scripts;

    
    FT_UInt           increase_x_height;

    AF_ScriptMetrics  metrics[AF_SCRIPT_MAX];

    AF_Module         module;         

  } AF_FaceGlobalsRec;


  




  FT_LOCAL( FT_Error )
  af_face_globals_new( FT_Face          face,
                       AF_FaceGlobals  *aglobals,
                       AF_Module        module );

  FT_LOCAL( FT_Error )
  af_face_globals_get_metrics( AF_FaceGlobals     globals,
                               FT_UInt            gindex,
                               FT_UInt            options,
                               AF_ScriptMetrics  *ametrics );

  FT_LOCAL( void )
  af_face_globals_free( AF_FaceGlobals  globals );

  FT_LOCAL_DEF( FT_Bool )
  af_face_globals_is_digit( AF_FaceGlobals  globals,
                            FT_UInt         gindex );

  


FT_END_HEADER

#endif 




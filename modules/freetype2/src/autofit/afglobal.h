


















#ifndef __AFGLOBAL_H__
#define __AFGLOBAL_H__


#include "aftypes.h"
#include "afmodule.h"
#include "hbshim.h"


FT_BEGIN_HEADER


  FT_LOCAL_ARRAY( AF_WritingSystemClass )
  af_writing_system_classes[];


#undef  SCRIPT
#define SCRIPT( s, S, d, h, sc1, sc2, sc3 )                    \
          AF_DECLARE_SCRIPT_CLASS( af_ ## s ## _script_class )

#include "afscript.h"

  FT_LOCAL_ARRAY( AF_ScriptClass )
  af_script_classes[];


#undef  STYLE
#define STYLE( s, S, d, ws, sc, ss, c )                      \
          AF_DECLARE_STYLE_CLASS( af_ ## s ## _style_class )

#include "afstyles.h"

  FT_LOCAL_ARRAY( AF_StyleClass )
  af_style_classes[];


#ifdef FT_DEBUG_LEVEL_TRACE
  FT_LOCAL_ARRAY( char* )
  af_style_names[];
#endif


  




  
#ifdef AF_CONFIG_OPTION_CJK
#define AF_STYLE_FALLBACK  AF_STYLE_HANI_DFLT
#else
#define AF_STYLE_FALLBACK  AF_STYLE_NONE_DFLT
#endif
  
#define AF_SCRIPT_DEFAULT  AF_SCRIPT_LATN
  
#define AF_STYLE_UNASSIGNED  0x7F
  
#define AF_DIGIT              0x80

  
#define AF_PROP_INCREASE_X_HEIGHT_MIN  6
#define AF_PROP_INCREASE_X_HEIGHT_MAX  0


  
  
  
  
  
  
  


  




  typedef struct  AF_FaceGlobalsRec_
  {
    FT_Face          face;
    FT_Long          glyph_count;    
    FT_Byte*         glyph_styles;

#ifdef FT_CONFIG_OPTION_USE_HARFBUZZ
    hb_font_t*       hb_font;
#endif

    
    FT_UInt          increase_x_height;

    AF_StyleMetrics  metrics[AF_STYLE_MAX];

    AF_Module        module;         

  } AF_FaceGlobalsRec;


  




  FT_LOCAL( FT_Error )
  af_face_globals_new( FT_Face          face,
                       AF_FaceGlobals  *aglobals,
                       AF_Module        module );

  FT_LOCAL( FT_Error )
  af_face_globals_get_metrics( AF_FaceGlobals    globals,
                               FT_UInt           gindex,
                               FT_UInt           options,
                               AF_StyleMetrics  *ametrics );

  FT_LOCAL( void )
  af_face_globals_free( AF_FaceGlobals  globals );

  FT_LOCAL_DEF( FT_Bool )
  af_face_globals_is_digit( AF_FaceGlobals  globals,
                            FT_UInt         gindex );

  


FT_END_HEADER

#endif 




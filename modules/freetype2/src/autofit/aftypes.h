

















  















#ifndef __AFTYPES_H__
#define __AFTYPES_H__

#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H


FT_BEGIN_HEADER

  
  
  
  
  
  
  

#ifdef FT_DEBUG_AUTOFIT

#include FT_CONFIG_STANDARD_LIBRARY_H

extern int    _af_debug_disable_horz_hints;
extern int    _af_debug_disable_vert_hints;
extern int    _af_debug_disable_blue_hints;
extern void*  _af_debug_hints;

#endif 


  
  
  
  
  
  
  

  typedef struct  AF_WidthRec_
  {
    FT_Pos  org;  
    FT_Pos  cur;  
    FT_Pos  fit;  

  } AF_WidthRec, *AF_Width;


  FT_LOCAL( void )
  af_sort_pos( FT_UInt  count,
               FT_Pos*  table );

  FT_LOCAL( void )
  af_sort_widths( FT_UInt   count,
                  AF_Width  widths );


  
  
  
  
  
  
  

  





  typedef FT_Int  AF_Angle;


#define AF_ANGLE_PI   256
#define AF_ANGLE_2PI  ( AF_ANGLE_PI * 2 )
#define AF_ANGLE_PI2  ( AF_ANGLE_PI / 2 )
#define AF_ANGLE_PI4  ( AF_ANGLE_PI / 4 )


#if 0
  


  FT_LOCAL( AF_Angle )
  af_angle_atan( FT_Pos  dx,
                 FT_Pos  dy );


  



  FT_LOCAL( AF_Angle )
  af_angle_diff( AF_Angle  angle1,
                 AF_Angle  angle2 );
#endif 


#define AF_ANGLE_DIFF( result, angle1, angle2 ) \
  FT_BEGIN_STMNT                                \
    AF_Angle  _delta = (angle2) - (angle1);     \
                                                \
                                                \
    _delta %= AF_ANGLE_2PI;                     \
    if ( _delta < 0 )                           \
      _delta += AF_ANGLE_2PI;                   \
                                                \
    if ( _delta > AF_ANGLE_PI )                 \
      _delta -= AF_ANGLE_2PI;                   \
                                                \
    result = _delta;                            \
  FT_END_STMNT


  


  typedef struct AF_GlyphHintsRec_*  AF_GlyphHints;


  
  
  
  
  
  
  

  




  typedef enum  AF_ScalerFlags_
  {
    AF_SCALER_FLAG_NO_HORIZONTAL = 1,  
    AF_SCALER_FLAG_NO_VERTICAL   = 2,  
    AF_SCALER_FLAG_NO_ADVANCE    = 4   

  } AF_ScalerFlags;


  typedef struct  AF_ScalerRec_
  {
    FT_Face         face;        
    FT_Fixed        x_scale;     
    FT_Fixed        y_scale;     
    FT_Pos          x_delta;     
    FT_Pos          y_delta;     
    FT_Render_Mode  render_mode; 
    FT_UInt32       flags;       

  } AF_ScalerRec, *AF_Scaler;


#define AF_SCALER_EQUAL_SCALES( a, b )      \
          ( (a)->x_scale == (b)->x_scale && \
            (a)->y_scale == (b)->y_scale && \
            (a)->x_delta == (b)->x_delta && \
            (a)->y_delta == (b)->y_delta )


  
  
  
  
  
  
  

  




















  typedef enum  AF_Script_
  {
    AF_SCRIPT_NONE  = 0,
    AF_SCRIPT_LATIN = 1,
    AF_SCRIPT_CJK   = 2,
    AF_SCRIPT_INDIC = 3,
#ifdef FT_OPTION_AUTOFIT2
    AF_SCRIPT_LATIN2,
#endif

    
    

    AF_SCRIPT_MAX   

  } AF_Script;


  typedef struct AF_ScriptClassRec_ const*  AF_ScriptClass;

  typedef struct  AF_ScriptMetricsRec_
  {
    AF_ScriptClass  clazz;
    AF_ScalerRec    scaler;
    FT_Bool         digits_have_same_width;

  } AF_ScriptMetricsRec, *AF_ScriptMetrics;


  


  typedef FT_Error
  (*AF_Script_InitMetricsFunc)( AF_ScriptMetrics  metrics,
                                FT_Face           face );

  typedef void
  (*AF_Script_ScaleMetricsFunc)( AF_ScriptMetrics  metrics,
                                 AF_Scaler         scaler );

  typedef void
  (*AF_Script_DoneMetricsFunc)( AF_ScriptMetrics  metrics );


  typedef FT_Error
  (*AF_Script_InitHintsFunc)( AF_GlyphHints     hints,
                              AF_ScriptMetrics  metrics );

  typedef void
  (*AF_Script_ApplyHintsFunc)( AF_GlyphHints     hints,
                               FT_Outline*       outline,
                               AF_ScriptMetrics  metrics );


  typedef struct  AF_Script_UniRangeRec_
  {
    FT_UInt32  first;
    FT_UInt32  last;

  } AF_Script_UniRangeRec;

#define AF_UNIRANGE_REC( a, b ) { (FT_UInt32)(a), (FT_UInt32)(b) }

  typedef const AF_Script_UniRangeRec  *AF_Script_UniRange;


  typedef struct  AF_ScriptClassRec_
  {
    AF_Script                   script;
    AF_Script_UniRange          script_uni_ranges; 

    FT_Offset                   script_metrics_size;
    AF_Script_InitMetricsFunc   script_metrics_init;
    AF_Script_ScaleMetricsFunc  script_metrics_scale;
    AF_Script_DoneMetricsFunc   script_metrics_done;

    AF_Script_InitHintsFunc     script_hints_init;
    AF_Script_ApplyHintsFunc    script_hints_apply;

  } AF_ScriptClassRec;


  
#ifndef FT_CONFIG_OPTION_PIC

#define AF_DECLARE_SCRIPT_CLASS( script_class ) \
  FT_CALLBACK_TABLE const AF_ScriptClassRec     \
  script_class;

#define AF_DEFINE_SCRIPT_CLASS( script_class, script_, ranges, m_size,     \
                                m_init, m_scale, m_done, h_init, h_apply ) \
  FT_CALLBACK_TABLE_DEF const AF_ScriptClassRec                            \
  script_class =                                                           \
  {                                                                        \
    script_,                                                               \
    ranges,                                                                \
                                                                           \
    m_size,                                                                \
                                                                           \
    m_init,                                                                \
    m_scale,                                                               \
    m_done,                                                                \
                                                                           \
    h_init,                                                                \
    h_apply                                                                \
  };

#else 

#define AF_DECLARE_SCRIPT_CLASS( script_class )          \
  FT_LOCAL( void )                                       \
  FT_Init_Class_##script_class( AF_ScriptClassRec* ac );

#define AF_DEFINE_SCRIPT_CLASS( script_class, script_, ranges, m_size,     \
                                m_init, m_scale, m_done, h_init, h_apply ) \
  FT_LOCAL_DEF( void )                                                     \
  FT_Init_Class_##script_class( AF_ScriptClassRec* ac )                    \
  {                                                                        \
    ac->script               = script_;                                    \
    ac->script_uni_ranges    = ranges;                                     \
                                                                           \
    ac->script_metrics_size  = m_size;                                     \
                                                                           \
    ac->script_metrics_init  = m_init;                                     \
    ac->script_metrics_scale = m_scale;                                    \
    ac->script_metrics_done  = m_done;                                     \
                                                                           \
    ac->script_hints_init    = h_init;                                     \
    ac->script_hints_apply   = h_apply;                                    \
  }

#endif 




FT_END_HEADER

#endif 




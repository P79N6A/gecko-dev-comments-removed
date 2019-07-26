

















#include "afglobal.h"

  
#undef  WRITING_SYSTEM
#define WRITING_SYSTEM( ws, WS )
#include "afwrtsys.h"

#include "aferrors.h"
#include "afpic.h"


#ifndef FT_CONFIG_OPTION_PIC

#undef  WRITING_SYSTEM
#define WRITING_SYSTEM( ws, WS )               \
          &af_ ## ws ## _writing_system_class,

  FT_LOCAL_ARRAY_DEF( AF_WritingSystemClass )
  af_writing_system_classes[] =
  {

#include "afwrtsys.h"

    NULL  
  };


#undef  SCRIPT
#define SCRIPT( s, S, d )             \
          &af_ ## s ## _script_class,

  FT_LOCAL_ARRAY_DEF( AF_ScriptClass )
  af_script_classes[] =
  {

#include "afscript.h"

    NULL  
  };

#endif 


#ifdef FT_DEBUG_LEVEL_TRACE

#undef  SCRIPT
#define SCRIPT( s, S, d )  #s,

  FT_LOCAL_ARRAY_DEF( char* )
  af_script_names[] =
  {

#include "afscript.h"

  };

#endif 


  

  static FT_Error
  af_face_globals_compute_script_coverage( AF_FaceGlobals  globals )
  {
    FT_Error    error;
    FT_Face     face        = globals->face;
    FT_CharMap  old_charmap = face->charmap;
    FT_Byte*    gscripts    = globals->glyph_scripts;
    FT_UInt     ss;
    FT_UInt     i;


    
    FT_MEM_SET( globals->glyph_scripts,
                AF_SCRIPT_NONE,
                globals->glyph_count );

    error = FT_Select_Charmap( face, FT_ENCODING_UNICODE );
    if ( error )
    {
     



      error = FT_Err_Ok;
      goto Exit;
    }

    
    for ( ss = 0; AF_SCRIPT_CLASSES_GET[ss]; ss++ )
    {
      AF_ScriptClass      script_class = AF_SCRIPT_CLASSES_GET[ss];
      AF_Script_UniRange  range;


      if ( script_class->script_uni_ranges == NULL )
        continue;

      



      for ( range = script_class->script_uni_ranges;
            range->first != 0;
            range++ )
      {
        FT_ULong  charcode = range->first;
        FT_UInt   gindex;


        gindex = FT_Get_Char_Index( face, charcode );

        if ( gindex != 0                             &&
             gindex < (FT_ULong)globals->glyph_count &&
             gscripts[gindex] == AF_SCRIPT_NONE )
          gscripts[gindex] = (FT_Byte)ss;

        for (;;)
        {
          charcode = FT_Get_Next_Char( face, charcode, &gindex );

          if ( gindex == 0 || charcode > range->last )
            break;

          if ( gindex < (FT_ULong)globals->glyph_count &&
               gscripts[gindex] == AF_SCRIPT_NONE )
            gscripts[gindex] = (FT_Byte)ss;
        }
      }
    }

    
    for ( i = 0x30; i <= 0x39; i++ )
    {
      FT_UInt  gindex = FT_Get_Char_Index( face, i );


      if ( gindex != 0 && gindex < (FT_ULong)globals->glyph_count )
        gscripts[gindex] |= AF_DIGIT;
    }

  Exit:
    



    if ( globals->module->fallback_script != AF_SCRIPT_NONE )
    {
      FT_Long  nn;


      for ( nn = 0; nn < globals->glyph_count; nn++ )
      {
        if ( ( gscripts[nn] & ~AF_DIGIT ) == AF_SCRIPT_NONE )
        {
          gscripts[nn] &= ~AF_SCRIPT_NONE;
          gscripts[nn] |= globals->module->fallback_script;
        }
      }
    }

    FT_Set_Charmap( face, old_charmap );
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  af_face_globals_new( FT_Face          face,
                       AF_FaceGlobals  *aglobals,
                       AF_Module        module )
  {
    FT_Error        error;
    FT_Memory       memory;
    AF_FaceGlobals  globals = NULL;


    memory = face->memory;

    if ( FT_ALLOC( globals, sizeof ( *globals ) +
                            face->num_glyphs * sizeof ( FT_Byte ) ) )
      goto Exit;

    globals->face          = face;
    globals->glyph_count   = face->num_glyphs;
    globals->glyph_scripts = (FT_Byte*)( globals + 1 );
    globals->module        = module;

    error = af_face_globals_compute_script_coverage( globals );
    if ( error )
    {
      af_face_globals_free( globals );
      globals = NULL;
    }

    globals->increase_x_height = AF_PROP_INCREASE_X_HEIGHT_MAX;

  Exit:
    *aglobals = globals;
    return error;
  }


  FT_LOCAL_DEF( void )
  af_face_globals_free( AF_FaceGlobals  globals )
  {
    if ( globals )
    {
      FT_Memory  memory = globals->face->memory;
      FT_UInt    nn;


      for ( nn = 0; nn < AF_SCRIPT_MAX; nn++ )
      {
        if ( globals->metrics[nn] )
        {
          AF_ScriptClass         script_class =
            AF_SCRIPT_CLASSES_GET[nn];
          AF_WritingSystemClass  writing_system_class =
            AF_WRITING_SYSTEM_CLASSES_GET[script_class->writing_system];


          if ( writing_system_class->script_metrics_done )
            writing_system_class->script_metrics_done( globals->metrics[nn] );

          FT_FREE( globals->metrics[nn] );
        }
      }

      globals->glyph_count   = 0;
      globals->glyph_scripts = NULL;  
      globals->face          = NULL;

      FT_FREE( globals );
    }
  }


  FT_LOCAL_DEF( FT_Error )
  af_face_globals_get_metrics( AF_FaceGlobals     globals,
                               FT_UInt            gindex,
                               FT_UInt            options,
                               AF_ScriptMetrics  *ametrics )
  {
    AF_ScriptMetrics  metrics = NULL;

    AF_Script              script = (AF_Script)( options & 15 );
    AF_WritingSystemClass  writing_system_class;
    AF_ScriptClass         script_class;

    FT_Error  error = FT_Err_Ok;


    if ( gindex >= (FT_ULong)globals->glyph_count )
    {
      error = FT_THROW( Invalid_Argument );
      goto Exit;
    }

    
    
    if ( script == AF_SCRIPT_DFLT || script + 1 >= AF_SCRIPT_MAX )
      script = (AF_Script)( globals->glyph_scripts[gindex] & AF_SCRIPT_NONE );

    script_class         = AF_SCRIPT_CLASSES_GET[script];
    writing_system_class = AF_WRITING_SYSTEM_CLASSES_GET
                             [script_class->writing_system];

    metrics = globals->metrics[script];
    if ( metrics == NULL )
    {
      
      FT_Memory  memory = globals->face->memory;


      if ( FT_ALLOC( metrics, writing_system_class->script_metrics_size ) )
        goto Exit;

      metrics->script_class = script_class;
      metrics->globals      = globals;

      if ( writing_system_class->script_metrics_init )
      {
        error = writing_system_class->script_metrics_init( metrics,
                                                           globals->face );
        if ( error )
        {
          if ( writing_system_class->script_metrics_done )
            writing_system_class->script_metrics_done( metrics );

          FT_FREE( metrics );
          goto Exit;
        }
      }

      globals->metrics[script] = metrics;
    }

  Exit:
    *ametrics = metrics;

    return error;
  }


  FT_LOCAL_DEF( FT_Bool )
  af_face_globals_is_digit( AF_FaceGlobals  globals,
                            FT_UInt         gindex )
  {
    if ( gindex < (FT_ULong)globals->glyph_count )
      return (FT_Bool)( globals->glyph_scripts[gindex] & AF_DIGIT );

    return (FT_Bool)0;
  }




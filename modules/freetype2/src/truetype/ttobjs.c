

















#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_TAGS_H
#include FT_INTERNAL_SFNT_H

#include "ttgload.h"
#include "ttpload.h"

#include "tterrors.h"

#ifdef TT_USE_BYTECODE_INTERPRETER
#include "ttinterp.h"
#endif

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
#include FT_TRUETYPE_UNPATENTED_H
#endif

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include "ttgxvar.h"
#endif

  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttobjs


#ifdef TT_USE_BYTECODE_INTERPRETER

  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( void )
  tt_glyphzone_done( TT_GlyphZone  zone )
  {
    FT_Memory  memory = zone->memory;


    if ( memory )
    {
      FT_FREE( zone->contours );
      FT_FREE( zone->tags );
      FT_FREE( zone->cur );
      FT_FREE( zone->org );
      FT_FREE( zone->orus );

      zone->max_points   = zone->n_points   = 0;
      zone->max_contours = zone->n_contours = 0;
      zone->memory       = NULL;
    }
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  tt_glyphzone_new( FT_Memory     memory,
                    FT_UShort     maxPoints,
                    FT_Short      maxContours,
                    TT_GlyphZone  zone )
  {
    FT_Error  error;


    FT_MEM_ZERO( zone, sizeof ( *zone ) );
    zone->memory = memory;

    if ( FT_NEW_ARRAY( zone->org,      maxPoints   ) ||
         FT_NEW_ARRAY( zone->cur,      maxPoints   ) ||
         FT_NEW_ARRAY( zone->orus,     maxPoints   ) ||
         FT_NEW_ARRAY( zone->tags,     maxPoints   ) ||
         FT_NEW_ARRAY( zone->contours, maxContours ) )
    {
      tt_glyphzone_done( zone );
    }
    else
    {
      zone->max_points   = maxPoints;
      zone->max_contours = maxContours;
    }

    return error;
  }
#endif 


  
  

  static FT_Bool
  tt_check_trickyness_family( FT_String*  name )
  {

#define TRICK_NAMES_MAX_CHARACTERS  16
#define TRICK_NAMES_COUNT            8

    static const char trick_names[TRICK_NAMES_COUNT]
                                 [TRICK_NAMES_MAX_CHARACTERS + 1] =
    {
      "DFKaiSho-SB",     
      "DFKaiShu",
      "DFKai-SB",        
      "HuaTianKaiTi?",   
      "HuaTianSongTi?",  
      "MingLiU",         
      "PMingLiU",        
      "MingLi43",        
    };

    int  nn;


    for ( nn = 0; nn < TRICK_NAMES_COUNT; nn++ )
      if ( ft_strstr( name, trick_names[nn] ) )
        return TRUE;

    return FALSE;
  }


  

  
  
  
  

  static FT_UInt32
  tt_synth_sfnt_checksum( FT_Stream  stream,
                          FT_ULong   length )
  {
    FT_Error   error;
    FT_UInt32  checksum = 0;
    int        i;


    if ( FT_FRAME_ENTER( length ) )
      return 0;

    for ( ; length > 3; length -= 4 )
      checksum += (FT_UInt32)FT_GET_ULONG();

    for ( i = 3; length > 0; length --, i-- )
      checksum += (FT_UInt32)( FT_GET_BYTE() << ( i * 8 ) );

    FT_FRAME_EXIT();

    return checksum;
  }


  

  static FT_ULong
  tt_get_sfnt_checksum( TT_Face    face,
                        FT_UShort  i )
  {
#if 0 
    if ( face->dir_tables[i].CheckSum )
      return face->dir_tables[i].CheckSum;
#endif

    if ( !face->goto_table )
      return 0;

    if ( face->goto_table( face,
                           face->dir_tables[i].Tag,
                           face->root.stream,
                           NULL ) )
      return 0;

    return (FT_ULong)tt_synth_sfnt_checksum( face->root.stream,
                                             face->dir_tables[i].Length );
  }


  typedef struct tt_sfnt_id_rec_
  {
    FT_ULong  CheckSum;
    FT_ULong  Length;

  } tt_sfnt_id_rec;


  static FT_Bool
  tt_check_trickyness_sfnt_ids( TT_Face  face )
  {
#define TRICK_SFNT_IDS_PER_FACE   3
#define TRICK_SFNT_IDS_NUM_FACES  13

    static const tt_sfnt_id_rec sfnt_id[TRICK_SFNT_IDS_NUM_FACES]
                                       [TRICK_SFNT_IDS_PER_FACE] = {

#define TRICK_SFNT_ID_cvt   0
#define TRICK_SFNT_ID_fpgm  1
#define TRICK_SFNT_ID_prep  2

      { 
        { 0x05bcf058, 0x000002e4 }, 
        { 0x28233bf1, 0x000087c4 }, 
        { 0xa344a1ea, 0x000001e1 }  
      },
      { 
        { 0x05bcf058, 0x000002e4 }, 
        { 0x28233bf1, 0x000087c4 }, 
        { 0xa344a1eb, 0x000001e1 }  
      },
      { 
        { 0x11e5ead4, 0x00000350 }, 
        { 0x5a30ca3b, 0x00009063 }, 
        { 0x13a42602, 0x0000007e }  
      },
      { 
        { 0xfffbfffc, 0x00000008 }, 
        { 0x9c9e48b8, 0x0000bea2 }, 
        { 0x70020112, 0x00000008 }  
      },
      { 
        { 0xfffbfffc, 0x00000008 }, 
        { 0x0a5a0483, 0x00017c39 }, 
        { 0x70020112, 0x00000008 }  
      },
      { 
        { 0x00000000, 0x00000000 }, 
        { 0x40c92555, 0x000000e5 }, 
        { 0xa39b58e3, 0x0000117c }  
      },
      { 
        { 0x00000000, 0x00000000 }, 
        { 0x33c41652, 0x000000e5 }, 
        { 0x26d6c52a, 0x00000f6a }  
      },
      { 
        { 0x00000000, 0x00000000 }, 
        { 0x6db1651d, 0x0000019d }, 
        { 0x6c6e4b03, 0x00002492 }  
      },
      { 
        { 0x00000000, 0x00000000 }, 
        { 0x40c92555, 0x000000e5 }, 
        { 0xde51fad0, 0x0000117c }  
      },
      { 
        { 0x00000000, 0x00000000 }, 
        { 0x85e47664, 0x000000e5 }, 
        { 0xa6c62831, 0x00001caa }  
      },
      { 
        { 0x00000000, 0x00000000 }, 
        { 0x2d891cfd, 0x0000019d }, 
        { 0xa0604633, 0x00001de8 }  
      },
      { 
        { 0x00000000, 0x00000000 }, 
        { 0x40aa774c, 0x000001cb }, 
        { 0x9b5caa96, 0x00001f9a }  
      },
      { 
        { 0x00000000, 0x00000000 }, 
        { 0x0d3de9cb, 0x00000141 }, 
        { 0xd4127766, 0x00002280 }  
      }
    };

    FT_ULong   checksum;
    int        num_matched_ids[TRICK_SFNT_IDS_NUM_FACES];
    FT_Bool    has_cvt, has_fpgm, has_prep;
    FT_UShort  i;
    int        j, k;


    FT_MEM_SET( num_matched_ids, 0,
                sizeof ( int ) * TRICK_SFNT_IDS_NUM_FACES );
    has_cvt  = FALSE;
    has_fpgm = FALSE;
    has_prep = FALSE;

    for ( i = 0; i < face->num_tables; i++ )
    {
      checksum = 0;

      switch( face->dir_tables[i].Tag )
      {
      case TTAG_cvt:
        k = TRICK_SFNT_ID_cvt;
        has_cvt  = TRUE;
        break;

      case TTAG_fpgm:
        k = TRICK_SFNT_ID_fpgm;
        has_fpgm = TRUE;
        break;

      case TTAG_prep:
        k = TRICK_SFNT_ID_prep;
        has_prep = TRUE;
        break;

      default:
        continue;
      }

      for ( j = 0; j < TRICK_SFNT_IDS_NUM_FACES; j++ )
        if ( face->dir_tables[i].Length == sfnt_id[j][k].Length )
        {
          if ( !checksum )
            checksum = tt_get_sfnt_checksum( face, i );

          if ( sfnt_id[j][k].CheckSum == checksum )
            num_matched_ids[j]++;

          if ( num_matched_ids[j] == TRICK_SFNT_IDS_PER_FACE )
            return TRUE;
        }
    }

    for ( j = 0; j < TRICK_SFNT_IDS_NUM_FACES; j++ )
    {
      if ( !has_cvt  && !sfnt_id[j][TRICK_SFNT_ID_cvt].Length )
        num_matched_ids[j] ++;
      if ( !has_fpgm && !sfnt_id[j][TRICK_SFNT_ID_fpgm].Length )
        num_matched_ids[j] ++;
      if ( !has_prep && !sfnt_id[j][TRICK_SFNT_ID_prep].Length )
        num_matched_ids[j] ++;
      if ( num_matched_ids[j] == TRICK_SFNT_IDS_PER_FACE )
        return TRUE;
    }

    return FALSE;
  }


  static FT_Bool
  tt_check_trickyness( FT_Face  face )
  {
    if ( !face )
      return FALSE;

    
    if ( face->family_name                               &&
         tt_check_trickyness_family( face->family_name ) )
      return TRUE;

    
    
    
    if ( tt_check_trickyness_sfnt_ids( (TT_Face)face ) )
      return TRUE;

    return FALSE;
  }


  
  static FT_Bool
  tt_check_single_notdef( FT_Face  ttface )
  {
    FT_Bool   result = FALSE;

    TT_Face   face = (TT_Face)ttface;
    FT_UInt   asize;
    FT_ULong  i;
    FT_ULong  glyph_index = 0;
    FT_UInt   count       = 0;


    for( i = 0; i < face->num_locations; i++ )
    {
      tt_face_get_location( face, i, &asize );
      if ( asize > 0 )
      {
        count += 1;
        if ( count > 1 )
          break;
        glyph_index = i;
      }
    }

    
    if ( count == 1 )
    {
      if ( glyph_index == 0 )
        result = TRUE;
      else
      {
        
        FT_Error error;
        char buf[8];


        error = FT_Get_Glyph_Name( ttface, glyph_index, buf, 8 );
        if ( !error                                            &&
             buf[0] == '.' && !ft_strncmp( buf, ".notdef", 8 ) )
          result = TRUE;
      }
    }

    return result;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  tt_face_init( FT_Stream      stream,
                FT_Face        ttface,      
                FT_Int         face_index,
                FT_Int         num_params,
                FT_Parameter*  params )
  {
    FT_Error      error;
    FT_Library    library;
    SFNT_Service  sfnt;
    TT_Face       face = (TT_Face)ttface;


    FT_TRACE2(( "TTF driver\n" ));

    library = ttface->driver->root.library;

    sfnt = (SFNT_Service)FT_Get_Module_Interface( library, "sfnt" );
    if ( !sfnt )
    {
      FT_ERROR(( "tt_face_init: cannot access `sfnt' module\n" ));
      error = TT_Err_Missing_Module;
      goto Exit;
    }

    
    if ( FT_STREAM_SEEK( 0 ) )
      goto Exit;

    
    error = sfnt->init_face( stream, face, face_index, num_params, params );
    if ( error )
      goto Exit;

    
    
    
    if ( face->format_tag != 0x00010000L &&    
         face->format_tag != 0x00020000L &&    
         face->format_tag != TTAG_true   )     
    {
      FT_TRACE2(( "  not a TTF font\n" ));
      goto Bad_Format;
    }

#ifdef TT_USE_BYTECODE_INTERPRETER
    ttface->face_flags |= FT_FACE_FLAG_HINTER;
#endif

    
    if ( face_index < 0 )
      return TT_Err_Ok;

    
    error = sfnt->load_face( stream, face, face_index, num_params, params );
    if ( error )
      goto Exit;

    if ( tt_check_trickyness( ttface ) )
      ttface->face_flags |= FT_FACE_FLAG_TRICKY;

    error = tt_face_load_hdmx( face, stream );
    if ( error )
      goto Exit;

    if ( FT_IS_SCALABLE( ttface ) )
    {

#ifdef FT_CONFIG_OPTION_INCREMENTAL

      if ( !ttface->internal->incremental_interface )
        error = tt_face_load_loca( face, stream );
      if ( !error )
        error = tt_face_load_cvt( face, stream );
      if ( !error )
        error = tt_face_load_fpgm( face, stream );
      if ( !error )
        error = tt_face_load_prep( face, stream );

      
      if ( !ttface->internal->incremental_interface &&
           ttface->num_fixed_sizes                  &&
           face->glyph_locations                    &&
           tt_check_single_notdef( ttface )         )
      {
        FT_TRACE5(( "tt_face_init:"
                    " Only the `.notdef' glyph has an outline.\n"
                    "             "
                    " Resetting scalable flag to FALSE.\n" ));

        ttface->face_flags &= ~FT_FACE_FLAG_SCALABLE;
      }

#else

      if ( !error )
        error = tt_face_load_loca( face, stream );
      if ( !error )
        error = tt_face_load_cvt( face, stream );
      if ( !error )
        error = tt_face_load_fpgm( face, stream );
      if ( !error )
        error = tt_face_load_prep( face, stream );

      
      if ( ttface->num_fixed_sizes          &&
           face->glyph_locations            &&
           tt_check_single_notdef( ttface ) )
      {
        FT_TRACE5(( "tt_face_init:"
                    " Only the `.notdef' glyph has an outline.\n"
                    "             "
                    " Resetting scalable flag to FALSE.\n" ));

        ttface->face_flags &= ~FT_FACE_FLAG_SCALABLE;
      }

#endif

    }

#if defined( TT_CONFIG_OPTION_UNPATENTED_HINTING    ) && \
    !defined( TT_CONFIG_OPTION_BYTECODE_INTERPRETER )

    {
      FT_Bool  unpatented_hinting;
      int      i;


      
      unpatented_hinting = FT_BOOL
        ( library->debug_hooks[FT_DEBUG_HOOK_UNPATENTED_HINTING] != NULL );

      for ( i = 0; i < num_params && !face->unpatented_hinting; i++ )
        if ( params[i].tag == FT_PARAM_TAG_UNPATENTED_HINTING )
          unpatented_hinting = TRUE;

      if ( !unpatented_hinting )
        ttface->internal->ignore_unpatented_hinter = TRUE;
    }

#endif 


    
    TT_Init_Glyph_Loading( face );

  Exit:
    return error;

  Bad_Format:
    error = TT_Err_Unknown_File_Format;
    goto Exit;
  }


  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( void )
  tt_face_done( FT_Face  ttface )           
  {
    TT_Face       face = (TT_Face)ttface;
    FT_Memory     memory;
    FT_Stream     stream;
    SFNT_Service  sfnt;


    if ( !face )
      return;

    memory = ttface->memory;
    stream = ttface->stream;
    sfnt   = (SFNT_Service)face->sfnt;

    
    if ( face->extra.finalizer )
      face->extra.finalizer( face->extra.data );

    if ( sfnt )
      sfnt->done_face( face );

    
    tt_face_done_loca( face );

    tt_face_free_hdmx( face );

    
    FT_FREE( face->cvt );
    face->cvt_size = 0;

    
    FT_FRAME_RELEASE( face->font_program );
    FT_FRAME_RELEASE( face->cvt_program );
    face->font_program_size = 0;
    face->cvt_program_size  = 0;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
    tt_done_blend( memory, face->blend );
    face->blend = NULL;
#endif
  }


  
  
  
  
  

#ifdef TT_USE_BYTECODE_INTERPRETER

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  tt_size_run_fpgm( TT_Size  size,
                    FT_Bool  pedantic )
  {
    TT_Face         face = (TT_Face)size->root.face;
    TT_ExecContext  exec;
    FT_Error        error;


    
    if ( size->debug )
      exec = size->context;
    else
      exec = ( (TT_Driver)FT_FACE_DRIVER( face ) )->context;

    if ( !exec )
      return TT_Err_Could_Not_Find_Context;

    TT_Load_Context( exec, face, size );

    exec->callTop = 0;
    exec->top     = 0;

    exec->period    = 64;
    exec->phase     = 0;
    exec->threshold = 0;

    exec->instruction_trap = FALSE;
    exec->F_dot_P          = 0x10000L;

    exec->pedantic_hinting = pedantic;

    {
      FT_Size_Metrics*  metrics    = &exec->metrics;
      TT_Size_Metrics*  tt_metrics = &exec->tt_metrics;


      metrics->x_ppem   = 0;
      metrics->y_ppem   = 0;
      metrics->x_scale  = 0;
      metrics->y_scale  = 0;

      tt_metrics->ppem  = 0;
      tt_metrics->scale = 0;
      tt_metrics->ratio = 0x10000L;
    }

    
    TT_Set_CodeRange( exec,
                      tt_coderange_font,
                      face->font_program,
                      face->font_program_size );

    
    TT_Clear_CodeRange( exec, tt_coderange_cvt );
    TT_Clear_CodeRange( exec, tt_coderange_glyph );

    if ( face->font_program_size > 0 )
    {
      error = TT_Goto_CodeRange( exec, tt_coderange_font, 0 );

      if ( !error )
      {
        FT_TRACE4(( "Executing `fpgm' table.\n" ));

        error = face->interpreter( exec );
      }
    }
    else
      error = TT_Err_Ok;

    if ( !error )
      TT_Save_Context( exec, size );

    return error;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  tt_size_run_prep( TT_Size  size,
                    FT_Bool  pedantic )
  {
    TT_Face         face = (TT_Face)size->root.face;
    TT_ExecContext  exec;
    FT_Error        error;


    
    if ( size->debug )
      exec = size->context;
    else
      exec = ( (TT_Driver)FT_FACE_DRIVER( face ) )->context;

    if ( !exec )
      return TT_Err_Could_Not_Find_Context;

    TT_Load_Context( exec, face, size );

    exec->callTop = 0;
    exec->top     = 0;

    exec->instruction_trap = FALSE;

    exec->pedantic_hinting = pedantic;

    TT_Set_CodeRange( exec,
                      tt_coderange_cvt,
                      face->cvt_program,
                      face->cvt_program_size );

    TT_Clear_CodeRange( exec, tt_coderange_glyph );

    if ( face->cvt_program_size > 0 )
    {
      error = TT_Goto_CodeRange( exec, tt_coderange_cvt, 0 );

      if ( !error && !size->debug )
      {
        FT_TRACE4(( "Executing `prep' table.\n" ));

        error = face->interpreter( exec );
      }
    }
    else
      error = TT_Err_Ok;

    
    size->GS = exec->GS;

    TT_Save_Context( exec, size );

    return error;
  }

#endif 


#ifdef TT_USE_BYTECODE_INTERPRETER

  static void
  tt_size_done_bytecode( FT_Size  ftsize )
  {
    TT_Size    size   = (TT_Size)ftsize;
    TT_Face    face   = (TT_Face)ftsize->face;
    FT_Memory  memory = face->root.memory;


    if ( size->debug )
    {
      
      size->context = NULL;
      size->debug   = FALSE;
    }

    FT_FREE( size->cvt );
    size->cvt_size = 0;

    
    FT_FREE( size->storage );
    size->storage_size = 0;

    
    tt_glyphzone_done( &size->twilight );

    FT_FREE( size->function_defs );
    FT_FREE( size->instruction_defs );

    size->num_function_defs    = 0;
    size->max_function_defs    = 0;
    size->num_instruction_defs = 0;
    size->max_instruction_defs = 0;

    size->max_func = 0;
    size->max_ins  = 0;

    size->bytecode_ready = 0;
    size->cvt_ready      = 0;
  }


  
  
  static FT_Error
  tt_size_init_bytecode( FT_Size  ftsize,
                         FT_Bool  pedantic )
  {
    FT_Error   error;
    TT_Size    size = (TT_Size)ftsize;
    TT_Face    face = (TT_Face)ftsize->face;
    FT_Memory  memory = face->root.memory;
    FT_Int     i;

    FT_UShort       n_twilight;
    TT_MaxProfile*  maxp = &face->max_profile;


    size->bytecode_ready = 1;
    size->cvt_ready      = 0;

    size->max_function_defs    = maxp->maxFunctionDefs;
    size->max_instruction_defs = maxp->maxInstructionDefs;

    size->num_function_defs    = 0;
    size->num_instruction_defs = 0;

    size->max_func = 0;
    size->max_ins  = 0;

    size->cvt_size     = face->cvt_size;
    size->storage_size = maxp->maxStorage;

    
    {
      TT_Size_Metrics*  metrics = &size->ttmetrics;


      metrics->rotated   = FALSE;
      metrics->stretched = FALSE;

      
      for ( i = 0; i < 4; i++ )
        metrics->compensations[i] = 0;
    }

    
    if ( FT_NEW_ARRAY( size->function_defs,    size->max_function_defs    ) ||
         FT_NEW_ARRAY( size->instruction_defs, size->max_instruction_defs ) ||
         FT_NEW_ARRAY( size->cvt,              size->cvt_size             ) ||
         FT_NEW_ARRAY( size->storage,          size->storage_size         ) )
      goto Exit;

    
    n_twilight = maxp->maxTwilightPoints;

    
    n_twilight += 4;

    error = tt_glyphzone_new( memory, n_twilight, 0, &size->twilight );
    if ( error )
      goto Exit;

    size->twilight.n_points = n_twilight;

    size->GS = tt_default_graphics_state;

    
    {
      FT_Library  library = face->root.driver->root.library;


      face->interpreter = (TT_Interpreter)
                            library->debug_hooks[FT_DEBUG_HOOK_TRUETYPE];
      if ( !face->interpreter )
        face->interpreter = (TT_Interpreter)TT_RunIns;
    }

    
    error = tt_size_run_fpgm( size, pedantic );

  Exit:
    if ( error )
      tt_size_done_bytecode( ftsize );

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  tt_size_ready_bytecode( TT_Size  size,
                          FT_Bool  pedantic )
  {
    FT_Error  error = TT_Err_Ok;


    if ( !size->bytecode_ready )
    {
      error = tt_size_init_bytecode( (FT_Size)size, pedantic );
      if ( error )
        goto Exit;
    }

    
    if ( !size->cvt_ready )
    {
      FT_UInt  i;
      TT_Face  face = (TT_Face)size->root.face;


      
      
      for ( i = 0; i < size->cvt_size; i++ )
        size->cvt[i] = FT_MulFix( face->cvt[i], size->ttmetrics.scale );

      
      for ( i = 0; i < (FT_UInt)size->twilight.n_points; i++ )
      {
        size->twilight.org[i].x = 0;
        size->twilight.org[i].y = 0;
        size->twilight.cur[i].x = 0;
        size->twilight.cur[i].y = 0;
      }

      
      for ( i = 0; i < (FT_UInt)size->storage_size; i++ )
        size->storage[i] = 0;

      size->GS = tt_default_graphics_state;

      error = tt_size_run_prep( size, pedantic );
      if ( !error )
        size->cvt_ready = 1;
    }

  Exit:
    return error;
  }

#endif 


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  tt_size_init( FT_Size  ttsize )           
  {
    TT_Size   size  = (TT_Size)ttsize;
    FT_Error  error = TT_Err_Ok;

#ifdef TT_USE_BYTECODE_INTERPRETER
    size->bytecode_ready = 0;
    size->cvt_ready      = 0;
#endif

    size->ttmetrics.valid = FALSE;
    size->strike_index    = 0xFFFFFFFFUL;

    return error;
  }


  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( void )
  tt_size_done( FT_Size  ttsize )           
  {
    TT_Size  size = (TT_Size)ttsize;


#ifdef TT_USE_BYTECODE_INTERPRETER
    if ( size->bytecode_ready )
      tt_size_done_bytecode( ttsize );
#endif

    size->ttmetrics.valid = FALSE;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  tt_size_reset( TT_Size  size )
  {
    TT_Face           face;
    FT_Error          error = TT_Err_Ok;
    FT_Size_Metrics*  metrics;


    size->ttmetrics.valid = FALSE;

    face = (TT_Face)size->root.face;

    metrics = &size->metrics;

    
    *metrics = size->root.metrics;

    if ( metrics->x_ppem < 1 || metrics->y_ppem < 1 )
      return TT_Err_Invalid_PPem;

    
    
    
    
    if ( face->header.Flags & 8 )
    {
      metrics->x_scale = FT_DivFix( metrics->x_ppem << 6,
                                    face->root.units_per_EM );
      metrics->y_scale = FT_DivFix( metrics->y_ppem << 6,
                                    face->root.units_per_EM );

      metrics->ascender =
        FT_PIX_ROUND( FT_MulFix( face->root.ascender, metrics->y_scale ) );
      metrics->descender =
        FT_PIX_ROUND( FT_MulFix( face->root.descender, metrics->y_scale ) );
      metrics->height =
        FT_PIX_ROUND( FT_MulFix( face->root.height, metrics->y_scale ) );
      metrics->max_advance =
        FT_PIX_ROUND( FT_MulFix( face->root.max_advance_width,
                                 metrics->x_scale ) );
    }

    
    if ( metrics->x_ppem >= metrics->y_ppem )
    {
      size->ttmetrics.scale   = metrics->x_scale;
      size->ttmetrics.ppem    = metrics->x_ppem;
      size->ttmetrics.x_ratio = 0x10000L;
      size->ttmetrics.y_ratio = FT_MulDiv( metrics->y_ppem,
                                           0x10000L,
                                           metrics->x_ppem );
    }
    else
    {
      size->ttmetrics.scale   = metrics->y_scale;
      size->ttmetrics.ppem    = metrics->y_ppem;
      size->ttmetrics.x_ratio = FT_MulDiv( metrics->x_ppem,
                                           0x10000L,
                                           metrics->y_ppem );
      size->ttmetrics.y_ratio = 0x10000L;
    }

#ifdef TT_USE_BYTECODE_INTERPRETER
    size->cvt_ready = 0;
#endif 

    if ( !error )
      size->ttmetrics.valid = TRUE;

    return error;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  tt_driver_init( FT_Module  ttdriver )     
  {

#ifdef TT_USE_BYTECODE_INTERPRETER

    TT_Driver  driver = (TT_Driver)ttdriver;


    if ( !TT_New_Context( driver ) )
      return TT_Err_Could_Not_Find_Context;

#else

    FT_UNUSED( ttdriver );

#endif

    return TT_Err_Ok;
  }


  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( void )
  tt_driver_done( FT_Module  ttdriver )     
  {
#ifdef TT_USE_BYTECODE_INTERPRETER
    TT_Driver  driver = (TT_Driver)ttdriver;


    
    if ( driver->context )
    {
      TT_Done_Context( driver->context );
      driver->context = NULL;
    }
#else
    FT_UNUSED( ttdriver );
#endif

  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  tt_slot_init( FT_GlyphSlot  slot )
  {
    return FT_GlyphLoader_CreateExtra( slot->internal->loader );
  }




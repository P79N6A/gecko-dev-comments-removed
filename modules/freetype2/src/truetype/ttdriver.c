


















#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_SFNT_H
#include FT_SERVICE_XFREE86_NAME_H

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include FT_MULTIPLE_MASTERS_H
#include FT_SERVICE_MULTIPLE_MASTERS_H
#endif

#include FT_SERVICE_TRUETYPE_ENGINE_H
#include FT_SERVICE_TRUETYPE_GLYF_H

#include "ttdriver.h"
#include "ttgload.h"
#include "ttpload.h"

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include "ttgxvar.h"
#endif

#include "tterrors.h"

#include "ttpic.h"

  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttdriver


  
  
  
  
  
  
  
  
  
  
  


#undef  PAIR_TAG
#define PAIR_TAG( left, right )  ( ( (FT_ULong)left << 16 ) | \
                                     (FT_ULong)right        )


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_Error
  tt_get_kerning( FT_Face     ttface,          
                  FT_UInt     left_glyph,
                  FT_UInt     right_glyph,
                  FT_Vector*  kerning )
  {
    TT_Face       face = (TT_Face)ttface;
    SFNT_Service  sfnt = (SFNT_Service)face->sfnt;


    kerning->x = 0;
    kerning->y = 0;

    if ( sfnt )
      kerning->x = sfnt->get_kerning( face, left_glyph, right_glyph );

    return 0;
  }


#undef PAIR_TAG


  static FT_Error
  tt_get_advances( FT_Face    ttface,
                   FT_UInt    start,
                   FT_UInt    count,
                   FT_Int32   flags,
                   FT_Fixed  *advances )
  {
    FT_UInt  nn;
    TT_Face  face  = (TT_Face) ttface;
    FT_Bool  check = FT_BOOL(
                       !( flags & FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH ) );


    

    if ( flags & FT_LOAD_VERTICAL_LAYOUT )
    {
      for ( nn = 0; nn < count; nn++ )
      {
        FT_Short   tsb;
        FT_UShort  ah;


        TT_Get_VMetrics( face, start + nn, check, &tsb, &ah );
        advances[nn] = ah;
      }
    }
    else
    {
      for ( nn = 0; nn < count; nn++ )
      {
        FT_Short   lsb;
        FT_UShort  aw;


        TT_Get_HMetrics( face, start + nn, check, &lsb, &aw );
        advances[nn] = aw;
      }
    }

    return TT_Err_Ok;
  }

  
  
  
  
  
  
  
  
  
  
  


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

  static FT_Error
  tt_size_select( FT_Size   size,
                  FT_ULong  strike_index )
  {
    TT_Face   ttface = (TT_Face)size->face;
    TT_Size   ttsize = (TT_Size)size;
    FT_Error  error  = TT_Err_Ok;


    ttsize->strike_index = strike_index;

    if ( FT_IS_SCALABLE( size->face ) )
    {
      
      FT_Select_Metrics( size->face, strike_index );

      tt_size_reset( ttsize );
    }
    else
    {
      SFNT_Service      sfnt    = (SFNT_Service) ttface->sfnt;
      FT_Size_Metrics*  metrics = &size->metrics;


      error = sfnt->load_strike_metrics( ttface, strike_index, metrics );
      if ( error )
        ttsize->strike_index = 0xFFFFFFFFUL;
    }

    return error;
  }

#endif 


  static FT_Error
  tt_size_request( FT_Size          size,
                   FT_Size_Request  req )
  {
    TT_Size   ttsize = (TT_Size)size;
    FT_Error  error  = TT_Err_Ok;


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    if ( FT_HAS_FIXED_SIZES( size->face ) )
    {
      TT_Face       ttface = (TT_Face)size->face;
      SFNT_Service  sfnt   = (SFNT_Service) ttface->sfnt;
      FT_ULong      strike_index;


      error = sfnt->set_sbit_strike( ttface, req, &strike_index );

      if ( error )
        ttsize->strike_index = 0xFFFFFFFFUL;
      else
        return tt_size_select( size, strike_index );
    }

#endif 

    FT_Request_Metrics( size->face, req );

    if ( FT_IS_SCALABLE( size->face ) )
      error = tt_size_reset( ttsize );

    return error;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_Error
  Load_Glyph( FT_GlyphSlot  ttslot,         
              FT_Size       ttsize,         
              FT_UInt       glyph_index,
              FT_Int32      load_flags )
  {
    TT_GlyphSlot  slot = (TT_GlyphSlot)ttslot;
    TT_Size       size = (TT_Size)ttsize;
    FT_Face       face = ttslot->face;
    FT_Error      error;


    if ( !slot )
      return TT_Err_Invalid_Slot_Handle;

    if ( !size )
      return TT_Err_Invalid_Size_Handle;

    if ( !face )
      return TT_Err_Invalid_Argument;

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    if ( glyph_index >= (FT_UInt)face->num_glyphs &&
         !face->internal->incremental_interface   )
#else
    if ( glyph_index >= (FT_UInt)face->num_glyphs )
#endif
      return TT_Err_Invalid_Argument;

    if ( load_flags & FT_LOAD_NO_HINTING )
    {
      
                

      if ( FT_IS_TRICKY( face ) )
        load_flags &= ~FT_LOAD_NO_HINTING;

      if ( load_flags & FT_LOAD_NO_AUTOHINT )
        load_flags |= FT_LOAD_NO_HINTING;
    }

    if ( load_flags & ( FT_LOAD_NO_RECURSE | FT_LOAD_NO_SCALE ) )
    {
      load_flags |= FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE;

      if ( !FT_IS_TRICKY( face ) )
        load_flags |= FT_LOAD_NO_HINTING;
    }

    
    error = TT_Load_Glyph( size, slot, glyph_index, load_flags );

    
    

    return error;
  }


  
  
  
  
  
  
  
  
  
  
  

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
  FT_DEFINE_SERVICE_MULTIMASTERSREC(tt_service_gx_multi_masters,
    (FT_Get_MM_Func)        NULL,
    (FT_Set_MM_Design_Func) NULL,
    (FT_Set_MM_Blend_Func)  TT_Set_MM_Blend,
    (FT_Get_MM_Var_Func)    TT_Get_MM_Var,
    (FT_Set_Var_Design_Func)TT_Set_Var_Design
  )
#endif

  static const FT_Service_TrueTypeEngineRec  tt_service_truetype_engine =
  {
#ifdef TT_USE_BYTECODE_INTERPRETER

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    FT_TRUETYPE_ENGINE_TYPE_UNPATENTED
#else
    FT_TRUETYPE_ENGINE_TYPE_PATENTED
#endif

#else 

    FT_TRUETYPE_ENGINE_TYPE_NONE

#endif 
  };

  FT_DEFINE_SERVICE_TTGLYFREC(tt_service_truetype_glyf,
    (TT_Glyf_GetLocationFunc)tt_face_get_location
  )

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
  FT_DEFINE_SERVICEDESCREC4(tt_services,
    FT_SERVICE_ID_XF86_NAME,       FT_XF86_FORMAT_TRUETYPE,
    FT_SERVICE_ID_MULTI_MASTERS,   &FT_TT_SERVICE_GX_MULTI_MASTERS_GET,
    FT_SERVICE_ID_TRUETYPE_ENGINE, &tt_service_truetype_engine,
    FT_SERVICE_ID_TT_GLYF,         &FT_TT_SERVICE_TRUETYPE_GLYF_GET
  )
#else
  FT_DEFINE_SERVICEDESCREC3(tt_services,
    FT_SERVICE_ID_XF86_NAME,       FT_XF86_FORMAT_TRUETYPE,
    FT_SERVICE_ID_TRUETYPE_ENGINE, &tt_service_truetype_engine,
    FT_SERVICE_ID_TT_GLYF,         &FT_TT_SERVICE_TRUETYPE_GLYF_GET
  )
#endif

  FT_CALLBACK_DEF( FT_Module_Interface )
  tt_get_interface( FT_Module    driver,    
                    const char*  tt_interface )
  {
    FT_Module_Interface  result;
    FT_Module            sfntd;
    SFNT_Service         sfnt;

    result = ft_service_list_lookup( FT_TT_SERVICES_GET, tt_interface );
    if ( result != NULL )
      return result;

    if ( !driver )
      return NULL;

    
    sfntd = FT_Get_Module( driver->library, "sfnt" );
    if ( sfntd )
    {
      sfnt = (SFNT_Service)( sfntd->clazz->module_interface );
      if ( sfnt )
        return sfnt->get_interface( driver, tt_interface );
    }

    return 0;
  }


  

#ifdef TT_USE_BYTECODE_INTERPRETER
#define TT_HINTER_FLAG   FT_MODULE_DRIVER_HAS_HINTER
#else
#define TT_HINTER_FLAG   0
#endif

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
#define TT_SIZE_SELECT    tt_size_select
#else
#define TT_SIZE_SELECT    0
#endif

  FT_DEFINE_DRIVER(tt_driver_class,
  
    
      FT_MODULE_FONT_DRIVER        |
      FT_MODULE_DRIVER_SCALABLE    |
      TT_HINTER_FLAG,

      sizeof ( TT_DriverRec ),

      "truetype",      
      0x10000L,        
      0x20000L,        

      (void*)0,        

      tt_driver_init,
      tt_driver_done,
      tt_get_interface,

    sizeof ( TT_FaceRec ),
    sizeof ( TT_SizeRec ),
    sizeof ( FT_GlyphSlotRec ),

    tt_face_init,
    tt_face_done,
    tt_size_init,
    tt_size_done,
    tt_slot_init,
    0,                      

    ft_stub_set_char_sizes, 
    ft_stub_set_pixel_sizes, 

    Load_Glyph,

    tt_get_kerning,
    0,                      
    tt_get_advances,

    tt_size_request,
    TT_SIZE_SELECT
  )




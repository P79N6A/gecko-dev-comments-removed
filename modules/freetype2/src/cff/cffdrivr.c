

















#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_SFNT_H
#include FT_TRUETYPE_IDS_H
#include FT_SERVICE_CID_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include FT_SERVICE_POSTSCRIPT_INFO_H
#include FT_SERVICE_POSTSCRIPT_NAME_H
#include FT_SERVICE_TT_CMAP_H

#include "cffdrivr.h"
#include "cffgload.h"
#include "cffload.h"
#include "cffcmap.h"

#include "cfferrs.h"

#include FT_SERVICE_XFREE86_NAME_H
#include FT_SERVICE_GLYPH_DICT_H


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffdriver


  
  
  
  
  
  
  
  
  
  
  


#undef  PAIR_TAG
#define PAIR_TAG( left, right )  ( ( (FT_ULong)left << 16 ) | \
                                     (FT_ULong)right        )


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_CALLBACK_DEF( FT_Error )
  cff_get_kerning( FT_Face     ttface,          
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

    return CFF_Err_Ok;
  }


#undef PAIR_TAG


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_CALLBACK_DEF( FT_Error )
  Load_Glyph( FT_GlyphSlot  cffslot,        
              FT_Size       cffsize,        
              FT_UInt       glyph_index,
              FT_Int32      load_flags )
  {
    FT_Error       error;
    CFF_GlyphSlot  slot = (CFF_GlyphSlot)cffslot;
    CFF_Size       size = (CFF_Size)cffsize;


    if ( !slot )
      return CFF_Err_Invalid_Slot_Handle;

    
    if ( !size )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    
    if ( load_flags & FT_LOAD_NO_SCALE )
      size = NULL;

    if ( size )
    {
      
      if ( cffsize->face != cffslot->face )
        return CFF_Err_Invalid_Face_Handle;
    }

    
    error = cff_slot_load( slot, size, glyph_index, load_flags );

    
    

    return error;
  }


  




  static FT_Error
  cff_get_glyph_name( CFF_Face    face,
                      FT_UInt     glyph_index,
                      FT_Pointer  buffer,
                      FT_UInt     buffer_max )
  {
    CFF_Font            font   = (CFF_Font)face->extra.data;
    FT_Memory           memory = FT_FACE_MEMORY( face );
    FT_String*          gname;
    FT_UShort           sid;
    FT_Service_PsCMaps  psnames;
    FT_Error            error;


    FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );
    if ( !psnames )
    {
      FT_ERROR(( "cff_get_glyph_name:" ));
      FT_ERROR(( " cannot get glyph name from CFF & CEF fonts\n" ));
      FT_ERROR(( "                   " ));
      FT_ERROR(( " without the `PSNames' module\n" ));
      error = CFF_Err_Unknown_File_Format;
      goto Exit;
    }

    
    sid = font->charset.sids[glyph_index];

    
    gname = cff_index_get_sid_string( &font->string_index, sid, psnames );

    if ( gname )
      FT_STRCPYN( buffer, gname, buffer_max );

    FT_FREE( gname );
    error = CFF_Err_Ok;

  Exit:
    return error;
  }


  static FT_UInt
  cff_get_name_index( CFF_Face    face,
                      FT_String*  glyph_name )
  {
    CFF_Font            cff;
    CFF_Charset         charset;
    FT_Service_PsCMaps  psnames;
    FT_Memory           memory = FT_FACE_MEMORY( face );
    FT_String*          name;
    FT_UShort           sid;
    FT_UInt             i;
    FT_Int              result;


    cff     = (CFF_FontRec *)face->extra.data;
    charset = &cff->charset;

    FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );
    if ( !psnames )
      return 0;

    for ( i = 0; i < cff->num_glyphs; i++ )
    {
      sid = charset->sids[i];

      if ( sid > 390 )
        name = cff_index_get_name( &cff->string_index, sid - 391 );
      else
        name = (FT_String *)psnames->adobe_std_strings( sid );

      if ( !name )
        continue;

      result = ft_strcmp( glyph_name, name );

      if ( sid > 390 )
        FT_FREE( name );

      if ( !result )
        return i;
    }

    return 0;
  }


  static const FT_Service_GlyphDictRec  cff_service_glyph_dict =
  {
    (FT_GlyphDict_GetNameFunc)  cff_get_glyph_name,
    (FT_GlyphDict_NameIndexFunc)cff_get_name_index,
  };


  




  static FT_Int
  cff_ps_has_glyph_names( FT_Face  face )
  {
    return ( face->face_flags & FT_FACE_FLAG_GLYPH_NAMES ) > 0;
  }


  static FT_Error
  cff_ps_get_font_info( CFF_Face         face,
                        PS_FontInfoRec*  afont_info )
  {
    CFF_Font  cff   = (CFF_Font)face->extra.data;
    FT_Error  error = FT_Err_Ok;


    if ( cff && cff->font_info == NULL )
    {
      CFF_FontRecDict     dict    = &cff->top_font.font_dict;
      PS_FontInfoRec     *font_info;
      FT_Memory           memory  = face->root.memory;
      FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)cff->psnames;


      if ( FT_ALLOC( font_info, sizeof ( *font_info ) ) )
        goto Fail;

      font_info->version     = cff_index_get_sid_string( &cff->string_index,
                                                         dict->version,
                                                         psnames );
      font_info->notice      = cff_index_get_sid_string( &cff->string_index,
                                                         dict->notice,
                                                         psnames );
      font_info->full_name   = cff_index_get_sid_string( &cff->string_index,
                                                         dict->full_name,
                                                         psnames );
      font_info->family_name = cff_index_get_sid_string( &cff->string_index,
                                                         dict->family_name,
                                                         psnames );
      font_info->weight      = cff_index_get_sid_string( &cff->string_index,
                                                         dict->weight,
                                                         psnames );
      font_info->italic_angle        = dict->italic_angle;
      font_info->is_fixed_pitch      = dict->is_fixed_pitch;
      font_info->underline_position  = (FT_Short)dict->underline_position;
      font_info->underline_thickness = (FT_Short)dict->underline_thickness;

      cff->font_info = font_info;
    }

    *afont_info = *cff->font_info;

  Fail:
    return error;
  }


  static const FT_Service_PsInfoRec  cff_service_ps_info =
  {
    (PS_GetFontInfoFunc)   cff_ps_get_font_info,
    (PS_HasGlyphNamesFunc) cff_ps_has_glyph_names,
    (PS_GetFontPrivateFunc)NULL         
  };


  




  static const char*
  cff_get_ps_name( CFF_Face  face )
  {
    CFF_Font  cff = (CFF_Font)face->extra.data;


    return (const char*)cff->font_name;
  }


  static const FT_Service_PsFontNameRec  cff_service_ps_name =
  {
    (FT_PsName_GetFunc)cff_get_ps_name
  };


  









  static FT_Error
  cff_get_cmap_info( FT_CharMap    charmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_CMap   cmap  = FT_CMAP( charmap );
    FT_Error  error = CFF_Err_Ok;


    cmap_info->language = 0;

    if ( cmap->clazz != &cff_cmap_encoding_class_rec &&
         cmap->clazz != &cff_cmap_unicode_class_rec  )
    {
      FT_Face             face    = FT_CMAP_FACE( cmap );
      FT_Library          library = FT_FACE_LIBRARY( face );
      FT_Module           sfnt    = FT_Get_Module( library, "sfnt" );
      FT_Service_TTCMaps  service =
        (FT_Service_TTCMaps)ft_module_get_service( sfnt,
                                                   FT_SERVICE_ID_TT_CMAP );


      if ( service && service->get_cmap_info )
        error = service->get_cmap_info( charmap, cmap_info );
    }

    return error;
  }


  static const FT_Service_TTCMapsRec  cff_service_get_cmap_info =
  {
    (TT_CMap_Info_GetFunc)cff_get_cmap_info
  };


  



  static FT_Error
  cff_get_ros( CFF_Face      face,
               const char*  *registry,
               const char*  *ordering,
               FT_Int       *supplement )
  {
    FT_Error  error = CFF_Err_Ok;
    CFF_Font  cff   = (CFF_Font)face->extra.data;


    if ( cff )
    {
      CFF_FontRecDict     dict    = &cff->top_font.font_dict;
      FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)cff->psnames;


      if ( dict->cid_registry == 0xFFFFU )
      {
        error = CFF_Err_Invalid_Argument;
        goto Fail;
      }

      if ( registry )
      {
        if ( cff->registry == NULL )
          cff->registry = cff_index_get_sid_string( &cff->string_index,
                                                    dict->cid_registry,
                                                    psnames );
        *registry = cff->registry;
      }
      
      if ( ordering )
      {
        if ( cff->ordering == NULL )
          cff->ordering = cff_index_get_sid_string( &cff->string_index,
                                                    dict->cid_ordering,
                                                    psnames );
        *ordering = cff->ordering;
      }

      if ( supplement )
        *supplement = dict->cid_supplement;
    }
      
  Fail:
    return error;
  }


  static const FT_Service_CIDRec  cff_service_cid_info =
  {
    (FT_CID_GetRegistryOrderingSupplementFunc)cff_get_ros
  };


  
  
  
  
  
  
  
  
  
  
  

  static const FT_ServiceDescRec  cff_services[] =
  {
    { FT_SERVICE_ID_XF86_NAME,            FT_XF86_FORMAT_CFF },
    { FT_SERVICE_ID_POSTSCRIPT_INFO,      &cff_service_ps_info },
    { FT_SERVICE_ID_POSTSCRIPT_FONT_NAME, &cff_service_ps_name },
#ifndef FT_CONFIG_OPTION_NO_GLYPH_NAMES
    { FT_SERVICE_ID_GLYPH_DICT,           &cff_service_glyph_dict },
#endif
    { FT_SERVICE_ID_TT_CMAP,              &cff_service_get_cmap_info },
    { FT_SERVICE_ID_CID,                  &cff_service_cid_info },
    { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  cff_get_interface( FT_Module    driver,       
                     const char*  module_interface )
  {
    FT_Module            sfnt;
    FT_Module_Interface  result;


    result = ft_service_list_lookup( cff_services, module_interface );
    if ( result != NULL )
      return  result;

    
    sfnt = FT_Get_Module( driver->library, "sfnt" );

    return sfnt ? sfnt->clazz->get_interface( sfnt, module_interface ) : 0;
  }


  

  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  cff_driver_class =
  {
    
    {
      FT_MODULE_FONT_DRIVER       |
      FT_MODULE_DRIVER_SCALABLE   |
      FT_MODULE_DRIVER_HAS_HINTER,

      sizeof( CFF_DriverRec ),
      "cff",
      0x10000L,
      0x20000L,

      0,   

      cff_driver_init,
      cff_driver_done,
      cff_get_interface,
    },

    
    sizeof( TT_FaceRec ),
    sizeof( CFF_SizeRec ),
    sizeof( CFF_GlyphSlotRec ),

    cff_face_init,
    cff_face_done,
    cff_size_init,
    cff_size_done,
    cff_slot_init,
    cff_slot_done,

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    ft_stub_set_char_sizes,
    ft_stub_set_pixel_sizes,
#endif

    Load_Glyph,

    cff_get_kerning,
    0,                      
    0,                      

    cff_size_request,

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
    cff_size_select
#else
    0                       
#endif
  };




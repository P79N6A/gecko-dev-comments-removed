

















#include <ft2build.h>
#include "cidriver.h"
#include "cidgload.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H

#include "ciderrs.h"

#include FT_SERVICE_POSTSCRIPT_NAME_H
#include FT_SERVICE_XFREE86_NAME_H
#include FT_SERVICE_POSTSCRIPT_INFO_H
#include FT_SERVICE_CID_H


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ciddriver


  




  static const char*
  cid_get_postscript_name( CID_Face  face )
  {
    const char*  result = face->cid.cid_font_name;


    if ( result && result[0] == '/' )
      result++;

    return result;
  }


  static const FT_Service_PsFontNameRec  cid_service_ps_name =
  {
    (FT_PsName_GetFunc) cid_get_postscript_name
  };


  




  static FT_Error
  cid_ps_get_font_info( FT_Face          face,
                        PS_FontInfoRec*  afont_info )
  {
    *afont_info = ((CID_Face)face)->cid.font_info;
    return 0;
  }


  static const FT_Service_PsInfoRec  cid_service_ps_info =
  {
    (PS_GetFontInfoFunc)   cid_ps_get_font_info,
    (PS_HasGlyphNamesFunc) NULL,        
    (PS_GetFontPrivateFunc)NULL         
  };


  



  static FT_Error
  cid_get_ros( CID_Face      face,
               const char*  *registry,
               const char*  *ordering,
               FT_Int       *supplement )
  {
    CID_FaceInfo  cid = &face->cid;


    if ( registry )
      *registry = cid->registry;
      
    if ( ordering )
      *ordering = cid->ordering;

    if ( supplement )
      *supplement = cid->supplement;
      
    return CID_Err_Ok;
  }


  static const FT_Service_CIDRec  cid_service_cid_info =
  {
    (FT_CID_GetRegistryOrderingSupplementFunc)cid_get_ros
  };


  




  static const FT_ServiceDescRec  cid_services[] =
  {
    { FT_SERVICE_ID_XF86_NAME,            FT_XF86_FORMAT_CID },
    { FT_SERVICE_ID_POSTSCRIPT_FONT_NAME, &cid_service_ps_name },
    { FT_SERVICE_ID_POSTSCRIPT_INFO,      &cid_service_ps_info },
    { FT_SERVICE_ID_CID,                  &cid_service_cid_info },
    { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  cid_get_interface( FT_Module    module,
                     const char*  cid_interface )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( cid_services, cid_interface );
  }



  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  t1cid_driver_class =
  {
    
    {
      FT_MODULE_FONT_DRIVER       |
      FT_MODULE_DRIVER_SCALABLE   |
      FT_MODULE_DRIVER_HAS_HINTER,

      sizeof( FT_DriverRec ),
      "t1cid",   
      0x10000L,  
      0x20000L,  

      0,

      cid_driver_init,
      cid_driver_done,
      cid_get_interface
    },

    
    sizeof( CID_FaceRec ),
    sizeof( CID_SizeRec ),
    sizeof( CID_GlyphSlotRec ),

    cid_face_init,
    cid_face_done,

    cid_size_init,
    cid_size_done,
    cid_slot_init,
    cid_slot_done,

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    ft_stub_set_char_sizes,
    ft_stub_set_pixel_sizes,
#endif

    cid_slot_load_glyph,

    0,                      
    0,                      

    0,                      

    cid_size_request,
    0                       
  };




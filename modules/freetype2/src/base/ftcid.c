
















#include <ft2build.h>
#include FT_CID_H
#include FT_INTERNAL_OBJECTS_H
#include FT_SERVICE_CID_H


  

  FT_EXPORT_DEF( FT_Error )
  FT_Get_CID_Registry_Ordering_Supplement( FT_Face       face,
                                           const char*  *registry,
                                           const char*  *ordering,
                                           FT_Int       *supplement)
  {
    FT_Error     error;
    const char*  r = NULL;
    const char*  o = NULL;
    FT_Int       s = 0;


    error = FT_Err_Invalid_Argument;

    if ( face )
    {
      FT_Service_CID  service;


      FT_FACE_FIND_SERVICE( face, service, CID );

      if ( service && service->get_ros )
        error = service->get_ros( face, &r, &o, &s );
    }

    if ( registry )
      *registry = r;

    if ( ordering )
      *ordering = o;

    if ( supplement )
      *supplement = s;

    return error;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Get_CID_Is_Internally_CID_Keyed( FT_Face   face,
                                      FT_Bool  *is_cid )
  {
    FT_Error  error = FT_Err_Invalid_Argument;
    FT_Bool   ic = 0;


    if ( face )
    {
      FT_Service_CID  service;


      FT_FACE_FIND_SERVICE( face, service, CID );

      if ( service && service->get_is_cid )
        error = service->get_is_cid( face, &ic);
    }

    if ( is_cid )
      *is_cid = ic;

    return error;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Get_CID_From_Glyph_Index( FT_Face   face,
                               FT_UInt   glyph_index,
                               FT_UInt  *cid )
  {
    FT_Error  error = FT_Err_Invalid_Argument;
    FT_UInt   c = 0;


    if ( face )
    {
      FT_Service_CID  service;


      FT_FACE_FIND_SERVICE( face, service, CID );

      if ( service && service->get_cid_from_glyph_index )
        error = service->get_cid_from_glyph_index( face, glyph_index, &c);
    }

    if ( cid )
      *cid = c;

    return error;
  }






















#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H

#include FT_INTERNAL_OBJECTS_H
#include FT_SERVICE_BDF_H


  

  FT_EXPORT_DEF( FT_Error )
  FT_Get_BDF_Charset_ID( FT_Face       face,
                         const char*  *acharset_encoding,
                         const char*  *acharset_registry )
  {
    FT_Error     error;
    const char*  encoding = NULL;
    const char*  registry = NULL;

    FT_Service_BDF  service;


    if ( !face )
      return FT_THROW( Invalid_Face_Handle );

    FT_FACE_FIND_SERVICE( face, service, BDF );

    if ( service && service->get_charset_id )
      error = service->get_charset_id( face, &encoding, &registry );
    else
      error = FT_THROW( Invalid_Argument );

    if ( acharset_encoding )
      *acharset_encoding = encoding;

    if ( acharset_registry )
      *acharset_registry = registry;

    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Get_BDF_Property( FT_Face           face,
                       const char*       prop_name,
                       BDF_PropertyRec  *aproperty )
  {
    FT_Error  error;

    FT_Service_BDF  service;


    if ( !face )
      return FT_THROW( Invalid_Face_Handle );

    if ( !aproperty )
      return FT_THROW( Invalid_Argument );

    aproperty->type = BDF_PROPERTY_TYPE_NONE;

    FT_FACE_FIND_SERVICE( face, service, BDF );

    if ( service && service->get_property )
      error = service->get_property( face, prop_name, aproperty );
    else
      error = FT_THROW( Invalid_Argument );

    return error;
  }




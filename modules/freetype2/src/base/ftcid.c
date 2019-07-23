
















#include <ft2build.h>
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




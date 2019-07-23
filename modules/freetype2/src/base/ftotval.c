
















#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_SERVICE_OPENTYPE_VALIDATE_H


  

  FT_EXPORT_DEF( FT_Error )
  FT_OpenType_Validate( FT_Face    face,
                        FT_UInt    validation_flags,
                        FT_Bytes  *BASE_table,
                        FT_Bytes  *GDEF_table,
                        FT_Bytes  *GPOS_table,
                        FT_Bytes  *GSUB_table,
                        FT_Bytes  *JSTF_table )
  {
    FT_Service_OTvalidate  service;
    FT_Error               error;


    if ( !face )
    {
      error = FT_Err_Invalid_Face_Handle;
      goto Exit;
    }

    if ( !( BASE_table &&
            GDEF_table &&
            GPOS_table &&
            GSUB_table &&
            JSTF_table ) )
    {
      error = FT_Err_Invalid_Argument;
      goto Exit;
    }

    FT_FACE_FIND_GLOBAL_SERVICE( face, service, OPENTYPE_VALIDATE );

    if ( service )
      error = service->validate( face,
                                 validation_flags,
                                 BASE_table,
                                 GDEF_table,
                                 GPOS_table,
                                 GSUB_table,
                                 JSTF_table );
    else
      error = FT_Err_Unimplemented_Feature;

  Exit:
    return error;
  }


  FT_EXPORT_DEF( void )
  FT_OpenType_Free( FT_Face   face,
                    FT_Bytes  table )
  {
    FT_Memory  memory = FT_FACE_MEMORY( face );


    FT_FREE( table );
  }




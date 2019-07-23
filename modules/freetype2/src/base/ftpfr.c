
















#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_SERVICE_PFR_H


  
  static FT_Service_PfrMetrics
  ft_pfr_check( FT_Face  face )
  {
    FT_Service_PfrMetrics  service;


    FT_FACE_LOOKUP_SERVICE( face, service, PFR_METRICS );

    return service;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Get_PFR_Metrics( FT_Face    face,
                      FT_UInt   *aoutline_resolution,
                      FT_UInt   *ametrics_resolution,
                      FT_Fixed  *ametrics_x_scale,
                      FT_Fixed  *ametrics_y_scale )
  {
    FT_Error               error = FT_Err_Ok;
    FT_Service_PfrMetrics  service;


    service = ft_pfr_check( face );
    if ( service )
    {
      error = service->get_metrics( face,
                                    aoutline_resolution,
                                    ametrics_resolution,
                                    ametrics_x_scale,
                                    ametrics_y_scale );
    }
    else if ( face )
    {
      FT_Fixed  x_scale, y_scale;


      
      *aoutline_resolution = face->units_per_EM;
      *ametrics_resolution = face->units_per_EM;

      x_scale = y_scale = 0x10000L;
      if ( face->size )
      {
        x_scale = face->size->metrics.x_scale;
        y_scale = face->size->metrics.y_scale;
      }
      *ametrics_x_scale = x_scale;
      *ametrics_y_scale = y_scale;
    }
    else
      error = FT_Err_Invalid_Argument;

    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Get_PFR_Kerning( FT_Face     face,
                      FT_UInt     left,
                      FT_UInt     right,
                      FT_Vector  *avector )
  {
    FT_Error               error;
    FT_Service_PfrMetrics  service;


    service = ft_pfr_check( face );
    if ( service )
      error = service->get_kerning( face, left, right, avector );
    else if ( face )
      error = FT_Get_Kerning( face, left, right,
                              FT_KERNING_UNSCALED, avector );
    else
      error = FT_Err_Invalid_Argument;

    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Get_PFR_Advance( FT_Face   face,
                      FT_UInt   gindex,
                      FT_Pos   *aadvance )
  {
    FT_Error               error;
    FT_Service_PfrMetrics  service;


    service = ft_pfr_check( face );
    if ( service )
    {
      error = service->get_advance( face, gindex, aadvance );
    }
    else
      
      error = FT_Err_Invalid_Argument;

    return error;
  }






















#include <ft2build.h>
#include FT_MULTIPLE_MASTERS_H
#include FT_INTERNAL_OBJECTS_H
#include FT_SERVICE_MULTIPLE_MASTERS_H


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_mm


  static FT_Error
  ft_face_get_mm_service( FT_Face                   face,
                          FT_Service_MultiMasters  *aservice )
  {
    FT_Error  error;


    *aservice = NULL;

    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    error = FT_Err_Invalid_Argument;

    if ( FT_HAS_MULTIPLE_MASTERS( face ) )
    {
      FT_FACE_LOOKUP_SERVICE( face,
                              *aservice,
                              MULTI_MASTERS );

      if ( aservice )
        error = FT_Err_Ok;
    }

    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Get_Multi_Master( FT_Face           face,
                       FT_Multi_Master  *amaster )
  {
    FT_Error                 error;
    FT_Service_MultiMasters  service;


    error = ft_face_get_mm_service( face, &service );
    if ( !error )
    {
      error = FT_Err_Invalid_Argument;
      if ( service->get_mm )
        error = service->get_mm( face, amaster );
    }

    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Get_MM_Var( FT_Face      face,
                 FT_MM_Var*  *amaster )
  {
    FT_Error                 error;
    FT_Service_MultiMasters  service;


    error = ft_face_get_mm_service( face, &service );
    if ( !error )
    {
      error = FT_Err_Invalid_Argument;
      if ( service->get_mm_var )
        error = service->get_mm_var( face, amaster );
    }

    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Set_MM_Design_Coordinates( FT_Face   face,
                                FT_UInt   num_coords,
                                FT_Long*  coords )
  {
    FT_Error                 error;
    FT_Service_MultiMasters  service;


    error = ft_face_get_mm_service( face, &service );
    if ( !error )
    {
      error = FT_Err_Invalid_Argument;
      if ( service->set_mm_design )
        error = service->set_mm_design( face, num_coords, coords );
    }

    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Set_Var_Design_Coordinates( FT_Face    face,
                                 FT_UInt    num_coords,
                                 FT_Fixed*  coords )
  {
    FT_Error                 error;
    FT_Service_MultiMasters  service;


    error = ft_face_get_mm_service( face, &service );
    if ( !error )
    {
      error = FT_Err_Invalid_Argument;
      if ( service->set_var_design )
        error = service->set_var_design( face, num_coords, coords );
    }

    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Set_MM_Blend_Coordinates( FT_Face    face,
                               FT_UInt    num_coords,
                               FT_Fixed*  coords )
  {
    FT_Error                 error;
    FT_Service_MultiMasters  service;


    error = ft_face_get_mm_service( face, &service );
    if ( !error )
    {
      error = FT_Err_Invalid_Argument;
      if ( service->set_mm_blend )
         error = service->set_mm_blend( face, num_coords, coords );
    }

    return error;
  }


  

  
  

  FT_EXPORT_DEF( FT_Error )
  FT_Set_Var_Blend_Coordinates( FT_Face    face,
                                FT_UInt    num_coords,
                                FT_Fixed*  coords )
  {
    FT_Error                 error;
    FT_Service_MultiMasters  service;


    error = ft_face_get_mm_service( face, &service );
    if ( !error )
    {
      error = FT_Err_Invalid_Argument;
      if ( service->set_mm_blend )
         error = service->set_mm_blend( face, num_coords, coords );
    }

    return error;
  }




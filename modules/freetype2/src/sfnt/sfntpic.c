

















#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include "sfntpic.h"
#include "sferrors.h"

#ifdef FT_CONFIG_OPTION_PIC

  
  FT_Error
  FT_Create_Class_sfnt_services( FT_Library           library,
                                 FT_ServiceDescRec**  ouput_class );

  void
  FT_Destroy_Class_sfnt_services( FT_Library          library,
                                  FT_ServiceDescRec*  clazz );

  void
  FT_Init_Class_sfnt_service_bdf( FT_Service_BDFRec*  clazz );

  void
  FT_Init_Class_sfnt_interface( FT_Library       library,
                                SFNT_Interface*  clazz );

  void
  FT_Init_Class_sfnt_service_glyph_dict(
    FT_Library                library,
    FT_Service_GlyphDictRec*  clazz );

  void
  FT_Init_Class_sfnt_service_ps_name(
    FT_Library                 library,
    FT_Service_PsFontNameRec*  clazz );

  void
  FT_Init_Class_tt_service_get_cmap_info(
    FT_Library              library,
    FT_Service_TTCMapsRec*  clazz );

  void
  FT_Init_Class_sfnt_service_sfnt_table(
    FT_Service_SFNT_TableRec*  clazz );

  
  FT_Error
  FT_Create_Class_tt_cmap_classes( FT_Library       library,
                                   TT_CMap_Class**  output_class );

  void
  FT_Destroy_Class_tt_cmap_classes( FT_Library      library,
                                    TT_CMap_Class*  clazz );

  void
  sfnt_module_class_pic_free( FT_Library  library )
  {
    FT_PIC_Container*  pic_container = &library->pic_container;
    FT_Memory  memory = library->memory;


    if ( pic_container->sfnt )
    {
      sfntModulePIC*  container = (sfntModulePIC*)pic_container->sfnt;


      if ( container->sfnt_services )
        FT_Destroy_Class_sfnt_services( library,
                                        container->sfnt_services );
      container->sfnt_services = NULL;
      if ( container->tt_cmap_classes )
        FT_Destroy_Class_tt_cmap_classes( library,
                                          container->tt_cmap_classes );
      container->tt_cmap_classes = NULL;
      FT_FREE( container );
      pic_container->sfnt = NULL;
    }
  }


  FT_Error
  sfnt_module_class_pic_init(  FT_Library library )
  {
    FT_PIC_Container*  pic_container = &library->pic_container;
    FT_Error           error = SFNT_Err_Ok;
    sfntModulePIC*     container;
    FT_Memory          memory = library->memory;


    
    if ( FT_ALLOC ( container, sizeof ( *container ) ) )
      return error;
    FT_MEM_SET( container, 0, sizeof ( *container ) );
    pic_container->sfnt = container;

    
    error = FT_Create_Class_sfnt_services( library,
                                           &container->sfnt_services );
    if ( error )
      goto Exit;
    error = FT_Create_Class_tt_cmap_classes( library,
                                             &container->tt_cmap_classes );
    if ( error )
      goto Exit;

    FT_Init_Class_sfnt_service_glyph_dict(
      library, &container->sfnt_service_glyph_dict );
    FT_Init_Class_sfnt_service_ps_name(
      library, &container->sfnt_service_ps_name );
    FT_Init_Class_tt_service_get_cmap_info(
      library, &container->tt_service_get_cmap_info );
    FT_Init_Class_sfnt_service_sfnt_table(
      &container->sfnt_service_sfnt_table );
#ifdef TT_CONFIG_OPTION_BDF
    FT_Init_Class_sfnt_service_bdf( &container->sfnt_service_bdf );
#endif
    FT_Init_Class_sfnt_interface( library, &container->sfnt_interface );

Exit:
    if ( error )
      sfnt_module_class_pic_free( library );
    return error;
  }



#endif 




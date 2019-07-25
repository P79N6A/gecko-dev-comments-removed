

















#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include "ttpic.h"

#ifdef FT_CONFIG_OPTION_PIC

  
  FT_Error FT_Create_Class_tt_services( FT_Library, FT_ServiceDescRec**);
  void FT_Destroy_Class_tt_services( FT_Library, FT_ServiceDescRec*);
  void FT_Init_Class_tt_service_gx_multi_masters(FT_Service_MultiMastersRec*);
  void FT_Init_Class_tt_service_truetype_glyf(FT_Service_TTGlyfRec*);

  void
  tt_driver_class_pic_free(  FT_Library library )
  {
    FT_PIC_Container* pic_container = &library->pic_container;
    FT_Memory memory = library->memory;
    if ( pic_container->truetype )
    {
      TTModulePIC* container = (TTModulePIC*)pic_container->truetype;
      if(container->tt_services)
        FT_Destroy_Class_tt_services(library, container->tt_services);
      container->tt_services = NULL;
      FT_FREE( container );
      pic_container->truetype = NULL;
    }
  }


  FT_Error
  tt_driver_class_pic_init( FT_Library  library )
  {
    FT_PIC_Container*  pic_container = &library->pic_container;
    FT_Error           error         = TT_Err_Ok;
    TTModulePIC*       container;
    FT_Memory          memory        = library->memory;


    
    if ( FT_ALLOC ( container, sizeof ( *container ) ) )
      return error;
    FT_MEM_SET( container, 0, sizeof(*container) );
    pic_container->truetype = container;

    
    error = FT_Create_Class_tt_services(library, &container->tt_services);
    if(error) 
      goto Exit;
#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
    FT_Init_Class_tt_service_gx_multi_masters(&container->tt_service_gx_multi_masters);
#endif
    FT_Init_Class_tt_service_truetype_glyf(&container->tt_service_truetype_glyf);
Exit:
    if(error)
      tt_driver_class_pic_free(library);
    return error;
  }

#endif 




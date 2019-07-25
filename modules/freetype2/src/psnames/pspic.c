

















#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include "pspic.h"

#ifdef FT_CONFIG_OPTION_PIC

  
  FT_Error FT_Create_Class_pscmaps_services( FT_Library, FT_ServiceDescRec**);
  void FT_Destroy_Class_pscmaps_services( FT_Library, FT_ServiceDescRec*);
  void FT_Init_Class_pscmaps_interface( FT_Library, FT_Service_PsCMapsRec*);

  void
  psnames_module_class_pic_free(  FT_Library library )
  {
    FT_PIC_Container* pic_container = &library->pic_container;
    FT_Memory memory = library->memory;
    if ( pic_container->psnames )
    {
      PSModulePIC* container = (PSModulePIC*)pic_container->psnames;
      if(container->pscmaps_services)
        FT_Destroy_Class_pscmaps_services(library, container->pscmaps_services);
      container->pscmaps_services = NULL;
      FT_FREE( container );
      pic_container->psnames = NULL;
    }
  }


  FT_Error
  psnames_module_class_pic_init( FT_Library  library )
  {
    FT_PIC_Container*  pic_container = &library->pic_container;
    FT_Error           error         = PSnames_Err_Ok;
    PSModulePIC*       container;
    FT_Memory          memory        = library->memory;


    
    if ( FT_ALLOC ( container, sizeof ( *container ) ) )
      return error;
    FT_MEM_SET( container, 0, sizeof(*container) );
    pic_container->psnames = container;

    
    error = FT_Create_Class_pscmaps_services(library, &container->pscmaps_services);
    if(error) 
      goto Exit;
    FT_Init_Class_pscmaps_interface(library, &container->pscmaps_interface);
    
Exit:
    if(error)
      psnames_module_class_pic_free(library);
    return error;
  }


#endif 




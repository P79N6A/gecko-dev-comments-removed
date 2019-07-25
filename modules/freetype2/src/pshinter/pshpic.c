

















#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include "pshpic.h"

#ifdef FT_CONFIG_OPTION_PIC

  
  void FT_Init_Class_pshinter_interface( FT_Library, PSHinter_Interface*);

  void
  pshinter_module_class_pic_free( FT_Library library )
  {
    FT_PIC_Container* pic_container = &library->pic_container;
    FT_Memory memory = library->memory;
    if ( pic_container->pshinter )
    {
      FT_FREE( pic_container->pshinter );
      pic_container->pshinter = NULL;
    }
  }


  FT_Error
  pshinter_module_class_pic_init( FT_Library  library )
  {
    FT_PIC_Container*  pic_container = &library->pic_container;
    FT_Error           error         = PSH_Err_Ok;
    PSHinterPIC*       container;
    FT_Memory          memory        = library->memory;


    
    if ( FT_ALLOC ( container, sizeof ( *container ) ) )
      return error;
    FT_MEM_SET( container, 0, sizeof ( *container ) );
    pic_container->pshinter = container;
    
    
    FT_Init_Class_pshinter_interface(library, &container->pshinter_interface);


    if(error)
      pshinter_module_class_pic_free(library);
    return error;
  }


#endif 



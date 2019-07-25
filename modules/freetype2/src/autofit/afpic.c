

















#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include "afpic.h"

#ifdef FT_CONFIG_OPTION_PIC

  
  void FT_Init_Class_af_autofitter_service( FT_Library, FT_AutoHinter_ServiceRec*);

  
#include "aflatin.h"
#include "aflatin2.h"
#include "afcjk.h"
#include "afdummy.h"
#include "afindic.h"

  void
  autofit_module_class_pic_free( FT_Library library )
  {
    FT_PIC_Container* pic_container = &library->pic_container;
    FT_Memory memory = library->memory;
    if ( pic_container->autofit )
    {
      FT_FREE( pic_container->autofit );
      pic_container->autofit = NULL;
    }
  }


  FT_Error
  autofit_module_class_pic_init( FT_Library  library )
  {
    FT_PIC_Container*  pic_container = &library->pic_container;
    FT_UInt            ss;
    FT_Error           error         = AF_Err_Ok;
    AFModulePIC*       container;
    FT_Memory          memory        = library->memory;


    
    if ( FT_ALLOC ( container, sizeof ( *container ) ) )
      return error;
    FT_MEM_SET( container, 0, sizeof ( *container ) );
    pic_container->autofit = container;

    
    for ( ss = 0 ; ss < AF_SCRIPT_CLASSES_REC_COUNT ; ss++ )
    {
      container->af_script_classes[ss] = &container->af_script_classes_rec[ss];
    }
    container->af_script_classes[AF_SCRIPT_CLASSES_COUNT-1] = NULL;
    
    
    ss = 0;
    FT_Init_Class_af_dummy_script_class(&container->af_script_classes_rec[ss++]);
#ifdef FT_OPTION_AUTOFIT2
    FT_Init_Class_af_latin2_script_class(&container->af_script_classes_rec[ss++]);
#endif
    FT_Init_Class_af_latin_script_class(&container->af_script_classes_rec[ss++]);
    FT_Init_Class_af_cjk_script_class(&container->af_script_classes_rec[ss++]);
    FT_Init_Class_af_indic_script_class(&container->af_script_classes_rec[ss++]);    

    FT_Init_Class_af_autofitter_service(library, &container->af_autofitter_service);


    if(error)
      autofit_module_class_pic_free(library);
    return error;
  }


#endif 




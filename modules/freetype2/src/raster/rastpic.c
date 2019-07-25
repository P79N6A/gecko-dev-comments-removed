

















#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include "rastpic.h"

#ifdef FT_CONFIG_OPTION_PIC

  
  void FT_Init_Class_ft_standard_raster(FT_Raster_Funcs*);

  void
  ft_raster1_renderer_class_pic_free(  FT_Library library )
  {
    FT_PIC_Container* pic_container = &library->pic_container;
    FT_Memory memory = library->memory;
    if ( pic_container->raster )
    {
      RasterPIC* container = (RasterPIC*)pic_container->raster;
      if(--container->ref_count)
        return;
      FT_FREE( container );
      pic_container->raster = NULL;
    }
  }


  FT_Error
  ft_raster1_renderer_class_pic_init( FT_Library library )
  {
    FT_PIC_Container* pic_container = &library->pic_container;
    FT_Error          error         = Raster_Err_Ok;
    RasterPIC*        container;
    FT_Memory         memory        = library->memory;


    

    if ( pic_container->raster )
    {
      ((RasterPIC*)pic_container->raster)->ref_count++;
      return error;
    }

    
    if ( FT_ALLOC ( container, sizeof ( *container ) ) )
      return error;
    FT_MEM_SET( container, 0, sizeof(*container) );
    pic_container->raster = container;
    container->ref_count = 1;

    
    FT_Init_Class_ft_standard_raster(&container->ft_standard_raster);

    if(error)
      ft_raster1_renderer_class_pic_free(library);
    return error;
  }

  
  FT_Error ft_raster5_renderer_class_pic_init(FT_Library library)
  {
    return ft_raster1_renderer_class_pic_init(library);
  }
  void ft_raster5_renderer_class_pic_free(FT_Library library)
  {
    ft_raster1_renderer_class_pic_free(library);
  }

#endif 




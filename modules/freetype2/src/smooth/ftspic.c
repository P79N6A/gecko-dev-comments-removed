

















#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include "ftspic.h"

#ifdef FT_CONFIG_OPTION_PIC

  
  void FT_Init_Class_ft_grays_raster(FT_Raster_Funcs*);

  void
  ft_smooth_renderer_class_pic_free(  FT_Library library )
  {
    FT_PIC_Container* pic_container = &library->pic_container;
    FT_Memory memory = library->memory;
    if ( pic_container->smooth )
    {
      SmoothPIC* container = (SmoothPIC*)pic_container->smooth;
      if(--container->ref_count)
        return;
      FT_FREE( container );
      pic_container->smooth = NULL;
    }
  }


  FT_Error
  ft_smooth_renderer_class_pic_init( FT_Library  library )
  {
    FT_PIC_Container*  pic_container = &library->pic_container;
    FT_Error           error         = Smooth_Err_Ok;
    SmoothPIC*         container;
    FT_Memory          memory        = library->memory;


    

    if(pic_container->smooth)
    {
      ((SmoothPIC*)pic_container->smooth)->ref_count++;
      return error;
    }

    
    if ( FT_ALLOC ( container, sizeof ( *container ) ) )
      return error;
    FT_MEM_SET( container, 0, sizeof(*container) );
    pic_container->smooth = container;
    container->ref_count = 1;

    
    FT_Init_Class_ft_grays_raster(&container->ft_grays_raster);

    if(error)
      ft_smooth_renderer_class_pic_free(library);
    return error;
  }

  
  FT_Error ft_smooth_lcd_renderer_class_pic_init(FT_Library library)
  {
    return ft_smooth_renderer_class_pic_init(library);
  }
  void ft_smooth_lcd_renderer_class_pic_free(FT_Library library)
  {
    ft_smooth_renderer_class_pic_free(library);
  }
  FT_Error ft_smooth_lcdv_renderer_class_pic_init(FT_Library library)
  {
    return ft_smooth_renderer_class_pic_init(library);
  }
  void ft_smooth_lcdv_renderer_class_pic_free(FT_Library library)
  {
    ft_smooth_renderer_class_pic_free(library);
  }

#endif 




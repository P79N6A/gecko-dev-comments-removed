

















#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include "basepic.h"

#ifdef FT_CONFIG_OPTION_PIC

  
  void FT_Init_Class_ft_outline_glyph_class( FT_Glyph_Class*  clazz );
  void FT_Init_Class_ft_bitmap_glyph_class( FT_Glyph_Class*  clazz );

  
  void FT_Init_Table_raccess_guess_table( ft_raccess_guess_rec*  record );

  
  FT_Error
  ft_create_default_module_classes( FT_Library  library );

  void
  ft_destroy_default_module_classes( FT_Library  library );

  void
  ft_base_pic_free( FT_Library  library )
  {
    FT_PIC_Container*  pic_container = &library->pic_container;
    FT_Memory          memory = library->memory;
    if ( pic_container->base )
    {
      
      ft_destroy_default_module_classes( library );

      FT_FREE( pic_container->base );
      pic_container->base = NULL;
    }
  }


  FT_Error
  ft_base_pic_init( FT_Library  library )
  {
    FT_PIC_Container*  pic_container = &library->pic_container;
    FT_Error           error = FT_Err_Ok;
    BasePIC*           container;
    FT_Memory          memory = library->memory;

    
    if ( FT_ALLOC ( container, sizeof ( *container ) ) )
      return error;
    FT_MEM_SET( container, 0, sizeof ( *container ) );
    pic_container->base = container;

    
    error = ft_create_default_module_classes( library );
    if ( error )
      goto Exit;

    
    FT_Init_Class_ft_outline_glyph_class(
      &container->ft_outline_glyph_class );
    FT_Init_Class_ft_bitmap_glyph_class(
      &container->ft_bitmap_glyph_class );
    FT_Init_Table_raccess_guess_table(
      (ft_raccess_guess_rec*)&container->ft_raccess_guess_table);

Exit:
    if( error )
      ft_base_pic_free( library );
    return error;
  }


#endif 




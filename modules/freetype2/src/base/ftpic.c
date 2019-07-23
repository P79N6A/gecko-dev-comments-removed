

















#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include "basepic.h"

#ifdef FT_CONFIG_OPTION_PIC

  

  FT_BASE_DEF( FT_Error )
  ft_pic_container_init( FT_Library library )
  {
    FT_PIC_Container* pic_container = &library->pic_container;
    FT_Error error = FT_Err_Ok;

    FT_MEM_SET( pic_container, 0, sizeof(*pic_container) );

    error = ft_base_pic_init( library );
    if(error)
      return error;

    return FT_Err_Ok;
  }


  
  FT_BASE_DEF( void )
  ft_pic_container_destroy( FT_Library library )
  {
    ft_base_pic_free( library );
  }

#endif 




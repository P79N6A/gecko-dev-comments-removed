
















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_MODULE_H


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_init

#undef  FT_USE_MODULE
#ifdef __cplusplus
#define FT_USE_MODULE( x )  extern "C" const FT_Module_Class  x;
#else
#define FT_USE_MODULE( x )  extern const FT_Module_Class  x;
#endif


#include FT_CONFIG_MODULES_H


#undef  FT_USE_MODULE
#define FT_USE_MODULE( x )  (const FT_Module_Class*)&(x),

  static
  const FT_Module_Class*  const ft_default_modules[] =
  {
#include FT_CONFIG_MODULES_H
    0
  };


  

  FT_EXPORT_DEF( void )
  FT_Add_Default_Modules( FT_Library  library )
  {
    FT_Error                       error;
    const FT_Module_Class* const*  cur;


    

    cur = ft_default_modules;
    while ( *cur )
    {
      error = FT_Add_Module( library, *cur );
      
      if ( error )
      {
        FT_ERROR(( "FT_Add_Default_Module: Cannot install `%s', error = 0x%x\n",
                   (*cur)->module_name, error ));
      }
      cur++;
    }
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Init_FreeType( FT_Library  *alibrary )
  {
    FT_Error   error;
    FT_Memory  memory;


    
    

    memory = FT_New_Memory();
    if ( !memory )
    {
      FT_ERROR(( "FT_Init_FreeType: cannot find memory manager\n" ));
      return FT_Err_Unimplemented_Feature;
    }

    
    

    error = FT_New_Library( memory, alibrary );
    if ( error )
      FT_Done_Memory( memory );
    else
    {
      (*alibrary)->version_major = FREETYPE_MAJOR;
      (*alibrary)->version_minor = FREETYPE_MINOR;
      (*alibrary)->version_patch = FREETYPE_PATCH;

      FT_Add_Default_Modules( *alibrary );
    }

    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Done_FreeType( FT_Library  library )
  {
    if ( library )
    {
      FT_Memory  memory = library->memory;


      
      FT_Done_Library( library );

      
      FT_Done_Memory( memory );
    }

    return FT_Err_Ok;
  }





















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_MODULE_H
#include "basepic.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_init

#ifndef FT_CONFIG_OPTION_PIC

#undef  FT_USE_MODULE
#ifdef __cplusplus
#define FT_USE_MODULE( type, x )  extern "C" const type  x;
#else
#define FT_USE_MODULE( type, x )  extern const type  x;
#endif


#include FT_CONFIG_MODULES_H


#undef  FT_USE_MODULE
#define FT_USE_MODULE( type, x )  (const FT_Module_Class*)&(x),

  static
  const FT_Module_Class*  const ft_default_modules[] =
  {
#include FT_CONFIG_MODULES_H
    0
  };

#else 

#ifdef __cplusplus
#define FT_EXTERNC  extern "C"
#else
#define FT_EXTERNC  extern
#endif

  
#undef  FT_USE_MODULE
#define FT_USE_MODULE( type, x )  \
  FT_EXTERNC FT_Error FT_Create_Class_##x( FT_Library library, FT_Module_Class** output_class ); \
  FT_EXTERNC void     FT_Destroy_Class_##x( FT_Library library, FT_Module_Class*  clazz );

#include FT_CONFIG_MODULES_H


  
#undef  FT_USE_MODULE
#define FT_USE_MODULE( type, x )  MODULE_CLASS_##x,

  enum
  {
#include FT_CONFIG_MODULES_H
    FT_NUM_MODULE_CLASSES
  };

    
#undef  FT_USE_MODULE
#define FT_USE_MODULE( type, x )  \
  if ( classes[i] ) { FT_Destroy_Class_##x(library, classes[i]); } \
  i++;                                                             \

  FT_BASE_DEF( void )
  ft_destroy_default_module_classes( FT_Library  library )
  {
    FT_Module_Class** classes;
    FT_Memory         memory;
    FT_UInt           i;
    BasePIC*          pic_container = (BasePIC*)library->pic_container.base;

    if ( !pic_container->default_module_classes )
      return;

    memory = library->memory;
    classes = pic_container->default_module_classes;
    i = 0;

#include FT_CONFIG_MODULES_H

    FT_FREE( classes );
    pic_container->default_module_classes = 0;
  }

  
#undef  FT_USE_MODULE
#define FT_USE_MODULE( type, x )                \
  error = FT_Create_Class_##x(library, &clazz); \
  if (error) goto Exit;                         \
  classes[i++] = clazz;

  FT_BASE_DEF( FT_Error )
  ft_create_default_module_classes( FT_Library  library )
  {
    FT_Error          error;
    FT_Memory         memory;
    FT_Module_Class** classes;
    FT_Module_Class*  clazz;
    FT_UInt           i;
    BasePIC*          pic_container = (BasePIC*)library->pic_container.base;

    memory = library->memory;  
    pic_container->default_module_classes = 0;

    if ( FT_ALLOC(classes, sizeof(FT_Module_Class*) * (FT_NUM_MODULE_CLASSES + 1) ) )
      return error;
    
    for (i = 0; i < FT_NUM_MODULE_CLASSES; i++)
      classes[i] = 0;
    classes[FT_NUM_MODULE_CLASSES] = 0;

    i = 0;

#include FT_CONFIG_MODULES_H

Exit:    
    if (error) ft_destroy_default_module_classes( library );
    else pic_container->default_module_classes = classes;

    return error;    
  }


#endif 

  

  FT_EXPORT_DEF( void )
  FT_Add_Default_Modules( FT_Library  library )
  {
    FT_Error                       error;
    const FT_Module_Class* const*  cur;


    

    cur = FT_DEFAULT_MODULES_GET;
    while ( *cur )
    {
      error = FT_Add_Module( library, *cur );
      
      if ( error )
        FT_TRACE0(( "FT_Add_Default_Module:"
                    " Cannot install `%s', error = 0x%x\n",
                    (*cur)->module_name, error ));
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
      FT_Add_Default_Modules( *alibrary );

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




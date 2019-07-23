

















#ifndef __FTMODAPI_H__
#define __FTMODAPI_H__


#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef FREETYPE_H
#error "freetype.h of FreeType 1 has been loaded!"
#error "Please fix the directory search order for header files"
#error "so that freetype.h of FreeType 2 is found first."
#endif


FT_BEGIN_HEADER


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  
#define FT_MODULE_FONT_DRIVER         1  /* this module is a font driver  */
#define FT_MODULE_RENDERER            2  /* this module is a renderer     */
#define FT_MODULE_HINTER              4  /* this module is a glyph hinter */
#define FT_MODULE_STYLER              8  /* this module is a styler       */

#define FT_MODULE_DRIVER_SCALABLE     0x100   /* the driver supports      */
                                              
#define FT_MODULE_DRIVER_NO_OUTLINES  0x200   /* the driver does not      */
                                              
#define FT_MODULE_DRIVER_HAS_HINTER   0x400   /* the driver provides its  */
                                              


  
#define ft_module_font_driver         FT_MODULE_FONT_DRIVER
#define ft_module_renderer            FT_MODULE_RENDERER
#define ft_module_hinter              FT_MODULE_HINTER
#define ft_module_styler              FT_MODULE_STYLER

#define ft_module_driver_scalable     FT_MODULE_DRIVER_SCALABLE
#define ft_module_driver_no_outlines  FT_MODULE_DRIVER_NO_OUTLINES
#define ft_module_driver_has_hinter   FT_MODULE_DRIVER_HAS_HINTER


  typedef FT_Pointer  FT_Module_Interface;


  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*FT_Module_Constructor)( FT_Module  module );


  
  
  
  
  
  
  
  
  
  
  
  typedef void
  (*FT_Module_Destructor)( FT_Module  module );


  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Module_Interface
  (*FT_Module_Requester)( FT_Module    module,
                          const char*  name );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_Module_Class_
  {
    FT_ULong               module_flags;
    FT_Long                module_size;
    const FT_String*       module_name;
    FT_Fixed               module_version;
    FT_Fixed               module_requires;

    const void*            module_interface;

    FT_Module_Constructor  module_init;
    FT_Module_Destructor   module_done;
    FT_Module_Requester    get_interface;

  } FT_Module_Class;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( FT_Error )
  FT_Add_Module( FT_Library              library,
                 const FT_Module_Class*  clazz );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( FT_Module )
  FT_Get_Module( FT_Library   library,
                 const char*  module_name );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( FT_Error )
  FT_Remove_Module( FT_Library  library,
                    FT_Module   module );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( FT_Error )
  FT_New_Library( FT_Memory    memory,
                  FT_Library  *alibrary );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( FT_Error )
  FT_Done_Library( FT_Library  library );



  typedef void
  (*FT_DebugHook_Func)( void*  arg );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( void )
  FT_Set_Debug_Hook( FT_Library         library,
                     FT_UInt            hook_index,
                     FT_DebugHook_Func  debug_hook );


  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( void )
  FT_Add_Default_Modules( FT_Library  library );



  

















  






























  typedef enum  FT_TrueTypeEngineType_
  {
    FT_TRUETYPE_ENGINE_TYPE_NONE = 0,
    FT_TRUETYPE_ENGINE_TYPE_UNPATENTED,
    FT_TRUETYPE_ENGINE_TYPE_PATENTED

  } FT_TrueTypeEngineType;


  



















  FT_EXPORT( FT_TrueTypeEngineType )
  FT_Get_TrueType_Engine_Type( FT_Library  library );


  


FT_END_HEADER

#endif 




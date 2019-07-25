

















#include "afmodule.h"
#include "afloader.h"
#include "afpic.h"

#ifdef FT_DEBUG_AUTOFIT
  int    _af_debug_disable_horz_hints;
  int    _af_debug_disable_vert_hints;
  int    _af_debug_disable_blue_hints;
  void*  _af_debug_hints;
#endif

#include FT_INTERNAL_OBJECTS_H


  typedef struct  FT_AutofitterRec_
  {
    FT_ModuleRec  root;
    AF_LoaderRec  loader[1];

  } FT_AutofitterRec, *FT_Autofitter;


  FT_CALLBACK_DEF( FT_Error )
  af_autofitter_init( FT_Autofitter  module )
  {
    return af_loader_init( module->loader, module->root.library->memory );
  }


  FT_CALLBACK_DEF( void )
  af_autofitter_done( FT_Autofitter  module )
  {
    af_loader_done( module->loader );
  }


  FT_CALLBACK_DEF( FT_Error )
  af_autofitter_load_glyph( FT_Autofitter  module,
                            FT_GlyphSlot   slot,
                            FT_Size        size,
                            FT_UInt        glyph_index,
                            FT_Int32       load_flags )
  {
    FT_UNUSED( size );

    return af_loader_load_glyph( module->loader, slot->face,
                                 glyph_index, load_flags );
  }


  FT_DEFINE_AUTOHINTER_SERVICE(
    af_autofitter_service,
    NULL,
    NULL,
    NULL,
    (FT_AutoHinter_GlyphLoadFunc)af_autofitter_load_glyph )

  FT_DEFINE_MODULE(
    autofit_module_class,

    FT_MODULE_HINTER,
    sizeof ( FT_AutofitterRec ),

    "autofitter",
    0x10000L,   
    0x20000L,   

    (const void*)&AF_AF_AUTOFITTER_SERVICE_GET,

    (FT_Module_Constructor)af_autofitter_init,
    (FT_Module_Destructor) af_autofitter_done,
    (FT_Module_Requester)  NULL )






















  
  
  
  
  


#ifndef __FTOBJS_H__
#define __FTOBJS_H__

#include <ft2build.h>
#include FT_RENDER_H
#include FT_SIZES_H
#include FT_LCD_FILTER_H
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_GLYPH_LOADER_H
#include FT_INTERNAL_DRIVER_H
#include FT_INTERNAL_AUTOHINT_H
#include FT_INTERNAL_SERVICE_H
#include FT_INTERNAL_PIC_H

#ifdef FT_CONFIG_OPTION_INCREMENTAL
#include FT_INCREMENTAL_H
#endif


FT_BEGIN_HEADER


  
  
  
  
#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE  0
#endif

#ifndef NULL
#define NULL  (void*)0
#endif


  
  
  
  
  
#define FT_MIN( a, b )  ( (a) < (b) ? (a) : (b) )
#define FT_MAX( a, b )  ( (a) > (b) ? (a) : (b) )

#define FT_ABS( a )     ( (a) < 0 ? -(a) : (a) )


#define FT_PAD_FLOOR( x, n )  ( (x) & ~((n)-1) )
#define FT_PAD_ROUND( x, n )  FT_PAD_FLOOR( (x) + ((n)/2), n )
#define FT_PAD_CEIL( x, n )   FT_PAD_FLOOR( (x) + ((n)-1), n )

#define FT_PIX_FLOOR( x )     ( (x) & ~63 )
#define FT_PIX_ROUND( x )     FT_PIX_FLOOR( (x) + 32 )
#define FT_PIX_CEIL( x )      FT_PIX_FLOOR( (x) + 63 )


  



  FT_BASE( FT_UInt32 )
  ft_highpow2( FT_UInt32  value );


  




#define  ft_isdigit( x )   ( ( (unsigned)(x) - '0' ) < 10U )

#define  ft_isxdigit( x )  ( ( (unsigned)(x) - '0' ) < 10U || \
                             ( (unsigned)(x) - 'a' ) < 6U  || \
                             ( (unsigned)(x) - 'A' ) < 6U  )

  
#define  ft_isupper( x )  ( ( (unsigned)(x) - 'A' ) < 26U )
#define  ft_islower( x )  ( ( (unsigned)(x) - 'a' ) < 26U )

#define  ft_isalpha( x )  ( ft_isupper( x ) || ft_islower( x ) )
#define  ft_isalnum( x )  ( ft_isdigit( x ) || ft_isalpha( x ) )


  
  
  
  
  
  
  
  
  
  
  

  
  typedef struct FT_CMapRec_*              FT_CMap;

  
  typedef const struct FT_CMap_ClassRec_*  FT_CMap_Class;

  
  typedef struct  FT_CMapRec_
  {
    FT_CharMapRec  charmap;
    FT_CMap_Class  clazz;

  } FT_CMapRec;

  
#define FT_CMAP( x )              ((FT_CMap)( x ))

  
#define FT_CMAP_PLATFORM_ID( x )  FT_CMAP( x )->charmap.platform_id
#define FT_CMAP_ENCODING_ID( x )  FT_CMAP( x )->charmap.encoding_id
#define FT_CMAP_ENCODING( x )     FT_CMAP( x )->charmap.encoding
#define FT_CMAP_FACE( x )         FT_CMAP( x )->charmap.face


  
  typedef FT_Error
  (*FT_CMap_InitFunc)( FT_CMap     cmap,
                       FT_Pointer  init_data );

  typedef void
  (*FT_CMap_DoneFunc)( FT_CMap  cmap );

  typedef FT_UInt
  (*FT_CMap_CharIndexFunc)( FT_CMap    cmap,
                            FT_UInt32  char_code );

  typedef FT_UInt
  (*FT_CMap_CharNextFunc)( FT_CMap     cmap,
                           FT_UInt32  *achar_code );

  typedef FT_UInt
  (*FT_CMap_CharVarIndexFunc)( FT_CMap    cmap,
                               FT_CMap    unicode_cmap,
                               FT_UInt32  char_code,
                               FT_UInt32  variant_selector );

  typedef FT_Bool
  (*FT_CMap_CharVarIsDefaultFunc)( FT_CMap    cmap,
                                   FT_UInt32  char_code,
                                   FT_UInt32  variant_selector );

  typedef FT_UInt32 *
  (*FT_CMap_VariantListFunc)( FT_CMap    cmap,
                              FT_Memory  mem );

  typedef FT_UInt32 *
  (*FT_CMap_CharVariantListFunc)( FT_CMap    cmap,
                                  FT_Memory  mem,
                                  FT_UInt32  char_code );

  typedef FT_UInt32 *
  (*FT_CMap_VariantCharListFunc)( FT_CMap    cmap,
                                  FT_Memory  mem,
                                  FT_UInt32  variant_selector );


  typedef struct  FT_CMap_ClassRec_
  {
    FT_ULong               size;
    FT_CMap_InitFunc       init;
    FT_CMap_DoneFunc       done;
    FT_CMap_CharIndexFunc  char_index;
    FT_CMap_CharNextFunc   char_next;

    
    

    FT_CMap_CharVarIndexFunc      char_var_index;
    FT_CMap_CharVarIsDefaultFunc  char_var_default;
    FT_CMap_VariantListFunc       variant_list;
    FT_CMap_CharVariantListFunc   charvariant_list;
    FT_CMap_VariantCharListFunc   variantchar_list;

  } FT_CMap_ClassRec;

#ifndef FT_CONFIG_OPTION_PIC

#define FT_DECLARE_CMAP_CLASS(class_) \
    FT_CALLBACK_TABLE const FT_CMap_ClassRec class_;

#define FT_DEFINE_CMAP_CLASS(class_, size_, init_, done_, char_index_,       \
        char_next_, char_var_index_, char_var_default_, variant_list_,       \
        charvariant_list_, variantchar_list_)                                \
  FT_CALLBACK_TABLE_DEF                                                      \
  const FT_CMap_ClassRec class_ =                                            \
  {                                                                          \
    size_, init_, done_, char_index_, char_next_, char_var_index_,           \
    char_var_default_, variant_list_, charvariant_list_, variantchar_list_   \
  };
#else 

#define FT_DECLARE_CMAP_CLASS(class_) \
    void FT_Init_Class_##class_( FT_Library library, FT_CMap_ClassRec*  clazz);

#define FT_DEFINE_CMAP_CLASS(class_, size_, init_, done_, char_index_,       \
        char_next_, char_var_index_, char_var_default_, variant_list_,       \
        charvariant_list_, variantchar_list_)                                \
  void                                                                       \
  FT_Init_Class_##class_( FT_Library library,                                \
                          FT_CMap_ClassRec*  clazz)                          \
  {                                                                          \
    FT_UNUSED(library);                                                      \
    clazz->size = size_;                                                     \
    clazz->init = init_;                                                     \
    clazz->done = done_;                                                     \
    clazz->char_index = char_index_;                                         \
    clazz->char_next = char_next_;                                           \
    clazz->char_var_index = char_var_index_;                                 \
    clazz->char_var_default = char_var_default_;                             \
    clazz->variant_list = variant_list_;                                     \
    clazz->charvariant_list = charvariant_list_;                             \
    clazz->variantchar_list = variantchar_list_;                             \
  } 
#endif 

  
  FT_BASE( FT_Error )
  FT_CMap_New( FT_CMap_Class  clazz,
               FT_Pointer     init_data,
               FT_CharMap     charmap,
               FT_CMap       *acmap );

  
  FT_BASE( void )
  FT_CMap_Done( FT_CMap  cmap );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_Face_InternalRec_
  {
#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    FT_UShort           reserved1;
    FT_Short            reserved2;
#endif
    FT_Matrix           transform_matrix;
    FT_Vector           transform_delta;
    FT_Int              transform_flags;

    FT_ServiceCacheRec  services;

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    FT_Incremental_InterfaceRec*  incremental_interface;
#endif

    FT_Bool             ignore_unpatented_hinter;
    FT_UInt             refcount;

  } FT_Face_InternalRec;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

#define FT_GLYPH_OWN_BITMAP  0x1

  typedef struct  FT_Slot_InternalRec_
  {
    FT_GlyphLoader  loader;
    FT_UInt         flags;
    FT_Bool         glyph_transformed;
    FT_Matrix       glyph_matrix;
    FT_Vector       glyph_delta;
    void*           glyph_hints;

  } FT_GlyphSlot_InternalRec;


#if 0

  
  
  
  
  
  
  
  
  
  

  typedef struct  FT_Size_InternalRec_
  {
    

  } FT_Size_InternalRec;

#endif


  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_ModuleRec_
  {
    FT_Module_Class*  clazz;
    FT_Library        library;
    FT_Memory         memory;
    FT_Generic        generic;

  } FT_ModuleRec;


  
#define FT_MODULE( x )          ((FT_Module)( x ))
#define FT_MODULE_CLASS( x )    FT_MODULE( x )->clazz
#define FT_MODULE_LIBRARY( x )  FT_MODULE( x )->library
#define FT_MODULE_MEMORY( x )   FT_MODULE( x )->memory


#define FT_MODULE_IS_DRIVER( x )  ( FT_MODULE_CLASS( x )->module_flags & \
                                    FT_MODULE_FONT_DRIVER )

#define FT_MODULE_IS_RENDERER( x )  ( FT_MODULE_CLASS( x )->module_flags & \
                                      FT_MODULE_RENDERER )

#define FT_MODULE_IS_HINTER( x )  ( FT_MODULE_CLASS( x )->module_flags & \
                                    FT_MODULE_HINTER )

#define FT_MODULE_IS_STYLER( x )  ( FT_MODULE_CLASS( x )->module_flags & \
                                    FT_MODULE_STYLER )

#define FT_DRIVER_IS_SCALABLE( x )  ( FT_MODULE_CLASS( x )->module_flags & \
                                      FT_MODULE_DRIVER_SCALABLE )

#define FT_DRIVER_USES_OUTLINES( x )  !( FT_MODULE_CLASS( x )->module_flags & \
                                         FT_MODULE_DRIVER_NO_OUTLINES )

#define FT_DRIVER_HAS_HINTER( x )  ( FT_MODULE_CLASS( x )->module_flags & \
                                     FT_MODULE_DRIVER_HAS_HINTER )


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_BASE( const void* )
  FT_Get_Module_Interface( FT_Library   library,
                           const char*  mod_name );

  FT_BASE( FT_Pointer )
  ft_module_get_service( FT_Module    module,
                         const char*  service_id );

 


  
  
  
  
  
  
  
  
  
  
  

  

#define FT_FACE( x )          ((FT_Face)(x))
#define FT_SIZE( x )          ((FT_Size)(x))
#define FT_SLOT( x )          ((FT_GlyphSlot)(x))

#define FT_FACE_DRIVER( x )   FT_FACE( x )->driver
#define FT_FACE_LIBRARY( x )  FT_FACE_DRIVER( x )->root.library
#define FT_FACE_MEMORY( x )   FT_FACE( x )->memory
#define FT_FACE_STREAM( x )   FT_FACE( x )->stream

#define FT_SIZE_FACE( x )     FT_SIZE( x )->face
#define FT_SLOT_FACE( x )     FT_SLOT( x )->face

#define FT_FACE_SLOT( x )     FT_FACE( x )->glyph
#define FT_FACE_SIZE( x )     FT_FACE( x )->size


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_BASE( FT_Error )
  FT_New_GlyphSlot( FT_Face        face,
                    FT_GlyphSlot  *aslot );


  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_BASE( void )
  FT_Done_GlyphSlot( FT_GlyphSlot  slot );

 

#define FT_REQUEST_WIDTH( req )                                            \
          ( (req)->horiResolution                                          \
              ? (FT_Pos)( (req)->width * (req)->horiResolution + 36 ) / 72 \
              : (req)->width )

#define FT_REQUEST_HEIGHT( req )                                            \
          ( (req)->vertResolution                                           \
              ? (FT_Pos)( (req)->height * (req)->vertResolution + 36 ) / 72 \
              : (req)->height )


  
  FT_BASE( void )
  FT_Select_Metrics( FT_Face   face,
                     FT_ULong  strike_index );


  
  FT_BASE( void )
  FT_Request_Metrics( FT_Face          face,
                      FT_Size_Request  req );


  
  FT_BASE( FT_Error )
  FT_Match_Size( FT_Face          face,
                 FT_Size_Request  req,
                 FT_Bool          ignore_width,
                 FT_ULong*        size_index );


  
  
  FT_BASE( void )
  ft_synthesize_vertical_metrics( FT_Glyph_Metrics*  metrics,
                                  FT_Pos             advance );


  
  
  FT_BASE( void )
  ft_glyphslot_free_bitmap( FT_GlyphSlot  slot );


  
  FT_BASE( FT_Error )
  ft_glyphslot_alloc_bitmap( FT_GlyphSlot  slot,
                             FT_ULong      size );


  
  
  FT_BASE( void )
  ft_glyphslot_set_bitmap( FT_GlyphSlot  slot,
                           FT_Byte*      buffer );


  
  
  
  
  
  
  
  
  
  
  


#define FT_RENDERER( x )      ((FT_Renderer)( x ))
#define FT_GLYPH( x )         ((FT_Glyph)( x ))
#define FT_BITMAP_GLYPH( x )  ((FT_BitmapGlyph)( x ))
#define FT_OUTLINE_GLYPH( x ) ((FT_OutlineGlyph)( x ))


  typedef struct  FT_RendererRec_
  {
    FT_ModuleRec            root;
    FT_Renderer_Class*      clazz;
    FT_Glyph_Format         glyph_format;
    FT_Glyph_Class          glyph_class;

    FT_Raster               raster;
    FT_Raster_Render_Func   raster_render;
    FT_Renderer_RenderFunc  render;

  } FT_RendererRec;


  
  
  
  
  
  
  
  
  
  
  


  
#define FT_DRIVER( x )        ((FT_Driver)(x))

  
#define FT_DRIVER_CLASS( x )  FT_DRIVER( x )->clazz


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_DriverRec_
  {
    FT_ModuleRec     root;
    FT_Driver_Class  clazz;

    FT_ListRec       faces_list;
    void*            extensions;

    FT_GlyphLoader   glyph_loader;

  } FT_DriverRec;


  
  
  
  
  
  
  
  
  
  
  


  
  
#define FT_DEBUG_HOOK_TRUETYPE            0


  
  
  
  
#define FT_DEBUG_HOOK_UNPATENTED_HINTING  1


  typedef void  (*FT_Bitmap_LcdFilterFunc)( FT_Bitmap*      bitmap,
                                            FT_Render_Mode  render_mode,
                                            FT_Library      library );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_LibraryRec_
  {
    FT_Memory          memory;           

    FT_Generic         generic;

    FT_Int             version_major;
    FT_Int             version_minor;
    FT_Int             version_patch;

    FT_UInt            num_modules;
    FT_Module          modules[FT_MAX_MODULES];  

    FT_ListRec         renderers;        
    FT_Renderer        cur_renderer;     
    FT_Module          auto_hinter;

    FT_Byte*           raster_pool;      
                                         
    FT_ULong           raster_pool_size; 

    FT_DebugHook_Func  debug_hooks[4];

#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
    FT_LcdFilter             lcd_filter;
    FT_Int                   lcd_extra;        
    FT_Byte                  lcd_weights[7];   
    FT_Bitmap_LcdFilterFunc  lcd_filter_func;  
#endif

#ifdef FT_CONFIG_OPTION_PIC
    FT_PIC_Container   pic_container;
#endif

    FT_UInt            refcount;

  } FT_LibraryRec;


  FT_BASE( FT_Renderer )
  FT_Lookup_Renderer( FT_Library       library,
                      FT_Glyph_Format  format,
                      FT_ListNode*     node );

  FT_BASE( FT_Error )
  FT_Render_Glyph_Internal( FT_Library      library,
                            FT_GlyphSlot    slot,
                            FT_Render_Mode  render_mode );

  typedef const char*
  (*FT_Face_GetPostscriptNameFunc)( FT_Face  face );

  typedef FT_Error
  (*FT_Face_GetGlyphNameFunc)( FT_Face     face,
                               FT_UInt     glyph_index,
                               FT_Pointer  buffer,
                               FT_UInt     buffer_max );

  typedef FT_UInt
  (*FT_Face_GetGlyphNameIndexFunc)( FT_Face     face,
                                    FT_String*  glyph_name );


#ifndef FT_CONFIG_OPTION_NO_DEFAULT_SYSTEM

  
  
  
  
  
  
  
  
  
  
  
  FT_BASE( FT_Memory )
  FT_New_Memory( void );


  
  
  
  
  
  
  
  
  
  
  
  FT_BASE( void )
  FT_Done_Memory( FT_Memory  memory );

#endif 


  
  
  
  
  

#ifndef FT_NO_DEFAULT_RASTER
  FT_EXPORT_VAR( FT_Raster_Funcs )  ft_default_raster;
#endif

  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
#ifndef FT_CONFIG_OPTION_PIC

#define FT_DEFINE_OUTLINE_FUNCS(class_, move_to_, line_to_, conic_to_,       \
                                cubic_to_, shift_, delta_)                   \
  static const FT_Outline_Funcs class_ =                                     \
  {                                                                          \
    move_to_, line_to_, conic_to_, cubic_to_, shift_, delta_                 \
  };

#else  

#define FT_DEFINE_OUTLINE_FUNCS(class_, move_to_, line_to_, conic_to_,       \
                                cubic_to_, shift_, delta_)                   \
  static FT_Error                                                            \
  Init_Class_##class_( FT_Outline_Funcs*  clazz )                            \
  {                                                                          \
    clazz->move_to = move_to_;                                               \
    clazz->line_to = line_to_;                                               \
    clazz->conic_to = conic_to_;                                             \
    clazz->cubic_to = cubic_to_;                                             \
    clazz->shift = shift_;                                                   \
    clazz->delta = delta_;                                                   \
    return FT_Err_Ok;                                                        \
  } 

#endif  

  
  
  
  
  
  
  
  
  
  
  
  
  
#ifndef FT_CONFIG_OPTION_PIC

#define FT_DEFINE_RASTER_FUNCS(class_, glyph_format_, raster_new_,           \
                               raster_reset_, raster_set_mode_,              \
                               raster_render_, raster_done_)                 \
  const FT_Raster_Funcs class_ =                                      \
  {                                                                          \
    glyph_format_, raster_new_, raster_reset_,                               \
    raster_set_mode_, raster_render_, raster_done_                           \
  };

#else  

#define FT_DEFINE_RASTER_FUNCS(class_, glyph_format_, raster_new_,           \
    raster_reset_, raster_set_mode_, raster_render_, raster_done_)           \
  void                                                                       \
  FT_Init_Class_##class_( FT_Raster_Funcs*  clazz )                          \
  {                                                                          \
    clazz->glyph_format = glyph_format_;                                     \
    clazz->raster_new = raster_new_;                                         \
    clazz->raster_reset = raster_reset_;                                     \
    clazz->raster_set_mode = raster_set_mode_;                               \
    clazz->raster_render = raster_render_;                                   \
    clazz->raster_done = raster_done_;                                       \
  } 

#endif  

  
  
  
  
  
  
  
  
  
  
  



  
  
  
  
  
  
  
  
  
  
  
  
  
#ifndef FT_CONFIG_OPTION_PIC

#define FT_DEFINE_GLYPH(class_, size_, format_, init_, done_, copy_,         \
                        transform_, bbox_, prepare_)                         \
  FT_CALLBACK_TABLE_DEF                                                      \
  const FT_Glyph_Class class_ =                                              \
  {                                                                          \
    size_, format_, init_, done_, copy_, transform_, bbox_, prepare_         \
  };

#else  

#define FT_DEFINE_GLYPH(class_, size_, format_, init_, done_, copy_,         \
                        transform_, bbox_, prepare_)                         \
  void                                                                       \
  FT_Init_Class_##class_( FT_Glyph_Class*  clazz )                           \
  {                                                                          \
    clazz->glyph_size = size_;                                               \
    clazz->glyph_format = format_;                                           \
    clazz->glyph_init = init_;                                               \
    clazz->glyph_done = done_;                                               \
    clazz->glyph_copy = copy_;                                               \
    clazz->glyph_transform = transform_;                                     \
    clazz->glyph_bbox = bbox_;                                               \
    clazz->glyph_prepare = prepare_;                                         \
  } 

#endif  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
#ifndef FT_CONFIG_OPTION_PIC

#define FT_DECLARE_RENDERER(class_)                                          \
    FT_EXPORT_VAR( const FT_Renderer_Class ) class_;

#define FT_DEFINE_RENDERER(class_,                                           \
                           flags_, size_, name_, version_, requires_,        \
                           interface_, init_, done_, get_interface_,         \
                           glyph_format_, render_glyph_, transform_glyph_,   \
                           get_glyph_cbox_, set_mode_, raster_class_ )       \
  FT_CALLBACK_TABLE_DEF                                                      \
  const FT_Renderer_Class  class_ =                                          \
  {                                                                          \
    FT_DEFINE_ROOT_MODULE(flags_,size_,name_,version_,requires_,             \
                          interface_,init_,done_,get_interface_)             \
    glyph_format_,                                                           \
                                                                             \
    render_glyph_,                                                           \
    transform_glyph_,                                                        \
    get_glyph_cbox_,                                                         \
    set_mode_,                                                               \
                                                                             \
    raster_class_                                                            \
  };

#else  

#define FT_DECLARE_RENDERER(class_)  FT_DECLARE_MODULE(class_)

#define FT_DEFINE_RENDERER(class_, \
                           flags_, size_, name_, version_, requires_,        \
                           interface_, init_, done_, get_interface_,         \
                           glyph_format_, render_glyph_, transform_glyph_,   \
                           get_glyph_cbox_, set_mode_, raster_class_ )       \
  void class_##_pic_free( FT_Library library );                              \
  FT_Error class_##_pic_init( FT_Library library );                          \
                                                                             \
  void                                                                       \
  FT_Destroy_Class_##class_( FT_Library        library,                      \
                        FT_Module_Class*  clazz )                            \
  {                                                                          \
    FT_Renderer_Class* rclazz = (FT_Renderer_Class*)clazz;                   \
    FT_Memory         memory = library->memory;                              \
    class_##_pic_free( library );                                            \
    if ( rclazz )                                                            \
      FT_FREE( rclazz );                                                     \
  }                                                                          \
                                                                             \
  FT_Error                                                                   \
  FT_Create_Class_##class_( FT_Library         library,                      \
                            FT_Module_Class**  output_class )                \
  {                                                                          \
    FT_Renderer_Class*  clazz;                                               \
    FT_Error            error;                                               \
    FT_Memory           memory = library->memory;                            \
                                                                             \
    if ( FT_ALLOC( clazz, sizeof(*clazz) ) )                                 \
      return error;                                                          \
                                                                             \
    error = class_##_pic_init( library );                                    \
    if(error)                                                                \
    {                                                                        \
      FT_FREE( clazz );                                                      \
      return error;                                                          \
    }                                                                        \
                                                                             \
    FT_DEFINE_ROOT_MODULE(flags_,size_,name_,version_,requires_,             \
                          interface_,init_,done_,get_interface_)             \
                                                                             \
    clazz->glyph_format       = glyph_format_;                               \
                                                                             \
    clazz->render_glyph       = render_glyph_;                               \
    clazz->transform_glyph    = transform_glyph_;                            \
    clazz->get_glyph_cbox     = get_glyph_cbox_;                             \
    clazz->set_mode           = set_mode_;                                   \
                                                                             \
    clazz->raster_class       = raster_class_;                               \
                                                                             \
    *output_class = (FT_Module_Class*)clazz;                                 \
    return FT_Err_Ok;                                                        \
  } 



#endif  

  
  
  
  
  
  
  
  
  
  
  


#ifdef FT_CONFIG_OPTION_PIC

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*FT_Module_Creator)( FT_Memory          memory,
                        FT_Module_Class**  output_class );

  
  
  
  
  
  
  
  
  
  
  
  
  typedef void
  (*FT_Module_Destroyer)( FT_Memory         memory,
                          FT_Module_Class*  clazz );

#endif

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
#ifndef FT_CONFIG_OPTION_PIC

#define FT_DECLARE_MODULE(class_)                                            \
  FT_CALLBACK_TABLE                                                          \
  const FT_Module_Class  class_;                                             \

#define FT_DEFINE_ROOT_MODULE(flags_, size_, name_, version_, requires_,     \
                              interface_, init_, done_, get_interface_)      \
  {                                                                          \
    flags_,                                                                  \
    size_,                                                                   \
                                                                             \
    name_,                                                                   \
    version_,                                                                \
    requires_,                                                               \
                                                                             \
    interface_,                                                              \
                                                                             \
    init_,                                                                   \
    done_,                                                                   \
    get_interface_,                                                          \
  },

#define FT_DEFINE_MODULE(class_, flags_, size_, name_, version_, requires_,  \
                         interface_, init_, done_, get_interface_)           \
  FT_CALLBACK_TABLE_DEF                                                      \
  const FT_Module_Class class_ =                                             \
  {                                                                          \
    flags_,                                                                  \
    size_,                                                                   \
                                                                             \
    name_,                                                                   \
    version_,                                                                \
    requires_,                                                               \
                                                                             \
    interface_,                                                              \
                                                                             \
    init_,                                                                   \
    done_,                                                                   \
    get_interface_,                                                          \
  };


#else 

#define FT_DECLARE_MODULE(class_)                                            \
  FT_Error FT_Create_Class_##class_( FT_Library library,                     \
                                     FT_Module_Class** output_class );       \
  void     FT_Destroy_Class_##class_( FT_Library library,                    \
                                      FT_Module_Class*  clazz );

#define FT_DEFINE_ROOT_MODULE(flags_, size_, name_, version_, requires_,     \
                              interface_, init_, done_, get_interface_)      \
    clazz->root.module_flags       = flags_;                                 \
    clazz->root.module_size        = size_;                                  \
    clazz->root.module_name        = name_;                                  \
    clazz->root.module_version     = version_;                               \
    clazz->root.module_requires    = requires_;                              \
                                                                             \
    clazz->root.module_interface   = interface_;                             \
                                                                             \
    clazz->root.module_init        = init_;                                  \
    clazz->root.module_done        = done_;                                  \
    clazz->root.get_interface      = get_interface_;               

#define FT_DEFINE_MODULE(class_, flags_, size_, name_, version_, requires_,  \
                         interface_, init_, done_, get_interface_)           \
  void class_##_pic_free( FT_Library library );                              \
  FT_Error class_##_pic_init( FT_Library library );                          \
                                                                             \
  void                                                                       \
  FT_Destroy_Class_##class_( FT_Library library,                             \
                             FT_Module_Class*  clazz )                       \
  {                                                                          \
    FT_Memory memory = library->memory;                                      \
    class_##_pic_free( library );                                            \
    if ( clazz )                                                             \
      FT_FREE( clazz );                                                      \
  }                                                                          \
                                                                             \
  FT_Error                                                                   \
  FT_Create_Class_##class_( FT_Library library,                              \
                            FT_Module_Class**  output_class )                \
  {                                                                          \
    FT_Memory memory = library->memory;                                      \
    FT_Module_Class*  clazz;                                                 \
    FT_Error          error;                                                 \
                                                                             \
    if ( FT_ALLOC( clazz, sizeof(*clazz) ) )                                 \
      return error;                                                          \
    error = class_##_pic_init( library );                                    \
    if(error)                                                                \
    {                                                                        \
      FT_FREE( clazz );                                                      \
      return error;                                                          \
    }                                                                        \
                                                                             \
    clazz->module_flags       = flags_;                                      \
    clazz->module_size        = size_;                                       \
    clazz->module_name        = name_;                                       \
    clazz->module_version     = version_;                                    \
    clazz->module_requires    = requires_;                                   \
                                                                             \
    clazz->module_interface   = interface_;                                  \
                                                                             \
    clazz->module_init        = init_;                                       \
    clazz->module_done        = done_;                                       \
    clazz->get_interface      = get_interface_;                              \
                                                                             \
    *output_class = clazz;                                                   \
    return FT_Err_Ok;                                                        \
  } 

#endif 


FT_END_HEADER

#endif 




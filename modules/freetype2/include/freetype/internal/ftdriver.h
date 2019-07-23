

















#ifndef __FTDRIVER_H__
#define __FTDRIVER_H__


#include <ft2build.h>
#include FT_MODULE_H


FT_BEGIN_HEADER


  typedef FT_Error
  (*FT_Face_InitFunc)( FT_Stream      stream,
                       FT_Face        face,
                       FT_Int         typeface_index,
                       FT_Int         num_params,
                       FT_Parameter*  parameters );

  typedef void
  (*FT_Face_DoneFunc)( FT_Face  face );


  typedef FT_Error
  (*FT_Size_InitFunc)( FT_Size  size );

  typedef void
  (*FT_Size_DoneFunc)( FT_Size  size );


  typedef FT_Error
  (*FT_Slot_InitFunc)( FT_GlyphSlot  slot );

  typedef void
  (*FT_Slot_DoneFunc)( FT_GlyphSlot  slot );


  typedef FT_Error
  (*FT_Size_RequestFunc)( FT_Size          size,
                          FT_Size_Request  req );

  typedef FT_Error
  (*FT_Size_SelectFunc)( FT_Size   size,
                         FT_ULong  size_index );

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  typedef FT_Error
  (*FT_Size_ResetPointsFunc)( FT_Size     size,
                              FT_F26Dot6  char_width,
                              FT_F26Dot6  char_height,
                              FT_UInt     horz_resolution,
                              FT_UInt     vert_resolution );

  typedef FT_Error
  (*FT_Size_ResetPixelsFunc)( FT_Size  size,
                              FT_UInt  pixel_width,
                              FT_UInt  pixel_height );

#endif 

  typedef FT_Error
  (*FT_Slot_LoadFunc)( FT_GlyphSlot  slot,
                       FT_Size       size,
                       FT_UInt       glyph_index,
                       FT_Int32      load_flags );


  typedef FT_UInt
  (*FT_CharMap_CharIndexFunc)( FT_CharMap  charmap,
                               FT_Long     charcode );

  typedef FT_Long
  (*FT_CharMap_CharNextFunc)( FT_CharMap  charmap,
                              FT_Long     charcode );


  typedef FT_Error
  (*FT_Face_GetKerningFunc)( FT_Face     face,
                             FT_UInt     left_glyph,
                             FT_UInt     right_glyph,
                             FT_Vector*  kerning );


  typedef FT_Error
  (*FT_Face_AttachFunc)( FT_Face    face,
                         FT_Stream  stream );


  typedef FT_Error
  (*FT_Face_GetAdvancesFunc)( FT_Face    face,
                              FT_UInt    first,
                              FT_UInt    count,
                              FT_Int32   flags,
                              FT_Fixed*  advances );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_Driver_ClassRec_
  {
    FT_Module_Class           root;

    FT_Long                   face_object_size;
    FT_Long                   size_object_size;
    FT_Long                   slot_object_size;

    FT_Face_InitFunc          init_face;
    FT_Face_DoneFunc          done_face;

    FT_Size_InitFunc          init_size;
    FT_Size_DoneFunc          done_size;

    FT_Slot_InitFunc          init_slot;
    FT_Slot_DoneFunc          done_slot;

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

    FT_Size_ResetPointsFunc   set_char_sizes;
    FT_Size_ResetPixelsFunc   set_pixel_sizes;

#endif 

    FT_Slot_LoadFunc          load_glyph;

    FT_Face_GetKerningFunc    get_kerning;
    FT_Face_AttachFunc        attach_file;
    FT_Face_GetAdvancesFunc   get_advances;

    
    FT_Size_RequestFunc       request_size;
    FT_Size_SelectFunc        select_size;

  } FT_Driver_ClassRec, *FT_Driver_Class;


  






#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  FT_BASE( FT_Error )
  ft_stub_set_char_sizes( FT_Size     size,
                          FT_F26Dot6  width,
                          FT_F26Dot6  height,
                          FT_UInt     horz_res,
                          FT_UInt     vert_res );

  FT_BASE( FT_Error )
  ft_stub_set_pixel_sizes( FT_Size  size,
                           FT_UInt  width,
                           FT_UInt  height );

#endif 

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
#ifndef FT_CONFIG_OPTION_PIC

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
#define FT_DEFINE_DRIVERS_OLD_INTERNALS(a_,b_) \
  a_, b_,
#else
  #define FT_DEFINE_DRIVERS_OLD_INTERNALS(a_,b_)
#endif

#define FT_DECLARE_DRIVER(class_)    \
  FT_CALLBACK_TABLE                  \
  const FT_Driver_ClassRec  class_;  

#define FT_DEFINE_DRIVER(class_,                                             \
                         flags_, size_, name_, version_, requires_,          \
                         interface_, init_, done_, get_interface_,           \
                         face_object_size_, size_object_size_,               \
                         slot_object_size_, init_face_, done_face_,          \
                         init_size_, done_size_, init_slot_, done_slot_,     \
                         old_set_char_sizes_, old_set_pixel_sizes_,          \
                         load_glyph_, get_kerning_, attach_file_,            \
                         get_advances_, request_size_, select_size_ )        \
  FT_CALLBACK_TABLE_DEF                                                      \
  const FT_Driver_ClassRec class_ =                                          \
  {                                                                          \
    FT_DEFINE_ROOT_MODULE(flags_,size_,name_,version_,requires_,interface_,  \
                          init_,done_,get_interface_)                        \
                                                                             \
    face_object_size_,                                                       \
    size_object_size_,                                                       \
    slot_object_size_,                                                       \
                                                                             \
    init_face_,                                                              \
    done_face_,                                                              \
                                                                             \
    init_size_,                                                              \
    done_size_,                                                              \
                                                                             \
    init_slot_,                                                              \
    done_slot_,                                                              \
                                                                             \
    FT_DEFINE_DRIVERS_OLD_INTERNALS(old_set_char_sizes_, old_set_pixel_sizes_) \
                                                                             \
    load_glyph_,                                                             \
                                                                             \
    get_kerning_,                                                            \
    attach_file_,                                                            \
    get_advances_,                                                           \
                                                                             \
    request_size_,                                                           \
    select_size_                                                             \
  };

#else  

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
#define FT_DEFINE_DRIVERS_OLD_INTERNALS(a_,b_) \
  clazz->set_char_sizes = a_; \
  clazz->set_pixel_sizes = b_;
#else
  #define FT_DEFINE_DRIVERS_OLD_INTERNALS(a_,b_)
#endif

#define FT_DECLARE_DRIVER(class_)    FT_DECLARE_MODULE(class_)

#define FT_DEFINE_DRIVER(class_,                                             \
                         flags_, size_, name_, version_, requires_,          \
                         interface_, init_, done_, get_interface_,           \
                         face_object_size_, size_object_size_,               \
                         slot_object_size_, init_face_, done_face_,          \
                         init_size_, done_size_, init_slot_, done_slot_,     \
                         old_set_char_sizes_, old_set_pixel_sizes_,          \
                         load_glyph_, get_kerning_, attach_file_,            \
                         get_advances_, request_size_, select_size_ )        \
  void class_##_pic_free( FT_Library library );                              \
  FT_Error class_##_pic_init( FT_Library library );                          \
                                                                             \
  void                                                                       \
  FT_Destroy_Class_##class_( FT_Library        library,                      \
                             FT_Module_Class*  clazz )                       \
  {                                                                          \
    FT_Memory       memory = library->memory;                                \
    FT_Driver_Class dclazz = (FT_Driver_Class)clazz;                         \
    class_##_pic_free( library );                                            \
    if ( dclazz )                                                            \
      FT_FREE( dclazz );                                                     \
  }                                                                          \
                                                                             \
  FT_Error                                                                   \
  FT_Create_Class_##class_( FT_Library        library,                       \
                            FT_Module_Class**  output_class )                \
  {                                                                          \
    FT_Driver_Class  clazz;                                                  \
    FT_Error         error;                                                  \
    FT_Memory        memory = library->memory;                               \
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
    FT_DEFINE_ROOT_MODULE(flags_,size_,name_,version_,requires_,interface_,  \
                          init_,done_,get_interface_)                        \
                                                                             \
    clazz->face_object_size    = face_object_size_;                          \
    clazz->size_object_size    = size_object_size_;                          \
    clazz->slot_object_size    = slot_object_size_;                          \
                                                                             \
    clazz->init_face           = init_face_;                                 \
    clazz->done_face           = done_face_;                                 \
                                                                             \
    clazz->init_size           = init_size_;                                 \
    clazz->done_size           = done_size_;                                 \
                                                                             \
    clazz->init_slot           = init_slot_;                                 \
    clazz->done_slot           = done_slot_;                                 \
                                                                             \
    FT_DEFINE_DRIVERS_OLD_INTERNALS(old_set_char_sizes_, old_set_pixel_sizes_) \
                                                                             \
    clazz->load_glyph          = load_glyph_;                                \
                                                                             \
    clazz->get_kerning         = get_kerning_;                               \
    clazz->attach_file         = attach_file_;                               \
    clazz->get_advances        = get_advances_;                              \
                                                                             \
    clazz->request_size        = request_size_;                              \
    clazz->select_size         = select_size_;                               \
                                                                             \
    *output_class = (FT_Module_Class*)clazz;                                 \
    return FT_Err_Ok;                                                        \
  }                


#endif 

FT_END_HEADER

#endif 




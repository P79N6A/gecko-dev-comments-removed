

















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
  (*FT_Face_GetAdvancesFunc)( FT_Face     face,
                              FT_UInt     first,
                              FT_UInt     count,
                              FT_Bool     vertical,
                              FT_UShort*  advances );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
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


FT_END_HEADER

#endif 




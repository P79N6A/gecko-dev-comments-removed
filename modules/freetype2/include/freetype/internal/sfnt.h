

















#ifndef __SFNT_H__
#define __SFNT_H__


#include <ft2build.h>
#include FT_INTERNAL_DRIVER_H
#include FT_INTERNAL_TRUETYPE_TYPES_H


FT_BEGIN_HEADER


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Init_Face_Func)( FT_Stream      stream,
                        TT_Face        face,
                        FT_Int         face_index,
                        FT_Int         num_params,
                        FT_Parameter*  params );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Load_Face_Func)( FT_Stream      stream,
                        TT_Face        face,
                        FT_Int         face_index,
                        FT_Int         num_params,
                        FT_Parameter*  params );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef void
  (*TT_Done_Face_Func)( TT_Face  face );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Load_Any_Func)( TT_Face    face,
                       FT_ULong   tag,
                       FT_Long    offset,
                       FT_Byte   *buffer,
                       FT_ULong*  length );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Find_SBit_Image_Func)( TT_Face          face,
                              FT_UInt          glyph_index,
                              FT_ULong         strike_index,
                              TT_SBit_Range   *arange,
                              TT_SBit_Strike  *astrike,
                              FT_ULong        *aglyph_offset );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Load_SBit_Metrics_Func)( FT_Stream        stream,
                                TT_SBit_Range    range,
                                TT_SBit_Metrics  metrics );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Load_SBit_Image_Func)( TT_Face              face,
                              FT_ULong             strike_index,
                              FT_UInt              glyph_index,
                              FT_UInt              load_flags,
                              FT_Stream            stream,
                              FT_Bitmap           *amap,
                              TT_SBit_MetricsRec  *ametrics );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Set_SBit_Strike_Func)( TT_Face          face,
                              FT_Size_Request  req,
                              FT_ULong*        astrike_index );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Load_Strike_Metrics_Func)( TT_Face           face,
                                  FT_ULong          strike_index,
                                  FT_Size_Metrics*  metrics );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Get_PS_Name_Func)( TT_Face      face,
                          FT_UInt      idx,
                          FT_String**  PSname );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Load_Metrics_Func)( TT_Face    face,
                           FT_Stream  stream,
                           FT_Bool    vertical );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Get_Metrics_Func)( TT_Face     face,
                          FT_Bool     vertical,
                          FT_UInt     gindex,
                          FT_Short*   abearing,
                          FT_UShort*  aadvance );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Load_Table_Func)( TT_Face    face,
                         FT_Stream  stream );


  
  
  
  
  
  
  
  
  
  
  
  typedef void
  (*TT_Free_Table_Func)( TT_Face  face );


  














  typedef FT_Int
  (*TT_Face_GetKerningFunc)( TT_Face  face,
                             FT_UInt  left_glyph,
                             FT_UInt  right_glyph );


  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  SFNT_Interface_
  {
    TT_Loader_GotoTableFunc      goto_table;

    TT_Init_Face_Func            init_face;
    TT_Load_Face_Func            load_face;
    TT_Done_Face_Func            done_face;
    FT_Module_Requester          get_interface;

    TT_Load_Any_Func             load_any;

    
    
    TT_Load_Table_Func           load_head;
    TT_Load_Metrics_Func         load_hhea;
    TT_Load_Table_Func           load_cmap;
    TT_Load_Table_Func           load_maxp;
    TT_Load_Table_Func           load_os2;
    TT_Load_Table_Func           load_post;

    TT_Load_Table_Func           load_name;
    TT_Free_Table_Func           free_name;

    
    TT_Load_Table_Func           load_kern;

    TT_Load_Table_Func           load_gasp;
    TT_Load_Table_Func           load_pclt;

    
    
    TT_Load_Table_Func           load_bhed;

    TT_Load_SBit_Image_Func      load_sbit_image;

    
    TT_Get_PS_Name_Func          get_psname;
    TT_Free_Table_Func           free_psnames;

    

    
    TT_Face_GetKerningFunc       get_kerning;

    

    
    
    TT_Load_Table_Func           load_font_dir;
    TT_Load_Metrics_Func         load_hmtx;

    TT_Load_Table_Func           load_eblc;
    TT_Free_Table_Func           free_eblc;

    TT_Set_SBit_Strike_Func      set_sbit_strike;
    TT_Load_Strike_Metrics_Func  load_strike_metrics;

    TT_Get_Metrics_Func          get_metrics;

  } SFNT_Interface;


  
  typedef SFNT_Interface*   SFNT_Service;

#ifndef FT_CONFIG_OPTION_PIC

#define FT_DEFINE_SFNT_INTERFACE(        \
          class_,                        \
          goto_table_,                   \
          init_face_,                    \
          load_face_,                    \
          done_face_,                    \
          get_interface_,                \
          load_any_,                     \
          load_head_,                    \
          load_hhea_,                    \
          load_cmap_,                    \
          load_maxp_,                    \
          load_os2_,                     \
          load_post_,                    \
          load_name_,                    \
          free_name_,                    \
          load_kern_,                    \
          load_gasp_,                    \
          load_pclt_,                    \
          load_bhed_,                    \
          load_sbit_image_,              \
          get_psname_,                   \
          free_psnames_,                 \
          get_kerning_,                  \
          load_font_dir_,                \
          load_hmtx_,                    \
          load_eblc_,                    \
          free_eblc_,                    \
          set_sbit_strike_,              \
          load_strike_metrics_,          \
          get_metrics_ )                 \
  static const SFNT_Interface  class_ =  \
  {                                      \
    goto_table_,                         \
    init_face_,                          \
    load_face_,                          \
    done_face_,                          \
    get_interface_,                      \
    load_any_,                           \
    load_head_,                          \
    load_hhea_,                          \
    load_cmap_,                          \
    load_maxp_,                          \
    load_os2_,                           \
    load_post_,                          \
    load_name_,                          \
    free_name_,                          \
    load_kern_,                          \
    load_gasp_,                          \
    load_pclt_,                          \
    load_bhed_,                          \
    load_sbit_image_,                    \
    get_psname_,                         \
    free_psnames_,                       \
    get_kerning_,                        \
    load_font_dir_,                      \
    load_hmtx_,                          \
    load_eblc_,                          \
    free_eblc_,                          \
    set_sbit_strike_,                    \
    load_strike_metrics_,                \
    get_metrics_,                        \
  };

#else 

#define FT_INTERNAL( a, a_ )  \
          clazz->a = a_;

#define FT_DEFINE_SFNT_INTERFACE(                       \
          class_,                                       \
          goto_table_,                                  \
          init_face_,                                   \
          load_face_,                                   \
          done_face_,                                   \
          get_interface_,                               \
          load_any_,                                    \
          load_head_,                                   \
          load_hhea_,                                   \
          load_cmap_,                                   \
          load_maxp_,                                   \
          load_os2_,                                    \
          load_post_,                                   \
          load_name_,                                   \
          free_name_,                                   \
          load_kern_,                                   \
          load_gasp_,                                   \
          load_pclt_,                                   \
          load_bhed_,                                   \
          load_sbit_image_,                             \
          get_psname_,                                  \
          free_psnames_,                                \
          get_kerning_,                                 \
          load_font_dir_,                               \
          load_hmtx_,                                   \
          load_eblc_,                                   \
          free_eblc_,                                   \
          set_sbit_strike_,                             \
          load_strike_metrics_,                         \
          get_metrics_ )                                \
  void                                                  \
  FT_Init_Class_ ## class_( FT_Library       library,   \
                            SFNT_Interface*  clazz )    \
  {                                                     \
    FT_UNUSED( library );                               \
                                                        \
    clazz->goto_table          = goto_table_;           \
    clazz->init_face           = init_face_;            \
    clazz->load_face           = load_face_;            \
    clazz->done_face           = done_face_;            \
    clazz->get_interface       = get_interface_;        \
    clazz->load_any            = load_any_;             \
    clazz->load_head           = load_head_;            \
    clazz->load_hhea           = load_hhea_;            \
    clazz->load_cmap           = load_cmap_;            \
    clazz->load_maxp           = load_maxp_;            \
    clazz->load_os2            = load_os2_;             \
    clazz->load_post           = load_post_;            \
    clazz->load_name           = load_name_;            \
    clazz->free_name           = free_name_;            \
    clazz->load_kern           = load_kern_;            \
    clazz->load_gasp           = load_gasp_;            \
    clazz->load_pclt           = load_pclt_;            \
    clazz->load_bhed           = load_bhed_;            \
    clazz->load_sbit_image     = load_sbit_image_;      \
    clazz->get_psname          = get_psname_;           \
    clazz->free_psnames        = free_psnames_;         \
    clazz->get_kerning         = get_kerning_;          \
    clazz->load_font_dir       = load_font_dir_;        \
    clazz->load_hmtx           = load_hmtx_;            \
    clazz->load_eblc           = load_eblc_;            \
    clazz->free_eblc           = free_eblc_;            \
    clazz->set_sbit_strike     = set_sbit_strike_;      \
    clazz->load_strike_metrics = load_strike_metrics_;  \
    clazz->get_metrics         = get_metrics_;          \
  }

#endif 

FT_END_HEADER

#endif 




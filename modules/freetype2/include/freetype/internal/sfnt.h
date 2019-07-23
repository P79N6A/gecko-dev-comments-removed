

















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


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Load_SFNT_HeaderRec_Func)( TT_Face      face,
                                  FT_Stream    stream,
                                  FT_Long      face_index,
                                  SFNT_Header  sfnt );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Load_Directory_Func)( TT_Face      face,
                             FT_Stream    stream,
                             SFNT_Header  sfnt );

#endif 


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
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


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Set_SBit_Strike_OldFunc)( TT_Face    face,
                                 FT_UInt    x_ppem,
                                 FT_UInt    y_ppem,
                                 FT_ULong*  astrike_index );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_CharMap_Load_Func)( TT_Face    face,
                           void*      cmap,
                           FT_Stream  input );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_CharMap_Free_Func)( TT_Face       face,
                           void*         cmap );

#endif 


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
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

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    TT_Load_SFNT_HeaderRec_Func  load_sfnt_header;
    TT_Load_Directory_Func       load_directory;
#endif

    
    
    TT_Load_Table_Func           load_head;
    TT_Load_Metrics_Func         load_hhea;
    TT_Load_Table_Func           load_cmap;
    TT_Load_Table_Func           load_maxp;
    TT_Load_Table_Func           load_os2;
    TT_Load_Table_Func           load_post;

    TT_Load_Table_Func           load_name;
    TT_Free_Table_Func           free_name;

    
#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    TT_Load_Table_Func           load_hdmx_stub;
    TT_Free_Table_Func           free_hdmx_stub;
#endif

    
    TT_Load_Table_Func           load_kern;

    TT_Load_Table_Func           load_gasp;
    TT_Load_Table_Func           load_pclt;

    
    
    TT_Load_Table_Func           load_bhed;

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

    
    TT_Set_SBit_Strike_OldFunc   set_sbit_strike_stub;
    TT_Load_Table_Func           load_sbits_stub;

    










    TT_Find_SBit_Image_Func      find_sbit_image;
    TT_Load_SBit_Metrics_Func    load_sbit_metrics;

#endif

    TT_Load_SBit_Image_Func      load_sbit_image;

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    TT_Free_Table_Func           free_sbits_stub;
#endif

    
    TT_Get_PS_Name_Func          get_psname;
    TT_Free_Table_Func           free_psnames;

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    TT_CharMap_Load_Func         load_charmap_stub;
    TT_CharMap_Free_Func         free_charmap_stub;
#endif

    

    
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

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
#define FT_DEFINE_DRIVERS_OLD_INTERNAL(a) \
  a, 
#else
  #define FT_DEFINE_DRIVERS_OLD_INTERNAL(a)
#endif
#define FT_INTERNAL(a) \
  a, 

#define FT_DEFINE_SFNT_INTERFACE(class_,                                     \
    goto_table_, init_face_, load_face_, done_face_, get_interface_,         \
    load_any_, load_sfnt_header_, load_directory_, load_head_,               \
    load_hhea_, load_cmap_, load_maxp_, load_os2_, load_post_,               \
    load_name_, free_name_, load_hdmx_stub_, free_hdmx_stub_,                \
    load_kern_, load_gasp_, load_pclt_, load_bhed_,                          \
    set_sbit_strike_stub_, load_sbits_stub_, find_sbit_image_,               \
    load_sbit_metrics_, load_sbit_image_, free_sbits_stub_,                  \
    get_psname_, free_psnames_, load_charmap_stub_, free_charmap_stub_,      \
    get_kerning_, load_font_dir_, load_hmtx_, load_eblc_, free_eblc_,        \
    set_sbit_strike_, load_strike_metrics_, get_metrics_ )                   \
  static const SFNT_Interface class_ =                                       \
  {                                                                          \
    FT_INTERNAL(goto_table_) \
    FT_INTERNAL(init_face_) \
    FT_INTERNAL(load_face_) \
    FT_INTERNAL(done_face_) \
    FT_INTERNAL(get_interface_) \
    FT_INTERNAL(load_any_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_sfnt_header_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_directory_) \
    FT_INTERNAL(load_head_) \
    FT_INTERNAL(load_hhea_) \
    FT_INTERNAL(load_cmap_) \
    FT_INTERNAL(load_maxp_) \
    FT_INTERNAL(load_os2_) \
    FT_INTERNAL(load_post_) \
    FT_INTERNAL(load_name_) \
    FT_INTERNAL(free_name_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_hdmx_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(free_hdmx_stub_) \
    FT_INTERNAL(load_kern_) \
    FT_INTERNAL(load_gasp_) \
    FT_INTERNAL(load_pclt_) \
    FT_INTERNAL(load_bhed_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(set_sbit_strike_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_sbits_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(find_sbit_image_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_sbit_metrics_) \
    FT_INTERNAL(load_sbit_image_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(free_sbits_stub_) \
    FT_INTERNAL(get_psname_) \
    FT_INTERNAL(free_psnames_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_charmap_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(free_charmap_stub_) \
    FT_INTERNAL(get_kerning_) \
    FT_INTERNAL(load_font_dir_) \
    FT_INTERNAL(load_hmtx_) \
    FT_INTERNAL(load_eblc_) \
    FT_INTERNAL(free_eblc_) \
    FT_INTERNAL(set_sbit_strike_) \
    FT_INTERNAL(load_strike_metrics_) \
    FT_INTERNAL(get_metrics_) \
  };

#else  

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
#define FT_DEFINE_DRIVERS_OLD_INTERNAL(a, a_) \
  clazz->a = a_;
#else
  #define FT_DEFINE_DRIVERS_OLD_INTERNAL(a, a_)
#endif
#define FT_INTERNAL(a, a_) \
  clazz->a = a_;

#define FT_DEFINE_SFNT_INTERFACE(class_,                                     \
    goto_table_, init_face_, load_face_, done_face_, get_interface_,         \
    load_any_, load_sfnt_header_, load_directory_, load_head_,               \
    load_hhea_, load_cmap_, load_maxp_, load_os2_, load_post_,               \
    load_name_, free_name_, load_hdmx_stub_, free_hdmx_stub_,                \
    load_kern_, load_gasp_, load_pclt_, load_bhed_,                          \
    set_sbit_strike_stub_, load_sbits_stub_, find_sbit_image_,               \
    load_sbit_metrics_, load_sbit_image_, free_sbits_stub_,                  \
    get_psname_, free_psnames_, load_charmap_stub_, free_charmap_stub_,      \
    get_kerning_, load_font_dir_, load_hmtx_, load_eblc_, free_eblc_,        \
    set_sbit_strike_, load_strike_metrics_, get_metrics_ )                   \
  void                                                                       \
  FT_Init_Class_##class_( FT_Library library, SFNT_Interface*  clazz )       \
  {                                                                          \
    FT_UNUSED(library);                                                      \
    FT_INTERNAL(goto_table,goto_table_) \
    FT_INTERNAL(init_face,init_face_) \
    FT_INTERNAL(load_face,load_face_) \
    FT_INTERNAL(done_face,done_face_) \
    FT_INTERNAL(get_interface,get_interface_) \
    FT_INTERNAL(load_any,load_any_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_sfnt_header,load_sfnt_header_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_directory,load_directory_) \
    FT_INTERNAL(load_head,load_head_) \
    FT_INTERNAL(load_hhea,load_hhea_) \
    FT_INTERNAL(load_cmap,load_cmap_) \
    FT_INTERNAL(load_maxp,load_maxp_) \
    FT_INTERNAL(load_os2,load_os2_) \
    FT_INTERNAL(load_post,load_post_) \
    FT_INTERNAL(load_name,load_name_) \
    FT_INTERNAL(free_name,free_name_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_hdmx_stub,load_hdmx_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(free_hdmx_stub,free_hdmx_stub_) \
    FT_INTERNAL(load_kern,load_kern_) \
    FT_INTERNAL(load_gasp,load_gasp_) \
    FT_INTERNAL(load_pclt,load_pclt_) \
    FT_INTERNAL(load_bhed,load_bhed_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(set_sbit_strike_stub,set_sbit_strike_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_sbits_stub,load_sbits_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(find_sbit_image,find_sbit_image_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_sbit_metrics,load_sbit_metrics_) \
    FT_INTERNAL(load_sbit_image,load_sbit_image_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(free_sbits_stub,free_sbits_stub_) \
    FT_INTERNAL(get_psname,get_psname_) \
    FT_INTERNAL(free_psnames,free_psnames_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_charmap_stub,load_charmap_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(free_charmap_stub,free_charmap_stub_) \
    FT_INTERNAL(get_kerning,get_kerning_) \
    FT_INTERNAL(load_font_dir,load_font_dir_) \
    FT_INTERNAL(load_hmtx,load_hmtx_) \
    FT_INTERNAL(load_eblc,load_eblc_) \
    FT_INTERNAL(free_eblc,free_eblc_) \
    FT_INTERNAL(set_sbit_strike,set_sbit_strike_) \
    FT_INTERNAL(load_strike_metrics,load_strike_metrics_) \
    FT_INTERNAL(get_metrics,get_metrics_) \
  } 

#endif  

FT_END_HEADER

#endif 






















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


FT_END_HEADER

#endif 




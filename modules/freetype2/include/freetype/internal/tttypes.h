


















#ifndef __TTTYPES_H__
#define __TTTYPES_H__


#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H
#include FT_INTERNAL_OBJECTS_H

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include FT_MULTIPLE_MASTERS_H
#endif


FT_BEGIN_HEADER


  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TTC_HeaderRec_
  {
    FT_ULong   tag;
    FT_Fixed   version;
    FT_Long    count;
    FT_ULong*  offsets;

  } TTC_HeaderRec;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  SFNT_HeaderRec_
  {
    FT_ULong   format_tag;
    FT_UShort  num_tables;
    FT_UShort  search_range;
    FT_UShort  entry_selector;
    FT_UShort  range_shift;

    FT_ULong   offset;  

  } SFNT_HeaderRec, *SFNT_Header;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_TableRec_
  {
    FT_ULong  Tag;        
    FT_ULong  CheckSum;   
    FT_ULong  Offset;     
    FT_ULong  Length;     

  } TT_TableRec, *TT_Table;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_LongMetricsRec_
  {
    FT_UShort  advance;
    FT_Short   bearing;

  } TT_LongMetricsRec, *TT_LongMetrics;


  
  
  
  
  
  
  
  
  
  typedef FT_Short  TT_ShortMetrics;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_NameEntryRec_
  {
    FT_UShort  platformID;
    FT_UShort  encodingID;
    FT_UShort  languageID;
    FT_UShort  nameID;
    FT_UShort  stringLength;
    FT_ULong   stringOffset;

    
    

    FT_Byte*   string;

  } TT_NameEntryRec, *TT_NameEntry;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_NameTableRec_
  {
    FT_UShort         format;
    FT_UInt           numNameRecords;
    FT_UInt           storageOffset;
    TT_NameEntryRec*  names;
    FT_Stream         stream;

  } TT_NameTableRec, *TT_NameTable;


  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_GaspRangeRec_
  {
    FT_UShort  maxPPEM;
    FT_UShort  gaspFlag;

  } TT_GaspRangeRec, *TT_GaspRange;


#define TT_GASP_GRIDFIT  0x01
#define TT_GASP_DOGRAY   0x02


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_Gasp_
  {
    FT_UShort     version;
    FT_UShort     numRanges;
    TT_GaspRange  gaspRanges;

  } TT_GaspRec;


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_HdmxEntryRec_
  {
    FT_Byte   ppem;
    FT_Byte   max_width;
    FT_Byte*  widths;

  } TT_HdmxEntryRec, *TT_HdmxEntry;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_HdmxRec_
  {
    FT_UShort     version;
    FT_Short      num_records;
    TT_HdmxEntry  records;

  } TT_HdmxRec, *TT_Hdmx;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_Kern0_PairRec_
  {
    FT_UShort  left;   
    FT_UShort  right;  
    FT_FWord   value;  

  } TT_Kern0_PairRec, *TT_Kern0_Pair;

#endif 


  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_SBit_MetricsRec_
  {
    FT_Byte  height;
    FT_Byte  width;

    FT_Char  horiBearingX;
    FT_Char  horiBearingY;
    FT_Byte  horiAdvance;

    FT_Char  vertBearingX;
    FT_Char  vertBearingY;
    FT_Byte  vertAdvance;

  } TT_SBit_MetricsRec, *TT_SBit_Metrics;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_SBit_Small_Metrics_
  {
    FT_Byte  height;
    FT_Byte  width;

    FT_Char  bearingX;
    FT_Char  bearingY;
    FT_Byte  advance;

  } TT_SBit_SmallMetricsRec, *TT_SBit_SmallMetrics;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_SBit_LineMetricsRec_
  {
    FT_Char  ascender;
    FT_Char  descender;
    FT_Byte  max_width;
    FT_Char  caret_slope_numerator;
    FT_Char  caret_slope_denominator;
    FT_Char  caret_offset;
    FT_Char  min_origin_SB;
    FT_Char  min_advance_SB;
    FT_Char  max_before_BL;
    FT_Char  min_after_BL;
    FT_Char  pads[2];

  } TT_SBit_LineMetricsRec, *TT_SBit_LineMetrics;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_SBit_RangeRec_
  {
    FT_UShort           first_glyph;
    FT_UShort           last_glyph;

    FT_UShort           index_format;
    FT_UShort           image_format;
    FT_ULong            image_offset;

    FT_ULong            image_size;
    TT_SBit_MetricsRec  metrics;
    FT_ULong            num_glyphs;

    FT_ULong*           glyph_offsets;
    FT_UShort*          glyph_codes;

    FT_ULong            table_offset;

  } TT_SBit_RangeRec, *TT_SBit_Range;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_SBit_StrikeRec_
  {
    FT_Int                  num_ranges;
    TT_SBit_Range           sbit_ranges;
    FT_ULong                ranges_offset;

    FT_ULong                color_ref;

    TT_SBit_LineMetricsRec  hori;
    TT_SBit_LineMetricsRec  vert;

    FT_UShort               start_glyph;
    FT_UShort               end_glyph;

    FT_Byte                 x_ppem;
    FT_Byte                 y_ppem;

    FT_Byte                 bit_depth;
    FT_Char                 flags;

  } TT_SBit_StrikeRec, *TT_SBit_Strike;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_SBit_ComponentRec_
  {
    FT_UShort  glyph_code;
    FT_Char    x_offset;
    FT_Char    y_offset;

  } TT_SBit_ComponentRec, *TT_SBit_Component;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_SBit_ScaleRec_
  {
    TT_SBit_LineMetricsRec  hori;
    TT_SBit_LineMetricsRec  vert;

    FT_Byte                 x_ppem;
    FT_Byte                 y_ppem;

    FT_Byte                 x_ppem_substitute;
    FT_Byte                 y_ppem_substitute;

  } TT_SBit_ScaleRec, *TT_SBit_Scale;


  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_Post_20Rec_
  {
    FT_UShort   num_glyphs;
    FT_UShort   num_names;
    FT_UShort*  glyph_indices;
    FT_Char**   glyph_names;

  } TT_Post_20Rec, *TT_Post_20;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_Post_25_
  {
    FT_UShort  num_glyphs;
    FT_Char*   offsets;

  } TT_Post_25Rec, *TT_Post_25;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_Post_NamesRec_
  {
    FT_Bool  loaded;

    union
    {
      TT_Post_20Rec  format_20;
      TT_Post_25Rec  format_25;

    } names;

  } TT_Post_NamesRec, *TT_Post_Names;


  
  
  
  
  
  
  
  
  
  
  


#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
  typedef struct GX_BlendRec_  *GX_Blend;
#endif

  
  
  
  
  
  
  
  
  
  
  

  







































#ifdef TT_CONFIG_OPTION_BDF

  typedef struct  TT_BDFRec_
  {
    FT_Byte*   table;
    FT_Byte*   table_end;
    FT_Byte*   strings;
    FT_UInt32  strings_size;
    FT_UInt    num_strikes;
    FT_Bool    loaded;

  } TT_BDFRec, *TT_BDF;

#endif 

  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct TT_FaceRec_*  TT_Face;


  
  typedef FT_Error
  (*TT_Interpreter)( void*  exec_context );

  
  typedef struct TT_LoaderRec_*  TT_Loader;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Loader_GotoTableFunc)( TT_Face    face,
                              FT_ULong   tag,
                              FT_Stream  stream,
                              FT_ULong*  length );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Loader_StartGlyphFunc)( TT_Loader  loader,
                               FT_UInt    glyph_index,
                               FT_ULong   offset,
                               FT_UInt    byte_count );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*TT_Loader_ReadGlyphFunc)( TT_Loader  loader );


  
  
  
  
  
  
  
  
  
  
  
  typedef void
  (*TT_Loader_EndGlyphFunc)( TT_Loader  loader );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_FaceRec_
  {
    FT_FaceRec            root;

    TTC_HeaderRec         ttc_header;

    FT_ULong              format_tag;
    FT_UShort             num_tables;
    TT_Table              dir_tables;

    TT_Header             header;       
    TT_HoriHeader         horizontal;   

    TT_MaxProfile         max_profile;
#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    FT_ULong              max_components;  
#endif

    FT_Bool               vertical_info;
    TT_VertHeader         vertical;     

    FT_UShort             num_names;    
    TT_NameTableRec       name_table;   

    TT_OS2                os2;          
    TT_Postscript         postscript;   

    FT_Byte*              cmap_table;   
    FT_ULong              cmap_size;

    TT_Loader_GotoTableFunc   goto_table;

    TT_Loader_StartGlyphFunc  access_glyph_frame;
    TT_Loader_EndGlyphFunc    forget_glyph_frame;
    TT_Loader_ReadGlyphFunc   read_glyph_header;
    TT_Loader_ReadGlyphFunc   read_simple_glyph;
    TT_Loader_ReadGlyphFunc   read_composite_glyph;

    
    
    void*                 sfnt;

    
    
    void*                 psnames;


    
    
    
    
    

    
#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    TT_HdmxRec            hdmx;
#endif

    
    TT_GaspRec            gasp;                 

    
    TT_PCLT               pclt;

    
#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    FT_ULong              num_sbit_strikes;
    TT_SBit_Strike        sbit_strikes;
#endif

    FT_ULong              num_sbit_scales;
    TT_SBit_Scale         sbit_scales;

    
    TT_Post_NamesRec      postscript_names;


    
    
    
    
    

    
#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    FT_UShort             num_locations_stub;
    FT_Long*              glyph_locations_stub;
#endif

    
    FT_ULong              font_program_size;
    FT_Byte*              font_program;

    
    FT_ULong              cvt_program_size;
    FT_Byte*              cvt_program;

    
    FT_ULong              cvt_size;
    FT_Short*             cvt;

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    
    FT_Int                num_kern_pairs;
    FT_Int                kern_table_index;
    TT_Kern0_Pair         kern_pairs;
#endif

    
    
    TT_Interpreter        interpreter;

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    
    FT_Bool               unpatented_hinting;
#endif

    
    
    
    
    
    

    FT_Generic            extra;

    const char*           postscript_name;

    
    
    FT_ULong              glyf_len;

    
#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
    FT_Bool               doblend;
    GX_Blend              blend;
#endif

    

    FT_Byte*              horz_metrics;
    FT_ULong              horz_metrics_size;

    FT_Byte*              vert_metrics;
    FT_ULong              vert_metrics_size;

    FT_UInt               num_locations;
    FT_Byte*              glyph_locations;

    FT_Byte*              hdmx_table;
    FT_ULong              hdmx_table_size;
    FT_UInt               hdmx_record_count;
    FT_ULong              hdmx_record_size;
    FT_Byte*              hdmx_record_sizes;

    FT_Byte*              sbit_table;
    FT_ULong              sbit_table_size;
    FT_UInt               sbit_num_strikes;

    FT_Byte*              kern_table;
    FT_ULong              kern_table_size;
    FT_UInt               num_kern_tables;
    FT_UInt32             kern_avail_bits;
    FT_UInt32             kern_order_bits;

#ifdef TT_CONFIG_OPTION_BDF
    TT_BDFRec             bdf;
#endif 

    
    FT_ULong              horz_metrics_offset;
    FT_ULong              vert_metrics_offset;

  } TT_FaceRec;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_GlyphZoneRec_
  {
    FT_Memory   memory;
    FT_UShort   max_points;
    FT_UShort   max_contours;
    FT_UShort   n_points;    
    FT_Short    n_contours;  

    FT_Vector*  org;         
    FT_Vector*  cur;         
    FT_Vector*  orus;        

    FT_Byte*    tags;        
    FT_UShort*  contours;    

    FT_UShort   first_point; 

  } TT_GlyphZoneRec, *TT_GlyphZone;


  
  typedef struct TT_ExecContextRec_*  TT_ExecContext;

  
  typedef struct  TT_LoaderRec_
  {
    FT_Face          face;
    FT_Size          size;
    FT_GlyphSlot     glyph;
    FT_GlyphLoader   gloader;

    FT_ULong         load_flags;
    FT_UInt          glyph_index;

    FT_Stream        stream;
    FT_Int           byte_len;

    FT_Short         n_contours;
    FT_BBox          bbox;
    FT_Int           left_bearing;
    FT_Int           advance;
    FT_Int           linear;
    FT_Bool          linear_def;
    FT_Bool          preserve_pps;
    FT_Vector        pp1;
    FT_Vector        pp2;

    FT_ULong         glyf_offset;

    
    TT_GlyphZoneRec  base;
    TT_GlyphZoneRec  zone;

    TT_ExecContext   exec;
    FT_Byte*         instructions;
    FT_ULong         ins_pos;

    
    void*            other;

    
    FT_Int           top_bearing;
    FT_Int           vadvance;
    FT_Vector        pp3;
    FT_Vector        pp4;

    
    FT_Byte*         cursor;
    FT_Byte*         limit;

  } TT_LoaderRec;


FT_END_HEADER

#endif 




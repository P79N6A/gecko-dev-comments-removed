


















#ifndef __T1TYPES_H__
#define __T1TYPES_H__


#include <ft2build.h>
#include FT_TYPE1_TABLES_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H
#include FT_INTERNAL_SERVICE_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H


FT_BEGIN_HEADER


  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  T1_EncodingRecRec_
  {
    FT_Int       num_chars;
    FT_Int       code_first;
    FT_Int       code_last;

    FT_UShort*   char_index;
    FT_String**  char_name;

  } T1_EncodingRec, *T1_Encoding;


  typedef enum  T1_EncodingType_
  {
    T1_ENCODING_TYPE_NONE = 0,
    T1_ENCODING_TYPE_ARRAY,
    T1_ENCODING_TYPE_STANDARD,
    T1_ENCODING_TYPE_ISOLATIN1,
    T1_ENCODING_TYPE_EXPERT

  } T1_EncodingType;


  typedef struct  T1_FontRec_
  {
    PS_FontInfoRec   font_info;         
    PS_PrivateRec    private_dict;      
    FT_String*       font_name;         

    T1_EncodingType  encoding_type;
    T1_EncodingRec   encoding;

    FT_Byte*         subrs_block;
    FT_Byte*         charstrings_block;
    FT_Byte*         glyph_names_block;

    FT_Int           num_subrs;
    FT_Byte**        subrs;
    FT_PtrDist*      subrs_len;

    FT_Int           num_glyphs;
    FT_String**      glyph_names;       
    FT_Byte**        charstrings;       
    FT_PtrDist*      charstrings_len;

    FT_Byte          paint_type;
    FT_Byte          font_type;
    FT_Matrix        font_matrix;
    FT_Vector        font_offset;
    FT_BBox          font_bbox;
    FT_Long          font_id;

    FT_Fixed         stroke_width;

  } T1_FontRec, *T1_Font;


  typedef struct  CID_SubrsRec_
  {
    FT_UInt    num_subrs;
    FT_Byte**  code;

  } CID_SubrsRec, *CID_Subrs;


  
  
  
  
  
  
  
  
  
  
  

  typedef struct  AFM_TrackKernRec_
  {
    FT_Int    degree;
    FT_Fixed  min_ptsize;
    FT_Fixed  min_kern;
    FT_Fixed  max_ptsize;
    FT_Fixed  max_kern;

  } AFM_TrackKernRec, *AFM_TrackKern;

  typedef struct  AFM_KernPairRec_
  {
    FT_Int  index1;
    FT_Int  index2;
    FT_Int  x;
    FT_Int  y;

  } AFM_KernPairRec, *AFM_KernPair;

  typedef struct  AFM_FontInfoRec_
  {
    FT_Bool        IsCIDFont;
    FT_BBox        FontBBox;
    FT_Fixed       Ascender;
    FT_Fixed       Descender;
    AFM_TrackKern  TrackKerns;   
    FT_Int         NumTrackKern;
    AFM_KernPair   KernPairs;    
    FT_Int         NumKernPair;

  } AFM_FontInfoRec, *AFM_FontInfo;


  
  
  
  
  
  
  
  
  
  
  


  typedef struct T1_FaceRec_*   T1_Face;
  typedef struct CID_FaceRec_*  CID_Face;


  typedef struct  T1_FaceRec_
  {
    FT_FaceRec      root;
    T1_FontRec      type1;
    const void*     psnames;
    const void*     psaux;
    const void*     afm_data;
    FT_CharMapRec   charmaprecs[2];
    FT_CharMap      charmaps[2];

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    PS_Unicodes     unicode_map;
#endif

    
    PS_Blend        blend;

    
    
    
    FT_Int           ndv_idx;
    FT_Int           cdv_idx;

    
    
    FT_UInt          len_buildchar;
    FT_Int*          buildchar;

    
    const void*     pshinter;

  } T1_FaceRec;


  typedef struct  CID_FaceRec_
  {
    FT_FaceRec       root;
    void*            psnames;
    void*            psaux;
    CID_FaceInfoRec  cid;
    void*            afm_data;
    CID_Subrs        subrs;

    
    void*            pshinter;

    
    FT_Byte*         binary_data; 
    FT_Stream        cid_stream;

  } CID_FaceRec;


FT_END_HEADER

#endif 




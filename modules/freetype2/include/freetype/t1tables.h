


















#ifndef __T1TABLES_H__
#define __T1TABLES_H__


#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef FREETYPE_H
#error "freetype.h of FreeType 1 has been loaded!"
#error "Please fix the directory search order for header files"
#error "so that freetype.h of FreeType 2 is found first."
#endif


FT_BEGIN_HEADER


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  
  


  
  
  
  
  
  
  
  
  
  
  typedef struct  PS_FontInfoRec_
  {
    FT_String*  version;
    FT_String*  notice;
    FT_String*  full_name;
    FT_String*  family_name;
    FT_String*  weight;
    FT_Long     italic_angle;
    FT_Bool     is_fixed_pitch;
    FT_Short    underline_position;
    FT_UShort   underline_thickness;

  } PS_FontInfoRec;


  
  
  
  
  
  
  
  
  typedef struct PS_FontInfoRec_*  PS_FontInfo;


  
  
  
  
  
  
  
  
  
  
  typedef PS_FontInfoRec  T1_FontInfo;


  
  
  
  
  
  
  
  
  
  
  typedef struct  PS_PrivateRec_
  {
    FT_Int     unique_id;
    FT_Int     lenIV;

    FT_Byte    num_blue_values;
    FT_Byte    num_other_blues;
    FT_Byte    num_family_blues;
    FT_Byte    num_family_other_blues;

    FT_Short   blue_values[14];
    FT_Short   other_blues[10];

    FT_Short   family_blues      [14];
    FT_Short   family_other_blues[10];

    FT_Fixed   blue_scale;
    FT_Int     blue_shift;
    FT_Int     blue_fuzz;

    FT_UShort  standard_width[1];
    FT_UShort  standard_height[1];

    FT_Byte    num_snap_widths;
    FT_Byte    num_snap_heights;
    FT_Bool    force_bold;
    FT_Bool    round_stem_up;

    FT_Short   snap_widths [13];  
    FT_Short   snap_heights[13];  

    FT_Fixed   expansion_factor;

    FT_Long    language_group;
    FT_Long    password;

    FT_Short   min_feature[2];

  } PS_PrivateRec;


  
  
  
  
  
  
  
  
  typedef struct PS_PrivateRec_*  PS_Private;


  
  
  
  
  
  
  
  
  
  
  typedef PS_PrivateRec  T1_Private;


  
  
  
  
  
  
  
  
  
  
  typedef enum  T1_Blend_Flags_
  {
    
    T1_BLEND_UNDERLINE_POSITION = 0,
    T1_BLEND_UNDERLINE_THICKNESS,
    T1_BLEND_ITALIC_ANGLE,

    
    T1_BLEND_BLUE_VALUES,
    T1_BLEND_OTHER_BLUES,
    T1_BLEND_STANDARD_WIDTH,
    T1_BLEND_STANDARD_HEIGHT,
    T1_BLEND_STEM_SNAP_WIDTHS,
    T1_BLEND_STEM_SNAP_HEIGHTS,
    T1_BLEND_BLUE_SCALE,
    T1_BLEND_BLUE_SHIFT,
    T1_BLEND_FAMILY_BLUES,
    T1_BLEND_FAMILY_OTHER_BLUES,
    T1_BLEND_FORCE_BOLD,

    
    T1_BLEND_MAX

  } T1_Blend_Flags;

  


  
#define t1_blend_underline_position   T1_BLEND_UNDERLINE_POSITION
#define t1_blend_underline_thickness  T1_BLEND_UNDERLINE_THICKNESS
#define t1_blend_italic_angle         T1_BLEND_ITALIC_ANGLE
#define t1_blend_blue_values          T1_BLEND_BLUE_VALUES
#define t1_blend_other_blues          T1_BLEND_OTHER_BLUES
#define t1_blend_standard_widths      T1_BLEND_STANDARD_WIDTH
#define t1_blend_standard_height      T1_BLEND_STANDARD_HEIGHT
#define t1_blend_stem_snap_widths     T1_BLEND_STEM_SNAP_WIDTHS
#define t1_blend_stem_snap_heights    T1_BLEND_STEM_SNAP_HEIGHTS
#define t1_blend_blue_scale           T1_BLEND_BLUE_SCALE
#define t1_blend_blue_shift           T1_BLEND_BLUE_SHIFT
#define t1_blend_family_blues         T1_BLEND_FAMILY_BLUES
#define t1_blend_family_other_blues   T1_BLEND_FAMILY_OTHER_BLUES
#define t1_blend_force_bold           T1_BLEND_FORCE_BOLD
#define t1_blend_max                  T1_BLEND_MAX


  
#define T1_MAX_MM_DESIGNS     16

  
#define T1_MAX_MM_AXIS        4

  
#define T1_MAX_MM_MAP_POINTS  20


  
  typedef struct  PS_DesignMap_
  {
    FT_Byte    num_points;
    FT_Long*   design_points;
    FT_Fixed*  blend_points;

  } PS_DesignMapRec, *PS_DesignMap;

  
  typedef PS_DesignMapRec  T1_DesignMap;


  typedef struct  PS_BlendRec_
  {
    FT_UInt          num_designs;
    FT_UInt          num_axis;

    FT_String*       axis_names[T1_MAX_MM_AXIS];
    FT_Fixed*        design_pos[T1_MAX_MM_DESIGNS];
    PS_DesignMapRec  design_map[T1_MAX_MM_AXIS];

    FT_Fixed*        weight_vector;
    FT_Fixed*        default_weight_vector;

    PS_FontInfo      font_infos[T1_MAX_MM_DESIGNS + 1];
    PS_Private       privates  [T1_MAX_MM_DESIGNS + 1];

    FT_ULong         blend_bitflags;

    FT_BBox*         bboxes    [T1_MAX_MM_DESIGNS + 1];

    

    
    
    
    
    FT_UInt          default_design_vector[T1_MAX_MM_DESIGNS];
    FT_UInt          num_default_design_vector;

  } PS_BlendRec, *PS_Blend;


  
  typedef PS_BlendRec  T1_Blend;


  
  
  
  
  
  
  
  
  typedef struct  CID_FaceDictRec_
  {
    PS_PrivateRec  private_dict;

    FT_UInt        len_buildchar;
    FT_Fixed       forcebold_threshold;
    FT_Pos         stroke_width;
    FT_Fixed       expansion_factor;

    FT_Byte        paint_type;
    FT_Byte        font_type;
    FT_Matrix      font_matrix;
    FT_Vector      font_offset;

    FT_UInt        num_subrs;
    FT_ULong       subrmap_offset;
    FT_Int         sd_bytes;

  } CID_FaceDictRec;


  
  
  
  
  
  
  
  
  typedef struct CID_FaceDictRec_*  CID_FaceDict;

  


  
  typedef CID_FaceDictRec  CID_FontDict;


  
  
  
  
  
  
  
  
  typedef struct  CID_FaceInfoRec_
  {
    FT_String*      cid_font_name;
    FT_Fixed        cid_version;
    FT_Int          cid_font_type;

    FT_String*      registry;
    FT_String*      ordering;
    FT_Int          supplement;

    PS_FontInfoRec  font_info;
    FT_BBox         font_bbox;
    FT_ULong        uid_base;

    FT_Int          num_xuid;
    FT_ULong        xuid[16];

    FT_ULong        cidmap_offset;
    FT_Int          fd_bytes;
    FT_Int          gd_bytes;
    FT_ULong        cid_count;

    FT_Int          num_dicts;
    CID_FaceDict    font_dicts;

    FT_ULong        data_offset;

  } CID_FaceInfoRec;


  
  
  
  
  
  
  
  
  typedef struct CID_FaceInfoRec_*  CID_FaceInfo;


  
  
  
  
  
  
  
  
  
  
  typedef CID_FaceInfoRec  CID_Info;


  





















  FT_EXPORT( FT_Int )
  FT_Has_PS_Glyph_Names( FT_Face  face );


  



























  FT_EXPORT( FT_Error )
  FT_Get_PS_Font_Info( FT_Face      face,
                       PS_FontInfo  afont_info );


  



























  FT_EXPORT( FT_Error )
  FT_Get_PS_Font_Private( FT_Face     face,
                          PS_Private  afont_private );

  


FT_END_HEADER

#endif 




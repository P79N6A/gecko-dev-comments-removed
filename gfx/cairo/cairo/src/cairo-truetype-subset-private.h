



































#ifndef CAIRO_TRUETYPE_SUBSET_PRIVATE_H
#define CAIRO_TRUETYPE_SUBSET_PRIVATE_H

#include "cairoint.h"












#define MAKE_TT_TAG(a, b, c, d)    (a<<24 | b<<16 | c<<8 | d)
#define TT_TAG_CFF    MAKE_TT_TAG('C','F','F',' ')
#define TT_TAG_cmap   MAKE_TT_TAG('c','m','a','p')
#define TT_TAG_cvt    MAKE_TT_TAG('c','v','t',' ')
#define TT_TAG_fpgm   MAKE_TT_TAG('f','p','g','m')
#define TT_TAG_glyf   MAKE_TT_TAG('g','l','y','f')
#define TT_TAG_head   MAKE_TT_TAG('h','e','a','d')
#define TT_TAG_hhea   MAKE_TT_TAG('h','h','e','a')
#define TT_TAG_hmtx   MAKE_TT_TAG('h','m','t','x')
#define TT_TAG_loca   MAKE_TT_TAG('l','o','c','a')
#define TT_TAG_maxp   MAKE_TT_TAG('m','a','x','p')
#define TT_TAG_name   MAKE_TT_TAG('n','a','m','e')
#define TT_TAG_prep   MAKE_TT_TAG('p','r','e','p')


typedef struct _tt_head {
    int16_t     version_1;
    int16_t     version_2;
    int16_t     revision_1;
    int16_t     revision_2;
    uint16_t    checksum_1;
    uint16_t    checksum_2;
    uint16_t    magic_1;
    uint16_t    magic_2;
    uint16_t    flags;
    uint16_t    units_per_em;
    int16_t     created_1;
    int16_t     created_2;
    int16_t     created_3;
    int16_t     created_4;
    int16_t     modified_1;
    int16_t     modified_2;
    int16_t     modified_3;
    int16_t     modified_4;
    int16_t     x_min;                  
    int16_t     y_min;                  
    int16_t     x_max;                  
    int16_t     y_max;                  
    uint16_t    mac_style;
    uint16_t    lowest_rec_pppem;
    int16_t     font_direction_hint;
    int16_t     index_to_loc_format;
    int16_t     glyph_data_format;
} tt_head_t;

typedef struct _tt_hhea {
    int16_t     version_1;
    int16_t     version_2;
    int16_t     ascender;               
    int16_t     descender;              
    int16_t     line_gap;               
    uint16_t    advance_max_width;      
    int16_t     min_left_side_bearing;  
    int16_t     min_right_side_bearing; 
    int16_t     x_max_extent;           
    int16_t     caret_slope_rise;
    int16_t     caret_slope_run;
    int16_t     reserved[5];
    int16_t     metric_data_format;
    uint16_t    num_hmetrics;
} tt_hhea_t;

typedef struct _tt_maxp {
    int16_t     version_1;
    int16_t     version_2;
    uint16_t    num_glyphs;
    uint16_t    max_points;
    uint16_t    max_contours;
    uint16_t    max_composite_points;
    uint16_t    max_composite_contours;
    uint16_t    max_zones;
    uint16_t    max_twilight_points;
    uint16_t    max_storage;
    uint16_t    max_function_defs;
    uint16_t    max_instruction_defs;
    uint16_t    max_stack_elements;
    uint16_t    max_size_of_instructions;
    uint16_t    max_component_elements;
    uint16_t    max_component_depth;
} tt_maxp_t;

typedef struct _tt_name_record {
    uint16_t platform;
    uint16_t encoding;
    uint16_t language;
    uint16_t name;
    uint16_t length;
    uint16_t offset;
} tt_name_record_t;

typedef struct _tt_name {
    uint16_t   format;
    uint16_t   num_records;
    uint16_t   strings_offset;
    tt_name_record_t records[1];
} tt_name_t;




#define TT_ARG_1_AND_2_ARE_WORDS     0x0001
#define TT_WE_HAVE_A_SCALE           0x0008
#define TT_MORE_COMPONENTS           0x0020
#define TT_WE_HAVE_AN_X_AND_Y_SCALE  0x0040
#define TT_WE_HAVE_A_TWO_BY_TWO      0x0080

typedef struct _tt_composite_glyph {
    uint16_t flags;
    uint16_t index;
    uint16_t args[7]; 
} tt_composite_glyph_t;

typedef struct _tt_glyph_data {
    int16_t           num_contours;
    int8_t            data[8];
    tt_composite_glyph_t glyph;
} tt_glyph_data_t;

#endif 

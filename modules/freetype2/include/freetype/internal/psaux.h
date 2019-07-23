


















#ifndef __PSAUX_H__
#define __PSAUX_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_TYPE1_TYPES_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H


FT_BEGIN_HEADER


  
  
  
  
  
  
  


  typedef struct PS_TableRec_*              PS_Table;
  typedef const struct PS_Table_FuncsRec_*  PS_Table_Funcs;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  PS_Table_FuncsRec_
  {
    FT_Error
    (*init)( PS_Table   table,
             FT_Int     count,
             FT_Memory  memory );

    void
    (*done)( PS_Table  table );

    FT_Error
    (*add)( PS_Table    table,
            FT_Int      idx,
            void*       object,
            FT_PtrDist  length );

    void
    (*release)( PS_Table  table );

  } PS_Table_FuncsRec;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  PS_TableRec_
  {
    FT_Byte*           block;          
    FT_Offset          cursor;         
    FT_Offset          capacity;       
    FT_Long            init;

    FT_Int             max_elems;
    FT_Int             num_elems;
    FT_Byte**          elements;       
    FT_PtrDist*        lengths;        

    FT_Memory          memory;
    PS_Table_FuncsRec  funcs;

  } PS_TableRec;


  
  
  
  
  
  
  

  typedef struct PS_ParserRec_*  PS_Parser;

  typedef struct T1_TokenRec_*   T1_Token;

  typedef struct T1_FieldRec_*   T1_Field;


  
  typedef enum  T1_TokenType_
  {
    T1_TOKEN_TYPE_NONE = 0,
    T1_TOKEN_TYPE_ANY,
    T1_TOKEN_TYPE_STRING,
    T1_TOKEN_TYPE_ARRAY,
    T1_TOKEN_TYPE_KEY, 

    
    T1_TOKEN_TYPE_MAX

  } T1_TokenType;


  
  typedef struct  T1_TokenRec_
  {
    FT_Byte*      start;   
    FT_Byte*      limit;   
    T1_TokenType  type;    

  } T1_TokenRec;


  
  typedef enum  T1_FieldType_
  {
    T1_FIELD_TYPE_NONE = 0,
    T1_FIELD_TYPE_BOOL,
    T1_FIELD_TYPE_INTEGER,
    T1_FIELD_TYPE_FIXED,
    T1_FIELD_TYPE_FIXED_1000,
    T1_FIELD_TYPE_STRING,
    T1_FIELD_TYPE_KEY,
    T1_FIELD_TYPE_BBOX,
    T1_FIELD_TYPE_INTEGER_ARRAY,
    T1_FIELD_TYPE_FIXED_ARRAY,
    T1_FIELD_TYPE_CALLBACK,

    
    T1_FIELD_TYPE_MAX

  } T1_FieldType;


  typedef enum  T1_FieldLocation_
  {
    T1_FIELD_LOCATION_CID_INFO,
    T1_FIELD_LOCATION_FONT_DICT,
    T1_FIELD_LOCATION_FONT_INFO,
    T1_FIELD_LOCATION_PRIVATE,
    T1_FIELD_LOCATION_BBOX,
    T1_FIELD_LOCATION_LOADER,
    T1_FIELD_LOCATION_FACE,
    T1_FIELD_LOCATION_BLEND,

    
    T1_FIELD_LOCATION_MAX

  } T1_FieldLocation;


  typedef void
  (*T1_Field_ParseFunc)( FT_Face     face,
                         FT_Pointer  parser );


  
  typedef struct  T1_FieldRec_
  {
    const char*         ident;        
    T1_FieldLocation    location;
    T1_FieldType        type;         
    T1_Field_ParseFunc  reader;
    FT_UInt             offset;       
    FT_Byte             size;         
    FT_UInt             array_max;    
                                      
    FT_UInt             count_offset; 
                                      
    FT_UInt             dict;         
  } T1_FieldRec;

#define T1_FIELD_DICT_FONTDICT ( 1 << 0 ) /* also FontInfo and FDArray */
#define T1_FIELD_DICT_PRIVATE  ( 1 << 1 )



#define T1_NEW_SIMPLE_FIELD( _ident, _type, _fname, _dict ) \
          {                                                 \
            _ident, T1CODE, _type,                          \
            0,                                              \
            FT_FIELD_OFFSET( _fname ),                      \
            FT_FIELD_SIZE( _fname ),                        \
            0, 0,                                           \
            _dict                                           \
          },

#define T1_NEW_CALLBACK_FIELD( _ident, _reader, _dict ) \
          {                                             \
            _ident, T1CODE, T1_FIELD_TYPE_CALLBACK,     \
            (T1_Field_ParseFunc)_reader,                \
            0, 0,                                       \
            0, 0,                                       \
            _dict                                       \
          },

#define T1_NEW_TABLE_FIELD( _ident, _type, _fname, _max, _dict ) \
          {                                                      \
            _ident, T1CODE, _type,                               \
            0,                                                   \
            FT_FIELD_OFFSET( _fname ),                           \
            FT_FIELD_SIZE_DELTA( _fname ),                       \
            _max,                                                \
            FT_FIELD_OFFSET( num_ ## _fname ),                   \
            _dict                                                \
          },

#define T1_NEW_TABLE_FIELD2( _ident, _type, _fname, _max, _dict ) \
          {                                                       \
            _ident, T1CODE, _type,                                \
            0,                                                    \
            FT_FIELD_OFFSET( _fname ),                            \
            FT_FIELD_SIZE_DELTA( _fname ),                        \
            _max, 0,                                              \
            _dict                                                 \
          },


#define T1_FIELD_BOOL( _ident, _fname, _dict )                             \
          T1_NEW_SIMPLE_FIELD( _ident, T1_FIELD_TYPE_BOOL, _fname, _dict )

#define T1_FIELD_NUM( _ident, _fname, _dict )                                 \
          T1_NEW_SIMPLE_FIELD( _ident, T1_FIELD_TYPE_INTEGER, _fname, _dict )

#define T1_FIELD_FIXED( _ident, _fname, _dict )                             \
          T1_NEW_SIMPLE_FIELD( _ident, T1_FIELD_TYPE_FIXED, _fname, _dict )

#define T1_FIELD_FIXED_1000( _ident, _fname, _dict )                     \
          T1_NEW_SIMPLE_FIELD( _ident, T1_FIELD_TYPE_FIXED_1000, _fname, \
                               _dict )

#define T1_FIELD_STRING( _ident, _fname, _dict )                             \
          T1_NEW_SIMPLE_FIELD( _ident, T1_FIELD_TYPE_STRING, _fname, _dict )

#define T1_FIELD_KEY( _ident, _fname, _dict )                             \
          T1_NEW_SIMPLE_FIELD( _ident, T1_FIELD_TYPE_KEY, _fname, _dict )

#define T1_FIELD_BBOX( _ident, _fname, _dict )                             \
          T1_NEW_SIMPLE_FIELD( _ident, T1_FIELD_TYPE_BBOX, _fname, _dict )


#define T1_FIELD_NUM_TABLE( _ident, _fname, _fmax, _dict )         \
          T1_NEW_TABLE_FIELD( _ident, T1_FIELD_TYPE_INTEGER_ARRAY, \
                              _fname, _fmax, _dict )

#define T1_FIELD_FIXED_TABLE( _ident, _fname, _fmax, _dict )     \
          T1_NEW_TABLE_FIELD( _ident, T1_FIELD_TYPE_FIXED_ARRAY, \
                              _fname, _fmax, _dict )

#define T1_FIELD_NUM_TABLE2( _ident, _fname, _fmax, _dict )         \
          T1_NEW_TABLE_FIELD2( _ident, T1_FIELD_TYPE_INTEGER_ARRAY, \
                               _fname, _fmax, _dict )

#define T1_FIELD_FIXED_TABLE2( _ident, _fname, _fmax, _dict )     \
          T1_NEW_TABLE_FIELD2( _ident, T1_FIELD_TYPE_FIXED_ARRAY, \
                               _fname, _fmax, _dict )

#define T1_FIELD_CALLBACK( _ident, _name, _dict )       \
          T1_NEW_CALLBACK_FIELD( _ident, _name, _dict )


  
  
  
  
  
  
  

  typedef const struct PS_Parser_FuncsRec_*  PS_Parser_Funcs;

  typedef struct  PS_Parser_FuncsRec_
  {
    void
    (*init)( PS_Parser  parser,
             FT_Byte*   base,
             FT_Byte*   limit,
             FT_Memory  memory );

    void
    (*done)( PS_Parser  parser );

    void
    (*skip_spaces)( PS_Parser  parser );
    void
    (*skip_PS_token)( PS_Parser  parser );

    FT_Long
    (*to_int)( PS_Parser  parser );
    FT_Fixed
    (*to_fixed)( PS_Parser  parser,
                 FT_Int     power_ten );

    FT_Error
    (*to_bytes)( PS_Parser  parser,
                 FT_Byte*   bytes,
                 FT_Long    max_bytes,
                 FT_Long*   pnum_bytes,
                 FT_Bool    delimiters );

    FT_Int
    (*to_coord_array)( PS_Parser  parser,
                       FT_Int     max_coords,
                       FT_Short*  coords );
    FT_Int
    (*to_fixed_array)( PS_Parser  parser,
                       FT_Int     max_values,
                       FT_Fixed*  values,
                       FT_Int     power_ten );

    void
    (*to_token)( PS_Parser  parser,
                 T1_Token   token );
    void
    (*to_token_array)( PS_Parser  parser,
                       T1_Token   tokens,
                       FT_UInt    max_tokens,
                       FT_Int*    pnum_tokens );

    FT_Error
    (*load_field)( PS_Parser       parser,
                   const T1_Field  field,
                   void**          objects,
                   FT_UInt         max_objects,
                   FT_ULong*       pflags );

    FT_Error
    (*load_field_table)( PS_Parser       parser,
                         const T1_Field  field,
                         void**          objects,
                         FT_UInt         max_objects,
                         FT_ULong*       pflags );

  } PS_Parser_FuncsRec;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  PS_ParserRec_
  {
    FT_Byte*   cursor;
    FT_Byte*   base;
    FT_Byte*   limit;
    FT_Error   error;
    FT_Memory  memory;

    PS_Parser_FuncsRec  funcs;

  } PS_ParserRec;


  
  
  
  
  
  
  


  typedef struct T1_BuilderRec_*  T1_Builder;


  typedef FT_Error
  (*T1_Builder_Check_Points_Func)( T1_Builder  builder,
                                   FT_Int      count );

  typedef void
  (*T1_Builder_Add_Point_Func)( T1_Builder  builder,
                                FT_Pos      x,
                                FT_Pos      y,
                                FT_Byte     flag );

  typedef FT_Error
  (*T1_Builder_Add_Point1_Func)( T1_Builder  builder,
                                 FT_Pos      x,
                                 FT_Pos      y );

  typedef FT_Error
  (*T1_Builder_Add_Contour_Func)( T1_Builder  builder );

  typedef FT_Error
  (*T1_Builder_Start_Point_Func)( T1_Builder  builder,
                                  FT_Pos      x,
                                  FT_Pos      y );

  typedef void
  (*T1_Builder_Close_Contour_Func)( T1_Builder  builder );


  typedef const struct T1_Builder_FuncsRec_*  T1_Builder_Funcs;

  typedef struct  T1_Builder_FuncsRec_
  {
    void
    (*init)( T1_Builder    builder,
             FT_Face       face,
             FT_Size       size,
             FT_GlyphSlot  slot,
             FT_Bool       hinting );

    void
    (*done)( T1_Builder   builder );

    T1_Builder_Check_Points_Func   check_points;
    T1_Builder_Add_Point_Func      add_point;
    T1_Builder_Add_Point1_Func     add_point1;
    T1_Builder_Add_Contour_Func    add_contour;
    T1_Builder_Start_Point_Func    start_point;
    T1_Builder_Close_Contour_Func  close_contour;

  } T1_Builder_FuncsRec;


  
  typedef enum  T1_ParseState_
  {
    T1_Parse_Start,
    T1_Parse_Have_Width,
    T1_Parse_Have_Moveto,
    T1_Parse_Have_Path

  } T1_ParseState;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  T1_BuilderRec_
  {
    FT_Memory       memory;
    FT_Face         face;
    FT_GlyphSlot    glyph;
    FT_GlyphLoader  loader;
    FT_Outline*     base;
    FT_Outline*     current;

    FT_Vector       last;

    FT_Pos          pos_x;
    FT_Pos          pos_y;

    FT_Vector       left_bearing;
    FT_Vector       advance;

    FT_BBox         bbox;          
    T1_ParseState   parse_state;
    FT_Bool         load_points;
    FT_Bool         no_recurse;
    FT_Bool         shift;

    FT_Bool         metrics_only;

    void*           hints_funcs;    
    void*           hints_globals;  

    T1_Builder_FuncsRec  funcs;

  } T1_BuilderRec;


  
  
  
  
  
  
  

#if 0

  
  
  
  
  
#define T1_MAX_SUBRS_CALLS  8


  
  
  
  
  
#define T1_MAX_CHARSTRINGS_OPERANDS  32

#endif 


  typedef struct  T1_Decoder_ZoneRec_
  {
    FT_Byte*  cursor;
    FT_Byte*  base;
    FT_Byte*  limit;

  } T1_Decoder_ZoneRec, *T1_Decoder_Zone;


  typedef struct T1_DecoderRec_*              T1_Decoder;
  typedef const struct T1_Decoder_FuncsRec_*  T1_Decoder_Funcs;


  typedef FT_Error
  (*T1_Decoder_Callback)( T1_Decoder  decoder,
                          FT_UInt     glyph_index );


  typedef struct  T1_Decoder_FuncsRec_
  {
    FT_Error
    (*init)( T1_Decoder           decoder,
             FT_Face              face,
             FT_Size              size,
             FT_GlyphSlot         slot,
             FT_Byte**            glyph_names,
             PS_Blend             blend,
             FT_Bool              hinting,
             FT_Render_Mode       hint_mode,
             T1_Decoder_Callback  callback );

    void
    (*done)( T1_Decoder  decoder );

    FT_Error
    (*parse_charstrings)( T1_Decoder  decoder,
                          FT_Byte*    base,
                          FT_UInt     len );

  } T1_Decoder_FuncsRec;


  typedef struct  T1_DecoderRec_
  {
    T1_BuilderRec        builder;

    FT_Long              stack[T1_MAX_CHARSTRINGS_OPERANDS];
    FT_Long*             top;

    T1_Decoder_ZoneRec   zones[T1_MAX_SUBRS_CALLS + 1];
    T1_Decoder_Zone      zone;

    FT_Service_PsCMaps   psnames;      
    FT_UInt              num_glyphs;
    FT_Byte**            glyph_names;

    FT_Int               lenIV;        
    FT_UInt              num_subrs;
    FT_Byte**            subrs;
    FT_PtrDist*          subrs_len;    

    FT_Matrix            font_matrix;
    FT_Vector            font_offset;

    FT_Int               flex_state;
    FT_Int               num_flex_vectors;
    FT_Vector            flex_vectors[7];

    PS_Blend             blend;       

    FT_Render_Mode       hint_mode;

    T1_Decoder_Callback  parse_callback;
    T1_Decoder_FuncsRec  funcs;

    FT_Int*              buildchar;
    FT_UInt              len_buildchar;

  } T1_DecoderRec;


  
  
  
  
  
  
  

  typedef struct AFM_ParserRec_*  AFM_Parser;

  typedef struct  AFM_Parser_FuncsRec_
  {
    FT_Error
    (*init)( AFM_Parser  parser,
             FT_Memory   memory,
             FT_Byte*    base,
             FT_Byte*    limit );

    void
    (*done)( AFM_Parser  parser );

    FT_Error
    (*parse)( AFM_Parser  parser );

  } AFM_Parser_FuncsRec;


  typedef struct AFM_StreamRec_*  AFM_Stream;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  AFM_ParserRec_
  {
    FT_Memory     memory;
    AFM_Stream    stream;

    AFM_FontInfo  FontInfo;

    FT_Int
    (*get_index)( const char*  name,
                  FT_UInt      len,
                  void*        user_data );

    void*         user_data;

  } AFM_ParserRec;


  
  
  
  
  
  
  

  typedef const struct T1_CMap_ClassesRec_*  T1_CMap_Classes;

  typedef struct T1_CMap_ClassesRec_
  {
    FT_CMap_Class  standard;
    FT_CMap_Class  expert;
    FT_CMap_Class  custom;
    FT_CMap_Class  unicode;

  } T1_CMap_ClassesRec;


  
  
  
  
  
  
  

  typedef struct  PSAux_ServiceRec_
  {
    
    const PS_Table_FuncsRec*    ps_table_funcs;
    const PS_Parser_FuncsRec*   ps_parser_funcs;
    const T1_Builder_FuncsRec*  t1_builder_funcs;
    const T1_Decoder_FuncsRec*  t1_decoder_funcs;

    void
    (*t1_decrypt)( FT_Byte*   buffer,
                   FT_Offset  length,
                   FT_UShort  seed );

    T1_CMap_Classes  t1_cmap_classes;

    
    const AFM_Parser_FuncsRec*  afm_parser_funcs;

  } PSAux_ServiceRec, *PSAux_Service;

  
  typedef PSAux_ServiceRec   PSAux_Interface;


  
  
  
  
  
  
  

#define IS_PS_NEWLINE( ch ) \
  ( (ch) == '\r' ||         \
    (ch) == '\n' )

#define IS_PS_SPACE( ch )  \
  ( (ch) == ' '         || \
    IS_PS_NEWLINE( ch ) || \
    (ch) == '\t'        || \
    (ch) == '\f'        || \
    (ch) == '\0' )

#define IS_PS_SPECIAL( ch )       \
  ( (ch) == '/'                || \
    (ch) == '(' || (ch) == ')' || \
    (ch) == '<' || (ch) == '>' || \
    (ch) == '[' || (ch) == ']' || \
    (ch) == '{' || (ch) == '}' || \
    (ch) == '%'                )

#define IS_PS_DELIM( ch )  \
  ( IS_PS_SPACE( ch )   || \
    IS_PS_SPECIAL( ch ) )

#define IS_PS_DIGIT( ch )        \
  ( (ch) >= '0' && (ch) <= '9' )

#define IS_PS_XDIGIT( ch )            \
  ( IS_PS_DIGIT( ch )              || \
    ( (ch) >= 'A' && (ch) <= 'F' ) || \
    ( (ch) >= 'a' && (ch) <= 'f' ) )

#define IS_PS_BASE85( ch )       \
  ( (ch) >= '!' && (ch) <= 'u' )

#define IS_PS_TOKEN( cur, limit, token )                                \
  ( (char)(cur)[0] == (token)[0]                                     && \
    ( (cur) + sizeof ( (token) ) == (limit) ||                          \
      ( (cur) + sizeof( (token) ) < (limit)          &&                 \
        IS_PS_DELIM( (cur)[sizeof ( (token) ) - 1] ) ) )             && \
    ft_strncmp( (char*)(cur), (token), sizeof ( (token) ) - 1 ) == 0 )


FT_END_HEADER

#endif 




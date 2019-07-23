
















#include "t42parse.h"
#include "t42error.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_LIST_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t42


  static void
  t42_parse_font_matrix( T42_Face    face,
                         T42_Loader  loader );
  static void
  t42_parse_encoding( T42_Face    face,
                      T42_Loader  loader );

  static void
  t42_parse_charstrings( T42_Face    face,
                         T42_Loader  loader );

  static void
  t42_parse_sfnts( T42_Face    face,
                   T42_Loader  loader );


  
  
  static const
  T1_FieldRec  t42_keywords[] = {

#undef  FT_STRUCTURE
#define FT_STRUCTURE  T1_FontInfo
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_FONT_INFO

    T1_FIELD_STRING( "version",            version,             0 )
    T1_FIELD_STRING( "Notice",             notice,              0 )
    T1_FIELD_STRING( "FullName",           full_name,           0 )
    T1_FIELD_STRING( "FamilyName",         family_name,         0 )
    T1_FIELD_STRING( "Weight",             weight,              0 )
    T1_FIELD_NUM   ( "ItalicAngle",        italic_angle,        0 )
    T1_FIELD_BOOL  ( "isFixedPitch",       is_fixed_pitch,      0 )
    T1_FIELD_NUM   ( "UnderlinePosition",  underline_position,  0 )
    T1_FIELD_NUM   ( "UnderlineThickness", underline_thickness, 0 )

#undef  FT_STRUCTURE
#define FT_STRUCTURE  T1_FontRec
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_FONT_DICT

    T1_FIELD_KEY  ( "FontName",    font_name,    0 )
    T1_FIELD_NUM  ( "PaintType",   paint_type,   0 )
    T1_FIELD_NUM  ( "FontType",    font_type,    0 )
    T1_FIELD_FIXED( "StrokeWidth", stroke_width, 0 )

#undef  FT_STRUCTURE
#define FT_STRUCTURE  FT_BBox
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_BBOX

    T1_FIELD_BBOX("FontBBox", xMin, 0 )

    T1_FIELD_CALLBACK( "FontMatrix",  t42_parse_font_matrix, 0 )
    T1_FIELD_CALLBACK( "Encoding",    t42_parse_encoding,    0 )
    T1_FIELD_CALLBACK( "CharStrings", t42_parse_charstrings, 0 )
    T1_FIELD_CALLBACK( "sfnts",       t42_parse_sfnts,       0 )

    { 0, T1_FIELD_LOCATION_CID_INFO, T1_FIELD_TYPE_NONE, 0, 0, 0, 0, 0, 0 }
  };


#define T1_Add_Table( p, i, o, l )  (p)->funcs.add( (p), i, o, l )
#define T1_Done_Table( p )          \
          do                        \
          {                         \
            if ( (p)->funcs.done )  \
              (p)->funcs.done( p ); \
          } while ( 0 )
#define T1_Release_Table( p )          \
          do                           \
          {                            \
            if ( (p)->funcs.release )  \
              (p)->funcs.release( p ); \
          } while ( 0 )

#define T1_Skip_Spaces( p )    (p)->root.funcs.skip_spaces( &(p)->root )
#define T1_Skip_PS_Token( p )  (p)->root.funcs.skip_PS_token( &(p)->root )

#define T1_ToInt( p )                          \
          (p)->root.funcs.to_int( &(p)->root )
#define T1_ToBytes( p, b, m, n, d )                          \
          (p)->root.funcs.to_bytes( &(p)->root, b, m, n, d )

#define T1_ToFixedArray( p, m, f, t )                           \
          (p)->root.funcs.to_fixed_array( &(p)->root, m, f, t )
#define T1_ToToken( p, t )                          \
          (p)->root.funcs.to_token( &(p)->root, t )

#define T1_Load_Field( p, f, o, m, pf )                         \
          (p)->root.funcs.load_field( &(p)->root, f, o, m, pf )
#define T1_Load_Field_Table( p, f, o, m, pf )                         \
          (p)->root.funcs.load_field_table( &(p)->root, f, o, m, pf )


  

  FT_LOCAL_DEF( FT_Error )
  t42_parser_init( T42_Parser     parser,
                   FT_Stream      stream,
                   FT_Memory      memory,
                   PSAux_Service  psaux )
  {
    FT_Error  error = T42_Err_Ok;
    FT_Long   size;


    psaux->ps_parser_funcs->init( &parser->root, 0, 0, memory );

    parser->stream    = stream;
    parser->base_len  = 0;
    parser->base_dict = 0;
    parser->in_memory = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    

    if ( FT_STREAM_SEEK( 0L ) ||
         FT_FRAME_ENTER( 17 ) )
      goto Exit;

    if ( ft_memcmp( stream->cursor, "%!PS-TrueTypeFont", 17 ) != 0 )
    {
      FT_TRACE2(( "not a Type42 font\n" ));
      error = T42_Err_Unknown_File_Format;
    }

    FT_FRAME_EXIT();

    if ( error || FT_STREAM_SEEK( 0 ) )
      goto Exit;

    size = stream->size;

    
    

    
    if ( !stream->read )
    {
      parser->base_dict = (FT_Byte*)stream->base + stream->pos;
      parser->base_len  = size;
      parser->in_memory = 1;

      
      if ( FT_STREAM_SKIP( size ) )
        goto Exit;
    }
    else
    {
      
      if ( FT_ALLOC( parser->base_dict, size )       ||
           FT_STREAM_READ( parser->base_dict, size ) )
        goto Exit;

      parser->base_len = size;
    }

    parser->root.base   = parser->base_dict;
    parser->root.cursor = parser->base_dict;
    parser->root.limit  = parser->root.cursor + parser->base_len;

  Exit:
    if ( error && !parser->in_memory )
      FT_FREE( parser->base_dict );

    return error;
  }


  FT_LOCAL_DEF( void )
  t42_parser_done( T42_Parser  parser )
  {
    FT_Memory  memory = parser->root.memory;


    
    if ( !parser->in_memory )
      FT_FREE( parser->base_dict );

    parser->root.funcs.done( &parser->root );
  }


  static int
  t42_is_space( FT_Byte  c )
  {
    return ( c == ' '  || c == '\t'              ||
             c == '\r' || c == '\n' || c == '\f' ||
             c == '\0'                           );
  }


  static void
  t42_parse_font_matrix( T42_Face    face,
                         T42_Loader  loader )
  {
    T42_Parser  parser = &loader->parser;
    FT_Matrix*  matrix = &face->type1.font_matrix;
    FT_Vector*  offset = &face->type1.font_offset;
    FT_Face     root   = (FT_Face)&face->root;
    FT_Fixed    temp[6];
    FT_Fixed    temp_scale;


    (void)T1_ToFixedArray( parser, 6, temp, 3 );

    temp_scale = FT_ABS( temp[3] );

    
    
    

    root->units_per_EM = (FT_UShort)( FT_DivFix( 1000 * 0x10000L,
                                                 temp_scale ) >> 16 );

    
    if ( temp_scale != 0x10000L ) {
      temp[0] = FT_DivFix( temp[0], temp_scale );
      temp[1] = FT_DivFix( temp[1], temp_scale );
      temp[2] = FT_DivFix( temp[2], temp_scale );
      temp[4] = FT_DivFix( temp[4], temp_scale );
      temp[5] = FT_DivFix( temp[5], temp_scale );
      temp[3] = 0x10000L;
    }

    matrix->xx = temp[0];
    matrix->yx = temp[1];
    matrix->xy = temp[2];
    matrix->yy = temp[3];

    
    offset->x = temp[4] >> 16;
    offset->y = temp[5] >> 16;
  }


  static void
  t42_parse_encoding( T42_Face    face,
                      T42_Loader  loader )
  {
    T42_Parser  parser = &loader->parser;
    FT_Byte*    cur;
    FT_Byte*    limit  = parser->root.limit;

    PSAux_Service  psaux  = (PSAux_Service)face->psaux;


    T1_Skip_Spaces( parser );
    cur = parser->root.cursor;
    if ( cur >= limit )
    {
      FT_ERROR(( "t42_parse_encoding: out of bounds!\n" ));
      parser->root.error = T42_Err_Invalid_File_Format;
      return;
    }

    
    
    if ( ft_isdigit( *cur ) || *cur == '[' )
    {
      T1_Encoding  encode          = &face->type1.encoding;
      FT_UInt      count, n;
      PS_Table     char_table      = &loader->encoding_table;
      FT_Memory    memory          = parser->root.memory;
      FT_Error     error;
      FT_Bool      only_immediates = 0;


      
      if ( *cur == '[' )
      {
        count           = 256;
        only_immediates = 1;
        parser->root.cursor++;
      }
      else
        count = (FT_UInt)T1_ToInt( parser );

      T1_Skip_Spaces( parser );
      if ( parser->root.cursor >= limit )
        return;

      
      loader->num_chars = encode->num_chars = count;
      if ( FT_NEW_ARRAY( encode->char_index, count )     ||
           FT_NEW_ARRAY( encode->char_name,  count )     ||
           FT_SET_ERROR( psaux->ps_table_funcs->init(
                           char_table, count, memory ) ) )
      {
        parser->root.error = error;
        return;
      }

      
      for ( n = 0; n < count; n++ )
      {
        char*  notdef = (char *)".notdef";


        T1_Add_Table( char_table, n, notdef, 8 );
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      n = 0;
      T1_Skip_Spaces( parser );

      while ( parser->root.cursor < limit )
      {
        cur = parser->root.cursor;

        
        if ( *cur == 'd' && cur + 3 < limit )
        {
          if ( cur[1] == 'e'          &&
               cur[2] == 'f'          &&
               t42_is_space( cur[3] ) )
          {
            FT_TRACE6(( "encoding end\n" ));
            cur += 3;
            break;
          }
        }
        if ( *cur == ']' )
        {
          FT_TRACE6(( "encoding end\n" ));
          cur++;
          break;
        }

        
        if ( ft_isdigit( *cur ) || only_immediates )
        {
          FT_Int  charcode;


          if ( only_immediates )
            charcode = n;
          else
          {
            charcode = (FT_Int)T1_ToInt( parser );
            T1_Skip_Spaces( parser );
          }

          cur = parser->root.cursor;

          if ( *cur == '/' && cur + 2 < limit && n < count )
          {
            FT_PtrDist  len;


            cur++;

            parser->root.cursor = cur;
            T1_Skip_PS_Token( parser );
            if ( parser->root.error )
              return;

            len = parser->root.cursor - cur;

            parser->root.error = T1_Add_Table( char_table, charcode,
                                               cur, len + 1 );
            if ( parser->root.error )
              return;
            char_table->elements[charcode][len] = '\0';

            n++;
          }
        }
        else
        {
          T1_Skip_PS_Token( parser );
          if ( parser->root.error )
            return;
        }

        T1_Skip_Spaces( parser );
      }

      face->type1.encoding_type  = T1_ENCODING_TYPE_ARRAY;
      parser->root.cursor        = cur;
    }

    
    
    else
    {
      if ( cur + 17 < limit                                            &&
           ft_strncmp( (const char*)cur, "StandardEncoding", 16 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_STANDARD;

      else if ( cur + 15 < limit                                          &&
                ft_strncmp( (const char*)cur, "ExpertEncoding", 14 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_EXPERT;

      else if ( cur + 18 < limit                                             &&
                ft_strncmp( (const char*)cur, "ISOLatin1Encoding", 17 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_ISOLATIN1;

      else
      {
        FT_ERROR(( "t42_parse_encoding: invalid token!\n" ));
        parser->root.error = T42_Err_Invalid_File_Format;
      }
    }
  }


  typedef enum  T42_Load_Status_
  {
    BEFORE_START,
    BEFORE_TABLE_DIR,
    OTHER_TABLES

  } T42_Load_Status;


  static void
  t42_parse_sfnts( T42_Face    face,
                   T42_Loader  loader )
  {
    T42_Parser  parser = &loader->parser;
    FT_Memory   memory = parser->root.memory;
    FT_Byte*    cur;
    FT_Byte*    limit  = parser->root.limit;
    FT_Error    error;
    FT_Int      num_tables = 0;
    FT_ULong    count, ttf_size = 0;

    FT_Long     n, string_size, old_string_size, real_size;
    FT_Byte*    string_buf = NULL;
    FT_Bool     allocated  = 0;

    T42_Load_Status  status;


    
    
    
    
    
    
    
    
    
    
    
    
    

    T1_Skip_Spaces( parser );

    if ( parser->root.cursor >= limit || *parser->root.cursor++ != '[' )
    {
      FT_ERROR(( "t42_parse_sfnts: can't find begin of sfnts vector!\n" ));
      error = T42_Err_Invalid_File_Format;
      goto Fail;
    }

    T1_Skip_Spaces( parser );
    status          = BEFORE_START;
    string_size     = 0;
    old_string_size = 0;
    count           = 0;

    while ( parser->root.cursor < limit )
    {
      cur = parser->root.cursor;

      if ( *cur == ']' )
      {
        parser->root.cursor++;
        goto Exit;
      }

      else if ( *cur == '<' )
      {
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          goto Exit;

        
        string_size = (FT_Long)( ( parser->root.cursor - cur - 2 + 1 ) / 2 );
        if ( FT_REALLOC( string_buf, old_string_size, string_size ) )
          goto Fail;

        allocated = 1;

        parser->root.cursor = cur;
        (void)T1_ToBytes( parser, string_buf, string_size, &real_size, 1 );
        old_string_size = string_size;
        string_size = real_size;
      }

      else if ( ft_isdigit( *cur ) )
      {
        if ( allocated )
        {
          FT_ERROR(( "t42_parse_sfnts: "
                     "can't handle mixed binary and hex strings!\n" ));
          error = T42_Err_Invalid_File_Format;
          goto Fail;
        }

        string_size = T1_ToInt( parser );

        T1_Skip_PS_Token( parser );             
        if ( parser->root.error )
          return;

        string_buf = parser->root.cursor + 1;   

        parser->root.cursor += string_size + 1;
        if ( parser->root.cursor >= limit )
        {
          FT_ERROR(( "t42_parse_sfnts: too many binary data!\n" ));
          error = T42_Err_Invalid_File_Format;
          goto Fail;
        }
      }

      if ( !string_buf )
      {
        FT_ERROR(( "t42_parse_sfnts: invalid data in sfnts array!\n" ));
        error = T42_Err_Invalid_File_Format;
        goto Fail;
      }

      
      if ( string_buf[string_size - 1] == 0 && ( string_size % 2 == 1 ) )
        string_size--;

      if ( !string_size )
      {
        FT_ERROR(( "t42_parse_sfnts: invalid string!\n" ));
        error = T42_Err_Invalid_File_Format;
        goto Fail;
      }

      for ( n = 0; n < string_size; n++ )
      {
        switch ( status )
        {
        case BEFORE_START:
          
          if ( count < 12 )
          {
            face->ttf_data[count++] = string_buf[n];
            continue;
          }
          else
          {
            num_tables = 16 * face->ttf_data[4] + face->ttf_data[5];
            status     = BEFORE_TABLE_DIR;
            ttf_size   = 12 + 16 * num_tables;

            if ( FT_REALLOC( face->ttf_data, 12, ttf_size ) )
              goto Fail;
          }
          

        case BEFORE_TABLE_DIR:
          
          if ( count < ttf_size )
          {
            face->ttf_data[count++] = string_buf[n];
            continue;
          }
          else
          {
            int       i;
            FT_ULong  len;


            for ( i = 0; i < num_tables; i++ )
            {
              FT_Byte*  p = face->ttf_data + 12 + 16 * i + 12;


              len = FT_PEEK_ULONG( p );

              
              ttf_size += ( len + 3 ) & ~3;
            }

            status         = OTHER_TABLES;
            face->ttf_size = ttf_size;

            
            if ( FT_REALLOC( face->ttf_data, 12 + 16 * num_tables,
                             ttf_size + 1 ) )
              goto Fail;
          }
          

        case OTHER_TABLES:
          
          if ( count >= ttf_size )
          {
            FT_ERROR(( "t42_parse_sfnts: too many binary data!\n" ));
            error = T42_Err_Invalid_File_Format;
            goto Fail;
          }
          face->ttf_data[count++] = string_buf[n];
        }
      }

      T1_Skip_Spaces( parser );
    }

    
    error = T42_Err_Invalid_File_Format;

  Fail:
    parser->root.error = error;

  Exit:
    if ( allocated )
      FT_FREE( string_buf );
  }


  static void
  t42_parse_charstrings( T42_Face    face,
                         T42_Loader  loader )
  {
    T42_Parser     parser       = &loader->parser;
    PS_Table       code_table   = &loader->charstrings;
    PS_Table       name_table   = &loader->glyph_names;
    PS_Table       swap_table   = &loader->swap_table;
    FT_Memory      memory       = parser->root.memory;
    FT_Error       error;

    PSAux_Service  psaux        = (PSAux_Service)face->psaux;

    FT_Byte*       cur;
    FT_Byte*       limit        = parser->root.limit;
    FT_UInt        n;
    FT_UInt        notdef_index = 0;
    FT_Byte        notdef_found = 0;


    T1_Skip_Spaces( parser );

    if ( parser->root.cursor >= limit )
    {
      FT_ERROR(( "t42_parse_charstrings: out of bounds!\n" ));
      error = T42_Err_Invalid_File_Format;
      goto Fail;
    }

    if ( ft_isdigit( *parser->root.cursor ) )
    {
      loader->num_glyphs = (FT_UInt)T1_ToInt( parser );
      if ( parser->root.error )
        return;
    }
    else if ( *parser->root.cursor == '<' )
    {
      
      
      FT_UInt  count = 0;


      T1_Skip_PS_Token( parser );
      if ( parser->root.error )
        return;
      T1_Skip_Spaces( parser );
      cur = parser->root.cursor;

      while ( parser->root.cursor < limit )
      {
        if ( *parser->root.cursor == '/' )
          count++;
        else if ( *parser->root.cursor == '>' )
        {
          loader->num_glyphs  = count;
          parser->root.cursor = cur;        
          break;
        }
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          return;
        T1_Skip_Spaces( parser );
      }
    }
    else
    {
      FT_ERROR(( "t42_parse_charstrings: invalid token!\n" ));
      error = T42_Err_Invalid_File_Format;
      goto Fail;
    }

    if ( parser->root.cursor >= limit )
    {
      FT_ERROR(( "t42_parse_charstrings: out of bounds!\n" ));
      error = T42_Err_Invalid_File_Format;
      goto Fail;
    }

    

    error = psaux->ps_table_funcs->init( code_table,
                                         loader->num_glyphs,
                                         memory );
    if ( error )
      goto Fail;

    error = psaux->ps_table_funcs->init( name_table,
                                         loader->num_glyphs,
                                         memory );
    if ( error )
      goto Fail;

    
    

    error = psaux->ps_table_funcs->init( swap_table, 4, memory );
    if ( error )
      goto Fail;

    n = 0;

    for (;;)
    {
      
      

      T1_Skip_Spaces( parser );

      cur = parser->root.cursor;
      if ( cur >= limit )
        break;

      
      if ( *cur   == 'e'          &&
           cur + 3 < limit        &&
           cur[1] == 'n'          &&
           cur[2] == 'd'          &&
           t42_is_space( cur[3] ) )
        break;
      if ( *cur == '>' )
        break;

      T1_Skip_PS_Token( parser );
      if ( parser->root.error )
        return;

      if ( *cur == '/' )
      {
        FT_PtrDist  len;


        if ( cur + 1 >= limit )
        {
          FT_ERROR(( "t42_parse_charstrings: out of bounds!\n" ));
          error = T42_Err_Invalid_File_Format;
          goto Fail;
        }

        cur++;                              
        len = parser->root.cursor - cur;

        error = T1_Add_Table( name_table, n, cur, len + 1 );
        if ( error )
          goto Fail;

        
        name_table->elements[n][len] = '\0';

        
        if ( *cur == '.'                                              &&
             ft_strcmp( ".notdef",
                        (const char*)(name_table->elements[n]) ) == 0 )
        {
          notdef_index = n;
          notdef_found = 1;
        }

        T1_Skip_Spaces( parser );

        cur = parser->root.cursor;

        (void)T1_ToInt( parser );
        if ( parser->root.cursor >= limit )
        {
          FT_ERROR(( "t42_parse_charstrings: out of bounds!\n" ));
          error = T42_Err_Invalid_File_Format;
          goto Fail;
        }

        len = parser->root.cursor - cur;

        error = T1_Add_Table( code_table, n, cur, len + 1 );
        if ( error )
          goto Fail;

        code_table->elements[n][len] = '\0';

        n++;
        if ( n >= loader->num_glyphs )
          break;
      }
    }

    loader->num_glyphs = n;

    if ( !notdef_found )
    {
      FT_ERROR(( "t42_parse_charstrings: no /.notdef glyph!\n" ));
      error = T42_Err_Invalid_File_Format;
      goto Fail;
    }

    
    if ( ft_strcmp( (const char*)".notdef",
                    (const char*)name_table->elements[0] ) )
    {
      
      
      
      
      

      
      error = T1_Add_Table( swap_table, 0,
                            name_table->elements[0],
                            name_table->lengths [0] );
      if ( error )
        goto Fail;

      
      error = T1_Add_Table( swap_table, 1,
                            code_table->elements[0],
                            code_table->lengths [0] );
      if ( error )
        goto Fail;

      
      error = T1_Add_Table( swap_table, 2,
                            name_table->elements[notdef_index],
                            name_table->lengths [notdef_index] );
      if ( error )
        goto Fail;

      
      error = T1_Add_Table( swap_table, 3,
                            code_table->elements[notdef_index],
                            code_table->lengths [notdef_index] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, notdef_index,
                            swap_table->elements[0],
                            swap_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, notdef_index,
                            swap_table->elements[1],
                            swap_table->lengths [1] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, 0,
                            swap_table->elements[2],
                            swap_table->lengths [2] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, 0,
                            swap_table->elements[3],
                            swap_table->lengths [3] );
      if ( error )
        goto Fail;

    }

    return;

  Fail:
    parser->root.error = error;
  }


  static FT_Error
  t42_load_keyword( T42_Face    face,
                    T42_Loader  loader,
                    T1_Field    field )
  {
    FT_Error  error;
    void*     dummy_object;
    void**    objects;
    FT_UInt   max_objects = 0;


    
    if ( field->type == T1_FIELD_TYPE_CALLBACK )
    {
      field->reader( (FT_Face)face, loader );
      error = loader->parser.root.error;
      goto Exit;
    }

    
    

    switch ( field->location )
    {
    case T1_FIELD_LOCATION_FONT_INFO:
      dummy_object = &face->type1.font_info;
      break;

    case T1_FIELD_LOCATION_BBOX:
      dummy_object = &face->type1.font_bbox;
      break;

    default:
      dummy_object = &face->type1;
    }

    objects = &dummy_object;

    if ( field->type == T1_FIELD_TYPE_INTEGER_ARRAY ||
         field->type == T1_FIELD_TYPE_FIXED_ARRAY   )
      error = T1_Load_Field_Table( &loader->parser, field,
                                   objects, max_objects, 0 );
    else
      error = T1_Load_Field( &loader->parser, field,
                             objects, max_objects, 0 );

   Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  t42_parse_dict( T42_Face    face,
                  T42_Loader  loader,
                  FT_Byte*    base,
                  FT_Long     size )
  {
    T42_Parser  parser     = &loader->parser;
    FT_Byte*    limit;
    FT_Int      n_keywords = (FT_Int)( sizeof ( t42_keywords ) /
                                         sizeof ( t42_keywords[0] ) );


    parser->root.cursor = base;
    parser->root.limit  = base + size;
    parser->root.error  = T42_Err_Ok;

    limit = parser->root.limit;

    T1_Skip_Spaces( parser );

    while ( parser->root.cursor < limit )
    {
      FT_Byte*  cur;


      cur = parser->root.cursor;

      
      if ( *cur == 'F' && cur + 25 < limit                    &&
           ft_strncmp( (char*)cur, "FontDirectory", 13 ) == 0 )
      {
        FT_Byte*  cur2;


        
        T1_Skip_PS_Token( parser );
        T1_Skip_Spaces  ( parser );
        cur = cur2 = parser->root.cursor;

        
        while ( cur < limit )
        {
          if ( *cur == 'k' && cur + 5 < limit             &&
                ft_strncmp( (char*)cur, "known", 5 ) == 0 )
            break;

          T1_Skip_PS_Token( parser );
          if ( parser->root.error )
            goto Exit;
          T1_Skip_Spaces  ( parser );
          cur = parser->root.cursor;
        }

        if ( cur < limit )
        {
          T1_TokenRec  token;


          
          T1_Skip_PS_Token( parser );
          T1_ToToken( parser, &token );

          
          if ( token.type == T1_TOKEN_TYPE_ARRAY )
            cur2 = parser->root.cursor;
        }
        parser->root.cursor = cur2;
      }

      
      else if ( *cur == '/' && cur + 2 < limit )
      {
        FT_PtrDist  len;


        cur++;

        parser->root.cursor = cur;
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          goto Exit;

        len = parser->root.cursor - cur;

        if ( len > 0 && len < 22 && parser->root.cursor < limit )
        {
          int  i;


          

          
          for ( i = 0; i < n_keywords; i++ )
          {
            T1_Field  keyword = (T1_Field)&t42_keywords[i];
            FT_Byte   *name   = (FT_Byte*)keyword->ident;


            if ( !name )
              continue;

            if ( cur[0] == name[0]                                  &&
                 len == (FT_PtrDist)ft_strlen( (const char *)name ) &&
                 ft_memcmp( cur, name, len ) == 0                   )
            {
              
              parser->root.error = t42_load_keyword( face,
                                                     loader,
                                                     keyword );
              if ( parser->root.error )
                return parser->root.error;
              break;
            }
          }
        }
      }
      else
      {
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          goto Exit;
      }

      T1_Skip_Spaces( parser );
    }

  Exit:
    return parser->root.error;
  }


  FT_LOCAL_DEF( void )
  t42_loader_init( T42_Loader  loader,
                   T42_Face    face )
  {
    FT_UNUSED( face );

    FT_MEM_ZERO( loader, sizeof ( *loader ) );
    loader->num_glyphs = 0;
    loader->num_chars  = 0;

    
    loader->encoding_table.init = 0;
    loader->charstrings.init    = 0;
    loader->glyph_names.init    = 0;
  }


  FT_LOCAL_DEF( void )
  t42_loader_done( T42_Loader  loader )
  {
    T42_Parser  parser = &loader->parser;


    
    T1_Release_Table( &loader->encoding_table );
    T1_Release_Table( &loader->charstrings );
    T1_Release_Table( &loader->glyph_names );
    T1_Release_Table( &loader->swap_table );

    
    t42_parser_done( parser );
  }




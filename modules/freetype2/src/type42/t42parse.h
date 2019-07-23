
















#ifndef __T42PARSE_H__
#define __T42PARSE_H__


#include "t42objs.h"
#include FT_INTERNAL_POSTSCRIPT_AUX_H


FT_BEGIN_HEADER

  typedef struct  T42_ParserRec_
  {
    PS_ParserRec  root;
    FT_Stream     stream;

    FT_Byte*      base_dict;
    FT_Long       base_len;

    FT_Bool       in_memory;

  } T42_ParserRec, *T42_Parser;


  typedef struct  T42_Loader_
  {
    T42_ParserRec  parser;          

    FT_UInt        num_chars;       
    PS_TableRec    encoding_table;  
                                    

    FT_UInt        num_glyphs;
    PS_TableRec    glyph_names;
    PS_TableRec    charstrings;
    PS_TableRec    swap_table;      

  } T42_LoaderRec, *T42_Loader;


  FT_LOCAL( FT_Error )
  t42_parser_init( T42_Parser     parser,
                   FT_Stream      stream,
                   FT_Memory      memory,
                   PSAux_Service  psaux );

  FT_LOCAL( void )
  t42_parser_done( T42_Parser  parser );


  FT_LOCAL( FT_Error )
  t42_parse_dict( T42_Face    face,
                  T42_Loader  loader,
                  FT_Byte*    base,
                  FT_Long     size );


  FT_LOCAL( void )
  t42_loader_init( T42_Loader  loader,
                   T42_Face    face );

  FT_LOCAL( void )
  t42_loader_done( T42_Loader  loader );


 

FT_END_HEADER


#endif 




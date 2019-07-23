

















#ifndef __AFMPARSE_H__
#define __AFMPARSE_H__


#include <ft2build.h>
#include FT_INTERNAL_POSTSCRIPT_AUX_H


FT_BEGIN_HEADER


  FT_LOCAL( FT_Error )
  afm_parser_init( AFM_Parser  parser,
                   FT_Memory   memory,
                   FT_Byte*    base,
                   FT_Byte*    limit );


  FT_LOCAL( void )
  afm_parser_done( AFM_Parser  parser );


  FT_LOCAL( FT_Error )
  afm_parser_parse( AFM_Parser  parser );


  enum  AFM_ValueType_
  {
    AFM_VALUE_TYPE_STRING,
    AFM_VALUE_TYPE_NAME,
    AFM_VALUE_TYPE_FIXED,   
    AFM_VALUE_TYPE_INTEGER,
    AFM_VALUE_TYPE_BOOL,
    AFM_VALUE_TYPE_INDEX    
  };


  typedef struct  AFM_ValueRec_
  {
    enum AFM_ValueType_  type;
    union {
      char*     s;
      FT_Fixed  f;
      FT_Int    i;
      FT_Bool   b;

    } u;

  } AFM_ValueRec, *AFM_Value;

#define  AFM_MAX_ARGUMENTS  5

  FT_LOCAL( FT_Int )
  afm_parser_read_vals( AFM_Parser  parser,
                        AFM_Value   vals,
                        FT_Int      n );

  
  FT_LOCAL( char* )
  afm_parser_next_key( AFM_Parser  parser,
                       FT_Bool     line,
                       FT_UInt*    len );

FT_END_HEADER

#endif 






















#include <ft2build.h>
#include "cffparse.h"
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_DEBUG_H

#include "cfferrs.h"
#include "cffpic.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffparse


  FT_LOCAL_DEF( void )
  cff_parser_init( CFF_Parser  parser,
                   FT_UInt     code,
                   void*       object,
                   FT_Library  library)
  {
    FT_MEM_ZERO( parser, sizeof ( *parser ) );

    parser->top         = parser->stack;
    parser->object_code = code;
    parser->object      = object;
    parser->library     = library;
  }


  
  static FT_Long
  cff_parse_integer( FT_Byte*  start,
                     FT_Byte*  limit )
  {
    FT_Byte*  p   = start;
    FT_Int    v   = *p++;
    FT_Long   val = 0;


    if ( v == 28 )
    {
      if ( p + 2 > limit )
        goto Bad;

      val = (FT_Short)( ( (FT_UShort)p[0] << 8 ) | p[1] );
      p  += 2;
    }
    else if ( v == 29 )
    {
      if ( p + 4 > limit )
        goto Bad;

      val = (FT_Long)( ( (FT_ULong)p[0] << 24 ) |
                       ( (FT_ULong)p[1] << 16 ) |
                       ( (FT_ULong)p[2] <<  8 ) |
                         (FT_ULong)p[3]         );
      p += 4;
    }
    else if ( v < 247 )
    {
      val = v - 139;
    }
    else if ( v < 251 )
    {
      if ( p + 1 > limit )
        goto Bad;

      val = ( v - 247 ) * 256 + p[0] + 108;
      p++;
    }
    else
    {
      if ( p + 1 > limit )
        goto Bad;

      val = -( v - 251 ) * 256 - p[0] - 108;
      p++;
    }

  Exit:
    return val;

  Bad:
    val = 0;
    FT_TRACE4(( "!!!END OF DATA:!!!" ));
    goto Exit;
  }


  static const FT_Long power_tens[] =
  {
    1L,
    10L,
    100L,
    1000L,
    10000L,
    100000L,
    1000000L,
    10000000L,
    100000000L,
    1000000000L
  };


  
  static FT_Fixed
  cff_parse_real( FT_Byte*  start,
                  FT_Byte*  limit,
                  FT_Long   power_ten,
                  FT_Long*  scaling )
  {
    FT_Byte*  p = start;
    FT_UInt   nib;
    FT_UInt   phase;

    FT_Long   result, number, exponent;
    FT_Int    sign = 0, exponent_sign = 0, have_overflow = 0;
    FT_Long   exponent_add, integer_length, fraction_length;


    if ( scaling )
      *scaling = 0;

    result = 0;

    number   = 0;
    exponent = 0;

    exponent_add    = 0;
    integer_length  = 0;
    fraction_length = 0;

    
    phase = 4;

    for (;;)
    {
      
      
      if ( phase )
      {
        p++;

        
        if ( p >= limit )
          goto Bad;
      }

      
      nib   = ( p[0] >> phase ) & 0xF;
      phase = 4 - phase;

      if ( nib == 0xE )
        sign = 1;
      else if ( nib > 9 )
        break;
      else
      {
        
        if ( number >= 0xCCCCCCCL )
          exponent_add++;
        
        else if ( nib || number )
        {
          integer_length++;
          number = number * 10 + nib;
        }
      }
    }

    
    if ( nib == 0xa )
      for (;;)
      {
        
        
        if ( phase )
        {
          p++;

          
          if ( p >= limit )
            goto Bad;
        }

        
        nib   = ( p[0] >> phase ) & 0xF;
        phase = 4 - phase;
        if ( nib >= 10 )
          break;

        
        if ( !nib && !number )
          exponent_add--;
        
        else if ( number < 0xCCCCCCCL && fraction_length < 9 )
        {
          fraction_length++;
          number = number * 10 + nib;
        }
      }

    
    if ( nib == 12 )
    {
      exponent_sign = 1;
      nib           = 11;
    }

    if ( nib == 11 )
    {
      for (;;)
      {
        
        
        if ( phase )
        {
          p++;

          
          if ( p >= limit )
            goto Bad;
        }

        
        nib   = ( p[0] >> phase ) & 0xF;
        phase = 4 - phase;
        if ( nib >= 10 )
          break;

        
        if ( exponent > 1000 )
          have_overflow = 1;
        else
          exponent = exponent * 10 + nib;
      }

      if ( exponent_sign )
        exponent = -exponent;
    }

    if ( !number )
      goto Exit;

    if ( have_overflow )
    {
      if ( exponent_sign )
        goto Underflow;
      else
        goto Overflow;
    }

    
    exponent += power_ten + exponent_add;

    if ( scaling )
    {
      
      fraction_length += integer_length;
      exponent        += integer_length;

      if ( fraction_length <= 5 )
      {
        if ( number > 0x7FFFL )
        {
          result   = FT_DivFix( number, 10 );
          *scaling = exponent - fraction_length + 1;
        }
        else
        {
          if ( exponent > 0 )
          {
            FT_Long  new_fraction_length, shift;


            
            new_fraction_length = FT_MIN( exponent, 5 );
            shift               = new_fraction_length - fraction_length;

            if ( shift > 0 )
            {
              exponent -= new_fraction_length;
              number   *= power_tens[shift];
              if ( number > 0x7FFFL )
              {
                number   /= 10;
                exponent += 1;
              }
            }
            else
              exponent -= fraction_length;
          }
          else
            exponent -= fraction_length;

          result   = (FT_Long)( (FT_ULong)number << 16 );
          *scaling = exponent;
        }
      }
      else
      {
        if ( ( number / power_tens[fraction_length - 5] ) > 0x7FFFL )
        {
          result   = FT_DivFix( number, power_tens[fraction_length - 4] );
          *scaling = exponent - 4;
        }
        else
        {
          result   = FT_DivFix( number, power_tens[fraction_length - 5] );
          *scaling = exponent - 5;
        }
      }
    }
    else
    {
      integer_length  += exponent;
      fraction_length -= exponent;

      if ( integer_length > 5 )
        goto Overflow;
      if ( integer_length < -5 )
        goto Underflow;

      
      if ( integer_length < 0 )
      {
        number          /= power_tens[-integer_length];
        fraction_length += integer_length;
      }

      
      if ( fraction_length == 10 )
      {
        number          /= 10;
        fraction_length -= 1;
      }

      
      if ( fraction_length > 0 )
      {
        if ( ( number / power_tens[fraction_length] ) > 0x7FFFL )
          goto Exit;

        result = FT_DivFix( number, power_tens[fraction_length] );
      }
      else
      {
        number *= power_tens[-fraction_length];

        if ( number > 0x7FFFL )
          goto Overflow;

        result = (FT_Long)( (FT_ULong)number << 16 );
      }
    }

  Exit:
    if ( sign )
      result = -result;

    return result;

  Overflow:
    result = 0x7FFFFFFFL;
    FT_TRACE4(( "!!!OVERFLOW:!!!" ));
    goto Exit;

  Underflow:
    result = 0;
    FT_TRACE4(( "!!!UNDERFLOW:!!!" ));
    goto Exit;

  Bad:
    result = 0;
    FT_TRACE4(( "!!!END OF DATA:!!!" ));
    goto Exit;
  }


  
  static FT_Long
  cff_parse_num( FT_Byte**  d )
  {
    return **d == 30 ? ( cff_parse_real( d[0], d[1], 0, NULL ) >> 16 )
                     :   cff_parse_integer( d[0], d[1] );
  }


  
  static FT_Fixed
  do_fixed( FT_Byte**  d,
            FT_Long    scaling )
  {
    if ( **d == 30 )
      return cff_parse_real( d[0], d[1], scaling, NULL );
    else
    {
      FT_Long  val = cff_parse_integer( d[0], d[1] );


      if ( scaling )
        val *= power_tens[scaling];

      if ( val > 0x7FFF )
      {
        val = 0x7FFFFFFFL;
        goto Overflow;
      }
      else if ( val < -0x7FFF )
      {
        val = -0x7FFFFFFFL;
        goto Overflow;
      }

      return (FT_Long)( (FT_ULong)val << 16 );

    Overflow:
      FT_TRACE4(( "!!!OVERFLOW:!!!" ));
      return val;
    }
  }


  
  static FT_Fixed
  cff_parse_fixed( FT_Byte**  d )
  {
    return do_fixed( d, 0 );
  }


  
  
  static FT_Fixed
  cff_parse_fixed_scaled( FT_Byte**  d,
                          FT_Long    scaling )
  {
    return do_fixed( d, scaling );
  }


  
  
  
  static FT_Fixed
  cff_parse_fixed_dynamic( FT_Byte**  d,
                           FT_Long*   scaling )
  {
    FT_ASSERT( scaling );

    if ( **d == 30 )
      return cff_parse_real( d[0], d[1], 0, scaling );
    else
    {
      FT_Long  number;
      FT_Int   integer_length;


      number = cff_parse_integer( d[0], d[1] );

      if ( number > 0x7FFFL )
      {
        for ( integer_length = 5; integer_length < 10; integer_length++ )
          if ( number < power_tens[integer_length] )
            break;

        if ( ( number / power_tens[integer_length - 5] ) > 0x7FFFL )
        {
          *scaling = integer_length - 4;
          return FT_DivFix( number, power_tens[integer_length - 4] );
        }
        else
        {
          *scaling = integer_length - 5;
          return FT_DivFix( number, power_tens[integer_length - 5] );
        }
      }
      else
      {
        *scaling = 0;
        return (FT_Long)( (FT_ULong)number << 16 );
      }
    }
  }


  static FT_Error
  cff_parse_font_matrix( CFF_Parser  parser )
  {
    CFF_FontRecDict  dict   = (CFF_FontRecDict)parser->object;
    FT_Matrix*       matrix = &dict->font_matrix;
    FT_Vector*       offset = &dict->font_offset;
    FT_ULong*        upm    = &dict->units_per_em;
    FT_Byte**        data   = parser->stack;
    FT_Error         error  = FT_ERR( Stack_Underflow );


    if ( parser->top >= parser->stack + 6 )
    {
      FT_Long  scaling;


      error = FT_Err_Ok;

      dict->has_font_matrix = TRUE;

      
      
      
      
      

      matrix->xx = cff_parse_fixed_dynamic( data++, &scaling );

      scaling = -scaling;

      if ( scaling < 0 || scaling > 9 )
      {
        

        FT_TRACE1(( "cff_parse_font_matrix:"
                    " strange scaling value for xx element (%d),\n"
                    "                      "
                    " using default matrix\n", scaling ));

        matrix->xx = 0x10000L;
        matrix->yx = 0;
        matrix->xy = 0;
        matrix->yy = 0x10000L;
        offset->x  = 0;
        offset->y  = 0;
        *upm       = 1;

        goto Exit;
      }

      matrix->yx = cff_parse_fixed_scaled( data++, scaling );
      matrix->xy = cff_parse_fixed_scaled( data++, scaling );
      matrix->yy = cff_parse_fixed_scaled( data++, scaling );
      offset->x  = cff_parse_fixed_scaled( data++, scaling );
      offset->y  = cff_parse_fixed_scaled( data,   scaling );

      *upm = power_tens[scaling];

      FT_TRACE4(( " [%f %f %f %f %f %f]\n",
                  (double)matrix->xx / *upm / 65536,
                  (double)matrix->xy / *upm / 65536,
                  (double)matrix->yx / *upm / 65536,
                  (double)matrix->yy / *upm / 65536,
                  (double)offset->x  / *upm / 65536,
                  (double)offset->y  / *upm / 65536 ));
    }

  Exit:
    return error;
  }


  static FT_Error
  cff_parse_font_bbox( CFF_Parser  parser )
  {
    CFF_FontRecDict  dict = (CFF_FontRecDict)parser->object;
    FT_BBox*         bbox = &dict->font_bbox;
    FT_Byte**        data = parser->stack;
    FT_Error         error;


    error = FT_ERR( Stack_Underflow );

    if ( parser->top >= parser->stack + 4 )
    {
      bbox->xMin = FT_RoundFix( cff_parse_fixed( data++ ) );
      bbox->yMin = FT_RoundFix( cff_parse_fixed( data++ ) );
      bbox->xMax = FT_RoundFix( cff_parse_fixed( data++ ) );
      bbox->yMax = FT_RoundFix( cff_parse_fixed( data   ) );
      error = FT_Err_Ok;

      FT_TRACE4(( " [%d %d %d %d]\n",
                  bbox->xMin / 65536,
                  bbox->yMin / 65536,
                  bbox->xMax / 65536,
                  bbox->yMax / 65536 ));
    }

    return error;
  }


  static FT_Error
  cff_parse_private_dict( CFF_Parser  parser )
  {
    CFF_FontRecDict  dict = (CFF_FontRecDict)parser->object;
    FT_Byte**        data = parser->stack;
    FT_Error         error;


    error = FT_ERR( Stack_Underflow );

    if ( parser->top >= parser->stack + 2 )
    {
      dict->private_size   = cff_parse_num( data++ );
      dict->private_offset = cff_parse_num( data   );
      FT_TRACE4(( " %lu %lu\n",
                  dict->private_size, dict->private_offset ));

      error = FT_Err_Ok;
    }

    return error;
  }


  static FT_Error
  cff_parse_cid_ros( CFF_Parser  parser )
  {
    CFF_FontRecDict  dict = (CFF_FontRecDict)parser->object;
    FT_Byte**        data = parser->stack;
    FT_Error         error;


    error = FT_ERR( Stack_Underflow );

    if ( parser->top >= parser->stack + 3 )
    {
      dict->cid_registry = (FT_UInt)cff_parse_num( data++ );
      dict->cid_ordering = (FT_UInt)cff_parse_num( data++ );
      if ( **data == 30 )
        FT_TRACE1(( "cff_parse_cid_ros: real supplement is rounded\n" ));
      dict->cid_supplement = cff_parse_num( data );
      if ( dict->cid_supplement < 0 )
        FT_TRACE1(( "cff_parse_cid_ros: negative supplement %d is found\n",
                   dict->cid_supplement ));
      error = FT_Err_Ok;

      FT_TRACE4(( " %d %d %d\n",
                  dict->cid_registry,
                  dict->cid_ordering,
                  dict->cid_supplement ));
    }

    return error;
  }


#define CFF_FIELD_NUM( code, name, id )             \
          CFF_FIELD( code, name, id, cff_kind_num )
#define CFF_FIELD_FIXED( code, name, id )             \
          CFF_FIELD( code, name, id, cff_kind_fixed )
#define CFF_FIELD_FIXED_1000( code, name, id )                 \
          CFF_FIELD( code, name, id, cff_kind_fixed_thousand )
#define CFF_FIELD_STRING( code, name, id )             \
          CFF_FIELD( code, name, id, cff_kind_string )
#define CFF_FIELD_BOOL( code, name, id )             \
          CFF_FIELD( code, name, id, cff_kind_bool )

#define CFFCODE_TOPDICT  0x1000
#define CFFCODE_PRIVATE  0x2000


#ifndef FT_CONFIG_OPTION_PIC


#undef  CFF_FIELD
#undef  CFF_FIELD_DELTA


#ifndef FT_DEBUG_LEVEL_TRACE


#define CFF_FIELD_CALLBACK( code, name, id ) \
          {                                  \
            cff_kind_callback,               \
            code | CFFCODE,                  \
            0, 0,                            \
            cff_parse_ ## name,              \
            0, 0                             \
          },

#define CFF_FIELD( code, name, id, kind ) \
          {                               \
            kind,                         \
            code | CFFCODE,               \
            FT_FIELD_OFFSET( name ),      \
            FT_FIELD_SIZE( name ),        \
            0, 0, 0                       \
          },

#define CFF_FIELD_DELTA( code, name, max, id ) \
          {                                    \
            cff_kind_delta,                    \
            code | CFFCODE,                    \
            FT_FIELD_OFFSET( name ),           \
            FT_FIELD_SIZE_DELTA( name ),       \
            0,                                 \
            max,                               \
            FT_FIELD_OFFSET( num_ ## name )    \
          },

  static const CFF_Field_Handler  cff_field_handlers[] =
  {

#include "cfftoken.h"

    { 0, 0, 0, 0, 0, 0, 0 }
  };


#else 



#define CFF_FIELD_CALLBACK( code, name, id ) \
          {                                  \
            cff_kind_callback,               \
            code | CFFCODE,                  \
            0, 0,                            \
            cff_parse_ ## name,              \
            0, 0,                            \
            id                               \
          },

#define CFF_FIELD( code, name, id, kind ) \
          {                               \
            kind,                         \
            code | CFFCODE,               \
            FT_FIELD_OFFSET( name ),      \
            FT_FIELD_SIZE( name ),        \
            0, 0, 0,                      \
            id                            \
          },

#define CFF_FIELD_DELTA( code, name, max, id ) \
          {                                    \
            cff_kind_delta,                    \
            code | CFFCODE,                    \
            FT_FIELD_OFFSET( name ),           \
            FT_FIELD_SIZE_DELTA( name ),       \
            0,                                 \
            max,                               \
            FT_FIELD_OFFSET( num_ ## name ),   \
            id                                 \
          },

  static const CFF_Field_Handler  cff_field_handlers[] =
  {

#include "cfftoken.h"

    { 0, 0, 0, 0, 0, 0, 0, 0 }
  };


#endif 


#else 


  void
  FT_Destroy_Class_cff_field_handlers( FT_Library          library,
                                       CFF_Field_Handler*  clazz )
  {
    FT_Memory  memory = library->memory;


    if ( clazz )
      FT_FREE( clazz );
  }


  FT_Error
  FT_Create_Class_cff_field_handlers( FT_Library           library,
                                      CFF_Field_Handler**  output_class )
  {
    CFF_Field_Handler*  clazz  = NULL;
    FT_Error            error;
    FT_Memory           memory = library->memory;

    int  i = 0;


#undef CFF_FIELD
#define CFF_FIELD( code, name, id, kind ) i++;
#undef CFF_FIELD_DELTA
#define CFF_FIELD_DELTA( code, name, max, id ) i++;
#undef CFF_FIELD_CALLBACK
#define CFF_FIELD_CALLBACK( code, name, id ) i++;

#include "cfftoken.h"

    i++; 

    if ( FT_ALLOC( clazz, sizeof ( CFF_Field_Handler ) * i ) )
      return error;

    i = 0;


#ifndef FT_DEBUG_LEVEL_TRACE


#undef CFF_FIELD_CALLBACK
#define CFF_FIELD_CALLBACK( code_, name_, id_ )        \
          clazz[i].kind         = cff_kind_callback;   \
          clazz[i].code         = code_ | CFFCODE;     \
          clazz[i].offset       = 0;                   \
          clazz[i].size         = 0;                   \
          clazz[i].reader       = cff_parse_ ## name_; \
          clazz[i].array_max    = 0;                   \
          clazz[i].count_offset = 0;                   \
          i++;

#undef  CFF_FIELD
#define CFF_FIELD( code_, name_, id_, kind_ )               \
          clazz[i].kind         = kind_;                    \
          clazz[i].code         = code_ | CFFCODE;          \
          clazz[i].offset       = FT_FIELD_OFFSET( name_ ); \
          clazz[i].size         = FT_FIELD_SIZE( name_ );   \
          clazz[i].reader       = 0;                        \
          clazz[i].array_max    = 0;                        \
          clazz[i].count_offset = 0;                        \
          i++;                                              \

#undef  CFF_FIELD_DELTA
#define CFF_FIELD_DELTA( code_, name_, max_, id_ )                  \
          clazz[i].kind         = cff_kind_delta;                   \
          clazz[i].code         = code_ | CFFCODE;                  \
          clazz[i].offset       = FT_FIELD_OFFSET( name_ );         \
          clazz[i].size         = FT_FIELD_SIZE_DELTA( name_ );     \
          clazz[i].reader       = 0;                                \
          clazz[i].array_max    = max_;                             \
          clazz[i].count_offset = FT_FIELD_OFFSET( num_ ## name_ ); \
          i++;

#include "cfftoken.h"

    clazz[i].kind         = 0;
    clazz[i].code         = 0;
    clazz[i].offset       = 0;
    clazz[i].size         = 0;
    clazz[i].reader       = 0;
    clazz[i].array_max    = 0;
    clazz[i].count_offset = 0;


#else 


#undef CFF_FIELD_CALLBACK
#define CFF_FIELD_CALLBACK( code_, name_, id_ )        \
          clazz[i].kind         = cff_kind_callback;   \
          clazz[i].code         = code_ | CFFCODE;     \
          clazz[i].offset       = 0;                   \
          clazz[i].size         = 0;                   \
          clazz[i].reader       = cff_parse_ ## name_; \
          clazz[i].array_max    = 0;                   \
          clazz[i].count_offset = 0;                   \
          clazz[i].id           = id_;                 \
          i++;

#undef  CFF_FIELD
#define CFF_FIELD( code_, name_, id_, kind_ )               \
          clazz[i].kind         = kind_;                    \
          clazz[i].code         = code_ | CFFCODE;          \
          clazz[i].offset       = FT_FIELD_OFFSET( name_ ); \
          clazz[i].size         = FT_FIELD_SIZE( name_ );   \
          clazz[i].reader       = 0;                        \
          clazz[i].array_max    = 0;                        \
          clazz[i].count_offset = 0;                        \
          clazz[i].id           = id_;                      \
          i++;                                              \

#undef  CFF_FIELD_DELTA
#define CFF_FIELD_DELTA( code_, name_, max_, id_ )                  \
          clazz[i].kind         = cff_kind_delta;                   \
          clazz[i].code         = code_ | CFFCODE;                  \
          clazz[i].offset       = FT_FIELD_OFFSET( name_ );         \
          clazz[i].size         = FT_FIELD_SIZE_DELTA( name_ );     \
          clazz[i].reader       = 0;                                \
          clazz[i].array_max    = max_;                             \
          clazz[i].count_offset = FT_FIELD_OFFSET( num_ ## name_ ); \
          clazz[i].id           = id_;                              \
          i++;

#include "cfftoken.h"

    clazz[i].kind         = 0;
    clazz[i].code         = 0;
    clazz[i].offset       = 0;
    clazz[i].size         = 0;
    clazz[i].reader       = 0;
    clazz[i].array_max    = 0;
    clazz[i].count_offset = 0;
    clazz[i].id           = 0;


#endif 


    *output_class = clazz;

    return FT_Err_Ok;
  }


#endif 


  FT_LOCAL_DEF( FT_Error )
  cff_parser_run( CFF_Parser  parser,
                  FT_Byte*    start,
                  FT_Byte*    limit )
  {
    FT_Byte*    p       = start;
    FT_Error    error   = FT_Err_Ok;
    FT_Library  library = parser->library;
    FT_UNUSED( library );


    parser->top    = parser->stack;
    parser->start  = start;
    parser->limit  = limit;
    parser->cursor = start;

    while ( p < limit )
    {
      FT_UInt  v = *p;


      if ( v >= 27 && v != 31 )
      {
        
        if ( parser->top - parser->stack >= CFF_MAX_STACK_DEPTH )
          goto Stack_Overflow;

        *parser->top ++ = p;

        
        if ( v == 30 )
        {
          
          p++;
          for (;;)
          {
            
            
            if ( p >= limit )
              goto Exit;
            v = p[0] >> 4;
            if ( v == 15 )
              break;
            v = p[0] & 0xF;
            if ( v == 15 )
              break;
            p++;
          }
        }
        else if ( v == 28 )
          p += 2;
        else if ( v == 29 )
          p += 4;
        else if ( v > 246 )
          p += 1;
      }
      else
      {
        
        

        FT_UInt                   code;
        FT_UInt                   num_args = (FT_UInt)
                                             ( parser->top - parser->stack );
        const CFF_Field_Handler*  field;


        *parser->top = p;
        code = v;
        if ( v == 12 )
        {
          
          p++;
          if ( p >= limit )
            goto Syntax_Error;

          code = 0x100 | p[0];
        }
        code = code | parser->object_code;

        for ( field = CFF_FIELD_HANDLERS_GET; field->kind; field++ )
        {
          if ( field->code == (FT_Int)code )
          {
            
            FT_Long   val;
            FT_Byte*  q = (FT_Byte*)parser->object + field->offset;


#ifdef FT_DEBUG_LEVEL_TRACE
            FT_TRACE4(( "  %s", field->id ));
#endif

            
            
            if ( field->kind != cff_kind_delta && num_args < 1 )
              goto Stack_Underflow;

            switch ( field->kind )
            {
            case cff_kind_bool:
            case cff_kind_string:
            case cff_kind_num:
              val = cff_parse_num( parser->stack );
              goto Store_Number;

            case cff_kind_fixed:
              val = cff_parse_fixed( parser->stack );
              goto Store_Number;

            case cff_kind_fixed_thousand:
              val = cff_parse_fixed_scaled( parser->stack, 3 );

            Store_Number:
              switch ( field->size )
              {
              case (8 / FT_CHAR_BIT):
                *(FT_Byte*)q = (FT_Byte)val;
                break;

              case (16 / FT_CHAR_BIT):
                *(FT_Short*)q = (FT_Short)val;
                break;

              case (32 / FT_CHAR_BIT):
                *(FT_Int32*)q = (FT_Int)val;
                break;

              default:  
                *(FT_Long*)q = val;
              }

#ifdef FT_DEBUG_LEVEL_TRACE
              switch ( field->kind )
              {
              case cff_kind_bool:
                FT_TRACE4(( " %s\n", val ? "true" : "false" ));
                break;

              case cff_kind_string:
                FT_TRACE4(( " %ld (SID)\n", val ));
                break;

              case cff_kind_num:
                FT_TRACE4(( " %ld\n", val ));
                break;

              case cff_kind_fixed:
                FT_TRACE4(( " %f\n", (double)val / 65536 ));
                break;

              case cff_kind_fixed_thousand:
                FT_TRACE4(( " %f\n", (double)val / 65536 / 1000 ));

              default:
                ; 
              }
#endif

              break;

            case cff_kind_delta:
              {
                FT_Byte*   qcount = (FT_Byte*)parser->object +
                                      field->count_offset;

                FT_Byte**  data = parser->stack;


                if ( num_args > field->array_max )
                  num_args = field->array_max;

                FT_TRACE4(( " [" ));

                
                *qcount = (FT_Byte)num_args;

                val = 0;
                while ( num_args > 0 )
                {
                  val += cff_parse_num( data++ );
                  switch ( field->size )
                  {
                  case (8 / FT_CHAR_BIT):
                    *(FT_Byte*)q = (FT_Byte)val;
                    break;

                  case (16 / FT_CHAR_BIT):
                    *(FT_Short*)q = (FT_Short)val;
                    break;

                  case (32 / FT_CHAR_BIT):
                    *(FT_Int32*)q = (FT_Int)val;
                    break;

                  default:  
                    *(FT_Long*)q = val;
                  }

                  FT_TRACE4(( " %ld", val ));

                  q += field->size;
                  num_args--;
                }

                FT_TRACE4(( "]\n" ));
              }
              break;

            default:  
              error = field->reader( parser );
              if ( error )
                goto Exit;
            }
            goto Found;
          }
        }

        
        

      Found:
        
        parser->top = parser->stack;
      }
      p++;
    }

  Exit:
    return error;

  Stack_Overflow:
    error = FT_THROW( Invalid_Argument );
    goto Exit;

  Stack_Underflow:
    error = FT_THROW( Invalid_Argument );
    goto Exit;

  Syntax_Error:
    error = FT_THROW( Invalid_Argument );
    goto Exit;
  }




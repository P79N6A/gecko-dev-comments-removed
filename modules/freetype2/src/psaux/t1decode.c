

















#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H
#include FT_OUTLINE_H

#include "t1decode.h"
#include "psobjs.h"

#include "psauxerr.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1decode


  typedef enum  T1_Operator_
  {
    op_none = 0,
    op_endchar,
    op_hsbw,
    op_seac,
    op_sbw,
    op_closepath,
    op_hlineto,
    op_hmoveto,
    op_hvcurveto,
    op_rlineto,
    op_rmoveto,
    op_rrcurveto,
    op_vhcurveto,
    op_vlineto,
    op_vmoveto,
    op_dotsection,
    op_hstem,
    op_hstem3,
    op_vstem,
    op_vstem3,
    op_div,
    op_callothersubr,
    op_callsubr,
    op_pop,
    op_return,
    op_setcurrentpoint,
    op_unknown15,

    op_max    

  } T1_Operator;


  static
  const FT_Int  t1_args_count[op_max] =
  {
    0, 
    0, 
    2, 
    5, 
    4, 
    0, 
    1, 
    1, 
    4, 
    2, 
    2, 
    6, 
    4, 
    1, 
    1, 
    0, 
    2, 
    6, 
    2, 
    6, 
    2, 
   -1, 
    1, 
    0, 
    0, 
    2, 
    2  
  };


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_Int
  t1_lookup_glyph_by_stdcharcode( T1_Decoder  decoder,
                                  FT_Int      charcode )
  {
    FT_UInt             n;
    const FT_String*    glyph_name;
    FT_Service_PsCMaps  psnames = decoder->psnames;


    
    if ( charcode < 0 || charcode > 255 )
      return -1;

    glyph_name = psnames->adobe_std_strings(
                   psnames->adobe_std_encoding[charcode]);

    for ( n = 0; n < decoder->num_glyphs; n++ )
    {
      FT_String*  name = (FT_String*)decoder->glyph_names[n];


      if ( name && name[0] == glyph_name[0]  &&
           ft_strcmp( name, glyph_name ) == 0 )
        return n;
    }

    return -1;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_Error
  t1operator_seac( T1_Decoder  decoder,
                   FT_Pos      asb,
                   FT_Pos      adx,
                   FT_Pos      ady,
                   FT_Int      bchar,
                   FT_Int      achar )
  {
    FT_Error     error;
    FT_Int       bchar_index, achar_index;
#if 0
    FT_Int       n_base_points;
    FT_Outline*  base = decoder->builder.base;
#endif
    FT_Vector    left_bearing, advance;


    
    adx += decoder->builder.left_bearing.x;

    
    
    if ( decoder->glyph_names == 0 )
    {
      FT_ERROR(( "t1operator_seac:" ));
      FT_ERROR(( " glyph names table not available in this font!\n" ));
      return PSaux_Err_Syntax_Error;
    }

    bchar_index = t1_lookup_glyph_by_stdcharcode( decoder, bchar );
    achar_index = t1_lookup_glyph_by_stdcharcode( decoder, achar );

    if ( bchar_index < 0 || achar_index < 0 )
    {
      FT_ERROR(( "t1operator_seac:" ));
      FT_ERROR(( " invalid seac character code arguments\n" ));
      return PSaux_Err_Syntax_Error;
    }

    
    
    if ( decoder->builder.no_recurse )
    {
      FT_GlyphSlot    glyph  = (FT_GlyphSlot)decoder->builder.glyph;
      FT_GlyphLoader  loader = glyph->internal->loader;
      FT_SubGlyph     subg;


      
      error = FT_GlyphLoader_CheckSubGlyphs( loader, 2 );
      if ( error )
        goto Exit;

      subg = loader->current.subglyphs;

      
      subg->index = bchar_index;
      subg->flags = FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES |
                    FT_SUBGLYPH_FLAG_USE_MY_METRICS;
      subg->arg1  = 0;
      subg->arg2  = 0;
      subg++;

      
      subg->index = achar_index;
      subg->flags = FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES;
      subg->arg1  = (FT_Int)( adx - asb );
      subg->arg2  = (FT_Int)ady;

      
      glyph->num_subglyphs = 2;
      glyph->subglyphs     = loader->base.subglyphs;
      glyph->format        = FT_GLYPH_FORMAT_COMPOSITE;

      loader->current.num_subglyphs = 2;
      goto Exit;
    }

    
    

    FT_GlyphLoader_Prepare( decoder->builder.loader );  

    error = t1_decoder_parse_glyph( decoder, bchar_index );
    if ( error )
      goto Exit;

    
    

    left_bearing = decoder->builder.left_bearing;
    advance      = decoder->builder.advance;

    decoder->builder.left_bearing.x = 0;
    decoder->builder.left_bearing.y = 0;

    decoder->builder.pos_x = adx - asb;
    decoder->builder.pos_y = ady;

    
    
    error = t1_decoder_parse_glyph( decoder, achar_index );
    if ( error )
      goto Exit;

    
    

    decoder->builder.left_bearing = left_bearing;
    decoder->builder.advance      = advance;

    decoder->builder.pos_x = 0;
    decoder->builder.pos_y = 0;

  Exit:
    return error;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  t1_decoder_parse_charstrings( T1_Decoder  decoder,
                                FT_Byte*    charstring_base,
                                FT_UInt     charstring_len )
  {
    FT_Error         error;
    T1_Decoder_Zone  zone;
    FT_Byte*         ip;
    FT_Byte*         limit;
    T1_Builder       builder = &decoder->builder;
    FT_Pos           x, y, orig_x, orig_y;
    FT_Int           known_othersubr_result_cnt   = 0;
    FT_Int           unknown_othersubr_result_cnt = 0;

    T1_Hints_Funcs   hinter;


    
#define start_point    t1_builder_start_point
#define check_points   t1_builder_check_points
#define add_point      t1_builder_add_point
#define add_point1     t1_builder_add_point1
#define add_contour    t1_builder_add_contour
#define close_contour  t1_builder_close_contour

    
    decoder->top  = decoder->stack;
    decoder->zone = decoder->zones;
    zone          = decoder->zones;

    builder->parse_state = T1_Parse_Start;

    hinter = (T1_Hints_Funcs)builder->hints_funcs;

    
    
    FT_ASSERT( ( decoder->len_buildchar == 0 ) ==
               ( decoder->buildchar == NULL ) );

    if ( decoder->len_buildchar > 0 )
      memset( &decoder->buildchar[0],
              0,
              sizeof( decoder->buildchar[0] ) *
                decoder->len_buildchar );

    FT_TRACE4(( "\nStart charstring\n" ));

    zone->base           = charstring_base;
    limit = zone->limit  = charstring_base + charstring_len;
    ip    = zone->cursor = zone->base;

    error = PSaux_Err_Ok;

    x = orig_x = builder->pos_x;
    y = orig_y = builder->pos_y;

    
    if ( hinter )
      hinter->open( hinter->hints );

    
    while ( ip < limit )
    {
      FT_Long*     top   = decoder->top;
      T1_Operator  op    = op_none;
      FT_Long      value = 0;


      FT_ASSERT( known_othersubr_result_cnt == 0   ||
                 unknown_othersubr_result_cnt == 0 );

      FT_TRACE5(( " (%d)", decoder->top - decoder->stack ));

      
      
      
      
      

      
      switch ( *ip++ )
      {
      case 1:
        op = op_hstem;
        break;

      case 3:
        op = op_vstem;
        break;
      case 4:
        op = op_vmoveto;
        break;
      case 5:
        op = op_rlineto;
        break;
      case 6:
        op = op_hlineto;
        break;
      case 7:
        op = op_vlineto;
        break;
      case 8:
        op = op_rrcurveto;
        break;
      case 9:
        op = op_closepath;
        break;
      case 10:
        op = op_callsubr;
        break;
      case 11:
        op = op_return;
        break;

      case 13:
        op = op_hsbw;
        break;
      case 14:
        op = op_endchar;
        break;

      case 15:          
        op = op_unknown15;
        break;

      case 21:
        op = op_rmoveto;
        break;
      case 22:
        op = op_hmoveto;
        break;

      case 30:
        op = op_vhcurveto;
        break;
      case 31:
        op = op_hvcurveto;
        break;

      case 12:
        if ( ip > limit )
        {
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "invalid escape (12+EOF)\n" ));
          goto Syntax_Error;
        }

        switch ( *ip++ )
        {
        case 0:
          op = op_dotsection;
          break;
        case 1:
          op = op_vstem3;
          break;
        case 2:
          op = op_hstem3;
          break;
        case 6:
          op = op_seac;
          break;
        case 7:
          op = op_sbw;
          break;
        case 12:
          op = op_div;
          break;
        case 16:
          op = op_callothersubr;
          break;
        case 17:
          op = op_pop;
          break;
        case 33:
          op = op_setcurrentpoint;
          break;

        default:
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "invalid escape (12+%d)\n",
                     ip[-1] ));
          goto Syntax_Error;
        }
        break;

      case 255:    
        if ( ip + 4 > limit )
        {
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "unexpected EOF in integer\n" ));
          goto Syntax_Error;
        }

        value = (FT_Int32)( ((FT_Long)ip[0] << 24) |
                            ((FT_Long)ip[1] << 16) |
                            ((FT_Long)ip[2] << 8 ) |
                                      ip[3] );
        ip += 4;
        break;

      default:
        if ( ip[-1] >= 32 )
        {
          if ( ip[-1] < 247 )
            value = (FT_Long)ip[-1] - 139;
          else
          {
            if ( ++ip > limit )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: " ));
              FT_ERROR(( "unexpected EOF in integer\n" ));
              goto Syntax_Error;
            }

            if ( ip[-2] < 251 )
              value =  ( ( (FT_Long)ip[-2] - 247 ) << 8 ) + ip[-1] + 108;
            else
              value = -( ( ( (FT_Long)ip[-2] - 251 ) << 8 ) + ip[-1] + 108 );
          }
        }
        else
        {
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "invalid byte (%d)\n", ip[-1] ));
          goto Syntax_Error;
        }
      }

      if ( unknown_othersubr_result_cnt > 0 )
      {
        switch ( op )
        {
        case op_callsubr:
        case op_return:
        case op_none:
        case op_pop:
          break;

        default:
          
          unknown_othersubr_result_cnt = 0;
          break;
        }
      }

      
      
      
      
      
      if ( op == op_none )
      {
        if ( top - decoder->stack >= T1_MAX_CHARSTRINGS_OPERANDS )
        {
          FT_ERROR(( "t1_decoder_parse_charstrings: stack overflow!\n" ));
          goto Syntax_Error;
        }

        FT_TRACE4(( " %ld", value ));

        *top++       = value;
        decoder->top = top;
      }
      else if ( op == op_callothersubr )  
      {
        FT_Int  subr_no;
        FT_Int  arg_cnt;


        FT_TRACE4(( " callothersubr" ));

        if ( top - decoder->stack < 2 )
          goto Stack_Underflow;

        top -= 2;

        subr_no = (FT_Int)top[1];
        arg_cnt = (FT_Int)top[0];

        
        
        
        
        
        
        
        
        
        

        if ( arg_cnt > top - decoder->stack )
          goto Stack_Underflow;

        top -= arg_cnt;

        known_othersubr_result_cnt   = 0;
        unknown_othersubr_result_cnt = 0;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        switch ( subr_no )
        {
        case 1:                     
          if ( arg_cnt != 0 )
            goto Unexpected_OtherSubr;

          decoder->flex_state        = 1;
          decoder->num_flex_vectors  = 0;
          if ( start_point( builder, x, y ) ||
               check_points( builder, 6 )   )
            goto Fail;
          break;

        case 2:                     
          {
            FT_Int  idx;


            if ( arg_cnt != 0 )
              goto Unexpected_OtherSubr;

            
            
            
            idx = decoder->num_flex_vectors++;
            if ( idx > 0 && idx < 7 )
              add_point( builder,
                         x,
                         y,
                         (FT_Byte)( idx == 3 || idx == 6 ) );
          }
          break;

        case 0:                     
          if ( arg_cnt != 3 )
            goto Unexpected_OtherSubr;

          if ( decoder->flex_state       == 0 ||
               decoder->num_flex_vectors != 7 )
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: "
                       "unexpected flex end\n" ));
            goto Syntax_Error;
          }

          
          known_othersubr_result_cnt = 2;
          break;

        case 3:                     
          if ( arg_cnt != 1 )
            goto Unexpected_OtherSubr;

          known_othersubr_result_cnt = 1;

          if ( hinter )
            hinter->reset( hinter->hints, builder->current->n_points );

          break;

        case 12:
        case 13:
          
          top = decoder->stack;
          break;

        case 14:
        case 15:
        case 16:
        case 17:
        case 18:                    
          {
            PS_Blend  blend = decoder->blend;
            FT_UInt   num_points, nn, mm;
            FT_Long*  delta;
            FT_Long*  values;


            if ( !blend )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: " ));
              FT_ERROR(( "unexpected multiple masters operator!\n" ));
              goto Syntax_Error;
            }

            num_points = (FT_UInt)subr_no - 13 + ( subr_no == 18 );
            if ( arg_cnt != (FT_Int)( num_points * blend->num_designs ) )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: " ));
              FT_ERROR(( "incorrect number of mm arguments\n" ));
              goto Syntax_Error;
            }

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            delta  = top + num_points;
            values = top;
            for ( nn = 0; nn < num_points; nn++ )
            {
              FT_Long  tmp = values[0];


              for ( mm = 1; mm < blend->num_designs; mm++ )
                tmp += FT_MulFix( *delta++, blend->weight_vector[mm] );

              *values++ = tmp;
            }

            known_othersubr_result_cnt = num_points;
            break;
          }

#ifdef CAN_HANDLE_NON_INTEGRAL_T1_OPERANDS

          
          
          

        case 19:
          
          
          
          {
            FT_Int    idx;
            PS_Blend  blend = decoder->blend;


            if ( arg_cnt != 1 || blend == NULL )
              goto Unexpected_OtherSubr;

            idx = top[0];

            if ( idx < 0                                                 ||
                 idx + blend->num_designs > decoder->face->len_buildchar )
              goto Unexpected_OtherSubr;

            memcpy( &decoder->buildchar[idx],
                    blend->weight_vector,
                    blend->num_designs *
                      sizeof( blend->weight_vector[ 0 ] ) );
          }
          break;

        case 20:
          
          
          if ( arg_cnt != 2 )
            goto Unexpected_OtherSubr;

          top[0] += top[1]; 

          known_othersubr_result_cnt = 1;
          break;

        case 21:
          
          
          if ( arg_cnt != 2 )
            goto Unexpected_OtherSubr;

          top[0] -= top[1]; 

          known_othersubr_result_cnt = 1;
          break;

        case 22:
          
          
          if ( arg_cnt != 2 )
            goto Unexpected_OtherSubr;

          top[0] *= top[1]; 

          known_othersubr_result_cnt = 1;
          break;

        case 23:
          
          
          if ( arg_cnt != 2 || top[1] == 0 )
            goto Unexpected_OtherSubr;

          top[0] /= top[1]; 

          known_othersubr_result_cnt = 1;
          break;

#endif

        case 24:
          
          
          {
            FT_Int    idx;
            PS_Blend  blend = decoder->blend;

            if ( arg_cnt != 2 || blend == NULL )
              goto Unexpected_OtherSubr;

            idx = top[1];

            if ( idx < 0 || (FT_UInt) idx >= decoder->len_buildchar )
              goto Unexpected_OtherSubr;

            decoder->buildchar[idx] = top[0];
          }
          break;

        case 25:
          
          
          
          {
            FT_Int    idx;
            PS_Blend  blend = decoder->blend;

            if ( arg_cnt != 1 || blend == NULL )
              goto Unexpected_OtherSubr;

            idx = top[0];

            if ( idx < 0 || (FT_UInt) idx >= decoder->len_buildchar )
              goto Unexpected_OtherSubr;

            top[0] = decoder->buildchar[idx];
          }

          known_othersubr_result_cnt = 1;
          break;

#if 0
        case 26:
          
          
          
          XXX who has left his mark on the (PostScript) stack ?;
          break;
#endif

        case 27:
          
          
          
          if ( arg_cnt != 4 )
            goto Unexpected_OtherSubr;

          if ( top[2] > top[3] )
            top[0] = top[1];

          known_othersubr_result_cnt = 1;
          break;

#ifdef CAN_HANDLE_NON_INTEGRAL_T1_OPERANDS
        case 28:
          
          
          if ( arg_cnt != 0 )
            goto Unexpected_OtherSubr;

          top[0] = FT_rand();
          known_othersubr_result_cnt = 1;
          break;
#endif

        default:
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "unknown othersubr [%d %d], wish me luck!\n",
                     arg_cnt, subr_no ));
          unknown_othersubr_result_cnt = arg_cnt;
          break;

        Unexpected_OtherSubr:
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "invalid othersubr [%d %d]!\n", arg_cnt, subr_no ));
          goto Syntax_Error;
        }

        top += known_othersubr_result_cnt;

        decoder->top = top;
      }
      else  
      {
        FT_Int  num_args = t1_args_count[op];


        FT_ASSERT( num_args >= 0 );

        if ( top - decoder->stack < num_args )
          goto Stack_Underflow;

        
        
        
        
        

#ifdef FT_DEBUG_LEVEL_TRACE

        switch ( op )
        {
        case op_callsubr:
        case op_div:
        case op_callothersubr:
        case op_pop:
        case op_return:
          break;

        default:
          if ( top - decoder->stack != num_args )
            FT_TRACE0(( "t1_decoder_parse_charstrings: "
                        "too much operands on the stack "
                        "(seen %d, expected %d)\n",
                        top - decoder->stack, num_args ));
            break;
        }

#endif 

        top -= num_args;

        switch ( op )
        {
        case op_endchar:
          FT_TRACE4(( " endchar" ));

          close_contour( builder );

          
          if ( hinter )
          {
            if (hinter->close( hinter->hints, builder->current->n_points ))
              goto Syntax_Error;

            
            hinter->apply( hinter->hints,
                           builder->current,
                           (PSH_Globals) builder->hints_globals,
                           decoder->hint_mode );
          }

          
          FT_GlyphLoader_Add( builder->loader );

          FT_TRACE4(( "\n" ));

          

#ifdef FT_DEBUG_LEVEL_TRACE

          if ( decoder->len_buildchar > 0 )
          {
            FT_UInt  i;


            FT_TRACE4(( "BuildCharArray = [ " ));

            for ( i = 0; i < decoder->len_buildchar; ++i )
              FT_TRACE4(( "%d ", decoder->buildchar[ i ] ));

            FT_TRACE4(( "]\n" ));
          }

#endif 

          FT_TRACE4(( "\n" ));

          
          return PSaux_Err_Ok;

        case op_hsbw:
          FT_TRACE4(( " hsbw" ));

          builder->parse_state = T1_Parse_Have_Width;

          builder->left_bearing.x += top[0];
          builder->advance.x       = top[1];
          builder->advance.y       = 0;

          orig_x = builder->last.x = x = builder->pos_x + top[0];
          orig_y = builder->last.y = y = builder->pos_y;

          FT_UNUSED( orig_y );

          
          
          
          if ( builder->metrics_only )
            return PSaux_Err_Ok;

          break;

        case op_seac:
          
          return t1operator_seac( decoder, top[0], top[1], top[2],
                                           (FT_Int)top[3], (FT_Int)top[4] );

        case op_sbw:
          FT_TRACE4(( " sbw" ));

          builder->parse_state = T1_Parse_Have_Width;

          builder->left_bearing.x += top[0];
          builder->left_bearing.y += top[1];
          builder->advance.x       = top[2];
          builder->advance.y       = top[3];

          builder->last.x = x = builder->pos_x + top[0];
          builder->last.y = y = builder->pos_y + top[1];

          
          
          
          if ( builder->metrics_only )
            return PSaux_Err_Ok;

          break;

        case op_closepath:
          FT_TRACE4(( " closepath" ));

          
          if ( builder->parse_state == T1_Parse_Have_Path   ||
               builder->parse_state == T1_Parse_Have_Moveto )
            close_contour( builder );

          builder->parse_state = T1_Parse_Have_Width;
          break;

        case op_hlineto:
          FT_TRACE4(( " hlineto" ));

          if ( start_point( builder, x, y ) )
            goto Fail;

          x += top[0];
          goto Add_Line;

        case op_hmoveto:
          FT_TRACE4(( " hmoveto" ));

          x += top[0];
          if ( !decoder->flex_state )
          {
            if ( builder->parse_state == T1_Parse_Start )
              goto Syntax_Error;
            builder->parse_state = T1_Parse_Have_Moveto;
          }
          break;

        case op_hvcurveto:
          FT_TRACE4(( " hvcurveto" ));

          if ( start_point( builder, x, y ) ||
               check_points( builder, 3 )   )
            goto Fail;

          x += top[0];
          add_point( builder, x, y, 0 );
          x += top[1];
          y += top[2];
          add_point( builder, x, y, 0 );
          y += top[3];
          add_point( builder, x, y, 1 );
          break;

        case op_rlineto:
          FT_TRACE4(( " rlineto" ));

          if ( start_point( builder, x, y ) )
            goto Fail;

          x += top[0];
          y += top[1];

        Add_Line:
          if ( add_point1( builder, x, y ) )
            goto Fail;
          break;

        case op_rmoveto:
          FT_TRACE4(( " rmoveto" ));

          x += top[0];
          y += top[1];
          if ( !decoder->flex_state )
          {
            if ( builder->parse_state == T1_Parse_Start )
              goto Syntax_Error;
            builder->parse_state = T1_Parse_Have_Moveto;
          }
          break;

        case op_rrcurveto:
          FT_TRACE4(( " rcurveto" ));

          if ( start_point( builder, x, y ) ||
               check_points( builder, 3 )   )
            goto Fail;

          x += top[0];
          y += top[1];
          add_point( builder, x, y, 0 );

          x += top[2];
          y += top[3];
          add_point( builder, x, y, 0 );

          x += top[4];
          y += top[5];
          add_point( builder, x, y, 1 );
          break;

        case op_vhcurveto:
          FT_TRACE4(( " vhcurveto" ));

          if ( start_point( builder, x, y ) ||
               check_points( builder, 3 )   )
            goto Fail;

          y += top[0];
          add_point( builder, x, y, 0 );
          x += top[1];
          y += top[2];
          add_point( builder, x, y, 0 );
          x += top[3];
          add_point( builder, x, y, 1 );
          break;

        case op_vlineto:
          FT_TRACE4(( " vlineto" ));

          if ( start_point( builder, x, y ) )
            goto Fail;

          y += top[0];
          goto Add_Line;

        case op_vmoveto:
          FT_TRACE4(( " vmoveto" ));

          y += top[0];
          if ( !decoder->flex_state )
          {
            if ( builder->parse_state == T1_Parse_Start )
              goto Syntax_Error;
            builder->parse_state = T1_Parse_Have_Moveto;
          }
          break;

        case op_div:
          FT_TRACE4(( " div" ));

          if ( top[1] )
          {
            *top = top[0] / top[1];
            ++top;
          }
          else
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: division by 0\n" ));
            goto Syntax_Error;
          }
          break;

        case op_callsubr:
          {
            FT_Int  idx;


            FT_TRACE4(( " callsubr" ));

            idx = (FT_Int)top[0];
            if ( idx < 0 || idx >= (FT_Int)decoder->num_subrs )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: "
                         "invalid subrs index\n" ));
              goto Syntax_Error;
            }

            if ( zone - decoder->zones >= T1_MAX_SUBRS_CALLS )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: "
                         "too many nested subrs\n" ));
              goto Syntax_Error;
            }

            zone->cursor = ip;  

            zone++;

            
            
            
            zone->base = decoder->subrs[idx];

            if ( decoder->subrs_len )
              zone->limit = zone->base + decoder->subrs_len[idx];
            else
            {
              
              
              zone->base  += ( decoder->lenIV >= 0 ? decoder->lenIV : 0 );
              zone->limit  = decoder->subrs[idx + 1];
            }

            zone->cursor = zone->base;

            if ( !zone->base )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: "
                         "invoking empty subrs!\n" ));
              goto Syntax_Error;
            }

            decoder->zone = zone;
            ip            = zone->base;
            limit         = zone->limit;
            break;
          }

        case op_pop:
          FT_TRACE4(( " pop" ));

          if ( known_othersubr_result_cnt > 0 )
          {
            known_othersubr_result_cnt--;
            
            break;
          }

          if ( unknown_othersubr_result_cnt == 0 )
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: "
                       "no more operands for othersubr!\n" ));
            goto Syntax_Error;
          }

          unknown_othersubr_result_cnt--;
          top++;   
          break;

        case op_return:
          FT_TRACE4(( " return" ));

          if ( zone <= decoder->zones )
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: unexpected return\n" ));
            goto Syntax_Error;
          }

          zone--;
          ip            = zone->cursor;
          limit         = zone->limit;
          decoder->zone = zone;
          break;

        case op_dotsection:
          FT_TRACE4(( " dotsection" ));

          break;

        case op_hstem:
          FT_TRACE4(( " hstem" ));

          
          if ( hinter )
          {
            
            hinter->stem( hinter->hints, 1, top );
          }

          break;

        case op_hstem3:
          FT_TRACE4(( " hstem3" ));

          
          if ( hinter )
            hinter->stem3( hinter->hints, 1, top );

          break;

        case op_vstem:
          FT_TRACE4(( " vstem" ));

          
          if ( hinter )
          {
            top[0] += orig_x;
            hinter->stem( hinter->hints, 0, top );
          }

          break;

        case op_vstem3:
          FT_TRACE4(( " vstem3" ));

          
          if ( hinter )
          {
            FT_Pos  dx = orig_x;


            top[0] += dx;
            top[2] += dx;
            top[4] += dx;
            hinter->stem3( hinter->hints, 0, top );
          }
          break;

        case op_setcurrentpoint:
          FT_TRACE4(( " setcurrentpoint" ));

          
          
          
          

          
          if ( decoder->flex_state != 1 )
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: " ));
            FT_ERROR(( "unexpected `setcurrentpoint'\n" ));

            goto Syntax_Error;
          }
          else
            decoder->flex_state = 0;
          break;

        case op_unknown15:
          FT_TRACE4(( " opcode_15" ));
          
          break;

        default:
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "unhandled opcode %d\n", op ));
          goto Syntax_Error;
        }

        
        
        
        

        decoder->top = top;

      } 

    } 

    FT_TRACE4(( "..end..\n\n" ));

  Fail:
    return error;

  Syntax_Error:
    return PSaux_Err_Syntax_Error;

  Stack_Underflow:
    return PSaux_Err_Stack_Underflow;
  }


  
  FT_LOCAL_DEF( FT_Error )
  t1_decoder_parse_glyph( T1_Decoder  decoder,
                          FT_UInt     glyph )
  {
    return decoder->parse_callback( decoder, glyph );
  }


  
  FT_LOCAL_DEF( FT_Error )
  t1_decoder_init( T1_Decoder           decoder,
                   FT_Face              face,
                   FT_Size              size,
                   FT_GlyphSlot         slot,
                   FT_Byte**            glyph_names,
                   PS_Blend             blend,
                   FT_Bool              hinting,
                   FT_Render_Mode       hint_mode,
                   T1_Decoder_Callback  parse_callback )
  {
    FT_MEM_ZERO( decoder, sizeof ( *decoder ) );

    
    {
      FT_Service_PsCMaps  psnames = 0;


      FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );
      if ( !psnames )
      {
        FT_ERROR(( "t1_decoder_init: " ));
        FT_ERROR(( "the `psnames' module is not available\n" ));
        return PSaux_Err_Unimplemented_Feature;
      }

      decoder->psnames = psnames;
    }

    t1_builder_init( &decoder->builder, face, size, slot, hinting );

    
    
    

    decoder->num_glyphs     = (FT_UInt)face->num_glyphs;
    decoder->glyph_names    = glyph_names;
    decoder->hint_mode      = hint_mode;
    decoder->blend          = blend;
    decoder->parse_callback = parse_callback;

    decoder->funcs          = t1_decoder_funcs;

    return PSaux_Err_Ok;
  }


  
  FT_LOCAL_DEF( void )
  t1_decoder_done( T1_Decoder  decoder )
  {
    t1_builder_done( &decoder->builder );
  }




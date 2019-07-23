

















#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_STREAM_H

#include "cidparse.h"

#include "ciderrs.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cidparse


  
  
  
  
  
  
  
  
  


  FT_LOCAL_DEF( FT_Error )
  cid_parser_new( CID_Parser*    parser,
                  FT_Stream      stream,
                  FT_Memory      memory,
                  PSAux_Service  psaux )
  {
    FT_Error  error;
    FT_ULong  base_offset, offset, ps_len;
    FT_Byte   *cur, *limit;
    FT_Byte   *arg1, *arg2;


    FT_MEM_ZERO( parser, sizeof ( *parser ) );
    psaux->ps_parser_funcs->init( &parser->root, 0, 0, memory );

    parser->stream = stream;

    base_offset = FT_STREAM_POS();

    
    if ( FT_FRAME_ENTER( 31 ) )
      goto Exit;

    if ( ft_strncmp( (char *)stream->cursor,
                     "%!PS-Adobe-3.0 Resource-CIDFont", 31 ) )
    {
      FT_TRACE2(( "[not a valid CID-keyed font]\n" ));
      error = CID_Err_Unknown_File_Format;
    }

    FT_FRAME_EXIT();
    if ( error )
      goto Exit;

  Again:
    
    
    {
      FT_Byte   buffer[256 + 10];
      FT_Int    read_len = 256 + 10;
      FT_Byte*  p        = buffer;


      for ( offset = (FT_ULong)FT_STREAM_POS(); ; offset += 256 )
      {
        FT_Int  stream_len;


        stream_len = stream->size - FT_STREAM_POS();
        if ( stream_len == 0 )
        {
          FT_TRACE2(( "cid_parser_new: no `StartData' keyword found\n" ));
          error = CID_Err_Unknown_File_Format;
          goto Exit;
        }

        read_len = FT_MIN( read_len, stream_len );
        if ( FT_STREAM_READ( p, read_len ) )
          goto Exit;

        if ( read_len < 256 )
          p[read_len]  = '\0';

        limit = p + read_len - 10;

        for ( p = buffer; p < limit; p++ )
        {
          if ( p[0] == 'S' && ft_strncmp( (char*)p, "StartData", 9 ) == 0 )
          {
            
            offset += p - buffer + 10;
            goto Found;
          }
          else if ( p[1] == 's' && ft_strncmp( (char*)p, "/sfnts", 6 ) == 0 )
          {
            offset += p - buffer + 7;
            goto Found;
          }
        }

        FT_MEM_MOVE( buffer, p, 10 );
        read_len = 256;
        p = buffer + 10;
      }
    }

  Found:
    
    
    

    ps_len = offset - base_offset;
    if ( FT_STREAM_SEEK( base_offset )                  ||
         FT_FRAME_EXTRACT( ps_len, parser->postscript ) )
      goto Exit;

    parser->data_offset    = offset;
    parser->postscript_len = ps_len;
    parser->root.base      = parser->postscript;
    parser->root.cursor    = parser->postscript;
    parser->root.limit     = parser->root.cursor + ps_len;
    parser->num_dict       = -1;

    
    
    
    

    arg1 = parser->root.cursor;
    cid_parser_skip_PS_token( parser );
    cid_parser_skip_spaces  ( parser );
    arg2 = parser->root.cursor;
    cid_parser_skip_PS_token( parser );
    cid_parser_skip_spaces  ( parser );

    limit = parser->root.limit;
    cur   = parser->root.cursor;

    while ( cur < limit )
    {
      if ( parser->root.error )
      {
        error = parser->root.error;
        goto Exit;
      }

      if ( cur[0] == 'S' && ft_strncmp( (char*)cur, "StartData", 9 ) == 0 )
      {
        if ( ft_strncmp( (char*)arg1, "(Hex)", 5 ) == 0 )
          parser->binary_length = ft_atol( (const char *)arg2 );

        limit = parser->root.limit;
        cur   = parser->root.cursor;
        goto Exit;
      }
      else if ( cur[1] == 's' && ft_strncmp( (char*)cur, "/sfnts", 6 ) == 0 )
      {
        FT_TRACE2(( "cid_parser_new: cannot handle Type 11 fonts\n" ));
        error = CID_Err_Unknown_File_Format;
        goto Exit;
      }

      cid_parser_skip_PS_token( parser );
      cid_parser_skip_spaces  ( parser );
      arg1 = arg2;
      arg2 = cur;
      cur  = parser->root.cursor;
    }

    
    
    FT_FRAME_RELEASE( parser->postscript );
    if ( !FT_STREAM_SEEK( offset ) )
      goto Again;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  cid_parser_done( CID_Parser*  parser )
  {
    
    if ( parser->postscript )
    {
      FT_Stream  stream = parser->stream;


      FT_FRAME_RELEASE( parser->postscript );
    }
    parser->root.funcs.done( &parser->root );
  }




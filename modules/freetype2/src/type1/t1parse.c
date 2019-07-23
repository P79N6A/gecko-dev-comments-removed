

















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H

#include "t1parse.h"

#include "t1errors.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1parse


  
  
  
  
  
  
  
  
  


  

  static FT_Error
  read_pfb_tag( FT_Stream   stream,
                FT_UShort  *atag,
                FT_ULong   *asize )
  {
    FT_Error   error;
    FT_UShort  tag;
    FT_ULong   size;


    *atag  = 0;
    *asize = 0;

    if ( !FT_READ_USHORT( tag ) )
    {
      if ( tag == 0x8001U || tag == 0x8002U )
      {
        if ( !FT_READ_ULONG_LE( size ) )
          *asize = size;
      }

      *atag = tag;
    }

    return error;
  }


  static FT_Error
  check_type1_format( FT_Stream    stream,
                      const char*  header_string,
                      size_t       header_length )
  {
    FT_Error   error;
    FT_UShort  tag;
    FT_ULong   dummy;


    if ( FT_STREAM_SEEK( 0 ) )
      goto Exit;

    error = read_pfb_tag( stream, &tag, &dummy );
    if ( error )
      goto Exit;

    
    
    
    if ( tag != 0x8001U && FT_STREAM_SEEK( 0 ) )
      goto Exit;

    if ( !FT_FRAME_ENTER( header_length ) )
    {
      error = T1_Err_Ok;

      if ( ft_memcmp( stream->cursor, header_string, header_length ) != 0 )
        error = T1_Err_Unknown_File_Format;

      FT_FRAME_EXIT();
    }

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  T1_New_Parser( T1_Parser      parser,
                 FT_Stream      stream,
                 FT_Memory      memory,
                 PSAux_Service  psaux )
  {
    FT_Error   error;
    FT_UShort  tag;
    FT_ULong   size;


    psaux->ps_parser_funcs->init( &parser->root, 0, 0, memory );

    parser->stream       = stream;
    parser->base_len     = 0;
    parser->base_dict    = 0;
    parser->private_len  = 0;
    parser->private_dict = 0;
    parser->in_pfb       = 0;
    parser->in_memory    = 0;
    parser->single_block = 0;

    
    error = check_type1_format( stream, "%!PS-AdobeFont", 14 );
    if ( error )
    {
      if ( error != T1_Err_Unknown_File_Format )
        goto Exit;

      error = check_type1_format( stream, "%!FontType", 10 );
      if ( error )
      {
        FT_TRACE2(( "[not a Type1 font]\n" ));
        goto Exit;
      }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    if ( FT_STREAM_SEEK( 0L ) )
      goto Exit;

    error = read_pfb_tag( stream, &tag, &size );
    if ( error )
      goto Exit;

    if ( tag != 0x8001U )
    {
      
      
      if ( FT_STREAM_SEEK( 0L ) )
        goto Exit;
      size = stream->size;
    }
    else
      parser->in_pfb = 1;

    
    

    
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
  T1_Finalize_Parser( T1_Parser  parser )
  {
    FT_Memory  memory = parser->root.memory;


    
    FT_FREE( parser->private_dict );

    
    if ( !parser->in_memory )
      FT_FREE( parser->base_dict );

    parser->root.funcs.done( &parser->root );
  }


  FT_LOCAL_DEF( FT_Error )
  T1_Get_Private_Dict( T1_Parser      parser,
                       PSAux_Service  psaux )
  {
    FT_Stream  stream = parser->stream;
    FT_Memory  memory = parser->root.memory;
    FT_Error   error  = T1_Err_Ok;
    FT_ULong   size;


    if ( parser->in_pfb )
    {
      
      
      
      
      FT_Long    start_pos = FT_STREAM_POS();
      FT_UShort  tag;


      parser->private_len = 0;
      for (;;)
      {
        error = read_pfb_tag( stream, &tag, &size );
        if ( error )
          goto Fail;

        if ( tag != 0x8002U )
          break;

        parser->private_len += size;

        if ( FT_STREAM_SKIP( size ) )
          goto Fail;
      }

      
      
      if ( parser->private_len == 0 )
      {
        FT_ERROR(( "T1_Get_Private_Dict:" ));
        FT_ERROR(( " invalid private dictionary section\n" ));
        error = T1_Err_Invalid_File_Format;
        goto Fail;
      }

      if ( FT_STREAM_SEEK( start_pos )                           ||
           FT_ALLOC( parser->private_dict, parser->private_len ) )
        goto Fail;

      parser->private_len = 0;
      for (;;)
      {
        error = read_pfb_tag( stream, &tag, &size );
        if ( error || tag != 0x8002U )
        {
          error = T1_Err_Ok;
          break;
        }

        if ( FT_STREAM_READ( parser->private_dict + parser->private_len,
                             size ) )
          goto Fail;

        parser->private_len += size;
      }
    }
    else
    {
      
      
      
      

      
      FT_Byte*  cur   = parser->base_dict;
      FT_Byte*  limit = cur + parser->base_len;
      FT_Byte   c;


    Again:
      for (;;)
      {
        c = cur[0];
        if ( c == 'e' && cur + 9 < limit )  
                                            
        {
          if ( cur[1] == 'e' &&
               cur[2] == 'x' &&
               cur[3] == 'e' &&
               cur[4] == 'c' )
            break;
        }
        cur++;
        if ( cur >= limit )
        {
          FT_ERROR(( "T1_Get_Private_Dict:" ));
          FT_ERROR(( " could not find `eexec' keyword\n" ));
          error = T1_Err_Invalid_File_Format;
          goto Exit;
        }
      }

      
      

      parser->root.cursor = parser->base_dict;
      parser->root.limit  = cur + 9;

      cur   = parser->root.cursor;
      limit = parser->root.limit;

      while ( cur < limit )
      {
        if ( *cur == 'e' && ft_strncmp( (char*)cur, "eexec", 5 ) == 0 )
          goto Found;

        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          break;
        T1_Skip_Spaces  ( parser );
        cur = parser->root.cursor;
      }

      
      

      cur   = limit;
      limit = parser->base_dict + parser->base_len;
      goto Again;

      
      
      

    Found:
      parser->root.limit = parser->base_dict + parser->base_len;

      T1_Skip_PS_Token( parser );
      cur = parser->root.cursor;
      if ( *cur == '\r' )
      {
        cur++;
        if ( *cur == '\n' )
          cur++;
      }
      else if ( *cur == '\n' )
        cur++;
      else
      {
        FT_ERROR(( "T1_Get_Private_Dict:" ));
        FT_ERROR(( " `eexec' not properly terminated\n" ));
        error = T1_Err_Invalid_File_Format;
        goto Exit;
      }

      size = parser->base_len - ( cur - parser->base_dict );

      if ( parser->in_memory )
      {
        
        if ( FT_ALLOC( parser->private_dict, size + 1 ) )
          goto Fail;
        parser->private_len = size;
      }
      else
      {
        parser->single_block = 1;
        parser->private_dict = parser->base_dict;
        parser->private_len  = size;
        parser->base_dict    = 0;
        parser->base_len     = 0;
      }

      
      

      
      
      

      if ( ft_isxdigit( cur[0] ) && ft_isxdigit( cur[1] ) &&
           ft_isxdigit( cur[2] ) && ft_isxdigit( cur[3] ) )
      {
        
        FT_Long  len;


        parser->root.cursor = cur;
        (void)psaux->ps_parser_funcs->to_bytes( &parser->root,
                                                parser->private_dict,
                                                parser->private_len,
                                                &len,
                                                0 );
        parser->private_len = len;

        
        parser->private_dict[len] = '\0';
      }
      else
        
        FT_MEM_MOVE( parser->private_dict, cur, size );
    }

    
    psaux->t1_decrypt( parser->private_dict, parser->private_len, 55665U );

    
    parser->private_dict[0] = ' ';
    parser->private_dict[1] = ' ';
    parser->private_dict[2] = ' ';
    parser->private_dict[3] = ' ';

    parser->root.base   = parser->private_dict;
    parser->root.cursor = parser->private_dict;
    parser->root.limit  = parser->root.cursor + parser->private_len;

  Fail:
  Exit:
    return error;
  }




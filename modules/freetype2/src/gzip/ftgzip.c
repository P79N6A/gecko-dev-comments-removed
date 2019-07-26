





















#include <ft2build.h>
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_DEBUG_H
#include FT_GZIP_H
#include FT_CONFIG_STANDARD_LIBRARY_H


#include FT_MODULE_ERRORS_H

#undef __FTERRORS_H__

#undef  FT_ERR_PREFIX
#define FT_ERR_PREFIX  Gzip_Err_
#define FT_ERR_BASE    FT_Mod_Err_Gzip

#include FT_ERRORS_H


#ifdef FT_CONFIG_OPTION_USE_ZLIB

#ifdef FT_CONFIG_OPTION_PIC
#error "gzip code does not support PIC yet"
#endif

#ifdef FT_CONFIG_OPTION_SYSTEM_ZLIB

#include <zlib.h>

#else 

 
 
 
 
 
 

#define NO_DUMMY_DECL
#ifndef USE_ZLIB_ZCALLOC
#define MY_ZCALLOC
#endif

#include "zlib.h"

#undef  SLOW
#define SLOW  1  /* we can't use asm-optimized sources here! */

#if defined( _MSC_VER )      
  
  
  
  
#pragma warning( push )
#pragma warning( disable : 4244 )
#endif 

  

#define NO_INFLATE_MASK
#include "zutil.h"
#include "inftrees.h"
#include "infblock.h"
#include "infcodes.h"
#include "infutil.h"
#undef  NO_INFLATE_MASK

  
#include "zutil.c"
#include "inftrees.c"
#include "infutil.c"
#include "infcodes.c"
#include "infblock.c"
#include "inflate.c"
#include "adler32.c"

#if defined( _MSC_VER )
#pragma warning( pop )
#endif

#endif 










  


  static voidpf
  ft_gzip_alloc( FT_Memory  memory,
                 uInt       items,
                 uInt       size )
  {
    FT_ULong    sz = (FT_ULong)size * items;
    FT_Error    error;
    FT_Pointer  p  = NULL;


    (void)FT_ALLOC( p, sz );
    return p;
  }


  static void
  ft_gzip_free( FT_Memory  memory,
                voidpf     address )
  {
    FT_MEM_FREE( address );
  }


#if !defined( FT_CONFIG_OPTION_SYSTEM_ZLIB ) && !defined( USE_ZLIB_ZCALLOC )

  local voidpf
  zcalloc ( voidpf    opaque,
            unsigned  items,
            unsigned  size )
  {
    return ft_gzip_alloc( (FT_Memory)opaque, items, size );
  }

  local void
  zcfree( voidpf  opaque,
          voidpf  ptr )
  {
    ft_gzip_free( (FT_Memory)opaque, ptr );
  }

#endif 










#define FT_GZIP_BUFFER_SIZE  4096

  typedef struct  FT_GZipFileRec_
  {
    FT_Stream  source;         
    FT_Stream  stream;         
    FT_Memory  memory;         
    z_stream   zstream;        

    FT_ULong   start;          
    FT_Byte    input[FT_GZIP_BUFFER_SIZE];   

    FT_Byte    buffer[FT_GZIP_BUFFER_SIZE];  
    FT_ULong   pos;                          
    FT_Byte*   cursor;
    FT_Byte*   limit;

  } FT_GZipFileRec, *FT_GZipFile;


  
#define FT_GZIP_ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define FT_GZIP_HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define FT_GZIP_EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define FT_GZIP_ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define FT_GZIP_COMMENT      0x10 /* bit 4 set: file comment present */
#define FT_GZIP_RESERVED     0xE0 /* bits 5..7: reserved */


  
  static FT_Error
  ft_gzip_check_header( FT_Stream  stream )
  {
    FT_Error  error;
    FT_Byte   head[4];


    if ( FT_STREAM_SEEK( 0 )       ||
         FT_STREAM_READ( head, 4 ) )
      goto Exit;

    
    
    if ( head[0] != 0x1f              ||
         head[1] != 0x8b              ||
         head[2] != Z_DEFLATED        ||
        (head[3] & FT_GZIP_RESERVED)  )
    {
      error = FT_THROW( Invalid_File_Format );
      goto Exit;
    }

    
    (void)FT_STREAM_SKIP( 6 );

    
    if ( head[3] & FT_GZIP_EXTRA_FIELD )
    {
      FT_UInt  len;


      if ( FT_READ_USHORT_LE( len ) ||
           FT_STREAM_SKIP( len )    )
        goto Exit;
    }

    
    if ( head[3] & FT_GZIP_ORIG_NAME )
      for (;;)
      {
        FT_UInt  c;


        if ( FT_READ_BYTE( c ) )
          goto Exit;

        if ( c == 0 )
          break;
      }

    
    if ( head[3] & FT_GZIP_COMMENT )
      for (;;)
      {
        FT_UInt  c;


        if ( FT_READ_BYTE( c ) )
          goto Exit;

        if ( c == 0 )
          break;
      }

    
    if ( head[3] & FT_GZIP_HEAD_CRC )
      if ( FT_STREAM_SKIP( 2 ) )
        goto Exit;

  Exit:
    return error;
  }


  static FT_Error
  ft_gzip_file_init( FT_GZipFile  zip,
                     FT_Stream    stream,
                     FT_Stream    source )
  {
    z_stream*  zstream = &zip->zstream;
    FT_Error   error   = FT_Err_Ok;


    zip->stream = stream;
    zip->source = source;
    zip->memory = stream->memory;

    zip->limit  = zip->buffer + FT_GZIP_BUFFER_SIZE;
    zip->cursor = zip->limit;
    zip->pos    = 0;

    
    {
      stream = source;

      error = ft_gzip_check_header( stream );
      if ( error )
        goto Exit;

      zip->start = FT_STREAM_POS();
    }

    
    zstream->zalloc = (alloc_func)ft_gzip_alloc;
    zstream->zfree  = (free_func) ft_gzip_free;
    zstream->opaque = stream->memory;

    zstream->avail_in = 0;
    zstream->next_in  = zip->buffer;

    if ( inflateInit2( zstream, -MAX_WBITS ) != Z_OK ||
         zstream->next_in == NULL                     )
      error = FT_THROW( Invalid_File_Format );

  Exit:
    return error;
  }


  static void
  ft_gzip_file_done( FT_GZipFile  zip )
  {
    z_stream*  zstream = &zip->zstream;


    inflateEnd( zstream );

    
    zstream->zalloc    = NULL;
    zstream->zfree     = NULL;
    zstream->opaque    = NULL;
    zstream->next_in   = NULL;
    zstream->next_out  = NULL;
    zstream->avail_in  = 0;
    zstream->avail_out = 0;

    zip->memory = NULL;
    zip->source = NULL;
    zip->stream = NULL;
  }


  static FT_Error
  ft_gzip_file_reset( FT_GZipFile  zip )
  {
    FT_Stream  stream = zip->source;
    FT_Error   error;


    if ( !FT_STREAM_SEEK( zip->start ) )
    {
      z_stream*  zstream = &zip->zstream;


      inflateReset( zstream );

      zstream->avail_in  = 0;
      zstream->next_in   = zip->input;
      zstream->avail_out = 0;
      zstream->next_out  = zip->buffer;

      zip->limit  = zip->buffer + FT_GZIP_BUFFER_SIZE;
      zip->cursor = zip->limit;
      zip->pos    = 0;
    }

    return error;
  }


  static FT_Error
  ft_gzip_file_fill_input( FT_GZipFile  zip )
  {
    z_stream*  zstream = &zip->zstream;
    FT_Stream  stream  = zip->source;
    FT_ULong   size;


    if ( stream->read )
    {
      size = stream->read( stream, stream->pos, zip->input,
                           FT_GZIP_BUFFER_SIZE );
      if ( size == 0 )
        return FT_THROW( Invalid_Stream_Operation );
    }
    else
    {
      size = stream->size - stream->pos;
      if ( size > FT_GZIP_BUFFER_SIZE )
        size = FT_GZIP_BUFFER_SIZE;

      if ( size == 0 )
        return FT_THROW( Invalid_Stream_Operation );

      FT_MEM_COPY( zip->input, stream->base + stream->pos, size );
    }
    stream->pos += size;

    zstream->next_in  = zip->input;
    zstream->avail_in = size;

    return FT_Err_Ok;
  }


  static FT_Error
  ft_gzip_file_fill_output( FT_GZipFile  zip )
  {
    z_stream*  zstream = &zip->zstream;
    FT_Error   error   = FT_Err_Ok;


    zip->cursor        = zip->buffer;
    zstream->next_out  = zip->cursor;
    zstream->avail_out = FT_GZIP_BUFFER_SIZE;

    while ( zstream->avail_out > 0 )
    {
      int  err;


      if ( zstream->avail_in == 0 )
      {
        error = ft_gzip_file_fill_input( zip );
        if ( error )
          break;
      }

      err = inflate( zstream, Z_NO_FLUSH );

      if ( err == Z_STREAM_END )
      {
        zip->limit = zstream->next_out;
        if ( zip->limit == zip->cursor )
          error = FT_THROW( Invalid_Stream_Operation );
        break;
      }
      else if ( err != Z_OK )
      {
        error = FT_THROW( Invalid_Stream_Operation );
        break;
      }
    }

    return error;
  }


  
  static FT_Error
  ft_gzip_file_skip_output( FT_GZipFile  zip,
                            FT_ULong     count )
  {
    FT_Error  error = FT_Err_Ok;
    FT_ULong  delta;


    for (;;)
    {
      delta = (FT_ULong)( zip->limit - zip->cursor );
      if ( delta >= count )
        delta = count;

      zip->cursor += delta;
      zip->pos    += delta;

      count -= delta;
      if ( count == 0 )
        break;

      error = ft_gzip_file_fill_output( zip );
      if ( error )
        break;
    }

    return error;
  }


  static FT_ULong
  ft_gzip_file_io( FT_GZipFile  zip,
                   FT_ULong     pos,
                   FT_Byte*     buffer,
                   FT_ULong     count )
  {
    FT_ULong  result = 0;
    FT_Error  error;


    
    
    if ( pos < zip->pos )
    {
      error = ft_gzip_file_reset( zip );
      if ( error )
        goto Exit;
    }

    
    if ( pos > zip->pos )
    {
      error = ft_gzip_file_skip_output( zip, (FT_ULong)( pos - zip->pos ) );
      if ( error )
        goto Exit;
    }

    if ( count == 0 )
      goto Exit;

    
    for (;;)
    {
      FT_ULong  delta;


      delta = (FT_ULong)( zip->limit - zip->cursor );
      if ( delta >= count )
        delta = count;

      FT_MEM_COPY( buffer, zip->cursor, delta );
      buffer      += delta;
      result      += delta;
      zip->cursor += delta;
      zip->pos    += delta;

      count -= delta;
      if ( count == 0 )
        break;

      error = ft_gzip_file_fill_output( zip );
      if ( error )
        break;
    }

  Exit:
    return result;
  }










  static void
  ft_gzip_stream_close( FT_Stream  stream )
  {
    FT_GZipFile  zip    = (FT_GZipFile)stream->descriptor.pointer;
    FT_Memory    memory = stream->memory;


    if ( zip )
    {
      
      ft_gzip_file_done( zip );

      FT_FREE( zip );

      stream->descriptor.pointer = NULL;
    }
  }


  static FT_ULong
  ft_gzip_stream_io( FT_Stream  stream,
                     FT_ULong   pos,
                     FT_Byte*   buffer,
                     FT_ULong   count )
  {
    FT_GZipFile  zip = (FT_GZipFile)stream->descriptor.pointer;


    return ft_gzip_file_io( zip, pos, buffer, count );
  }


  static FT_ULong
  ft_gzip_get_uncompressed_size( FT_Stream  stream )
  {
    FT_Error  error;
    FT_ULong  old_pos;
    FT_ULong  result = 0;


    old_pos = stream->pos;
    if ( !FT_Stream_Seek( stream, stream->size - 4 ) )
    {
      result = FT_Stream_ReadULong( stream, &error );
      if ( error )
        result = 0;

      (void)FT_Stream_Seek( stream, old_pos );
    }

    return result;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Stream_OpenGzip( FT_Stream  stream,
                      FT_Stream  source )
  {
    FT_Error     error;
    FT_Memory    memory = source->memory;
    FT_GZipFile  zip = NULL;


    



    error = ft_gzip_check_header( source );
    if ( error )
      goto Exit;

    FT_ZERO( stream );
    stream->memory = memory;

    if ( !FT_QNEW( zip ) )
    {
      error = ft_gzip_file_init( zip, stream, source );
      if ( error )
      {
        FT_FREE( zip );
        goto Exit;
      }

      stream->descriptor.pointer = zip;
    }

    







    {
      FT_ULong  zip_size = ft_gzip_get_uncompressed_size( source );


      if ( zip_size != 0 && zip_size < 40 * 1024 )
      {
        FT_Byte*  zip_buff = NULL;


        if ( !FT_ALLOC( zip_buff, zip_size ) )
        {
          FT_ULong  count;


          count = ft_gzip_file_io( zip, 0, zip_buff, zip_size );
          if ( count == zip_size )
          {
            ft_gzip_file_done( zip );
            FT_FREE( zip );

            stream->descriptor.pointer = NULL;

            stream->size  = zip_size;
            stream->pos   = 0;
            stream->base  = zip_buff;
            stream->read  = NULL;
            stream->close = ft_gzip_stream_close;

            goto Exit;
          }

          ft_gzip_file_io( zip, 0, NULL, 0 );
          FT_FREE( zip_buff );
        }
        error = FT_Err_Ok;
      }
    }

    stream->size  = 0x7FFFFFFFL;  
    stream->pos   = 0;
    stream->base  = 0;
    stream->read  = ft_gzip_stream_io;
    stream->close = ft_gzip_stream_close;

  Exit:
    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Gzip_Uncompress( FT_Memory       memory,
                      FT_Byte*        output,
                      FT_ULong*       output_len,
                      const FT_Byte*  input,
                      FT_ULong        input_len )
  {
    z_stream  stream;
    int       err;


    

    stream.next_in  = (Bytef*)input;
    stream.avail_in = (uInt)input_len;

    stream.next_out  = output;
    stream.avail_out = (uInt)*output_len;

    stream.zalloc = (alloc_func)ft_gzip_alloc;
    stream.zfree  = (free_func) ft_gzip_free;
    stream.opaque = memory;

    err = inflateInit2( &stream, MAX_WBITS );
    if ( err != Z_OK )
      return FT_THROW( Invalid_Argument );

    err = inflate( &stream, Z_FINISH );
    if ( err != Z_STREAM_END )
    {
      inflateEnd( &stream );
      if ( err == Z_OK )
        err = Z_BUF_ERROR;
    }
    else
    {
      *output_len = stream.total_out;

      err = inflateEnd( &stream );
    }

    if ( err == Z_MEM_ERROR )
      return FT_THROW( Out_Of_Memory );

    if ( err == Z_BUF_ERROR )
      return FT_THROW( Array_Too_Large );

    if ( err == Z_DATA_ERROR )
      return FT_THROW( Invalid_Table );

    return FT_Err_Ok;
  }


#else 

  FT_EXPORT_DEF( FT_Error )
  FT_Stream_OpenGzip( FT_Stream  stream,
                      FT_Stream  source )
  {
    FT_UNUSED( stream );
    FT_UNUSED( source );

    return FT_THROW( Unimplemented_Feature );
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Gzip_Uncompress( FT_Memory       memory,
                      FT_Byte*        output,
                      FT_ULong*       output_len,
                      const FT_Byte*  input,
                      FT_ULong        input_len )
  {
    FT_UNUSED( memory );
    FT_UNUSED( output );
    FT_UNUSED( output_len );
    FT_UNUSED( input );
    FT_UNUSED( input_len );

    return FT_THROW( Unimplemented_Feature );
  }

#endif 




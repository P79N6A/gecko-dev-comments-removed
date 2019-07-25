
























#include <ft2build.h>
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_DEBUG_H
#include FT_BZIP2_H
#include FT_CONFIG_STANDARD_LIBRARY_H


#include FT_MODULE_ERRORS_H

#undef __FTERRORS_H__

#undef  FT_ERR_PREFIX
#define FT_ERR_PREFIX  Bzip2_Err_
#define FT_ERR_BASE    FT_Mod_Err_Bzip2

#include FT_ERRORS_H


#ifdef FT_CONFIG_OPTION_USE_BZIP2

#ifdef FT_CONFIG_OPTION_PIC
#error "bzip2 code does not support PIC yet"
#endif

#define BZ_NO_STDIO
#include <bzlib.h>










  


  typedef void *(* alloc_func)(void*, int, int);
  typedef void (* free_func)(void*, void*);

  static void*
  ft_bzip2_alloc( FT_Memory  memory,
                  int        items,
                  int        size )
  {
    FT_ULong    sz = (FT_ULong)size * items;
    FT_Error    error;
    FT_Pointer  p  = NULL;


    (void)FT_ALLOC( p, sz );
    return p;
  }


  static void
  ft_bzip2_free( FT_Memory  memory,
                 void*      address )
  {
    FT_MEM_FREE( address );
  }










#define FT_BZIP2_BUFFER_SIZE  4096

  typedef struct  FT_BZip2FileRec_
  {
    FT_Stream  source;         
    FT_Stream  stream;         
    FT_Memory  memory;         
    bz_stream  bzstream;       

    FT_Byte    input[FT_BZIP2_BUFFER_SIZE];  

    FT_Byte    buffer[FT_BZIP2_BUFFER_SIZE]; 
    FT_ULong   pos;                          
    FT_Byte*   cursor;
    FT_Byte*   limit;

  } FT_BZip2FileRec, *FT_BZip2File;


  
  static FT_Error
  ft_bzip2_check_header( FT_Stream  stream )
  {
    FT_Error  error = Bzip2_Err_Ok;
    FT_Byte   head[4];


    if ( FT_STREAM_SEEK( 0 )       ||
         FT_STREAM_READ( head, 4 ) )
      goto Exit;

    
    
    if ( head[0] != 0x42  ||
         head[1] != 0x5a  ||
         head[2] != 0x68  )  
    {
      error = Bzip2_Err_Invalid_File_Format;
      goto Exit;
    }

  Exit:
    return error;
  }


  static FT_Error
  ft_bzip2_file_init( FT_BZip2File  zip,
                      FT_Stream     stream,
                      FT_Stream     source )
  {
    bz_stream*  bzstream = &zip->bzstream;
    FT_Error    error    = Bzip2_Err_Ok;


    zip->stream = stream;
    zip->source = source;
    zip->memory = stream->memory;

    zip->limit  = zip->buffer + FT_BZIP2_BUFFER_SIZE;
    zip->cursor = zip->limit;
    zip->pos    = 0;

    
    {
      stream = source;

      error = ft_bzip2_check_header( stream );
      if ( error )
        goto Exit;

      if ( FT_STREAM_SEEK( 0 ) )
        goto Exit;
    }

    
    bzstream->bzalloc = (alloc_func)ft_bzip2_alloc;
    bzstream->bzfree  = (free_func) ft_bzip2_free;
    bzstream->opaque  = zip->memory;

    bzstream->avail_in = 0;
    bzstream->next_in  = (char*)zip->buffer;

    if ( BZ2_bzDecompressInit( bzstream, 0, 0 ) != BZ_OK ||
         bzstream->next_in == NULL                       )
      error = Bzip2_Err_Invalid_File_Format;

  Exit:
    return error;
  }


  static void
  ft_bzip2_file_done( FT_BZip2File  zip )
  {
    bz_stream*  bzstream = &zip->bzstream;


    BZ2_bzDecompressEnd( bzstream );

    
    bzstream->bzalloc   = NULL;
    bzstream->bzfree    = NULL;
    bzstream->opaque    = NULL;
    bzstream->next_in   = NULL;
    bzstream->next_out  = NULL;
    bzstream->avail_in  = 0;
    bzstream->avail_out = 0;

    zip->memory = NULL;
    zip->source = NULL;
    zip->stream = NULL;
  }


  static FT_Error
  ft_bzip2_file_reset( FT_BZip2File  zip )
  {
    FT_Stream  stream = zip->source;
    FT_Error   error;


    if ( !FT_STREAM_SEEK( 0 ) )
    {
      bz_stream*  bzstream = &zip->bzstream;


      BZ2_bzDecompressEnd( bzstream );

      bzstream->avail_in  = 0;
      bzstream->next_in   = (char*)zip->input;
      bzstream->avail_out = 0;
      bzstream->next_out  = (char*)zip->buffer;

      zip->limit  = zip->buffer + FT_BZIP2_BUFFER_SIZE;
      zip->cursor = zip->limit;
      zip->pos    = 0;

      BZ2_bzDecompressInit( bzstream, 0, 0 );
    }

    return error;
  }


  static FT_Error
  ft_bzip2_file_fill_input( FT_BZip2File  zip )
  {
    bz_stream*  bzstream = &zip->bzstream;
    FT_Stream   stream    = zip->source;
    FT_ULong    size;


    if ( stream->read )
    {
      size = stream->read( stream, stream->pos, zip->input,
                           FT_BZIP2_BUFFER_SIZE );
      if ( size == 0 )
        return Bzip2_Err_Invalid_Stream_Operation;
    }
    else
    {
      size = stream->size - stream->pos;
      if ( size > FT_BZIP2_BUFFER_SIZE )
        size = FT_BZIP2_BUFFER_SIZE;

      if ( size == 0 )
        return Bzip2_Err_Invalid_Stream_Operation;

      FT_MEM_COPY( zip->input, stream->base + stream->pos, size );
    }
    stream->pos += size;

    bzstream->next_in  = (char*)zip->input;
    bzstream->avail_in = size;

    return Bzip2_Err_Ok;
  }


  static FT_Error
  ft_bzip2_file_fill_output( FT_BZip2File  zip )
  {
    bz_stream*  bzstream = &zip->bzstream;
    FT_Error    error    = Bzip2_Err_Ok;


    zip->cursor         = zip->buffer;
    bzstream->next_out  = (char*)zip->cursor;
    bzstream->avail_out = FT_BZIP2_BUFFER_SIZE;

    while ( bzstream->avail_out > 0 )
    {
      int  err;


      if ( bzstream->avail_in == 0 )
      {
        error = ft_bzip2_file_fill_input( zip );
        if ( error )
          break;
      }

      err = BZ2_bzDecompress( bzstream );

      if ( err == BZ_STREAM_END )
      {
        zip->limit = (FT_Byte*)bzstream->next_out;
        if ( zip->limit == zip->cursor )
          error = Bzip2_Err_Invalid_Stream_Operation;
        break;
      }
      else if ( err != BZ_OK )
      {
        error = Bzip2_Err_Invalid_Stream_Operation;
        break;
      }
    }

    return error;
  }


  
  static FT_Error
  ft_bzip2_file_skip_output( FT_BZip2File  zip,
                             FT_ULong      count )
  {
    FT_Error  error = Bzip2_Err_Ok;
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

      error = ft_bzip2_file_fill_output( zip );
      if ( error )
        break;
    }

    return error;
  }


  static FT_ULong
  ft_bzip2_file_io( FT_BZip2File  zip,
                    FT_ULong      pos,
                    FT_Byte*      buffer,
                    FT_ULong      count )
  {
    FT_ULong  result = 0;
    FT_Error  error;


    
    
    if ( pos < zip->pos )
    {
      error = ft_bzip2_file_reset( zip );
      if ( error )
        goto Exit;
    }

    
    if ( pos > zip->pos )
    {
      error = ft_bzip2_file_skip_output( zip, (FT_ULong)( pos - zip->pos ) );
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

      error = ft_bzip2_file_fill_output( zip );
      if ( error )
        break;
    }

  Exit:
    return result;
  }










  static void
  ft_bzip2_stream_close( FT_Stream  stream )
  {
    FT_BZip2File  zip    = (FT_BZip2File)stream->descriptor.pointer;
    FT_Memory     memory = stream->memory;


    if ( zip )
    {
      
      ft_bzip2_file_done( zip );

      FT_FREE( zip );

      stream->descriptor.pointer = NULL;
    }
  }


  static FT_ULong
  ft_bzip2_stream_io( FT_Stream  stream,
                      FT_ULong   pos,
                      FT_Byte*   buffer,
                      FT_ULong   count )
  {
    FT_BZip2File  zip = (FT_BZip2File)stream->descriptor.pointer;


    return ft_bzip2_file_io( zip, pos, buffer, count );
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Stream_OpenBzip2( FT_Stream  stream,
                       FT_Stream  source )
  {
    FT_Error      error;
    FT_Memory     memory = source->memory;
    FT_BZip2File  zip = NULL;


    



    error = ft_bzip2_check_header( source );
    if ( error )
      goto Exit;

    FT_ZERO( stream );
    stream->memory = memory;

    if ( !FT_QNEW( zip ) )
    {
      error = ft_bzip2_file_init( zip, stream, source );
      if ( error )
      {
        FT_FREE( zip );
        goto Exit;
      }

      stream->descriptor.pointer = zip;
    }

    stream->size  = 0x7FFFFFFFL;  
    stream->pos   = 0;
    stream->base  = 0;
    stream->read  = ft_bzip2_stream_io;
    stream->close = ft_bzip2_stream_close;

  Exit:
    return error;
  }

#else  

  FT_EXPORT_DEF( FT_Error )
  FT_Stream_OpenBzip2( FT_Stream  stream,
                       FT_Stream  source )
  {
    FT_UNUSED( stream );
    FT_UNUSED( source );

    return Bzip2_Err_Unimplemented_Feature;
  }

#endif 




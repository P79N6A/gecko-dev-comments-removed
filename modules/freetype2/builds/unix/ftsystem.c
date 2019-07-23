

















#include <ft2build.h>
  
#include <ftconfig.h>
#include FT_INTERNAL_DEBUG_H
#include FT_SYSTEM_H
#include FT_ERRORS_H
#include FT_TYPES_H
#include FT_INTERNAL_STREAM_H

  
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/mman.h>
#ifndef MAP_FILE
#define MAP_FILE  0x00
#endif

#ifdef MUNMAP_USES_VOIDP
#define MUNMAP_ARG_CAST  void *
#else
#define MUNMAP_ARG_CAST  char *
#endif

#ifdef NEED_MUNMAP_DECL

#ifdef __cplusplus
  extern "C"
#else
  extern
#endif
  int
  munmap( char*  addr,
          int    len );

#define MUNMAP_ARG_CAST  char *

#endif 


#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_CALLBACK_DEF( void* )
  ft_alloc( FT_Memory  memory,
            long       size )
  {
    FT_UNUSED( memory );

    return malloc( size );
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_CALLBACK_DEF( void* )
  ft_realloc( FT_Memory  memory,
              long       cur_size,
              long       new_size,
              void*      block )
  {
    FT_UNUSED( memory );
    FT_UNUSED( cur_size );

    return realloc( block, new_size );
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_CALLBACK_DEF( void )
  ft_free( FT_Memory  memory,
           void*      block )
  {
    FT_UNUSED( memory );

    free( block );
  }


  
  
  
  
  


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_io

  
  
#define STREAM_FILE( stream )  ( (FILE*)stream->descriptor.pointer )


  
  
  
  
  
  
  
  
  
  
  
  FT_CALLBACK_DEF( void )
  ft_close_stream_by_munmap( FT_Stream  stream )
  {
    munmap( (MUNMAP_ARG_CAST)stream->descriptor.pointer, stream->size );

    stream->descriptor.pointer = NULL;
    stream->size               = 0;
    stream->base               = 0;
  }


  
  
  
  
  
  
  
  
  
  
  
  FT_CALLBACK_DEF( void )
  ft_close_stream_by_free( FT_Stream  stream )
  {
    ft_free( NULL, stream->descriptor.pointer );

    stream->descriptor.pointer = NULL;
    stream->size               = 0;
    stream->base               = 0;
  }


  

  FT_BASE_DEF( FT_Error )
  FT_Stream_Open( FT_Stream    stream,
                  const char*  filepathname )
  {
    int          file;
    struct stat  stat_buf;


    if ( !stream )
      return FT_Err_Invalid_Stream_Handle;

    
    file = open( filepathname, O_RDONLY );
    if ( file < 0 )
    {
      FT_ERROR(( "FT_Stream_Open:" ));
      FT_ERROR(( " could not open `%s'\n", filepathname ));
      return FT_Err_Cannot_Open_Resource;
    }

    
    
    
    
    
#ifdef F_SETFD
#ifdef FD_CLOEXEC
    (void)fcntl( file, F_SETFD, FD_CLOEXEC );
#else
    (void)fcntl( file, F_SETFD, 1 );
#endif 
#endif 

    if ( fstat( file, &stat_buf ) < 0 )
    {
      FT_ERROR(( "FT_Stream_Open:" ));
      FT_ERROR(( " could not `fstat' file `%s'\n", filepathname ));
      goto Fail_Map;
    }

    
    
    
    
    
    
    
    
    
    if ( stat_buf.st_size > LONG_MAX )
    {
      FT_ERROR(( "FT_Stream_Open: file is too big" ));
      goto Fail_Map;
    }

    
    stream->size = (unsigned long)stat_buf.st_size;
    stream->pos  = 0;
    stream->base = (unsigned char *)mmap( NULL,
                                          stream->size,
                                          PROT_READ,
                                          MAP_FILE | MAP_PRIVATE,
                                          file,
                                          0 );

    
    if ( (long)stream->base != -1 && stream->base != NULL )
      stream->close = ft_close_stream_by_munmap;
    else
    {
      ssize_t  total_read_count;


      FT_ERROR(( "FT_Stream_Open:" ));
      FT_ERROR(( " could not `mmap' file `%s'\n", filepathname ));

      stream->base = (unsigned char*)ft_alloc( NULL, stream->size );

      if ( !stream->base )
      {
        FT_ERROR(( "FT_Stream_Open:" ));
        FT_ERROR(( " could not `alloc' memory\n" ));
        goto Fail_Map;
      }

      total_read_count = 0;
      do {
        ssize_t  read_count;


        read_count = read( file,
                           stream->base + total_read_count,
                           stream->size - total_read_count );

        if ( read_count <= 0 )
        {
          if ( read_count == -1 && errno == EINTR )
            continue;

          FT_ERROR(( "FT_Stream_Open:" ));
          FT_ERROR(( " error while `read'ing file `%s'\n", filepathname ));
          goto Fail_Read;
        }

        total_read_count += read_count;

      } while ( (unsigned long)total_read_count != stream->size );

      stream->close = ft_close_stream_by_free;
    }

    close( file );

    stream->descriptor.pointer = stream->base;
    stream->pathname.pointer   = (char*)filepathname;

    stream->read = 0;

    FT_TRACE1(( "FT_Stream_Open:" ));
    FT_TRACE1(( " opened `%s' (%d bytes) successfully\n",
                filepathname, stream->size ));

    return FT_Err_Ok;

  Fail_Read:
    ft_free( NULL, stream->base );

  Fail_Map:
    close( file );

    stream->base = NULL;
    stream->size = 0;
    stream->pos  = 0;

    return FT_Err_Cannot_Open_Stream;
  }


#ifdef FT_DEBUG_MEMORY

  extern FT_Int
  ft_mem_debug_init( FT_Memory  memory );

  extern void
  ft_mem_debug_done( FT_Memory  memory );

#endif


  

  FT_BASE_DEF( FT_Memory )
  FT_New_Memory( void )
  {
    FT_Memory  memory;


    memory = (FT_Memory)malloc( sizeof ( *memory ) );
    if ( memory )
    {
      memory->user    = 0;
      memory->alloc   = ft_alloc;
      memory->realloc = ft_realloc;
      memory->free    = ft_free;
#ifdef FT_DEBUG_MEMORY
      ft_mem_debug_init( memory );
#endif
    }

    return memory;
  }


  

  FT_BASE_DEF( void )
  FT_Done_Memory( FT_Memory  memory )
  {
#ifdef FT_DEBUG_MEMORY
    ft_mem_debug_done( memory );
#endif
    memory->free( memory, memory );
  }




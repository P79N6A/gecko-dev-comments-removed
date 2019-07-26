





































#include "cf2ft.h"
#include FT_INTERNAL_DEBUG_H

#include "cf2glue.h"
#include "cf2arrst.h"

#include "cf2error.h"


  






  FT_LOCAL_DEF( void )
  cf2_arrstack_init( CF2_ArrStack  arrstack,
                     FT_Memory     memory,
                     FT_Error*     error,
                     size_t        sizeItem )
  {
    FT_ASSERT( arrstack != NULL );

    
    arrstack->memory    = memory;
    arrstack->error     = error;
    arrstack->sizeItem  = sizeItem;
    arrstack->allocated = 0;
    arrstack->chunk     = 10;    
    arrstack->count     = 0;
    arrstack->totalSize = 0;
    arrstack->ptr       = NULL;
  }


  FT_LOCAL_DEF( void )
  cf2_arrstack_finalize( CF2_ArrStack  arrstack )
  {
    FT_Memory  memory = arrstack->memory;     


    FT_ASSERT( arrstack != NULL );

    arrstack->allocated = 0;
    arrstack->count     = 0;
    arrstack->totalSize = 0;

    
    FT_FREE( arrstack->ptr );
  }


  
  
  static FT_Bool
  cf2_arrstack_setNumElements( CF2_ArrStack  arrstack,
                               size_t        numElements )
  {
    FT_ASSERT( arrstack != NULL );

    {
      FT_Error   error  = FT_Err_Ok;        
      FT_Memory  memory = arrstack->memory; 

      FT_Long  newSize = (FT_Long)( numElements * arrstack->sizeItem );


      if ( numElements > LONG_MAX / arrstack->sizeItem )
        goto exit;


      FT_ASSERT( newSize > 0 );   

      if ( !FT_REALLOC( arrstack->ptr, arrstack->totalSize, newSize ) )
      {
        arrstack->allocated = numElements;
        arrstack->totalSize = newSize;

        if ( arrstack->count > numElements )
        {
          
          CF2_SET_ERROR( arrstack->error, Stack_Overflow );
          arrstack->count = numElements;
          return FALSE;
        }

        return TRUE;     
      }
    }

  exit:
    
    CF2_SET_ERROR( arrstack->error, Out_Of_Memory );

    return FALSE;
  }


  
  FT_LOCAL_DEF( void )
  cf2_arrstack_setCount( CF2_ArrStack  arrstack,
                         size_t        numElements )
  {
    FT_ASSERT( arrstack != NULL );

    if ( numElements > arrstack->allocated )
    {
      
      if ( !cf2_arrstack_setNumElements( arrstack, numElements ) )
        return;
    }

    arrstack->count = numElements;
  }


  
  FT_LOCAL_DEF( void )
  cf2_arrstack_clear( CF2_ArrStack  arrstack )
  {
    FT_ASSERT( arrstack != NULL );

    arrstack->count = 0;
  }


  
  FT_LOCAL_DEF( size_t )
  cf2_arrstack_size( const CF2_ArrStack  arrstack )
  {
    FT_ASSERT( arrstack != NULL );

    return arrstack->count;
  }


  FT_LOCAL_DEF( void* )
  cf2_arrstack_getBuffer( const CF2_ArrStack  arrstack )
  {
    FT_ASSERT( arrstack != NULL );

    return arrstack->ptr;
  }


  
  FT_LOCAL_DEF( void* )
  cf2_arrstack_getPointer( const CF2_ArrStack  arrstack,
                           size_t              idx )
  {
    void*  newPtr;


    FT_ASSERT( arrstack != NULL );

    if ( idx >= arrstack->count )
    {
      
      CF2_SET_ERROR( arrstack->error, Stack_Overflow );
      idx = 0;    
    }

    newPtr = (FT_Byte*)arrstack->ptr + idx * arrstack->sizeItem;

    return newPtr;
  }


  
  
  
  FT_LOCAL_DEF( void )
  cf2_arrstack_push( CF2_ArrStack  arrstack,
                     const void*   ptr )
  {
    FT_ASSERT( arrstack != NULL );

    if ( arrstack->count == arrstack->allocated )
    {
      
      if ( !cf2_arrstack_setNumElements(
             arrstack, arrstack->allocated + arrstack->chunk ) )
      {
        
        return;
      }
    }

    FT_ASSERT( ptr != NULL );

    {
      size_t  offset = arrstack->count * arrstack->sizeItem;
      void*   newPtr = (FT_Byte*)arrstack->ptr + offset;


      FT_MEM_COPY( newPtr, ptr, arrstack->sizeItem );
      arrstack->count += 1;
    }
  }




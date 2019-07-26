





































#ifndef __CF2ARRST_H__
#define __CF2ARRST_H__


#include "cf2error.h"


FT_BEGIN_HEADER


  
  
  typedef struct  CF2_ArrStackRec_
  {
    FT_Memory  memory;
    FT_Error*  error;

    size_t  sizeItem;       
    size_t  allocated;      
    size_t  chunk;          
    size_t  count;          
    size_t  totalSize;      

    void*  ptr;             

  } CF2_ArrStackRec, *CF2_ArrStack;


  FT_LOCAL( void )
  cf2_arrstack_init( CF2_ArrStack  arrstack,
                     FT_Memory     memory,
                     FT_Error*     error,
                     size_t        sizeItem );
  FT_LOCAL( void )
  cf2_arrstack_finalize( CF2_ArrStack  arrstack );

  FT_LOCAL( void )
  cf2_arrstack_setCount( CF2_ArrStack  arrstack,
                         size_t        numElements );
  FT_LOCAL( void )
  cf2_arrstack_clear( CF2_ArrStack  arrstack );
  FT_LOCAL( size_t )
  cf2_arrstack_size( const CF2_ArrStack  arrstack );

  FT_LOCAL( void* )
  cf2_arrstack_getBuffer( const CF2_ArrStack  arrstack );
  FT_LOCAL( void* )
  cf2_arrstack_getPointer( const CF2_ArrStack  arrstack,
                           size_t              idx );

  FT_LOCAL( void )
  cf2_arrstack_push( CF2_ArrStack  arrstack,
                     const void*   ptr );


FT_END_HEADER


#endif 




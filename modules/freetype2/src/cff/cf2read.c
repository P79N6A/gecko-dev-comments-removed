





































#include "cf2ft.h"
#include FT_INTERNAL_DEBUG_H

#include "cf2glue.h"

#include "cf2error.h"


  
  
#define CF2_IO_FAIL  0


#if CF2_IO_FAIL

  
  static int
  randomError2( void )
  {
    
    return (double)rand() / RAND_MAX < .00;
  }

  
  static CF2_Int
  randomValue()
  {
    return (double)rand() / RAND_MAX < .00 ? rand() : 0;
  }

#endif 


  
  
  
  
  

  
  FT_LOCAL_DEF( CF2_Int )
  cf2_buf_readByte( CF2_Buffer  buf )
  {
    if ( buf->ptr < buf->end )
    {
#if CF2_IO_FAIL
      if ( randomError2() )
      {
        CF2_SET_ERROR( buf->error, Invalid_Stream_Operation );
        return 0;
      }

      return *(buf->ptr)++ + randomValue();
#else
      return *(buf->ptr)++;
#endif
    }
    else
    {
      CF2_SET_ERROR( buf->error, Invalid_Stream_Operation );
      return 0;
    }
  }


  
  FT_LOCAL_DEF( FT_Bool )
  cf2_buf_isEnd( CF2_Buffer  buf )
  {
    return (FT_Bool)( buf->ptr >= buf->end );
  }




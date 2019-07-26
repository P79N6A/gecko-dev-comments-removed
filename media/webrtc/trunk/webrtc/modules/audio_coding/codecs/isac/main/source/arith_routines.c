









#include "arith_routines.h"
#include "settings.h"






int WebRtcIsac_EncTerminate(Bitstr *streamdata) 
{
  uint8_t *stream_ptr;


  
  stream_ptr = streamdata->stream + streamdata->stream_index;

  
  if ( streamdata->W_upper > 0x01FFFFFF )
  {
    streamdata->streamval += 0x01000000;
    
    if (streamdata->streamval < 0x01000000)
    {
      
      while ( !(++(*--stream_ptr)) );
      
      stream_ptr = streamdata->stream + streamdata->stream_index;
    }
    
    *stream_ptr++ = (uint8_t) (streamdata->streamval >> 24);
  }
  else
  {
    streamdata->streamval += 0x00010000;
    
    if (streamdata->streamval < 0x00010000)
    {
      
      while ( !(++(*--stream_ptr)) );
      
      stream_ptr = streamdata->stream + streamdata->stream_index;
    }
    
    *stream_ptr++ = (uint8_t) (streamdata->streamval >> 24);
    *stream_ptr++ = (uint8_t) ((streamdata->streamval >> 16) & 0x00FF);
  }

  
  return (int)(stream_ptr - streamdata->stream);
}

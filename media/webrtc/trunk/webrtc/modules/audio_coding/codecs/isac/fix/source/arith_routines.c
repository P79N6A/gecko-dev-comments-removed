

















#include "arith_routins.h"













int16_t WebRtcIsacfix_EncTerminate(Bitstr_enc *streamData)
{
  uint16_t *streamPtr;
  uint16_t negCarry;

  
  streamPtr = streamData->stream + streamData->stream_index;

  
  if ( streamData->W_upper > 0x01FFFFFF )
  {
    streamData->streamval += 0x01000000;

    
    if (streamData->streamval < 0x01000000)
    {
      
      if (streamData->full == 0) {
        
        negCarry = *streamPtr;
        negCarry += 0x0100;
        *streamPtr = negCarry;

        
        while (!(negCarry))
        {
          negCarry = *--streamPtr;
          negCarry++;
          *streamPtr = negCarry;
        }
      } else {
        


        while ( !(++(*--streamPtr)) );
      }

      
      streamPtr = streamData->stream + streamData->stream_index;
    }
    
    if (streamData->full == 0) {
      *streamPtr++ += (uint16_t) WEBRTC_SPL_RSHIFT_W32(streamData->streamval, 24);
      streamData->full = 1;
    } else {
      *streamPtr = (uint16_t) WEBRTC_SPL_LSHIFT_W32(
          WEBRTC_SPL_RSHIFT_W32(streamData->streamval, 24), 8);
      streamData->full = 0;
    }
  }
  else
  {
    streamData->streamval += 0x00010000;

    
    if (streamData->streamval < 0x00010000)
    {
      
      if (streamData->full == 0) {
        
        negCarry = *streamPtr;
        negCarry += 0x0100;
        *streamPtr = negCarry;

        
        while (!(negCarry))
        {
          negCarry = *--streamPtr;
          negCarry++;
          *streamPtr = negCarry;
        }
      } else {
        
        while ( !(++(*--streamPtr)) );
      }

      
      streamPtr = streamData->stream + streamData->stream_index;
    }
    
    if (streamData->full) {
      *streamPtr++ = (uint16_t) WEBRTC_SPL_RSHIFT_W32(streamData->streamval, 16);
    } else {
      *streamPtr++ |= (uint16_t) WEBRTC_SPL_RSHIFT_W32(streamData->streamval, 24);
      *streamPtr = (uint16_t) WEBRTC_SPL_RSHIFT_W32(streamData->streamval, 8)
          & 0xFF00;
    }
  }

  
  return (((streamPtr - streamData->stream)<<1) + !(streamData->full));
}

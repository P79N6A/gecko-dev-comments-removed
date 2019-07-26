

















#include "arith_routins.h"













WebRtc_Word16 WebRtcIsacfix_EncTerminate(Bitstr_enc *streamData)
{
  WebRtc_UWord16 *streamPtr;
  WebRtc_UWord16 negCarry;

  
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
      *streamPtr++ += (WebRtc_UWord16) WEBRTC_SPL_RSHIFT_W32(streamData->streamval, 24);
      streamData->full = 1;
    } else {
      *streamPtr = (WebRtc_UWord16) WEBRTC_SPL_LSHIFT_W32(
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
      *streamPtr++ = (WebRtc_UWord16) WEBRTC_SPL_RSHIFT_W32(streamData->streamval, 16);
    } else {
      *streamPtr++ |= (WebRtc_UWord16) WEBRTC_SPL_RSHIFT_W32(streamData->streamval, 24);
      *streamPtr = (WebRtc_UWord16) WEBRTC_SPL_RSHIFT_W32(streamData->streamval, 8)
          & 0xFF00;
    }
  }

  
  return (((streamPtr - streamData->stream)<<1) + !(streamData->full));
}

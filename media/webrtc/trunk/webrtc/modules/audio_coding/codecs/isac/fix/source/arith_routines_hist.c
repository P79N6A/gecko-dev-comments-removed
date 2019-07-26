
















#include "arith_routins.h"
















int WebRtcIsacfix_EncHistMulti(Bitstr_enc *streamData,
                              const WebRtc_Word16 *data,
                              const WebRtc_UWord16 **cdf,
                              const WebRtc_Word16 lenData)
{
  WebRtc_UWord32 W_lower;
  WebRtc_UWord32 W_upper;
  WebRtc_UWord32 W_upper_LSB;
  WebRtc_UWord32 W_upper_MSB;
  WebRtc_UWord16 *streamPtr;
  WebRtc_UWord16 negCarry;
  WebRtc_UWord16 *maxStreamPtr;
  WebRtc_UWord16 *streamPtrCarry;
  WebRtc_UWord32 cdfLo;
  WebRtc_UWord32 cdfHi;
  int k;


  

  streamPtr = streamData->stream + streamData->stream_index;
  maxStreamPtr = streamData->stream + STREAM_MAXW16_60MS - 1;

  W_upper = streamData->W_upper;

  for (k = lenData; k > 0; k--)
  {
    
    cdfLo = (WebRtc_UWord32) *(*cdf + (WebRtc_UWord32)*data);
    cdfHi = (WebRtc_UWord32) *(*cdf++ + (WebRtc_UWord32)*data++ + 1);

    
    W_upper_LSB = W_upper & 0x0000FFFF;
    W_upper_MSB = WEBRTC_SPL_RSHIFT_W32(W_upper, 16);
    W_lower = WEBRTC_SPL_UMUL(W_upper_MSB, cdfLo);
    W_lower += WEBRTC_SPL_UMUL_RSFT16(W_upper_LSB, cdfLo);
    W_upper = WEBRTC_SPL_UMUL(W_upper_MSB, cdfHi);
    W_upper += WEBRTC_SPL_UMUL_RSFT16(W_upper_LSB, cdfHi);

    
    W_upper -= ++W_lower;

    
    streamData->streamval += W_lower;

    
    if (streamData->streamval < W_lower)
    {
      
      streamPtrCarry = streamPtr;
      if (streamData->full == 0) {
        negCarry = *streamPtrCarry;
        negCarry += 0x0100;
        *streamPtrCarry = negCarry;
        while (!(negCarry))
        {
          negCarry = *--streamPtrCarry;
          negCarry++;
          *streamPtrCarry = negCarry;
        }
      } else {
        while ( !(++(*--streamPtrCarry)) );
      }
    }

    

    while ( !(W_upper & 0xFF000000) )
    {
      W_upper = WEBRTC_SPL_LSHIFT_W32(W_upper, 8);
      if (streamData->full == 0) {
        *streamPtr++ += (WebRtc_UWord16) WEBRTC_SPL_RSHIFT_W32(streamData->streamval, 24);
        streamData->full = 1;
      } else {
        *streamPtr = (WebRtc_UWord16) WEBRTC_SPL_LSHIFT_W32(
            WEBRTC_SPL_RSHIFT_W32(streamData->streamval, 24), 8);
        streamData->full = 0;
      }

      if( streamPtr > maxStreamPtr ) {
        return -ISAC_DISALLOWED_BITSTREAM_LENGTH;
      }
      streamData->streamval = WEBRTC_SPL_LSHIFT_W32(streamData->streamval, 8);
    }
  }

  
  streamData->stream_index = streamPtr - streamData->stream;
  streamData->W_upper = W_upper;

  return 0;
}





















WebRtc_Word16 WebRtcIsacfix_DecHistBisectMulti(WebRtc_Word16 *data,
                                              Bitstr_dec *streamData,
                                              const WebRtc_UWord16 **cdf,
                                              const WebRtc_UWord16 *cdfSize,
                                              const WebRtc_Word16 lenData)
{
  WebRtc_UWord32    W_lower = 0;
  WebRtc_UWord32    W_upper;
  WebRtc_UWord32    W_tmp;
  WebRtc_UWord32    W_upper_LSB;
  WebRtc_UWord32    W_upper_MSB;
  WebRtc_UWord32    streamval;
  const WebRtc_UWord16 *streamPtr;
  const WebRtc_UWord16 *cdfPtr;
  WebRtc_Word16     sizeTmp;
  int             k;


  streamPtr = streamData->stream + streamData->stream_index;
  W_upper = streamData->W_upper;

  
  if (W_upper == 0) {
    return -2;
  }

  
  if (streamData->stream_index == 0)
  {
    
    streamval = WEBRTC_SPL_LSHIFT_W32((WebRtc_UWord32)*streamPtr++, 16);
    streamval |= *streamPtr++;
  } else {
    streamval = streamData->streamval;
  }

  for (k = lenData; k > 0; k--)
  {
    
    W_upper_LSB = W_upper & 0x0000FFFF;
    W_upper_MSB = WEBRTC_SPL_RSHIFT_W32(W_upper, 16);

    
    sizeTmp = WEBRTC_SPL_RSHIFT_W16(*cdfSize++, 1);
    cdfPtr = *cdf + (sizeTmp - 1);

    
    for ( ;; )
    {
      W_tmp = WEBRTC_SPL_UMUL_32_16(W_upper_MSB, *cdfPtr);
      W_tmp += WEBRTC_SPL_UMUL_32_16_RSFT16(W_upper_LSB, *cdfPtr);
      sizeTmp = WEBRTC_SPL_RSHIFT_W16(sizeTmp, 1);
      if (sizeTmp == 0) {
        break;
      }

      if (streamval > W_tmp)
      {
        W_lower = W_tmp;
        cdfPtr += sizeTmp;
      } else {
        W_upper = W_tmp;
        cdfPtr -= sizeTmp;
      }
    }
    if (streamval > W_tmp)
    {
      W_lower = W_tmp;
      *data++ = cdfPtr - *cdf++;
    } else {
      W_upper = W_tmp;
      *data++ = cdfPtr - *cdf++ - 1;
    }

    
    W_upper -= ++W_lower;

    
    streamval -= W_lower;

    
    
    while ( !(W_upper & 0xFF000000) )
    {
      
      if (streamData->full == 0) {
        streamval = WEBRTC_SPL_LSHIFT_W32(streamval, 8) |
            (*streamPtr++ & 0x00FF);
        streamData->full = 1;
      } else {
        streamval = WEBRTC_SPL_LSHIFT_W32(streamval, 8) |
            WEBRTC_SPL_RSHIFT_W16(*streamPtr, 8);
        streamData->full = 0;
      }
      W_upper = WEBRTC_SPL_LSHIFT_W32(W_upper, 8);
    }


    
    if (W_upper == 0) {
      return -2;
    }

  }

  streamData->stream_index = streamPtr - streamData->stream;
  streamData->W_upper = W_upper;
  streamData->streamval = streamval;

  if ( W_upper > 0x01FFFFFF ) {
    return (streamData->stream_index*2 - 3 + !streamData->full);
  } else {
    return (streamData->stream_index*2 - 2 + !streamData->full);
  }
}






















WebRtc_Word16 WebRtcIsacfix_DecHistOneStepMulti(WebRtc_Word16 *data,
                                               Bitstr_dec *streamData,
                                               const WebRtc_UWord16 **cdf,
                                               const WebRtc_UWord16 *initIndex,
                                               const WebRtc_Word16 lenData)
{
  WebRtc_UWord32    W_lower;
  WebRtc_UWord32    W_upper;
  WebRtc_UWord32    W_tmp;
  WebRtc_UWord32    W_upper_LSB;
  WebRtc_UWord32    W_upper_MSB;
  WebRtc_UWord32    streamval;
  const WebRtc_UWord16 *streamPtr;
  const WebRtc_UWord16 *cdfPtr;
  int             k;


  streamPtr = streamData->stream + streamData->stream_index;
  W_upper = streamData->W_upper;
  
  if (W_upper == 0) {
    return -2;
  }

  
  if (streamData->stream_index == 0)
  {
    
    streamval = WEBRTC_SPL_LSHIFT_U32(*streamPtr++, 16);
    streamval |= *streamPtr++;
  } else {
    streamval = streamData->streamval;
  }

  for (k = lenData; k > 0; k--)
  {
    
    W_upper_LSB = W_upper & 0x0000FFFF;
    W_upper_MSB = WEBRTC_SPL_RSHIFT_U32(W_upper, 16);

    
    cdfPtr = *cdf + (*initIndex++);
    W_tmp = WEBRTC_SPL_UMUL_32_16(W_upper_MSB, *cdfPtr);
    W_tmp += WEBRTC_SPL_UMUL_32_16_RSFT16(W_upper_LSB, *cdfPtr);

    if (streamval > W_tmp)
    {
      for ( ;; )
      {
        W_lower = W_tmp;

        
        if (cdfPtr[0] == 65535) {
          return -3;
        }

        W_tmp = WEBRTC_SPL_UMUL_32_16(W_upper_MSB, *++cdfPtr);
        W_tmp += WEBRTC_SPL_UMUL_32_16_RSFT16(W_upper_LSB, *cdfPtr);

        if (streamval <= W_tmp) {
          break;
        }
      }
      W_upper = W_tmp;
      *data++ = cdfPtr - *cdf++ - 1;
    } else {
      for ( ;; )
      {
        W_upper = W_tmp;
        --cdfPtr;

        
        if (cdfPtr < *cdf) {
          return -3;
        }

        W_tmp = WEBRTC_SPL_UMUL_32_16(W_upper_MSB, *cdfPtr);
        W_tmp += WEBRTC_SPL_UMUL_32_16_RSFT16(W_upper_LSB, *cdfPtr);

        if (streamval > W_tmp) {
          break;
        }
      }
      W_lower = W_tmp;
      *data++ = cdfPtr - *cdf++;
    }

    
    W_upper -= ++W_lower;

    
    streamval -= W_lower;

    
    
    while ( !(W_upper & 0xFF000000) )
    {
      
      if (streamData->full == 0) {
        streamval = WEBRTC_SPL_LSHIFT_W32(streamval, 8) | (*streamPtr++ & 0x00FF);
        streamData->full = 1;
      } else {
        streamval = WEBRTC_SPL_LSHIFT_W32(streamval, 8) | (*streamPtr >> 8);
        streamData->full = 0;
      }
      W_upper = WEBRTC_SPL_LSHIFT_W32(W_upper, 8);
    }
  }

  streamData->stream_index = streamPtr - streamData->stream;
  streamData->W_upper = W_upper;
  streamData->streamval = streamval;

  
  if ( W_upper > 0x01FFFFFF ) {
    return (streamData->stream_index*2 - 3 + !streamData->full);
  } else {
    return (streamData->stream_index*2 - 2 + !streamData->full);
  }
}

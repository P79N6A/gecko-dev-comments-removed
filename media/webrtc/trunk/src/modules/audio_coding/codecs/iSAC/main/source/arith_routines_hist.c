









#include "settings.h"
#include "arith_routines.h"





void WebRtcIsac_EncHistMulti(Bitstr *streamdata, 
                             const int *data,  
                             const WebRtc_UWord16 **cdf, 
                             const int N)   
{
  WebRtc_UWord32 W_lower, W_upper;
  WebRtc_UWord32 W_upper_LSB, W_upper_MSB;
  WebRtc_UWord8 *stream_ptr;
  WebRtc_UWord8 *stream_ptr_carry;
  WebRtc_UWord32 cdf_lo, cdf_hi;
  int k;


  
  stream_ptr = streamdata->stream + streamdata->stream_index;
  W_upper = streamdata->W_upper;

  for (k=N; k>0; k--)
  {
    
    cdf_lo = (WebRtc_UWord32) *(*cdf + *data);
    cdf_hi = (WebRtc_UWord32) *(*cdf++ + *data++ + 1);

    
    W_upper_LSB = W_upper & 0x0000FFFF;
    W_upper_MSB = W_upper >> 16;
    W_lower = W_upper_MSB * cdf_lo;
    W_lower += (W_upper_LSB * cdf_lo) >> 16;
    W_upper = W_upper_MSB * cdf_hi;
    W_upper += (W_upper_LSB * cdf_hi) >> 16;

    
    W_upper -= ++W_lower;

    
    streamdata->streamval += W_lower;

    
    if (streamdata->streamval < W_lower)
    {
      
      stream_ptr_carry = stream_ptr;
      while (!(++(*--stream_ptr_carry)));
    }

    
    while ( !(W_upper & 0xFF000000) )      
    {
      W_upper <<= 8;
      *stream_ptr++ = (WebRtc_UWord8) (streamdata->streamval >> 24);
      streamdata->streamval <<= 8;
    }
  }

  
  streamdata->stream_index = (int)(stream_ptr - streamdata->stream);
  streamdata->W_upper = W_upper;

  return;
}







int WebRtcIsac_DecHistBisectMulti(int *data,     
                                  Bitstr *streamdata,   
                                  const WebRtc_UWord16 **cdf,  
                                  const WebRtc_UWord16 *cdf_size, 
                                  const int N)    
{
  WebRtc_UWord32    W_lower, W_upper;
  WebRtc_UWord32    W_tmp;
  WebRtc_UWord32    W_upper_LSB, W_upper_MSB;
  WebRtc_UWord32    streamval;
  const   WebRtc_UWord8 *stream_ptr;
  const   WebRtc_UWord16 *cdf_ptr;
  int     size_tmp;
  int     k;

  W_lower = 0; 
  stream_ptr = streamdata->stream + streamdata->stream_index;
  W_upper = streamdata->W_upper;
  if (W_upper == 0)
    
    return -2;

  if (streamdata->stream_index == 0)   
  {
    
    streamval = *stream_ptr << 24;
    streamval |= *++stream_ptr << 16;
    streamval |= *++stream_ptr << 8;
    streamval |= *++stream_ptr;
  } else {
    streamval = streamdata->streamval;
  }

  for (k=N; k>0; k--)
  {
    
    W_upper_LSB = W_upper & 0x0000FFFF;
    W_upper_MSB = W_upper >> 16;

    
    size_tmp = *cdf_size++ >> 1;
    cdf_ptr = *cdf + (size_tmp - 1);

    
    for ( ;; )
    {
      W_tmp = W_upper_MSB * *cdf_ptr;
      W_tmp += (W_upper_LSB * *cdf_ptr) >> 16;
      size_tmp >>= 1;
      if (size_tmp == 0) break;
      if (streamval > W_tmp)
      {
        W_lower = W_tmp;
        cdf_ptr += size_tmp;
      } else {
        W_upper = W_tmp;
        cdf_ptr -= size_tmp;
      }
    }
    if (streamval > W_tmp)
    {
      W_lower = W_tmp;
      *data++ = (int)(cdf_ptr - *cdf++);
    } else {
      W_upper = W_tmp;
      *data++ = (int)(cdf_ptr - *cdf++ - 1);
    }

    
    W_upper -= ++W_lower;

    
    streamval -= W_lower;

    
    while ( !(W_upper & 0xFF000000) )    
    {
      
      streamval = (streamval << 8) | *++stream_ptr;
      W_upper <<= 8;
    }

    if (W_upper == 0)
      
      return -2;


  }

  streamdata->stream_index = (int)(stream_ptr - streamdata->stream);
  streamdata->W_upper = W_upper;
  streamdata->streamval = streamval;


  
  if ( W_upper > 0x01FFFFFF )
    return streamdata->stream_index - 2;
  else
    return streamdata->stream_index - 1;
}








int WebRtcIsac_DecHistOneStepMulti(int *data,        
                                   Bitstr *streamdata,      
                                   const WebRtc_UWord16 **cdf,   
                                   const WebRtc_UWord16 *init_index, 
                                   const int N)     
{
  WebRtc_UWord32    W_lower, W_upper;
  WebRtc_UWord32    W_tmp;
  WebRtc_UWord32    W_upper_LSB, W_upper_MSB;
  WebRtc_UWord32    streamval;
  const   WebRtc_UWord8 *stream_ptr;
  const   WebRtc_UWord16 *cdf_ptr;
  int     k;


  stream_ptr = streamdata->stream + streamdata->stream_index;
  W_upper = streamdata->W_upper;
  if (W_upper == 0)
    
    return -2;

  if (streamdata->stream_index == 0)   
  {
    
    streamval = *stream_ptr << 24;
    streamval |= *++stream_ptr << 16;
    streamval |= *++stream_ptr << 8;
    streamval |= *++stream_ptr;
  } else {
    streamval = streamdata->streamval;
  }


  for (k=N; k>0; k--)
  {
    
    W_upper_LSB = W_upper & 0x0000FFFF;
    W_upper_MSB = W_upper >> 16;

    
    cdf_ptr = *cdf + (*init_index++);
    W_tmp = W_upper_MSB * *cdf_ptr;
    W_tmp += (W_upper_LSB * *cdf_ptr) >> 16;
    if (streamval > W_tmp)
    {
      for ( ;; )
      {
        W_lower = W_tmp;
        if (cdf_ptr[0]==65535)
          
          return -3;
        W_tmp = W_upper_MSB * *++cdf_ptr;
        W_tmp += (W_upper_LSB * *cdf_ptr) >> 16;
        if (streamval <= W_tmp) break;
      }
      W_upper = W_tmp;
      *data++ = (int)(cdf_ptr - *cdf++ - 1);
    } else {
      for ( ;; )
      {
        W_upper = W_tmp;
        --cdf_ptr;
        if (cdf_ptr<*cdf) {
          
          return -3;
        }
        W_tmp = W_upper_MSB * *cdf_ptr;
        W_tmp += (W_upper_LSB * *cdf_ptr) >> 16;
        if (streamval > W_tmp) break;
      }
      W_lower = W_tmp;
      *data++ = (int)(cdf_ptr - *cdf++);
    }

    
    W_upper -= ++W_lower;
    
    streamval -= W_lower;

    
    while ( !(W_upper & 0xFF000000) )    
    {
      
      streamval = (streamval << 8) | *++stream_ptr;
      W_upper <<= 8;
    }
  }

  streamdata->stream_index = (int)(stream_ptr - streamdata->stream);
  streamdata->W_upper = W_upper;
  streamdata->streamval = streamval;


  
  if ( W_upper > 0x01FFFFFF )
    return streamdata->stream_index - 2;
  else
    return streamdata->stream_index - 1;
}

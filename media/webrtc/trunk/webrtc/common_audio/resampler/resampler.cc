














#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "webrtc/common_audio/resampler/include/resampler.h"
#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"




#if defined(WEBRTC_ANDROID) || defined(WEBRTC_GONK)
#define RESAMPLER_QUALITY 2
#else
#define RESAMPLER_QUALITY 3
#endif

namespace webrtc
{

Resampler::Resampler() : state_(NULL), type_(kResamplerSynchronous)
{
  
}

Resampler::Resampler(int in_freq, int out_freq, ResamplerType type) :
  state_(NULL) 
{
  Reset(in_freq, out_freq, type);
}

Resampler::~Resampler()
{
  if (state_)
  {
    speex_resampler_destroy(state_);
  }
}

int Resampler::ResetIfNeeded(int in_freq, int out_freq, ResamplerType type)
{
  if (!state_ || type != type_ ||
      in_freq != in_freq_ || out_freq != out_freq_)
  {
    
    
    
    
    return Reset(in_freq, out_freq, type);
  } else {
    return 0;
  }
}

int Resampler::Reset(int in_freq, int out_freq, ResamplerType type)
{
  uint32_t channels = (type == kResamplerSynchronousStereo ||
                       type == kResamplerFixedSynchronousStereo) ? 2 : 1;

  if (state_)
  {
    speex_resampler_destroy(state_);
    state_ = NULL;
  }
  type_ = type;
  channels_ = channels;
  in_freq_ = in_freq;
  out_freq_ = out_freq;

  
  if (in_freq != out_freq || !IsFixedRate())
  {
    state_ = speex_resampler_init(channels, in_freq, out_freq, RESAMPLER_QUALITY, NULL);
    if (!state_)
    {
      return -1;
    }
  }
  return 0;
}



int Resampler::Push(const int16_t* samples_in, int length_in,
                    int16_t* samples_out, int max_len, int &out_len)
{
  if (max_len < length_in)
  {
    return -1;
  }
  if (!state_)
  {
    if (!IsFixedRate() || in_freq_ != out_freq_)
    {
      
      
      return -1;
    }

    
    
    
    
    memcpy(samples_out, samples_in, length_in*sizeof(*samples_in));
    out_len = length_in;
    return 0;
  }
  assert(channels_ == 1 || channels_ == 2);
  spx_uint32_t len = length_in = (length_in >> (channels_ - 1));
  spx_uint32_t out = (spx_uint32_t) (max_len >> (channels_ - 1));
  if ((speex_resampler_process_interleaved_int(state_, samples_in, &len,
                             samples_out, &out) != RESAMPLER_ERR_SUCCESS) ||
      len != (spx_uint32_t) length_in)
  {
    return -1;
  }
  out_len = (int) (channels_ * out);
  return 0;
}

}  
















#ifndef WEBRTC_RESAMPLER_RESAMPLER_H_
#define WEBRTC_RESAMPLER_RESAMPLER_H_

#include "webrtc/typedefs.h"
#include <speex/speex_resampler.h>

namespace webrtc
{

#define FIXED_RATE_RESAMPLER 0x10
enum ResamplerType
{
    kResamplerSynchronous            = 0x00,
    kResamplerSynchronousStereo      = 0x01,
    kResamplerFixedSynchronous       = 0x00 | FIXED_RATE_RESAMPLER,
    kResamplerFixedSynchronousStereo = 0x01 | FIXED_RATE_RESAMPLER,
};

class Resampler
{
public:
    Resampler();
    
    Resampler(int in_freq, int out_freq, ResamplerType type);
    ~Resampler();

    
    int Reset(int in_freq, int out_freq, ResamplerType type);

    
    int ResetIfNeeded(int in_freq, int out_freq, ResamplerType type);

    
    int Push(const int16_t* samples_in, int length_in,
             int16_t* samples_out, int max_len, int &out_len);

private:
    bool IsFixedRate() { return !!(type_ & FIXED_RATE_RESAMPLER); }

    SpeexResamplerState* state_;

    int in_freq_;
    int out_freq_;
    int channels_;
    ResamplerType type_;
};

}  

#endif 

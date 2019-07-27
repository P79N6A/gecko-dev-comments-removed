














#ifndef WEBRTC_RESAMPLER_RESAMPLER_H_
#define WEBRTC_RESAMPLER_RESAMPLER_H_

#include "webrtc/typedefs.h"

namespace webrtc
{



enum ResamplerType
{
    
    

    kResamplerSynchronous = 0x10,
    kResamplerAsynchronous = 0x11,
    kResamplerSynchronousStereo = 0x20,
    kResamplerAsynchronousStereo = 0x21,
    kResamplerInvalid = 0xff
};


enum ResamplerMode
{
    kResamplerMode1To1,
    kResamplerMode1To2,
    kResamplerMode1To3,
    kResamplerMode1To4,
    kResamplerMode1To6,
    kResamplerMode1To12,
    kResamplerMode2To3,
    kResamplerMode2To11,
    kResamplerMode4To11,
    kResamplerMode8To11,
    kResamplerMode11To16,
    kResamplerMode11To32,
    kResamplerMode2To1,
    kResamplerMode3To1,
    kResamplerMode4To1,
    kResamplerMode6To1,
    kResamplerMode12To1,
    kResamplerMode3To2,
    kResamplerMode11To2,
    kResamplerMode11To4,
    kResamplerMode11To8
};

class Resampler
{

public:
    Resampler();
    
    Resampler(int inFreq, int outFreq, ResamplerType type);
    ~Resampler();

    
    int Reset(int inFreq, int outFreq, ResamplerType type);

    
    int ResetIfNeeded(int inFreq, int outFreq, ResamplerType type);

    
    int Push(const int16_t* samplesIn, int lengthIn, int16_t* samplesOut,
             int maxLen, int &outLen);

    
    int Insert(int16_t* samplesIn, int lengthIn);

    
    int Pull(int16_t* samplesOut, int desiredLen, int &outLen);

private:
    
    void* state1_;
    void* state2_;
    void* state3_;

    
    int16_t* in_buffer_;
    int16_t* out_buffer_;
    int in_buffer_size_;
    int out_buffer_size_;
    int in_buffer_size_max_;
    int out_buffer_size_max_;

    
    int my_in_frequency_khz_;
    int my_out_frequency_khz_;
    ResamplerMode my_mode_;
    ResamplerType my_type_;

    
    Resampler* slave_left_;
    Resampler* slave_right_;
};

}  

#endif 

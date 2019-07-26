









#ifndef WEBRTC_VOICE_ENGINE_DTMF_INBAND_H
#define WEBRTC_VOICE_ENGINE_DTMF_INBAND_H

#if _MSC_VER > 1000
#pragma once
#endif

#include "typedefs.h"
#include "voice_engine_defines.h"

namespace webrtc {
class CriticalSectionWrapper;

class DtmfInband
{
public:
    DtmfInband(const int32_t id);

    virtual ~DtmfInband();

    void Init();

    int SetSampleRate(const uint16_t frequency);

    int GetSampleRate(uint16_t& frequency);

    int AddTone(const uint8_t eventCode,
                int32_t lengthMs,
                int32_t attenuationDb);

    int ResetTone();
    int StartTone(const uint8_t eventCode, int32_t attenuationDb);

    int StopTone();

    bool IsAddingTone();

    int Get10msTone(int16_t output[320], uint16_t& outputSizeInSamples);

    uint32_t DelaySinceLastTone() const;

    void UpdateDelaySinceLastTone();

private:
    void ReInit();
    int16_t DtmfFix_generate(int16_t* decoded,
                             const int16_t value,
                             const int16_t volume,
                             const int16_t frameLen,
                             const int16_t fs);

private:
    enum {kDtmfFrameSizeMs = 10};
    enum {kDtmfAmpHigh = 32768};
    enum {kDtmfAmpLow  = 23171};	

    int16_t DtmfFix_generateSignal(const int16_t a1_times2,
                                   const int16_t a2_times2,
                                   const int16_t volume,
                                   int16_t* signal,
                                   const int16_t length);

private:
    CriticalSectionWrapper& _critSect;
    int32_t _id;
    uint16_t _outputFrequencyHz;  
    int16_t _oldOutputLow[2];     
    int16_t _oldOutputHigh[2];    
    int16_t _frameLengthSamples;  
    int32_t _remainingSamples;
    int16_t _eventCode;           
    int16_t _attenuationDb;       
    int32_t _lengthMs;
    bool _reinit;  
    bool _playing;
    uint32_t _delaySinceLastToneMS; 
};

}   

#endif

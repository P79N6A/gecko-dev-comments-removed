









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_DEFINES_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_DEFINES_H

#include "typedefs.h"

namespace webrtc {

static const int kAdmMaxDeviceNameSize = 128;
static const int kAdmMaxFileNameSize = 512;
static const int kAdmMaxGuidSize = 128;

static const int kAdmMinPlayoutBufferSizeMs = 10;
static const int kAdmMaxPlayoutBufferSizeMs = 250;





class AudioDeviceObserver
{
public:
    enum ErrorCode
    {
        kRecordingError = 0,
        kPlayoutError = 1
    };
    enum WarningCode
    {
        kRecordingWarning = 0,
        kPlayoutWarning = 1
    };

    virtual void OnErrorIsReported(const ErrorCode error) = 0;
    virtual void OnWarningIsReported(const WarningCode warning) = 0;

protected:
    virtual ~AudioDeviceObserver() {}
};





class AudioTransport
{
public:
    virtual int32_t RecordedDataIsAvailable(const void* audioSamples,
                                            const uint32_t nSamples,
                                            const uint8_t nBytesPerSample,
                                            const uint8_t nChannels,
                                            const uint32_t samplesPerSec,
                                            const uint32_t totalDelayMS,
                                            const int32_t clockDrift,
                                            const uint32_t currentMicLevel,
                                            uint32_t& newMicLevel) = 0;   

    virtual int32_t NeedMorePlayData(const uint32_t nSamples,
                                     const uint8_t nBytesPerSample,
                                     const uint8_t nChannels,
                                     const uint32_t samplesPerSec,
                                     void* audioSamples,
                                     uint32_t& nSamplesOut) = 0;

protected:
    virtual ~AudioTransport() {}
};

}  

#endif











#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_BUFFER_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_BUFFER_H

#include "typedefs.h"
#include "common_audio/resampler/include/resampler.h"
#include "file_wrapper.h"
#include "audio_device.h"
#include "list_wrapper.h"

namespace webrtc {
class CriticalSectionWrapper;

const WebRtc_UWord32 kPulsePeriodMs = 1000;
const WebRtc_UWord32 kMaxBufferSizeBytes = 3840; 

class AudioDeviceObserver;
class MediaFile;

class AudioDeviceBuffer
{
public:
    void SetId(WebRtc_UWord32 id);
    WebRtc_Word32 RegisterAudioCallback(AudioTransport* audioCallback);

    WebRtc_Word32 InitPlayout();
    WebRtc_Word32 InitRecording();

    WebRtc_Word32 SetRecordingSampleRate(WebRtc_UWord32 fsHz);
    WebRtc_Word32 SetPlayoutSampleRate(WebRtc_UWord32 fsHz);
    WebRtc_Word32 RecordingSampleRate() const;
    WebRtc_Word32 PlayoutSampleRate() const;

    WebRtc_Word32 SetRecordingChannels(WebRtc_UWord8 channels);
    WebRtc_Word32 SetPlayoutChannels(WebRtc_UWord8 channels);
    WebRtc_UWord8 RecordingChannels() const;
    WebRtc_UWord8 PlayoutChannels() const;
    WebRtc_Word32 SetRecordingChannel(
        const AudioDeviceModule::ChannelType channel);
    WebRtc_Word32 RecordingChannel(
        AudioDeviceModule::ChannelType& channel) const;

    WebRtc_Word32 SetRecordedBuffer(const void* audioBuffer,
                                    WebRtc_UWord32 nSamples);
    WebRtc_Word32 SetCurrentMicLevel(WebRtc_UWord32 level);
    WebRtc_Word32 SetVQEData(WebRtc_UWord32 playDelayMS,
                             WebRtc_UWord32 recDelayMS,
                             WebRtc_Word32 clockDrift);
    WebRtc_Word32 DeliverRecordedData();
    WebRtc_UWord32 NewMicLevel() const;

    WebRtc_Word32 RequestPlayoutData(WebRtc_UWord32 nSamples);
    WebRtc_Word32 GetPlayoutData(void* audioBuffer);

    WebRtc_Word32 StartInputFileRecording(
        const char fileName[kAdmMaxFileNameSize]);
    WebRtc_Word32 StopInputFileRecording();
    WebRtc_Word32 StartOutputFileRecording(
        const char fileName[kAdmMaxFileNameSize]);
    WebRtc_Word32 StopOutputFileRecording();

    AudioDeviceBuffer();
    ~AudioDeviceBuffer();

private:
    void _EmptyList();

private:
    WebRtc_Word32                   _id;
    CriticalSectionWrapper&         _critSect;
    CriticalSectionWrapper&         _critSectCb;

    AudioTransport*                 _ptrCbAudioTransport;

    WebRtc_UWord32                  _recSampleRate;
    WebRtc_UWord32                  _playSampleRate;

    WebRtc_UWord8                   _recChannels;
    WebRtc_UWord8                   _playChannels;

    
    AudioDeviceModule::ChannelType _recChannel;

    
    WebRtc_UWord8                   _recBytesPerSample;
    WebRtc_UWord8                   _playBytesPerSample;

    
    int8_t                          _recBuffer[kMaxBufferSizeBytes];

    
    WebRtc_UWord32                  _recSamples;
    WebRtc_UWord32                  _recSize;           

    
    int8_t                          _playBuffer[kMaxBufferSizeBytes];

    
    WebRtc_UWord32                  _playSamples;
    WebRtc_UWord32                  _playSize;          

    FileWrapper&                    _recFile;
    FileWrapper&                    _playFile;

    WebRtc_UWord32                  _currentMicLevel;
    WebRtc_UWord32                  _newMicLevel;

    WebRtc_UWord32                  _playDelayMS;
    WebRtc_UWord32                  _recDelayMS;

    WebRtc_Word32                   _clockDrift;

    bool                            _measureDelay;
    ListWrapper                     _pulseList;
    WebRtc_UWord32                  _lastPulseTime;
};

}  

#endif  

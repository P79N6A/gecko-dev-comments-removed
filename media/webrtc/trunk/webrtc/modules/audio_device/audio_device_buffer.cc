









#include "trace.h"
#include "critical_section_wrapper.h"
#include "audio_device_buffer.h"
#include "audio_device_utility.h"
#include "audio_device_config.h"

#include <stdlib.h>
#include <string.h>
#include <cassert>

#include "signal_processing_library.h"

namespace webrtc {





AudioDeviceBuffer::AudioDeviceBuffer() :
    _id(-1),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _critSectCb(*CriticalSectionWrapper::CreateCriticalSection()),
    _ptrCbAudioTransport(NULL),
    _recSampleRate(0),
    _playSampleRate(0),
    _recChannels(0),
    _playChannels(0),
    _recChannel(AudioDeviceModule::kChannelBoth),
    _recBytesPerSample(0),
    _playBytesPerSample(0),
    _recSamples(0),
    _recSize(0),
    _playSamples(0),
    _playSize(0),
    _recFile(*FileWrapper::Create()),
    _playFile(*FileWrapper::Create()),
    _currentMicLevel(0),
    _newMicLevel(0),
    _playDelayMS(0),
    _recDelayMS(0),
    _clockDrift(0),
    _measureDelay(false),    
    _pulseList(),
    _lastPulseTime(AudioDeviceUtility::GetTimeInMS())
{
    
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s created", __FUNCTION__);
    memset(_recBuffer, 0, kMaxBufferSizeBytes);
    memset(_playBuffer, 0, kMaxBufferSizeBytes);
}





AudioDeviceBuffer::~AudioDeviceBuffer()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s destroyed", __FUNCTION__);
    {
        CriticalSectionScoped lock(&_critSect);

        _recFile.Flush();
        _recFile.CloseFile();
        delete &_recFile;

        _playFile.Flush();
        _playFile.CloseFile();
        delete &_playFile;

        _EmptyList();
    }

    delete &_critSect;
    delete &_critSectCb;
}





void AudioDeviceBuffer::SetId(uint32_t id)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id, "AudioDeviceBuffer::SetId(id=%d)", id);
    _id = id;
}





int32_t AudioDeviceBuffer::RegisterAudioCallback(AudioTransport* audioCallback)
{
    CriticalSectionScoped lock(&_critSectCb);
    _ptrCbAudioTransport = audioCallback;

    return 0;
}





int32_t AudioDeviceBuffer::InitPlayout()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (_measureDelay)
    {
        _EmptyList();
        _lastPulseTime = AudioDeviceUtility::GetTimeInMS();
    }

    return 0;
}





int32_t AudioDeviceBuffer::InitRecording()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (_measureDelay)
    {
        _EmptyList();
        _lastPulseTime = AudioDeviceUtility::GetTimeInMS();
    }

    return 0;
}





int32_t AudioDeviceBuffer::SetRecordingSampleRate(uint32_t fsHz)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "AudioDeviceBuffer::SetRecordingSampleRate(fsHz=%u)", fsHz);

    CriticalSectionScoped lock(&_critSect);
    _recSampleRate = fsHz;
    return 0;
}





int32_t AudioDeviceBuffer::SetPlayoutSampleRate(uint32_t fsHz)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "AudioDeviceBuffer::SetPlayoutSampleRate(fsHz=%u)", fsHz);

    CriticalSectionScoped lock(&_critSect);
    _playSampleRate = fsHz;
    return 0;
}





int32_t AudioDeviceBuffer::RecordingSampleRate() const
{
    return _recSampleRate;
}





int32_t AudioDeviceBuffer::PlayoutSampleRate() const
{
    return _playSampleRate;
}





int32_t AudioDeviceBuffer::SetRecordingChannels(uint8_t channels)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "AudioDeviceBuffer::SetRecordingChannels(channels=%u)", channels);

    CriticalSectionScoped lock(&_critSect);
    _recChannels = channels;
    _recBytesPerSample = 2*channels;  
    return 0;
}





int32_t AudioDeviceBuffer::SetPlayoutChannels(uint8_t channels)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "AudioDeviceBuffer::SetPlayoutChannels(channels=%u)", channels);

    CriticalSectionScoped lock(&_critSect);
    _playChannels = channels;
    
    _playBytesPerSample = 2*channels;
    return 0;
}












int32_t AudioDeviceBuffer::SetRecordingChannel(const AudioDeviceModule::ChannelType channel)
{
    CriticalSectionScoped lock(&_critSect);

    if (_recChannels == 1)
    {
        return -1;
    }

    if (channel == AudioDeviceModule::kChannelBoth)
    {
        
        _recBytesPerSample = 4;
    }
    else
    {
        
        _recBytesPerSample = 2;
    }
    _recChannel = channel;

    return 0;
}





int32_t AudioDeviceBuffer::RecordingChannel(AudioDeviceModule::ChannelType& channel) const
{
    channel = _recChannel;
    return 0;
}





uint8_t AudioDeviceBuffer::RecordingChannels() const
{
    return _recChannels;
}





uint8_t AudioDeviceBuffer::PlayoutChannels() const
{
    return _playChannels;
}





int32_t AudioDeviceBuffer::SetCurrentMicLevel(uint32_t level)
{
    _currentMicLevel = level;
    return 0;
}





uint32_t AudioDeviceBuffer::NewMicLevel() const
{
    return _newMicLevel;
}





int32_t AudioDeviceBuffer::SetVQEData(uint32_t playDelayMS, uint32_t recDelayMS, int32_t clockDrift)
{
    if ((playDelayMS + recDelayMS) > 300)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceUtility, _id, "too long delay (play:%i rec:%i)", playDelayMS, recDelayMS, clockDrift);
    }

    _playDelayMS = playDelayMS;
    _recDelayMS = recDelayMS;
    _clockDrift = clockDrift;

    return 0;
}





int32_t AudioDeviceBuffer::StartInputFileRecording(
    const char fileName[kAdmMaxFileNameSize])
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    _recFile.Flush();
    _recFile.CloseFile();

    return (_recFile.OpenFile(fileName, false, false, false));
}





int32_t AudioDeviceBuffer::StopInputFileRecording()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    _recFile.Flush();
    _recFile.CloseFile();

    return 0;
}





int32_t AudioDeviceBuffer::StartOutputFileRecording(
    const char fileName[kAdmMaxFileNameSize])
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    _playFile.Flush();
    _playFile.CloseFile();

    return (_playFile.OpenFile(fileName, false, false, false));
}





int32_t AudioDeviceBuffer::StopOutputFileRecording()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    _playFile.Flush();
    _playFile.CloseFile();

    return 0;
}
















int32_t AudioDeviceBuffer::SetRecordedBuffer(const void* audioBuffer,
                                             uint32_t nSamples)
{
    CriticalSectionScoped lock(&_critSect);

    if (_recBytesPerSample == 0)
    {
        assert(false);
        return -1;
    }

    _recSamples = nSamples;
    _recSize = _recBytesPerSample*nSamples; 
    if (_recSize > kMaxBufferSizeBytes)
    {
        assert(false);
        return -1;
    }

    if (nSamples != _recSamples)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "invalid number of recorded samples (%d)", nSamples);
        return -1;
    }

    if (_recChannel == AudioDeviceModule::kChannelBoth)
    {
        
        memcpy(&_recBuffer[0], audioBuffer, _recSize);
    }
    else
    {
        int16_t* ptr16In = (int16_t*)audioBuffer;
        int16_t* ptr16Out = (int16_t*)&_recBuffer[0];

        if (AudioDeviceModule::kChannelRight == _recChannel)
        {
            ptr16In++;
        }

        
        for (uint32_t i = 0; i < _recSamples; i++)
        {
            *ptr16Out = *ptr16In;
            ptr16Out++;
            ptr16In++;
            ptr16In++;
        }
    }

    if (_recFile.Open())
    {
        
        _recFile.Write(&_recBuffer[0], _recSize);
    }

    return 0;
}





int32_t AudioDeviceBuffer::DeliverRecordedData()
{
    CriticalSectionScoped lock(&_critSectCb);

    
    if ((_recSampleRate == 0)     ||
        (_recSamples == 0)        ||
        (_recBytesPerSample == 0) ||
        (_recChannels == 0))
    {
        assert(false);
        return -1;
    }

    if (_ptrCbAudioTransport == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "failed to deliver recorded data (AudioTransport does not exist)");
        return 0;
    }

    int32_t res(0);
    uint32_t newMicLevel(0);
    uint32_t totalDelayMS = _playDelayMS +_recDelayMS;

    if (_measureDelay)
    {
        CriticalSectionScoped lock(&_critSect);

        memset(&_recBuffer[0], 0, _recSize);
        uint32_t time = AudioDeviceUtility::GetTimeInMS();
        if (time - _lastPulseTime > 500)
        {
            _pulseList.PushBack(time);
            _lastPulseTime = time;

            int16_t* ptr16 = (int16_t*)&_recBuffer[0];
            *ptr16 = 30000;
        }
    }

    res = _ptrCbAudioTransport->RecordedDataIsAvailable(&_recBuffer[0],
                                                        _recSamples,
                                                        _recBytesPerSample,
                                                        _recChannels,
                                                        _recSampleRate,
                                                        totalDelayMS,
                                                        _clockDrift,
                                                        _currentMicLevel,
                                                        newMicLevel);
    if (res != -1)
    {
        _newMicLevel = newMicLevel;
    }

    return 0;
}





int32_t AudioDeviceBuffer::RequestPlayoutData(uint32_t nSamples)
{
    uint32_t playSampleRate = 0;
    uint8_t playBytesPerSample = 0;
    uint8_t playChannels = 0;
    {
        CriticalSectionScoped lock(&_critSect);

        
        
        playSampleRate = _playSampleRate;
        playBytesPerSample = _playBytesPerSample;
        playChannels = _playChannels;

        
        if ((playBytesPerSample == 0) ||
            (playChannels == 0)       ||
            (playSampleRate == 0))
        {
            assert(false);
            return -1;
        }

        _playSamples = nSamples;
        _playSize = playBytesPerSample * nSamples;  
        if (_playSize > kMaxBufferSizeBytes)
        {
            assert(false);
            return -1;
        }

        if (nSamples != _playSamples)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "invalid number of samples to be played out (%d)", nSamples);
            return -1;
        }
    }

    uint32_t nSamplesOut(0);

    CriticalSectionScoped lock(&_critSectCb);

    if (_ptrCbAudioTransport == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "failed to feed data to playout (AudioTransport does not exist)");
        return 0;
    }

    if (_ptrCbAudioTransport)
    {
        uint32_t res(0);

        res = _ptrCbAudioTransport->NeedMorePlayData(_playSamples,
                                                     playBytesPerSample,
                                                     playChannels,
                                                     playSampleRate,
                                                     &_playBuffer[0],
                                                     nSamplesOut);
        if (res != 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "NeedMorePlayData() failed");
        }

        
        

        if (_measureDelay)
        {
            CriticalSectionScoped lock(&_critSect);

            int16_t maxAbs = WebRtcSpl_MaxAbsValueW16((const int16_t*)&_playBuffer[0], (int16_t)nSamplesOut*_playChannels);
            if (maxAbs > 1000)
            {
                uint32_t nowTime = AudioDeviceUtility::GetTimeInMS();

                if (!_pulseList.Empty())
                {
                    ListItem* item = _pulseList.First();
                    if (item)
                    {
                        int16_t maxIndex = WebRtcSpl_MaxAbsIndexW16((const int16_t*)&_playBuffer[0], (int16_t)nSamplesOut*_playChannels);
                        uint32_t pulseTime = item->GetUnsignedItem();
                        uint32_t diff = nowTime - pulseTime + (10*maxIndex)/(nSamplesOut*_playChannels);
                        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "diff time in playout delay (%d)", diff);
                    }
                    _pulseList.PopFront();
                }
            }
        }
    }

    return nSamplesOut;
}





int32_t AudioDeviceBuffer::GetPlayoutData(void* audioBuffer)
{
    CriticalSectionScoped lock(&_critSect);

    if (_playSize > kMaxBufferSizeBytes)
    {
       WEBRTC_TRACE(kTraceError, kTraceUtility, _id, "_playSize %i exceeds "
       "kMaxBufferSizeBytes in AudioDeviceBuffer::GetPlayoutData", _playSize);
       assert(false);
       return -1;
    }

    memcpy(audioBuffer, &_playBuffer[0], _playSize);

    if (_playFile.Open())
    {
        
        _playFile.Write(&_playBuffer[0], _playSize);
    }

    return _playSamples;
}





void AudioDeviceBuffer::_EmptyList()
{
    while (!_pulseList.Empty())
    {
        ListItem* item = _pulseList.First();
        if (item)
        {
            
        }
        _pulseList.PopFront();
    }
}

}  

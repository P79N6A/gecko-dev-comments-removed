






























#ifndef WEBRTC_VOICE_ENGINE_VOE_HARDWARE_H
#define WEBRTC_VOICE_ENGINE_VOE_HARDWARE_H

#include "common_types.h"

namespace webrtc {

class VoiceEngine;

class WEBRTC_DLLEXPORT VoEHardware
{
public:
    
    
    
    static VoEHardware* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    virtual int GetNumOfRecordingDevices(int& devices) = 0;

    
    virtual int GetNumOfPlayoutDevices(int& devices) = 0;

    
    
    
    virtual int GetRecordingDeviceName(int index, char strNameUTF8[128],
                                       char strGuidUTF8[128]) = 0;

    
    
    
    virtual int GetPlayoutDeviceName(int index, char strNameUTF8[128],
                                     char strGuidUTF8[128]) = 0;

    
    virtual int GetRecordingDeviceStatus(bool& isAvailable) = 0;

    
    virtual int GetPlayoutDeviceStatus(bool& isAvailable) = 0;

    
    virtual int SetRecordingDevice(
        int index, StereoChannel recordingChannel = kStereoBoth) = 0;

    
    virtual int SetPlayoutDevice(int index) = 0;

    
    virtual int SetAudioDeviceLayer(AudioLayers audioLayer) = 0;

    
    virtual int GetAudioDeviceLayer(AudioLayers& audioLayer) = 0;

    
    
    virtual int GetCPULoad(int& loadPercent) = 0;

    
    
    
    
    
    virtual int GetSystemCPULoad(int& loadPercent) = 0;

    
    virtual int ResetAudioDevice() = 0;

    
    virtual int AudioDeviceControl(
        unsigned int par1, unsigned int par2, unsigned int par3) = 0;

    
    virtual int SetLoudspeakerStatus(bool enable) = 0;

    
    virtual int GetLoudspeakerStatus(bool& enabled) = 0;

    
    virtual int SetRecordingSampleRate(unsigned int samples_per_sec) = 0;
    virtual int RecordingSampleRate(unsigned int* samples_per_sec) const = 0;
    virtual int SetPlayoutSampleRate(unsigned int samples_per_sec) = 0;
    virtual int PlayoutSampleRate(unsigned int* samples_per_sec) const = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual int EnableBuiltInAEC(bool enable) = 0;
    virtual bool BuiltInAECIsEnabled() const = 0;

protected:
    VoEHardware() {}
    virtual ~VoEHardware() {}
};

} 

#endif  

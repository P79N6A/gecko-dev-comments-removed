









#ifndef WEBRTC_VOICE_ENGINE_VOE_HARDWARE_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_HARDWARE_IMPL_H

#include "webrtc/voice_engine/include/voe_hardware.h"

#include "webrtc/voice_engine/shared_data.h"

namespace webrtc
{

class VoEHardwareImpl: public VoEHardware
{
public:
    virtual int GetNumOfRecordingDevices(int& devices);

    virtual int GetNumOfPlayoutDevices(int& devices);

    virtual int GetRecordingDeviceName(int index,
                                       char strNameUTF8[128],
                                       char strGuidUTF8[128]);

    virtual int GetPlayoutDeviceName(int index,
                                     char strNameUTF8[128],
                                     char strGuidUTF8[128]);

    virtual int SetRecordingDevice(
        int index,
        StereoChannel recordingChannel = kStereoBoth);

    virtual int SetPlayoutDevice(int index);

    virtual int SetAudioDeviceLayer(AudioLayers audioLayer);

    virtual int GetAudioDeviceLayer(AudioLayers& audioLayer);

    virtual int SetRecordingSampleRate(unsigned int samples_per_sec);
    virtual int RecordingSampleRate(unsigned int* samples_per_sec) const;
    virtual int SetPlayoutSampleRate(unsigned int samples_per_sec);
    virtual int PlayoutSampleRate(unsigned int* samples_per_sec) const;

protected:
    VoEHardwareImpl(voe::SharedData* shared);
    virtual ~VoEHardwareImpl();

private:
    voe::SharedData* _shared;
};

}  

#endif  

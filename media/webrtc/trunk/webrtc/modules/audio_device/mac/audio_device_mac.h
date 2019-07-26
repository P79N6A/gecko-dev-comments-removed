









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_MAC_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_MAC_H

#include "webrtc/modules/audio_device/audio_device_generic.h"
#include "webrtc/modules/audio_device/mac/audio_mixer_manager_mac.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

#include <AudioToolbox/AudioConverter.h>
#include <CoreAudio/CoreAudio.h>
#include <mach/semaphore.h>

struct PaUtilRingBuffer;

namespace webrtc
{
class EventWrapper;
class ThreadWrapper;

const uint32_t N_REC_SAMPLES_PER_SEC = 48000;
const uint32_t N_PLAY_SAMPLES_PER_SEC = 48000;

const uint32_t N_REC_CHANNELS = 1; 
const uint32_t N_PLAY_CHANNELS = 2; 
const uint32_t N_DEVICE_CHANNELS = 64;

const uint32_t ENGINE_REC_BUF_SIZE_IN_SAMPLES = (N_REC_SAMPLES_PER_SEC / 100);
const uint32_t ENGINE_PLAY_BUF_SIZE_IN_SAMPLES = (N_PLAY_SAMPLES_PER_SEC / 100);

const int N_BLOCKS_IO = 2;
const int N_BUFFERS_IN = 2;  
const int N_BUFFERS_OUT = 3;  

const uint32_t TIMER_PERIOD_MS = (2 * 10 * N_BLOCKS_IO * 1000000);

const uint32_t REC_BUF_SIZE_IN_SAMPLES =
    ENGINE_REC_BUF_SIZE_IN_SAMPLES * N_DEVICE_CHANNELS * N_BUFFERS_IN;
const uint32_t PLAY_BUF_SIZE_IN_SAMPLES =
    ENGINE_PLAY_BUF_SIZE_IN_SAMPLES * N_PLAY_CHANNELS * N_BUFFERS_OUT;

class AudioDeviceMac: public AudioDeviceGeneric
{
public:
    AudioDeviceMac(const int32_t id);
    ~AudioDeviceMac();

    
    virtual int32_t
        ActiveAudioLayer(AudioDeviceModule::AudioLayer& audioLayer) const;

    
    virtual int32_t Init();
    virtual int32_t Terminate();
    virtual bool Initialized() const;

    
    virtual int16_t PlayoutDevices();
    virtual int16_t RecordingDevices();
    virtual int32_t PlayoutDeviceName(
        uint16_t index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]);
    virtual int32_t RecordingDeviceName(
        uint16_t index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]);

    
    virtual int32_t SetPlayoutDevice(uint16_t index);
    virtual int32_t SetPlayoutDevice(
        AudioDeviceModule::WindowsDeviceType device);
    virtual int32_t SetRecordingDevice(uint16_t index);
    virtual int32_t SetRecordingDevice(
        AudioDeviceModule::WindowsDeviceType device);

    
    virtual int32_t PlayoutIsAvailable(bool& available);
    virtual int32_t InitPlayout();
    virtual bool PlayoutIsInitialized() const;
    virtual int32_t RecordingIsAvailable(bool& available);
    virtual int32_t InitRecording();
    virtual bool RecordingIsInitialized() const;

    
    virtual int32_t StartPlayout();
    virtual int32_t StopPlayout();
    virtual bool Playing() const;
    virtual int32_t StartRecording();
    virtual int32_t StopRecording();
    virtual bool Recording() const;

    
    virtual int32_t SetAGC(bool enable);
    virtual bool AGC() const;

    
    virtual int32_t SetWaveOutVolume(uint16_t volumeLeft, uint16_t volumeRight);
    virtual int32_t WaveOutVolume(uint16_t& volumeLeft,
                                  uint16_t& volumeRight) const;

    
    virtual int32_t SpeakerIsAvailable(bool& available);
    virtual int32_t InitSpeaker();
    virtual bool SpeakerIsInitialized() const;
    virtual int32_t MicrophoneIsAvailable(bool& available);
    virtual int32_t InitMicrophone();
    virtual bool MicrophoneIsInitialized() const;

    
    virtual int32_t SpeakerVolumeIsAvailable(bool& available);
    virtual int32_t SetSpeakerVolume(uint32_t volume);
    virtual int32_t SpeakerVolume(uint32_t& volume) const;
    virtual int32_t MaxSpeakerVolume(uint32_t& maxVolume) const;
    virtual int32_t MinSpeakerVolume(uint32_t& minVolume) const;
    virtual int32_t SpeakerVolumeStepSize(uint16_t& stepSize) const;

    
    virtual int32_t MicrophoneVolumeIsAvailable(bool& available);
    virtual int32_t SetMicrophoneVolume(uint32_t volume);
    virtual int32_t MicrophoneVolume(uint32_t& volume) const;
    virtual int32_t MaxMicrophoneVolume(uint32_t& maxVolume) const;
    virtual int32_t MinMicrophoneVolume(uint32_t& minVolume) const;
    virtual int32_t
        MicrophoneVolumeStepSize(uint16_t& stepSize) const;

    
    virtual int32_t MicrophoneMuteIsAvailable(bool& available);
    virtual int32_t SetMicrophoneMute(bool enable);
    virtual int32_t MicrophoneMute(bool& enabled) const;

    
    virtual int32_t SpeakerMuteIsAvailable(bool& available);
    virtual int32_t SetSpeakerMute(bool enable);
    virtual int32_t SpeakerMute(bool& enabled) const;

    
    virtual int32_t MicrophoneBoostIsAvailable(bool& available);
    virtual int32_t SetMicrophoneBoost(bool enable);
    virtual int32_t MicrophoneBoost(bool& enabled) const;

    
    virtual int32_t StereoPlayoutIsAvailable(bool& available);
    virtual int32_t SetStereoPlayout(bool enable);
    virtual int32_t StereoPlayout(bool& enabled) const;
    virtual int32_t StereoRecordingIsAvailable(bool& available);
    virtual int32_t SetStereoRecording(bool enable);
    virtual int32_t StereoRecording(bool& enabled) const;

    
    virtual int32_t
        SetPlayoutBuffer(const AudioDeviceModule::BufferType type,
                         uint16_t sizeMS);
    virtual int32_t PlayoutBuffer(AudioDeviceModule::BufferType& type,
                                  uint16_t& sizeMS) const;
    virtual int32_t PlayoutDelay(uint16_t& delayMS) const;
    virtual int32_t RecordingDelay(uint16_t& delayMS) const;

    
    virtual int32_t CPULoad(uint16_t& load) const;

public:
    virtual bool PlayoutWarning() const;
    virtual bool PlayoutError() const;
    virtual bool RecordingWarning() const;
    virtual bool RecordingError() const;
    virtual void ClearPlayoutWarning();
    virtual void ClearPlayoutError();
    virtual void ClearRecordingWarning();
    virtual void ClearRecordingError();

public:
    virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer);

private:
    void Lock()
    {
        _critSect.Enter();
    }
    ;
    void UnLock()
    {
        _critSect.Leave();
    }
    ;
    int32_t Id()
    {
        return _id;
    }

    static void AtomicSet32(int32_t* theValue, int32_t newValue);
    static int32_t AtomicGet32(int32_t* theValue);

    static void logCAMsg(const TraceLevel level,
                         const TraceModule module,
                         const int32_t id, const char *msg,
                         const char *err);

    int32_t GetNumberDevices(const AudioObjectPropertyScope scope,
                             AudioDeviceID scopedDeviceIds[],
                             const uint32_t deviceListLength);

    int32_t GetDeviceName(const AudioObjectPropertyScope scope,
                          const uint16_t index, char* name);

    int32_t InitDevice(uint16_t userDeviceIndex,
                       AudioDeviceID& deviceId, bool isInput);

    static OSStatus
        objectListenerProc(AudioObjectID objectId, UInt32 numberAddresses,
                           const AudioObjectPropertyAddress addresses[],
                           void* clientData);

    OSStatus
        implObjectListenerProc(AudioObjectID objectId, UInt32 numberAddresses,
                               const AudioObjectPropertyAddress addresses[]);

    int32_t HandleDeviceChange();

    int32_t
        HandleStreamFormatChange(AudioObjectID objectId,
                                 AudioObjectPropertyAddress propertyAddress);

    int32_t
        HandleDataSourceChange(AudioObjectID objectId,
                               AudioObjectPropertyAddress propertyAddress);

    int32_t
        HandleProcessorOverload(AudioObjectPropertyAddress propertyAddress);

private:
    static OSStatus deviceIOProc(AudioDeviceID device,
                                 const AudioTimeStamp *now,
                                 const AudioBufferList *inputData,
                                 const AudioTimeStamp *inputTime,
                                 AudioBufferList *outputData,
                                 const AudioTimeStamp* outputTime,
                                 void *clientData);

    static OSStatus
        outConverterProc(AudioConverterRef audioConverter,
                         UInt32 *numberDataPackets, AudioBufferList *data,
                         AudioStreamPacketDescription **dataPacketDescription,
                         void *userData);

    static OSStatus inDeviceIOProc(AudioDeviceID device,
                                   const AudioTimeStamp *now,
                                   const AudioBufferList *inputData,
                                   const AudioTimeStamp *inputTime,
                                   AudioBufferList *outputData,
                                   const AudioTimeStamp *outputTime,
                                   void *clientData);

    static OSStatus
        inConverterProc(AudioConverterRef audioConverter,
                        UInt32 *numberDataPackets, AudioBufferList *data,
                        AudioStreamPacketDescription **dataPacketDescription,
                        void *inUserData);

    OSStatus implDeviceIOProc(const AudioBufferList *inputData,
                              const AudioTimeStamp *inputTime,
                              AudioBufferList *outputData,
                              const AudioTimeStamp *outputTime);

    OSStatus implOutConverterProc(UInt32 *numberDataPackets,
                                  AudioBufferList *data);

    OSStatus implInDeviceIOProc(const AudioBufferList *inputData,
                                const AudioTimeStamp *inputTime);

    OSStatus implInConverterProc(UInt32 *numberDataPackets,
                                 AudioBufferList *data);

    static bool RunCapture(void*);
    static bool RunRender(void*);
    bool CaptureWorkerThread();
    bool RenderWorkerThread();

private:
    bool KeyPressed();

private:
    AudioDeviceBuffer* _ptrAudioBuffer;

    CriticalSectionWrapper& _critSect;

    EventWrapper& _stopEventRec;
    EventWrapper& _stopEvent;

    ThreadWrapper* _captureWorkerThread;
    ThreadWrapper* _renderWorkerThread;
    uint32_t _captureWorkerThreadId;
    uint32_t _renderWorkerThreadId;

    int32_t _id;

    AudioMixerManagerMac _mixerManager;

    uint16_t _inputDeviceIndex;
    uint16_t _outputDeviceIndex;
    AudioDeviceID _inputDeviceID;
    AudioDeviceID _outputDeviceID;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1050
    AudioDeviceIOProcID _inDeviceIOProcID;
    AudioDeviceIOProcID _deviceIOProcID;
#endif
    bool _inputDeviceIsSpecified;
    bool _outputDeviceIsSpecified;

    uint8_t _recChannels;
    uint8_t _playChannels;

    Float32* _captureBufData;
    SInt16* _renderBufData;

    SInt16 _renderConvertData[PLAY_BUF_SIZE_IN_SAMPLES];

    AudioDeviceModule::BufferType _playBufType;

private:
    bool _initialized;
    bool _isShutDown;
    bool _recording;
    bool _playing;
    bool _recIsInitialized;
    bool _playIsInitialized;
    bool _AGC;

    
    int32_t _renderDeviceIsAlive;
    int32_t _captureDeviceIsAlive;

    bool _twoDevices;
    bool _doStop; 
    bool _doStopRec; 
    bool _macBookPro;
    bool _macBookProPanRight;

    AudioConverterRef _captureConverter;
    AudioConverterRef _renderConverter;

    AudioStreamBasicDescription _outStreamFormat;
    AudioStreamBasicDescription _outDesiredFormat;
    AudioStreamBasicDescription _inStreamFormat;
    AudioStreamBasicDescription _inDesiredFormat;

    uint32_t _captureLatencyUs;
    uint32_t _renderLatencyUs;

    
    mutable int32_t _captureDelayUs;
    mutable int32_t _renderDelayUs;

    int32_t _renderDelayOffsetSamples;

private:
    uint16_t _playBufDelayFixed; 

    uint16_t _playWarning;
    uint16_t _playError;
    uint16_t _recWarning;
    uint16_t _recError;

    PaUtilRingBuffer* _paCaptureBuffer;
    PaUtilRingBuffer* _paRenderBuffer;

    semaphore_t _renderSemaphore;
    semaphore_t _captureSemaphore;

    int _captureBufSizeSamples;
    int _renderBufSizeSamples;

private:
    
    
    bool prev_key_state_[0x5d];
};

}  

#endif  

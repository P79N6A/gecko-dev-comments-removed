
































#ifndef WEBRTC_VOICE_ENGINE_VOE_BASE_H
#define WEBRTC_VOICE_ENGINE_VOE_BASE_H

#include "common_types.h"

namespace webrtc {

class AudioDeviceModule;
class AudioProcessing;

const int kVoEDefault = -1;


class WEBRTC_DLLEXPORT VoiceEngineObserver
{
public:
    
    
    
    virtual void CallbackOnError(const int channel, const int errCode) = 0;

protected:
    virtual ~VoiceEngineObserver() {}
};


class WEBRTC_DLLEXPORT VoiceEngine
{
public:
    
    
    static VoiceEngine* Create();

    
    
    
    
    static bool Delete(VoiceEngine*& voiceEngine);

    
    
    static int SetTraceFilter(const unsigned int filter);

    
    static int SetTraceFile(const char* fileNameUTF8,
                            const bool addFileCounter = false);

    
    
    static int SetTraceCallback(TraceCallback* callback);

    static int SetAndroidObjects(void* javaVM, void* env, void* context);

protected:
    VoiceEngine() {}
    ~VoiceEngine() {}
};


class WEBRTC_DLLEXPORT VoEBase
{
public:
    
    
    
    static VoEBase* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    virtual int Release() = 0;

    
    
    virtual int RegisterVoiceEngineObserver(VoiceEngineObserver& observer) = 0;

    
    
    virtual int DeRegisterVoiceEngineObserver() = 0;

    
    
    
    
    
    
    
    
    
    
    virtual int Init(AudioDeviceModule* external_adm = NULL,
                     AudioProcessing* audioproc = NULL) = 0;

    
    virtual AudioProcessing* audio_processing() = 0;

    
    virtual int Terminate() = 0;

    
    virtual int MaxNumOfChannels() = 0;

    
    virtual int CreateChannel() = 0;

    
    virtual int DeleteChannel(int channel) = 0;

    
    
    virtual int StartReceive(int channel) = 0;

    
    virtual int StopReceive(int channel) = 0;

    
    
    virtual int StartPlayout(int channel) = 0;

    
    
    virtual int StopPlayout(int channel) = 0;

    
    
    virtual int StartSend(int channel) = 0;

    
    virtual int StopSend(int channel) = 0;

    
    virtual int GetVersion(char version[1024]) = 0;

    
    virtual int LastError() = 0;

    
    virtual int SetOnHoldStatus(int channel, bool enable,
                                OnHoldModes mode = kHoldSendAndPlay) = 0;

    
    virtual int GetOnHoldStatus(int channel, bool& enabled,
                                OnHoldModes& mode) = 0;

    
    virtual int SetNetEQPlayoutMode(int channel, NetEqModes mode) = 0;

    
    virtual int GetNetEQPlayoutMode(int channel, NetEqModes& mode) = 0;

protected:
    VoEBase() {}
    virtual ~VoEBase() {}
};

} 

#endif  

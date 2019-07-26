
















#include <android/log.h>
#include <stdlib.h>

#include "audio_device_utility.h"
#include "audio_device_jni_android.h"
#include "audio_device_config.h"

#include "trace.h"
#include "thread_wrapper.h"
#include "event_wrapper.h"

namespace webrtc
{


JavaVM* AudioDeviceAndroidJni::globalJvm = NULL;
JNIEnv* AudioDeviceAndroidJni::globalJNIEnv = NULL;
jobject AudioDeviceAndroidJni::globalContext = NULL;
jclass AudioDeviceAndroidJni::globalScClass = NULL;









WebRtc_Word32 AudioDeviceAndroidJni::SetAndroidAudioDeviceObjects(
    void* javaVM,
    void* env,
    void* context) {
  __android_log_print(ANDROID_LOG_DEBUG, "WEBRTC", "JNI:%s", __FUNCTION__);

  
  globalJvm = reinterpret_cast<JavaVM*>(javaVM);

  if (env) {
    globalJNIEnv = reinterpret_cast<JNIEnv*>(env);
    
    jclass javaScClassLocal = globalJNIEnv->FindClass(
        "org/webrtc/voiceengine/WebRTCAudioDevice");
    if (!javaScClassLocal) {
      WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
                   "%s: could not find java class", __FUNCTION__);
      return -1; 
    }

    
    
    globalScClass = reinterpret_cast<jclass> (
        globalJNIEnv->NewGlobalRef(javaScClassLocal));
    if (!globalScClass) {
      WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
                   "%s: could not create reference", __FUNCTION__);
      return -1;
    }

    globalContext = globalJNIEnv->NewGlobalRef(
        reinterpret_cast<jobject>(context));
    if (!globalContext) {
      WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
                   "%s: could not create context reference", __FUNCTION__);
      return -1;
    }

    
    globalJNIEnv->DeleteLocalRef(javaScClassLocal);
  }
  else { 
    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, -1,
                 "%s: env is NULL, assuming deinit", __FUNCTION__);

    if (!globalJNIEnv) {
      WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, -1,
                   "%s: saved env already NULL", __FUNCTION__);
      return 0;
    }

    globalJNIEnv->DeleteGlobalRef(globalScClass);
    globalScClass = reinterpret_cast<jclass>(NULL);

    globalJNIEnv->DeleteGlobalRef(globalContext);
    globalContext = reinterpret_cast<jobject>(NULL);

    globalJNIEnv = reinterpret_cast<JNIEnv*>(NULL);
  }

  return 0;
}









AudioDeviceAndroidJni::AudioDeviceAndroidJni(const WebRtc_Word32 id) :
            _ptrAudioBuffer(NULL),
            _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
            _id(id),
            _timeEventRec(*EventWrapper::Create()),
            _timeEventPlay(*EventWrapper::Create()),
            _recStartStopEvent(*EventWrapper::Create()),
            _playStartStopEvent(*EventWrapper::Create()),
            _ptrThreadPlay(NULL),
            _ptrThreadRec(NULL),
            _recThreadID(0),
            _playThreadID(0),
            _playThreadIsInitialized(false),
            _recThreadIsInitialized(false),
            _shutdownPlayThread(false),
            _shutdownRecThread(false),
            
            _recordingDeviceIsSpecified(false),
            _playoutDeviceIsSpecified(false), _initialized(false),
            _recording(false), _playing(false), _recIsInitialized(false),
            _playIsInitialized(false), _micIsInitialized(false),
            _speakerIsInitialized(false), _startRec(false), _stopRec(false),
            _startPlay(false), _stopPlay(false), _playWarning(0),
            _playError(0), _recWarning(0), _recError(0), _delayPlayout(0),
            _delayRecording(0),
            _AGC(false),
            _samplingFreqIn((N_REC_SAMPLES_PER_SEC/1000)),
            _samplingFreqOut((N_PLAY_SAMPLES_PER_SEC/1000)),
            _maxSpeakerVolume(0),
            _loudSpeakerOn(false),
            _recAudioSource(1), 
            _javaVM(NULL), _jniEnvPlay(NULL),
            _jniEnvRec(NULL), _javaScClass(0), _javaScObj(0),
            _javaPlayBuffer(0), _javaRecBuffer(0), _javaDirectPlayBuffer(NULL),
            _javaDirectRecBuffer(NULL), _javaMidPlayAudio(0),
            _javaMidRecAudio(0)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);

    memset(_recBuffer, 0, sizeof(_recBuffer));
}





AudioDeviceAndroidJni::~AudioDeviceAndroidJni()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);

    Terminate();

    delete &_recStartStopEvent;
    delete &_playStartStopEvent;
    delete &_timeEventRec;
    delete &_timeEventPlay;
    delete &_critSect;
}









void AudioDeviceAndroidJni::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{

    CriticalSectionScoped lock(&_critSect);

    _ptrAudioBuffer = audioBuffer;

    
    _ptrAudioBuffer->SetRecordingSampleRate(N_REC_SAMPLES_PER_SEC);
    _ptrAudioBuffer->SetPlayoutSampleRate(N_PLAY_SAMPLES_PER_SEC);
    _ptrAudioBuffer->SetRecordingChannels(N_REC_CHANNELS);
    _ptrAudioBuffer->SetPlayoutChannels(N_PLAY_CHANNELS);
}





WebRtc_Word32 AudioDeviceAndroidJni::ActiveAudioLayer(
        AudioDeviceModule::AudioLayer& audioLayer) const
{

    audioLayer = AudioDeviceModule::kPlatformDefaultAudio;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::Init()
{

    CriticalSectionScoped lock(&_critSect);

    if (_initialized)
    {
        return 0;
    }

    _playWarning = 0;
    _playError = 0;
    _recWarning = 0;
    _recError = 0;

    
    
    
    if (InitJavaResources() != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: Failed to init Java resources", __FUNCTION__);
        return -1;
    }

    
    
    if (InitSampleRate() != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: Failed to init samplerate", __FUNCTION__);
        return -1;
    }

    
    const char* threadName = "jni_audio_capture_thread";
    _ptrThreadRec = ThreadWrapper::CreateThread(RecThreadFunc, this,
                                                kRealtimePriority, threadName);
    if (_ptrThreadRec == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to create the rec audio thread");
        return -1;
    }

    unsigned int threadID(0);
    if (!_ptrThreadRec->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the rec audio thread");
        delete _ptrThreadRec;
        _ptrThreadRec = NULL;
        return -1;
    }
    _recThreadID = threadID;

    
    threadName = "jni_audio_render_thread";
    _ptrThreadPlay = ThreadWrapper::CreateThread(PlayThreadFunc, this,
                                                 kRealtimePriority, threadName);
    if (_ptrThreadPlay == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to create the play audio thread");
        return -1;
    }

    threadID = 0;
    if (!_ptrThreadPlay->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the play audio thread");
        delete _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        return -1;
    }
    _playThreadID = threadID;

    _initialized = true;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::Terminate()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized)
    {
        return 0;
    }

    
    StopRecording();
    _shutdownRecThread = true;
    _timeEventRec.Set(); 
    if (_ptrThreadRec)
    {
        
        _critSect.Leave();
        if (kEventSignaled != _recStartStopEvent.Wait(5000))
        {
            WEBRTC_TRACE(
                         kTraceError,
                         kTraceAudioDevice,
                         _id,
                         "%s: Recording thread shutdown timed out, cannot "
                         "terminate thread",
                         __FUNCTION__);
            
            return -1;
        }
        _recStartStopEvent.Reset();
        _critSect.Enter();

        
        ThreadWrapper* tmpThread = _ptrThreadRec;
        _ptrThreadRec = NULL;
        _critSect.Leave();
        tmpThread->SetNotAlive();
        
        _timeEventRec.Set();
        if (tmpThread->Stop())
        {
            delete tmpThread;
            _jniEnvRec = NULL;
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  failed to close down the rec audio thread");
        }
        _critSect.Enter();

        _recThreadIsInitialized = false;
    }
    _micIsInitialized = false;
    _recordingDeviceIsSpecified = false;

    
    StopPlayout();
    _shutdownPlayThread = true;
    _timeEventPlay.Set(); 
    if (_ptrThreadPlay)
    {
        
        _critSect.Leave();
        if (kEventSignaled != _playStartStopEvent.Wait(5000))
        {
            WEBRTC_TRACE(
                         kTraceError,
                         kTraceAudioDevice,
                         _id,
                         "%s: Playout thread shutdown timed out, cannot "
                         "terminate thread",
                         __FUNCTION__);
            
            return -1;
        }
        _playStartStopEvent.Reset();
        _critSect.Enter();

        
        ThreadWrapper* tmpThread = _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        _critSect.Leave();
        tmpThread->SetNotAlive();
        _timeEventPlay.Set();
        if (tmpThread->Stop())
        {
            delete tmpThread;
            _jniEnvPlay = NULL;
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  failed to close down the play audio thread");
        }
        _critSect.Enter();

        _playThreadIsInitialized = false;
    }
    _speakerIsInitialized = false;
    _playoutDeviceIsSpecified = false;

    
    JNIEnv *env;
    bool isAttached = false;

    
    if (_javaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        
        
        jint res = _javaVM->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "%s: Could not attach thread to JVM (%d, %p)",
                         __FUNCTION__, res, env);
            return -1;
        }
        isAttached = true;
    }

    
    _javaMidPlayAudio = 0;
    _javaMidRecAudio = 0;
    _javaDirectPlayBuffer = NULL;
    _javaDirectRecBuffer = NULL;

    
    
    env->DeleteGlobalRef(_javaPlayBuffer);
    _javaPlayBuffer = 0;
    env->DeleteGlobalRef(_javaRecBuffer);
    _javaRecBuffer = 0;

    
    
    env->DeleteGlobalRef(_javaScObj);
    _javaScObj = 0;
    _javaScClass = 0;

    
    if (isAttached)
    {
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "%s: Could not detach thread from JVM", __FUNCTION__);
        }
    }

    _initialized = false;

    return 0;
}





bool AudioDeviceAndroidJni::Initialized() const
{

    return (_initialized);
}





WebRtc_Word32 AudioDeviceAndroidJni::SpeakerIsAvailable(bool& available)
{

    
    available = true;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::InitSpeaker()
{

    CriticalSectionScoped lock(&_critSect);

    if (_playing)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Playout already started");
        return -1;
    }

    if (!_playoutDeviceIsSpecified)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Playout device is not specified");
        return -1;
    }

    
    
    _speakerIsInitialized = true;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::MicrophoneIsAvailable(bool& available)
{

    
    available = true;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::InitMicrophone()
{

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Recording already started");
        return -1;
    }

    if (!_recordingDeviceIsSpecified)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording device is not specified");
        return -1;
    }

    
    
    _micIsInitialized = true;

    return 0;
}





bool AudioDeviceAndroidJni::SpeakerIsInitialized() const
{

    return _speakerIsInitialized;
}





bool AudioDeviceAndroidJni::MicrophoneIsInitialized() const
{

    return _micIsInitialized;
}





WebRtc_Word32 AudioDeviceAndroidJni::SpeakerVolumeIsAvailable(bool& available)
{

    available = true; 

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetSpeakerVolume(WebRtc_UWord32 volume)
{

    if (!_speakerIsInitialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Speaker not initialized");
        return -1;
    }
    if (!globalContext)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Context is not set");
        return -1;
    }

    
    JNIEnv *env;
    bool isAttached = false;

    if (_javaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        
        
        jint res = _javaVM->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not attach thread to JVM (%d, %p)", res, env);
            return -1;
        }
        isAttached = true;
    }

    
    jmethodID setPlayoutVolumeID = env->GetMethodID(_javaScClass,
                                                    "SetPlayoutVolume", "(I)I");

    
    jint res = env->CallIntMethod(_javaScObj, setPlayoutVolumeID,
                                  static_cast<int> (volume));
    if (res < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "SetPlayoutVolume failed (%d)", res);
        return -1;
    }

    
    if (isAttached)
    {
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  Could not detach thread from JVM");
        }
    }

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SpeakerVolume(WebRtc_UWord32& volume) const
{

    if (!_speakerIsInitialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Speaker not initialized");
        return -1;
    }
    if (!globalContext)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Context is not set");
        return -1;
    }

    
    JNIEnv *env;
    bool isAttached = false;

    if (_javaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        
        
        jint res = _javaVM->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not attach thread to JVM (%d, %p)", res, env);
            return -1;
        }
        isAttached = true;
    }

    
    jmethodID getPlayoutVolumeID = env->GetMethodID(_javaScClass,
                                                    "GetPlayoutVolume", "()I");

    
    jint level = env->CallIntMethod(_javaScObj, getPlayoutVolumeID);
    if (level < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "GetPlayoutVolume failed (%d)", level);
        return -1;
    }

    
    if (isAttached)
    {
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  Could not detach thread from JVM");
        }
    }

    volume = static_cast<WebRtc_UWord32> (level);

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetWaveOutVolume(
    WebRtc_UWord16 ,
    WebRtc_UWord16 )
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::WaveOutVolume(
    WebRtc_UWord16& ,
    WebRtc_UWord16& ) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::MaxSpeakerVolume(
        WebRtc_UWord32& maxVolume) const
{

    if (!_speakerIsInitialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Speaker not initialized");
        return -1;
    }

    maxVolume = _maxSpeakerVolume;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::MinSpeakerVolume(
        WebRtc_UWord32& minVolume) const
{

    if (!_speakerIsInitialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Speaker not initialized");
        return -1;
    }

    minVolume = 0;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SpeakerVolumeStepSize(
        WebRtc_UWord16& stepSize) const
{

    if (!_speakerIsInitialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Speaker not initialized");
        return -1;
    }

    stepSize = 1;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SpeakerMuteIsAvailable(bool& available)
{

    available = false; 

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetSpeakerMute(bool )
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::SpeakerMute(bool& ) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::MicrophoneMuteIsAvailable(bool& available)
{

    available = false; 

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetMicrophoneMute(bool )
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::MicrophoneMute(bool& ) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::MicrophoneBoostIsAvailable(bool& available)
{

    available = false; 

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetMicrophoneBoost(bool enable)
{

    if (!_micIsInitialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Microphone not initialized");
        return -1;
    }

    if (enable)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Enabling not available");
        return -1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::MicrophoneBoost(bool& enabled) const
{

    if (!_micIsInitialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Microphone not initialized");
        return -1;
    }

    enabled = false;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::StereoRecordingIsAvailable(bool& available)
{

    available = false; 

    return 0;
}







WebRtc_Word32 AudioDeviceAndroidJni::SetStereoRecording(bool enable)
{

    if (enable)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Enabling not available");
        return -1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::StereoRecording(bool& enabled) const
{

    enabled = false;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::StereoPlayoutIsAvailable(bool& available)
{

    available = false; 

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetStereoPlayout(bool enable)
{

    if (enable)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Enabling not available");
        return -1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::StereoPlayout(bool& enabled) const
{

    enabled = false;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetAGC(bool enable)
{

    _AGC = enable;

    return 0;
}





bool AudioDeviceAndroidJni::AGC() const
{

    return _AGC;
}





WebRtc_Word32 AudioDeviceAndroidJni::MicrophoneVolumeIsAvailable(
        bool& available)
{

    available = false; 

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetMicrophoneVolume(
        WebRtc_UWord32 )
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::MicrophoneVolume(
        WebRtc_UWord32& ) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::MaxMicrophoneVolume(
        WebRtc_UWord32& ) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::MinMicrophoneVolume(
        WebRtc_UWord32& ) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::MicrophoneVolumeStepSize(
        WebRtc_UWord16& ) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word16 AudioDeviceAndroidJni::PlayoutDevices()
{

    
    return 1;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetPlayoutDevice(WebRtc_UWord16 index)
{

    if (_playIsInitialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Playout already initialized");
        return -1;
    }

    if (0 != index)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Device index is out of range [0,0]");
        return -1;
    }

    
    
    _playoutDeviceIsSpecified = true;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetPlayoutDevice(
        AudioDeviceModule::WindowsDeviceType )
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::PlayoutDeviceName(
        WebRtc_UWord16 index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize])
{

    if (0 != index)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Device index is out of range [0,0]");
        return -1;
    }

    
    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::RecordingDeviceName(
        WebRtc_UWord16 index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize])
{

    if (0 != index)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Device index is out of range [0,0]");
        return -1;
    }

    
    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return 0;
}





WebRtc_Word16 AudioDeviceAndroidJni::RecordingDevices()
{

    
    return 1;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetRecordingDevice(WebRtc_UWord16 index)
{

    if (_recIsInitialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording already initialized");
        return -1;
    }

    
    
    _recAudioSource = index;
    _recordingDeviceIsSpecified = true;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetRecordingDevice(
        AudioDeviceModule::WindowsDeviceType )
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::PlayoutIsAvailable(bool& available)
{

    available = false;

    
    WebRtc_Word32 res = InitPlayout();

    
    StopPlayout();

    if (res != -1)
    {
        available = true;
    }

    return res;
}





WebRtc_Word32 AudioDeviceAndroidJni::RecordingIsAvailable(bool& available)
{

    available = false;

    
    WebRtc_Word32 res = InitRecording();

    
    StopRecording();

    if (res != -1)
    {
        available = true;
    }

    return res;
}





WebRtc_Word32 AudioDeviceAndroidJni::InitPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Not initialized");
        return -1;
    }

    if (_playing)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Playout already started");
        return -1;
    }

    if (!_playoutDeviceIsSpecified)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Playout device is not specified");
        return -1;
    }

    if (_playIsInitialized)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Playout already initialized");
        return 0;
    }

    
    if (InitSpeaker() == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  InitSpeaker() failed");
    }

    
    JNIEnv *env;
    bool isAttached = false;

    
    if (_javaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "attaching");

        
        
        jint res = _javaVM->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not attach thread to JVM (%d, %p)", res, env);
            return -1;
        }
        isAttached = true;
    }

    
    jmethodID initPlaybackID = env->GetMethodID(_javaScClass, "InitPlayback",
                                                "(I)I");

    int samplingFreq = 44100;
    if (_samplingFreqOut != 44)
    {
        samplingFreq = _samplingFreqOut * 1000;
    }

    int retVal = -1;

    
    jint res = env->CallIntMethod(_javaScObj, initPlaybackID, samplingFreq);
    if (res < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "InitPlayback failed (%d)", res);
    }
    else
    {
        
        _ptrAudioBuffer->SetPlayoutSampleRate(_samplingFreqOut * 1000);
        _playIsInitialized = true;
        retVal = 0;
    }

    
    if (isAttached)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "detaching");
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  Could not detach thread from JVM");
        }
    }

    return retVal;
}





WebRtc_Word32 AudioDeviceAndroidJni::InitRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Not initialized");
        return -1;
    }

    if (_recording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Recording already started");
        return -1;
    }

    if (!_recordingDeviceIsSpecified)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording device is not specified");
        return -1;
    }

    if (_recIsInitialized)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Recording already initialized");
        return 0;
    }

    
    if (InitMicrophone() == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  InitMicrophone() failed");
    }

    
    JNIEnv *env;
    bool isAttached = false;

    
    if (_javaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        
        
        jint res = _javaVM->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not attach thread to JVM (%d, %p)", res, env);
            return -1;
        }
        isAttached = true;
    }

    
    jmethodID initRecordingID = env->GetMethodID(_javaScClass, "InitRecording",
                                                 "(II)I");

    int samplingFreq = 44100;
    if (_samplingFreqIn != 44)
    {
        samplingFreq = _samplingFreqIn * 1000;
    }

    int retVal = -1;

    
    jint res = env->CallIntMethod(_javaScObj, initRecordingID, _recAudioSource,
                                  samplingFreq);
    if (res < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "InitRecording failed (%d)", res);
    }
    else
    {
        
        _ptrAudioBuffer->SetRecordingSampleRate(_samplingFreqIn * 1000);

        
        _delayRecording = res / _samplingFreqIn;

        _recIsInitialized = true;
        retVal = 0;
    }

    
    if (isAttached)
    {
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  Could not detach thread from JVM");
        }
    }

    return retVal;
}





WebRtc_Word32 AudioDeviceAndroidJni::StartRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_recIsInitialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording not initialized");
        return -1;
    }

    if (_recording)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Recording already started");
        return 0;
    }

    
    JNIEnv *env;
    bool isAttached = false;

    
    if (_javaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        
        
        jint res = _javaVM->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not attach thread to JVM (%d, %p)", res, env);
            return -1;
        }
        isAttached = true;
    }

    
    jmethodID startRecordingID = env->GetMethodID(_javaScClass,
                                                  "StartRecording", "()I");

    
    jint res = env->CallIntMethod(_javaScObj, startRecordingID);
    if (res < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "StartRecording failed (%d)", res);
        return -1;
    }

    _recWarning = 0;
    _recError = 0;

    
    _startRec = true;
    _timeEventRec.Set(); 
    _critSect.Leave();
    
    if (kEventSignaled != _recStartStopEvent.Wait(5000))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Timeout or error starting");
    }
    _recStartStopEvent.Reset();
    _critSect.Enter();

    
    if (isAttached)
    {
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  Could not detach thread from JVM");
        }
    }

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::StopRecording()

{

    CriticalSectionScoped lock(&_critSect);

    if (!_recIsInitialized)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Recording is not initialized");
        return 0;
    }

    
    
    _startRec = false;

    
    JNIEnv *env;
    bool isAttached = false;

    
    if (_javaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        
        
        jint res = _javaVM->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not attach thread to JVM (%d, %p)", res, env);
            return -1;
        }
        isAttached = true;
    }

    
    jmethodID stopRecordingID = env->GetMethodID(_javaScClass, "StopRecording",
                                                 "()I");

    
    jint res = env->CallIntMethod(_javaScObj, stopRecordingID);
    if (res < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "StopRecording failed (%d)", res);
    }

    _recIsInitialized = false;
    _recording = false;
    _recWarning = 0;
    _recError = 0;

    
    if (isAttached)
    {
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  Could not detach thread from JVM");
        }
    }

    return 0;
}





bool AudioDeviceAndroidJni::RecordingIsInitialized() const
{

    return _recIsInitialized;
}





bool AudioDeviceAndroidJni::Recording() const
{

    return _recording;
}





bool AudioDeviceAndroidJni::PlayoutIsInitialized() const
{

    return _playIsInitialized;
}





WebRtc_Word32 AudioDeviceAndroidJni::StartPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Playout not initialized");
        return -1;
    }

    if (_playing)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Playout already started");
        return 0;
    }

    
    JNIEnv *env;
    bool isAttached = false;

    
    if (_javaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        
        
        jint res = _javaVM->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not attach thread to JVM (%d, %p)", res, env);
            return -1;
        }
        isAttached = true;
    }

    
    jmethodID startPlaybackID = env->GetMethodID(_javaScClass, "StartPlayback",
                                                 "()I");

    
    jint res = env->CallIntMethod(_javaScObj, startPlaybackID);
    if (res < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "StartPlayback failed (%d)", res);
        return -1;
    }

    _playWarning = 0;
    _playError = 0;

    
    _startPlay = true;
    _timeEventPlay.Set(); 
    _critSect.Leave();
    
    if (kEventSignaled != _playStartStopEvent.Wait(5000))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Timeout or error starting");
    }
    _playStartStopEvent.Reset();
    _critSect.Enter();

    
    if (isAttached)
    {
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  Could not detach thread from JVM");
        }
    }

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::StopPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Playout is not initialized");
        return 0;
    }

    
    JNIEnv *env;
    bool isAttached = false;

    
    if (_javaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        
        
        jint res = _javaVM->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not attach thread to JVM (%d, %p)", res, env);
            return -1;
        }
        isAttached = true;
    }

    
    jmethodID stopPlaybackID = env->GetMethodID(_javaScClass, "StopPlayback",
                                                "()I");

    
    jint res = env->CallIntMethod(_javaScObj, stopPlaybackID);
    if (res < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "StopPlayback failed (%d)", res);
    }

    _playIsInitialized = false;
    _playing = false;
    _playWarning = 0;
    _playError = 0;

    
    if (isAttached)
    {
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  Could not detach thread from JVM");
        }
    }

    return 0;
}







WebRtc_Word32 AudioDeviceAndroidJni::PlayoutDelay(WebRtc_UWord16& delayMS) const
{
    delayMS = _delayPlayout;

    return 0;
}







WebRtc_Word32 AudioDeviceAndroidJni::RecordingDelay(
        WebRtc_UWord16& delayMS) const
{
    delayMS = _delayRecording;

    return 0;
}





bool AudioDeviceAndroidJni::Playing() const
{

    return _playing;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetPlayoutBuffer(
        const AudioDeviceModule::BufferType ,
        WebRtc_UWord16 )
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidJni::PlayoutBuffer(
        AudioDeviceModule::BufferType& type,
        WebRtc_UWord16& sizeMS) const
{

    type = AudioDeviceModule::kAdaptiveBufferSize;
    sizeMS = _delayPlayout; 

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::CPULoad(WebRtc_UWord16& ) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





bool AudioDeviceAndroidJni::PlayoutWarning() const
{
    return (_playWarning > 0);
}





bool AudioDeviceAndroidJni::PlayoutError() const
{
    return (_playError > 0);
}





bool AudioDeviceAndroidJni::RecordingWarning() const
{
    return (_recWarning > 0);
}





bool AudioDeviceAndroidJni::RecordingError() const
{
    return (_recError > 0);
}





void AudioDeviceAndroidJni::ClearPlayoutWarning()
{
    _playWarning = 0;
}





void AudioDeviceAndroidJni::ClearPlayoutError()
{
    _playError = 0;
}





void AudioDeviceAndroidJni::ClearRecordingWarning()
{
    _recWarning = 0;
}





void AudioDeviceAndroidJni::ClearRecordingError()
{
    _recError = 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetRecordingSampleRate(
        const WebRtc_UWord32 samplesPerSec)
{

    if (samplesPerSec > 48000 || samplesPerSec < 8000)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Invalid sample rate");
        return -1;
    }

    
    if (samplesPerSec == 44100)
    {
        _samplingFreqIn = 44;
    }
    else
    {
        _samplingFreqIn = samplesPerSec / 1000;
    }

    
    _ptrAudioBuffer->SetRecordingSampleRate(samplesPerSec);

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetPlayoutSampleRate(
        const WebRtc_UWord32 samplesPerSec)
{

    if (samplesPerSec > 48000 || samplesPerSec < 8000)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Invalid sample rate");
        return -1;
    }

    
    if (samplesPerSec == 44100)
    {
        _samplingFreqOut = 44;
    }
    else
    {
        _samplingFreqOut = samplesPerSec / 1000;
    }

    
    _ptrAudioBuffer->SetPlayoutSampleRate(samplesPerSec);

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::SetLoudspeakerStatus(bool enable)
{

    if (!globalContext)
    {
        WEBRTC_TRACE(kTraceError, kTraceUtility, -1,
                     "  Context is not set");
        return -1;
    }

    
    JNIEnv *env;
    bool isAttached = false;

    if (_javaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        
        
        jint res = _javaVM->AttachCurrentThread(&env, NULL);

        
        if ((res < 0) || !env)
        {
            WEBRTC_TRACE(kTraceError, kTraceUtility, -1,
                         "  Could not attach thread to JVM (%d, %p)", res, env);
            return -1;
        }
        isAttached = true;
    }

    
    jmethodID setPlayoutSpeakerID = env->GetMethodID(_javaScClass,
                                                     "SetPlayoutSpeaker",
                                                     "(Z)I");

    
    jint res = env->CallIntMethod(_javaScObj, setPlayoutSpeakerID, enable);
    if (res < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceUtility, -1,
                     "  SetPlayoutSpeaker failed (%d)", res);
        return -1;
    }

    _loudSpeakerOn = enable;

    
    if (isAttached)
    {
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceUtility, -1,
                         "  Could not detach thread from JVM");
        }
    }

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidJni::GetLoudspeakerStatus(bool& enabled) const
{

    enabled = _loudSpeakerOn;

    return 0;
}













WebRtc_Word32 AudioDeviceAndroidJni::InitJavaResources()
{
    
    _javaVM = globalJvm;
    _javaScClass = globalScClass;

    
    if (!_javaVM)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: Not a valid Java VM pointer", __FUNCTION__);
        return -1;
    }

    
    JNIEnv *env;
    bool isAttached = false;

    
    if (_javaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        
        
        jint res = _javaVM->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "%s: Could not attach thread to JVM (%d, %p)",
                         __FUNCTION__, res, env);
            return -1;
        }
        isAttached = true;
    }

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "get method id");

    
    jmethodID cid = env->GetMethodID(_javaScClass, "<init>", "()V");
    if (cid == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not get constructor ID", __FUNCTION__);
        return -1; 
    }

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "construct object", __FUNCTION__);

    
    jobject javaScObjLocal = env->NewObject(_javaScClass, cid);
    if (!javaScObjLocal)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "%s: could not create Java sc object", __FUNCTION__);
        return -1;
    }

    
    
    _javaScObj = env->NewGlobalRef(javaScObjLocal);
    if (!_javaScObj)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not create Java sc object reference",
                     __FUNCTION__);
        return -1;
    }

    
    env->DeleteLocalRef(javaScObjLocal);

    
    

    
    if (globalContext) {
      jfieldID context_id = env->GetFieldID(globalScClass,
                                            "_context",
                                            "Landroid/content/Context;");
      if (!context_id) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not get _context id", __FUNCTION__);
        return -1;
      }

      env->SetObjectField(_javaScObj, context_id, globalContext);
      jobject javaContext = env->GetObjectField(_javaScObj, context_id);
      if (!javaContext) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not set or get _context", __FUNCTION__);
        return -1;
      }
    }
    else {
      WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                   "%s: did not set Context - some functionality is not "
                   "supported",
                   __FUNCTION__);
    }

    
    

    
    jfieldID fidPlayBuffer = env->GetFieldID(_javaScClass, "_playBuffer",
                                             "Ljava/nio/ByteBuffer;");
    if (!fidPlayBuffer)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not get play buffer fid", __FUNCTION__);
        return -1;
    }

    
    jobject javaPlayBufferLocal =
            env->GetObjectField(_javaScObj, fidPlayBuffer);
    if (!javaPlayBufferLocal)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not get play buffer", __FUNCTION__);
        return -1;
    }

    
    
    
    _javaPlayBuffer = env->NewGlobalRef(javaPlayBufferLocal);
    if (!_javaPlayBuffer)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not get play buffer reference", __FUNCTION__);
        return -1;
    }

    
    env->DeleteLocalRef(javaPlayBufferLocal);

    
    _javaDirectPlayBuffer = env->GetDirectBufferAddress(_javaPlayBuffer);
    if (!_javaDirectPlayBuffer)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not get direct play buffer", __FUNCTION__);
        return -1;
    }

    
    _javaMidPlayAudio = env->GetMethodID(_javaScClass, "PlayAudio", "(I)I");
    if (!_javaMidPlayAudio)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not get play audio mid", __FUNCTION__);
        return -1;
    }

    
    

    
    jfieldID fidRecBuffer = env->GetFieldID(_javaScClass, "_recBuffer",
                                            "Ljava/nio/ByteBuffer;");
    if (!fidRecBuffer)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not get rec buffer fid", __FUNCTION__);
        return -1;
    }

    
    jobject javaRecBufferLocal = env->GetObjectField(_javaScObj, fidRecBuffer);
    if (!javaRecBufferLocal)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not get rec buffer", __FUNCTION__);
        return -1;
    }

    
    
    
    _javaRecBuffer = env->NewGlobalRef(javaRecBufferLocal);
    if (!_javaRecBuffer)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not get rec buffer reference", __FUNCTION__);
        return -1;
    }

    
    env->DeleteLocalRef(javaRecBufferLocal);

    
    _javaDirectRecBuffer = env->GetDirectBufferAddress(_javaRecBuffer);
    if (!_javaDirectRecBuffer)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not get direct rec buffer", __FUNCTION__);
        return -1;
    }

    
    _javaMidRecAudio = env->GetMethodID(_javaScClass, "RecordAudio", "(I)I");
    if (!_javaMidRecAudio)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: could not get rec audio mid", __FUNCTION__);
        return -1;
    }

    
    if (isAttached)
    {
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "%s: Could not detach thread from JVM", __FUNCTION__);
        }
    }

    return 0;
}









WebRtc_Word32 AudioDeviceAndroidJni::InitSampleRate()
{
    int samplingFreq = 44100;
    jint res = 0;

    
    JNIEnv *env;
    bool isAttached = false;

    
    if (_javaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        
        
        jint res = _javaVM->AttachCurrentThread(&env, NULL);
        if ((res < 0) || !env)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "%s: Could not attach thread to JVM (%d, %p)",
                         __FUNCTION__, res, env);
            return -1;
        }
        isAttached = true;
    }

    if (_samplingFreqIn > 0)
    {
        
        samplingFreq = 44100;
        if (_samplingFreqIn != 44)
        {
            samplingFreq = _samplingFreqIn * 1000;
        }
        WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id,
                     "  Trying configured recording sampling rate %d",
                     samplingFreq);
    }

    
    jmethodID initRecordingID = env->GetMethodID(_javaScClass, "InitRecording",
                                                 "(II)I");

    bool keepTrying = true;
    while (keepTrying)
    {
        
        res = env->CallIntMethod(_javaScObj, initRecordingID, _recAudioSource,
                                 samplingFreq);
        if (res < 0)
        {
            switch (samplingFreq)
            {
                case 44100:
                    samplingFreq = 16000;
                    break;
                case 16000:
                    samplingFreq = 8000;
                    break;
                default: 
                    WEBRTC_TRACE(kTraceError,
                                 kTraceAudioDevice, _id,
                                 "%s: InitRecording failed (%d)", __FUNCTION__,
                                 res);
                    return -1;
            }
        }
        else
        {
            keepTrying = false;
        }
    }

    
    if (samplingFreq == 44100)
    {
        _samplingFreqIn = 44;
    }
    else
    {
        _samplingFreqIn = samplingFreq / 1000;
    }

    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id,
                 "Recording sample rate set to (%d)", _samplingFreqIn);

    
    jmethodID stopRecordingID = env->GetMethodID(_javaScClass, "StopRecording",
                                                 "()I");

    
    res = env->CallIntMethod(_javaScObj, stopRecordingID);
    if (res < 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "StopRecording failed (%d)", res);
    }

    
    jmethodID initPlaybackID = env->GetMethodID(_javaScClass, "InitPlayback",
                                                "(I)I");

    if (_samplingFreqOut > 0)
    {
        
        samplingFreq = 44100;
        if (_samplingFreqOut != 44)
        {
            samplingFreq = _samplingFreqOut * 1000;
        }
        WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id,
                     "  Trying configured playback sampling rate %d",
                     samplingFreq);
    }
    else
    {
        
        if (samplingFreq == 8000)
        {
            
            samplingFreq = 16000;
        }
        
    }

    keepTrying = true;
    while (keepTrying)
    {
        
        res = env->CallIntMethod(_javaScObj, initPlaybackID, samplingFreq);
        if (res < 0)
        {
            switch (samplingFreq)
            {
                case 44100:
                    samplingFreq = 16000;
                    break;
                case 16000:
                    samplingFreq = 8000;
                    break;
                default: 
                    WEBRTC_TRACE(kTraceError,
                                 kTraceAudioDevice, _id,
                                 "InitPlayback failed (%d)", res);
                    return -1;
            }
        }
        else
        {
            keepTrying = false;
        }
    }

    
    _maxSpeakerVolume = static_cast<WebRtc_UWord32> (res);
    if (_maxSpeakerVolume < 1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Did not get valid max speaker volume value (%d)",
                     _maxSpeakerVolume);
    }

    
    if (samplingFreq == 44100)
    {
        _samplingFreqOut = 44;
    }
    else
    {
        _samplingFreqOut = samplingFreq / 1000;
    }

    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id,
                 "Playback sample rate set to (%d)", _samplingFreqOut);

    
    jmethodID stopPlaybackID = env->GetMethodID(_javaScClass, "StopPlayback",
                                                "()I");

    
    res = env->CallIntMethod(_javaScObj, stopPlaybackID);
    if (res < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "StopPlayback failed (%d)", res);
    }

    
    if (isAttached)
    {
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "%s: Could not detach thread from JVM", __FUNCTION__);
        }
    }

    return 0;
}









bool AudioDeviceAndroidJni::PlayThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceAndroidJni*> (pThis)->PlayThreadProcess());
}





bool AudioDeviceAndroidJni::RecThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceAndroidJni*> (pThis)->RecThreadProcess());
}





bool AudioDeviceAndroidJni::PlayThreadProcess()
{
    if (!_playThreadIsInitialized)
    {
        

        
        jint res = _javaVM->AttachCurrentThread(&_jniEnvPlay, NULL);
        if ((res < 0) || !_jniEnvPlay)
        {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice,
                         _id,
                         "Could not attach playout thread to JVM (%d, %p)",
                         res, _jniEnvPlay);
            return false; 
        }

        _playThreadIsInitialized = true;
    }

    if (!_playing)
    {
        switch (_timeEventPlay.Wait(1000))
        {
            case kEventSignaled:
                WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice,
                             _id, "Playout thread event signal");
                _timeEventPlay.Reset();
                break;
            case kEventError:
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                             _id, "Playout thread event error");
                return true;
            case kEventTimeout:
                WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice,
                             _id, "Playout thread event timeout");
                return true;
        }
    }

    Lock();

    if (_startPlay)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "_startPlay true, performing initial actions");
        _startPlay = false;
        _playing = true;
        _playWarning = 0;
        _playError = 0;
        _playStartStopEvent.Set();
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "Sent signal");
    }

    if (_playing)
    {
        WebRtc_Word8 playBuffer[2 * 480]; 
        WebRtc_UWord32 samplesToPlay = _samplingFreqOut * 10;

        
        
        
        UnLock();
        WebRtc_UWord32 nSamples =
                _ptrAudioBuffer->RequestPlayoutData(samplesToPlay);
        Lock();

        
        if (!_playing)
        {
            UnLock();
            return true;
        }

        nSamples = _ptrAudioBuffer->GetPlayoutData(playBuffer);
        if (nSamples != samplesToPlay)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  invalid number of output samples(%d)", nSamples);
            _playWarning = 1;
        }

        
        
        memcpy(_javaDirectPlayBuffer, playBuffer, nSamples * 2);

        UnLock();

        
        
        
        jint res = _jniEnvPlay->CallIntMethod(_javaScObj, _javaMidPlayAudio,
                                              2 * nSamples);
        if (res < 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "PlayAudio failed (%d)", res);
            _playWarning = 1;
        }
        else if (res > 0)
        {
            
            _delayPlayout = res / _samplingFreqOut;
        }
        
        

        Lock();

    } 

    if (_shutdownPlayThread)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "Detaching thread from Java VM");

        
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice,
                         _id, "Could not detach playout thread from JVM");
            _shutdownPlayThread = false;
            
            
        }
        else
        {
            _jniEnvPlay = NULL;
            _shutdownPlayThread = false;
            _playStartStopEvent.Set(); 
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "Sent signal");
        }
    }

    UnLock();
    return true;
}





bool AudioDeviceAndroidJni::RecThreadProcess()
{
    if (!_recThreadIsInitialized)
    {
        

        
        jint res = _javaVM->AttachCurrentThread(&_jniEnvRec, NULL);

        
        if ((res < 0) || !_jniEnvRec)
        {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice,
                         _id, "Could not attach rec thread to JVM (%d, %p)",
                         res, _jniEnvRec);
            return false; 
        }

        _recThreadIsInitialized = true;
    }

    
    if (!_recording)
    {
        switch (_timeEventRec.Wait(1000))
        {
            case kEventSignaled:
                WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice,
                             _id, "Recording thread event signal");
                _timeEventRec.Reset();
                break;
            case kEventError:
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                             _id, "Recording thread event error");
                return true;
            case kEventTimeout:
                WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice,
                             _id, "Recording thread event timeout");
                return true;
        }
    }

    Lock();

    if (_startRec)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "_startRec true, performing initial actions");
        _startRec = false;
        _recording = true;
        _recWarning = 0;
        _recError = 0;
        _recStartStopEvent.Set();
    }

    if (_recording)
    {
        WebRtc_UWord32 samplesToRec = _samplingFreqIn * 10;

        
        
        
        UnLock();
        jint playDelayInSamples = _jniEnvRec->CallIntMethod(_javaScObj,
                                                            _javaMidRecAudio,
                                                            2 * samplesToRec);
        if (playDelayInSamples < 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "RecordAudio failed");
            _recWarning = 1;
        }
        else
        {
            _delayPlayout = playDelayInSamples / _samplingFreqOut;
        }
        Lock();

        
        if (_recording)
        {



            
            
            
            memcpy(_recBuffer, _javaDirectRecBuffer, 2 * samplesToRec);

            
            
            _ptrAudioBuffer->SetRecordedBuffer(_recBuffer, samplesToRec);

            
            _ptrAudioBuffer->SetVQEData(_delayPlayout, _delayRecording, 0);

            
            
            UnLock();
            _ptrAudioBuffer->DeliverRecordedData();
            Lock();
        }

    } 

    if (_shutdownRecThread)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "Detaching rec thread from Java VM");

        
        if (_javaVM->DetachCurrentThread() < 0)
        {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice,
                         _id, "Could not detach recording thread from JVM");
            _shutdownRecThread = false;
            
            
        }
        else
        {
            _jniEnvRec = NULL;
            _shutdownRecThread = false;
            _recStartStopEvent.Set(); 

            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "Sent signal rec");
        }
    }

    UnLock();
    return true;
}

} 

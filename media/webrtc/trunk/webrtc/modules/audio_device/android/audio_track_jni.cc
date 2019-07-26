
















#include "webrtc/modules/audio_device/android/audio_track_jni.h"

#include <android/log.h>
#include <stdlib.h>

#include "webrtc/modules/audio_device/audio_device_config.h"
#include "webrtc/modules/audio_device/audio_device_utility.h"

#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

JavaVM* AudioTrackJni::globalJvm = NULL;
JNIEnv* AudioTrackJni::globalJNIEnv = NULL;
jobject AudioTrackJni::globalContext = NULL;
jclass AudioTrackJni::globalScClass = NULL;

int32_t AudioTrackJni::SetAndroidAudioDeviceObjects(void* javaVM, void* env,
                                                    void* context) {
  assert(env);
  globalJvm = reinterpret_cast<JavaVM*>(javaVM);
  globalJNIEnv = reinterpret_cast<JNIEnv*>(env);
  
  jclass javaScClassLocal = globalJNIEnv->FindClass(
      "org/webrtc/voiceengine/WebRtcAudioTrack");
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
  return 0;
}

void AudioTrackJni::ClearAndroidAudioDeviceObjects() {
  WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, -1,
               "%s: env is NULL, assuming deinit", __FUNCTION__);

  globalJvm = NULL;
  if (!globalJNIEnv) {
    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, -1,
                 "%s: saved env already NULL", __FUNCTION__);
    return;
  }

  globalJNIEnv->DeleteGlobalRef(globalContext);
  globalContext = reinterpret_cast<jobject>(NULL);

  globalJNIEnv->DeleteGlobalRef(globalScClass);
  globalScClass = reinterpret_cast<jclass>(NULL);

  globalJNIEnv = reinterpret_cast<JNIEnv*>(NULL);
}

AudioTrackJni::AudioTrackJni(const int32_t id)
    : _javaVM(NULL),
      _jniEnvPlay(NULL),
      _javaScClass(0),
      _javaScObj(0),
      _javaPlayBuffer(0),
      _javaDirectPlayBuffer(NULL),
      _javaMidPlayAudio(0),
      _ptrAudioBuffer(NULL),
      _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
      _id(id),
      _initialized(false),
      _timeEventPlay(*EventWrapper::Create()),
      _playStartStopEvent(*EventWrapper::Create()),
      _ptrThreadPlay(NULL),
      _playThreadID(0),
      _playThreadIsInitialized(false),
      _shutdownPlayThread(false),
      _playoutDeviceIsSpecified(false),
      _playing(false),
      _playIsInitialized(false),
      _speakerIsInitialized(false),
      _startPlay(false),
      _playWarning(0),
      _playError(0),
      _delayPlayout(0),
      _samplingFreqOut((N_PLAY_SAMPLES_PER_SEC/1000)),
      _maxSpeakerVolume(0) {
}

AudioTrackJni::~AudioTrackJni() {
  WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
               "%s destroyed", __FUNCTION__);

  Terminate();

  delete &_playStartStopEvent;
  delete &_timeEventPlay;
  delete &_critSect;
}

int32_t AudioTrackJni::Init() {
  CriticalSectionScoped lock(&_critSect);
  if (_initialized)
  {
    return 0;
  }

  _playWarning = 0;
  _playError = 0;

  
  
  
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

  const char* threadName = "jni_audio_render_thread";
  _ptrThreadPlay = ThreadWrapper::CreateThread(PlayThreadFunc, this,
                                               kRealtimePriority, threadName);
  if (_ptrThreadPlay == NULL)
  {
    WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                 "  failed to create the play audio thread");
    return -1;
  }

  unsigned int threadID = 0;
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

int32_t AudioTrackJni::Terminate() {
  CriticalSectionScoped lock(&_critSect);
  if (!_initialized)
  {
    return 0;
  }

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
  _javaDirectPlayBuffer = NULL;

  
  
  env->DeleteGlobalRef(_javaPlayBuffer);
  _javaPlayBuffer = 0;

  
  
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

int32_t AudioTrackJni::PlayoutDeviceName(uint16_t index,
                                         char name[kAdmMaxDeviceNameSize],
                                         char guid[kAdmMaxGuidSize]) {
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

int32_t AudioTrackJni::SetPlayoutDevice(uint16_t index) {
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

int32_t AudioTrackJni::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType device) {
  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
  return -1;
}


int32_t AudioTrackJni::PlayoutIsAvailable(bool& available) {  
  available = false;

  
  int32_t res = InitPlayout();

  
  StopPlayout();

    if (res != -1)
    {
      available = true;
    }

    return res;
}

int32_t AudioTrackJni::InitPlayout() {
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

int32_t AudioTrackJni::StartPlayout() {
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

int32_t AudioTrackJni::StopPlayout() {
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

int32_t AudioTrackJni::SpeakerIsAvailable(bool& available) {  
  
  available = true;
  return 0;
}

int32_t AudioTrackJni::InitSpeaker() {
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

int32_t AudioTrackJni::SpeakerVolumeIsAvailable(bool& available) {  
  available = true; 
  return 0;
}

int32_t AudioTrackJni::SetSpeakerVolume(uint32_t volume) {
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

int32_t AudioTrackJni::SpeakerVolume(uint32_t& volume) const {  
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

  volume = static_cast<uint32_t> (level);

  return 0;
}


int32_t AudioTrackJni::MaxSpeakerVolume(uint32_t& maxVolume) const {  
  if (!_speakerIsInitialized)
  {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "  Speaker not initialized");
    return -1;
  }

  maxVolume = _maxSpeakerVolume;

  return 0;
}

int32_t AudioTrackJni::MinSpeakerVolume(uint32_t& minVolume) const {  
  if (!_speakerIsInitialized)
  {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "  Speaker not initialized");
    return -1;
  }
  minVolume = 0;
  return 0;
}

int32_t AudioTrackJni::SpeakerVolumeStepSize(
    uint16_t& stepSize) const {  
  if (!_speakerIsInitialized)
  {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "  Speaker not initialized");
    return -1;
  }

  stepSize = 1;

  return 0;
}

int32_t AudioTrackJni::SpeakerMuteIsAvailable(bool& available) {  
  available = false; 
  return 0;
}

int32_t AudioTrackJni::SetSpeakerMute(bool enable) {
  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
  return -1;
}

int32_t AudioTrackJni::SpeakerMute(bool& ) const {

  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
  return -1;
}

int32_t AudioTrackJni::StereoPlayoutIsAvailable(bool& available) {  
  available = false; 
  return 0;
}

int32_t AudioTrackJni::SetStereoPlayout(bool enable) {
  if (enable)
  {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "  Enabling not available");
    return -1;
  }

  return 0;
}

int32_t AudioTrackJni::StereoPlayout(bool& enabled) const {  
  enabled = false;
  return 0;
}

int32_t AudioTrackJni::SetPlayoutBuffer(
    const AudioDeviceModule::BufferType type,
    uint16_t sizeMS) {
  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
  return -1;
}


int32_t AudioTrackJni::PlayoutBuffer(
    AudioDeviceModule::BufferType& type,  
    uint16_t& sizeMS) const {  
  type = AudioDeviceModule::kAdaptiveBufferSize;
  sizeMS = _delayPlayout; 

    return 0;
}

int32_t AudioTrackJni::PlayoutDelay(uint16_t& delayMS) const {  
  delayMS = _delayPlayout;
  return 0;
}

void AudioTrackJni::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) {
  CriticalSectionScoped lock(&_critSect);
  _ptrAudioBuffer = audioBuffer;
  
  _ptrAudioBuffer->SetPlayoutSampleRate(N_PLAY_SAMPLES_PER_SEC);
  _ptrAudioBuffer->SetPlayoutChannels(N_PLAY_CHANNELS);
}

int32_t AudioTrackJni::SetPlayoutSampleRate(const uint32_t samplesPerSec) {
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

bool AudioTrackJni::PlayoutWarning() const {
  return (_playWarning > 0);
}

bool AudioTrackJni::PlayoutError() const {
  return (_playError > 0);
}

void AudioTrackJni::ClearPlayoutWarning() {
  _playWarning = 0;
}

void AudioTrackJni::ClearPlayoutError() {
  _playError = 0;
}

int32_t AudioTrackJni::SetLoudspeakerStatus(bool enable) {
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

int32_t AudioTrackJni::GetLoudspeakerStatus(bool& enabled) const {  
  enabled = _loudSpeakerOn;
  return 0;
}

int32_t AudioTrackJni::InitJavaResources() {
  
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

int32_t AudioTrackJni::InitSampleRate() {
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

  bool keepTrying = true;
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

  
  _maxSpeakerVolume = static_cast<uint32_t> (res);
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

bool AudioTrackJni::PlayThreadFunc(void* pThis)
{
  return (static_cast<AudioTrackJni*> (pThis)->PlayThreadProcess());
}

bool AudioTrackJni::PlayThreadProcess()
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
    int8_t playBuffer[2 * 480]; 
    uint32_t samplesToPlay = _samplingFreqOut * 10;

    
    
    
    UnLock();
    uint32_t nSamples =
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

}  

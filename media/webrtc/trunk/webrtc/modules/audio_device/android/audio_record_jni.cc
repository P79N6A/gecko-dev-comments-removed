
















#include "webrtc/modules/audio_device/android/audio_record_jni.h"

#include <android/log.h>
#include <stdlib.h>

#include "webrtc/modules/audio_device/android/audio_common.h"
#include "webrtc/modules/audio_device/audio_device_config.h"
#include "webrtc/modules/audio_device/audio_device_utility.h"

#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

JavaVM* AudioRecordJni::globalJvm = NULL;
JNIEnv* AudioRecordJni::globalJNIEnv = NULL;
jobject AudioRecordJni::globalContext = NULL;
jclass AudioRecordJni::globalScClass = NULL;

int32_t AudioRecordJni::SetAndroidAudioDeviceObjects(void* javaVM, void* env,
                                                     void* context) {
  assert(env);
  globalJvm = reinterpret_cast<JavaVM*>(javaVM);
  globalJNIEnv = reinterpret_cast<JNIEnv*>(env);
  
  jclass javaScClassLocal = globalJNIEnv->FindClass(
      "org/webrtc/voiceengine/WebRtcAudioRecord");
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

void AudioRecordJni::ClearAndroidAudioDeviceObjects() {
  WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, -1,
               "%s: env is NULL, assuming deinit", __FUNCTION__);

  globalJvm = NULL;;
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

AudioRecordJni::AudioRecordJni(
    const int32_t id, PlayoutDelayProvider* delay_provider)
    : _javaVM(NULL),
      _jniEnvRec(NULL),
      _javaScClass(0),
      _javaScObj(0),
      _javaRecBuffer(0),
      _javaDirectRecBuffer(NULL),
      _javaMidRecAudio(0),
      _ptrAudioBuffer(NULL),
      _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
      _id(id),
      _delay_provider(delay_provider),
      _initialized(false),
      _timeEventRec(*EventWrapper::Create()),
      _recStartStopEvent(*EventWrapper::Create()),
      _ptrThreadRec(NULL),
      _recThreadID(0),
      _recThreadIsInitialized(false),
      _shutdownRecThread(false),
      _recordingDeviceIsSpecified(false),
      _recording(false),
      _recIsInitialized(false),
      _micIsInitialized(false),
      _startRec(false),
      _recWarning(0),
      _recError(0),
      _delayRecording(0),
      _AGC(false),
      _samplingFreqIn((N_REC_SAMPLES_PER_SEC)),
      _recAudioSource(1) { 
  memset(_recBuffer, 0, sizeof(_recBuffer));
}

AudioRecordJni::~AudioRecordJni() {
  WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
               "%s destroyed", __FUNCTION__);

  Terminate();

  delete &_recStartStopEvent;
  delete &_timeEventRec;
  delete &_critSect;
}

int32_t AudioRecordJni::Init() {
  CriticalSectionScoped lock(&_critSect);

  if (_initialized)
  {
    return 0;
  }

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
  _initialized = true;

  return 0;
}

int32_t AudioRecordJni::Terminate() {
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

  
  _javaMidRecAudio = 0;
  _javaDirectRecBuffer = NULL;

  
  
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

int32_t AudioRecordJni::RecordingDeviceName(uint16_t index,
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

int32_t AudioRecordJni::SetRecordingDevice(uint16_t index) {
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

int32_t AudioRecordJni::SetRecordingDevice(
    AudioDeviceModule::WindowsDeviceType device) {
  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
  return -1;
}

int32_t AudioRecordJni::RecordingIsAvailable(bool& available) {  
  available = false;

  
  int32_t res = InitRecording();

  
  StopRecording();

  if (res != -1)
  {
    available = true;
  }

  return res;
}

int32_t AudioRecordJni::InitRecording() {
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

  int retVal = -1;

  
  jint res = env->CallIntMethod(_javaScObj, initRecordingID, _recAudioSource,
                                _samplingFreqIn);
  if (res < 0)
  {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "InitRecording failed (%d)", res);
  }
  else
  {
    
    _ptrAudioBuffer->SetRecordingSampleRate(_samplingFreqIn);

    
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

int32_t AudioRecordJni::StartRecording() {
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

int32_t AudioRecordJni::StopRecording() {
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

int32_t AudioRecordJni::SetAGC(bool enable) {
  _AGC = enable;
  return 0;
}

int32_t AudioRecordJni::MicrophoneIsAvailable(bool& available) {  
  
  available = true;
  return 0;
}

int32_t AudioRecordJni::InitMicrophone() {
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

int32_t AudioRecordJni::MicrophoneVolumeIsAvailable(
    bool& available) {  
  available = false;  
  return 0;
}

int32_t AudioRecordJni::SetMicrophoneVolume( uint32_t ) {

  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
  return -1;
}

int32_t AudioRecordJni::MicrophoneVolume(uint32_t& volume) const {  
  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
  return -1;
}

int32_t AudioRecordJni::MaxMicrophoneVolume(
    uint32_t& maxVolume) const {  
  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
  return -1;
}

int32_t AudioRecordJni::MinMicrophoneVolume(
    uint32_t& minVolume) const {  
  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
  return -1;
}

int32_t AudioRecordJni::MicrophoneVolumeStepSize(
    uint16_t& stepSize) const {
  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
  return -1;
}

int32_t AudioRecordJni::MicrophoneMuteIsAvailable(bool& available) {  
  available = false; 
  return 0;
}

int32_t AudioRecordJni::SetMicrophoneMute(bool enable) {
  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
  return -1;
}

int32_t AudioRecordJni::MicrophoneMute(bool& enabled) const {  
  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
  return -1;
}

int32_t AudioRecordJni::MicrophoneBoostIsAvailable(bool& available) {  
  available = false; 
  return 0;
}

int32_t AudioRecordJni::SetMicrophoneBoost(bool enable) {
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

int32_t AudioRecordJni::MicrophoneBoost(bool& enabled) const {  
  if (!_micIsInitialized)
  {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "  Microphone not initialized");
    return -1;
  }

  enabled = false;

  return 0;
}

int32_t AudioRecordJni::StereoRecordingIsAvailable(bool& available) {  
  available = false; 
  return 0;
}

int32_t AudioRecordJni::SetStereoRecording(bool enable) {
  if (enable)
  {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "  Enabling not available");
    return -1;
  }

  return 0;
}

int32_t AudioRecordJni::StereoRecording(bool& enabled) const {  
  enabled = false;
  return 0;
}

int32_t AudioRecordJni::RecordingDelay(uint16_t& delayMS) const {  
  delayMS = _delayRecording;
  return 0;
}

bool AudioRecordJni::RecordingWarning() const {
  return (_recWarning > 0);
}

bool AudioRecordJni::RecordingError() const {
  return (_recError > 0);
}

void AudioRecordJni::ClearRecordingWarning() {
  _recWarning = 0;
}

void AudioRecordJni::ClearRecordingError() {
  _recError = 0;
}

void AudioRecordJni::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) {
  CriticalSectionScoped lock(&_critSect);
  _ptrAudioBuffer = audioBuffer;
  
  _ptrAudioBuffer->SetRecordingSampleRate(N_REC_SAMPLES_PER_SEC);
  _ptrAudioBuffer->SetRecordingChannels(N_REC_CHANNELS);
}

int32_t AudioRecordJni::SetRecordingSampleRate(const uint32_t samplesPerSec) {
  if (samplesPerSec > 48000 || samplesPerSec < 8000)
  {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "  Invalid sample rate");
    return -1;
  }

  
  _samplingFreqin = samplesPerSec;

  
  _ptrAudioBuffer->SetRecordingSampleRate(samplesPerSec);

  return 0;
}

int32_t AudioRecordJni::InitJavaResources() {
  
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

int32_t AudioRecordJni::InitSampleRate() {
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
    
    samplingFreq = _samplingFreqIn;
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

  
  _samplingFreqIn = samplingFreq;

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

bool AudioRecordJni::RecThreadFunc(void* pThis)
{
  return (static_cast<AudioRecordJni*> (pThis)->RecThreadProcess());
}

bool AudioRecordJni::RecThreadProcess()
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
    uint32_t samplesToRec = _samplingFreqIn / 100;

    
    
    
    UnLock();
    jint recDelayInSamples = _jniEnvRec->CallIntMethod(_javaScObj,
                                                        _javaMidRecAudio,
                                                        2 * samplesToRec);
    if (recDelayInSamples < 0)
    {
      WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                   "RecordAudio failed");
      _recWarning = 1;
    }
    else
    {
      _delayRecording = (recDelayInSamples * 1000) / _samplingFreqIn;
    }
    Lock();

    
    if (_recording)
    {
      
      

      
      
      
      memcpy(_recBuffer, _javaDirectRecBuffer, 2 * samplesToRec);

      
      
      _ptrAudioBuffer->SetRecordedBuffer(_recBuffer, samplesToRec);

      
      _ptrAudioBuffer->SetVQEData(_delay_provider->PlayoutDelayMs(),
                                  _delayRecording, 0);

      
      
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

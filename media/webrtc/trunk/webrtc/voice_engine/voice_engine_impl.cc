









#if defined(WEBRTC_ANDROID)
#include "webrtc/modules/audio_device/android/audio_device_template.h"
#if !defined(WEBRTC_GONK)
#include "webrtc/modules/audio_device/android/audio_record_jni.h"
#include "webrtc/modules/audio_device/android/audio_track_jni.h"
#endif
#if !defined(WEBRTC_CHROMIUM_BUILD)
#include "webrtc/modules/audio_device/android/opensles_input.h"
#include "webrtc/modules/audio_device/android/opensles_output.h"
#endif
#endif

#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/voice_engine/voice_engine_impl.h"

namespace webrtc
{





static int32_t gVoiceEngineInstanceCounter = 0;

VoiceEngine* GetVoiceEngine(const Config* config, bool owns_config)
{
#if (defined _WIN32)
  HMODULE hmod = LoadLibrary(TEXT("VoiceEngineTestingDynamic.dll"));

  if (hmod) {
    typedef VoiceEngine* (*PfnGetVoiceEngine)(void);
    PfnGetVoiceEngine pfn = (PfnGetVoiceEngine)GetProcAddress(
        hmod,"GetVoiceEngine");
    if (pfn) {
      VoiceEngine* self = pfn();
      if (owns_config) {
        delete config;
      }
      return (self);
    }
  }
#endif

    VoiceEngineImpl* self = new VoiceEngineImpl(config, owns_config);
    if (self != NULL)
    {
        self->AddRef();  
        gVoiceEngineInstanceCounter++;
    }
    return self;
}

int VoiceEngineImpl::AddRef() {
  return ++_ref_count;
}


int VoiceEngineImpl::Release() {
  int new_ref = --_ref_count;
  assert(new_ref >= 0);
  if (new_ref == 0) {
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, -1,
                 "VoiceEngineImpl self deleting (voiceEngine=0x%p)",
                 this);

    
    
    
    
    
    Terminate();
    delete this;
  }

  return new_ref;
}

VoiceEngine* VoiceEngine::Create() {
  Config* config = new Config();
  return GetVoiceEngine(config, true);
}

VoiceEngine* VoiceEngine::Create(const Config& config) {
  return GetVoiceEngine(&config, false);
}

int VoiceEngine::SetTraceFilter(unsigned int filter)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice,
                 VoEId(gVoiceEngineInstanceCounter, -1),
                 "SetTraceFilter(filter=0x%x)", filter);

    
    uint32_t oldFilter = Trace::level_filter();
    Trace::set_level_filter(filter);

    
    if (kTraceNone == oldFilter)
    {
        WEBRTC_TRACE(kTraceApiCall, kTraceVoice, -1,
                     "SetTraceFilter(filter=0x%x)", filter);
    }

    return 0;
}

int VoiceEngine::SetTraceFile(const char* fileNameUTF8,
                              bool addFileCounter)
{
    int ret = Trace::SetTraceFile(fileNameUTF8, addFileCounter);
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice,
                 VoEId(gVoiceEngineInstanceCounter, -1),
                 "SetTraceFile(fileNameUTF8=%s, addFileCounter=%d)",
                 fileNameUTF8, addFileCounter);
    return (ret);
}

int VoiceEngine::SetTraceCallback(TraceCallback* callback)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice,
                 VoEId(gVoiceEngineInstanceCounter, -1),
                 "SetTraceCallback(callback=0x%x)", callback);
    return (Trace::SetTraceCallback(callback));
}

bool VoiceEngine::Delete(VoiceEngine*& voiceEngine)
{
    if (voiceEngine == NULL)
        return false;

    VoiceEngineImpl* s = static_cast<VoiceEngineImpl*>(voiceEngine);
    
    int ref = s->Release();
    voiceEngine = NULL;

    if (ref != 0) {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, -1,
            "VoiceEngine::Delete did not release the very last reference.  "
            "%d references remain.", ref);
    }

    return true;
}

#if !defined(WEBRTC_CHROMIUM_BUILD)
int VoiceEngine::SetAndroidObjects(void* javaVM, void* env, void* context)
{
#ifdef WEBRTC_ANDROID
#ifdef WEBRTC_ANDROID_OPENSLES
  typedef AudioDeviceTemplate<OpenSlesInput, OpenSlesOutput>
      AudioDeviceInstance;
#endif
#if !defined(WEBRTC_GONK) && defined(ANDROID)
  typedef AudioDeviceTemplate<AudioRecordJni, AudioTrackJni>
      AudioDeviceInstanceJni;
#endif
  if (javaVM && env && context) {
#if !defined(WEBRTC_GONK) && defined(ANDROID)
    AudioDeviceInstanceJni::SetAndroidAudioDeviceObjects(javaVM, env, context);
#endif
    AudioDeviceInstance::SetAndroidAudioDeviceObjects(javaVM, env, context);
  } else {
#if !defined(WEBRTC_GONK) && defined(ANDROID)
    AudioDeviceInstanceJni::ClearAndroidAudioDeviceObjects();
#endif
    AudioDeviceInstance::ClearAndroidAudioDeviceObjects();
  }
  return 0;
#else
  return -1;
#endif
}
#endif

}  

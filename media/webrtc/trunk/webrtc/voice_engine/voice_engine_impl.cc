









#if defined(WEBRTC_ANDROID) && !defined(WEBRTC_ANDROID_OPENSLES)
#include "modules/audio_device/android/audio_device_jni_android.h"
#endif

#include "voice_engine_impl.h"
#include "trace.h"

namespace webrtc
{





static WebRtc_Word32 gVoiceEngineInstanceCounter = 0;

extern "C"
{
WEBRTC_DLLEXPORT VoiceEngine* GetVoiceEngine();

VoiceEngine* GetVoiceEngine()
{
    VoiceEngineImpl* self = new VoiceEngineImpl();
    VoiceEngine* ve = reinterpret_cast<VoiceEngine*>(self);
    if (ve != NULL)
    {
        self->AddRef();  
        gVoiceEngineInstanceCounter++;
    }
    return ve;
}
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

    delete this;
  }

  return new_ref;
}

VoiceEngine* VoiceEngine::Create()
{
#if (defined _WIN32)
    HMODULE hmod_ = LoadLibrary(TEXT("VoiceEngineTestingDynamic.dll"));

    if (hmod_)
    {
        typedef VoiceEngine* (*PfnGetVoiceEngine)(void);
        PfnGetVoiceEngine pfn = (PfnGetVoiceEngine)GetProcAddress(
                hmod_,"GetVoiceEngine");
        if (pfn)
        {
            VoiceEngine* self = pfn();
            return (self);
        }
    }
#endif

    return GetVoiceEngine();
}

int VoiceEngine::SetTraceFilter(const unsigned int filter)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice,
                 VoEId(gVoiceEngineInstanceCounter, -1),
                 "SetTraceFilter(filter=0x%x)", filter);

    
    WebRtc_UWord32 oldFilter = 0;
    Trace::LevelFilter(oldFilter);

    
    WebRtc_Word32 ret = Trace::SetLevelFilter(filter);

    
    if (kTraceNone == oldFilter)
    {
        WEBRTC_TRACE(kTraceApiCall, kTraceVoice, -1,
                     "SetTraceFilter(filter=0x%x)", filter);
    }

    return (ret);
}

int VoiceEngine::SetTraceFile(const char* fileNameUTF8,
                              const bool addFileCounter)
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

    VoiceEngineImpl* s = reinterpret_cast<VoiceEngineImpl*>(voiceEngine);
    
    int ref = s->Release();
    voiceEngine = NULL;

    if (ref != 0) {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, -1,
            "VoiceEngine::Delete did not release the very last reference.  "
            "%d references remain.", ref);
    }

    return true;
}

int VoiceEngine::SetAndroidObjects(void* javaVM, void* env, void* context)
{
#ifdef WEBRTC_ANDROID
#ifdef WEBRTC_ANDROID_OPENSLES
  return 0;
#else
  return AudioDeviceAndroidJni::SetAndroidAudioDeviceObjects(
      javaVM, env, context);
#endif
#else
  return -1;
#endif
}

} 

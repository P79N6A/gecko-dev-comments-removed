



#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#if defined(PR_LOG)
#error "This file must be #included before any IPDL-generated files or other files that #include prlog.h"
#endif

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#include "CSFLog.h"
#include "prenv.h"

#ifdef PR_LOGGING
static PRLogModuleInfo*
GetUserMediaLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("GetUserMedia");
  return sLog;
}
#endif

#undef LOG
#define LOG(args) PR_LOG(GetUserMediaLog(), PR_LOG_DEBUG, args)

#include "MediaEngineWebRTC.h"
#include "ImageContainer.h"

namespace mozilla {

void
MediaEngineWebRTC::EnumerateVideoDevices(nsTArray<nsRefPtr<MediaEngineVideoSource> >* aVSources)
{
  webrtc::ViEBase* ptrViEBase;
  webrtc::ViECapture* ptrViECapture;
  
  MutexAutoLock lock(mMutex);

  int32_t width  = MediaEngineWebRTCVideoSource::DEFAULT_VIDEO_WIDTH;
  int32_t height = MediaEngineWebRTCVideoSource::DEFAULT_VIDEO_HEIGHT;
  int32_t fps    = MediaEngineWebRTCVideoSource::DEFAULT_VIDEO_FPS;
  int32_t minfps = MediaEngineWebRTCVideoSource::DEFAULT_VIDEO_MIN_FPS;

  
  
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefs = do_GetService("@mozilla.org/preferences-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(prefs);

    if (branch) {
      branch->GetIntPref("media.navigator.video.default_width", &width);
      branch->GetIntPref("media.navigator.video.default_height", &height);
      branch->GetIntPref("media.navigator.video.default_fps", &fps);
      branch->GetIntPref("media.navigator.video.default_min_fps", &minfps);
    }
  }
  LOG(("%s: %dx%d @%dfps (min %d)", __FUNCTION__, width, height, fps, minfps));

  bool changed = false;
  if (width != mWidth || height != mHeight || fps != mFPS || minfps != mMinFPS) {
    changed = true;
  }
  mWidth = width;
  mHeight = height;
  mFPS = fps;
  mMinFPS = minfps;

  if (!mVideoEngine) {
    if (!(mVideoEngine = webrtc::VideoEngine::Create())) {
      return;
    }
  }

  PRLogModuleInfo *logs = GetWebRTCLogInfo();
  if (!gWebrtcTraceLoggingOn && logs && logs->level > 0) {
    
    gWebrtcTraceLoggingOn = 1;

    const char *file = PR_GetEnv("WEBRTC_TRACE_FILE");
    if (!file) {
      file = "WebRTC.log";
    }

    LOG(("Logging webrtc to %s level %d", __FUNCTION__, file, logs->level));

    mVideoEngine->SetTraceFilter(logs->level);
    mVideoEngine->SetTraceFile(file);
  }

  ptrViEBase = webrtc::ViEBase::GetInterface(mVideoEngine);
  if (!ptrViEBase) {
    return;
  }

  if (!mVideoEngineInit) {
    if (ptrViEBase->Init() < 0) {
      return;
    }
    mVideoEngineInit = true;
  }

  ptrViECapture = webrtc::ViECapture::GetInterface(mVideoEngine);
  if (!ptrViECapture) {
    return;
  }

  







  int num = ptrViECapture->NumberOfCaptureDevices();
  if (num <= 0) {
    return;
  }

  for (int i = 0; i < num; i++) {
    const unsigned int kMaxDeviceNameLength = 128; 
    const unsigned int kMaxUniqueIdLength = 256;
    char deviceName[kMaxDeviceNameLength];
    char uniqueId[kMaxUniqueIdLength];

    
    deviceName[0] = '\0';
    uniqueId[0] = '\0';
    int error = ptrViECapture->GetCaptureDevice(i, deviceName,
                                                sizeof(deviceName), uniqueId,
                                                sizeof(uniqueId));

    if (error) {
      LOG((" VieCapture:GetCaptureDevice: Failed %d",
           ptrViEBase->LastError() ));
      continue;
    }
#ifdef DEBUG
    LOG(("  Capture Device Index %d, Name %s", i, deviceName));

    webrtc::CaptureCapability cap;
    int numCaps = ptrViECapture->NumberOfCapabilities(uniqueId, kMaxUniqueIdLength);
    LOG(("Number of Capabilities %d", numCaps));
    for (int j = 0; j < numCaps; j++) {
      if (ptrViECapture->GetCaptureCapability(uniqueId, kMaxUniqueIdLength,
                                              j, cap ) != 0 ) {
        break;
      }
      LOG(("type=%d width=%d height=%d maxFPS=%d",
           cap.rawType, cap.width, cap.height, cap.maxFPS ));
    }
#endif

    if (uniqueId[0] == '\0') {
      
      strncpy(uniqueId, deviceName, sizeof(uniqueId));
	  uniqueId[sizeof(uniqueId)-1] = '\0'; 
    }

    nsRefPtr<MediaEngineWebRTCVideoSource> vSource;
    NS_ConvertUTF8toUTF16 uuid(uniqueId);
    if (!changed && mVideoSources.Get(uuid, getter_AddRefs(vSource))) {
      
      aVSources->AppendElement(vSource.get());
    } else {
      vSource = new MediaEngineWebRTCVideoSource(mVideoEngine, i,
                                                 width, height, fps, minfps);
      mVideoSources.Put(uuid, vSource); 
      aVSources->AppendElement(vSource);
    }
  }

  ptrViEBase->Release();
  ptrViECapture->Release();

  return;
}

void
MediaEngineWebRTC::EnumerateAudioDevices(nsTArray<nsRefPtr<MediaEngineAudioSource> >* aASources)
{
  webrtc::VoEBase* ptrVoEBase = NULL;
  webrtc::VoEHardware* ptrVoEHw = NULL;
  
  MutexAutoLock lock(mMutex);

  if (!mVoiceEngine) {
    mVoiceEngine = webrtc::VoiceEngine::Create();
    if (!mVoiceEngine) {
      return;
    }
  }

  ptrVoEBase = webrtc::VoEBase::GetInterface(mVoiceEngine);
  if (!ptrVoEBase) {
    return;
  }

  if (!mAudioEngineInit) {
    if (ptrVoEBase->Init() < 0) {
      return;
    }
    mAudioEngineInit = true;
  }

  ptrVoEHw = webrtc::VoEHardware::GetInterface(mVoiceEngine);
  if (!ptrVoEHw)  {
    return;
  }

  int nDevices = 0;
  ptrVoEHw->GetNumOfRecordingDevices(nDevices);
  for (int i = 0; i < nDevices; i++) {
    
    char deviceName[128];
    char uniqueId[128];
    
    deviceName[0] = '\0';
    uniqueId[0] = '\0';

    int error = ptrVoEHw->GetRecordingDeviceName(i, deviceName, uniqueId);
    if (error) {
      LOG((" VoEHardware:GetRecordingDeviceName: Failed %d",
           ptrVoEBase->LastError() ));
      continue;
    }

    LOG(("  Capture Device Index %d, Name %s Uuid %s", i, deviceName, uniqueId));
    if (uniqueId[0] == '\0') {
      
      MOZ_ASSERT(sizeof(deviceName) == sizeof(uniqueId)); 
      strcpy(uniqueId,deviceName); 
    }

    nsRefPtr<MediaEngineWebRTCAudioSource> aSource;
    NS_ConvertUTF8toUTF16 uuid(uniqueId);
    if (mAudioSources.Get(uuid, getter_AddRefs(aSource))) {
      
      aASources->AppendElement(aSource.get());
    } else {
      aSource = new MediaEngineWebRTCAudioSource(
        mVoiceEngine, i, deviceName, uniqueId
      );
      mAudioSources.Put(uuid, aSource); 
      aASources->AppendElement(aSource);
    }
  }

  ptrVoEHw->Release();
  ptrVoEBase->Release();
}

void
MediaEngineWebRTC::Shutdown()
{
  
  MutexAutoLock lock(mMutex);

  if (mVideoEngine) {
    mVideoSources.Clear();
    webrtc::VideoEngine::Delete(mVideoEngine);
  }

  if (mVoiceEngine) {
    mAudioSources.Clear();
    webrtc::VoiceEngine::Delete(mVoiceEngine);
  }

  mVideoEngine = NULL;
  mVoiceEngine = NULL;
}

}

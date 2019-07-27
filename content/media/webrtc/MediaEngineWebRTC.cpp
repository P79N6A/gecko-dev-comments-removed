



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

#include "MediaEngineWebRTC.h"
#include "ImageContainer.h"
#include "nsIComponentRegistrar.h"
#include "MediaEngineTabVideoSource.h"
#include "nsITabSource.h"
#include "MediaTrackConstraints.h"

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidJNIWrapper.h"
#include "AndroidBridge.h"
#endif

#undef LOG
#define LOG(args) PR_LOG(GetUserMediaLog(), PR_LOG_DEBUG, args)

namespace mozilla {

MediaEngineWebRTC::MediaEngineWebRTC(MediaEnginePrefs &aPrefs)
    : mMutex("mozilla::MediaEngineWebRTC")
    , mScreenEngine(nullptr)
    , mBrowserEngine(nullptr)
    , mWinEngine(nullptr)
    , mAppEngine(nullptr)
    , mVideoEngine(nullptr)
    , mVoiceEngine(nullptr)
    , mVideoEngineInit(false)
    , mAudioEngineInit(false)
    , mScreenEngineInit(false)
    , mBrowserEngineInit(false)
    , mAppEngineInit(false)
{
#ifndef MOZ_B2G_CAMERA
  nsCOMPtr<nsIComponentRegistrar> compMgr;
  NS_GetComponentRegistrar(getter_AddRefs(compMgr));
  if (compMgr) {
    compMgr->IsContractIDRegistered(NS_TABSOURCESERVICE_CONTRACTID, &mHasTabVideoSource);
  }
#else
  AsyncLatencyLogger::Get()->AddRef();
#endif
  
  gFarendObserver = new AudioOutputObserver();

  NS_NewNamedThread("AudioGUM", getter_AddRefs(mThread));
  MOZ_ASSERT(mThread);
}

void
MediaEngineWebRTC::EnumerateVideoDevices(MediaSourceType aMediaSource,
                                         nsTArray<nsRefPtr<MediaEngineVideoSource> >* aVSources)
{
  
  MutexAutoLock lock(mMutex);

 #ifdef MOZ_B2G_CAMERA
  if (aMediaSource != MediaSourceType::Camera) {
    
    return;
  }

  







  int num = 0;
  nsresult result;
  result = ICameraControl::GetNumberOfCameras(num);
  if (num <= 0 || result != NS_OK) {
    return;
  }

  for (int i = 0; i < num; i++) {
    nsCString cameraName;
    result = ICameraControl::GetCameraName(i, cameraName);
    if (result != NS_OK) {
      continue;
    }

    nsRefPtr<MediaEngineWebRTCVideoSource> vSource;
    NS_ConvertUTF8toUTF16 uuid(cameraName);
    if (mVideoSources.Get(uuid, getter_AddRefs(vSource))) {
      
      aVSources->AppendElement(vSource.get());
    } else {
      vSource = new MediaEngineWebRTCVideoSource(i, aMediaSource);
      mVideoSources.Put(uuid, vSource); 
      aVSources->AppendElement(vSource);
    }
  }

  return;
#else
  ScopedCustomReleasePtr<webrtc::ViEBase> ptrViEBase;
  ScopedCustomReleasePtr<webrtc::ViECapture> ptrViECapture;
  webrtc::Config configSet;
  webrtc::VideoEngine *videoEngine = nullptr;
  bool *videoEngineInit = nullptr;

#ifdef MOZ_WIDGET_ANDROID
  
  JavaVM *jvm = mozilla::AndroidBridge::Bridge()->GetVM();

  if (webrtc::VideoEngine::SetAndroidObjects(jvm) != 0) {
    LOG(("VieCapture:SetAndroidObjects Failed"));
    return;
  }
#endif

  switch (aMediaSource) {
    case MediaSourceType::Window:
      mWinEngineConfig.Set<webrtc::CaptureDeviceInfo>(
          new webrtc::CaptureDeviceInfo(webrtc::CaptureDeviceType::Window));
      if (!mWinEngine) {
        if (!(mWinEngine = webrtc::VideoEngine::Create(mWinEngineConfig))) {
          return;
        }
      }
      videoEngine = mWinEngine;
      videoEngineInit = &mWinEngineInit;
      break;
    case MediaSourceType::Application:
      mAppEngineConfig.Set<webrtc::CaptureDeviceInfo>(
          new webrtc::CaptureDeviceInfo(webrtc::CaptureDeviceType::Application));
      if (!mAppEngine) {
        if (!(mAppEngine = webrtc::VideoEngine::Create(mAppEngineConfig))) {
          return;
        }
      }
      videoEngine = mAppEngine;
      videoEngineInit = &mAppEngineInit;
      break;
    case MediaSourceType::Screen:
      mScreenEngineConfig.Set<webrtc::CaptureDeviceInfo>(
          new webrtc::CaptureDeviceInfo(webrtc::CaptureDeviceType::Screen));
      if (!mScreenEngine) {
        if (!(mScreenEngine = webrtc::VideoEngine::Create(mScreenEngineConfig))) {
          return;
        }
      }
      videoEngine = mScreenEngine;
      videoEngineInit = &mScreenEngineInit;
      break;
    case MediaSourceType::Browser:
      mBrowserEngineConfig.Set<webrtc::CaptureDeviceInfo>(
          new webrtc::CaptureDeviceInfo(webrtc::CaptureDeviceType::Browser));
      if (!mBrowserEngine) {
        if (!(mBrowserEngine = webrtc::VideoEngine::Create(mBrowserEngineConfig))) {
          return;
        }
      }
      videoEngine = mBrowserEngine;
      videoEngineInit = &mBrowserEngineInit;
      break;
    case MediaSourceType::Camera:
      
    default:
      if (!mVideoEngine) {
        if (!(mVideoEngine = webrtc::VideoEngine::Create())) {
          return;
        }
      }
      videoEngine = mVideoEngine;
      videoEngineInit = &mVideoEngineInit;
      break;
  }

  ptrViEBase = webrtc::ViEBase::GetInterface(videoEngine);
  if (!ptrViEBase) {
    return;
  }
  if (ptrViEBase->Init() < 0) {
    return;
  }
  *videoEngineInit = true;

  ptrViECapture = webrtc::ViECapture::GetInterface(videoEngine);
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
    if (mVideoSources.Get(uuid, getter_AddRefs(vSource))) {
      
      aVSources->AppendElement(vSource.get());
    } else {
      vSource = new MediaEngineWebRTCVideoSource(videoEngine, i, aMediaSource);
      mVideoSources.Put(uuid, vSource); 
      aVSources->AppendElement(vSource);
    }
  }

  if (mHasTabVideoSource)
    aVSources->AppendElement(new MediaEngineTabVideoSource());

  return;
#endif
}

void
MediaEngineWebRTC::EnumerateAudioDevices(MediaSourceType aMediaSource,
                                         nsTArray<nsRefPtr<MediaEngineAudioSource> >* aASources)
{
  ScopedCustomReleasePtr<webrtc::VoEBase> ptrVoEBase;
  ScopedCustomReleasePtr<webrtc::VoEHardware> ptrVoEHw;
  
  MutexAutoLock lock(mMutex);

#ifdef MOZ_WIDGET_ANDROID
  jobject context = mozilla::AndroidBridge::Bridge()->GetGlobalContextRef();

  
  JavaVM *jvm = mozilla::AndroidBridge::Bridge()->GetVM();
  JNIEnv *env = GetJNIForThread();

  if (webrtc::VoiceEngine::SetAndroidObjects(jvm, env, (void*)context) != 0) {
    LOG(("VoiceEngine:SetAndroidObjects Failed"));
    return;
  }
#endif

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
  int i;
#if defined(MOZ_WIDGET_ANDROID) || defined(MOZ_WIDGET_GONK)
  i = 0; 
#else
  
  i = -1;
#endif
  for (; i < nDevices; i++) {
    
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
        mThread, mVoiceEngine, i, deviceName, uniqueId
      );
      mAudioSources.Put(uuid, aSource); 
      aASources->AppendElement(aSource);
    }
  }
}

void
MediaEngineWebRTC::Shutdown()
{
  
  MutexAutoLock lock(mMutex);

  
  mVideoSources.Clear();
  mAudioSources.Clear();
  if (mVideoEngine) {
    mVideoEngine->SetTraceCallback(nullptr);
    webrtc::VideoEngine::Delete(mVideoEngine);
  }

  if (mScreenEngine) {
    mScreenEngine->SetTraceCallback(nullptr);
    webrtc::VideoEngine::Delete(mScreenEngine);
  }
  if (mWinEngine) {
    mWinEngine->SetTraceCallback(nullptr);
    webrtc::VideoEngine::Delete(mWinEngine);
  }
  if (mBrowserEngine) {
    mBrowserEngine->SetTraceCallback(nullptr);
    webrtc::VideoEngine::Delete(mBrowserEngine);
  }
  if (mAppEngine) {
    mAppEngine->SetTraceCallback(nullptr);
    webrtc::VideoEngine::Delete(mAppEngine);
  }

  if (mVoiceEngine) {
    mVoiceEngine->SetTraceCallback(nullptr);
    webrtc::VoiceEngine::Delete(mVoiceEngine);
  }

  mVideoEngine = nullptr;
  mVoiceEngine = nullptr;
  mScreenEngine = nullptr;
  mWinEngine = nullptr;
  mBrowserEngine = nullptr;
  mAppEngine = nullptr;

  if (mThread) {
    mThread->Shutdown();
    mThread = nullptr;
  }
}

}

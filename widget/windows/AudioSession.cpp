





#include <windows.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>

#include "nsIStringBundle.h"
#include "nsIUUIDGenerator.h"
#include "nsIXULAppInfo.h"


#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "mozilla/Attributes.h"

#include <objbase.h>

namespace mozilla {
namespace widget {






class AudioSession final : public IAudioSessionEvents {
private:
  AudioSession();
  ~AudioSession();
public:
  static AudioSession* GetSingleton();

  
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP QueryInterface(REFIID, void**);
  STDMETHODIMP_(ULONG) Release();

  
  STDMETHODIMP OnChannelVolumeChanged(DWORD aChannelCount,
                                      float aChannelVolumeArray[],
                                      DWORD aChangedChannel,
                                      LPCGUID aContext);
  STDMETHODIMP OnDisplayNameChanged(LPCWSTR aDisplayName, LPCGUID aContext);
  STDMETHODIMP OnGroupingParamChanged(LPCGUID aGroupingParam, LPCGUID aContext);
  STDMETHODIMP OnIconPathChanged(LPCWSTR aIconPath, LPCGUID aContext);
  STDMETHODIMP OnSessionDisconnected(AudioSessionDisconnectReason aReason);
private:
  nsresult OnSessionDisconnectedInternal();
public:
  STDMETHODIMP OnSimpleVolumeChanged(float aVolume,
                                     BOOL aMute,
                                     LPCGUID aContext);
  STDMETHODIMP OnStateChanged(AudioSessionState aState);

  nsresult Start();
  nsresult Stop();
  void StopInternal();

  nsresult GetSessionData(nsID& aID,
                          nsString& aSessionName,
                          nsString& aIconPath);

  nsresult SetSessionData(const nsID& aID,
                          const nsString& aSessionName,
                          const nsString& aIconPath);

  enum SessionState {
    UNINITIALIZED, 
    STARTED, 
    CLONED, 
    FAILED, 
    STOPPED, 
    AUDIO_SESSION_DISCONNECTED 
  };
protected:
  nsRefPtr<IAudioSessionControl> mAudioSessionControl;
  nsString mDisplayName;
  nsString mIconPath;
  nsID mSessionGroupingParameter;
  SessionState mState;

  ThreadSafeAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

  static AudioSession* sService;
};

nsresult
StartAudioSession()
{
  return AudioSession::GetSingleton()->Start();
}

nsresult
StopAudioSession()
{
  return AudioSession::GetSingleton()->Stop();
}

nsresult
GetAudioSessionData(nsID& aID,
                    nsString& aSessionName,
                    nsString& aIconPath)
{
  return AudioSession::GetSingleton()->GetSessionData(aID,
                                                      aSessionName,
                                                      aIconPath);
}

nsresult
RecvAudioSessionData(const nsID& aID,
                     const nsString& aSessionName,
                     const nsString& aIconPath)
{
  return AudioSession::GetSingleton()->SetSessionData(aID,
                                                      aSessionName,
                                                      aIconPath);
}

AudioSession* AudioSession::sService = nullptr;

AudioSession::AudioSession()
{
  mState = UNINITIALIZED;
}

AudioSession::~AudioSession()
{

}

AudioSession*
AudioSession::GetSingleton()
{
  if (!(AudioSession::sService)) {
    nsRefPtr<AudioSession> service = new AudioSession();
    service.forget(&AudioSession::sService);
  }

  
  
  return AudioSession::sService;
}


NS_IMPL_ADDREF(AudioSession)
NS_IMPL_RELEASE(AudioSession)

STDMETHODIMP
AudioSession::QueryInterface(REFIID iid, void **ppv)
{
  const IID IID_IAudioSessionEvents = __uuidof(IAudioSessionEvents);
  if ((IID_IUnknown == iid) ||
      (IID_IAudioSessionEvents == iid)) {
    *ppv = static_cast<IAudioSessionEvents*>(this);
    AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}




nsresult
AudioSession::Start()
{
  MOZ_ASSERT(mState == UNINITIALIZED ||
             mState == CLONED ||
             mState == AUDIO_SESSION_DISCONNECTED,
             "State invariants violated");

  const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
  const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
  const IID IID_IAudioSessionManager = __uuidof(IAudioSessionManager);

  HRESULT hr;

  
  
  CoInitialize(nullptr);

  if (mState == UNINITIALIZED) {
    mState = FAILED;

    
    if (XRE_IsContentProcess())
      return NS_ERROR_FAILURE;

    MOZ_ASSERT(XRE_IsParentProcess(),
               "Should only get here in a chrome process!");

    nsCOMPtr<nsIStringBundleService> bundleService = 
      do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    NS_ENSURE_TRUE(bundleService, NS_ERROR_FAILURE);
    nsCOMPtr<nsIStringBundle> bundle;
    bundleService->CreateBundle("chrome://branding/locale/brand.properties",
                                getter_AddRefs(bundle));
    NS_ENSURE_TRUE(bundle, NS_ERROR_FAILURE);

    bundle->GetStringFromName(MOZ_UTF16("brandFullName"),
                              getter_Copies(mDisplayName));

    wchar_t *buffer;
    mIconPath.GetMutableData(&buffer, MAX_PATH);

    
    
    ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);

    nsCOMPtr<nsIUUIDGenerator> uuidgen =
      do_GetService("@mozilla.org/uuid-generator;1");
    NS_ENSURE_TRUE(uuidgen, NS_ERROR_FAILURE);
    uuidgen->GenerateUUIDInPlace(&mSessionGroupingParameter);
  }

  mState = FAILED;

  MOZ_ASSERT(!mDisplayName.IsEmpty() || !mIconPath.IsEmpty(),
             "Should never happen ...");

  nsRefPtr<IMMDeviceEnumerator> enumerator;
  hr = ::CoCreateInstance(CLSID_MMDeviceEnumerator,
                          nullptr,
                          CLSCTX_ALL,
                          IID_IMMDeviceEnumerator,
                          getter_AddRefs(enumerator));
  if (FAILED(hr))
    return NS_ERROR_NOT_AVAILABLE;

  nsRefPtr<IMMDevice> device;
  hr = enumerator->GetDefaultAudioEndpoint(EDataFlow::eRender,
                                           ERole::eMultimedia,
                                           getter_AddRefs(device));
  if (FAILED(hr)) {
    if (hr == E_NOTFOUND)
      return NS_ERROR_NOT_AVAILABLE;
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<IAudioSessionManager> manager;
  hr = device->Activate(IID_IAudioSessionManager,
                        CLSCTX_ALL,
                        nullptr,
                        getter_AddRefs(manager));
  if (FAILED(hr))
    return NS_ERROR_FAILURE;

  hr = manager->GetAudioSessionControl(nullptr,
                                       FALSE,
                                       getter_AddRefs(mAudioSessionControl));
  if (FAILED(hr))
    return NS_ERROR_FAILURE;

  hr = mAudioSessionControl->SetGroupingParam((LPCGUID)&mSessionGroupingParameter,
                                              nullptr);
  if (FAILED(hr)) {
    StopInternal();
    return NS_ERROR_FAILURE;
  }

  hr = mAudioSessionControl->SetDisplayName(mDisplayName.get(), nullptr);
  if (FAILED(hr)) {
    StopInternal();
    return NS_ERROR_FAILURE;
  }

  hr = mAudioSessionControl->SetIconPath(mIconPath.get(), nullptr);
  if (FAILED(hr)) {
    StopInternal();
    return NS_ERROR_FAILURE;
  }

  hr = mAudioSessionControl->RegisterAudioSessionNotification(this);
  if (FAILED(hr)) {
    StopInternal();
    return NS_ERROR_FAILURE;
  }

  mState = STARTED;

  return NS_OK;
}

void
AudioSession::StopInternal()
{
  static const nsID blankId = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} };

  if (mAudioSessionControl) {
    mAudioSessionControl->SetGroupingParam((LPCGUID)&blankId, nullptr);
    mAudioSessionControl->UnregisterAudioSessionNotification(this);
    mAudioSessionControl = nullptr;
  }
}

nsresult
AudioSession::Stop()
{
  MOZ_ASSERT(mState == STARTED ||
             mState == UNINITIALIZED || 
             mState == FAILED,
             "State invariants violated");
  mState = STOPPED;

  nsRefPtr<AudioSession> kungFuDeathGrip;
  kungFuDeathGrip.swap(sService);

  if (!XRE_IsContentProcess())
    StopInternal();

  

  ::CoUninitialize();

  return NS_OK;
}

void CopynsID(nsID& lhs, const nsID& rhs)
{
  lhs.m0 = rhs.m0;
  lhs.m1 = rhs.m1;
  lhs.m2 = rhs.m2;
  for (int i = 0; i < 8; i++ ) {
    lhs.m3[i] = rhs.m3[i];
  }
}

nsresult
AudioSession::GetSessionData(nsID& aID,
                             nsString& aSessionName,
                             nsString& aIconPath)
{
  MOZ_ASSERT(mState == FAILED ||
             mState == STARTED ||
             mState == CLONED,
             "State invariants violated");

  CopynsID(aID, mSessionGroupingParameter);
  aSessionName = mDisplayName;
  aIconPath = mIconPath;

  if (mState == FAILED)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult
AudioSession::SetSessionData(const nsID& aID,
                             const nsString& aSessionName,
                             const nsString& aIconPath)
{
  MOZ_ASSERT(mState == UNINITIALIZED,
             "State invariants violated");
  MOZ_ASSERT(!XRE_IsParentProcess(),
             "Should never get here in a chrome process!");
  mState = CLONED;

  CopynsID(mSessionGroupingParameter, aID);
  mDisplayName = aSessionName;
  mIconPath = aIconPath;
  return NS_OK;
}

STDMETHODIMP
AudioSession::OnChannelVolumeChanged(DWORD aChannelCount,
                                     float aChannelVolumeArray[],
                                     DWORD aChangedChannel,
                                     LPCGUID aContext)
{
  return S_OK; 
}

STDMETHODIMP
AudioSession::OnDisplayNameChanged(LPCWSTR aDisplayName,
                                   LPCGUID aContext)
{
  return S_OK; 
}

STDMETHODIMP
AudioSession::OnGroupingParamChanged(LPCGUID aGroupingParam,
                                     LPCGUID aContext)
{
  return S_OK; 
}

STDMETHODIMP
AudioSession::OnIconPathChanged(LPCWSTR aIconPath,
                                LPCGUID aContext)
{
  return S_OK; 
}

STDMETHODIMP
AudioSession::OnSessionDisconnected(AudioSessionDisconnectReason aReason)
{
  
  
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &AudioSession::OnSessionDisconnectedInternal);
  NS_DispatchToMainThread(runnable);
  return S_OK;
}

nsresult
AudioSession::OnSessionDisconnectedInternal()
{
  if (!mAudioSessionControl)
    return NS_OK;

  mAudioSessionControl->UnregisterAudioSessionNotification(this);
  mAudioSessionControl = nullptr;

  mState = AUDIO_SESSION_DISCONNECTED;
  CoUninitialize();
  Start(); 
  return NS_OK;
}

STDMETHODIMP
AudioSession::OnSimpleVolumeChanged(float aVolume,
                                    BOOL aMute,
                                    LPCGUID aContext)
{
  return S_OK; 
}

STDMETHODIMP
AudioSession::OnStateChanged(AudioSessionState aState)
{
  return S_OK; 
}

} 
} 

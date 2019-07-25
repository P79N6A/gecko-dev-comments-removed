





































#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN

#include <windows.h>
#include <audiopolicy.h>
#include <Mmdeviceapi.h>

#include "nsIStringBundle.h"
#include "nsIUUIDGenerator.h"
#include "nsIXULAppInfo.h"


#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"

#include <objbase.h>

namespace mozilla {
namespace widget {






class AudioSession: public IAudioSessionEvents {
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
  STDMETHODIMP OnSimpleVolumeChanged(float aVolume,
                                     BOOL aMute,
                                     LPCGUID aContext);
  STDMETHODIMP OnStateChanged(AudioSessionState aState);

  nsresult Start();
  nsresult Stop();

protected:
  nsRefPtr<IAudioSessionControl> mAudioSessionControl;
  nsString mDisplayName;
  nsString mIconPath;
  nsID mSessionGroupingParameter;

  nsAutoRefCnt mRefCnt;
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

AudioSession* AudioSession::sService = NULL;

AudioSession::AudioSession()
{

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


NS_IMPL_THREADSAFE_ADDREF(AudioSession)
NS_IMPL_THREADSAFE_RELEASE(AudioSession)

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
  const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
  const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
  const IID IID_IAudioSessionManager = __uuidof(IAudioSessionManager);

  HRESULT hr;

  if (FAILED(::CoInitialize(NULL)))
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStringBundleService> bundleService = 
    do_GetService(NS_STRINGBUNDLE_CONTRACTID);
  NS_ENSURE_TRUE(bundleService, NS_ERROR_FAILURE);

  nsCOMPtr<nsIStringBundle> bundle;
  bundleService->CreateBundle("chrome://branding/locale/brand.properties",
                              getter_AddRefs(bundle));
  NS_ENSURE_TRUE(bundle, NS_ERROR_FAILURE);

  bundle->GetStringFromName(NS_LITERAL_STRING("brandFullName").get(),
                            getter_Copies(mDisplayName));

  PRUnichar *buffer;
  mIconPath.GetMutableData(&buffer, MAX_PATH);

  
  
  ::GetModuleFileNameW(NULL, buffer, MAX_PATH);

  nsCOMPtr<nsIUUIDGenerator> uuidgen =
    do_GetService("@mozilla.org/uuid-generator;1");
  NS_ASSERTION(uuidgen, "No UUID-Generator?!?");

  uuidgen->GenerateUUIDInPlace(&mSessionGroupingParameter);

  nsRefPtr<IMMDeviceEnumerator> enumerator;
  hr = ::CoCreateInstance(CLSID_MMDeviceEnumerator,
                          NULL,
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
                        NULL,
                        getter_AddRefs(manager));
  if (FAILED(hr))
    return NS_ERROR_FAILURE;

  hr = manager->GetAudioSessionControl(NULL,
                                       FALSE,
                                       getter_AddRefs(mAudioSessionControl));
  if (FAILED(hr))
    return NS_ERROR_FAILURE;

  hr = mAudioSessionControl->SetGroupingParam((LPCGUID)&mSessionGroupingParameter,
                                              NULL);
  if (FAILED(hr)) {
    StopInternal();
    return NS_ERROR_FAILURE;
  }

  hr = mAudioSessionControl->SetDisplayName(mDisplayName.get(), NULL);
  if (FAILED(hr)) {
    StopInternal();
    return NS_ERROR_FAILURE;
  }

  hr = mAudioSessionControl->SetIconPath(mIconPath.get(), NULL);
  if (FAILED(hr)) {
    StopInternal();
    return NS_ERROR_FAILURE;
  }

  hr = mAudioSessionControl->RegisterAudioSessionNotification(this);
  if (FAILED(hr)) {
    StopInternal();
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

void
AudioSession::StopInternal()
{
  if (mAudioSessionControl) {
    mAudioSessionControl->SetGroupingParam((LPCGUID)&blankId, NULL);
    mAudioSessionControl->UnregisterAudioSessionNotification(this);
    mAudioSessionControl = nsnull;
  }
}

nsresult
AudioSession::Stop()
{
  const nsID blankId = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} };
  nsRefPtr<AudioSession> kungFuDeathGrip;
  kungFuDeathGrip.swap(sService);

  StopInternal();

  

  ::CoUninitialize();

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
  if (!mAudioSessionControl)
    return S_OK;

  mAudioSessionControl->UnregisterAudioSessionNotification(this);
  mAudioSessionControl = nsnull;
  Start(); 
  return S_OK;
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

#endif 

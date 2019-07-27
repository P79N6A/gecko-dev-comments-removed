





#include "mozilla/dom/MediaKeySystemAccess.h"
#include "mozilla/dom/MediaKeySystemAccessBinding.h"
#include "mozilla/Preferences.h"
#include "nsContentTypeParser.h"
#ifdef MOZ_FMP4
#include "MP4Decoder.h"
#endif
#ifdef XP_WIN
#include "mozilla/WindowsVersion.h"
#include "WMFDecoderModule.h"
#endif
#include "nsContentCID.h"
#include "nsServiceManagerUtils.h"
#include "mozIGeckoMediaPluginService.h"
#include "VideoUtils.h"
#include "mozilla/Services.h"
#include "nsIObserverService.h"
#include "mozilla/EMEUtils.h"
#include "GMPUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(MediaKeySystemAccess,
                                      mParent)
NS_IMPL_CYCLE_COLLECTING_ADDREF(MediaKeySystemAccess)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MediaKeySystemAccess)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MediaKeySystemAccess)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

MediaKeySystemAccess::MediaKeySystemAccess(nsPIDOMWindow* aParent,
                                           const nsAString& aKeySystem)
  : mParent(aParent)
  , mKeySystem(aKeySystem)
{
}

MediaKeySystemAccess::~MediaKeySystemAccess()
{
}

JSObject*
MediaKeySystemAccess::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return MediaKeySystemAccessBinding::Wrap(aCx, this, aGivenProto);
}

nsPIDOMWindow*
MediaKeySystemAccess::GetParentObject() const
{
  return mParent;
}

void
MediaKeySystemAccess::GetKeySystem(nsString& aRetVal) const
{
  aRetVal = mKeySystem;
}

already_AddRefed<Promise>
MediaKeySystemAccess::CreateMediaKeys(ErrorResult& aRv)
{
  nsRefPtr<MediaKeys> keys(new MediaKeys(mParent, mKeySystem));
  return keys->Init(aRv);
}

static bool
HaveGMPFor(mozIGeckoMediaPluginService* aGMPService,
           const nsCString& aKeySystem,
           const nsCString& aAPI,
           const nsCString& aTag = EmptyCString())
{
  nsTArray<nsCString> tags;
  tags.AppendElement(aKeySystem);
  if (!aTag.IsEmpty()) {
    tags.AppendElement(aTag);
  }
  bool hasPlugin = false;
  if (NS_FAILED(aGMPService->HasPluginForAPI(aAPI,
                                             &tags,
                                             &hasPlugin))) {
    return false;
  }
  return hasPlugin;
}

static MediaKeySystemStatus
EnsureMinCDMVersion(mozIGeckoMediaPluginService* aGMPService,
                    const nsAString& aKeySystem,
                    int32_t aMinCdmVersion,
                    bool aCheckForV6=false)
{
  nsTArray<nsCString> tags;
  tags.AppendElement(NS_ConvertUTF16toUTF8(aKeySystem));
  bool hasPlugin;
  nsAutoCString versionStr;
  if (NS_FAILED(aGMPService->GetPluginVersionForAPI(NS_LITERAL_CSTRING(GMP_API_DECRYPTOR),
                                                    &tags,
                                                    &hasPlugin,
                                                    versionStr)) ||
      
      (aCheckForV6 && !hasPlugin &&
       NS_FAILED(aGMPService->GetPluginVersionForAPI(NS_LITERAL_CSTRING(GMP_API_DECRYPTOR_COMPAT),
                                                     &tags,
                                                     &hasPlugin,
                                                     versionStr)))) {
    return MediaKeySystemStatus::Error;
  }

  if (!hasPlugin) {
    return MediaKeySystemStatus::Cdm_not_installed;
  }

  if (aMinCdmVersion == NO_CDM_VERSION) {
    return MediaKeySystemStatus::Available;
  }

  nsresult rv;
  int32_t version = versionStr.ToInteger(&rv);
  if (NS_FAILED(rv) || version < 0 || aMinCdmVersion > version) {
    return MediaKeySystemStatus::Cdm_insufficient_version;
  }

  return MediaKeySystemStatus::Available;
}


MediaKeySystemStatus
MediaKeySystemAccess::GetKeySystemStatus(const nsAString& aKeySystem,
                                         int32_t aMinCdmVersion)
{
  MOZ_ASSERT(Preferences::GetBool("media.eme.enabled", false));
  nsCOMPtr<mozIGeckoMediaPluginService> mps =
    do_GetService("@mozilla.org/gecko-media-plugin-service;1");
  if (NS_WARN_IF(!mps)) {
    return MediaKeySystemStatus::Error;
  }

  if (aKeySystem.EqualsLiteral("org.w3.clearkey")) {
    if (!Preferences::GetBool("media.eme.clearkey.enabled", true)) {
      return MediaKeySystemStatus::Cdm_disabled;
    }
    return EnsureMinCDMVersion(mps, aKeySystem, aMinCdmVersion);
  }

#ifdef XP_WIN
  if ((aKeySystem.EqualsLiteral("com.adobe.access") ||
       aKeySystem.EqualsLiteral("com.adobe.primetime"))) {
    
    if (!IsVistaOrLater()) {
      return MediaKeySystemStatus::Cdm_not_supported;
    }
    if (!Preferences::GetBool("media.gmp-eme-adobe.enabled", false)) {
      return MediaKeySystemStatus::Cdm_disabled;
    }
    if ((!WMFDecoderModule::HasH264() || !WMFDecoderModule::HasAAC()) ||
        !EMEVoucherFileExists()) {
      
      
      
      return MediaKeySystemStatus::Cdm_not_supported;
    }
    return EnsureMinCDMVersion(mps, aKeySystem, aMinCdmVersion, true);
  }
#endif

  return MediaKeySystemStatus::Cdm_not_supported;
}

static bool
IsPlayableWithGMP(mozIGeckoMediaPluginService* aGMPS,
                  const nsAString& aKeySystem,
                  const nsAString& aContentType)
{
#ifdef MOZ_FMP4
  nsContentTypeParser parser(aContentType);
  nsAutoString mimeType;
  nsresult rv = parser.GetType(mimeType);
  if (NS_FAILED(rv)) {
    return false;
  }

  if (!mimeType.EqualsLiteral("audio/mp4") &&
      !mimeType.EqualsLiteral("audio/x-m4a") &&
      !mimeType.EqualsLiteral("video/mp4")) {
    return false;
  }

  nsAutoString codecs;
  parser.GetParameter("codecs", codecs);

  NS_ConvertUTF16toUTF8 mimeTypeUTF8(mimeType);
  bool hasAAC = false;
  bool hasH264 = false;
  bool hasMP3 = false;
  if (!MP4Decoder::CanHandleMediaType(mimeTypeUTF8,
                                      codecs,
                                      hasAAC,
                                      hasH264,
                                      hasMP3) ||
      hasMP3) {
    return false;
  }
  return (!hasAAC ||
          !(HaveGMPFor(aGMPS,
                       NS_ConvertUTF16toUTF8(aKeySystem),
                       NS_LITERAL_CSTRING(GMP_API_DECRYPTOR),
                       NS_LITERAL_CSTRING("aac")) ||
            
            HaveGMPFor(aGMPS,
                       NS_ConvertUTF16toUTF8(aKeySystem),
                       NS_LITERAL_CSTRING(GMP_API_DECRYPTOR_COMPAT),
                       NS_LITERAL_CSTRING("aac")))) &&
         (!hasH264 ||
          !(HaveGMPFor(aGMPS,
                       NS_ConvertUTF16toUTF8(aKeySystem),
                       NS_LITERAL_CSTRING(GMP_API_DECRYPTOR),
                       NS_LITERAL_CSTRING("h264")) ||
            
            HaveGMPFor(aGMPS,
                       NS_ConvertUTF16toUTF8(aKeySystem),
                       NS_LITERAL_CSTRING(GMP_API_DECRYPTOR_COMPAT),
                       NS_LITERAL_CSTRING("h264"))));
#else
  return false;
#endif
}


bool
MediaKeySystemAccess::IsSupported(const nsAString& aKeySystem,
                                  const Sequence<MediaKeySystemOptions>& aOptions)
{
  nsCOMPtr<mozIGeckoMediaPluginService> mps =
    do_GetService("@mozilla.org/gecko-media-plugin-service;1");
  if (NS_WARN_IF(!mps)) {
    return false;
  }

  for (size_t i = 0; i < aOptions.Length(); i++) {
    const MediaKeySystemOptions& options = aOptions[i];
    if (!options.mInitDataType.EqualsLiteral("cenc")) {
      continue;
    }
    if (!options.mAudioCapability.IsEmpty() ||
        !options.mVideoCapability.IsEmpty()) {
      
      
      continue;
    }
    if (!options.mAudioType.IsEmpty() &&
        !IsPlayableWithGMP(mps, aKeySystem, options.mAudioType)) {
      continue;
    }
    if (!options.mVideoType.IsEmpty() &&
        !IsPlayableWithGMP(mps, aKeySystem, options.mVideoType)) {
      continue;
    }

    
    
    
    

    return true;
  }
  return false;
}


void
MediaKeySystemAccess::NotifyObservers(nsIDOMWindow* aWindow,
                                      const nsAString& aKeySystem,
                                      MediaKeySystemStatus aStatus)
{
  RequestMediaKeySystemAccessNotification data;
  data.mKeySystem = aKeySystem;
  data.mStatus = aStatus;
  nsAutoString json;
  data.ToJSON(json);
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (obs) {
    obs->NotifyObservers(aWindow, "mediakeys-request", json.get());
  }
}

} 
} 

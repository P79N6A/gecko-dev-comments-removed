




#include "mozilla/Preferences.h"
#include "mozilla/dom/TimeRanges.h"
#include "MediaResource.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "AndroidMediaPluginHost.h"
#include "nsXPCOMStrings.h"
#include "nsISeekableStream.h"
#include "AndroidMediaReader.h"
#include "nsIGfxInfo.h"
#include "gfxCrashReporterUtils.h"
#include "prmem.h"
#include "prlink.h"
#include "AndroidMediaResourceServer.h"
#include "nsServiceManagerUtils.h"

#include "MPAPI.h"

#include "nsIPropertyBag2.h"

#if defined(ANDROID) || defined(MOZ_WIDGET_GONK)
#include "android/log.h"
#define ALOG(args...)  __android_log_print(ANDROID_LOG_INFO, "AndroidMediaPluginHost" , ## args)
#else
#define ALOG(args...)
#endif

using namespace MPAPI;

Decoder::Decoder() :
  mResource(nullptr), mPrivate(nullptr)
{
}

namespace mozilla {

static char* GetResource(Decoder *aDecoder)
{
  return static_cast<char*>(aDecoder->mResource);
}

class GetIntPrefEvent : public nsRunnable {
public:
  GetIntPrefEvent(const char* aPref, int32_t* aResult)
    : mPref(aPref), mResult(aResult) {}
  NS_IMETHOD Run() {
    return Preferences::GetInt(mPref, mResult);
  }
private:
  const char* mPref;
  int32_t*    mResult;
};

static bool GetIntPref(const char* aPref, int32_t* aResult)
{
  
  
  NS_ENSURE_TRUE(aPref, false);
  NS_ENSURE_TRUE(aResult, false);
  nsCOMPtr<nsIRunnable> event = new GetIntPrefEvent(aPref, aResult);
  return NS_SUCCEEDED(NS_DispatchToMainThread(event, NS_DISPATCH_SYNC));
}

static bool
GetSystemInfoString(const char *aKey, char *aResult, size_t aResultLength)
{
  NS_ENSURE_TRUE(aKey, false);
  NS_ENSURE_TRUE(aResult, false);

  nsCOMPtr<nsIPropertyBag2> infoService = do_GetService("@mozilla.org/system-info;1");
  NS_ASSERTION(infoService, "Could not find a system info service");

  nsAutoCString key(aKey);
  nsAutoCString info;
  nsresult rv = infoService->GetPropertyAsACString(NS_ConvertUTF8toUTF16(key),
                                                  info);

  NS_ENSURE_SUCCESS(rv, false);

  strncpy(aResult, info.get(), aResultLength);

  return true;
}

static PluginHost sPluginHost = {
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  GetIntPref,
  GetSystemInfoString
};



static bool IsOmxSupported()
{
  bool forceEnabled =
      Preferences::GetBool("stagefright.force-enabled", false);
  bool disabled =
      Preferences::GetBool("stagefright.disabled", false);

  if (disabled) {
    NS_WARNING("XXX stagefright disabled\n");
    return false;
  }

  ScopedGfxFeatureReporter reporter("Stagefright", forceEnabled);

  if (!forceEnabled) {
    nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
    if (gfxInfo) {
      int32_t status;
      if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_STAGEFRIGHT, &status))) {
        if (status != nsIGfxInfo::FEATURE_STATUS_OK) {
          NS_WARNING("XXX stagefright blacklisted\n");
          return false;
        }
      }
    }
  }

  reporter.SetSuccessful();
  return true;
}




static const char* GetOmxLibraryName()
{
#if defined(ANDROID) && !defined(MOZ_WIDGET_GONK)
  nsCOMPtr<nsIPropertyBag2> infoService = do_GetService("@mozilla.org/system-info;1");
  NS_ASSERTION(infoService, "Could not find a system info service");

  int32_t version;
  nsresult rv = infoService->GetPropertyAsInt32(NS_LITERAL_STRING("version"), &version);
  if (NS_SUCCEEDED(rv)) {
    ALOG("Android Version is: %d", version);
  }

  nsAutoString release_version;
  rv = infoService->GetPropertyAsAString(NS_LITERAL_STRING("release_version"), release_version);
  if (NS_SUCCEEDED(rv)) {
    ALOG("Android Release Version is: %s", NS_LossyConvertUTF16toASCII(release_version).get());
  }

  nsAutoString device;
  rv = infoService->GetPropertyAsAString(NS_LITERAL_STRING("device"), device);
  if (NS_SUCCEEDED(rv)) {
    ALOG("Android Device is: %s", NS_LossyConvertUTF16toASCII(device).get());
  }

  nsAutoString manufacturer;
  rv = infoService->GetPropertyAsAString(NS_LITERAL_STRING("manufacturer"), manufacturer);
  if (NS_SUCCEEDED(rv)) {
    ALOG("Android Manufacturer is: %s", NS_LossyConvertUTF16toASCII(manufacturer).get());
  }

  nsAutoString hardware;
  rv = infoService->GetPropertyAsAString(NS_LITERAL_STRING("hardware"), hardware);
  if (NS_SUCCEEDED(rv)) {
    ALOG("Android Hardware is: %s", NS_LossyConvertUTF16toASCII(hardware).get());
  }
#endif

  if (!IsOmxSupported())
    return nullptr;

#if defined(ANDROID) && !defined(MOZ_WIDGET_GONK)
  if (version >= 17) {
    return "libomxpluginkk.so";
  }
  else if (version == 13 || version == 12 || version == 11) {
    return "libomxpluginhc.so";
  }
  else if (version == 10 && release_version >= NS_LITERAL_STRING("2.3.6")) {
    
    
    return "libomxplugingb.so";
  }
  else if (version == 10 && release_version >= NS_LITERAL_STRING("2.3.4") &&
           device.Find("HTC") == 0) {
    
    
    return "libomxplugingb.so";
  }
  else if (version == 9 || (version == 10 && release_version <= NS_LITERAL_STRING("2.3.5"))) {
    
    
    return "libomxplugingb235.so";
  }
  else if (version < 9) {
    
    return nullptr;
  }

  
  return "libomxplugin.so";

#elif defined(ANDROID) && defined(MOZ_WIDGET_GONK)
  return "libomxplugin.so";
#else
  return nullptr;
#endif
}

AndroidMediaPluginHost::AndroidMediaPluginHost() {
  MOZ_COUNT_CTOR(AndroidMediaPluginHost);
  MOZ_ASSERT(NS_IsMainThread());

  mResourceServer = AndroidMediaResourceServer::Start();

  const char* name = GetOmxLibraryName();
  ALOG("Loading OMX Plugin: %s", name ? name : "nullptr");
  if (name) {
    char *path = PR_GetLibraryFilePathname("libxul.so", (PRFuncPtr) GetOmxLibraryName);
    PRLibrary *lib = nullptr;
    if (path) {
      nsAutoCString libpath(path);
      PR_Free(path);
      int32_t slash = libpath.RFindChar('/');
      if (slash != kNotFound) {
        libpath.Truncate(slash + 1);
        libpath.Append(name);
        lib = PR_LoadLibrary(libpath.get());
      }
    }
    if (!lib)
      lib = PR_LoadLibrary(name);

    if (lib) {
      Manifest *manifest = static_cast<Manifest *>(PR_FindSymbol(lib, "MPAPI_MANIFEST"));
      if (manifest) {
        mPlugins.AppendElement(manifest);
        ALOG("OMX plugin successfully loaded");
     }
    }
  }
}

AndroidMediaPluginHost::~AndroidMediaPluginHost() {
  mResourceServer->Stop();
  MOZ_COUNT_DTOR(AndroidMediaPluginHost);
}

bool AndroidMediaPluginHost::FindDecoder(const nsACString& aMimeType, const char* const** aCodecs)
{
  const char *chars;
  size_t len = NS_CStringGetData(aMimeType, &chars, nullptr);
  for (size_t n = 0; n < mPlugins.Length(); ++n) {
    Manifest *plugin = mPlugins[n];
    const char* const *codecs;
    if (plugin->CanDecode(chars, len, &codecs)) {
      if (aCodecs)
        *aCodecs = codecs;
      return true;
    }
  }
  return false;
}

MPAPI::Decoder *AndroidMediaPluginHost::CreateDecoder(MediaResource *aResource, const nsACString& aMimeType)
{
  NS_ENSURE_TRUE(aResource, nullptr);

  nsAutoPtr<Decoder> decoder(new Decoder());
  if (!decoder) {
    return nullptr;
  }

  const char *chars;
  size_t len = NS_CStringGetData(aMimeType, &chars, nullptr);
  for (size_t n = 0; n < mPlugins.Length(); ++n) {
    Manifest *plugin = mPlugins[n];
    const char* const *codecs;
    if (!plugin->CanDecode(chars, len, &codecs)) {
      continue;
    }

    nsCString url;
    nsresult rv = mResourceServer->AddResource(aResource, url);
    if (NS_FAILED (rv)) continue;

    decoder->mResource = strdup(url.get());
    if (plugin->CreateDecoder(&sPluginHost, decoder, chars, len)) {
      aResource->AddRef();
      return decoder.forget();
    }
  }

  return nullptr;
}

void AndroidMediaPluginHost::DestroyDecoder(Decoder *aDecoder)
{
  aDecoder->DestroyDecoder(aDecoder);
  char* resource = GetResource(aDecoder);
  if (resource) {
    
    
    mResourceServer->RemoveResource(nsCString(resource));
    free(resource);
  }
  delete aDecoder;
}

AndroidMediaPluginHost *sAndroidMediaPluginHost = nullptr;
AndroidMediaPluginHost *EnsureAndroidMediaPluginHost()
{
  MOZ_DIAGNOSTIC_ASSERT(NS_IsMainThread());
  if (!sAndroidMediaPluginHost) {
    sAndroidMediaPluginHost = new AndroidMediaPluginHost();
  }
  return sAndroidMediaPluginHost;
}

AndroidMediaPluginHost *GetAndroidMediaPluginHost()
{
  MOZ_ASSERT(sAndroidMediaPluginHost);
  return sAndroidMediaPluginHost;
}

void AndroidMediaPluginHost::Shutdown()
{
  delete sAndroidMediaPluginHost;
  sAndroidMediaPluginHost = nullptr;
}

} 

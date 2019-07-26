




#include "mozilla/Preferences.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/dom/TimeRanges.h"
#include "MediaResource.h"
#include "nsHTMLMediaElement.h"
#include "MediaPluginHost.h"
#include "nsXPCOMStrings.h"
#include "nsISeekableStream.h"
#include "pratom.h"
#include "MediaPluginReader.h"
#include "nsIGfxInfo.h"
#include "gfxCrashReporterUtils.h"
#include "prmem.h"

#include "MPAPI.h"

#if defined(ANDROID) || defined(MOZ_WIDGET_GONK)
#include "android/log.h"
#define ALOG(args...)  __android_log_print(ANDROID_LOG_INFO, "MediaPluginHost" , ## args)
#else
#define ALOG(args...)
#endif

using namespace MPAPI;

Decoder::Decoder() :
  mResource(NULL), mPrivate(NULL)
{
}

namespace mozilla {

static MediaResource *GetResource(Decoder *aDecoder)
{
  return reinterpret_cast<MediaResource *>(aDecoder->mResource);
}

static bool Read(Decoder *aDecoder, char *aBuffer, int64_t aOffset, uint32_t aCount, uint32_t* aBytes)
{
  MediaResource *resource = GetResource(aDecoder);
  if (aOffset != resource->Tell()) {
    nsresult rv = resource->Seek(nsISeekableStream::NS_SEEK_SET, aOffset);
    if (NS_FAILED(rv)) {
      return false;
    }
  }
  nsresult rv = resource->Read(aBuffer, aCount, aBytes);
  if (NS_FAILED(rv)) {
    return false;
  }
  return true;
}

static uint64_t GetLength(Decoder *aDecoder)
{
  return GetResource(aDecoder)->GetLength();
}

static void SetMetaDataReadMode(Decoder *aDecoder)
{
  GetResource(aDecoder)->SetReadMode(MediaCacheStream::MODE_METADATA);
}

static void SetPlaybackReadMode(Decoder *aDecoder)
{
  GetResource(aDecoder)->SetReadMode(MediaCacheStream::MODE_PLAYBACK);
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
  nsCOMPtr<GetIntPrefEvent> event = new GetIntPrefEvent(aPref, aResult);
  return NS_SUCCEEDED(NS_DispatchToMainThread(event, NS_DISPATCH_SYNC));
}

static PluginHost sPluginHost = {
  Read,
  GetLength,
  SetMetaDataReadMode,
  SetPlaybackReadMode,
  GetIntPref
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
        if (status != nsIGfxInfo::FEATURE_NO_INFO) {
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
  if (!IsOmxSupported())
    return nullptr;

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

  if (version == 15 &&
      (device.Find("LT28", false) == 0 ||
       device.Find("LT26", false) == 0 ||
       device.Find("LT22", false) == 0 ||
       device.Find("IS12", false) == 0 ||
       device.Find("MT27", false) == 0)) {
    
    return "libomxpluginsony.so";
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
  else if (version == 8) {
    
    return "libomxpluginfroyo.so";
  }
  else if (version < 8) {
    
    return nullptr;
  }

  
  return "libomxplugin.so";

#elif defined(ANDROID) && defined(MOZ_WIDGET_GONK)
  return "libomxplugin.so";
#else
  return nullptr;
#endif
}

MediaPluginHost::MediaPluginHost() {
  MOZ_COUNT_CTOR(MediaPluginHost);

  const char* name = GetOmxLibraryName();
  ALOG("Loading OMX Plugin: %s", name ? name : "nullptr");
  if (name) {
    char *path = PR_GetLibraryFilePathname("libxul.so", (PRFuncPtr) GetOmxLibraryName);
    PRLibrary *lib = NULL;
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

MediaPluginHost::~MediaPluginHost() {
  MOZ_COUNT_DTOR(MediaPluginHost);
}

bool MediaPluginHost::FindDecoder(const nsACString& aMimeType, const char* const** aCodecs)
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

MPAPI::Decoder *MediaPluginHost::CreateDecoder(MediaResource *aResource, const nsACString& aMimeType)
{
  const char *chars;
  size_t len = NS_CStringGetData(aMimeType, &chars, nullptr);

  Decoder *decoder = new Decoder();
  if (!decoder) {
    return nullptr;
  }
  decoder->mResource = aResource;

  for (size_t n = 0; n < mPlugins.Length(); ++n) {
    Manifest *plugin = mPlugins[n];
    const char* const *codecs;
    if (!plugin->CanDecode(chars, len, &codecs)) {
      continue;
    }
    if (plugin->CreateDecoder(&sPluginHost, decoder, chars, len)) {
      return decoder;
    }
  }

  return nullptr;
}

void MediaPluginHost::DestroyDecoder(Decoder *aDecoder)
{
  aDecoder->DestroyDecoder(aDecoder);
  delete aDecoder;
}

MediaPluginHost *sMediaPluginHost = nullptr;
MediaPluginHost *GetMediaPluginHost()
{
  if (!sMediaPluginHost) {
    sMediaPluginHost = new MediaPluginHost();
  }
  return sMediaPluginHost;
}

void MediaPluginHost::Shutdown()
{
  if (sMediaPluginHost) {
    delete sMediaPluginHost;
    sMediaPluginHost = nullptr;
  }
}

} 

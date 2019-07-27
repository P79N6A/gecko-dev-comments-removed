







#include "mozilla/Logging.h"

#include "nsFontFaceLoader.h"

#include "nsError.h"
#include "nsContentUtils.h"
#include "mozilla/Preferences.h"
#include "FontFaceSet.h"
#include "nsPresContext.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsIHttpChannel.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"

#include "mozilla/gfx/2D.h"

using namespace mozilla;

#define LOG(args) MOZ_LOG(gfxUserFontSet::GetUserFontsLog(), mozilla::LogLevel::Debug, args)
#define LOG_ENABLED() MOZ_LOG_TEST(gfxUserFontSet::GetUserFontsLog(), \
                                  LogLevel::Debug)

nsFontFaceLoader::nsFontFaceLoader(gfxUserFontEntry* aUserFontEntry,
                                   nsIURI* aFontURI,
                                   mozilla::dom::FontFaceSet* aFontFaceSet,
                                   nsIChannel* aChannel)
  : mUserFontEntry(aUserFontEntry),
    mFontURI(aFontURI),
    mFontFaceSet(aFontFaceSet),
    mChannel(aChannel)
{
}

nsFontFaceLoader::~nsFontFaceLoader()
{
  if (mUserFontEntry) {
    mUserFontEntry->mLoader = nullptr;
  }
  if (mLoadTimer) {
    mLoadTimer->Cancel();
    mLoadTimer = nullptr;
  }
  if (mFontFaceSet) {
    mFontFaceSet->RemoveLoader(this);
  }
}

void
nsFontFaceLoader::StartedLoading(nsIStreamLoader* aStreamLoader)
{
  int32_t loadTimeout =
    Preferences::GetInt("gfx.downloadable_fonts.fallback_delay", 3000);
  if (loadTimeout > 0) {
    mLoadTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mLoadTimer) {
      mLoadTimer->InitWithFuncCallback(LoadTimerCallback,
                                       static_cast<void*>(this),
                                       loadTimeout,
                                       nsITimer::TYPE_ONE_SHOT);
    }
  } else {
    mUserFontEntry->mFontDataLoadingState = gfxUserFontEntry::LOADING_SLOWLY;
  }
  mStreamLoader = aStreamLoader;
}

void
nsFontFaceLoader::LoadTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsFontFaceLoader* loader = static_cast<nsFontFaceLoader*>(aClosure);

  if (!loader->mFontFaceSet) {
    
    return;
  }

  gfxUserFontEntry* ufe = loader->mUserFontEntry.get();
  bool updateUserFontSet = true;

  
  
  if (ufe->mFontDataLoadingState == gfxUserFontEntry::LOADING_STARTED) {
    int64_t contentLength;
    uint32_t numBytesRead;
    if (NS_SUCCEEDED(loader->mChannel->GetContentLength(&contentLength)) &&
        contentLength > 0 &&
        contentLength < UINT32_MAX &&
        NS_SUCCEEDED(loader->mStreamLoader->GetNumBytesRead(&numBytesRead)) &&
        numBytesRead > 3 * (uint32_t(contentLength) >> 2))
    {
      
      
      
      ufe->mFontDataLoadingState = gfxUserFontEntry::LOADING_ALMOST_DONE;
      uint32_t delay;
      loader->mLoadTimer->GetDelay(&delay);
      loader->mLoadTimer->InitWithFuncCallback(LoadTimerCallback,
                                               static_cast<void*>(loader),
                                               delay >> 1,
                                               nsITimer::TYPE_ONE_SHOT);
      updateUserFontSet = false;
      LOG(("userfonts (%p) 75%% done, resetting timer\n", loader));
    }
  }

  
  
  
  if (updateUserFontSet) {
    ufe->mFontDataLoadingState = gfxUserFontEntry::LOADING_SLOWLY;
    nsPresContext* ctx = loader->mFontFaceSet->GetPresContext();
    if (ctx) {
      loader->mFontFaceSet->IncrementGeneration();
      ctx->UserFontSetUpdated(loader->GetUserFontEntry());
      LOG(("userfonts (%p) timeout reflow\n", loader));
    }
  }
}

NS_IMPL_ISUPPORTS(nsFontFaceLoader, nsIStreamLoaderObserver)

NS_IMETHODIMP
nsFontFaceLoader::OnStreamComplete(nsIStreamLoader* aLoader,
                                   nsISupports* aContext,
                                   nsresult aStatus,
                                   uint32_t aStringLen,
                                   const uint8_t* aString)
{
  if (!mFontFaceSet) {
    
    return aStatus;
  }

  mFontFaceSet->RemoveLoader(this);

  if (LOG_ENABLED()) {
    nsAutoCString fontURI;
    mFontURI->GetSpec(fontURI);
    if (NS_SUCCEEDED(aStatus)) {
      LOG(("userfonts (%p) download completed - font uri: (%s)\n",
           this, fontURI.get()));
    } else {
      LOG(("userfonts (%p) download failed - font uri: (%s) error: %8.8x\n",
           this, fontURI.get(), aStatus));
    }
  }

  if (NS_SUCCEEDED(aStatus)) {
    
    
    
    
    
    nsCOMPtr<nsIRequest> request;
    nsCOMPtr<nsIHttpChannel> httpChannel;
    aLoader->GetRequest(getter_AddRefs(request));
    httpChannel = do_QueryInterface(request);
    if (httpChannel) {
      bool succeeded;
      nsresult rv = httpChannel->GetRequestSucceeded(&succeeded);
      if (NS_SUCCEEDED(rv) && !succeeded) {
        aStatus = NS_ERROR_NOT_AVAILABLE;
      }
    }
  }

  
  
  
  
  
  
  nsPresContext* ctx = mFontFaceSet->GetPresContext();
  bool fontUpdate =
    mUserFontEntry->FontDataDownloadComplete(aString, aStringLen, aStatus);

  
  if (fontUpdate && ctx) {
    
    
    ctx->UserFontSetUpdated(mUserFontEntry);
    LOG(("userfonts (%p) reflow\n", this));
  }

  
  mFontFaceSet = nullptr;
  if (mLoadTimer) {
    mLoadTimer->Cancel();
    mLoadTimer = nullptr;
  }

  return NS_SUCCESS_ADOPTED_DATA;
}

void
nsFontFaceLoader::Cancel()
{
  mUserFontEntry->mFontDataLoadingState = gfxUserFontEntry::NOT_LOADING;
  mUserFontEntry->mLoader = nullptr;
  mFontFaceSet = nullptr;
  if (mLoadTimer) {
    mLoadTimer->Cancel();
    mLoadTimer = nullptr;
  }
  mChannel->Cancel(NS_BINDING_ABORTED);
}

nsresult
nsFontFaceLoader::CheckLoadAllowed(nsIPrincipal* aSourcePrincipal,
                                   nsIURI* aTargetURI,
                                   nsISupports* aContext)
{
  nsresult rv;

  if (!aSourcePrincipal)
    return NS_OK;

  
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  rv = secMan->CheckLoadURIWithPrincipal(aSourcePrincipal, aTargetURI,
                                        nsIScriptSecurityManager::STANDARD);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_FONT,
                                 aTargetURI,
                                 aSourcePrincipal,
                                 aContext,
                                 EmptyCString(), 
                                 nullptr,
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());

  if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
    return NS_ERROR_CONTENT_BLOCKED;
  }

  return NS_OK;
}

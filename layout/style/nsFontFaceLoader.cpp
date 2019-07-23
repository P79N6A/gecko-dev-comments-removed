








































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "nsFontFaceLoader.h"

#include "nsError.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsIStreamListener.h"
#include "nsNetUtil.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsContentUtils.h"

#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"

#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsContentErrors.h"
#include "nsCrossSiteListenerProxy.h"

#ifdef PR_LOGGING
static PRLogModuleInfo *gFontDownloaderLog = PR_NewLogModule("fontdownloader");
#endif 

#define LOG(args) PR_LOG(gFontDownloaderLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gFontDownloaderLog, PR_LOG_DEBUG)


nsFontFaceLoader::nsFontFaceLoader(gfxFontEntry *aFontToLoad, nsIURI *aFontURI,
                                   nsUserFontSet *aFontSet, nsIChannel *aChannel)
  : mFontEntry(aFontToLoad), mFontURI(aFontURI), mFontSet(aFontSet),
    mChannel(aChannel)
{
}

nsFontFaceLoader::~nsFontFaceLoader()
{
  if (mFontSet) {
    mFontSet->RemoveLoader(this);
  }
}

NS_IMPL_ISUPPORTS1(nsFontFaceLoader, nsIStreamLoaderObserver)

NS_IMETHODIMP
nsFontFaceLoader::OnStreamComplete(nsIStreamLoader* aLoader,
                                   nsISupports* aContext,
                                   nsresult aStatus,
                                   PRUint32 aStringLen,
                                   const PRUint8* aString)
{
  if (!mFontSet) {
    
    return aStatus;
  }

  mFontSet->RemoveLoader(this);

#ifdef PR_LOGGING
  if (LOG_ENABLED()) {
    nsCAutoString fontURI;
    mFontURI->GetSpec(fontURI);
    if (NS_SUCCEEDED(aStatus)) {
      LOG(("fontdownloader (%p) download completed - font uri: (%s)\n", 
           this, fontURI.get()));
    } else {
      LOG(("fontdownloader (%p) download failed - font uri: (%s) error: %8.8x\n", 
           this, fontURI.get(), aStatus));
    }
  }
#endif

  nsPresContext *ctx = mFontSet->GetPresContext();
  NS_ASSERTION(ctx && !ctx->PresShell()->IsDestroying(),
               "We should have been canceled already");

  
  gfxUserFontSet *userFontSet = ctx->GetUserFontSet();
  if (!userFontSet) {
    return aStatus;
  }

  
  
  
  PRBool fontUpdate = userFontSet->OnLoadComplete(mFontEntry,
                                                  aString, aStringLen,
                                                  aStatus);

  
  if (fontUpdate) {
    
    
    ctx->UserFontSetUpdated();
    LOG(("fontdownloader (%p) reflow\n", this));
  }

  return NS_SUCCESS_ADOPTED_DATA;
}

void
nsFontFaceLoader::Cancel()
{
  mFontSet = nsnull;
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
    
  
  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_FONT,
                                 aTargetURI,
                                 aSourcePrincipal,
                                 aContext,
                                 EmptyCString(), 
                                 nsnull,
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());

  if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
    return NS_ERROR_CONTENT_BLOCKED;
  }

  return NS_OK;
}
  
nsUserFontSet::nsUserFontSet(nsPresContext *aContext)
  : mPresContext(aContext)
{
  NS_ASSERTION(mPresContext, "null context passed to nsUserFontSet");
  mLoaders.Init();
}

nsUserFontSet::~nsUserFontSet()
{
  NS_ASSERTION(mLoaders.Count() == 0, "mLoaders should have been emptied");
}

static PLDHashOperator DestroyIterator(nsPtrHashKey<nsFontFaceLoader>* aKey,
                                       void* aUserArg)
{
  aKey->GetKey()->Cancel();
  return PL_DHASH_REMOVE;
}

void
nsUserFontSet::Destroy()
{
  mPresContext = nsnull;
  mLoaders.EnumerateEntries(DestroyIterator, nsnull);
}

void
nsUserFontSet::RemoveLoader(nsFontFaceLoader *aLoader)
{
  mLoaders.RemoveEntry(aLoader);
}

nsresult 
nsUserFontSet::StartLoad(gfxFontEntry *aFontToLoad, 
                          const gfxFontFaceSrc *aFontFaceSrc)
{
  nsresult rv;
  
  
  nsIPresShell *ps = mPresContext->PresShell();
  if (!ps)
    return NS_ERROR_FAILURE;
    
  NS_ASSERTION(aFontFaceSrc && !aFontFaceSrc->mIsLocal, 
               "bad font face url passed to fontloader");
  NS_ASSERTION(aFontFaceSrc->mURI, "null font uri");
  if (!aFontFaceSrc->mURI)
    return NS_ERROR_FAILURE;

  
  
  
  nsCOMPtr<nsIPrincipal> principal = ps->GetDocument()->NodePrincipal();

  NS_ASSERTION(aFontFaceSrc->mOriginPrincipal, 
               "null origin principal in @font-face rule");
  if (aFontFaceSrc->mUseOriginPrincipal) {
    principal = do_QueryInterface(aFontFaceSrc->mOriginPrincipal);
  }
  
  rv = nsFontFaceLoader::CheckLoadAllowed(principal, aFontFaceSrc->mURI, 
                                          ps->GetDocument());
  if (NS_FAILED(rv)) {
#ifdef PR_LOGGING
    if (LOG_ENABLED()) {
      nsCAutoString fontURI, referrerURI;
      aFontFaceSrc->mURI->GetSpec(fontURI);
      if (aFontFaceSrc->mReferrer)
        aFontFaceSrc->mReferrer->GetSpec(referrerURI);
      LOG(("fontdownloader download blocked - font uri: (%s) "
           "referrer uri: (%s) err: %8.8x\n", 
          fontURI.get(), referrerURI.get(), rv));
    }
#endif    
    return rv;
  }

  nsCOMPtr<nsIStreamLoader> streamLoader;
  nsCOMPtr<nsILoadGroup> loadGroup(ps->GetDocument()->GetDocumentLoadGroup());

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel),
                     aFontFaceSrc->mURI,
                     nsnull,
                     loadGroup,
                     nsnull,
                     nsIRequest::LOAD_NORMAL);
                     
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsFontFaceLoader> fontLoader =
    new nsFontFaceLoader(aFontToLoad, aFontFaceSrc->mURI, this, channel);

  if (!fontLoader)
    return NS_ERROR_OUT_OF_MEMORY;

#ifdef PR_LOGGING
  if (LOG_ENABLED()) {
    nsCAutoString fontURI, referrerURI;
    aFontFaceSrc->mURI->GetSpec(fontURI);
    if (aFontFaceSrc->mReferrer)
      aFontFaceSrc->mReferrer->GetSpec(referrerURI);
    LOG(("fontdownloader (%p) download start - font uri: (%s) "
         "referrer uri: (%s)\n", 
         fontLoader.get(), fontURI.get(), referrerURI.get()));
  }
#endif  

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (httpChannel)
    httpChannel->SetReferrer(aFontFaceSrc->mReferrer);
  rv = NS_NewStreamLoader(getter_AddRefs(streamLoader), fontLoader);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRBool inherits = PR_FALSE;
  rv = NS_URIChainHasFlags(aFontFaceSrc->mURI,
                           nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT,
                           &inherits);
  if (NS_SUCCEEDED(rv) && inherits) {
    
    rv = channel->AsyncOpen(streamLoader, nsnull);
  } else {
    nsCOMPtr<nsIStreamListener> listener =
      new nsCrossSiteListenerProxy(streamLoader, principal, channel, 
                                   PR_FALSE, &rv);
    if (NS_FAILED(rv)) {
      fontLoader->DropChannel();  
    }
    NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = channel->AsyncOpen(listener, nsnull);
  }

  if (NS_SUCCEEDED(rv)) {
    mLoaders.PutEntry(fontLoader);
  }

  return rv;
}










































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "nsFontFaceLoader.h"

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


#ifdef PR_LOGGING
static PRLogModuleInfo *gFontDownloaderLog = PR_NewLogModule("fontdownloader");
#endif 

#define LOG(args) PR_LOG(gFontDownloaderLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gFontDownloaderLog, PR_LOG_DEBUG)


#define ENFORCE_SAME_SITE_ORIGIN "gfx.downloadable_fonts.enforce_same_site_origin"
static PRBool gEnforceSameSiteOrigin = PR_TRUE;

static PRBool
CheckMayLoad(nsIDocument* aDoc, nsIURI* aURI)
{
  
  static PRBool init = PR_FALSE;

  if (!init) {
    init = PR_TRUE;
    nsContentUtils::AddBoolPrefVarCache(ENFORCE_SAME_SITE_ORIGIN, &gEnforceSameSiteOrigin);
  }

  if (!gEnforceSameSiteOrigin)
    return PR_TRUE;

  if (!aDoc)
    return PR_FALSE;

  nsresult rv = aDoc->NodePrincipal()->CheckMayLoad(aURI, PR_TRUE);
  return NS_SUCCEEDED(rv);
}


nsFontFaceLoader::nsFontFaceLoader(gfxFontEntry *aFontToLoad, nsIURI *aFontURI,
                                   gfxUserFontSet::LoaderContext *aContext)
  : mFontEntry(aFontToLoad), mFontURI(aFontURI), mLoaderContext(aContext)
{

}

nsFontFaceLoader::~nsFontFaceLoader()
{

}

NS_IMPL_ISUPPORTS1(nsFontFaceLoader, nsIStreamLoaderObserver)

NS_IMETHODIMP
nsFontFaceLoader::OnStreamComplete(nsIStreamLoader* aLoader,
                                   nsISupports* aContext,
                                   nsresult aStatus,
                                   PRUint32 aStringLen,
                                   const PRUint8* aString)
{

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

  PRBool fontUpdate;

  
  fontUpdate = mLoaderContext->mUserFontSet->OnLoadComplete(mFontEntry, 
                                                            aString, aStringLen,
                                                            aStatus);

  
  if (fontUpdate) {
    nsFontFaceLoaderContext *loaderCtx 
                       = static_cast<nsFontFaceLoaderContext*> (mLoaderContext);

    nsIPresShell *ps = loaderCtx->mPresContext->PresShell();
    if (ps) {
      
      ps->StyleChangeReflow();
      LOG(("fontdownloader (%p) reflow\n", this));
    }
  }

  return aStatus;
}

PRBool
nsFontFaceLoader::CreateHandler(gfxFontEntry *aFontToLoad, nsIURI *aFontURI, 
                                gfxUserFontSet::LoaderContext *aContext)
{
  
  nsFontFaceLoaderContext *loaderCtx 
                             = static_cast<nsFontFaceLoaderContext*> (aContext);

  nsIPresShell *ps = loaderCtx->mPresContext->PresShell();
  if (!ps)
    return PR_FALSE;

  if (!CheckMayLoad(ps->GetDocument(), aFontURI))
    return PR_FALSE;

  nsRefPtr<nsFontFaceLoader> fontLoader = new nsFontFaceLoader(aFontToLoad, 
                                                               aFontURI, 
                                                               aContext);
  if (!fontLoader)
    return PR_FALSE;

#ifdef PR_LOGGING
  if (LOG_ENABLED()) {
    nsCAutoString fontURI;
    aFontURI->GetSpec(fontURI);
    LOG(("fontdownloader (%p) download start - font uri: (%s)\n", 
         fontLoader.get(), fontURI.get()));
  }
#endif  

  nsCOMPtr<nsIStreamLoader> streamLoader;
  nsCOMPtr<nsILoadGroup> loadGroup(ps->GetDocument()->GetDocumentLoadGroup());
  nsCOMPtr<nsIInterfaceRequestor> sameOriginChecker 
                                       = nsContentUtils::GetSameOriginChecker();

  nsresult rv = NS_NewStreamLoader(getter_AddRefs(streamLoader), aFontURI, 
                                   fontLoader, nsnull, loadGroup, 
                                   sameOriginChecker);

  return NS_SUCCEEDED(rv);
}











































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

NS_IMPL_ISUPPORTS1(nsFontFaceLoader, nsIDownloadObserver)

static nsresult
MakeTempFileName(nsIFile** tempFile)
{
  nsresult rv;

  rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, tempFile);
  NS_ENSURE_SUCCESS(rv, rv);

  
  static PRUint16 count = 0;
  PRTime now = PR_Now();
  PRUint32 current = (PRUint32) now;

  ++count;
  char buf[256];
  sprintf(buf, "mozfont_%8.8x%4.4x.ttf", current, count);

  rv = (*tempFile)->AppendNative(nsDependentCString(buf));
  NS_ENSURE_SUCCESS(rv, rv);

  return (*tempFile)->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
}


nsresult 
nsFontFaceLoader::Init()
{
#ifdef PR_LOGGING
  if (LOG_ENABLED()) {
    nsCAutoString fontURI;
    mFontURI->GetSpec(fontURI);
    LOG(("fontdownloader (%p) download start - font uri: (%s)\n", 
         this, fontURI.get()));
  }
#endif  

  nsresult rv;

  nsCOMPtr<nsIFile> tempFile;
  rv = MakeTempFileName(getter_AddRefs(tempFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewDownloader(getter_AddRefs(mDownloader), this, tempFile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInterfaceRequestor> sameOriginChecker 
                                       = nsContentUtils::GetSameOriginChecker();

  rv = NS_OpenURI(mDownloader, nsnull, mFontURI, nsnull, nsnull, 
                  sameOriginChecker);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsFontFaceLoader::OnDownloadComplete(nsIDownloader *aDownloader,
                                     nsIRequest   *aRequest,
                                     nsISupports  *aContext,
                                     nsresult     aStatus,
                                     nsIFile      *aFile)
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

  if (NS_SUCCEEDED(aStatus) && aFile) {
    
    mFaceData.mFormatFlags = 0;
    mFaceData.mFontFile = aFile;
    mFaceData.mDownloader = aDownloader;
  }

  
  fontUpdate = mLoaderContext->mUserFontSet->OnLoadComplete(mFontEntry, 
                                                            mFaceData, 
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

  return NS_OK;
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

  nsRefPtr<nsFontFaceLoader> loader = new nsFontFaceLoader(aFontToLoad, 
                                                           aFontURI, 
                                                           aContext);
  if (!loader)
    return PR_FALSE;

  nsresult rv = loader->Init();
  return NS_SUCCEEDED(rv);
}






































#include "nsLayoutStylesheetCache.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsICSSLoader.h"
#include "nsIFile.h"
#include "nsLayoutCID.h"
#include "nsNetUtil.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"

NS_IMPL_ISUPPORTS1(nsLayoutStylesheetCache, nsIObserver)

nsresult
nsLayoutStylesheetCache::Observe(nsISupports* aSubject,
                            const char* aTopic,
                            const PRUnichar* aData)
{
  if (!strcmp(aTopic, "profile-before-change")) {
    mUserContentSheet = nsnull;
    mUserChromeSheet  = nsnull;
  }
  else if (!strcmp(aTopic, "profile-do-change")) {
    InitFromProfile();
  }
  else if (strcmp(aTopic, "chrome-flush-skin-caches") == 0 ||
           strcmp(aTopic, "chrome-flush-caches") == 0) {
    mScrollbarsSheet = nsnull;
    mFormsSheet = nsnull;
  }
  else {
    NS_NOTREACHED("Unexpected observer topic.");
  }
  return NS_OK;
}

nsICSSStyleSheet*
nsLayoutStylesheetCache::ScrollbarsSheet()
{
  EnsureGlobal();
  if (!gStyleCache)
    return nsnull;

  if (!gStyleCache->mScrollbarsSheet) {
    nsCOMPtr<nsIURI> sheetURI;
    NS_NewURI(getter_AddRefs(sheetURI),
#ifdef XP_MACOSX
              NS_LITERAL_CSTRING("chrome://global/skin/nativescrollbars.css"));
#else
              NS_LITERAL_CSTRING("chrome://global/skin/xulscrollbars.css"));
#endif


    if (sheetURI)
      LoadSheet(sheetURI, gStyleCache->mScrollbarsSheet, PR_FALSE);
#ifdef XP_MACOSX
    NS_ASSERTION(gStyleCache->mScrollbarsSheet, "Could not load nativescrollbars.css.");
#else
    NS_ASSERTION(gStyleCache->mScrollbarsSheet, "Could not load xulscrollbars.css.");
#endif
  }

  return gStyleCache->mScrollbarsSheet;
}

nsICSSStyleSheet*
nsLayoutStylesheetCache::FormsSheet()
{
  EnsureGlobal();
  if (!gStyleCache)
    return nsnull;

  if (!gStyleCache->mFormsSheet) {
    nsCOMPtr<nsIURI> sheetURI;
      NS_NewURI(getter_AddRefs(sheetURI),
                NS_LITERAL_CSTRING("resource://gre/res/forms.css"));

    
    if (sheetURI)
      LoadSheet(sheetURI, gStyleCache->mFormsSheet, PR_TRUE);

    NS_ASSERTION(gStyleCache->mFormsSheet, "Could not load forms.css.");
  }

  return gStyleCache->mFormsSheet;
}

nsICSSStyleSheet*
nsLayoutStylesheetCache::UserContentSheet()
{
  EnsureGlobal();
  if (!gStyleCache)
    return nsnull;

  return gStyleCache->mUserContentSheet;
}

nsICSSStyleSheet*
nsLayoutStylesheetCache::UserChromeSheet()
{
  EnsureGlobal();
  if (!gStyleCache)
    return nsnull;

  return gStyleCache->mUserChromeSheet;
}

void
nsLayoutStylesheetCache::Shutdown()
{
  NS_IF_RELEASE(gCSSLoader);
  NS_IF_RELEASE(gStyleCache);
}

nsLayoutStylesheetCache::nsLayoutStylesheetCache()
{
  nsCOMPtr<nsIObserverService> obsSvc =
    do_GetService("@mozilla.org/observer-service;1");
  NS_ASSERTION(obsSvc, "No global observer service?");

  if (obsSvc) {
    obsSvc->AddObserver(this, "profile-before-change", PR_FALSE);
    obsSvc->AddObserver(this, "profile-do-change", PR_FALSE);
    obsSvc->AddObserver(this, "chrome-flush-skin-caches", PR_FALSE);
    obsSvc->AddObserver(this, "chrome-flush-caches", PR_FALSE);
  }

  InitFromProfile();
}

nsLayoutStylesheetCache::~nsLayoutStylesheetCache()
{
  gCSSLoader = nsnull;
  gStyleCache = nsnull;
}

void
nsLayoutStylesheetCache::EnsureGlobal()
{
  if (gStyleCache) return;

  gStyleCache = new nsLayoutStylesheetCache();
  if (!gStyleCache) return;

  NS_ADDREF(gStyleCache);
}

void
nsLayoutStylesheetCache::InitFromProfile()
{
  nsCOMPtr<nsIFile> contentFile;
  nsCOMPtr<nsIFile> chromeFile;

  NS_GetSpecialDirectory(NS_APP_USER_CHROME_DIR,
                         getter_AddRefs(contentFile));
  if (!contentFile) {
    
    return;
  }

  contentFile->Clone(getter_AddRefs(chromeFile));
  if (!chromeFile) return;

  contentFile->Append(NS_LITERAL_STRING("userContent.css"));
  chromeFile->Append(NS_LITERAL_STRING("userChrome.css"));

  LoadSheetFile(contentFile, mUserContentSheet);
  LoadSheetFile(chromeFile, mUserChromeSheet);
}

void
nsLayoutStylesheetCache::LoadSheetFile(nsIFile* aFile, nsCOMPtr<nsICSSStyleSheet> &aSheet)
{
  PRBool exists = PR_FALSE;
  aFile->Exists(&exists);

  if (!exists) return;

  nsCOMPtr<nsIURI> uri;
  NS_NewFileURI(getter_AddRefs(uri), aFile);

  LoadSheet(uri, aSheet, PR_FALSE);
}

void
nsLayoutStylesheetCache::LoadSheet(nsIURI* aURI, nsCOMPtr<nsICSSStyleSheet> &aSheet,
                                   PRBool aEnableUnsafeRules)
{
  if (!aURI) {
    NS_ERROR("Null URI. Out of memory?");
    return;
  }

  if (!gCSSLoader)
    NS_NewCSSLoader(&gCSSLoader);

  if (gCSSLoader) {
    gCSSLoader->LoadSheetSync(aURI, aEnableUnsafeRules, getter_AddRefs(aSheet));
  }
}  

nsLayoutStylesheetCache*
nsLayoutStylesheetCache::gStyleCache = nsnull;

nsICSSLoader*
nsLayoutStylesheetCache::gCSSLoader = nsnull;

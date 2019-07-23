




































#include "nsLayoutStylesheetCache.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsICSSLoader.h"
#include "nsIFile.h"
#include "nsLayoutCID.h"
#include "nsNetUtil.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsIXULRuntime.h"

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
              NS_LITERAL_CSTRING("chrome://global/skin/scrollbars.css"));

    
    if (sheetURI)
      LoadSheet(sheetURI, gStyleCache->mScrollbarsSheet, PR_FALSE);
    NS_ASSERTION(gStyleCache->mScrollbarsSheet, "Could not load scrollbars.css.");
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
                NS_LITERAL_CSTRING("resource://gre-resources/forms.css"));

    
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

nsICSSStyleSheet*
nsLayoutStylesheetCache::UASheet()
{
  EnsureGlobal();
  if (!gStyleCache)
    return nsnull;

  return gStyleCache->mUASheet;
}

nsICSSStyleSheet*
nsLayoutStylesheetCache::QuirkSheet()
{
  EnsureGlobal();
  if (!gStyleCache)
    return nsnull;

  return gStyleCache->mQuirkSheet;
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

  
  
  nsCOMPtr<nsIURI> uri;
  NS_NewURI(getter_AddRefs(uri), "resource://gre-resources/ua.css");
  if (uri) {
    LoadSheet(uri, mUASheet, PR_TRUE);
  }
  NS_ASSERTION(mUASheet, "Could not load ua.css");

  NS_NewURI(getter_AddRefs(uri), "resource://gre-resources/quirk.css");
  if (uri) {
    LoadSheet(uri, mQuirkSheet, PR_TRUE);
  }
  NS_ASSERTION(mQuirkSheet, "Could not load quirk.css");
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
  nsCOMPtr<nsIXULRuntime> appInfo = do_GetService("@mozilla.org/xre/app-info;1");
  if (appInfo) {
    PRBool inSafeMode = PR_FALSE;
    appInfo->GetInSafeMode(&inSafeMode);
    if (inSafeMode)
      return;
  }
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
    gCSSLoader->LoadSheetSync(aURI, aEnableUnsafeRules, PR_TRUE,
                              getter_AddRefs(aSheet));
  }
}  

nsLayoutStylesheetCache*
nsLayoutStylesheetCache::gStyleCache = nsnull;

nsICSSLoader*
nsLayoutStylesheetCache::gCSSLoader = nsnull;

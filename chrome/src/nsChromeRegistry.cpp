








































#include "nsChromeRegistry.h"

#include <string.h>

#include "prio.h"
#include "prprf.h"
#if defined(XP_WIN)
#include <windows.h>
#elif defined(XP_MACOSX)
#include <CoreServices/CoreServices.h>
#elif defined(MOZ_WIDGET_GTK2)
#include <gtk/gtk.h>
#endif

#include "nsAppDirectoryServiceDefs.h"
#include "nsArrayEnumerator.h"
#include "nsComponentManager.h"
#include "nsStringEnumerator.h"
#include "nsEnumeratorUtils.h"
#include "nsCOMPtr.h"
#include "nsDOMError.h"
#include "nsEscape.h"
#include "nsInt64.h"
#include "nsLayoutCID.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsWidgetsCID.h"
#include "nsXPCOMCIDInternal.h"
#include "nsXPIDLString.h"
#include "nsXULAppAPI.h"
#include "nsTextFormatter.h"

#include "nsIAtom.h"
#include "nsICommandLine.h"
#include "nsCSSStyleSheet.h"
#include "nsIConsoleService.h"
#include "nsIDirectoryService.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDocShell.h"
#include "nsIDocumentObserver.h"
#include "nsIDOMElement.h"
#include "nsIDOMLocation.h"
#include "nsIDOMWindowCollection.h"
#include "nsIDOMWindowInternal.h"
#include "nsIFileChannel.h"
#include "nsIFileURL.h"
#include "nsIIOService.h"
#include "nsIJARProtocolHandler.h"
#include "nsIJARURI.h"
#include "nsILocalFile.h"
#include "nsILocaleService.h"
#include "nsILookAndFeel.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIPresShell.h"
#include "nsIProtocolHandler.h"
#include "nsIResProtocolHandler.h"
#include "nsIScriptError.h"
#include "nsIServiceManager.h"
#include "nsISimpleEnumerator.h"
#include "nsIStyleSheet.h"
#include "nsISupportsArray.h"
#include "nsIVersionComparator.h"
#include "nsIWindowMediator.h"
#include "nsIXPConnect.h"
#include "nsIXULAppInfo.h"
#include "nsIXULRuntime.h"

#define UILOCALE_CMD_LINE_ARG "UILocale"

#define MATCH_OS_LOCALE_PREF "intl.locale.matchOS"
#define SELECTED_LOCALE_PREF "general.useragent.locale"
#define SELECTED_SKIN_PREF   "general.skins.selectedSkin"

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

nsChromeRegistry* nsChromeRegistry::gChromeRegistry;



static void
LogMessage(const char* aMsg, ...)
{
  nsCOMPtr<nsIConsoleService> console 
    (do_GetService(NS_CONSOLESERVICE_CONTRACTID));
  if (!console)
    return;

  va_list args;
  va_start(args, aMsg);
  char* formatted = PR_vsmprintf(aMsg, args);
  va_end(args);
  if (!formatted)
    return;

  console->LogStringMessage(NS_ConvertUTF8toUTF16(formatted).get());
  PR_smprintf_free(formatted);
}

static void
LogMessageWithContext(nsIURI* aURL, PRUint32 aLineNumber, PRUint32 flags,
                      const char* aMsg, ...)
{
  nsresult rv;

  nsCOMPtr<nsIConsoleService> console 
    (do_GetService(NS_CONSOLESERVICE_CONTRACTID));

  nsCOMPtr<nsIScriptError> error
    (do_CreateInstance(NS_SCRIPTERROR_CONTRACTID));
  if (!console || !error)
    return;

  va_list args;
  va_start(args, aMsg);
  char* formatted = PR_vsmprintf(aMsg, args);
  va_end(args);
  if (!formatted)
    return;

  nsCString spec;
  if (aURL)
    aURL->GetSpec(spec);

  rv = error->Init(NS_ConvertUTF8toUTF16(formatted).get(),
                   NS_ConvertUTF8toUTF16(spec).get(),
                   nsnull,
                   aLineNumber, 0, flags, "chrome registration");
  PR_smprintf_free(formatted);

  if (NS_FAILED(rv))
    return;

  console->LogMessage(error);
}













static PRBool
LanguagesMatch(const nsACString& a, const nsACString& b)
{
  if (a.Length() < 2 || b.Length() < 2)
    return PR_FALSE;

  nsACString::const_iterator as, ae, bs, be;
  a.BeginReading(as);
  a.EndReading(ae);
  b.BeginReading(bs);
  b.EndReading(be);

  while (*as == *bs) {
    if (*as == '-')
      return PR_TRUE;
 
    ++as; ++bs;

    
    if (as == ae && bs == be)
      return PR_TRUE;

    
    if (as == ae)
      return (*bs == '-');

    
    if (bs == be)
      return (*as == '-');
  }

  return PR_FALSE;
}

static PRBool
CanLoadResource(nsIURI* aResourceURI)
{
  PRBool isLocalResource = PR_FALSE;
  (void)NS_URIChainHasFlags(aResourceURI,
                            nsIProtocolHandler::URI_IS_LOCAL_RESOURCE,
                            &isLocalResource);
  return isLocalResource;
}

nsChromeRegistry::ProviderEntry*
nsChromeRegistry::nsProviderArray::GetProvider(const nsACString& aPreferred, MatchType aType)
{
  PRInt32 i = mArray.Count();
  if (!i)
    return nsnull;

  ProviderEntry* found = nsnull;  
  ProviderEntry* entry;

  while (i--) {
    entry = reinterpret_cast<ProviderEntry*>(mArray[i]);
    if (aPreferred.Equals(entry->provider))
      return entry;

    if (aType != LOCALE)
      continue;

    if (LanguagesMatch(aPreferred, entry->provider)) {
      found = entry;
      continue;
    }

    if (!found && entry->provider.EqualsLiteral("en-US"))
      found = entry;
  }

  if (!found && aType != EXACT)
    return entry;

  return found;
}

nsIURI*
nsChromeRegistry::nsProviderArray::GetBase(const nsACString& aPreferred, MatchType aType)
{
  ProviderEntry* provider = GetProvider(aPreferred, aType);

  if (!provider)
    return nsnull;

  return provider->baseURI;
}

const nsACString&
nsChromeRegistry::nsProviderArray::GetSelected(const nsACString& aPreferred, MatchType aType)
{
  ProviderEntry* entry = GetProvider(aPreferred, aType);

  if (entry)
    return entry->provider;

  return EmptyCString();
}

void
nsChromeRegistry::nsProviderArray::SetBase(const nsACString& aProvider, nsIURI* aBaseURL)
{
  ProviderEntry* provider = GetProvider(aProvider, EXACT);

  if (provider) {
    provider->baseURI = aBaseURL;
    return;
  }

  
  provider = new ProviderEntry(aProvider, aBaseURL);
  if (!provider)
    return; 

  mArray.AppendElement(provider);
}

void
nsChromeRegistry::nsProviderArray::EnumerateToArray(nsTArray<nsCString> *a)
{
  PRInt32 i = mArray.Count();
  while (i--) {
    ProviderEntry *entry = reinterpret_cast<ProviderEntry*>(mArray[i]);
    a->AppendElement(entry->provider);
  }
}

void
nsChromeRegistry::nsProviderArray::Clear()
{
  PRInt32 i = mArray.Count();
  while (i--) {
    ProviderEntry* entry = reinterpret_cast<ProviderEntry*>(mArray[i]);
    delete entry;
  }

  mArray.Clear();
}

nsChromeRegistry::PackageEntry::PackageEntry(const nsACString& aPackage) :
  package(aPackage), flags(0)
{
}

PLHashNumber
nsChromeRegistry::HashKey(PLDHashTable *table, const void *key)
{
  const nsACString& str = *reinterpret_cast<const nsACString*>(key);
  return HashString(str);
}

PRBool
nsChromeRegistry::MatchKey(PLDHashTable *table, const PLDHashEntryHdr *entry,
                           const void *key)
{
  const nsACString& str = *reinterpret_cast<const nsACString*>(key);
  const PackageEntry* pentry = static_cast<const PackageEntry*>(entry);
  return str.Equals(pentry->package);
}

void
nsChromeRegistry::ClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  PackageEntry* pentry = static_cast<PackageEntry*>(entry);
  pentry->~PackageEntry();
}

PRBool
nsChromeRegistry::InitEntry(PLDHashTable *table, PLDHashEntryHdr *entry,
                            const void *key)
{
  const nsACString& str = *reinterpret_cast<const nsACString*>(key);

  new (entry) PackageEntry(str);
  return PR_TRUE;
}

const PLDHashTableOps
nsChromeRegistry::kTableOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  HashKey,
  MatchKey,
  PL_DHashMoveEntryStub,
  ClearEntry,
  PL_DHashFinalizeStub,
  InitEntry
};

void
nsChromeRegistry::OverlayListEntry::AddURI(nsIURI* aURI)
{
  PRInt32 i = mArray.Count();
  while (i--) {
    PRBool equals;
    if (NS_SUCCEEDED(aURI->Equals(mArray[i], &equals)) && equals)
        return;
  }

  mArray.AppendObject(aURI);
}

void
nsChromeRegistry::OverlayListHash::Add(nsIURI* aBase, nsIURI* aOverlay)
{
  OverlayListEntry* entry = mTable.PutEntry(aBase);
  if (entry)
    entry->AddURI(aOverlay);
}

const nsCOMArray<nsIURI>*
nsChromeRegistry::OverlayListHash::GetArray(nsIURI* aBase)
{
  OverlayListEntry* entry = mTable.GetEntry(aBase);
  if (!entry)
    return nsnull;

  return &entry->mArray;
}

nsChromeRegistry::~nsChromeRegistry()
{
  if (mPackagesHash.ops)
    PL_DHashTableFinish(&mPackagesHash);
  gChromeRegistry = nsnull;
}

NS_INTERFACE_MAP_BEGIN(nsChromeRegistry)
  NS_INTERFACE_MAP_ENTRY(nsIChromeRegistry)
  NS_INTERFACE_MAP_ENTRY(nsIXULChromeRegistry)
  NS_INTERFACE_MAP_ENTRY(nsIToolkitChromeRegistry)
#ifdef MOZ_XUL
  NS_INTERFACE_MAP_ENTRY(nsIXULOverlayProvider)
#endif
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIChromeRegistry)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsChromeRegistry)
NS_IMPL_RELEASE(nsChromeRegistry)




static nsresult
getUILangCountry(nsACString& aUILang)
{
  nsresult rv;

  nsCOMPtr<nsILocaleService> localeService = do_GetService(NS_LOCALESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString uiLang;
  rv = localeService->GetLocaleComponentForUserAgent(uiLang);
  NS_ENSURE_SUCCESS(rv, rv);

  CopyUTF16toUTF8(uiLang, aUILang);
  return NS_OK;
}

nsresult
nsChromeRegistry::Init()
{
  nsresult rv;

  
  
  
  
  NS_ASSERTION(nsCOMPtr<nsIIOService>(mozilla::services::GetIOService()),
               "I/O service not registered or available early enough?");

  if (!PL_DHashTableInit(&mPackagesHash, &kTableOps,
                         nsnull, sizeof(PackageEntry), 16))
    return NS_ERROR_FAILURE;

  if (!mOverlayHash.Init() ||
      !mStyleHash.Init() ||
      !mOverrideTable.Init())
    return NS_ERROR_FAILURE;

  mSelectedLocale = NS_LITERAL_CSTRING("en-US");
  mSelectedSkin = NS_LITERAL_CSTRING("classic/1.0");

  
  
  
  
  gChromeRegistry = this;

  PRBool safeMode = PR_FALSE;
  nsCOMPtr<nsIXULRuntime> xulrun (do_GetService(XULAPPINFO_SERVICE_CONTRACTID));
  if (xulrun)
    xulrun->GetInSafeMode(&safeMode);
  
  nsCOMPtr<nsIPrefService> prefserv (do_GetService(NS_PREFSERVICE_CONTRACTID));
  nsCOMPtr<nsIPrefBranch> prefs;

  if (safeMode)
    prefserv->GetDefaultBranch(nsnull, getter_AddRefs(prefs));
  else
    prefs = do_QueryInterface(prefserv);

  if (!prefs) {
    NS_WARNING("Could not get pref service!");
  }
  else {
    nsXPIDLCString provider;
    rv = prefs->GetCharPref(SELECTED_SKIN_PREF, getter_Copies(provider));
    if (NS_SUCCEEDED(rv))
      mSelectedSkin = provider;

    SelectLocaleFromPref(prefs);

    nsCOMPtr<nsIPrefBranch2> prefs2 (do_QueryInterface(prefs));
    if (prefs2) {
      rv = prefs2->AddObserver(MATCH_OS_LOCALE_PREF, this, PR_TRUE);
      rv = prefs2->AddObserver(SELECTED_LOCALE_PREF, this, PR_TRUE);
      rv = prefs2->AddObserver(SELECTED_SKIN_PREF, this, PR_TRUE);
    }
  }

  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  if (obsService) {
    obsService->AddObserver(this, "command-line-startup", PR_TRUE);
    obsService->AddObserver(this, "profile-initial-state", PR_TRUE);
  }

  mInitialized = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsChromeRegistry::CheckForOSAccessibility()
{
  nsresult rv;

  nsCOMPtr<nsILookAndFeel> lookAndFeel (do_GetService(kLookAndFeelCID));
  if (lookAndFeel) {
    PRInt32 useAccessibilityTheme = 0;

    rv = lookAndFeel->GetMetric(nsILookAndFeel::eMetric_UseAccessibilityTheme,
                                useAccessibilityTheme);

    if (NS_SUCCEEDED(rv) && useAccessibilityTheme) {
      
      if (!mSelectedSkin.EqualsLiteral("classic/1.0")) {
        mSelectedSkin.AssignLiteral("classic/1.0");
        RefreshSkins();
      }

      nsCOMPtr<nsIPrefBranch2> prefs (do_GetService(NS_PREFSERVICE_CONTRACTID));
      if (prefs) {
        prefs->RemoveObserver(SELECTED_SKIN_PREF, this);
      }
    }
  }

  return NS_OK;
}

nsresult
nsChromeRegistry::GetProviderAndPath(nsIURL* aChromeURL,
                                     nsACString& aProvider, nsACString& aPath)
{
  nsresult rv;

#ifdef DEBUG
  PRBool isChrome;
  aChromeURL->SchemeIs("chrome", &isChrome);
  NS_ASSERTION(isChrome, "Non-chrome URI?");
#endif

  nsCAutoString path;
  rv = aChromeURL->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  if (path.Length() < 3) {
    LogMessage("Invalid chrome URI: %s", path.get());
    return NS_ERROR_FAILURE;
  }

  path.SetLength(nsUnescapeCount(path.BeginWriting()));
  NS_ASSERTION(path.First() == '/', "Path should always begin with a slash!");

  PRInt32 slash = path.FindChar('/', 1);
  if (slash == 1) {
    LogMessage("Invalid chrome URI: %s", path.get());
    return NS_ERROR_FAILURE;
  }

  if (slash == -1) {
    aPath.Truncate();
  }
  else {
    if (slash == (PRInt32) path.Length() - 1)
      aPath.Truncate();
    else
      aPath.Assign(path.get() + slash + 1, path.Length() - slash - 1);

    --slash;
  }

  aProvider.Assign(path.get() + 1, slash);
  return NS_OK;
}


nsresult
nsChromeRegistry::Canonify(nsIURL* aChromeURL)
{
  NS_NAMED_LITERAL_CSTRING(kSlash, "/");

  nsresult rv;

  nsCAutoString provider, path;
  rv = GetProviderAndPath(aChromeURL, provider, path);
  NS_ENSURE_SUCCESS(rv, rv);

  if (path.IsEmpty()) {
    nsCAutoString package;
    rv = aChromeURL->GetHost(package);
    NS_ENSURE_SUCCESS(rv, rv);

    
    path.Assign(kSlash + provider + kSlash + package);
    if (provider.EqualsLiteral("content")) {
      path.AppendLiteral(".xul");
    }
    else if (provider.EqualsLiteral("locale")) {
      path.AppendLiteral(".dtd");
    }
    else if (provider.EqualsLiteral("skin")) {
      path.AppendLiteral(".css");
    }
    else {
      return NS_ERROR_INVALID_ARG;
    }
    aChromeURL->SetPath(path);
  }
  else {
    
    
    const char* pos = path.BeginReading();
    const char* end = path.EndReading();
    while (pos < end) {
      switch (*pos) {
        case ':':
          return NS_ERROR_DOM_BAD_URI;
        case '.':
          if (pos[1] == '.')
            return NS_ERROR_DOM_BAD_URI;
          break;
        case '%':
          
          
          if (pos[1] == '2' &&
               ( pos[2] == 'e' || pos[2] == 'E' || 
                 pos[2] == '5' ))
            return NS_ERROR_DOM_BAD_URI;
          break;
        case '?':
        case '#':
          pos = end;
          continue;
      }
      ++pos;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsChromeRegistry::ConvertChromeURL(nsIURI* aChromeURI, nsIURI* *aResult)
{
  nsresult rv;
  NS_ASSERTION(aChromeURI, "null url!");

  if (mOverrideTable.Get(aChromeURI, aResult))
    return NS_OK;

  nsCOMPtr<nsIURL> chromeURL (do_QueryInterface(aChromeURI));
  NS_ENSURE_TRUE(chromeURL, NS_NOINTERFACE);

  nsCAutoString package, provider, path;
  rv = chromeURL->GetHostPort(package);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetProviderAndPath(chromeURL, provider, path);
  NS_ENSURE_SUCCESS(rv, rv);

  PackageEntry* entry =
    static_cast<PackageEntry*>(PL_DHashTableOperate(&mPackagesHash,
                                                    & (nsACString&) package,
                                                    PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_FREE(entry)) {
    if (!mInitialized)
      return NS_ERROR_NOT_INITIALIZED;

    LogMessage("No chrome package registered for chrome://%s/%s/%s",
               package.get(), provider.get(), path.get());

    return NS_ERROR_FAILURE;
  }

  if (entry->flags & PackageEntry::PLATFORM_PACKAGE) {
#if defined(XP_WIN) || defined(XP_OS2)
    path.Insert("win/", 0);
#elif defined(XP_MACOSX)
    path.Insert("mac/", 0);
#else
    path.Insert("unix/", 0);
#endif
  }

  nsIURI* baseURI = nsnull;
  if (provider.EqualsLiteral("locale")) {
    baseURI = entry->locales.GetBase(mSelectedLocale, nsProviderArray::LOCALE);
  }
  else if (provider.EqualsLiteral("skin")) {
    baseURI = entry->skins.GetBase(mSelectedSkin, nsProviderArray::ANY);
  }
  else if (provider.EqualsLiteral("content")) {
    baseURI = entry->baseURI;
  }

  if (!baseURI) {
    LogMessage("No chrome package registered for chrome://%s/%s/%s",
               package.get(), provider.get(), path.get());
    return NS_ERROR_FAILURE;
  }

  return NS_NewURI(aResult, path, nsnull, baseURI);
}

nsresult
nsChromeRegistry::GetSelectedLocale(const nsACString& aPackage, nsACString& aLocale)
{
  PackageEntry* entry =
    static_cast<PackageEntry*>(PL_DHashTableOperate(&mPackagesHash,
                                                       & aPackage,
                                                       PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_FREE(entry))
    return NS_ERROR_FAILURE;

  aLocale = entry->locales.GetSelected(mSelectedLocale, nsProviderArray::LOCALE);
  if (aLocale.IsEmpty())
    return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
nsChromeRegistry::IsLocaleRTL(const nsACString& package, PRBool *aResult)
{
  *aResult = PR_FALSE;

  nsCAutoString locale;
  GetSelectedLocale(package, locale);
  if (locale.Length() < 2)
    return NS_OK;

  
  
  
  nsCAutoString prefString = NS_LITERAL_CSTRING("intl.uidirection.") + locale;
  nsCOMPtr<nsIPrefBranch> prefBranch (do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (!prefBranch)
    return NS_OK;
  
  nsXPIDLCString dir;
  prefBranch->GetCharPref(prefString.get(), getter_Copies(dir));
  if (dir.IsEmpty()) {
    PRInt32 hyphen = prefString.FindChar('-');
    if (hyphen >= 1) {
      nsCAutoString shortPref(Substring(prefString, 0, hyphen));
      prefBranch->GetCharPref(shortPref.get(), getter_Copies(dir));
    }
  }
  *aResult = dir.EqualsLiteral("rtl");
  return NS_OK;
}

NS_IMETHODIMP
nsChromeRegistry::GetLocalesForPackage(const nsACString& aPackage,
                                       nsIUTF8StringEnumerator* *aResult)
{
  nsTArray<nsCString> *a = new nsTArray<nsCString>;
  if (!a)
    return NS_ERROR_OUT_OF_MEMORY;

  PackageEntry* entry =
    static_cast<PackageEntry*>(PL_DHashTableOperate(&mPackagesHash,
                                                       & aPackage,
                                                       PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
    entry->locales.EnumerateToArray(a);
  }

  nsresult rv = NS_NewAdoptingUTF8StringEnumerator(aResult, a);
  if (NS_FAILED(rv))
    delete a;

  return rv;
}

#ifdef MOZ_XUL
NS_IMETHODIMP
nsChromeRegistry::GetStyleOverlays(nsIURI *aChromeURL,
                                   nsISimpleEnumerator **aResult)
{
  const nsCOMArray<nsIURI>* parray = mStyleHash.GetArray(aChromeURL);
  if (!parray)
    return NS_NewEmptyEnumerator(aResult);

  return NS_NewArrayEnumerator(aResult, *parray);
}

NS_IMETHODIMP
nsChromeRegistry::GetXULOverlays(nsIURI *aChromeURL, nsISimpleEnumerator **aResult)
{
  const nsCOMArray<nsIURI>* parray = mOverlayHash.GetArray(aChromeURL);
  if (!parray)
    return NS_NewEmptyEnumerator(aResult);

  return NS_NewArrayEnumerator(aResult, *parray);
}
#endif 






static void FlushSkinBindingsForWindow(nsIDOMWindowInternal* aWindow)
{
  
  nsCOMPtr<nsIDOMDocument> domDocument;
  aWindow->GetDocument(getter_AddRefs(domDocument));
  if (!domDocument)
    return;

  nsCOMPtr<nsIDocument> document = do_QueryInterface(domDocument);
  if (!document)
    return;

  
  document->FlushSkinBindings();
}


NS_IMETHODIMP nsChromeRegistry::RefreshSkins()
{
  nsCOMPtr<nsIWindowMediator> windowMediator
    (do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (!windowMediator)
    return NS_OK;

  nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
  windowMediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator));
  PRBool more;
  windowEnumerator->HasMoreElements(&more);
  while (more) {
    nsCOMPtr<nsISupports> protoWindow;
    windowEnumerator->GetNext(getter_AddRefs(protoWindow));
    if (protoWindow) {
      nsCOMPtr<nsIDOMWindowInternal> domWindow = do_QueryInterface(protoWindow);
      if (domWindow)
        FlushSkinBindingsForWindow(domWindow);
    }
    windowEnumerator->HasMoreElements(&more);
  }

  FlushSkinCaches();

  windowMediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator));
  windowEnumerator->HasMoreElements(&more);
  while (more) {
    nsCOMPtr<nsISupports> protoWindow;
    windowEnumerator->GetNext(getter_AddRefs(protoWindow));
    if (protoWindow) {
      nsCOMPtr<nsIDOMWindowInternal> domWindow = do_QueryInterface(protoWindow);
      if (domWindow)
        RefreshWindow(domWindow);
    }
    windowEnumerator->HasMoreElements(&more);
  }
   
  return NS_OK;
}

void
nsChromeRegistry::FlushSkinCaches()
{
  nsCOMPtr<nsIObserverService> obsSvc =
    mozilla::services::GetObserverService();
  NS_ASSERTION(obsSvc, "Couldn't get observer service.");

  obsSvc->NotifyObservers(static_cast<nsIChromeRegistry*>(this),
                          NS_CHROME_FLUSH_SKINS_TOPIC, nsnull);
}

static PRBool IsChromeURI(nsIURI* aURI)
{
    PRBool isChrome=PR_FALSE;
    if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && isChrome)
        return PR_TRUE;
    return PR_FALSE;
}


nsresult nsChromeRegistry::RefreshWindow(nsIDOMWindowInternal* aWindow)
{
  
  nsCOMPtr<nsIDOMWindowCollection> frames;
  aWindow->GetFrames(getter_AddRefs(frames));
  PRUint32 length;
  frames->GetLength(&length);
  PRUint32 j;
  for (j = 0; j < length; j++) {
    nsCOMPtr<nsIDOMWindow> childWin;
    frames->Item(j, getter_AddRefs(childWin));
    nsCOMPtr<nsIDOMWindowInternal> childInt(do_QueryInterface(childWin));
    RefreshWindow(childInt);
  }

  nsresult rv;
  
  nsCOMPtr<nsIDOMDocument> domDocument;
  aWindow->GetDocument(getter_AddRefs(domDocument));
  if (!domDocument)
    return NS_OK;

  nsCOMPtr<nsIDocument> document = do_QueryInterface(domDocument);
  if (!document)
    return NS_OK;

  
  nsCOMPtr<nsIPresShell> shell = document->GetPrimaryShell();
  if (shell) {
    
    nsCOMArray<nsIStyleSheet> agentSheets;
    rv = shell->GetAgentStyleSheets(agentSheets);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMArray<nsIStyleSheet> newAgentSheets;
    for (PRInt32 l = 0; l < agentSheets.Count(); ++l) {
      nsIStyleSheet *sheet = agentSheets[l];

      nsIURI* uri = sheet->GetSheetURI();

      if (IsChromeURI(uri)) {
        
        nsRefPtr<nsCSSStyleSheet> newSheet;
        rv = document->LoadChromeSheetSync(uri, PR_TRUE,
                                           getter_AddRefs(newSheet));
        if (NS_FAILED(rv)) return rv;
        if (newSheet) {
          rv = newAgentSheets.AppendObject(newSheet) ? NS_OK : NS_ERROR_FAILURE;
          if (NS_FAILED(rv)) return rv;
        }
      }
      else {  
        rv = newAgentSheets.AppendObject(sheet) ? NS_OK : NS_ERROR_FAILURE;
        if (NS_FAILED(rv)) return rv;
      }
    }

    rv = shell->SetAgentStyleSheets(newAgentSheets);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMArray<nsIStyleSheet> oldSheets;
  nsCOMArray<nsIStyleSheet> newSheets;

  PRInt32 count = document->GetNumberOfStyleSheets();

  
  PRInt32 i;
  for (i = 0; i < count; i++) {
    
    nsIStyleSheet *styleSheet = document->GetStyleSheetAt(i);

    if (!oldSheets.AppendObject(styleSheet)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  
  
  for (i = 0; i < count; i++) {
    nsRefPtr<nsCSSStyleSheet> sheet = do_QueryObject(oldSheets[i]);
    nsIURI* uri = sheet ? sheet->GetOriginalURI() : nsnull;

    if (uri && IsChromeURI(uri)) {
      
      nsRefPtr<nsCSSStyleSheet> newSheet;
      
      
      document->LoadChromeSheetSync(uri, PR_FALSE, getter_AddRefs(newSheet));
      
      newSheets.AppendObject(newSheet);
    }
    else {
      
      newSheets.AppendObject(sheet);
    }
  }

  
  document->UpdateStyleSheets(oldSheets, newSheets);
  return NS_OK;
}

void
nsChromeRegistry::FlushAllCaches()
{
  nsCOMPtr<nsIObserverService> obsSvc =
    mozilla::services::GetObserverService();
  NS_ASSERTION(obsSvc, "Couldn't get observer service.");

  obsSvc->NotifyObservers((nsIChromeRegistry*) this,
                          NS_CHROME_FLUSH_TOPIC, nsnull);
}  


NS_IMETHODIMP
nsChromeRegistry::ReloadChrome()
{
  FlushAllCaches();
  
  nsresult rv = NS_OK;

  
  nsCOMPtr<nsIWindowMediator> windowMediator
    (do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (windowMediator) {
    nsCOMPtr<nsISimpleEnumerator> windowEnumerator;

    rv = windowMediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator));
    if (NS_SUCCEEDED(rv)) {
      
      PRBool more;
      rv = windowEnumerator->HasMoreElements(&more);
      if (NS_FAILED(rv)) return rv;
      while (more) {
        nsCOMPtr<nsISupports> protoWindow;
        rv = windowEnumerator->GetNext(getter_AddRefs(protoWindow));
        if (NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsIDOMWindowInternal> domWindow =
            do_QueryInterface(protoWindow);
          if (domWindow) {
            nsCOMPtr<nsIDOMLocation> location;
            domWindow->GetLocation(getter_AddRefs(location));
            if (location) {
              rv = location->Reload(PR_FALSE);
              if (NS_FAILED(rv)) return rv;
            }
          }
        }
        rv = windowEnumerator->HasMoreElements(&more);
        if (NS_FAILED(rv)) return rv;
      }
    }
  }
  return rv;
}

NS_IMETHODIMP
nsChromeRegistry::AllowScriptsForPackage(nsIURI* aChromeURI, PRBool *aResult)
{
  nsresult rv;
  *aResult = PR_FALSE;

#ifdef DEBUG
  PRBool isChrome;
  aChromeURI->SchemeIs("chrome", &isChrome);
  NS_ASSERTION(isChrome, "Non-chrome URI passed to AllowScriptsForPackage!");
#endif

  nsCOMPtr<nsIURL> url (do_QueryInterface(aChromeURI));
  NS_ENSURE_TRUE(url, NS_NOINTERFACE);

  nsCAutoString provider, file;
  rv = GetProviderAndPath(url, provider, file);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!provider.EqualsLiteral("skin"))
    *aResult = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsChromeRegistry::AllowContentToAccess(nsIURI *aURI, PRBool *aResult)
{
  nsresult rv;

  *aResult = PR_FALSE;

#ifdef DEBUG
  PRBool isChrome;
  aURI->SchemeIs("chrome", &isChrome);
  NS_ASSERTION(isChrome, "Non-chrome URI passed to AllowContentToAccess!");
#endif

  nsCOMPtr<nsIURL> url = do_QueryInterface(aURI);
  if (!url) {
    NS_ERROR("Chrome URL doesn't implement nsIURL.");
    return NS_ERROR_UNEXPECTED;
  }

  nsCAutoString package;
  rv = url->GetHostPort(package);
  NS_ENSURE_SUCCESS(rv, rv);

  PackageEntry *entry =
    static_cast<PackageEntry*>(PL_DHashTableOperate(&mPackagesHash,
                                                    & (nsACString&) package,
                                                    PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
    *aResult = !!(entry->flags & PackageEntry::CONTENT_ACCESSIBLE);
  }
  return NS_OK;
}

static PLDHashOperator
RemoveAll(PLDHashTable *table, PLDHashEntryHdr *entry, PRUint32 number, void *arg)
{
  return (PLDHashOperator) (PL_DHASH_NEXT | PL_DHASH_REMOVE);
}

NS_IMETHODIMP
nsChromeRegistry::CheckForNewChrome()
{
  PL_DHashTableEnumerate(&mPackagesHash, RemoveAll, nsnull);
  mOverlayHash.Clear();
  mStyleHash.Clear();
  mOverrideTable.Clear();

  nsComponentManagerImpl::gComponentManager->RereadChromeManifests();
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsChromeRegistry::WrappersEnabled(nsIURI *aURI)
{
  nsCOMPtr<nsIURL> chromeURL (do_QueryInterface(aURI));
  if (!chromeURL)
    return PR_FALSE;

  PRBool isChrome = PR_FALSE;
  nsresult rv = chromeURL->SchemeIs("chrome", &isChrome);
  if (NS_FAILED(rv) || !isChrome)
    return PR_FALSE;

  nsCAutoString package;
  rv = chromeURL->GetHostPort(package);
  if (NS_FAILED(rv))
    return PR_FALSE;

  PackageEntry* entry =
    static_cast<PackageEntry*>(PL_DHashTableOperate(&mPackagesHash,
                                                    & (nsACString&) package,
                                                    PL_DHASH_LOOKUP));

  return PL_DHASH_ENTRY_IS_LIVE(entry);
}

nsresult
nsChromeRegistry::SelectLocaleFromPref(nsIPrefBranch* prefs)
{
  nsresult rv;
  PRBool matchOSLocale = PR_FALSE, userLocaleOverride = PR_FALSE;
  prefs->PrefHasUserValue(SELECTED_LOCALE_PREF, &userLocaleOverride);
  rv = prefs->GetBoolPref(MATCH_OS_LOCALE_PREF, &matchOSLocale);

  if (NS_SUCCEEDED(rv) && matchOSLocale && !userLocaleOverride) {
    
    nsCAutoString uiLocale;
    rv = getUILangCountry(uiLocale);
    if (NS_SUCCEEDED(rv))
      mSelectedLocale = uiLocale;
  }
  else {
    nsXPIDLCString provider;
    rv = prefs->GetCharPref(SELECTED_LOCALE_PREF, getter_Copies(provider));
    if (NS_SUCCEEDED(rv)) {
      mSelectedLocale = provider;
    }
  }

  return rv;
}

NS_IMETHODIMP nsChromeRegistry::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
  nsresult rv = NS_OK;

  if (!strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic)) {
    nsCOMPtr<nsIPrefBranch> prefs (do_QueryInterface(aSubject));
    NS_ASSERTION(prefs, "Bad observer call!");

    NS_ConvertUTF16toUTF8 pref(someData);

    if (pref.EqualsLiteral(MATCH_OS_LOCALE_PREF) ||
        pref.EqualsLiteral(SELECTED_LOCALE_PREF)) {
      rv = SelectLocaleFromPref(prefs);
      if (NS_SUCCEEDED(rv) && mProfileLoaded)
        FlushAllCaches();
    }
    else if (pref.EqualsLiteral(SELECTED_SKIN_PREF)) {
      nsXPIDLCString provider;
      rv = prefs->GetCharPref(pref.get(), getter_Copies(provider));
      if (NS_FAILED(rv)) {
        NS_ERROR("Couldn't get new locale pref!");
        return rv;
      }

      mSelectedSkin = provider;
      RefreshSkins();
    } else {
      NS_ERROR("Unexpected pref!");
    }
  }
  else if (!strcmp("command-line-startup", aTopic)) {
    nsCOMPtr<nsICommandLine> cmdLine (do_QueryInterface(aSubject));
    if (cmdLine) {
      nsAutoString uiLocale;
      rv = cmdLine->HandleFlagWithParam(NS_LITERAL_STRING(UILOCALE_CMD_LINE_ARG),
                                        PR_FALSE, uiLocale);
      if (NS_SUCCEEDED(rv) && !uiLocale.IsEmpty()) {
        CopyUTF16toUTF8(uiLocale, mSelectedLocale);
        nsCOMPtr<nsIPrefBranch2> prefs (do_GetService(NS_PREFSERVICE_CONTRACTID));
        if (prefs) {
          prefs->RemoveObserver(SELECTED_LOCALE_PREF, this);
        }
      }
    }
  }
  else if (!strcmp("profile-initial-state", aTopic)) {
    mProfileLoaded = PR_TRUE;
  }
  else {
    NS_ERROR("Unexpected observer topic!");
  }

  return rv;
}

nsIURI*
nsChromeRegistry::ManifestProcessingContext::GetManifestURI()
{
  if (!mManifestURI) {
    nsCOMPtr<nsIIOService> io = mozilla::services::GetIOService();
    if (!io) {
      NS_WARNING("No IO service trying to process chrome manifests");
      return NULL;
    }

    io->NewFileURI(mFile, getter_AddRefs(mManifestURI));
  }
  return mManifestURI;
}

nsIXPConnect*
nsChromeRegistry::ManifestProcessingContext::GetXPConnect()
{
  if (!mXPConnect)
    mXPConnect = do_GetService("@mozilla.org/js/xpc/XPConnect;1");

  return mXPConnect;
}

already_AddRefed<nsIURI>
nsChromeRegistry::ManifestProcessingContext::ResolveURI(const char* uri)
{
  nsIURI* baseuri = GetManifestURI();
  if (!baseuri)
    return NULL;

  nsCOMPtr<nsIURI> resolved;
  nsresult rv = NS_NewURI(getter_AddRefs(resolved), uri, baseuri);
  if (NS_FAILED(rv))
    return NULL;

  return resolved.forget();
}

static void
EnsureLowerCase(char *aBuf)
{
  for (; *aBuf; ++aBuf) {
    char ch = *aBuf;
    if (ch >= 'A' && ch <= 'Z')
      *aBuf = ch + 'a' - 'A';
  }
}

void
nsChromeRegistry::ManifestContent(ManifestProcessingContext& cx, int lineno,
                                  char *const * argv, bool platform, bool contentaccessible)
{
  char* package = argv[0];
  char* uri = argv[1];

  EnsureLowerCase(package);

  nsCOMPtr<nsIURI> resolved = cx.ResolveURI(uri);
  if (!resolved) {
    LogMessageWithContext(cx.GetManifestURI(), lineno, nsIScriptError::warningFlag,
                          "During chrome registration, unable to create URI '%s'.", uri);
    return;
  }

  if (!CanLoadResource(resolved)) {
    LogMessageWithContext(resolved, lineno, nsIScriptError::warningFlag,
                          "During chrome registration, cannot register non-local URI '%s' as content.",
                          uri);
    return;
  }

  PackageEntry* entry =
    static_cast<PackageEntry*>(PL_DHashTableOperate(&mPackagesHash,
                                                    & (const nsACString&) nsDependentCString(package),
                                                    PL_DHASH_ADD));
  if (!entry)
    return;

  entry->baseURI = resolved;

  if (platform)
    entry->flags |= PackageEntry::PLATFORM_PACKAGE;
  if (contentaccessible)
    entry->flags |= PackageEntry::CONTENT_ACCESSIBLE;
  if (cx.GetXPConnect()) {
    nsCAutoString urlp("chrome://");
    urlp.Append(package);
    urlp.Append('/');

    cx.GetXPConnect()->FlagSystemFilenamePrefix(urlp.get(), true);
  }
}

void
nsChromeRegistry::ManifestLocale(ManifestProcessingContext& cx, int lineno,
                                 char *const * argv, bool platform, bool contentaccessible)
{
  char* package = argv[0];
  char* provider = argv[1];
  char* uri = argv[2];

  EnsureLowerCase(package);

  nsCOMPtr<nsIURI> resolved = cx.ResolveURI(uri);
  if (!resolved) {
    LogMessageWithContext(cx.GetManifestURI(), lineno, nsIScriptError::warningFlag,
                          "During chrome registration, unable to create URI '%s'.", uri);
    return;
  }

  if (!CanLoadResource(resolved)) {
    LogMessageWithContext(resolved, lineno, nsIScriptError::warningFlag,
                          "During chrome registration, cannot register non-local URI '%s' as content.",
                          uri);
    return;
  }

  PackageEntry* entry =
    static_cast<PackageEntry*>(PL_DHashTableOperate(&mPackagesHash,
                                                    & (const nsACString&) nsDependentCString(package),
                                                    PL_DHASH_ADD));
  if (!entry)
    return;

  entry->locales.SetBase(nsDependentCString(provider), resolved);
}

void
nsChromeRegistry::ManifestSkin(ManifestProcessingContext& cx, int lineno,
                               char *const * argv, bool platform, bool contentaccessible)
{
  char* package = argv[0];
  char* provider = argv[1];
  char* uri = argv[2];

  EnsureLowerCase(package);

  nsCOMPtr<nsIURI> resolved = cx.ResolveURI(uri);
  if (!resolved) {
    LogMessageWithContext(cx.GetManifestURI(), lineno, nsIScriptError::warningFlag,
                          "During chrome registration, unable to create URI '%s'.", uri);
    return;
  }

  if (!CanLoadResource(resolved)) {
    LogMessageWithContext(resolved, lineno, nsIScriptError::warningFlag,
                          "During chrome registration, cannot register non-local URI '%s' as content.",
                          uri);
    return;
  }

  PackageEntry* entry =
    static_cast<PackageEntry*>(PL_DHashTableOperate(&mPackagesHash,
                                                    & (const nsACString&) nsDependentCString(package),
                                                    PL_DHASH_ADD));
  if (!entry)
    return;

  entry->skins.SetBase(nsDependentCString(provider), resolved);
}

void
nsChromeRegistry::ManifestOverlay(ManifestProcessingContext& cx, int lineno,
                                  char *const * argv, bool platform, bool contentaccessible)
{
  char* base = argv[0];
  char* overlay = argv[1];

  nsCOMPtr<nsIURI> baseuri = cx.ResolveURI(base);
  nsCOMPtr<nsIURI> overlayuri = cx.ResolveURI(overlay);
  if (!baseuri || !overlayuri) {
    LogMessageWithContext(cx.GetManifestURI(), lineno, nsIScriptError::warningFlag,
                          "During chrome registration, unable to create URI.");
    return;
  }

  if (!CanLoadResource(overlayuri)) {
    LogMessageWithContext(cx.GetManifestURI(), lineno, nsIScriptError::warningFlag,
                          "Cannot register non-local URI '%s' as an overlay.", overlay);
    return;
  }

  mOverlayHash.Add(baseuri, overlayuri);
}

void
nsChromeRegistry::ManifestStyle(ManifestProcessingContext& cx, int lineno,
                                char *const * argv, bool platform, bool contentaccessible)
{
  char* base = argv[0];
  char* overlay = argv[1];

  nsCOMPtr<nsIURI> baseuri = cx.ResolveURI(base);
  nsCOMPtr<nsIURI> overlayuri = cx.ResolveURI(overlay);
  if (!baseuri || !overlayuri) {
    LogMessageWithContext(cx.GetManifestURI(), lineno, nsIScriptError::warningFlag,
                          "During chrome registration, unable to create URI.");
    return;
  }

  if (!CanLoadResource(overlayuri)) {
    LogMessageWithContext(cx.GetManifestURI(), lineno, nsIScriptError::warningFlag,
                          "Cannot register non-local URI '%s' as a style overlay.", overlay);
    return;
  }

  mStyleHash.Add(baseuri, overlayuri);
}

void
nsChromeRegistry::ManifestOverride(ManifestProcessingContext& cx, int lineno,
                                   char *const * argv, bool platform, bool contentaccessible)
{
  char* chrome = argv[0];
  char* resolved = argv[1];

  nsCOMPtr<nsIURI> chromeuri = cx.ResolveURI(chrome);
  nsCOMPtr<nsIURI> resolveduri = cx.ResolveURI(resolved);
  if (!chromeuri || !resolveduri) {
    LogMessageWithContext(cx.GetManifestURI(), lineno, nsIScriptError::warningFlag,
                          "During chrome registration, unable to create URI.");
    return;
  }

  if (!CanLoadResource(resolveduri)) {
    LogMessageWithContext(cx.GetManifestURI(), lineno, nsIScriptError::warningFlag,
                          "Cannot register non-local URI '%s' for an override.", resolved);
    return;
  }
  mOverrideTable.Put(chromeuri, resolveduri);
}

void
nsChromeRegistry::ManifestResource(ManifestProcessingContext& cx, int lineno,
                                   char *const * argv, bool platform, bool contentaccessible)
{
  char* package = argv[0];
  char* uri = argv[1];

  EnsureLowerCase(package);
  nsDependentCString host(package);

  nsCOMPtr<nsIIOService> io = mozilla::services::GetIOService();
  if (!io) {
    NS_WARNING("No IO service trying to process chrome manifests");
    return;
  }

  nsCOMPtr<nsIProtocolHandler> ph;
  nsresult rv = io->GetProtocolHandler("resource", getter_AddRefs(ph));
  if (NS_FAILED(rv))
    return;
  
  nsCOMPtr<nsIResProtocolHandler> rph = do_QueryInterface(ph);

  PRBool exists = PR_FALSE;
  rv = rph->HasSubstitution(host, &exists);
  if (exists) {
    LogMessageWithContext(cx.GetManifestURI(), lineno, nsIScriptError::warningFlag,
                          "Duplicate resource declaration for '%s' ignored.", package);
    return;
  }

  nsCOMPtr<nsIURI> resolved = cx.ResolveURI(uri);
  if (!resolved) {
    LogMessageWithContext(cx.GetManifestURI(), lineno, nsIScriptError::warningFlag,
                          "During chrome registration, unable to create URI '%s'.", uri);
    return;
  }

  if (!CanLoadResource(resolved)) {
    LogMessageWithContext(cx.GetManifestURI(), lineno, nsIScriptError::warningFlag,
                          "Warning: cannot register non-local URI '%s' as a resource.",
                          uri);
    return;
  }

  rph->SetSubstitution(host, resolved);
}

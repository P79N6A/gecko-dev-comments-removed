








































#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "nsAppDirectoryServiceDefs.h"
#include "nsBrowserProfileMigratorUtils.h"
#include "nsCOMPtr.h"
#include "nsCRTGlue.h"
#include "nsNetCID.h"
#include "nsDocShellCID.h"
#include "nsDebug.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsStringAPI.h"
#include "plstr.h"
#include "prio.h"
#include "prmem.h"
#include "prlong.h"
#include "nsICookieManager2.h"
#include "nsIEProfileMigrator.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsISimpleEnumerator.h"
#include "nsISupportsArray.h"
#include "nsIProfileMigrator.h"
#include "nsIBrowserProfileMigrator.h"
#include "nsIObserverService.h"

#include <objbase.h>
#include <shlguid.h>
#include <urlhist.h>
#include <comdef.h>
#include <shlobj.h>
#include <intshcut.h>

#include "nsIBrowserHistory.h"
#include "nsIGlobalHistory.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIURI.h"
#include "nsIPasswordManager.h"
#include "nsIPasswordManagerInternal.h"
#include "nsIFormHistory.h"
#include "nsIRDFService.h"
#include "nsIRDFContainer.h"
#include "nsIURL.h"
#ifdef MOZ_PLACES_BOOKMARKS
#include "nsINavBookmarksService.h"
#include "nsBrowserCompsCID.h"
#else
#include "nsIBookmarksService.h"
#endif
#include "nsIStringBundle.h"
#include "nsNetUtil.h"
#include "nsToolkitCompsCID.h"
#include "nsUnicharUtils.h"
#include "nsIWindowsRegKey.h"

#define TRIDENTPROFILE_BUNDLE       "chrome://browser/locale/migration/migration.properties"

const int sInitialCookieBufferSize = 1024; 
const int sUsernameLengthLimit     = 80;
const int sHostnameLengthLimit     = 255;




void  __stdcall _com_issue_error(HRESULT hr)
{
  
}





typedef void (*regEntryHandler)(nsIWindowsRegKey *, const nsString&,
                                nsIPrefBranch *, char *);


void
TranslateYNtoTF(nsIWindowsRegKey *aRegKey, const nsString& aRegValueName,
                nsIPrefBranch *aPrefs, char *aPrefKeyName) {

  
  nsAutoString regValue; 
  if (NS_SUCCEEDED(aRegKey->ReadStringValue(aRegValueName, regValue)))
    aPrefs->SetBoolPref(aPrefKeyName, regValue.EqualsLiteral("yes"));
}


void
TranslateYNtoFT(nsIWindowsRegKey *aRegKey, const nsString& aRegValueName,
                nsIPrefBranch *aPrefs, char *aPrefKeyName) {

  
  nsAutoString regValue; 
  if (NS_SUCCEEDED(aRegKey->ReadStringValue(aRegValueName, regValue)))
    aPrefs->SetBoolPref(aPrefKeyName, !regValue.EqualsLiteral("yes"));
}

void
TranslateYNtoImageBehavior(nsIWindowsRegKey *aRegKey,
                           const nsString& aRegValueName,
                           nsIPrefBranch *aPrefs, char *aPrefKeyName) {
  
  nsAutoString regValue; 
  if (NS_SUCCEEDED(aRegKey->ReadStringValue(aRegValueName, regValue)) &&
      !regValue.IsEmpty()) {
    if (regValue.EqualsLiteral("yes"))
      aPrefs->SetIntPref(aPrefKeyName, 1);
    else
      aPrefs->SetIntPref(aPrefKeyName, 2);
  }
}

void
TranslateDWORDtoHTTPVersion(nsIWindowsRegKey *aRegKey,
                            const nsString& aRegValueName,
                            nsIPrefBranch *aPrefs, char *aPrefKeyName) {
  PRUint32 val;
  if (NS_SUCCEEDED(aRegKey->ReadIntValue(aRegValueName, &val))) {
    if (val & 0x1) 
      aPrefs->SetCharPref(aPrefKeyName, "1.1");
    else
      aPrefs->SetCharPref(aPrefKeyName, "1.0");
  }
}


void
TranslateDRGBtoHRGB(nsIWindowsRegKey *aRegKey, const nsString& aRegValueName,
                    nsIPrefBranch *aPrefs, char *aPrefKeyName) {

  
  char prefStringValue[10];

  nsAutoString regValue; 
  if (NS_SUCCEEDED(aRegKey->ReadStringValue(aRegValueName, regValue)) &&
      !regValue.IsEmpty()) {
    int red, green, blue;
    ::swscanf(regValue.get(), L"%d,%d,%d", &red, &green, &blue);
    ::sprintf(prefStringValue, "#%02X%02X%02X", red, green, blue);
    aPrefs->SetCharPref(aPrefKeyName, prefStringValue);
  }
}


void
TranslateDWORDtoPRInt32(nsIWindowsRegKey *aRegKey,
                        const nsString& aRegValueName,
                        nsIPrefBranch *aPrefs, char *aPrefKeyName) {

  
  PRInt32 prefIntValue = 0;

  if (NS_SUCCEEDED(aRegKey->ReadIntValue(aRegValueName, 
                   NS_REINTERPRET_CAST(PRUint32 *, &prefIntValue))))
    aPrefs->SetIntPref(aPrefKeyName, prefIntValue);
}


void
TranslateString(nsIWindowsRegKey *aRegKey, const nsString& aRegValueName,
                nsIPrefBranch *aPrefs, char *aPrefKeyName) {
  nsAutoString regValue; 
  if (NS_SUCCEEDED(aRegKey->ReadStringValue(aRegValueName, regValue)) &&
      !regValue.IsEmpty()) {
    aPrefs->SetCharPref(aPrefKeyName, NS_ConvertUTF16toUTF8(regValue).get());
  }
}



void
TranslateLanglist(nsIWindowsRegKey *aRegKey, const nsString& aRegValueName,
                  nsIPrefBranch *aPrefs, char *aPrefKeyName) {

  nsAutoString lang;
  if (NS_FAILED(aRegKey->ReadStringValue(aRegValueName, lang)))
      return;

  
  

  char prefStringValue[MAX_PATH]; 
  NS_LossyConvertUTF16toASCII langCstr(lang);
  const char   *source = langCstr.get(),
               *sourceEnd = source + langCstr.Length();
  char   *dest = prefStringValue,
         *destEnd = dest + (MAX_PATH-2); 
  PRBool  skip = PR_FALSE,
          comma = PR_FALSE;

  while (source < sourceEnd && *source && dest < destEnd) {
    if (*source == ',')
      skip = PR_FALSE;
    else if (*source == ';')
      skip = PR_TRUE;
    if (!skip) {
      if (comma && *source != ' ')
        *dest++ = ' ';
      *dest++ = *source;
    }
    comma = *source == ',';
    ++source;
  }
  *dest = 0;

  aPrefs->SetCharPref(aPrefKeyName, prefStringValue);
}

static int CALLBACK
fontEnumProc(const LOGFONTW *aLogFont, const TEXTMETRICW *aMetric,
             DWORD aFontType, LPARAM aClosure) {
  *((int *) aClosure) = aLogFont->lfPitchAndFamily & FF_ROMAN;
  return 0;
}
void
TranslatePropFont(nsIWindowsRegKey *aRegKey, const nsString& aRegValueName,
                  nsIPrefBranch *aPrefs, char *aPrefKeyName) {

  HDC      dc = ::GetDC(0);
  LOGFONTW lf;
  int      isSerif = 1;

  
  lf.lfCharSet = DEFAULT_CHARSET;
  lf.lfPitchAndFamily = 0;
  nsAutoString font;
  if (NS_FAILED(aRegKey->ReadStringValue(aRegValueName, font)))
      return;

  ::wcsncpy(lf.lfFaceName, font.get(), LF_FACESIZE);
  lf.lfFaceName[LF_FACESIZE - 1] = L'\0';
  ::EnumFontFamiliesExW(dc, &lf, fontEnumProc, (LPARAM) &isSerif, 0);
  ::ReleaseDC(0, dc);

  
  
  nsDependentCString generic(isSerif ? "serif" : "sans-serif");
  nsCAutoString prefName("font.name.");
  prefName.Append(generic);
  prefName.Append(".x-western");
  aPrefs->SetCharPref(prefName.get(), NS_ConvertUTF16toUTF8(font).get());
  aPrefs->SetCharPref("font.default.x-western", generic.get());
}





struct regEntry {
  char            *regKeyName,     
                  *regValueName;   
  char            *prefKeyName;    
  regEntryHandler  entryHandler;   
};

const regEntry gRegEntries[] = {
  { "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete",
     "AutoSuggest",
     "browser.urlbar.autocomplete.enabled",
     TranslateYNtoTF },
  { "Software\\Microsoft\\Internet Explorer\\International",
    "AcceptLanguage",
    "intl.accept_languages",
    TranslateLanglist },
  
  
  { "Software\\Microsoft\\Internet Explorer\\International\\Scripts\\3",
    "IEFixedFontName",
    "font.name.monospace.x-western",
    TranslateString },
  { 0, 
    "IEPropFontName",
    "", 
    TranslatePropFont },
  { "Software\\Microsoft\\Internet Explorer\\Main",
    "Use_DlgBox_Colors",
    "browser.display.use_system_colors",
    TranslateYNtoTF },
  { 0,
    "Use FormSuggest",
    "browser.formfill.enable",
    TranslateYNtoTF },
  { 0,
    "FormSuggest Passwords",
    "signon.rememberSignons",
    TranslateYNtoTF },
#if 0
  
  { 0,
    "Start Page",
     REG_SZ,
    "browser.startup.homepage",
    TranslateString },
#endif
  { 0, 
    "Anchor Underline",
    "browser.underline_anchors",
    TranslateYNtoTF },
  { 0,
    "Display Inline Images",
    "permissions.default.image",
    TranslateYNtoImageBehavior },
  { 0,
    "Enable AutoImageResize",
    "browser.enable_automatic_image_resizing",
    TranslateYNtoTF },
  { 0,
    "Move System Caret",
    "accessibility.browsewithcaret",
    TranslateYNtoTF },
  { 0,
    "NotifyDownloadComplete",
    "browser.download.manager.showAlertOnComplete",
    TranslateYNtoTF },
  { 0,
    "SmoothScroll",   
    "general.smoothScroll",
    TranslateYNtoTF },
  { "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
    "EnableHttp1_1",
    "network.http.version",
    TranslateDWORDtoHTTPVersion },
  { 0,
    "ProxyHttp1.1",
    "network.http.proxy.version",
    TranslateDWORDtoHTTPVersion },

  { "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Url History",
    "DaysToKeep",
    "browser.history_expire_days",
    TranslateDWORDtoPRInt32 },
  { "Software\\Microsoft\\Internet Explorer\\Settings",
    "Always Use My Colors",            
    "browser.display.use_document_colors",
    TranslateYNtoFT },
  { 0,
    "Text Color",
    "browser.display.foreground_color",
    TranslateDRGBtoHRGB },
  { 0,
    "Background Color",
    "browser.display.background_color",
    TranslateDRGBtoHRGB },
  { 0,
    "Anchor Color",
    "browser.anchor_color",
    TranslateDRGBtoHRGB },
  { 0,
    "Anchor Color Visited",
    "browser.visited_color",
    TranslateDRGBtoHRGB },
  { 0,
    "Always Use My Font Face",    
    "browser.display.use_document_fonts",
    TranslateYNtoFT },
  { "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Url History",
    "DaysToKeep",
    "browser.history_expire_days",
    TranslateDWORDtoPRInt32 }
};

#if 0
user_pref("font.size.fixed.x-western", 14);
user_pref("font.size.variable.x-western", 15);
#endif



NS_IMETHODIMP
nsIEProfileMigrator::Migrate(PRUint16 aItems, nsIProfileStartup* aStartup, const PRUnichar* aProfile)
{
  nsresult rv = NS_OK;

  PRBool aReplace = PR_FALSE;

  if (aStartup) {
    aReplace = PR_TRUE;
    rv = aStartup->DoStartup();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NOTIFY_OBSERVERS(MIGRATION_STARTED, nsnull);

  COPY_DATA(CopyPreferences,  aReplace, nsIBrowserProfileMigrator::SETTINGS);
  COPY_DATA(CopyCookies,      aReplace, nsIBrowserProfileMigrator::COOKIES);
  COPY_DATA(CopyHistory,      aReplace, nsIBrowserProfileMigrator::HISTORY);
  COPY_DATA(CopyFormData,     aReplace, nsIBrowserProfileMigrator::FORMDATA);
  COPY_DATA(CopyPasswords,    aReplace, nsIBrowserProfileMigrator::PASSWORDS);
  COPY_DATA(CopyFavorites,    aReplace, nsIBrowserProfileMigrator::BOOKMARKS);

  NOTIFY_OBSERVERS(MIGRATION_ENDED, nsnull);

  return rv;
}

NS_IMETHODIMP
nsIEProfileMigrator::GetMigrateData(const PRUnichar* aProfile, 
                                    PRBool aReplace,
                                    PRUint16* aResult)
{
  
  *aResult = nsIBrowserProfileMigrator::SETTINGS | nsIBrowserProfileMigrator::COOKIES | 
             nsIBrowserProfileMigrator::HISTORY | nsIBrowserProfileMigrator::FORMDATA |
             nsIBrowserProfileMigrator::PASSWORDS | nsIBrowserProfileMigrator::BOOKMARKS;
  return NS_OK;
}

NS_IMETHODIMP
nsIEProfileMigrator::GetSourceExists(PRBool* aResult)
{
  
  *aResult = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsIEProfileMigrator::GetSourceHasMultipleProfiles(PRBool* aResult)
{
  *aResult = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsIEProfileMigrator::GetSourceProfiles(nsISupportsArray** aResult)
{
  *aResult = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsIEProfileMigrator::GetSourceHomePageURL(nsACString& aResult)
{
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  NS_NAMED_LITERAL_STRING(homeURLKey,
                          "Software\\Microsoft\\Internet Explorer\\Main");
  if (!regKey ||
      NS_FAILED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                             homeURLKey, nsIWindowsRegKey::ACCESS_READ)))
    return NS_OK;
  
  NS_NAMED_LITERAL_STRING(homeURLValName, "Start Page");
  nsAutoString  homeURLVal;
  if (NS_SUCCEEDED(regKey->ReadStringValue(homeURLValName, homeURLVal))) {
    
    
    
    
    nsCAutoString  homePageURL;
    nsCOMPtr<nsIURI> homePageURI;

    if (NS_SUCCEEDED(NS_NewURI(getter_AddRefs(homePageURI), homeURLVal)))
        if (NS_SUCCEEDED(homePageURI->GetSpec(homePageURL)) 
            && !homePageURL.IsEmpty())
            aResult.Assign(homePageURL);
  }
  return NS_OK;
}




NS_IMPL_ISUPPORTS1(nsIEProfileMigrator, nsIBrowserProfileMigrator);

nsIEProfileMigrator::nsIEProfileMigrator() 
{
  mObserverService = do_GetService("@mozilla.org/observer-service;1");
}

nsIEProfileMigrator::~nsIEProfileMigrator() 
{
}

nsresult
nsIEProfileMigrator::CopyHistory(PRBool aReplace) 
{
  nsCOMPtr<nsIBrowserHistory> hist(do_GetService(NS_GLOBALHISTORY2_CONTRACTID));
  nsCOMPtr<nsIIOService> ios(do_GetService(NS_IOSERVICE_CONTRACTID));

  
  ::CoInitialize(NULL);

  IUrlHistoryStg2* ieHistory;
  HRESULT hr = ::CoCreateInstance(CLSID_CUrlHistory, 
                                  NULL,
                                  CLSCTX_INPROC_SERVER, 
                                  IID_IUrlHistoryStg2, 
                                  reinterpret_cast<void**>(&ieHistory));
  if (SUCCEEDED(hr)) {
    IEnumSTATURL* enumURLs;
    hr = ieHistory->EnumUrls(&enumURLs);
    if (SUCCEEDED(hr)) {
      STATURL statURL;
      ULONG fetched;
      _bstr_t url;
      nsCAutoString scheme;
      SYSTEMTIME st;
      PRBool validScheme = PR_FALSE;
      PRUnichar* tempTitle = nsnull;

      for (int count = 0; (hr = enumURLs->Next(1, &statURL, &fetched)) == S_OK; ++count) {
        if (statURL.pwcsUrl) {
          
          tempTitle = statURL.pwcsTitle ? (PRUnichar*)((wchar_t*)(statURL.pwcsTitle)) : nsnull;

          
          ::FileTimeToSystemTime(&(statURL.ftLastVisited), &st);
          PRExplodedTime prt;
          prt.tm_year = st.wYear;
          prt.tm_month = st.wMonth - 1; 
          prt.tm_mday = st.wDay;
          prt.tm_hour = st.wHour;
          prt.tm_min = st.wMinute;
          prt.tm_sec = st.wSecond;
          prt.tm_usec = st.wMilliseconds * 1000;
          prt.tm_wday = 0;
          prt.tm_yday = 0;
          prt.tm_params.tp_gmt_offset = 0;
          prt.tm_params.tp_dst_offset = 0;
          PRTime lastVisited = PR_ImplodeTime(&prt);

          
          url = statURL.pwcsUrl;

          NS_ConvertUTF16toUTF8 urlStr(url);

          if (NS_FAILED(ios->ExtractScheme(urlStr, scheme))) {
            ::CoTaskMemFree(statURL.pwcsUrl);    
            if (statURL.pwcsTitle) 
              ::CoTaskMemFree(statURL.pwcsTitle);
            continue;
          }
          ToLowerCase(scheme);

          
          
          
          
          const char* schemes[] = { "http", "https", "ftp", "file" };
          for (int i = 0; i < 4; ++i) {
            if (validScheme = scheme.Equals(schemes[i]))
              break;
          }
          
          
          if (validScheme) {
            nsCOMPtr<nsIURI> uri;
            ios->NewURI(urlStr, nsnull, nsnull, getter_AddRefs(uri));
            if (uri) {
              if (tempTitle) 
                hist->AddPageWithDetails(uri, tempTitle, lastVisited);
              else
                hist->AddPageWithDetails(uri, url, lastVisited);
            }
          }
          ::CoTaskMemFree(statURL.pwcsUrl);    
        }
        if (statURL.pwcsTitle) 
          ::CoTaskMemFree(statURL.pwcsTitle);    
      }
      nsCOMPtr<nsIRDFRemoteDataSource> ds(do_QueryInterface(hist));
      if (ds)
        ds->Flush();
  
      enumURLs->Release();
    }

    ieHistory->Release();
  }
  ::CoUninitialize();

  
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  NS_NAMED_LITERAL_STRING(typedURLKey,
                          "Software\\Microsoft\\Internet Explorer\\TypedURLs");
  if (regKey && 
      NS_SUCCEEDED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                                typedURLKey, nsIWindowsRegKey::ACCESS_READ))) {
    int offset = 0;

    while (1) {
      nsAutoString valueName;
      if (NS_FAILED(regKey->GetValueName(offset, valueName)))
        break;

      nsAutoString url; 
      if (Substring(valueName, 0, 3).EqualsLiteral("url") &&
          NS_SUCCEEDED(regKey->ReadStringValue(valueName, url))) {
        nsCOMPtr<nsIURI> uri;
        ios->NewURI(NS_ConvertUTF16toUTF8(url), nsnull, nsnull,
                    getter_AddRefs(uri));
        if (uri)
          hist->MarkPageAsTyped(uri);
      }
      ++offset;
    }
  }

  return NS_OK;
}



























































typedef HRESULT (WINAPI *PStoreCreateInstancePtr)(IPStore**, DWORD, DWORD, DWORD);

struct SignonData {
  PRUnichar* user;
  PRUnichar* pass;
  char*      realm;
};






static GUID IEPStoreAutocompGUID = { 0xe161255a, 0x37c3, 0x11d2, { 0xbc, 0xaa, 0x00, 0xc0, 0x4f, 0xd9, 0x29, 0xdb } };
static GUID IEPStoreSiteAuthGUID = { 0x5e7e8100, 0x9138, 0x11d1, { 0x94, 0x5a, 0x00, 0xc0, 0x4f, 0xc3, 0x08, 0xff } };



















































nsresult
nsIEProfileMigrator::CopyPasswords(PRBool aReplace)
{
  HRESULT hr;
  nsresult rv;
  nsVoidArray signonsFound;

  HMODULE pstoreDLL = ::LoadLibrary("pstorec.dll");
  if (!pstoreDLL) {
    
    
    
    return NS_ERROR_FAILURE;
  }

  PStoreCreateInstancePtr PStoreCreateInstance = (PStoreCreateInstancePtr)::GetProcAddress(pstoreDLL, "PStoreCreateInstance");
  IPStorePtr PStore;
  hr = PStoreCreateInstance(&PStore, 0, 0, 0);

  rv = GetSignonsListFromPStore(PStore, &signonsFound);
  if (NS_SUCCEEDED(rv))
    ResolveAndMigrateSignons(PStore, &signonsFound);

  MigrateSiteAuthSignons(PStore);
  return NS_OK;
}














nsresult
nsIEProfileMigrator::MigrateSiteAuthSignons(IPStore* aPStore)
{
  HRESULT hr;

  NS_ENSURE_ARG_POINTER(aPStore);

  nsCOMPtr<nsIPasswordManager> pwmgr(do_GetService("@mozilla.org/passwordmanager;1"));
  if (!pwmgr)
    return NS_OK;

  GUID mtGuid = {0};
  IEnumPStoreItemsPtr enumItems = NULL;
  hr = aPStore->EnumItems(0, &IEPStoreSiteAuthGUID, &mtGuid, 0, &enumItems);
  if (SUCCEEDED(hr) && enumItems != NULL) {
    LPWSTR itemName = NULL;
    while ((enumItems->Next(1, &itemName, 0) == S_OK) && itemName) {
      unsigned long count = 0;
      unsigned char* data = NULL;

      hr = aPStore->ReadItem(0, &IEPStoreSiteAuthGUID, &mtGuid, itemName,
                             &count, &data, NULL, 0);
      if (SUCCEEDED(hr) && data) {
        unsigned long i;
        unsigned char* password = NULL;
        for (i = 0; i < count; i++)
          if (data[i] == ':') {
            data[i] = '\0';
            if (i + 1 < count)
              password = &data[i + 1];
            break;
          }

        nsAutoString realm(itemName);
        if (Substring(realm, 0, 6).EqualsLiteral("DPAPI:")) 
          password = NULL; 

        if (password) {
          int idx;
          idx = realm.FindChar('/');
          if (idx) {
            realm.Replace(idx, 1, NS_LITERAL_STRING(" ("));
            realm.Append(')');
          }
          
          
          pwmgr->AddUser(NS_ConvertUTF16toUTF8(realm),
                         NS_ConvertASCIItoUTF16((char *)data),
                         NS_ConvertASCIItoUTF16((char *)password));
        }
        ::CoTaskMemFree(data);
      }
    }
  }
  return NS_OK;
}

nsresult
nsIEProfileMigrator::GetSignonsListFromPStore(IPStore* aPStore, nsVoidArray* aSignonsFound)
{
  HRESULT hr;

  NS_ENSURE_ARG_POINTER(aPStore);

  IEnumPStoreItemsPtr enumItems = NULL;
  hr = aPStore->EnumItems(0, &IEPStoreAutocompGUID, &IEPStoreAutocompGUID, 0, &enumItems);
  if (SUCCEEDED(hr) && enumItems != NULL) {
    LPWSTR itemName = NULL;
    while ((enumItems->Next(1, &itemName, 0) == S_OK) && itemName) {
      unsigned long count = 0;
      unsigned char* data = NULL;

      
      
      hr = aPStore->ReadItem(0, &IEPStoreAutocompGUID, &IEPStoreAutocompGUID, itemName, &count, &data, NULL, 0);
      if (SUCCEEDED(hr) && data) {
        nsAutoString itemNameString(itemName);
        if (StringTail(itemNameString, 11).
            LowerCaseEqualsLiteral(":stringdata")) {
          
          const nsAString& key = Substring(itemNameString, 0, itemNameString.Length() - 11);
          char* realm = nsnull;
          if (KeyIsURI(key, &realm)) {
            
            
            unsigned char* username = NULL;
            unsigned char* pass = NULL;
            GetUserNameAndPass(data, count, &username, &pass);

            if (username && pass) {
              
              
              
              
              SignonData* d = new SignonData;
              if (!d)
                return NS_ERROR_OUT_OF_MEMORY;
              d->user = (PRUnichar*)username;
              d->pass = (PRUnichar*)pass;
              d->realm = realm; 
              aSignonsFound->AppendElement(d);
            }
          }
        }
      }
    }
  }
  return NS_OK;
}

PRBool
nsIEProfileMigrator::KeyIsURI(const nsAString& aKey, char** aRealm)
{
  *aRealm = nsnull;

  nsCOMPtr<nsIURI> uri;

  if (NS_FAILED(NS_NewURI(getter_AddRefs(uri), aKey))) 
    return PR_FALSE;

  PRBool validScheme = PR_FALSE;
  const char* schemes[] = { "http", "https" };
  for (int i = 0; i < 2; ++i) {
    uri->SchemeIs(schemes[i], &validScheme);
    if (validScheme) {
      nsCAutoString realm;
      uri->GetScheme(realm);
      realm.AppendLiteral("://");

      nsCAutoString host;
      uri->GetHost(host);
      realm.Append(host);

      *aRealm = ToNewCString(realm);
      return validScheme;
    }
  }
  return PR_FALSE;
}

nsresult
nsIEProfileMigrator::ResolveAndMigrateSignons(IPStore* aPStore, nsVoidArray* aSignonsFound)
{
  HRESULT hr;

  IEnumPStoreItemsPtr enumItems = NULL;
  hr = aPStore->EnumItems(0, &IEPStoreAutocompGUID, &IEPStoreAutocompGUID, 0, &enumItems);
  if (SUCCEEDED(hr) && enumItems != NULL) {
    LPWSTR itemName = NULL;
    while ((enumItems->Next(1, &itemName, 0) == S_OK) && itemName) {
      unsigned long count = 0;
      unsigned char* data = NULL;

      hr = aPStore->ReadItem(0, &IEPStoreAutocompGUID, &IEPStoreAutocompGUID, itemName, &count, &data, NULL, 0);
      if (SUCCEEDED(hr) && data) {
        nsAutoString itemNameString(itemName);
        if (StringTail(itemNameString, 11).
            LowerCaseEqualsLiteral(":stringdata")) {
          
          const nsAString& key = Substring(itemNameString, 0, itemNameString.Length() - 11);
          
          
          
          nsCString realm;
          if (!KeyIsURI(key, getter_Copies(realm))) {
            
            EnumerateUsernames(key, (PRUnichar*)data, (count/sizeof(PRUnichar)), aSignonsFound);
          }
        }

        ::CoTaskMemFree(data);
      }
    }
    
    
    
    PRInt32 signonCount = aSignonsFound->Count();
    for (PRInt32 i = 0; i < signonCount; ++i) {
      SignonData* sd = (SignonData*)aSignonsFound->ElementAt(i);
      ::CoTaskMemFree(sd->user);  
      NS_Free(sd->realm);
      delete sd;
    }
  }
  return NS_OK;
}

void
nsIEProfileMigrator::EnumerateUsernames(const nsAString& aKey, PRUnichar* aData, unsigned long aCount, nsVoidArray* aSignonsFound)
{
  nsCOMPtr<nsIPasswordManagerInternal> pwmgr(do_GetService("@mozilla.org/passwordmanager;1"));
  if (!pwmgr)
    return;

  PRUnichar* cursor = aData;
  PRInt32 offset = 0;
  PRInt32 signonCount = aSignonsFound->Count();

  while (offset < aCount) {
    nsAutoString curr; curr = cursor;

    
    for (PRInt32 i = 0; i < signonCount; ++i) {
      SignonData* sd = (SignonData*)aSignonsFound->ElementAt(i);
      if (curr.Equals(sd->user)) {
        
        nsDependentString usernameStr(sd->user), passStr(sd->pass);
        nsDependentCString realm(sd->realm);
        pwmgr->AddUserFull(realm, usernameStr, passStr, aKey, EmptyString());
      }
    }

    
    PRInt32 advance = curr.Length() + 1;
    cursor += advance; 
    offset += advance;
  } 
}

void 
nsIEProfileMigrator::GetUserNameAndPass(unsigned char* data, unsigned long len, unsigned char** username, unsigned char** pass)
{
  *username = data;
  *pass = NULL;

  unsigned char* temp = data; 

  for (unsigned int i = 0; i < len; i += 2, temp += 2*sizeof(unsigned char)) {
    if (*temp == '\0') {
      *pass = temp + 2*sizeof(unsigned char);
      break;
    }
  }
}


















nsresult
nsIEProfileMigrator::CopyFormData(PRBool aReplace)
{
  HRESULT hr;

  HMODULE pstoreDLL = ::LoadLibrary("pstorec.dll");
  if (!pstoreDLL) {
    
    
    
    return NS_ERROR_FAILURE;
  }

  PStoreCreateInstancePtr PStoreCreateInstance = (PStoreCreateInstancePtr)::GetProcAddress(pstoreDLL, "PStoreCreateInstance");
  IPStorePtr PStore = NULL;
  hr = PStoreCreateInstance(&PStore, 0, 0, 0);
  if (FAILED(hr) || PStore == NULL)
    return NS_OK;

  IEnumPStoreItemsPtr enumItems = NULL;
  hr = PStore->EnumItems(0, &IEPStoreAutocompGUID, &IEPStoreAutocompGUID, 0, &enumItems);
  if (SUCCEEDED(hr) && enumItems != NULL) {
    LPWSTR itemName = NULL;
    while ((enumItems->Next(1, &itemName, 0) == S_OK) && itemName) {
      unsigned long count = 0;
      unsigned char* data = NULL;

      
      hr = PStore->ReadItem(0, &IEPStoreAutocompGUID, &IEPStoreAutocompGUID, itemName, &count, &data, NULL, 0);
      if (SUCCEEDED(hr) && data) {
        nsAutoString itemNameString(itemName);
        if (StringTail(itemNameString, 11).
            LowerCaseEqualsLiteral(":stringdata")) {
          
          const nsAString& key = Substring(itemNameString, 0, itemNameString.Length() - 11);
          nsCString realm;
          if (!KeyIsURI(key, getter_Copies(realm))) {
            nsresult rv = AddDataToFormHistory(key, (PRUnichar*)data, (count/sizeof(PRUnichar)));
            if (NS_FAILED(rv)) return rv;
          }
        }
      }
    }
  }
  return NS_OK;
}

nsresult
nsIEProfileMigrator::AddDataToFormHistory(const nsAString& aKey, PRUnichar* aData, unsigned long aCount)
{
  nsCOMPtr<nsIFormHistory2> formHistory(do_GetService("@mozilla.org/satchel/form-history;1"));
  if (!formHistory)
    return NS_ERROR_OUT_OF_MEMORY;

  PRUnichar* cursor = aData;
  PRInt32 offset = 0;

  while (offset < aCount) {
    nsAutoString curr; curr = cursor;

    formHistory->AddEntry(aKey, curr);

    
    PRInt32 advance = curr.Length() + 1;
    cursor += advance; 
    offset += advance;
  } 

  return NS_OK;
}







nsresult
nsIEProfileMigrator::CopyFavorites(PRBool aReplace) {
  
  
  nsresult rv;

#ifdef MOZ_PLACES_BOOKMARKS
  nsCOMPtr<nsINavBookmarksService> bms(do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt64 root;
  rv = bms->GetBookmarksRoot(&root);
  NS_ENSURE_SUCCESS(rv, rv);
#else
  nsCOMPtr<nsIRDFService> rdf(do_GetService("@mozilla.org/rdf/rdf-service;1"));
  nsCOMPtr<nsIRDFResource> root;
  rdf->GetResource(NS_LITERAL_CSTRING("NC:BookmarksRoot"), getter_AddRefs(root));

  nsCOMPtr<nsIBookmarksService> bms(do_GetService("@mozilla.org/browser/bookmarks-service;1"));
  NS_ENSURE_TRUE(bms, NS_ERROR_FAILURE);
  PRBool dummy;
  bms->ReadBookmarks(&dummy);
#endif

  nsAutoString personalToolbarFolderName;

#ifdef MOZ_PLACES_BOOKMARKS
  PRInt64 folder;
#else
  nsCOMPtr<nsIRDFResource> folder;
#endif
  if (!aReplace) {
    nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIStringBundle> bundle;
    bundleService->CreateBundle(TRIDENTPROFILE_BUNDLE, getter_AddRefs(bundle));

    nsString sourceNameIE;
    bundle->GetStringFromName(NS_LITERAL_STRING("sourceNameIE").get(), 
                              getter_Copies(sourceNameIE));

    const PRUnichar* sourceNameStrings[] = { sourceNameIE.get() };
    nsString importedIEFavsTitle;
    bundle->FormatStringFromName(NS_LITERAL_STRING("importedBookmarksFolder").get(),
                                 sourceNameStrings, 1, getter_Copies(importedIEFavsTitle));

#ifdef MOZ_PLACES_BOOKMARKS
    bms->CreateFolder(root, importedIEFavsTitle, -1, &folder);
#else
    bms->CreateFolderInContainer(importedIEFavsTitle.get(), root, -1, getter_AddRefs(folder));
#endif
  }
  else {
    
    
    nsCOMPtr<nsIWindowsRegKey> regKey = 
      do_CreateInstance("@mozilla.org/windows-registry-key;1");
    NS_NAMED_LITERAL_STRING(toolbarKey,
                            "Software\\Microsoft\\Internet Explorer\\Toolbar");
    if (regKey && 
        NS_SUCCEEDED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                                  toolbarKey, nsIWindowsRegKey::ACCESS_READ))) {
      nsAutoString linksFolderName;
      if (NS_SUCCEEDED(regKey->ReadStringValue(
                       NS_LITERAL_STRING("LinksFolderName"),
                       linksFolderName)))
        personalToolbarFolderName = linksFolderName; 
    }
    folder = root;
  }

  nsCOMPtr<nsIProperties> fileLocator(do_GetService("@mozilla.org/file/directory_service;1", &rv));
  if (NS_FAILED(rv)) 
      return rv;

  nsCOMPtr<nsIFile> favoritesDirectory;
  fileLocator->Get("Favs", NS_GET_IID(nsIFile), getter_AddRefs(favoritesDirectory));

  
  
  
  
  if (favoritesDirectory) {
    rv = ParseFavoritesFolder(favoritesDirectory, folder, bms, personalToolbarFolderName, PR_TRUE);
    if (NS_FAILED(rv)) return rv;
  }

  return CopySmartKeywords(root);
}

nsresult
#ifdef MOZ_PLACES_BOOKMARKS
nsIEProfileMigrator::CopySmartKeywords(PRInt64 aParentFolder)
#else
nsIEProfileMigrator::CopySmartKeywords(nsIRDFResource* aParentFolder)
#endif
{ 
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  NS_NAMED_LITERAL_STRING(searchUrlKey,
                          "Software\\Microsoft\\Internet Explorer\\SearchUrl");
  if (regKey && 
      NS_SUCCEEDED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                                searchUrlKey, nsIWindowsRegKey::ACCESS_READ))) {

#ifdef MOZ_PLACES_BOOKMARKS
    nsresult rv;
    nsCOMPtr<nsINavBookmarksService> bms(do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt64 keywordsFolder = 0;
#else
    nsCOMPtr<nsIBookmarksService> bms(do_GetService("@mozilla.org/browser/bookmarks-service;1"));
    nsCOMPtr<nsIRDFResource> keywordsFolder, bookmark;
#endif

    nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    
    nsCOMPtr<nsIStringBundle> bundle;
    bundleService->CreateBundle(TRIDENTPROFILE_BUNDLE, getter_AddRefs(bundle));


    int offset = 0;
    while (1) {
      nsAutoString keyName;
      if (NS_FAILED(regKey->GetChildName(offset, keyName)))
        break;

      if (!keywordsFolder) {
        nsString sourceNameIE;
        bundle->GetStringFromName(NS_LITERAL_STRING("sourceNameIE").get(), 
                                  getter_Copies(sourceNameIE));

        const PRUnichar* sourceNameStrings[] = { sourceNameIE.get() };
        nsString importedIESearchUrlsTitle;
        bundle->FormatStringFromName(NS_LITERAL_STRING("importedSearchURLsFolder").get(),
                                    sourceNameStrings, 1, getter_Copies(importedIESearchUrlsTitle));
#ifdef MOZ_PLACES_BOOKMARKS
        bms->CreateFolder(aParentFolder, importedIESearchUrlsTitle, -1, &keywordsFolder);
#else
        bms->CreateFolderInContainer(importedIESearchUrlsTitle.get(), aParentFolder, -1, 
                                     getter_AddRefs(keywordsFolder));
#endif
      }

      nsCOMPtr<nsIWindowsRegKey> childKey; 
      if (NS_SUCCEEDED(regKey->OpenChild(keyName,
                       nsIWindowsRegKey::ACCESS_READ,
                       getter_AddRefs(childKey)))) {
        nsAutoString url; 
        if (NS_SUCCEEDED(childKey->ReadStringValue(EmptyString(), url))) {
          nsCOMPtr<nsIURI> uri;
          if (NS_FAILED(NS_NewURI(getter_AddRefs(uri), url))) {
            NS_WARNING("Invalid url while importing smart keywords of MS IE");
            ++offset;
            childKey->Close();
            continue;
          }
#ifdef MOZ_PLACES_BOOKMARKS
          PRInt64 id;
          bms->InsertItem(keywordsFolder, uri,
                          nsINavBookmarksService::DEFAULT_INDEX, &id);
          bms->SetItemTitle(id, keyName);
#else
          nsCAutoString hostCStr;
          uri->GetHost(hostCStr);
          NS_ConvertUTF8toUTF16 host(hostCStr); 

          const PRUnichar* nameStrings[] = { host.get() };
          nsString keywordName;
          nsresult rv = bundle->FormatStringFromName(
                        NS_LITERAL_STRING("importedSearchURLsTitle").get(),
                        nameStrings, 1, getter_Copies(keywordName));

          const PRUnichar* descStrings[] = { keyName.get(), host.get() };
          nsString keywordDesc;
          rv = bundle->FormatStringFromName(
                       NS_LITERAL_STRING("importedSearchUrlDesc").get(),
                       descStrings, 2, getter_Copies(keywordDesc));
          bms->CreateBookmarkInContainer(keywordName.get(), 
                                         url.get(),
                                         keyName.get(), 
                                         keywordDesc.get(), 
                                         nsnull,
                                         nsnull, 
                                         keywordsFolder, 
                                         -1, 
                                         getter_AddRefs(bookmark));
#endif
        }
        childKey->Close();
      }

      ++offset;
    }
  }

  return NS_OK;
}

void 
nsIEProfileMigrator::ResolveShortcut(const nsString &aFileName, char** aOutURL) 
{
  HRESULT result;

  IUniformResourceLocator* urlLink = nsnull;
  result = ::CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                              IID_IUniformResourceLocator, (void**)&urlLink);
  if (SUCCEEDED(result) && urlLink) {
    IPersistFile* urlFile = nsnull;
    result = urlLink->QueryInterface(IID_IPersistFile, (void**)&urlFile);
    if (SUCCEEDED(result) && urlFile) {
      result = urlFile->Load(aFileName.get(), STGM_READ);
      if (SUCCEEDED(result) ) {
        LPSTR lpTemp = nsnull;
        result = urlLink->GetURL(&lpTemp);
        if (SUCCEEDED(result) && lpTemp) {
          *aOutURL = PL_strdup(lpTemp);

          
          ::CoTaskMemFree(lpTemp);
        }
      }
      urlFile->Release();
    }
    urlLink->Release();
  }
}

nsresult
nsIEProfileMigrator::ParseFavoritesFolder(nsIFile* aDirectory, 
#ifdef MOZ_PLACES_BOOKMARKS
                                          PRInt64 aParentFolder,
                                          nsINavBookmarksService* aBookmarksService,
#else
                                          nsIRDFResource* aParentResource,
                                          nsIBookmarksService* aBookmarksService,
#endif 
                                          const nsAString& aPersonalToolbarFolderName,
                                          PRBool aIsAtRootLevel)
{
  nsresult rv;

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = aDirectory->GetDirectoryEntries(getter_AddRefs(entries));
  if (NS_FAILED(rv)) return rv;

  do {
    PRBool hasMore = PR_FALSE;
    rv = entries->HasMoreElements(&hasMore);
    if (NS_FAILED(rv) || !hasMore) break;

    nsCOMPtr<nsISupports> supp;
    rv = entries->GetNext(getter_AddRefs(supp));
    if (NS_FAILED(rv)) break;

    nsCOMPtr<nsIFile> currFile(do_QueryInterface(supp));

    nsCOMPtr<nsIURI> uri;
    rv = NS_NewFileURI(getter_AddRefs(uri), currFile);
    if (NS_FAILED(rv)) break;

    nsAutoString bookmarkName;
    currFile->GetLeafName(bookmarkName);

    PRBool isSymlink = PR_FALSE;
    PRBool isDir = PR_FALSE;

    currFile->IsSymlink(&isSymlink);
    currFile->IsDirectory(&isDir);

    if (isSymlink) {
      
      
      

      
      nsAutoString path;
      rv = currFile->GetTarget(path);
      if (NS_FAILED(rv)) continue;

      nsCOMPtr<nsILocalFile> localFile;
      rv = NS_NewLocalFile(path, PR_TRUE, getter_AddRefs(localFile));
      if (NS_FAILED(rv)) continue;

      
      
      rv = localFile->IsDirectory(&isDir);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!isDir) continue;

      
      NS_NAMED_LITERAL_STRING(lnkExt, ".lnk");
      PRInt32 lnkExtStart = bookmarkName.Length() - lnkExt.Length();
      if (StringEndsWith(bookmarkName, lnkExt,
                         CaseInsensitiveCompare))
        bookmarkName.SetLength(lnkExtStart);

#ifdef MOZ_PLACES_BOOKMARKS
      nsCOMPtr<nsIURI> bookmarkURI;
      rv = NS_NewFileURI(getter_AddRefs(bookmarkURI), localFile);
      NS_ENSURE_SUCCESS(rv, rv);
      PRInt64 id;
      rv = aBookmarksService->InsertItem(aParentFolder, bookmarkURI,
                                         nsINavBookmarksService::DEFAULT_INDEX, &id);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aBookmarksService->SetItemTitle(id, bookmarkName);
      NS_ENSURE_SUCCESS(rv, rv);
#else
      nsCAutoString spec;
      nsCOMPtr<nsIFile> filePath(localFile);
      
      rv = NS_GetURLSpecFromFile(filePath, spec);
      if (NS_FAILED(rv)) continue;

      nsCOMPtr<nsIRDFResource> bookmark;
      
      
      
      
      aBookmarksService->CreateBookmarkInContainer(bookmarkName.get(), 
                                                   NS_ConvertUTF8toUTF16(spec).get(), 
                                                   nsnull,
                                                   nsnull, 
                                                   nsnull, 
                                                   nsnull, 
                                                   aParentResource, 
                                                   -1, 
                                                   getter_AddRefs(bookmark));
#endif
      if (NS_FAILED(rv)) continue;
    }
    else if (isDir) {
#ifdef MOZ_PLACES_BOOKMARKS
      PRInt64 folder;
#else
      nsCOMPtr<nsIRDFResource> folder;
#endif
      if (bookmarkName.Equals(aPersonalToolbarFolderName)) {
#ifdef MOZ_PLACES_BOOKMARKS
        aBookmarksService->GetToolbarFolder(&folder);
        
        
        aBookmarksService->RemoveFolderChildren(folder);
#else
        aBookmarksService->GetBookmarksToolbarFolder(getter_AddRefs(folder));
        
        
        
        nsCOMPtr<nsIRDFContainer> ctr(do_CreateInstance("@mozilla.org/rdf/container;1"));
        nsCOMPtr<nsIRDFDataSource> bmds(do_QueryInterface(aBookmarksService));
        ctr->Init(bmds, folder);

        nsCOMPtr<nsISimpleEnumerator> e;
        ctr->GetElements(getter_AddRefs(e));

        PRBool hasMore;
        e->HasMoreElements(&hasMore);
        while (hasMore) {
          nsCOMPtr<nsIRDFResource> b;
          e->GetNext(getter_AddRefs(b));

          ctr->RemoveElement(b, PR_FALSE);

          e->HasMoreElements(&hasMore);
        }
#endif
      }
      else {
#ifdef MOZ_PLACES_BOOKMARKS
        rv = aBookmarksService->CreateFolder(aParentFolder,
                                             bookmarkName,
                                             nsINavBookmarksService::DEFAULT_INDEX,
                                             &folder);
#else
        rv = aBookmarksService->CreateFolderInContainer(bookmarkName.get(), 
                                                        aParentResource, 
                                                        -1, 
                                                        getter_AddRefs(folder));
#endif
        if (NS_FAILED(rv)) continue;
      }

      rv = ParseFavoritesFolder(currFile, folder, aBookmarksService, aPersonalToolbarFolderName, PR_FALSE);
      if (NS_FAILED(rv)) continue;
    }
    else {
      nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
      nsCAutoString extension;

      url->GetFileExtension(extension);
      if (!extension.Equals("url", CaseInsensitiveCompare))
        continue;

      nsAutoString name(Substring(bookmarkName, 0, 
                                  bookmarkName.Length() - extension.Length() - 1));

      nsAutoString path;
      currFile->GetPath(path);

      nsCString resolvedURL;
      ResolveShortcut(path, getter_Copies(resolvedURL));

#ifdef MOZ_PLACES_BOOKMARKS
      nsCOMPtr<nsIURI> resolvedURI;
      rv = NS_NewURI(getter_AddRefs(resolvedURI), resolvedURL);
      NS_ENSURE_SUCCESS(rv, rv);
      PRInt64 id;
      rv = aBookmarksService->InsertItem(aParentFolder, resolvedURI,
                                         nsINavBookmarksService::DEFAULT_INDEX, &id);
      if (NS_FAILED(rv)) continue;
      rv = aBookmarksService->SetItemTitle(id, name);
#else
      nsCOMPtr<nsIRDFResource> bookmark;
      
      
      
      
      
      rv = aBookmarksService->CreateBookmarkInContainer(name.get(), 
                                                        NS_ConvertUTF8toUTF16(resolvedURL).get(), 
                                                        nsnull, 
                                                        nsnull, 
                                                        nsnull, 
                                                        nsnull, 
                                                        aParentResource, 
                                                        -1, 
                                                        getter_AddRefs(bookmark));
#endif
      if (NS_FAILED(rv)) continue;
    }
  }
  while (1);

  return rv;
}

nsresult
nsIEProfileMigrator::CopyPreferences(PRBool aReplace) 
{
  PRBool          regKeyOpen = PR_FALSE;
  const regEntry  *entry,
                  *endEntry = gRegEntries + NS_ARRAY_LENGTH(gRegEntries);
                              

  nsCOMPtr<nsIPrefBranch> prefs;

  { 
    nsCOMPtr<nsIPrefService> pserve(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (pserve)
      pserve->GetBranch("", getter_AddRefs(prefs));
  }
  if (!prefs)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!regKey)
    return NS_ERROR_UNEXPECTED;
  
  
  for (entry = gRegEntries; entry < endEntry; ++entry) {

    
    if (entry->regKeyName) {
      if (regKeyOpen) {
        regKey->Close();
        regKeyOpen = PR_FALSE;
      }
      regKeyOpen = NS_SUCCEEDED(regKey->
                                Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                                NS_ConvertASCIItoUTF16(
                                  nsDependentCString(entry->regKeyName)),
                                nsIWindowsRegKey::ACCESS_READ));
    }

    if (regKeyOpen) 
      
      entry->entryHandler(regKey,
                          NS_ConvertASCIItoUTF16(
                            nsDependentCString(entry->regValueName)),
                          prefs, entry->prefKeyName);
  }

  nsresult rv = CopySecurityPrefs(prefs);
  if (NS_FAILED(rv)) return rv;

  rv = CopyProxyPreferences(prefs);
  if (NS_FAILED(rv)) return rv;

  return CopyStyleSheet(aReplace);
}



nsresult
nsIEProfileMigrator::CopyCookies(PRBool aReplace) 
{
  
  
  PRBool rv = NS_OK;

  nsCOMPtr<nsIFile> cookiesDir;
  nsCOMPtr<nsISimpleEnumerator> cookieFiles;

  nsCOMPtr<nsICookieManager2> cookieManager(do_GetService(NS_COOKIEMANAGER_CONTRACTID));
  if (!cookieManager)
    return NS_ERROR_FAILURE;

  
  NS_GetSpecialDirectory(NS_WIN_COOKIES_DIR, getter_AddRefs(cookiesDir));
  if (cookiesDir)
    cookiesDir->GetDirectoryEntries(getter_AddRefs(cookieFiles));
  if (!cookieFiles)
    return NS_ERROR_FAILURE;

  
  PRUnichar username[sUsernameLengthLimit+2];
  ::GetEnvironmentVariableW(L"USERNAME", username,
                            sizeof(username)/sizeof(PRUnichar));
  username[sUsernameLengthLimit] = L'\0';
  wcscat(username, L"@");
  int usernameLength = wcslen(username);

  
  char *fileContents = (char *) PR_Malloc(sInitialCookieBufferSize);
  if (!fileContents)
    return NS_ERROR_OUT_OF_MEMORY;
  PRUint32 fileContentsSize = sInitialCookieBufferSize;

  do { 
    
    PRBool moreFiles = PR_FALSE;
    if (NS_FAILED(cookieFiles->HasMoreElements(&moreFiles)) || !moreFiles)
      break;

    nsCOMPtr<nsISupports> supFile;
    cookieFiles->GetNext(getter_AddRefs(supFile));
    nsCOMPtr<nsIFile> cookieFile(do_QueryInterface(supFile));
    if (!cookieFile) {
      rv = NS_ERROR_FAILURE;
      break; 
    }

    
    nsAutoString fileName;
    cookieFile->GetLeafName(fileName);
    const nsAString &fileOwner = Substring(fileName, 0, usernameLength);
    if (!fileOwner.Equals(username, CaseInsensitiveCompare))
      continue;

    
    
    PRInt64 llFileSize;
    if (NS_FAILED(cookieFile->GetFileSize(&llFileSize)))
      continue;

    PRUint32 fileSize, readSize;
    LL_L2UI(fileSize, llFileSize);
    if (fileSize >= fileContentsSize) {
      PR_Free(fileContents);
      fileContents = (char *) PR_Malloc(fileSize+1);
      if (!fileContents) {
        rv = NS_ERROR_FAILURE;
        break; 
      }
      fileContentsSize = fileSize;
    }

    
    PRFileDesc *fd;
    nsCOMPtr<nsILocalFile> localCookieFile(do_QueryInterface(cookieFile));
    if (localCookieFile &&
        NS_SUCCEEDED(localCookieFile->OpenNSPRFileDesc(PR_RDONLY, 0444, &fd))) {

      readSize = PR_Read(fd, fileContents, fileSize);
      PR_Close(fd);

      if (fileSize == readSize) { 
        nsresult onerv;
        onerv = CopyCookiesFromBuffer(fileContents, readSize, cookieManager);
        if (NS_FAILED(onerv))
          rv = onerv;
      }
    }
  } while(1);

  if (fileContents)
    PR_Free(fileContents);
  return rv;
}



nsresult
nsIEProfileMigrator::CopyCookiesFromBuffer(char *aBuffer,
                                           PRUint32 aBufferLength,
                                           nsICookieManager2 *aCookieManager) 
{
  nsresult  rv = NS_OK;

  const char *bufferEnd = aBuffer + aBufferLength;
  
  char    *name,
          *value,
          *host,
          *path,
          *flags,
          *expirationDate1, *expirationDate2,
          *creationDate1, *creationDate2,
          *terminator;
  int      flagsValue;
  time_t   expirationDate,
           creationDate;
  char     hostCopy[sHostnameLengthLimit+1],
          *hostCopyConstructor,
          *hostCopyEnd = hostCopy + sHostnameLengthLimit;

  do { 
    DelimitField(&aBuffer, bufferEnd, &name);
    DelimitField(&aBuffer, bufferEnd, &value);
    DelimitField(&aBuffer, bufferEnd, &host);
    DelimitField(&aBuffer, bufferEnd, &flags);
    DelimitField(&aBuffer, bufferEnd, &expirationDate1);
    DelimitField(&aBuffer, bufferEnd, &expirationDate2);
    DelimitField(&aBuffer, bufferEnd, &creationDate1);
    DelimitField(&aBuffer, bufferEnd, &creationDate2);
    DelimitField(&aBuffer, bufferEnd, &terminator);

    
    if (terminator >= bufferEnd)
      break;

    
    if (*value == '\0')
      continue;

    
    ::sscanf(flags, "%d", &flagsValue);
    expirationDate = FileTimeToTimeT(expirationDate1, expirationDate2);
    creationDate = FileTimeToTimeT(creationDate1, creationDate2);

    

    hostCopyConstructor = hostCopy;

    
    
    if (*host && *host != '.' && *host != '/')
      *hostCopyConstructor++ = '.';

    
    for (path = host; *path && *path != '/'; ++path)
      ;
    int hostLength = path - host;
    if (hostLength > hostCopyEnd - hostCopyConstructor)
      hostLength = hostCopyEnd - hostCopyConstructor;
    PL_strncpy(hostCopyConstructor, host, hostLength);
    hostCopyConstructor += hostLength;

    *hostCopyConstructor = '\0';

    nsDependentCString stringName(name),
                       stringPath(path);

    
    if (hostCopy[0] == '.')
      aCookieManager->Remove(nsDependentCString(hostCopy+1),
                             stringName, stringPath, PR_FALSE);

    nsresult onerv;
    
    onerv = aCookieManager->Add(nsDependentCString(hostCopy),
                                stringPath,
                                stringName,
                                nsDependentCString(value),
                                flagsValue & 0x1,
                                PR_FALSE,
                                PRInt64(expirationDate));
    if (NS_FAILED(onerv)) {
      rv = onerv;
      break;
    }

  } while(aBuffer < bufferEnd);

  return rv;
}












void
nsIEProfileMigrator::DelimitField(char **aBuffer,
                                  const char *aBufferEnd,
                                  char **aField) 
{
  char *scan = *aBuffer;
  *aField = scan;
  while (scan < aBufferEnd && (*scan != '\r' && *scan != '\n'))
    ++scan;
  if (scan+1 < aBufferEnd && (*(scan+1) == '\r' || *(scan+1) == '\n') &&
                             *scan != *(scan+1)) {
    *scan = '\0';
    scan += 2;
  } else {
    if (scan <= aBufferEnd) 
      *scan = '\0';
    ++scan;
  }
  *aBuffer = scan;
}


time_t
nsIEProfileMigrator::FileTimeToTimeT(const char *aLowDateIntString,
                                     const char *aHighDateIntString) 
{
  FILETIME   fileTime;
  SYSTEMTIME systemTime;
  tm         tTime;
  time_t     rv;

  ::sscanf(aLowDateIntString, "%ld", &fileTime.dwLowDateTime);
  ::sscanf(aHighDateIntString, "%ld", &fileTime.dwHighDateTime);
  ::FileTimeToSystemTime(&fileTime, &systemTime);
  tTime.tm_year = systemTime.wYear - 1900;
  tTime.tm_mon = systemTime.wMonth-1;
  tTime.tm_mday = systemTime.wDay;
  tTime.tm_hour = systemTime.wHour;
  tTime.tm_min = systemTime.wMinute;
  tTime.tm_sec = systemTime.wSecond;
  tTime.tm_isdst = -1;
  rv = ::mktime(&tTime);
  return rv < 0 ? 0 : rv;
}



nsresult
nsIEProfileMigrator::CopyStyleSheet(PRBool aReplace) 
{
  nsresult rv = NS_OK; 

  
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!regKey)
    return NS_ERROR_UNEXPECTED;

  NS_NAMED_LITERAL_STRING(styleKey,
                          "Software\\Microsoft\\Internet Explorer\\Styles");
  if (NS_FAILED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                             styleKey, nsIWindowsRegKey::ACCESS_READ)))
    return NS_OK;

  NS_NAMED_LITERAL_STRING(myStyleValName, "Use My StyleSheet");
  PRUint32 type, useMyStyle; 
  if (NS_SUCCEEDED(regKey->GetValueType(myStyleValName, &type)) &&
      type == nsIWindowsRegKey::TYPE_INT &&
      NS_SUCCEEDED(regKey->ReadIntValue(myStyleValName, &useMyStyle)) &&
      useMyStyle == 1) {

    nsAutoString tridentFilename;
    if (NS_SUCCEEDED(regKey->ReadStringValue(
                     NS_LITERAL_STRING("User Stylesheet"), tridentFilename))) {

      
      
      nsCOMPtr<nsILocalFile> tridentFile(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
      if (tridentFile) {
        PRBool exists;

        tridentFile->InitWithPath(tridentFilename);
        tridentFile->Exists(&exists);
        if (exists) {
          
          nsCOMPtr<nsIFile> chromeDir;
          NS_GetSpecialDirectory(NS_APP_USER_CHROME_DIR,
                                 getter_AddRefs(chromeDir));
          if (chromeDir)
            rv = tridentFile->CopyTo(chromeDir,
                              NS_LITERAL_STRING("userContent.css"));
        }
      }
    }
  }
  return rv;
}

void
nsIEProfileMigrator::GetUserStyleSheetFile(nsIFile **aUserFile) 
{
  nsCOMPtr<nsIFile> userChrome;

  *aUserFile = 0;

  
  NS_GetSpecialDirectory(NS_APP_USER_CHROME_DIR, getter_AddRefs(userChrome));

  if (userChrome) {
    PRBool exists;
    userChrome->Exists(&exists);
    if (!exists &&
        NS_FAILED(userChrome->Create(nsIFile::DIRECTORY_TYPE, 0755)))
      return;

    
    userChrome->Append(NS_LITERAL_STRING("userContent.css"));
    *aUserFile = userChrome;
    NS_ADDREF(*aUserFile);
  }
}

nsresult
nsIEProfileMigrator::CopySecurityPrefs(nsIPrefBranch* aPrefs)
{
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  NS_NAMED_LITERAL_STRING(key,
      "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
  if (regKey && 
      NS_SUCCEEDED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                                key, nsIWindowsRegKey::ACCESS_READ))) {
    
    PRUint32 value;
    if (NS_SUCCEEDED(regKey->ReadIntValue(NS_LITERAL_STRING("SecureProtocols"),
                                          &value))) { 
      aPrefs->SetBoolPref("security.enable_ssl2", (value >> 3) & PR_TRUE);
      aPrefs->SetBoolPref("security.enable_ssl3", (value >> 5) & PR_TRUE);
      aPrefs->SetBoolPref("security.enable_tls",  (value >> 7) & PR_TRUE);
    }
  }

  return NS_OK;
}

struct ProxyData {
  char*   prefix;
  PRInt32 prefixLength;
  PRBool  proxyConfigured;
  char*   hostPref;
  char*   portPref;
};

nsresult
nsIEProfileMigrator::CopyProxyPreferences(nsIPrefBranch* aPrefs)
{
  nsCOMPtr<nsIWindowsRegKey> regKey =
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  NS_NAMED_LITERAL_STRING(key,
    "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
  if (regKey && 
      NS_SUCCEEDED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                                key, nsIWindowsRegKey::ACCESS_READ))) {
    nsAutoString buf; 
    
    
    if (NS_SUCCEEDED(regKey->
                     ReadStringValue(NS_LITERAL_STRING("AutoConfigURL"), buf))) {
      
      
      SetUnicharPref("network.proxy.autoconfig_url", buf, aPrefs);
      aPrefs->SetIntPref("network.proxy.type", 2);
    }

    
    PRUint32 enabled;
    if (NS_SUCCEEDED(regKey->
                     ReadIntValue(NS_LITERAL_STRING("ProxyEnable"), &enabled)))
      aPrefs->SetIntPref("network.proxy.type", (enabled & 0x1) ? 1 : 0); 
    
    if (NS_SUCCEEDED(regKey->
                     ReadStringValue(NS_LITERAL_STRING("ProxyOverride"), buf)))
      ParseOverrideServers(buf, aPrefs);

    if (NS_SUCCEEDED(regKey->
                     ReadStringValue(NS_LITERAL_STRING("ProxyServer"), buf))) {

      ProxyData data[] = {
        { "ftp=",     4, PR_FALSE, "network.proxy.ftp",
          "network.proxy.ftp_port"    },
        { "gopher=",  7, PR_FALSE, "network.proxy.gopher",
          "network.proxy.gopher_port" },
        { "http=",    5, PR_FALSE, "network.proxy.http",
          "network.proxy.http_port"   },
        { "https=",   6, PR_FALSE, "network.proxy.ssl",
          "network.proxy.ssl_port"    },
        { "socks=",   6, PR_FALSE, "network.proxy.socks",
          "network.proxy.socks_port"  },
      };

      PRInt32 startIndex = 0, count = 0;
      PRBool foundSpecificProxy = PR_FALSE;
      for (PRUint32 i = 0; i < 5; ++i) {
        PRInt32 offset = buf.Find(NS_ConvertASCIItoUTF16(data[i].prefix));
        if (offset >= 0) {
          foundSpecificProxy = PR_TRUE;

          data[i].proxyConfigured = PR_TRUE;

          startIndex = offset + data[i].prefixLength;

          PRInt32 terminal = buf.FindChar(';', offset);
          count = terminal > startIndex ? terminal - startIndex : 
                                          buf.Length() - startIndex;

          
          SetProxyPref(Substring(buf, startIndex, count), data[i].hostPref,
                       data[i].portPref, aPrefs);
        }
      }

      if (!foundSpecificProxy) {
        
        
        
        for (PRUint32 i = 0; i < 5; ++i)
          SetProxyPref(buf, data[i].hostPref, data[i].portPref, aPrefs);
        aPrefs->SetBoolPref("network.proxy.share_proxy_settings", PR_TRUE);
      }
    }

  }

  return NS_OK;
}











































#include "mozilla/Util.h"

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
#include "nsILocalFileWin.h"
#include "nsAutoPtr.h"

#include "prnetdb.h"

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
#include "nsILoginManagerIEMigrationHelper.h"
#include "nsILoginInfo.h"
#include "nsIFormHistory.h"
#include "nsIRDFService.h"
#include "nsIRDFContainer.h"
#include "nsIURL.h"
#include "nsINavBookmarksService.h"
#include "nsBrowserCompsCID.h"
#include "nsIStringBundle.h"
#include "nsNetUtil.h"
#include "nsToolkitCompsCID.h"
#include "nsUnicharUtils.h"
#include "nsIWindowsRegKey.h"
#include "nsISupportsPrimitives.h"

#define TRIDENTPROFILE_BUNDLE       "chrome://browser/locale/migration/migration.properties"

#define REGISTRY_IE_MAIN_KEY \
  NS_LITERAL_STRING("Software\\Microsoft\\Internet Explorer\\Main")
#define REGISTRY_IE_TYPEDURL_KEY \
  NS_LITERAL_STRING("Software\\Microsoft\\Internet Explorer\\TypedURLs")
#define REGISTRY_IE_TOOLBAR_KEY \
  NS_LITERAL_STRING("Software\\Microsoft\\Internet Explorer\\Toolbar")
#define REGISTRY_IE_SEARCHURL_KEY \
  NS_LITERAL_STRING("Software\\Microsoft\\Internet Explorer\\SearchUrl")

using namespace mozilla;

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
                   reinterpret_cast<PRUint32 *>(&prefIntValue))))
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
  bool    skip = false,
          comma = false;

  while (source < sourceEnd && *source && dest < destEnd) {
    if (*source == ',')
      skip = false;
    else if (*source == ';')
      skip = true;
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
    TranslateYNtoFT }
};

#if 0
user_pref("font.size.fixed.x-western", 14);
user_pref("font.size.variable.x-western", 15);
#endif



NS_IMETHODIMP
nsIEProfileMigrator::Migrate(PRUint16 aItems, nsIProfileStartup* aStartup, const PRUnichar* aProfile)
{
  nsresult rv = NS_OK;

  bool aReplace = false;

  if (aStartup) {
    aReplace = true;
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
                                    bool aReplace,
                                    PRUint16* aResult)
{
  if (TestForIE7()) {
    
    
    *aResult = nsIBrowserProfileMigrator::SETTINGS |
               nsIBrowserProfileMigrator::COOKIES | 
               nsIBrowserProfileMigrator::HISTORY |
               nsIBrowserProfileMigrator::BOOKMARKS;
  }
  else {
    *aResult = nsIBrowserProfileMigrator::SETTINGS |
               nsIBrowserProfileMigrator::COOKIES | 
               nsIBrowserProfileMigrator::HISTORY |
               nsIBrowserProfileMigrator::FORMDATA |
               nsIBrowserProfileMigrator::PASSWORDS |
               nsIBrowserProfileMigrator::BOOKMARKS;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsIEProfileMigrator::GetSourceExists(bool* aResult)
{
  
  *aResult = true;

  return NS_OK;
}

NS_IMETHODIMP
nsIEProfileMigrator::GetSourceHasMultipleProfiles(bool* aResult)
{
  *aResult = false;
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
  if (!regKey ||
      NS_FAILED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                             REGISTRY_IE_MAIN_KEY,
                             nsIWindowsRegKey::ACCESS_READ)))
    return NS_OK;
  
  NS_NAMED_LITERAL_STRING(homeURLValName, "Start Page");
  nsAutoString homeURLVal;

  if (NS_SUCCEEDED(regKey->ReadStringValue(homeURLValName, homeURLVal))) {
    
    
    
    
    nsCAutoString  homePageURL;
    nsCOMPtr<nsIURI> homePageURI;

    if (NS_SUCCEEDED(NS_NewURI(getter_AddRefs(homePageURI), homeURLVal))) {
      if (NS_SUCCEEDED(homePageURI->GetSpec(homePageURL)) && !homePageURL.IsEmpty()) {
          aResult.Assign(homePageURL);
      }
    }
  }

  
  
  
  NS_NAMED_LITERAL_STRING(ssRegKeyName, "Secondary Start Pages");
  nsAutoString secondaryList;

  if (NS_SUCCEEDED(regKey->ReadStringValue(ssRegKeyName, secondaryList)) &&
      !secondaryList.IsEmpty()) {
    nsTArray<nsCString> parsedList;
    if (!ParseString(NS_ConvertUTF16toUTF8(secondaryList), '\0', parsedList))
      return NS_OK;

    
    for (PRUint32 index = 0; index < parsedList.Length(); ++index) {
      nsCOMPtr<nsIURI> uri;
      nsCAutoString homePage;
      
      
      if (NS_SUCCEEDED(NS_NewURI(getter_AddRefs(uri), parsedList[index]))) {
        if (NS_SUCCEEDED(uri->GetSpec(homePage)) && !homePage.IsEmpty()) {
            aResult.AppendLiteral("|");
            aResult.Append(homePage);
        }
      }
    }
  }

  return NS_OK;
}




NS_IMPL_ISUPPORTS2(nsIEProfileMigrator, nsIBrowserProfileMigrator, nsINavHistoryBatchCallback);

nsIEProfileMigrator::nsIEProfileMigrator()
{
  mObserverService = do_GetService("@mozilla.org/observer-service;1");
}

nsIEProfileMigrator::~nsIEProfileMigrator() 
{
}



bool 
nsIEProfileMigrator::TestForIE7()
{
  nsCOMPtr<nsIWindowsRegKey> regKey =  
    do_CreateInstance("@mozilla.org/windows-registry-key;1"); 
  if (!regKey)  
    return false;  

  NS_NAMED_LITERAL_STRING(key,
      "Applications\\iexplore.exe\\shell\\open\\command");
  if (NS_FAILED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                             key, nsIWindowsRegKey::ACCESS_QUERY_VALUE)))
    return false;

  nsAutoString iePath;
  if (NS_FAILED(regKey->ReadStringValue(EmptyString(), iePath)))
    return false; 

  
  PRUint32 bufLength =  
    ::ExpandEnvironmentStringsW(iePath.get(), 
                               L"", 0); 
  if (bufLength == 0) 
    return false; 

  nsAutoArrayPtr<PRUnichar> destination(new PRUnichar[bufLength]); 
  if (!destination) 
    return false; 

  if (!::ExpandEnvironmentStringsW(iePath.get(), 
                                   destination, 
                                   bufLength)) 
    return false; 

  iePath = destination; 

  if (StringBeginsWith(iePath, NS_LITERAL_STRING("\""))) {
    iePath.Cut(0,1);
    PRUint32 index = iePath.FindChar('\"', 0);
    if (index > 0)
      iePath.Cut(index,iePath.Length());
  }

  nsCOMPtr<nsILocalFile> lf; 
  NS_NewLocalFile(iePath, true, getter_AddRefs(lf)); 

  nsCOMPtr<nsILocalFileWin> lfw = do_QueryInterface(lf); 
  if (!lfw)
   return false;
   
  nsAutoString ieVersion;
  if (NS_FAILED(lfw->GetVersionInfoField("FileVersion", ieVersion)))
   return false;

  if (ieVersion.Length() > 2) {
    PRUint32 index = ieVersion.FindChar('.', 0);
    if (index < 0)
      return false;
    ieVersion.Cut(index, ieVersion.Length());
    PRInt32 ver = wcstol(ieVersion.get(), nsnull, 0);
    if (ver >= 7) 
      return true;
  }

  return false;
}

NS_IMETHODIMP
nsIEProfileMigrator::RunBatched(nsISupports* aUserData)
{
  PRUint8 batchAction;
  nsCOMPtr<nsISupportsPRUint8> strWrapper(do_QueryInterface(aUserData));
  NS_ASSERTION(strWrapper, "Unable to create nsISupportsPRUint8 wrapper!");
  nsresult rv = strWrapper->GetData(&batchAction);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (batchAction) {
    case BATCH_ACTION_HISTORY:
      rv = CopyHistoryBatched(false);
      break;
    case BATCH_ACTION_HISTORY_REPLACE:
      rv = CopyHistoryBatched(true);
      break;
    case BATCH_ACTION_BOOKMARKS:
      rv = CopyFavoritesBatched(false);
      break;
    case BATCH_ACTION_BOOKMARKS_REPLACE:
      rv = CopyFavoritesBatched(true);
      break;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsIEProfileMigrator::CopyHistory(bool aReplace)
{
  nsresult rv;
  nsCOMPtr<nsINavHistoryService> history =
    do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint8 batchAction = aReplace ? BATCH_ACTION_HISTORY_REPLACE
                                 : BATCH_ACTION_HISTORY;
  nsCOMPtr<nsISupportsPRUint8> supports =
    do_CreateInstance(NS_SUPPORTS_PRUINT8_CONTRACTID);
  NS_ENSURE_TRUE(supports, NS_ERROR_OUT_OF_MEMORY);
  rv = supports->SetData(batchAction);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = history->RunInBatchMode(this, supports);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsIEProfileMigrator::CopyHistoryBatched(bool aReplace)
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
      bool validScheme = false;
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
  if (regKey && 
      NS_SUCCEEDED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                                REGISTRY_IE_TYPEDURL_KEY,
                                nsIWindowsRegKey::ACCESS_READ))) {
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






static GUID IEPStoreAutocompGUID = { 0xe161255a, 0x37c3, 0x11d2, { 0xbc, 0xaa, 0x00, 0xc0, 0x4f, 0xd9, 0x29, 0xdb } };
static GUID IEPStoreSiteAuthGUID = { 0x5e7e8100, 0x9138, 0x11d1, { 0x94, 0x5a, 0x00, 0xc0, 0x4f, 0xc3, 0x08, 0xff } };



















































nsresult
nsIEProfileMigrator::CopyPasswords(bool aReplace)
{
  HRESULT hr;
  nsresult rv;
  nsTArray<SignonData> signonsFound;

  HMODULE pstoreDLL = ::LoadLibraryW(L"pstorec.dll");
  if (!pstoreDLL) {
    
    
    
    return NS_ERROR_FAILURE;
  }

  PStoreCreateInstancePtr PStoreCreateInstance = (PStoreCreateInstancePtr)::GetProcAddress(pstoreDLL, "PStoreCreateInstance");
  IPStore* PStore;
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

  nsCOMPtr<nsILoginManagerIEMigrationHelper> pwmgr(
    do_GetService("@mozilla.org/login-manager/storage/legacy;1"));
  if (!pwmgr)
    return NS_OK;

  GUID mtGuid = {0};
  IEnumPStoreItems* enumItems = NULL;
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

        nsAutoString host(itemName), realm;
        if (Substring(host, 0, 6).EqualsLiteral("DPAPI:")) 
          password = NULL; 

        if (password) {
          int idx;
          idx = host.FindChar('/');
          if (idx) {
            realm.Assign(Substring(host, idx + 1));
            host.Assign(Substring(host, 0, idx));
          }
          
          
          nsresult rv;

          nsCOMPtr<nsILoginInfo> aLogin (do_CreateInstance(
                                           NS_LOGININFO_CONTRACTID, &rv));
          NS_ENSURE_SUCCESS(rv, rv);

          
          
          aLogin->SetHostname(host);
          aLogin->SetHttpRealm(realm);
          aLogin->SetUsername(NS_ConvertUTF8toUTF16((char *)data));
          aLogin->SetPassword(NS_ConvertUTF8toUTF16((char *)password));
          aLogin->SetUsernameField(EmptyString());
          aLogin->SetPasswordField(EmptyString());

          pwmgr->MigrateAndAddLogin(aLogin);
        }
        ::CoTaskMemFree(data);
      }
    }
  }
  return NS_OK;
}

nsresult
nsIEProfileMigrator::GetSignonsListFromPStore(IPStore* aPStore, nsTArray<SignonData>* aSignonsFound)
{
  HRESULT hr;

  NS_ENSURE_ARG_POINTER(aPStore);

  IEnumPStoreItems* enumItems = NULL;
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
              
              
              
              
              SignonData* d = aSignonsFound->AppendElement();
              if (!d)
                return NS_ERROR_OUT_OF_MEMORY;
              d->user = (PRUnichar*)username;
              d->pass = (PRUnichar*)pass;
              d->realm = realm; 
            }
          }
        }
      }
    }
  }
  return NS_OK;
}

bool
nsIEProfileMigrator::KeyIsURI(const nsAString& aKey, char** aRealm)
{
  *aRealm = nsnull;

  nsCOMPtr<nsIURI> uri;

  if (NS_FAILED(NS_NewURI(getter_AddRefs(uri), aKey))) 
    return false;

  bool validScheme = false;
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
  return false;
}

nsresult
nsIEProfileMigrator::ResolveAndMigrateSignons(IPStore* aPStore, nsTArray<SignonData>* aSignonsFound)
{
  HRESULT hr;

  IEnumPStoreItems* enumItems = NULL;
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
    
    
    
    PRUint32 signonCount = aSignonsFound->Length();
    for (PRUint32 i = 0; i < signonCount; ++i) {
      SignonData &sd = aSignonsFound->ElementAt(i);
      ::CoTaskMemFree(sd.user);  
      NS_Free(sd.realm);
    }
    aSignonsFound->Clear();
  }
  return NS_OK;
}

void
nsIEProfileMigrator::EnumerateUsernames(const nsAString& aKey, PRUnichar* aData, unsigned long aCount, nsTArray<SignonData>* aSignonsFound)
{
  nsCOMPtr<nsILoginManagerIEMigrationHelper> pwmgr(
    do_GetService("@mozilla.org/login-manager/storage/legacy;1"));
  if (!pwmgr)
    return;

  PRUnichar* cursor = aData;
  PRUint32 offset = 0;
  PRUint32 signonCount = aSignonsFound->Length();

  while (offset < aCount) {
    nsAutoString curr; curr = cursor;

    
    for (PRUint32 i = 0; i < signonCount; ++i) {
      SignonData &sd = aSignonsFound->ElementAt(i);
      if (curr.Equals(sd.user)) {
        
        nsDependentString usernameStr(sd.user), passStr(sd.pass);
        nsAutoString realm(NS_ConvertUTF8toUTF16(sd.realm));

        nsresult rv;

        nsCOMPtr<nsILoginInfo> aLogin (do_CreateInstance(NS_LOGININFO_CONTRACTID, &rv));
        NS_ENSURE_SUCCESS(rv, );

        
        
        
        
        
        
        aLogin->SetHostname(realm);
        aLogin->SetFormSubmitURL(EmptyString());
        aLogin->SetUsername(usernameStr);
        aLogin->SetPassword(passStr);
        aLogin->SetUsernameField(aKey);
        aLogin->SetPasswordField(EmptyString());

        pwmgr->MigrateAndAddLogin(aLogin);
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
nsIEProfileMigrator::CopyFormData(bool aReplace)
{
  HRESULT hr;

  HMODULE pstoreDLL = ::LoadLibraryW(L"pstorec.dll");
  if (!pstoreDLL) {
    
    
    
    return NS_ERROR_FAILURE;
  }

  PStoreCreateInstancePtr PStoreCreateInstance = (PStoreCreateInstancePtr)::GetProcAddress(pstoreDLL, "PStoreCreateInstance");
  IPStore* PStore = NULL;
  hr = PStoreCreateInstance(&PStore, 0, 0, 0);
  if (FAILED(hr) || PStore == NULL)
    return NS_OK;

  IEnumPStoreItems* enumItems = NULL;
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
  PRUint32 offset = 0;

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
nsIEProfileMigrator::CopyFavorites(bool aReplace)
{
  nsresult rv;
  nsCOMPtr<nsINavBookmarksService> bookmarks =
    do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint8 batchAction = aReplace ? BATCH_ACTION_BOOKMARKS_REPLACE
                                 : BATCH_ACTION_BOOKMARKS;
  nsCOMPtr<nsISupportsPRUint8> supports =
    do_CreateInstance(NS_SUPPORTS_PRUINT8_CONTRACTID);
  NS_ENSURE_TRUE(supports, NS_ERROR_OUT_OF_MEMORY);
  rv = supports->SetData(batchAction);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = bookmarks->RunInBatchMode(this, supports);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsIEProfileMigrator::CopyFavoritesBatched(bool aReplace)
{
  
  
  
  nsresult rv;

  nsCOMPtr<nsINavBookmarksService> bms =
    do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 bookmarksMenuFolderId;
  rv = bms->GetBookmarksMenuFolder(&bookmarksMenuFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString personalToolbarFolderName;
  PRInt64 folder;
  if (!aReplace) {
    nsCOMPtr<nsIStringBundleService> bundleService =
      do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIStringBundle> bundle;
    rv = bundleService->CreateBundle(TRIDENTPROFILE_BUNDLE,
                                     getter_AddRefs(bundle));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString sourceNameIE;
    rv = bundle->GetStringFromName(NS_LITERAL_STRING("sourceNameIE").get(),
                                   getter_Copies(sourceNameIE));
    NS_ENSURE_SUCCESS(rv, rv);

    const PRUnichar* sourceNameStrings[] = { sourceNameIE.get() };
    nsString importedIEFavsTitle;
    rv = bundle->FormatStringFromName(NS_LITERAL_STRING("importedBookmarksFolder").get(),
                                      sourceNameStrings, 1,
                                      getter_Copies(importedIEFavsTitle));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = bms->CreateFolder(bookmarksMenuFolderId,
                           NS_ConvertUTF16toUTF8(importedIEFavsTitle),
                           nsINavBookmarksService::DEFAULT_INDEX,
                           &folder);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    nsCOMPtr<nsIFile> profile;
    GetProfilePath(nsnull, profile);
    rv = InitializeBookmarks(profile);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsCOMPtr<nsIWindowsRegKey> regKey =
      do_CreateInstance("@mozilla.org/windows-registry-key;1");
    if (regKey &&
        NS_SUCCEEDED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                                  REGISTRY_IE_TOOLBAR_KEY,
                                  nsIWindowsRegKey::ACCESS_READ))) {
      nsAutoString linksFolderName;
      if (NS_SUCCEEDED(regKey->ReadStringValue(
                         NS_LITERAL_STRING("LinksFolderName"),
                         linksFolderName)))
        personalToolbarFolderName = linksFolderName;
    }
    folder = bookmarksMenuFolderId;
  }

  nsCOMPtr<nsIProperties> fileLocator =
    do_GetService("@mozilla.org/file/directory_service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFile> favoritesDirectory;
  (void)fileLocator->Get(NS_WIN_FAVORITES_DIR, NS_GET_IID(nsIFile),
                         getter_AddRefs(favoritesDirectory));

  
  
  
  
  if (favoritesDirectory) {
    rv = ParseFavoritesFolder(favoritesDirectory, folder, bms,
                              personalToolbarFolderName, true);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return CopySmartKeywords(bms, bookmarksMenuFolderId);
}

nsresult
nsIEProfileMigrator::CopySmartKeywords(nsINavBookmarksService* aBMS,
                                       PRInt64 aParentFolder)
{ 
  nsresult rv;

  nsCOMPtr<nsIWindowsRegKey> regKey =
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (regKey && 
      NS_SUCCEEDED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                                REGISTRY_IE_SEARCHURL_KEY,
                                nsIWindowsRegKey::ACCESS_READ))) {

    nsCOMPtr<nsIStringBundleService> bundleService =
      do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIStringBundle> bundle;
    rv = bundleService->CreateBundle(TRIDENTPROFILE_BUNDLE,
                                     getter_AddRefs(bundle));
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt64 keywordsFolder = 0;
    int offset = 0;
    while (1) {
      nsAutoString keyName;
      if (NS_FAILED(regKey->GetChildName(offset, keyName)))
        break;

      if (!keywordsFolder) {
        nsString sourceNameIE;
        rv = bundle->GetStringFromName(NS_LITERAL_STRING("sourceNameIE").get(),
                                       getter_Copies(sourceNameIE));
        NS_ENSURE_SUCCESS(rv, rv);

        const PRUnichar* sourceNameStrings[] = { sourceNameIE.get() };
        nsString importedIESearchUrlsTitle;
        rv = bundle->FormatStringFromName(NS_LITERAL_STRING("importedSearchURLsFolder").get(),
                                          sourceNameStrings, 1,
                                          getter_Copies(importedIESearchUrlsTitle));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = aBMS->CreateFolder(aParentFolder,
                                NS_ConvertUTF16toUTF8(importedIESearchUrlsTitle),
                                nsINavBookmarksService::DEFAULT_INDEX,
                                &keywordsFolder);
        NS_ENSURE_SUCCESS(rv, rv);
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
          PRInt64 id;
          rv = aBMS->InsertBookmark(keywordsFolder, uri,
                                    nsINavBookmarksService::DEFAULT_INDEX,
                                    NS_ConvertUTF16toUTF8(keyName),
                                    &id);
          NS_ENSURE_SUCCESS(rv, rv);
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

  IUniformResourceLocatorW* urlLink = nsnull;
  result = ::CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                              IID_IUniformResourceLocatorW, (void**)&urlLink);
  if (SUCCEEDED(result) && urlLink) {
    IPersistFile* urlFile = nsnull;
    result = urlLink->QueryInterface(IID_IPersistFile, (void**)&urlFile);
    if (SUCCEEDED(result) && urlFile) {
      result = urlFile->Load(aFileName.get(), STGM_READ);
      if (SUCCEEDED(result) ) {
        LPWSTR lpTemp = nsnull;
        result = urlLink->GetURL(&lpTemp);
        if (SUCCEEDED(result) && lpTemp) {
          *aOutURL = (char*)ToNewUTF8String(nsDependentString(lpTemp));
          
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
                                          PRInt64 aParentFolder,
                                          nsINavBookmarksService* aBMS,
                                          const nsAString& aPersonalToolbarFolderName,
                                          bool aIsAtRootLevel)
{
  nsresult rv;

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = aDirectory->GetDirectoryEntries(getter_AddRefs(entries));
  NS_ENSURE_SUCCESS(rv, rv);

  do {
    bool hasMore = false;
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

    bool isSymlink = false;
    bool isDir = false;

    currFile->IsSymlink(&isSymlink);
    currFile->IsDirectory(&isDir);

    if (isSymlink) {
      
      
      

      
      nsAutoString path;
      rv = currFile->GetTarget(path);
      if (NS_FAILED(rv)) continue;

      nsCOMPtr<nsILocalFile> localFile;
      rv = NS_NewLocalFile(path, true, getter_AddRefs(localFile));
      if (NS_FAILED(rv)) continue;

      
      
      rv = localFile->IsDirectory(&isDir);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!isDir) continue;

      
      NS_NAMED_LITERAL_STRING(lnkExt, ".lnk");
      PRInt32 lnkExtStart = bookmarkName.Length() - lnkExt.Length();
      if (StringEndsWith(bookmarkName, lnkExt,
                         CaseInsensitiveCompare))
        bookmarkName.SetLength(lnkExtStart);

      nsCOMPtr<nsIURI> bookmarkURI;
      rv = NS_NewFileURI(getter_AddRefs(bookmarkURI), localFile);
      if (NS_FAILED(rv)) continue;
      PRInt64 id;
      rv = aBMS->InsertBookmark(aParentFolder, bookmarkURI,
                                nsINavBookmarksService::DEFAULT_INDEX,
                                NS_ConvertUTF16toUTF8(bookmarkName),
                                &id);
      if (NS_FAILED(rv)) continue;
    }
    else if (isDir) {
      PRInt64 folderId;
      if (bookmarkName.Equals(aPersonalToolbarFolderName)) {
        rv = aBMS->GetToolbarFolder(&folderId);
        if (NS_FAILED(rv)) break;
      }
      else {
        rv = aBMS->CreateFolder(aParentFolder,
                                NS_ConvertUTF16toUTF8(bookmarkName),
                                nsINavBookmarksService::DEFAULT_INDEX,
                                &folderId);
        if (NS_FAILED(rv)) continue;
      }

      rv = ParseFavoritesFolder(currFile, folderId,
                                aBMS, aPersonalToolbarFolderName,
                                false);
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

      nsCOMPtr<nsIURI> resolvedURI;
      rv = NS_NewURI(getter_AddRefs(resolvedURI), resolvedURL);
      if (NS_FAILED(rv)) continue;
      PRInt64 id;
      rv = aBMS->InsertBookmark(aParentFolder, resolvedURI,
                                nsINavBookmarksService::DEFAULT_INDEX,
                                NS_ConvertUTF16toUTF8(name), &id);
      if (NS_FAILED(rv)) continue;
    }
  }
  while (1);

  return rv;
}

nsresult
nsIEProfileMigrator::CopyPreferences(bool aReplace) 
{
  bool            regKeyOpen = false;
  const regEntry  *entry,
                  *endEntry = ArrayEnd(gRegEntries);
                              

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
        regKeyOpen = false;
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
nsIEProfileMigrator::CopyCookies(bool aReplace) 
{
  
  
  nsresult rv = NS_OK;

  nsCOMPtr<nsIFile> cookiesDir;
  nsCOMPtr<nsISimpleEnumerator> cookieFiles;

  nsCOMPtr<nsICookieManager2> cookieManager(do_GetService(NS_COOKIEMANAGER_CONTRACTID));
  if (!cookieManager)
    return NS_ERROR_FAILURE;

  
  NS_GetSpecialDirectory(NS_WIN_COOKIES_DIR, getter_AddRefs(cookiesDir));
  if (!cookiesDir)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIWindowsRegKey> regKey =
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (regKey) {
    NS_NAMED_LITERAL_STRING(regPath,"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System");
    if (NS_SUCCEEDED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE,
                                  regPath,
                                  nsIWindowsRegKey::ACCESS_QUERY_VALUE))) {
      PRUint32 value;
      if (NS_SUCCEEDED(regKey->ReadIntValue(NS_LITERAL_STRING("EnableLUA"),
                                    &value)) &&
          value == 1) {
          nsAutoString dir;
          
          
          
          cookiesDir->GetLeafName(dir);
          if (!dir.EqualsLiteral("Low"))
            cookiesDir->Append(NS_LITERAL_STRING("Low"));
      }
    }
  }

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
    
    bool moreFiles = false;
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

    
    
    bool isIPAddress = false;
    if (hostCopy[0] == '.') {
      aCookieManager->Remove(nsDependentCString(hostCopy+1),
                             stringName, stringPath, false);
      PRNetAddr addr;
      if (PR_StringToNetAddr(hostCopy+1, &addr) == PR_SUCCESS)
        isIPAddress = true;
    }

    nsresult onerv;
    
    onerv = aCookieManager->Add(nsDependentCString(hostCopy + (isIPAddress ? 1 : 0)),
                                stringPath,
                                stringName,
                                nsDependentCString(value),
                                flagsValue & 0x1, 
                                false, 
                                false, 
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
nsIEProfileMigrator::CopyStyleSheet(bool aReplace) 
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
        bool exists;

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
    bool exists;
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
      aPrefs->SetBoolPref("security.enable_ssl3", (value >> 5) & true);
      aPrefs->SetBoolPref("security.enable_tls",  (value >> 7) & true);
    }
  }

  return NS_OK;
}

struct ProxyData {
  char*   prefix;
  PRInt32 prefixLength;
  bool    proxyConfigured;
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

    PRUint32 proxyType = 0;
    
    
    if (NS_SUCCEEDED(regKey->
                     ReadStringValue(NS_LITERAL_STRING("AutoConfigURL"), buf))) {
      
      
      SetUnicharPref("network.proxy.autoconfig_url", buf, aPrefs);
      proxyType = 2;
    }

    
    PRUint32 enabled;
    if (NS_SUCCEEDED(regKey->
                     ReadIntValue(NS_LITERAL_STRING("ProxyEnable"), &enabled))) {
      if (enabled & 0x1)
        proxyType = 1;
    }
    
    aPrefs->SetIntPref("network.proxy.type", proxyType); 
    
    if (NS_SUCCEEDED(regKey->
                     ReadStringValue(NS_LITERAL_STRING("ProxyOverride"), buf)))
      ParseOverrideServers(buf, aPrefs);

    if (NS_SUCCEEDED(regKey->
                     ReadStringValue(NS_LITERAL_STRING("ProxyServer"), buf))) {

      ProxyData data[] = {
        { "ftp=",     4, false, "network.proxy.ftp",
          "network.proxy.ftp_port"    },
        { "http=",    5, false, "network.proxy.http",
          "network.proxy.http_port"   },
        { "https=",   6, false, "network.proxy.ssl",
          "network.proxy.ssl_port"    },
        { "socks=",   6, false, "network.proxy.socks",
          "network.proxy.socks_port"  },
      };

      PRInt32 startIndex = 0, count = 0;
      bool foundSpecificProxy = false;
      for (PRUint32 i = 0; i < ArrayLength(data); ++i) {
        PRInt32 offset = buf.Find(NS_ConvertASCIItoUTF16(data[i].prefix));
        if (offset >= 0) {
          foundSpecificProxy = true;

          data[i].proxyConfigured = true;

          startIndex = offset + data[i].prefixLength;

          PRInt32 terminal = buf.FindChar(';', offset);
          count = terminal > startIndex ? terminal - startIndex : 
                                          buf.Length() - startIndex;

          
          SetProxyPref(Substring(buf, startIndex, count), data[i].hostPref,
                       data[i].portPref, aPrefs);
        }
      }

      if (!foundSpecificProxy) {
        
        
        
        for (PRUint32 i = 0; i < ArrayLength(data); ++i)
          SetProxyPref(buf, data[i].hostPref, data[i].portPref, aPrefs);
        aPrefs->SetBoolPref("network.proxy.share_proxy_settings", true);
      }
    }

  }

  return NS_OK;
}


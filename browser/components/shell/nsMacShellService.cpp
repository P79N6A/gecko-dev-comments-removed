




#include "nsDirectoryServiceDefs.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIImageLoadingContent.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsILocalFileMac.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsIURL.h"
#include "nsIWebBrowserPersist.h"
#include "nsMacShellService.h"
#include "nsNetUtil.h"
#include "nsShellService.h"
#include "nsStringAPI.h"
#include "nsIDocShell.h"
#include "nsILoadContext.h"

#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>

#define NETWORK_PREFPANE NS_LITERAL_CSTRING("/System/Library/PreferencePanes/Network.prefPane")
#define DESKTOP_PREFPANE NS_LITERAL_CSTRING("/System/Library/PreferencePanes/DesktopScreenEffectsPref.prefPane")

#define SAFARI_BUNDLE_IDENTIFIER "com.apple.Safari"

NS_IMPL_ISUPPORTS(nsMacShellService, nsIMacShellService, nsIShellService, nsIWebProgressListener)

NS_IMETHODIMP
nsMacShellService::IsDefaultBrowser(bool aStartupCheck,
                                    bool aForAllTypes,
                                    bool* aIsDefaultBrowser)
{
  *aIsDefaultBrowser = false;

  CFStringRef firefoxID = ::CFBundleGetIdentifier(::CFBundleGetMainBundle());
  if (!firefoxID) {
    
    
    
    return NS_ERROR_FAILURE;
  }

  
  
  CFStringRef defaultBrowserID = ::LSCopyDefaultHandlerForURLScheme(CFSTR("http"));
  if (defaultBrowserID) {
    *aIsDefaultBrowser = ::CFStringCompare(firefoxID, defaultBrowserID, 0) == kCFCompareEqualTo;
    ::CFRelease(defaultBrowserID);
  }

  
  
  
  if (aStartupCheck)
    mCheckedThisSession = true;

  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::SetDefaultBrowser(bool aClaimAllTypes, bool aForAllUsers)
{
  

  CFStringRef firefoxID = ::CFBundleGetIdentifier(::CFBundleGetMainBundle());
  if (!firefoxID) {
    return NS_ERROR_FAILURE;
  }

  if (::LSSetDefaultHandlerForURLScheme(CFSTR("http"), firefoxID) != noErr) {
    return NS_ERROR_FAILURE;
  }
  if (::LSSetDefaultHandlerForURLScheme(CFSTR("https"), firefoxID) != noErr) {
    return NS_ERROR_FAILURE;
  }

  if (aClaimAllTypes) {
    if (::LSSetDefaultHandlerForURLScheme(CFSTR("ftp"), firefoxID) != noErr) {
      return NS_ERROR_FAILURE;
    }
    if (::LSSetDefaultRoleHandlerForContentType(kUTTypeHTML, kLSRolesAll, firefoxID) != noErr) {
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::GetShouldCheckDefaultBrowser(bool* aResult)
{
  
  
  if (mCheckedThisSession) {
    *aResult = false;
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  if (NS_FAILED(rv)) {
    return rv;
  }

  return prefs->GetBoolPref(PREF_CHECKDEFAULTBROWSER, aResult);
}

NS_IMETHODIMP
nsMacShellService::SetShouldCheckDefaultBrowser(bool aShouldCheck)
{
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  if (NS_FAILED(rv)) {
    return rv;
  }

  return prefs->SetBoolPref(PREF_CHECKDEFAULTBROWSER, aShouldCheck);
}

NS_IMETHODIMP
nsMacShellService::GetCanSetDesktopBackground(bool* aResult)
{
  *aResult = true;
  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::SetDesktopBackground(nsIDOMElement* aElement, 
                                        int32_t aPosition)
{
  

  
  nsresult rv;
  nsCOMPtr<nsIImageLoadingContent> imageContent = do_QueryInterface(aElement,
                                                                    &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI> imageURI;
  rv = imageContent->GetCurrentURI(getter_AddRefs(imageURI));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIURI *docURI = content->OwnerDoc()->GetDocumentURI();
  if (!docURI)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIURL> imageURL(do_QueryInterface(imageURI));
  if (!imageURL) {
    
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsAutoCString fileName;
  imageURL->GetFileName(fileName);
  nsCOMPtr<nsIProperties> fileLocator
    (do_GetService("@mozilla.org/file/directory_service;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  fileLocator->Get(NS_OSX_PICTURE_DOCUMENTS_DIR, NS_GET_IID(nsIFile),
                   getter_AddRefs(mBackgroundFile));
  if (!mBackgroundFile)
    return NS_ERROR_OUT_OF_MEMORY;

  nsAutoString fileNameUnicode;
  CopyUTF8toUTF16(fileName, fileNameUnicode);

  
  mBackgroundFile->Append(fileNameUnicode);

  
  nsCOMPtr<nsIWebBrowserPersist> wbp
    (do_CreateInstance("@mozilla.org/embedding/browser/nsWebBrowserPersist;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t flags = nsIWebBrowserPersist::PERSIST_FLAGS_NO_CONVERSION | 
                   nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES |
                   nsIWebBrowserPersist::PERSIST_FLAGS_FROM_CACHE;

  wbp->SetPersistFlags(flags);
  wbp->SetProgressListener(this);

  nsCOMPtr<nsILoadContext> loadContext;
  nsCOMPtr<nsISupports> container = content->OwnerDoc()->GetContainer();
  nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(container);
  if (docShell) {
    loadContext = do_QueryInterface(docShell);
  }

  return wbp->SaveURI(imageURI, nullptr, docURI, nullptr, nullptr,
                      mBackgroundFile, loadContext);
}

NS_IMETHODIMP
nsMacShellService::OnProgressChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    int32_t aCurSelfProgress,
                                    int32_t aMaxSelfProgress,
                                    int32_t aCurTotalProgress,
                                    int32_t aMaxTotalProgress)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::OnLocationChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    nsIURI* aLocation,
                                    uint32_t aFlags)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::OnStatusChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  nsresult aStatus,
                                  const char16_t* aMessage)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::OnSecurityChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    uint32_t aState)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::OnStateChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 uint32_t aStateFlags,
                                 nsresult aStatus)
{
  if (aStateFlags & STATE_STOP) {
    nsCOMPtr<nsIObserverService> os(do_GetService("@mozilla.org/observer-service;1"));
    if (os)
      os->NotifyObservers(nullptr, "shell:desktop-background-changed", nullptr);

    bool exists = false;
    mBackgroundFile->Exists(&exists);
    if (!exists)
      return NS_OK;

    nsAutoCString nativePath;
    mBackgroundFile->GetNativePath(nativePath);

    AEDesc tAEDesc = { typeNull, nil };
    OSErr err = noErr;
    AliasHandle aliasHandle = nil;
    FSRef pictureRef;
    OSStatus status;

    
    status = ::FSPathMakeRef((const UInt8*)nativePath.get(), &pictureRef,
                             nullptr);
    if (status == noErr) {
      err = ::FSNewAlias(nil, &pictureRef, &aliasHandle);
      if (err == noErr && aliasHandle == nil)
        err = paramErr;

      if (err == noErr) {
        
        
        char handleState = ::HGetState((Handle)aliasHandle);
        ::HLock((Handle)aliasHandle);
        err = ::AECreateDesc(typeAlias, *aliasHandle,
                             GetHandleSize((Handle)aliasHandle), &tAEDesc);
        
        ::HSetState((Handle)aliasHandle, handleState);
        ::DisposeHandle((Handle)aliasHandle);
      }
      if (err == noErr) {
        AppleEvent tAppleEvent;
        OSType sig = 'MACS';
        AEBuildError tAEBuildError;
        
        err = ::AEBuildAppleEvent(kAECoreSuite, kAESetData, typeApplSignature,
                                  &sig, sizeof(OSType), kAutoGenerateReturnID,
                                  kAnyTransactionID, &tAppleEvent, &tAEBuildError,
                                  "'----':'obj '{want:type (prop),form:prop" \
                                  ",seld:type('dpic'),from:'null'()},data:(@)",
                                  &tAEDesc);
        if (err == noErr) {
          AppleEvent reply = { typeNull, nil };
          
          err = ::AESend(&tAppleEvent, &reply, kAENoReply, kAENormalPriority,
                         kNoTimeOut, nil, nil);
          ::AEDisposeDesc(&tAppleEvent);
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::OpenApplication(int32_t aApplication)
{
  nsresult rv = NS_OK;
  CFURLRef appURL = nil;
  OSStatus err = noErr;

  switch (aApplication) {
  case nsIShellService::APPLICATION_MAIL:
    {
      CFURLRef tempURL = ::CFURLCreateWithString(kCFAllocatorDefault,
                                                 CFSTR("mailto:"), nullptr);
      err = ::LSGetApplicationForURL(tempURL, kLSRolesAll, nullptr, &appURL);
      ::CFRelease(tempURL);
    }
    break;
  case nsIShellService::APPLICATION_NEWS:
    {
      CFURLRef tempURL = ::CFURLCreateWithString(kCFAllocatorDefault,
                                                 CFSTR("news:"), nullptr);
      err = ::LSGetApplicationForURL(tempURL, kLSRolesAll, nullptr, &appURL);
      ::CFRelease(tempURL);
    }
    break;
  case nsIMacShellService::APPLICATION_KEYCHAIN_ACCESS:
    err = ::LSGetApplicationForInfo('APPL', 'kcmr', nullptr, kLSRolesAll,
                                    nullptr, &appURL);
    break;
  case nsIMacShellService::APPLICATION_NETWORK:
    {
      nsCOMPtr<nsIFile> lf;
      rv = NS_NewNativeLocalFile(NETWORK_PREFPANE, true, getter_AddRefs(lf));
      NS_ENSURE_SUCCESS(rv, rv);
      bool exists;
      lf->Exists(&exists);
      if (!exists)
        return NS_ERROR_FILE_NOT_FOUND;
      return lf->Launch();
    }  
    break;
  case nsIMacShellService::APPLICATION_DESKTOP:
    {
      nsCOMPtr<nsIFile> lf;
      rv = NS_NewNativeLocalFile(DESKTOP_PREFPANE, true, getter_AddRefs(lf));
      NS_ENSURE_SUCCESS(rv, rv);
      bool exists;
      lf->Exists(&exists);
      if (!exists)
        return NS_ERROR_FILE_NOT_FOUND;
      return lf->Launch();
    }  
    break;
  }

  if (appURL && err == noErr) {
    err = ::LSOpenCFURLRef(appURL, nullptr);
    rv = err != noErr ? NS_ERROR_FAILURE : NS_OK;

    ::CFRelease(appURL);
  }

  return rv;
}

NS_IMETHODIMP
nsMacShellService::GetDesktopBackgroundColor(uint32_t *aColor)
{
  
  
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMacShellService::SetDesktopBackgroundColor(uint32_t aColor)
{
  
  
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMacShellService::OpenApplicationWithURI(nsIFile* aApplication, const nsACString& aURI)
{
  nsCOMPtr<nsILocalFileMac> lfm(do_QueryInterface(aApplication));
  CFURLRef appURL;
  nsresult rv = lfm->GetCFURL(&appURL);
  if (NS_FAILED(rv))
    return rv;
  
  const nsCString spec(aURI);
  const UInt8* uriString = (const UInt8*)spec.get();
  CFURLRef uri = ::CFURLCreateWithBytes(nullptr, uriString, aURI.Length(),
                                        kCFStringEncodingUTF8, nullptr);
  if (!uri) 
    return NS_ERROR_OUT_OF_MEMORY;
  
  CFArrayRef uris = ::CFArrayCreate(nullptr, (const void**)&uri, 1, nullptr);
  if (!uris) {
    ::CFRelease(uri);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  LSLaunchURLSpec launchSpec;
  launchSpec.appURL = appURL;
  launchSpec.itemURLs = uris;
  launchSpec.passThruParams = nullptr;
  launchSpec.launchFlags = kLSLaunchDefaults;
  launchSpec.asyncRefCon = nullptr;
  
  OSErr err = ::LSOpenFromURLSpec(&launchSpec, nullptr);
  
  ::CFRelease(uris);
  ::CFRelease(uri);
  
  return err != noErr ? NS_ERROR_FAILURE : NS_OK;
}

NS_IMETHODIMP
nsMacShellService::GetDefaultFeedReader(nsIFile** _retval)
{
  nsresult rv = NS_ERROR_FAILURE;
  *_retval = nullptr;

  CFStringRef defaultHandlerID = ::LSCopyDefaultHandlerForURLScheme(CFSTR("feed"));
  if (!defaultHandlerID) {
    defaultHandlerID = ::CFStringCreateWithCString(kCFAllocatorDefault,
                                                   SAFARI_BUNDLE_IDENTIFIER,
                                                   kCFStringEncodingASCII);
  }

  CFURLRef defaultHandlerURL = nullptr;
  OSStatus status = ::LSFindApplicationForInfo(kLSUnknownCreator,
                                               defaultHandlerID,
                                               nullptr, 
                                               nullptr, 
                                               &defaultHandlerURL);

  if (status == noErr && defaultHandlerURL) {
    nsCOMPtr<nsILocalFileMac> defaultReader =
      do_CreateInstance("@mozilla.org/file/local;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = defaultReader->InitWithCFURL(defaultHandlerURL);
      if (NS_SUCCEEDED(rv)) {
        NS_ADDREF(*_retval = defaultReader);
        rv = NS_OK;
      }
    }

    ::CFRelease(defaultHandlerURL);
  }

  ::CFRelease(defaultHandlerID);

  return rv;
}

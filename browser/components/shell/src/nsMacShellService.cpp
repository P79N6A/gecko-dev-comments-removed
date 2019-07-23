






































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

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>

#define NETWORK_PREFPANE NS_LITERAL_CSTRING("/System/Library/PreferencePanes/Network.prefPane")
#define DESKTOP_PREFPANE NS_LITERAL_CSTRING("/System/Library/PreferencePanes/DesktopScreenEffectsPref.prefPane")

#define SAFARI_BUNDLE_IDENTIFIER NS_LITERAL_CSTRING("com.apple.Safari")



extern "C" {
  
  
  extern OSStatus _LSCopyDefaultSchemeHandlerURL(CFStringRef scheme, CFURLRef *appURL);
  extern OSStatus _LSSetDefaultSchemeHandlerURL(CFStringRef scheme, CFURLRef appURL);
  extern OSStatus _LSSaveAndRefresh(void);
  
  
  extern OSStatus _LSSetWeakBindingForType(OSType inType,
                                           OSType inCreator,
                                           CFStringRef inExtension,
                                           LSRolesMask inRoleMask,
                                           const FSRef* inBindingRef);
}

NS_IMPL_ISUPPORTS3(nsMacShellService, nsIMacShellService, nsIShellService, nsIWebProgressListener)

NS_IMETHODIMP
nsMacShellService::IsDefaultBrowser(PRBool aStartupCheck, PRBool* aIsDefaultBrowser)
{
  *aIsDefaultBrowser = PR_TRUE;

  
  
  
  
  

  CFStringRef firefoxID = ::CFBundleGetIdentifier(CFBundleGetMainBundle());
  if (!firefoxID) {
    
    
    
    return NS_ERROR_FAILURE;
  }

  ::CFRetain(firefoxID);

  
  CFURLRef defaultBrowserURL;
  OSStatus err = ::_LSCopyDefaultSchemeHandlerURL(CFSTR("http"),
                                                  &defaultBrowserURL);

  nsresult rv = NS_ERROR_FAILURE;
  if (err == noErr) {
    
    CFBundleRef defaultBrowserBundle = ::CFBundleCreate(NULL, 
                                                        defaultBrowserURL);
    if (defaultBrowserBundle) {
      CFStringRef defaultBrowserID = ::CFBundleGetIdentifier(defaultBrowserBundle);
      if (defaultBrowserID) {
        ::CFRetain(defaultBrowserID);
        
        *aIsDefaultBrowser = ::CFStringCompare(firefoxID, defaultBrowserID, 0)
                             == kCFCompareEqualTo;
        ::CFRelease(defaultBrowserID);
      }
      else {
        
        
        *aIsDefaultBrowser = PR_FALSE;
      }

      ::CFRelease(defaultBrowserBundle);
      rv = NS_OK;
    }

    ::CFRelease(defaultBrowserURL);
  }

  
  ::CFRelease(firefoxID);

  
  
  
  if (aStartupCheck)
    mCheckedThisSession = PR_TRUE;

  return rv;
}

NS_IMETHODIMP
nsMacShellService::SetDefaultBrowser(PRBool aClaimAllTypes, PRBool aForAllUsers)
{
  

  CFURLRef firefoxURL = ::CFBundleCopyBundleURL(CFBundleGetMainBundle());

  ::_LSSetDefaultSchemeHandlerURL(CFSTR("http"), firefoxURL);
  ::_LSSetDefaultSchemeHandlerURL(CFSTR("https"), firefoxURL);

  if (aClaimAllTypes) {
    ::_LSSetDefaultSchemeHandlerURL(CFSTR("ftp"), firefoxURL);

    FSRef firefoxFSRef;
    
    if (::CFURLGetFSRef(firefoxURL, &firefoxFSRef)); {
      
      ::_LSSetWeakBindingForType(0, 0, CFSTR("html"), kLSRolesAll, &firefoxFSRef);
      ::_LSSetWeakBindingForType(0, 0, CFSTR("htm"), kLSRolesAll, &firefoxFSRef);
    }
  }
  ::_LSSaveAndRefresh();

  ::CFRelease(firefoxURL);
  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::GetShouldCheckDefaultBrowser(PRBool* aResult)
{
  
  
  if (mCheckedThisSession) {
    *aResult = PR_FALSE;
    return NS_OK;
  }

  nsCOMPtr<nsIPrefBranch> prefs;
  nsCOMPtr<nsIPrefService> pserve(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (pserve)
    pserve->GetBranch("", getter_AddRefs(prefs));

  prefs->GetBoolPref(PREF_CHECKDEFAULTBROWSER, aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::SetShouldCheckDefaultBrowser(PRBool aShouldCheck)
{
  nsCOMPtr<nsIPrefBranch> prefs;
  nsCOMPtr<nsIPrefService> pserve(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (pserve)
    pserve->GetBranch("", getter_AddRefs(prefs));

  prefs->SetBoolPref(PREF_CHECKDEFAULTBROWSER, aShouldCheck);

  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::SetDesktopBackground(nsIDOMElement* aElement, 
                                        PRInt32 aPosition)
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
  nsCOMPtr<nsIDocument> doc;
  doc = content->GetOwnerDoc();
  if (!doc)
    return NS_ERROR_FAILURE;

  nsIURI *docURI = doc->GetDocumentURI();
  if (!docURI)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIURL> imageURL(do_QueryInterface(imageURI));
  if (!imageURL) {
    
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsCAutoString fileName;
  imageURL->GetFileName(fileName);
  nsCOMPtr<nsIProperties> fileLocator
    (do_GetService("@mozilla.org/file/directory_service;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  fileLocator->Get(NS_OSX_PICTURE_DOCUMENTS_DIR, NS_GET_IID(nsILocalFile),
                   getter_AddRefs(mBackgroundFile));
  if (!mBackgroundFile)
    return NS_ERROR_OUT_OF_MEMORY;

  nsAutoString fileNameUnicode;
  CopyUTF8toUTF16(fileName, fileNameUnicode);

  
  mBackgroundFile->Append(fileNameUnicode);

  
  nsCOMPtr<nsIWebBrowserPersist> wbp
    (do_CreateInstance("@mozilla.org/embedding/browser/nsWebBrowserPersist;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 flags = nsIWebBrowserPersist::PERSIST_FLAGS_NO_CONVERSION | 
                   nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES |
                   nsIWebBrowserPersist::PERSIST_FLAGS_FROM_CACHE;

  wbp->SetPersistFlags(flags);
  wbp->SetProgressListener(this);

  return wbp->SaveURI(imageURI, nsnull, docURI, nsnull, nsnull,
                      mBackgroundFile);
}

NS_IMETHODIMP
nsMacShellService::OnProgressChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    PRInt32 aCurSelfProgress,
                                    PRInt32 aMaxSelfProgress,
                                    PRInt32 aCurTotalProgress,
                                    PRInt32 aMaxTotalProgress)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::OnLocationChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    nsIURI* aLocation)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::OnStatusChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  nsresult aStatus,
                                  const PRUnichar* aMessage)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::OnSecurityChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    PRUint32 aState)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMacShellService::OnStateChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 PRUint32 aStateFlags,
                                 nsresult aStatus)
{
  if (aStateFlags & STATE_STOP) {
    nsCOMPtr<nsIObserverService> os(do_GetService("@mozilla.org/observer-service;1"));
    if (os)
      os->NotifyObservers(nsnull, "shell:desktop-background-changed", nsnull);

    PRBool exists = PR_FALSE;
    mBackgroundFile->Exists(&exists);
    if (!exists)
      return NS_OK;

    nsCAutoString nativePath;
    mBackgroundFile->GetNativePath(nativePath);

    AEDesc tAEDesc = { typeNull, nil };
    OSErr err = noErr;
    AliasHandle aliasHandle = nil;
    FSRef pictureRef;
    OSStatus status;

    
    status = ::FSPathMakeRef((const UInt8*)nativePath.get(), &pictureRef, NULL);
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
nsMacShellService::OpenApplication(PRInt32 aApplication)
{
  nsresult rv = NS_OK;
  CFURLRef appURL = nil;
  OSStatus err = noErr;

  switch (aApplication) {
  case nsIShellService::APPLICATION_MAIL:
    {
      CFURLRef tempURL = ::CFURLCreateWithString(kCFAllocatorDefault,
                                                 CFSTR("mailto:"), NULL);
      err = ::LSGetApplicationForURL(tempURL, kLSRolesAll, NULL, &appURL);
      ::CFRelease(tempURL);
    }
    break;
  case nsIShellService::APPLICATION_NEWS:
    {
      CFURLRef tempURL = ::CFURLCreateWithString(kCFAllocatorDefault,
                                                 CFSTR("news:"), NULL);
      err = ::LSGetApplicationForURL(tempURL, kLSRolesAll, NULL, &appURL);
      ::CFRelease(tempURL);
    }
    break;
  case nsIMacShellService::APPLICATION_KEYCHAIN_ACCESS:
    err = ::LSGetApplicationForInfo('APPL', 'kcmr', NULL, kLSRolesAll, NULL,
                                    &appURL);
    break;
  case nsIMacShellService::APPLICATION_NETWORK:
    {
      nsCOMPtr<nsILocalFile> lf;
      rv = NS_NewNativeLocalFile(NETWORK_PREFPANE, PR_TRUE, getter_AddRefs(lf));
      NS_ENSURE_SUCCESS(rv, rv);
      PRBool exists;
      lf->Exists(&exists);
      if (!exists)
        return NS_ERROR_FILE_NOT_FOUND;
      return lf->Launch();
    }  
    break;
  case nsIMacShellService::APPLICATION_DESKTOP:
    {
      nsCOMPtr<nsILocalFile> lf;
      rv = NS_NewNativeLocalFile(DESKTOP_PREFPANE, PR_TRUE, getter_AddRefs(lf));
      NS_ENSURE_SUCCESS(rv, rv);
      PRBool exists;
      lf->Exists(&exists);
      if (!exists)
        return NS_ERROR_FILE_NOT_FOUND;
      return lf->Launch();
    }  
    break;
  }

  if (appURL && err == noErr) {
    err = ::LSOpenCFURLRef(appURL, NULL);
    rv = err != noErr ? NS_ERROR_FAILURE : NS_OK;

    ::CFRelease(appURL);
  }

  return rv;
}

NS_IMETHODIMP
nsMacShellService::GetDesktopBackgroundColor(PRUint32 *aColor)
{
  
  
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMacShellService::SetDesktopBackgroundColor(PRUint32 aColor)
{
  
  
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMacShellService::OpenApplicationWithURI(nsILocalFile* aApplication, const nsACString& aURI)
{
  nsCOMPtr<nsILocalFileMac> lfm(do_QueryInterface(aApplication));
  CFURLRef appURL;
  nsresult rv = lfm->GetCFURL(&appURL);
  if (NS_FAILED(rv))
    return rv;
  
  const nsCString spec(aURI);
  const UInt8* uriString = (const UInt8*)spec.get();
  CFURLRef uri = ::CFURLCreateWithBytes(NULL, uriString, aURI.Length(),
                                        kCFStringEncodingUTF8, NULL);
  if (!uri) 
    return NS_ERROR_OUT_OF_MEMORY;
  
  CFArrayRef uris = ::CFArrayCreate(NULL, (const void**)&uri, 1, NULL);
  if (!uris) {
    ::CFRelease(uri);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  LSLaunchURLSpec launchSpec;
  launchSpec.appURL = appURL;
  launchSpec.itemURLs = uris;
  launchSpec.passThruParams = NULL;
  launchSpec.launchFlags = kLSLaunchDefaults;
  launchSpec.asyncRefCon = NULL;
  
  OSErr err = ::LSOpenFromURLSpec(&launchSpec, NULL);
  
  ::CFRelease(uris);
  ::CFRelease(uri);
  
  return err != noErr ? NS_ERROR_FAILURE : NS_OK;
}

NS_IMETHODIMP
nsMacShellService::GetDefaultFeedReader(nsILocalFile** _retval)
{
  nsresult rv = NS_ERROR_FAILURE;
  *_retval = nsnull;

  CFURLRef defaultHandlerURL;
  OSStatus err = ::_LSCopyDefaultSchemeHandlerURL(CFSTR("feed"),
                                                  &defaultHandlerURL);
  if (defaultHandlerURL) {
    nsCOMPtr<nsILocalFileMac> defaultReader =
      do_CreateInstance("@mozilla.org/file/local;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = defaultReader->InitWithCFURL(defaultHandlerURL);
      if (NS_SUCCEEDED(rv)) {
        
        nsCAutoString bundleIdentifier;

        
        rv = NS_ERROR_FAILURE;
        if (NS_FAILED(defaultReader->GetBundleIdentifier(bundleIdentifier)) ||
            !bundleIdentifier.Equals(SAFARI_BUNDLE_IDENTIFIER)) {
          NS_ADDREF(*_retval = defaultReader);
          rv = NS_OK;
        }
      }
    }

    ::CFRelease(defaultHandlerURL);
  }

  return rv;
}

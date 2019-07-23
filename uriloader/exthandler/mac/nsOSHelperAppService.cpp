






































#include "nsOSHelperAppService.h"
#include "nsISupports.h"
#include "nsString.h"
#include "nsAutoBuffer.h"
#include "nsXPIDLString.h"
#include "nsIURL.h"
#include "nsILocalFile.h"
#include "nsILocalFileMac.h"
#include "nsMimeTypes.h"
#include "nsIStringBundle.h"
#include "nsIPromptService.h"
#include "nsMemory.h"
#include "nsCRT.h"
#include "nsMIMEInfoMac.h"

#include "nsIInternetConfigService.h"
#include "nsEmbedCID.h"
#include <LaunchServices.h>


#define HELPERAPPLAUNCHER_BUNDLE_URL "chrome://global/locale/helperAppLauncher.properties"
#define BRAND_BUNDLE_URL "chrome://branding/locale/brand.properties"

extern "C" {
  
  
  extern OSStatus _LSCopyDefaultSchemeHandlerURL(CFStringRef scheme,
                                                 CFURLRef *appURL);
}

nsOSHelperAppService::nsOSHelperAppService() : nsExternalHelperAppService()
{
}

nsOSHelperAppService::~nsOSHelperAppService()
{}

nsresult nsOSHelperAppService::OSProtocolHandlerExists(const char * aProtocolScheme, PRBool * aHandlerExists)
{
  
  *aHandlerExists = PR_FALSE;
  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIInternetConfigService> icService (do_GetService(NS_INTERNETCONFIGSERVICE_CONTRACTID));
  if (icService)
  {
    rv = icService->HasProtocolHandler(aProtocolScheme, aHandlerExists);
    if (rv == NS_ERROR_NOT_AVAILABLE)
    {
      
      
      
      *aHandlerExists = PR_FALSE;
      rv = NS_OK;

      
      
      
      
    }
  }
  return rv;
}

NS_IMETHODIMP nsOSHelperAppService::GetApplicationDescription(const nsACString& aScheme, nsAString& _retval)
{
  nsresult rv = NS_ERROR_NOT_AVAILABLE;

  CFStringRef schemeCFString = 
    ::CFStringCreateWithBytes(kCFAllocatorDefault,
                              (const UInt8 *)PromiseFlatCString(aScheme).get(),
                              aScheme.Length(),
                              kCFStringEncodingUTF8,
                              false);
  if (schemeCFString) {
    
    
    CFURLRef handlerBundleURL;
    OSStatus err = ::_LSCopyDefaultSchemeHandlerURL(schemeCFString,
                                                    &handlerBundleURL);
    if (err == noErr) {
      CFBundleRef handlerBundle = ::CFBundleCreate(NULL, handlerBundleURL);
      if (handlerBundle) {
        
        CFStringRef bundleName =
          (CFStringRef)::CFBundleGetValueForInfoDictionaryKey(handlerBundle,
                                                              kCFBundleNameKey);
        if (bundleName) {
          nsAutoBuffer<UniChar, 255> buffer;
          CFIndex bundleNameLength = ::CFStringGetLength(bundleName);
          buffer.EnsureElemCapacity(bundleNameLength);
          ::CFStringGetCharacters(bundleName, CFRangeMake(0, bundleNameLength),
                                  buffer.get());
          _retval.Assign(buffer.get(), bundleNameLength);
          rv = NS_OK;
        }

        ::CFRelease(handlerBundle);
      }

      ::CFRelease(handlerBundleURL);
    }

    ::CFRelease(schemeCFString);
  }

  return rv;
}

nsresult nsOSHelperAppService::GetFileTokenForPath(const PRUnichar * aPlatformAppPath, nsIFile ** aFile)
{
  nsresult rv;
  nsCOMPtr<nsILocalFileMac> localFile (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv,rv);

  CFURLRef pathAsCFURL;
  CFStringRef pathAsCFString = ::CFStringCreateWithCharacters(NULL,
                                                              aPlatformAppPath,
                                                              nsCRT::strlen(aPlatformAppPath));
  if (!pathAsCFString)
    return NS_ERROR_OUT_OF_MEMORY;

  if (::CFStringGetCharacterAtIndex(pathAsCFString, 0) == '/') {
    
    pathAsCFURL = ::CFURLCreateWithFileSystemPath(nsnull, pathAsCFString,
                                                  kCFURLPOSIXPathStyle, PR_FALSE);
    if (!pathAsCFURL) {
      ::CFRelease(pathAsCFString);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  else {
    
    

    
    
    if (::CFStringGetLength(pathAsCFString) == 0 ||
        ::CFStringGetCharacterAtIndex(pathAsCFString, 0) == ':')
    {
      ::CFRelease(pathAsCFString);
      return NS_ERROR_FILE_UNRECOGNIZED_PATH;
    }

    pathAsCFURL = ::CFURLCreateWithFileSystemPath(nsnull, pathAsCFString,
                                                  kCFURLHFSPathStyle, PR_FALSE);
    if (!pathAsCFURL) {
      ::CFRelease(pathAsCFString);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  rv = localFile->InitWithCFURL(pathAsCFURL);
  ::CFRelease(pathAsCFString);
  ::CFRelease(pathAsCFURL);
  if (NS_FAILED(rv))
    return rv;
  *aFile = localFile;
  NS_IF_ADDREF(*aFile);

  return NS_OK;
}




NS_IMETHODIMP nsOSHelperAppService::GetFromTypeAndExtension(const nsACString& aType, const nsACString& aFileExt, nsIMIMEInfo ** aMIMEInfo)
{
  
  nsresult rv = nsExternalHelperAppService::GetFromTypeAndExtension(aType, aFileExt, aMIMEInfo);
  if (NS_SUCCEEDED(rv) && *aMIMEInfo) 
  {
    UpdateCreatorInfo(*aMIMEInfo);
  }
  return rv;
}

already_AddRefed<nsIMIMEInfo>
nsOSHelperAppService::GetMIMEInfoFromOS(const nsACString& aMIMEType,
                                        const nsACString& aFileExt,
                                        PRBool * aFound)
{
  nsIMIMEInfo* mimeInfo = nsnull;
  *aFound = PR_TRUE;

  const nsCString& flatType = PromiseFlatCString(aMIMEType);
  const nsCString& flatExt = PromiseFlatCString(aFileExt);

  
  nsCOMPtr<nsIInternetConfigService> icService (do_GetService(NS_INTERNETCONFIGSERVICE_CONTRACTID));
  PR_LOG(mLog, PR_LOG_DEBUG, ("Mac: HelperAppService lookup for type '%s' ext '%s' (IC: 0x%p)\n",
                              flatType.get(), flatExt.get(), icService.get()));
  if (icService)
  {
    nsCOMPtr<nsIMIMEInfo> miByType, miByExt;
    if (!aMIMEType.IsEmpty())
      icService->FillInMIMEInfo(flatType.get(), flatExt.get(), getter_AddRefs(miByType));

    PRBool hasDefault = PR_FALSE;
    if (miByType)
      miByType->GetHasDefaultHandler(&hasDefault);

    if (!aFileExt.IsEmpty() && (!hasDefault || !miByType)) {
      icService->GetMIMEInfoFromExtension(flatExt.get(), getter_AddRefs(miByExt));
      if (miByExt && !aMIMEType.IsEmpty()) {
        
        nsIMIMEInfo* pByExt = miByExt.get();
        nsMIMEInfoBase* byExt = static_cast<nsMIMEInfoBase*>(pByExt);
        byExt->SetMIMEType(aMIMEType);
      }
    }
    PR_LOG(mLog, PR_LOG_DEBUG, ("OS gave us: By Type: 0x%p By Ext: 0x%p type has default: %s\n",
                                miByType.get(), miByExt.get(), hasDefault ? "true" : "false"));

    
    if (!hasDefault && miByType && miByExt) {
      
      
      
      
      
      
      nsIMIMEInfo* pByType = miByType.get();
      nsIMIMEInfo* pByExt = miByExt.get();
      nsMIMEInfoBase* byType = static_cast<nsMIMEInfoBase*>(pByType);
      nsMIMEInfoBase* byExt = static_cast<nsMIMEInfoBase*>(pByExt);
      if (!byType || !byExt) {
        NS_ERROR("IC gave us an nsIMIMEInfo that's no nsMIMEInfoBase! Fix nsOSHelperAppService.");
        return nsnull;
      }
      
      byType->CopyBasicDataTo(byExt);
      miByType = miByExt;
    }
    if (miByType)
      miByType.swap(mimeInfo);
    else if (miByExt)
      miByExt.swap(mimeInfo);
  }

  if (!mimeInfo) {
    *aFound = PR_FALSE;
    PR_LOG(mLog, PR_LOG_DEBUG, ("Creating new mimeinfo\n"));
    mimeInfo = new nsMIMEInfoMac(aMIMEType);
    if (!mimeInfo)
      return nsnull;
    NS_ADDREF(mimeInfo);

    if (!aFileExt.IsEmpty())
      mimeInfo->AppendExtension(aFileExt);
  }
  
  return mimeInfo;
}

already_AddRefed<nsIHandlerInfo>
nsOSHelperAppService::GetProtocolInfoFromOS(const nsACString &aScheme,
                                            PRBool *found)
{
  NS_ASSERTION(!aScheme.IsEmpty(), "No scheme was specified!");

  nsresult rv = OSProtocolHandlerExists(nsPromiseFlatCString(aScheme).get(),
                                        found);
  if (NS_FAILED(rv))
    return nsnull;

  nsMIMEInfoMac *handlerInfo =
    new nsMIMEInfoMac(aScheme, nsMIMEInfoBase::eProtocolInfo);
  NS_ENSURE_TRUE(handlerInfo, nsnull);
  NS_ADDREF(handlerInfo);

  if (!*found) {
    
    
    return handlerInfo;
  }

  nsAutoString desc;
  GetApplicationDescription(aScheme, desc);
  handlerInfo->SetDefaultDescription(desc);

  return handlerInfo;
}



void nsOSHelperAppService::UpdateCreatorInfo(nsIMIMEInfo * aMIMEInfo)
{
  PRUint32 macCreatorType;
  PRUint32 macFileType;
  aMIMEInfo->GetMacType(&macFileType);
  aMIMEInfo->GetMacCreator(&macCreatorType);
  
  if (macFileType == 0 || macCreatorType == 0)
  {
    
    nsCAutoString mimeType;
    aMIMEInfo->GetMIMEType(mimeType);
    nsCOMPtr<nsIInternetConfigService> icService (do_GetService(NS_INTERNETCONFIGSERVICE_CONTRACTID));
    if (icService)
    {
      nsCOMPtr<nsIMIMEInfo> osMimeObject;
      icService->FillInMIMEInfo(mimeType.get(), nsnull, getter_AddRefs(osMimeObject));
      if (osMimeObject)
      {
        osMimeObject->GetMacType(&macFileType);
        osMimeObject->GetMacCreator(&macCreatorType);
        aMIMEInfo->SetMacCreator(macCreatorType);
        aMIMEInfo->SetMacType(macFileType);
      } 
    } 
  } 
} 

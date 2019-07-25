









































#include "nsOSHelperAppService.h"
#include "nsISupports.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIURL.h"
#include "nsIMIMEInfo.h"
#include "nsMIMEInfoWin.h"
#include "nsMimeTypes.h"
#include "nsILocalFileWin.h"
#include "nsIProcess.h"
#include "plstr.h"
#include "nsAutoPtr.h"
#include "nsNativeCharsetUtils.h"
#include "nsIWindowsRegKey.h"


#include <shellapi.h>

#define LOG(args) PR_LOG(mLog, PR_LOG_DEBUG, args)


static nsresult GetExtensionFrom4xRegistryInfo(const nsACString& aMimeType, 
                                               nsString& aFileExtension);
static nsresult GetExtensionFromWindowsMimeDatabase(const nsACString& aMimeType,
                                                    nsString& aFileExtension);

nsOSHelperAppService::nsOSHelperAppService() : 
  nsExternalHelperAppService()
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  , mAppAssoc(nsnull)
#endif
{
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  CoInitialize(NULL);
  CoCreateInstance(CLSID_ApplicationAssociationRegistration, NULL, CLSCTX_INPROC,
                   IID_IApplicationAssociationRegistration, (void**)&mAppAssoc);
#endif
}

nsOSHelperAppService::~nsOSHelperAppService()
{
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  if (mAppAssoc)
    mAppAssoc->Release();
  mAppAssoc = nsnull;
  CoUninitialize();
#endif
}



static nsresult GetExtensionFromWindowsMimeDatabase(const nsACString& aMimeType,
                                                    nsString& aFileExtension)
{
  nsAutoString mimeDatabaseKey;
  mimeDatabaseKey.AssignLiteral("MIME\\Database\\Content Type\\");

  AppendASCIItoUTF16(aMimeType, mimeDatabaseKey);

  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!regKey) 
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                             mimeDatabaseKey,
                             nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  
  if (NS_SUCCEEDED(rv))
     regKey->ReadStringValue(NS_LITERAL_STRING("Extension"), aFileExtension);

  return NS_OK;
}





static nsresult GetExtensionFrom4xRegistryInfo(const nsACString& aMimeType,
                                               nsString& aFileExtension)
{
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!regKey) 
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv = regKey->
    Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
         NS_LITERAL_STRING("Software\\Netscape\\Netscape Navigator\\Suffixes"),
         nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv))
    return NS_ERROR_NOT_AVAILABLE;
   
  rv = regKey->ReadStringValue(NS_ConvertASCIItoUTF16(aMimeType),
                               aFileExtension);
  if (NS_FAILED(rv))
    return NS_OK;

  aFileExtension.Insert(PRUnichar('.'), 0);
      
  
  

  PRInt32 pos = aFileExtension.FindChar(PRUnichar(','));
  if (pos > 0) {
    
    
    aFileExtension.Truncate(pos); 
  }
   
  return NS_OK;
}

nsresult nsOSHelperAppService::OSProtocolHandlerExists(const char * aProtocolScheme, bool * aHandlerExists)
{
  
  *aHandlerExists = PR_FALSE;
  if (aProtocolScheme && *aProtocolScheme)
  {
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
    
    if (mAppAssoc) {
      PRUnichar * pResult = nsnull;
      NS_ConvertASCIItoUTF16 scheme(aProtocolScheme);
      
      HRESULT hr = mAppAssoc->QueryCurrentDefault(scheme.get(),
                                                  AT_URLPROTOCOL, AL_EFFECTIVE,
                                                  &pResult);
      if (SUCCEEDED(hr)) {
        CoTaskMemFree(pResult);
        *aHandlerExists = PR_TRUE;
      }
      return NS_OK;
    }
#endif

    HKEY hKey;
    LONG err = ::RegOpenKeyExW(HKEY_CLASSES_ROOT,
                               NS_ConvertASCIItoUTF16(aProtocolScheme).get(),
                               0,
                               KEY_QUERY_VALUE,
                               &hKey);
    if (err == ERROR_SUCCESS)
    {
      err = ::RegQueryValueExW(hKey, L"URL Protocol", NULL, NULL, NULL, NULL);
      *aHandlerExists = (err == ERROR_SUCCESS);
      
      ::RegCloseKey(hKey);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsOSHelperAppService::GetApplicationDescription(const nsACString& aScheme, nsAString& _retval)
{
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!regKey) 
    return NS_ERROR_NOT_AVAILABLE;

  NS_ConvertASCIItoUTF16 buf(aScheme);

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  
  if (mAppAssoc) {
    PRUnichar * pResult = nsnull;
    
    HRESULT hr = mAppAssoc->QueryCurrentDefault(buf.get(),
                                                AT_URLPROTOCOL, AL_EFFECTIVE,
                                                &pResult);
    if (SUCCEEDED(hr)) {
      nsCOMPtr<nsIFile> app;
      nsAutoString appInfo(pResult);
      CoTaskMemFree(pResult);
      if (NS_SUCCEEDED(GetDefaultAppInfo(appInfo, _retval, getter_AddRefs(app))))
        return NS_OK;
    }
    return NS_ERROR_NOT_AVAILABLE;
  }
#endif

  nsCOMPtr<nsIFile> app;
  GetDefaultAppInfo(buf, _retval, getter_AddRefs(app));

  if (!_retval.Equals(buf))
    return NS_OK;

  
  buf.AppendLiteral("\\shell\\open\\command");
  nsresult rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                             buf,
                             nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv))
    return NS_ERROR_NOT_AVAILABLE;   
   
  rv = regKey->ReadStringValue(EmptyString(), _retval); 

  return NS_SUCCEEDED(rv) ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}










nsresult nsOSHelperAppService::GetMIMEInfoFromRegistry(const nsAFlatString& fileType, nsIMIMEInfo *pInfo)
{
  nsresult rv = NS_OK;

  NS_ENSURE_ARG(pInfo);
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!regKey) 
    return NS_ERROR_NOT_AVAILABLE;

  rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                    fileType, nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;
 
  
  nsAutoString description;
  rv = regKey->ReadStringValue(EmptyString(), description);
  if (NS_SUCCEEDED(rv))
    pInfo->SetDescription(description);

  return NS_OK;
}







 bool
nsOSHelperAppService::typeFromExtEquals(const PRUnichar* aExt, const char *aType)
{
  if (!aType)
    return PR_FALSE;
  nsAutoString fileExtToUse;
  if (aExt[0] != PRUnichar('.'))
    fileExtToUse = PRUnichar('.');

  fileExtToUse.Append(aExt);

  bool eq = false;
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!regKey) 
    return eq;

  nsresult rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                             fileExtToUse,
                             nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv))
      return eq;
   
  nsAutoString type;
  rv = regKey->ReadStringValue(NS_LITERAL_STRING("Content Type"), type);
  if (NS_SUCCEEDED(rv))
     eq = type.EqualsASCII(aType);

  return eq;
}


static void CleanupHandlerPath(nsString& aPath)
{
  

  
  
  
  

  PRInt32 lastCommaPos = aPath.RFindChar(',');
  if (lastCommaPos != kNotFound)
    aPath.Truncate(lastCommaPos);

  aPath.AppendLiteral(" ");

  
  PRUint32 index = aPath.Find(".exe ", PR_TRUE);
  if (index == kNotFound)
    index = aPath.Find(".dll ", PR_TRUE);
  if (index == kNotFound)
    index = aPath.Find(".cpl ", PR_TRUE);

  if (index != kNotFound)
    aPath.Truncate(index + 4);
  aPath.Trim(" ", PR_TRUE, PR_TRUE);
}



static void StripRundll32(nsString& aCommandString)
{
  
  
  
  
  

  NS_NAMED_LITERAL_STRING(rundllSegment, "rundll32.exe ");
  NS_NAMED_LITERAL_STRING(rundllSegmentShort, "rundll32 ");

  
  PRInt32 strLen = rundllSegment.Length();
  PRInt32 index = aCommandString.Find(rundllSegment, PR_TRUE);
  if (index == kNotFound) {
    strLen = rundllSegmentShort.Length();
    index = aCommandString.Find(rundllSegmentShort, PR_TRUE);
  }

  if (index != kNotFound) {
    PRUint32 rundllSegmentLength = index + strLen;
    aCommandString.Cut(0, rundllSegmentLength);
  }
}






 bool nsOSHelperAppService::CleanupCmdHandlerPath(nsAString& aCommandHandler)
{
  nsAutoString handlerCommand(aCommandHandler);

  
  
  
  
  
  
  
  
  
  
  
  
  

  
  PRUint32 bufLength = ::ExpandEnvironmentStringsW(handlerCommand.get(),
                                                   L"", 0);
  if (bufLength == 0) 
    return PR_FALSE;

  nsAutoArrayPtr<PRUnichar> destination(new PRUnichar[bufLength]);
  if (!destination)
    return PR_FALSE;
  if (!::ExpandEnvironmentStringsW(handlerCommand.get(), destination,
                                   bufLength))
    return PR_FALSE;

  handlerCommand = destination;

  
  handlerCommand.StripChars("\"");

  
  
  StripRundll32(handlerCommand);

  
  
  CleanupHandlerPath(handlerCommand);

  aCommandHandler.Assign(handlerCommand);
  return PR_TRUE;
}





nsresult
nsOSHelperAppService::GetDefaultAppInfo(const nsAString& aAppInfo,
                                        nsAString& aDefaultDescription, 
                                        nsIFile** aDefaultApplication)
{
  nsAutoString handlerCommand;

  
  
  aDefaultDescription = aAppInfo;
  *aDefaultApplication = nsnull;

  if (aAppInfo.IsEmpty())
    return NS_ERROR_FAILURE;

  
  
  
  
  

  nsAutoString handlerKeyName(aAppInfo);

  nsCOMPtr<nsIWindowsRegKey> chkKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!chkKey) 
    return NS_ERROR_FAILURE;
      
  nsresult rv = chkKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                             handlerKeyName, 
                             nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv)) {
    
    handlerCommand.Assign(aAppInfo);
  }
  else {
    handlerKeyName.AppendLiteral("\\shell\\open\\command");
    nsCOMPtr<nsIWindowsRegKey> regKey = 
      do_CreateInstance("@mozilla.org/windows-registry-key;1");
    if (!regKey) 
      return NS_ERROR_FAILURE;

    nsresult rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                               handlerKeyName, 
                               nsIWindowsRegKey::ACCESS_QUERY_VALUE);
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;
     
    
    rv = regKey->ReadStringValue(EmptyString(), handlerCommand);
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;
  }

  if (!CleanupCmdHandlerPath(handlerCommand))
    return NS_ERROR_FAILURE;

  
  
  
  
  
  nsCOMPtr<nsILocalFile> lf;
  NS_NewLocalFile(handlerCommand, PR_TRUE, getter_AddRefs(lf));
  if (!lf)
    return NS_ERROR_FILE_NOT_FOUND;

  nsILocalFileWin* lfw = nsnull;
  CallQueryInterface(lf, &lfw);

  if (lfw) {
    
    lfw->GetVersionInfoField("FileDescription", aDefaultDescription);
    
    *aDefaultApplication = lfw;
  }

  return NS_OK;
}

already_AddRefed<nsMIMEInfoWin> nsOSHelperAppService::GetByExtension(const nsAFlatString& aFileExt, const char *aTypeHint)
{
  if (aFileExt.IsEmpty())
    return nsnull;

  
  
  nsAutoString fileExtToUse;
  if (aFileExt.First() != PRUnichar('.'))
    fileExtToUse = PRUnichar('.');

  fileExtToUse.Append(aFileExt);

  
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!regKey) 
    return nsnull; 

  nsresult rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                             fileExtToUse,
                             nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv))
    return nsnull; 

  nsCAutoString typeToUse;
  if (aTypeHint && *aTypeHint) {
    typeToUse.Assign(aTypeHint);
  }
  else {
    nsAutoString temp;
    if (NS_FAILED(regKey->ReadStringValue(NS_LITERAL_STRING("Content Type"),
                  temp)) || temp.IsEmpty()) {
      return nsnull; 
    }
    
    LossyAppendUTF16toASCII(temp, typeToUse);
  }

  nsMIMEInfoWin* mimeInfo = new nsMIMEInfoWin(typeToUse);
  if (!mimeInfo)
    return nsnull; 

  NS_ADDREF(mimeInfo);

  
  mimeInfo->AppendExtension(NS_ConvertUTF16toUTF8(Substring(fileExtToUse, 1)));
  mimeInfo->SetPreferredAction(nsIMIMEInfo::useSystemDefault);

  nsAutoString appInfo;
  bool found;

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  
  if (mAppAssoc) {
    
    
    nsString assocType(fileExtToUse);
    PRUnichar * pResult = nsnull;
    HRESULT hr = mAppAssoc->QueryCurrentDefault(assocType.get(),
                                                AT_FILEEXTENSION, AL_EFFECTIVE,
                                                &pResult);
    if (SUCCEEDED(hr)) {
      found = PR_TRUE;
      appInfo.Assign(pResult);
      CoTaskMemFree(pResult);
    } 
    else {
      found = PR_FALSE;
    }
  } 
  else
#endif
  {
    found = NS_SUCCEEDED(regKey->ReadStringValue(EmptyString(), 
                                                 appInfo));
  }

  
  if (appInfo.EqualsLiteral("XPSViewer.Document"))
    found = PR_FALSE;

  if (!found) {
    NS_IF_RELEASE(mimeInfo); 
    return nsnull;
  }

  
  nsAutoString defaultDescription;
  nsCOMPtr<nsIFile> defaultApplication;
  
  if (NS_FAILED(GetDefaultAppInfo(appInfo, defaultDescription,
                                  getter_AddRefs(defaultApplication)))) {
    NS_IF_RELEASE(mimeInfo);
    return nsnull;
  }

  mimeInfo->SetDefaultDescription(defaultDescription);
  mimeInfo->SetDefaultApplicationHandler(defaultApplication);

  
  GetMIMEInfoFromRegistry(appInfo, mimeInfo);

  return mimeInfo;
}

already_AddRefed<nsIMIMEInfo> nsOSHelperAppService::GetMIMEInfoFromOS(const nsACString& aMIMEType, const nsACString& aFileExt, bool *aFound)
{
  *aFound = PR_TRUE;

  const nsCString& flatType = PromiseFlatCString(aMIMEType);
  const nsCString& flatExt = PromiseFlatCString(aFileExt);

  nsAutoString fileExtension;
  







  if (!aMIMEType.LowerCaseEqualsLiteral(APPLICATION_OCTET_STREAM)) {
    
    
    GetExtensionFromWindowsMimeDatabase(aMIMEType, fileExtension);
    LOG(("Windows mime database: extension '%s'\n", fileExtension.get()));
    if (fileExtension.IsEmpty()) {
      GetExtensionFrom4xRegistryInfo(aMIMEType, fileExtension);
      LOG(("4.x Registry: extension '%s'\n", fileExtension.get()));
    }
  }
  
  nsMIMEInfoWin* mi = nsnull;
  if (!fileExtension.IsEmpty())
    mi = GetByExtension(fileExtension, flatType.get()).get();
  LOG(("Extension lookup on '%s' found: 0x%p\n", fileExtension.get(), mi));

  bool hasDefault = false;
  if (mi) {
    mi->GetHasDefaultHandler(&hasDefault);
    
    
    
    
    
    if (!aFileExt.IsEmpty() && typeFromExtEquals(NS_ConvertUTF8toUTF16(flatExt).get(), flatType.get())) {
      LOG(("Appending extension '%s' to mimeinfo, because its mimetype is '%s'\n",
           flatExt.get(), flatType.get()));
      bool extExist = false;
      mi->ExtensionExists(aFileExt, &extExist);
      if (!extExist)
        mi->AppendExtension(aFileExt);
    }
  }
  if (!mi || !hasDefault) {
    nsRefPtr<nsMIMEInfoWin> miByExt =
      GetByExtension(NS_ConvertUTF8toUTF16(aFileExt), flatType.get());
    LOG(("Ext. lookup for '%s' found 0x%p\n", flatExt.get(), miByExt.get()));
    if (!miByExt && mi)
      return mi;
    if (miByExt && !mi) {
      miByExt.swap(mi);
      return mi;
    }
    if (!miByExt && !mi) {
      *aFound = PR_FALSE;
      mi = new nsMIMEInfoWin(flatType);
      if (mi) {
        NS_ADDREF(mi);
        if (!aFileExt.IsEmpty())
          mi->AppendExtension(aFileExt);
      }
      
      return mi;
    }

    
    nsCOMPtr<nsIFile> defaultApp;
    nsAutoString desc;
    miByExt->GetDefaultDescription(desc);

    mi->SetDefaultDescription(desc);
  }
  return mi;
}

NS_IMETHODIMP
nsOSHelperAppService::GetProtocolHandlerInfoFromOS(const nsACString &aScheme,
                                                   bool *found,
                                                   nsIHandlerInfo **_retval)
{
  NS_ASSERTION(!aScheme.IsEmpty(), "No scheme was specified!");

  nsresult rv = OSProtocolHandlerExists(nsPromiseFlatCString(aScheme).get(),
                                        found);
  if (NS_FAILED(rv))
    return rv;

  nsMIMEInfoWin *handlerInfo =
    new nsMIMEInfoWin(aScheme, nsMIMEInfoBase::eProtocolInfo);
  NS_ENSURE_TRUE(handlerInfo, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*_retval = handlerInfo);

  if (!*found) {
    
    
    return NS_OK;
  }

  nsAutoString desc;
  GetApplicationDescription(aScheme, desc);
  handlerInfo->SetDefaultDescription(desc);

  return NS_OK;
}


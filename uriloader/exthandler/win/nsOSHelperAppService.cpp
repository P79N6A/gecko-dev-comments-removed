






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
  , mAppAssoc(nullptr)
{
  CoInitialize(nullptr);
  CoCreateInstance(CLSID_ApplicationAssociationRegistration, nullptr,
                   CLSCTX_INPROC, IID_IApplicationAssociationRegistration,
                   (void**)&mAppAssoc);
}

nsOSHelperAppService::~nsOSHelperAppService()
{
  if (mAppAssoc)
    mAppAssoc->Release();
  mAppAssoc = nullptr;
  CoUninitialize();
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

  aFileExtension.Insert(char16_t('.'), 0);
      
  
  

  int32_t pos = aFileExtension.FindChar(char16_t(','));
  if (pos > 0) {
    
    
    aFileExtension.Truncate(pos); 
  }
   
  return NS_OK;
}

nsresult nsOSHelperAppService::OSProtocolHandlerExists(const char * aProtocolScheme, bool * aHandlerExists)
{
  
  *aHandlerExists = false;
  if (aProtocolScheme && *aProtocolScheme)
  {
    
    if (mAppAssoc) {
      wchar_t * pResult = nullptr;
      NS_ConvertASCIItoUTF16 scheme(aProtocolScheme);
      
      HRESULT hr = mAppAssoc->QueryCurrentDefault(scheme.get(),
                                                  AT_URLPROTOCOL, AL_EFFECTIVE,
                                                  &pResult);
      if (SUCCEEDED(hr)) {
        CoTaskMemFree(pResult);
        *aHandlerExists = true;
      }
      return NS_OK;
    }

    HKEY hKey;
    LONG err = ::RegOpenKeyExW(HKEY_CLASSES_ROOT,
                               NS_ConvertASCIItoUTF16(aProtocolScheme).get(),
                               0,
                               KEY_QUERY_VALUE,
                               &hKey);
    if (err == ERROR_SUCCESS)
    {
      err = ::RegQueryValueExW(hKey, L"URL Protocol",
                               nullptr, nullptr, nullptr, nullptr);
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

  
  if (mAppAssoc) {
    wchar_t * pResult = nullptr;
    
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
nsOSHelperAppService::typeFromExtEquals(const char16_t* aExt, const char *aType)
{
  if (!aType)
    return false;
  nsAutoString fileExtToUse;
  if (aExt[0] != char16_t('.'))
    fileExtToUse = char16_t('.');

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
  

  
  
  
  

  int32_t lastCommaPos = aPath.RFindChar(',');
  if (lastCommaPos != kNotFound)
    aPath.Truncate(lastCommaPos);

  aPath.Append(' ');

  
  uint32_t index = aPath.Find(".exe ", true);
  if (index == kNotFound)
    index = aPath.Find(".dll ", true);
  if (index == kNotFound)
    index = aPath.Find(".cpl ", true);

  if (index != kNotFound)
    aPath.Truncate(index + 4);
  aPath.Trim(" ", true, true);
}



static void StripRundll32(nsString& aCommandString)
{
  
  
  
  
  

  NS_NAMED_LITERAL_STRING(rundllSegment, "rundll32.exe ");
  NS_NAMED_LITERAL_STRING(rundllSegmentShort, "rundll32 ");

  
  int32_t strLen = rundllSegment.Length();
  int32_t index = aCommandString.Find(rundllSegment, true);
  if (index == kNotFound) {
    strLen = rundllSegmentShort.Length();
    index = aCommandString.Find(rundllSegmentShort, true);
  }

  if (index != kNotFound) {
    uint32_t rundllSegmentLength = index + strLen;
    aCommandString.Cut(0, rundllSegmentLength);
  }
}






 bool nsOSHelperAppService::CleanupCmdHandlerPath(nsAString& aCommandHandler)
{
  nsAutoString handlerCommand(aCommandHandler);

  
  
  
  
  
  
  
  
  
  
  
  
  

  
  uint32_t bufLength = ::ExpandEnvironmentStringsW(handlerCommand.get(),
                                                   L"", 0);
  if (bufLength == 0) 
    return false;

  nsAutoArrayPtr<wchar_t> destination(new wchar_t[bufLength]);
  if (!destination)
    return false;
  if (!::ExpandEnvironmentStringsW(handlerCommand.get(), destination,
                                   bufLength))
    return false;

  handlerCommand = static_cast<const wchar_t*>(destination);

  
  handlerCommand.StripChars("\"");

  
  
  StripRundll32(handlerCommand);

  
  
  CleanupHandlerPath(handlerCommand);

  aCommandHandler.Assign(handlerCommand);
  return true;
}





nsresult
nsOSHelperAppService::GetDefaultAppInfo(const nsAString& aAppInfo,
                                        nsAString& aDefaultDescription, 
                                        nsIFile** aDefaultApplication)
{
  nsAutoString handlerCommand;

  
  
  aDefaultDescription = aAppInfo;
  *aDefaultApplication = nullptr;

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
    if (NS_FAILED(rv)) {

      
      nsAutoString delegateExecute;
      rv = regKey->ReadStringValue(NS_LITERAL_STRING("DelegateExecute"), delegateExecute);
      NS_ENSURE_SUCCESS(rv, rv);

      
      nsAutoString delegateExecuteRegPath;
      delegateExecuteRegPath.AssignLiteral("CLSID\\");
      delegateExecuteRegPath.Append(delegateExecute);
      delegateExecuteRegPath.AppendLiteral("\\InProcServer32");
      rv = chkKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                        delegateExecuteRegPath, 
                        nsIWindowsRegKey::ACCESS_QUERY_VALUE);
      if (NS_SUCCEEDED(rv)) {
        rv = chkKey->ReadStringValue(EmptyString(), handlerCommand);
      }

      if (NS_FAILED(rv)) {
        
        delegateExecuteRegPath.AssignLiteral("CLSID\\");
        delegateExecuteRegPath.Append(delegateExecute);
        delegateExecuteRegPath.AppendLiteral("\\LocalServer32");
        rv = chkKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                          delegateExecuteRegPath, 
                          nsIWindowsRegKey::ACCESS_QUERY_VALUE);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = chkKey->ReadStringValue(EmptyString(), handlerCommand);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  if (!CleanupCmdHandlerPath(handlerCommand))
    return NS_ERROR_FAILURE;

  
  
  
  
  
  nsCOMPtr<nsIFile> lf;
  NS_NewLocalFile(handlerCommand, true, getter_AddRefs(lf));
  if (!lf)
    return NS_ERROR_FILE_NOT_FOUND;

  nsILocalFileWin* lfw = nullptr;
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
    return nullptr;

  
  
  nsAutoString fileExtToUse;
  if (aFileExt.First() != char16_t('.'))
    fileExtToUse = char16_t('.');

  fileExtToUse.Append(aFileExt);

  
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!regKey) 
    return nullptr; 

  nsresult rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                             fileExtToUse,
                             nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv))
    return nullptr; 

  nsAutoCString typeToUse;
  if (aTypeHint && *aTypeHint) {
    typeToUse.Assign(aTypeHint);
  }
  else {
    nsAutoString temp;
    if (NS_FAILED(regKey->ReadStringValue(NS_LITERAL_STRING("Content Type"),
                  temp)) || temp.IsEmpty()) {
      return nullptr; 
    }
    
    LossyAppendUTF16toASCII(temp, typeToUse);
  }

  nsRefPtr<nsMIMEInfoWin> mimeInfo = new nsMIMEInfoWin(typeToUse);

  
  mimeInfo->AppendExtension(NS_ConvertUTF16toUTF8(Substring(fileExtToUse, 1)));
  mimeInfo->SetPreferredAction(nsIMIMEInfo::useSystemDefault);

  nsAutoString appInfo;
  bool found;

  
  if (mAppAssoc) {
    
    
    nsString assocType(fileExtToUse);
    wchar_t * pResult = nullptr;
    HRESULT hr = mAppAssoc->QueryCurrentDefault(assocType.get(),
                                                AT_FILEEXTENSION, AL_EFFECTIVE,
                                                &pResult);
    if (SUCCEEDED(hr)) {
      found = true;
      appInfo.Assign(pResult);
      CoTaskMemFree(pResult);
    } 
    else {
      found = false;
    }
  } 
  else
  {
    found = NS_SUCCEEDED(regKey->ReadStringValue(EmptyString(), 
                                                 appInfo));
  }

  
  if (appInfo.EqualsLiteral("XPSViewer.Document"))
    found = false;

  if (!found) {
    return nullptr;
  }

  
  nsAutoString defaultDescription;
  nsCOMPtr<nsIFile> defaultApplication;
  
  if (NS_FAILED(GetDefaultAppInfo(appInfo, defaultDescription,
                                  getter_AddRefs(defaultApplication)))) {
    return nullptr;
  }

  mimeInfo->SetDefaultDescription(defaultDescription);
  mimeInfo->SetDefaultApplicationHandler(defaultApplication);

  
  GetMIMEInfoFromRegistry(appInfo, mimeInfo);

  return mimeInfo.forget();
}

already_AddRefed<nsIMIMEInfo> nsOSHelperAppService::GetMIMEInfoFromOS(const nsACString& aMIMEType, const nsACString& aFileExt, bool *aFound)
{
  *aFound = true;

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
  
  nsRefPtr<nsMIMEInfoWin> mi;
  if (!fileExtension.IsEmpty())
    mi = GetByExtension(fileExtension, flatType.get());
  LOG(("Extension lookup on '%s' found: 0x%p\n", fileExtension.get(), mi.get()));

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
      return mi.forget();
    if (miByExt && !mi) {
      return miByExt.forget();
    }
    if (!miByExt && !mi) {
      *aFound = false;
      mi = new nsMIMEInfoWin(flatType);
      if (!aFileExt.IsEmpty()) {
        mi->AppendExtension(aFileExt);
      }
      
      return mi.forget();
    }

    
    nsCOMPtr<nsIFile> defaultApp;
    nsAutoString desc;
    miByExt->GetDefaultDescription(desc);

    mi->SetDefaultDescription(desc);
  }
  return mi.forget();
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


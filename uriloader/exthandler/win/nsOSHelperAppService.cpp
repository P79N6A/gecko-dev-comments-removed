







































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


#include <windows.h>


#include <shellapi.h>

#define LOG(args) PR_LOG(mLog, PR_LOG_DEBUG, args)


static nsresult GetExtensionFrom4xRegistryInfo(const nsACString& aMimeType, 
                                               nsString& aFileExtension);
static nsresult GetExtensionFromWindowsMimeDatabase(const nsACString& aMimeType,
                                                    nsString& aFileExtension);

nsOSHelperAppService::nsOSHelperAppService() : nsExternalHelperAppService()
{}

nsOSHelperAppService::~nsOSHelperAppService()
{}



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

nsresult nsOSHelperAppService::OSProtocolHandlerExists(const char * aProtocolScheme, PRBool * aHandlerExists)
{
  
  *aHandlerExists = PR_FALSE;
  if (aProtocolScheme && *aProtocolScheme)
  {
     HKEY hKey;
     LONG err = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, aProtocolScheme, 0,
                               KEY_QUERY_VALUE, &hKey);
     if (err == ERROR_SUCCESS)
     {
       err = ::RegQueryValueEx(hKey, "URL Protocol", NULL, NULL, NULL, NULL);
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







 PRBool
nsOSHelperAppService::typeFromExtEquals(const PRUnichar* aExt, const char *aType)
{
  if (!aType)
    return PR_FALSE;
  nsAutoString fileExtToUse;
  if (aExt[0] != PRUnichar('.'))
    fileExtToUse = PRUnichar('.');

  fileExtToUse.Append(aExt);

  PRBool eq = PR_FALSE;
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

static void RemoveParameters(nsString& aPath)
{
  
  
  
  
  
  
  
  
  
  
  
  
  if (aPath.First() == PRUnichar('"')) {
    aPath = Substring(aPath, 1, aPath.Length() - 1);
    PRInt32 nextQuote = aPath.FindChar(PRUnichar('"'));
    if (nextQuote != kNotFound)
      aPath.Truncate(nextQuote);
  }
  else {
    PRInt32 firstSpace = aPath.FindChar(PRUnichar(' '));
    if (firstSpace != kNotFound) 
      aPath.Truncate(firstSpace);
  }
}


















nsresult
nsOSHelperAppService::GetDefaultAppInfo(const nsAString& aTypeName,
                                        nsAString& aDefaultDescription, 
                                        nsIFile** aDefaultApplication)
{
  
  
  aDefaultDescription = aTypeName;
  *aDefaultApplication = nsnull;

  nsAutoString handlerKeyName(aTypeName);
  handlerKeyName.AppendLiteral("\\shell\\open\\command");
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!regKey) 
    return NS_OK;

  nsresult rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                             handlerKeyName, 
                             nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv))
    return NS_OK;
   
  
  nsAutoString handlerCommand;
  rv = regKey->ReadStringValue(EmptyString(), handlerCommand);
  if (NS_FAILED(rv))
    return NS_OK;

  nsAutoString handlerFilePath;
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  NS_NAMED_LITERAL_STRING(rundllSegment, "rundll32.exe ");
  if (StringBeginsWith(handlerCommand, rundllSegment)) {
    PRInt32 lastCommaPos = handlerCommand.RFindChar(',');
    PRUint32 rundllSegmentLength = rundllSegment.Length();
    PRUint32 len;
    if (lastCommaPos != kNotFound)
      len = lastCommaPos - rundllSegmentLength;
    else
      len = handlerCommand.Length() - rundllSegmentLength;
    handlerFilePath = Substring(handlerCommand, rundllSegmentLength, len);
  }
  else
    handlerFilePath = handlerCommand;

  
  
  RemoveParameters(handlerFilePath);

  
  
  

  DWORD required = ::ExpandEnvironmentStringsW(handlerFilePath.get(),
                                               L"", 0);
  if (!required) 
    return NS_ERROR_FAILURE;

  nsAutoArrayPtr<WCHAR> destination(new WCHAR[required]); 
  if (!destination)
    return NS_ERROR_OUT_OF_MEMORY;
  if (!::ExpandEnvironmentStringsW(handlerFilePath.get(), destination,
                                   required))
    return NS_ERROR_FAILURE;

  handlerFilePath = destination;

  nsCOMPtr<nsILocalFile> lf;
  NS_NewLocalFile(handlerFilePath, PR_TRUE, getter_AddRefs(lf));
  if (!lf)
    return NS_ERROR_OUT_OF_MEMORY;

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
                  temp))) {
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

  nsAutoString description;
  PRBool found = NS_SUCCEEDED(regKey->ReadStringValue(EmptyString(), 
                                                      description));

  nsAutoString defaultDescription;
  nsCOMPtr<nsIFile> defaultApplication;
  GetDefaultAppInfo(description, defaultDescription,
                    getter_AddRefs(defaultApplication));

  mimeInfo->SetDefaultDescription(defaultDescription);
  mimeInfo->SetDefaultApplicationHandler(defaultApplication);

  
  if (found)
  {
      GetMIMEInfoFromRegistry(description, mimeInfo);
  }
  else {
    NS_IF_RELEASE(mimeInfo); 
  }

  return mimeInfo;
}

already_AddRefed<nsIMIMEInfo> nsOSHelperAppService::GetMIMEInfoFromOS(const nsACString& aMIMEType, const nsACString& aFileExt, PRBool *aFound)
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

  PRBool hasDefault = PR_FALSE;
  if (mi) {
    mi->GetHasDefaultHandler(&hasDefault);
    
    
    
    
    
    if (!aFileExt.IsEmpty() && typeFromExtEquals(NS_ConvertUTF8toUTF16(flatExt).get(), flatType.get())) {
      LOG(("Appending extension '%s' to mimeinfo, because its mimetype is '%s'\n",
           flatExt.get(), flatType.get()));
      PRBool extExist = PR_FALSE;
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

already_AddRefed<nsIHandlerInfo>
nsOSHelperAppService::GetProtocolInfoFromOS(const nsACString &aScheme)
{
  NS_ASSERTION(!aScheme.IsEmpty(), "No scheme was specified!");

  PRBool exists;
  nsresult rv = OSProtocolHandlerExists(nsPromiseFlatCString(aScheme).get(),
                                        &exists);
  if (NS_FAILED(rv) || !exists)
    return nsnull;

  nsMIMEInfoWin *handlerInfo =
    new nsMIMEInfoWin(aScheme, nsMIMEInfoBase::eProtocolInfo);
  NS_ENSURE_TRUE(handlerInfo, nsnull);
  NS_ADDREF(handlerInfo);

  nsAutoString desc;
  GetApplicationDescription(aScheme, desc);
  handlerInfo->SetDefaultDescription(desc);

  return handlerInfo;
}


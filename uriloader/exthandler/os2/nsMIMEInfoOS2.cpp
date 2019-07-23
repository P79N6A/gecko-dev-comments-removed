










































#ifdef MOZ_OS2_HIGH_MEMORY

#include <os2safe.h>
#endif

#include "nsMIMEInfoOS2.h"
#include "nsOSHelperAppService.h"
#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsReadableUtils.h"
#include "nsIProcess.h"
#include "nsNetUtil.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIVariant.h"
#include "nsArrayEnumerator.h"
#include "nsIRwsService.h"
#include <stdlib.h>



#define SALT_SIZE 8
#define TABLE_SIZE 36
static const PRUnichar table[] = 
  { 'a','b','c','d','e','f','g','h','i','j',
    'k','l','m','n','o','p','q','r','s','t',
    'u','v','w','x','y','z','0','1','2','3',
    '4','5','6','7','8','9'};


static PRBool sUseRws = PR_TRUE;



NS_IMPL_ISUPPORTS_INHERITED1(nsMIMEInfoOS2, nsMIMEInfoBase, nsIPropertyBag)

nsMIMEInfoOS2::~nsMIMEInfoOS2()
{
}



static nsresult Make8Dot3Name(nsIFile *aFile, nsACString& aPath)
{
  nsCAutoString leafName;
  aFile->GetNativeLeafName(leafName);
  const char *lastDot = strrchr(leafName.get(), '.');

  char suffix[8] = "";
  if (lastDot) {
    strncpy(suffix, lastDot, 4);
    suffix[4] = '\0';
  }

  nsCOMPtr<nsIFile> tempPath;
  nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tempPath));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString saltedTempLeafName;
  do {
    saltedTempLeafName.Truncate();

    
    
    double fpTime;
    LL_L2D(fpTime, PR_Now());
    srand((uint)(fpTime * 1e-6 + 0.5));

    for (PRInt32 i=0; i < SALT_SIZE; i++)
      saltedTempLeafName.Append(table[(rand()%TABLE_SIZE)]);

    AppendASCIItoUTF16(suffix, saltedTempLeafName);
    rv = aFile->CopyTo(tempPath, saltedTempLeafName);
  } while (NS_FAILED(rv));

  nsCOMPtr<nsPIExternalAppLauncher>
    helperAppService(do_GetService(NS_EXTERNALHELPERAPPSERVICE_CONTRACTID));
  if (!helperAppService)
    return NS_ERROR_FAILURE;

  tempPath->Append(saltedTempLeafName);
  helperAppService->DeleteTemporaryFileOnExit(tempPath);
  tempPath->GetNativePath(aPath);

  return rv;
}





NS_IMETHODIMP nsMIMEInfoOS2::LaunchWithFile(nsIFile *aFile)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIFile> application;
  if (mPreferredAction == useHelperApp) {
    nsCOMPtr<nsILocalHandlerApp> localHandlerApp =
      do_QueryInterface(mPreferredApplication, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = localHandlerApp->GetExecutable(getter_AddRefs(application));
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (mPreferredAction == useSystemDefault) {
    application = mDefaultApplication;
  } else {
    return NS_ERROR_INVALID_ARG;
  }

  nsCAutoString filePath;
  aFile->GetNativePath(filePath);

  
  if (!application) {
    rv = NS_ERROR_FAILURE;

    
    
    if (sUseRws) {
      PRUint32 appHandle;
      GetDefaultAppHandle(&appHandle);
      if (appHandle) {
        nsCOMPtr<nsIRwsService> rwsSvc(do_GetService("@mozilla.org/rwsos2;1"));
        if (!rwsSvc) {
          sUseRws = PR_FALSE;
        } else {
          
          
          rv = rwsSvc->OpenWithAppHandle(filePath.get(), appHandle);
        }
      }
    }

    
    if (NS_FAILED(rv)) {
      if (WinSetObjectData(WinQueryObject(filePath.get()), "OPEN=DEFAULT"))
        rv = NS_OK;
    }

    return rv;
  }

  
  nsCAutoString appPath;
  if (application) {
    application->GetNativePath(appPath);
  }

  ULONG ulAppType;
  DosQueryAppType(appPath.get(), &ulAppType);
  if (ulAppType & (FAPPTYP_DOS |
                   FAPPTYP_WINDOWSPROT31 |
                   FAPPTYP_WINDOWSPROT |
                   FAPPTYP_WINDOWSREAL)) {
    rv = Make8Dot3Name(aFile, filePath);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  filePath.Insert('\"', 0);
  filePath.Append('\"');

  
  
  rv = NS_ERROR_FAILURE;
  if (sUseRws) {
    nsCOMPtr<nsIRwsService> rwsSvc(do_GetService("@mozilla.org/rwsos2;1"));
    if (!rwsSvc) {
      sUseRws = PR_FALSE;
    } else {
      rv = rwsSvc->OpenWithAppPath(filePath.get(), appPath.get());
    }
  }

  
  if (NS_FAILED(rv)) {
    nsCOMPtr<nsIProcess> process = do_CreateInstance(NS_PROCESS_CONTRACTID);
    if (NS_FAILED(rv = process->Init(application)))
      return rv;
    const char *strPath = filePath.get();
    return process->Run(PR_FALSE, &strPath, 1);
  }

  return rv;
}





NS_IMETHODIMP nsMIMEInfoOS2::GetHasDefaultHandler(PRBool *_retval)
{
  *_retval = !mDefaultAppDescription.IsEmpty();
  return NS_OK;
}





NS_IMETHODIMP
nsMIMEInfoOS2::GetDefaultDescription(nsAString& aDefaultDescription)
{
  if (mDefaultAppDescription.IsEmpty() && mDefaultApplication)
    mDefaultApplication->GetLeafName(aDefaultDescription);
  else
    aDefaultDescription = mDefaultAppDescription;

  return NS_OK;
}






void nsMIMEInfoOS2::GetDefaultApplication(nsIFile **aDefaultAppHandler)
{
  *aDefaultAppHandler = mDefaultApplication;
  NS_IF_ADDREF(*aDefaultAppHandler);
  return;
}

void nsMIMEInfoOS2::SetDefaultApplication(nsIFile *aDefaultApplication)
{
  mDefaultApplication = aDefaultApplication;
  return;
}





void nsMIMEInfoOS2::GetDefaultAppHandle(PRUint32 *aHandle)
{
  if (aHandle) {
    if (mDefaultAppHandle <= 0x10000 || mDefaultAppHandle >= 0x40000)
      mDefaultAppHandle = 0;
    *aHandle = mDefaultAppHandle;
  }
  return;
}

void nsMIMEInfoOS2::SetDefaultAppHandle(PRUint32 aHandle)
{
  if (aHandle <= 0x10000 || aHandle >= 0x40000)
    mDefaultAppHandle = 0;
  else
    mDefaultAppHandle = aHandle;
  return;
}



nsresult nsMIMEInfoOS2::LoadUriInternal(nsIURI *aURL)
{
  nsresult rv;
  nsCOMPtr<nsIPrefService> thePrefsService(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  if (!thePrefsService) {
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIURI> uri = do_CreateInstance(NS_STANDARDURL_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }
  nsCAutoString urlSpec;
  aURL->GetSpec(urlSpec);
  uri->SetSpec(urlSpec);

  
  nsCAutoString uProtocol;
  uri->GetScheme(uProtocol);

  nsCAutoString prefName;
  prefName = NS_LITERAL_CSTRING("applications.") + uProtocol;

  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = thePrefsService->GetBranch(prefName.get(), getter_AddRefs(prefBranch));
  nsXPIDLCString prefString;
  if (NS_SUCCEEDED(rv)) {
    rv = prefBranch->GetCharPref(prefName.get(), getter_Copies(prefString));
  }

  nsCAutoString applicationName;
  nsCAutoString parameters;

  if (NS_FAILED(rv) || prefString.IsEmpty()) {
    char szAppFromINI[CCHMAXPATH];
    char szParamsFromINI[MAXINIPARAMLENGTH];
    
    rv = GetApplicationAndParametersFromINI(uProtocol,
                                            szAppFromINI, sizeof(szAppFromINI),
                                            szParamsFromINI, sizeof(szParamsFromINI));
    if (NS_SUCCEEDED(rv)) {
      applicationName = szAppFromINI;
      parameters = szParamsFromINI;
    } else {
      return NS_ERROR_FAILURE;
    }
  }

  
  nsCAutoString uURL, uUsername, uPassword, uHost, uPort, uPath;
  nsCAutoString uEmail, uGroup;
  PRInt32 iPort;

  
  
  aURL->GetAsciiSpec(uURL);
  uri->GetAsciiHost(uHost);
  uri->GetUsername(uUsername);
  NS_UnescapeURL(uUsername);
  uri->GetPassword(uPassword);
  NS_UnescapeURL(uPassword);
  uri->GetPort(&iPort);
  
  if (iPort != -1)
    uPort.AppendInt(iPort);
  uri->GetPath(uPath);
  NS_UnescapeURL(uPath);

  
  
  
  uEmail = uUsername + NS_LITERAL_CSTRING("@") + uHost;
  
  
  uGroup = Substring(uPath, 1, uPath.Length());

  NS_NAMED_LITERAL_CSTRING(url, "%url%");
  NS_NAMED_LITERAL_CSTRING(username, "%username%");
  NS_NAMED_LITERAL_CSTRING(password, "%password%");
  NS_NAMED_LITERAL_CSTRING(host, "%host%");
  NS_NAMED_LITERAL_CSTRING(port, "%port%");
  NS_NAMED_LITERAL_CSTRING(email, "%email%");
  NS_NAMED_LITERAL_CSTRING(group, "%group%");
  NS_NAMED_LITERAL_CSTRING(msgid, "%msgid%");
  NS_NAMED_LITERAL_CSTRING(channel, "%channel%");

  PRBool replaced = PR_FALSE;
  if (applicationName.IsEmpty() && parameters.IsEmpty()) {
    
    applicationName.Append(prefString);

    prefName.Append(".");
    nsCOMPtr<nsIPrefBranch> prefBranch;
    rv = thePrefsService->GetBranch(prefName.get(), getter_AddRefs(prefBranch));
    if (NS_SUCCEEDED(rv) && prefBranch) {
      rv = prefBranch->GetCharPref("parameters", getter_Copies(prefString));
      
      if (NS_SUCCEEDED(rv) && !prefString.IsEmpty()) {
        parameters.Append(" ");
        parameters.Append(prefString);

        PRInt32 pos = parameters.Find(url.get());
        if (pos != kNotFound) {
          nsCAutoString uURL;
          aURL->GetSpec(uURL);
          NS_UnescapeURL(uURL);
          uURL.Cut(0, uProtocol.Length()+1);
          parameters.Replace(pos, url.Length(), uURL);
          replaced = PR_TRUE;
        }
      } else {
        
        if (!uPort.IsEmpty()) {
          rv = prefBranch->GetCharPref("port", getter_Copies(prefString));
          if (NS_SUCCEEDED(rv) && !prefString.IsEmpty()) {
            parameters.Append(" ");
            parameters.Append(prefString);
          }
        }
        
        if (!uUsername.IsEmpty()) {
          rv = prefBranch->GetCharPref("username", getter_Copies(prefString));
          if (NS_SUCCEEDED(rv) && !prefString.IsEmpty()) {
            parameters.Append(" ");
            parameters.Append(prefString);
          }
        }
        
        if (!uPassword.IsEmpty()) {
          rv = prefBranch->GetCharPref("password", getter_Copies(prefString));
          if (NS_SUCCEEDED(rv) && !prefString.IsEmpty()) {
            parameters.Append(" ");
            parameters.Append(prefString);
          }
        }
        
        if (!uHost.IsEmpty()) {
          rv = prefBranch->GetCharPref("host", getter_Copies(prefString));
          if (NS_SUCCEEDED(rv) && !prefString.IsEmpty()) {
            parameters.Append(" ");
            parameters.Append(prefString);
          }
        }
      }
    }
  }

#ifdef DEBUG_peter
  printf("uURL=%s\n", uURL.get());
  printf("uUsername=%s\n", uUsername.get());
  printf("uPassword=%s\n", uPassword.get());
  printf("uHost=%s\n", uHost.get());
  printf("uPort=%s\n", uPort.get());
  printf("uPath=%s\n", uPath.get());
  printf("uEmail=%s\n", uEmail.get());
  printf("uGroup=%s\n", uGroup.get());
#endif

  PRInt32 pos;
  pos = parameters.Find(url.get());
  if (pos != kNotFound) {
    replaced = PR_TRUE;
    parameters.Replace(pos, url.Length(), uURL);
  }
  pos = parameters.Find(username.get());
  if (pos != kNotFound) {
    replaced = PR_TRUE;
    parameters.Replace(pos, username.Length(), uUsername);
  }
  pos = parameters.Find(password.get());
  if (pos != kNotFound) {
    replaced = PR_TRUE;
    parameters.Replace(pos, password.Length(), uPassword);
  }
  pos = parameters.Find(host.get());
  if (pos != kNotFound) {
    replaced = PR_TRUE;
    parameters.Replace(pos, host.Length(), uHost);
  }
  pos = parameters.Find(port.get());
  if (pos != kNotFound) {
    replaced = PR_TRUE;
    parameters.Replace(pos, port.Length(), uPort);
  }
  pos = parameters.Find(email.get());
  if (pos != kNotFound) {
    replaced = PR_TRUE;
    parameters.Replace(pos, email.Length(), uEmail);
  }
  pos = parameters.Find(group.get());
  if (pos != kNotFound) {
    replaced = PR_TRUE;
    parameters.Replace(pos, group.Length(), uGroup);
  }
  pos = parameters.Find(msgid.get());
  if (pos != kNotFound) {
    replaced = PR_TRUE;
    parameters.Replace(pos, msgid.Length(), uEmail);
  }
  pos = parameters.Find(channel.get());
  if (pos != kNotFound) {
    replaced = PR_TRUE;
    parameters.Replace(pos, channel.Length(), uGroup);
  }
  
  
  
  if (!replaced) {
    parameters.Append(" ");
    parameters.Append(uURL);
  }

  const char *params[3];
  params[0] = parameters.get();
#ifdef DEBUG_peter
  printf("params[0]=%s\n", params[0]);
#endif
  PRInt32 numParams = 1;

  nsCOMPtr<nsILocalFile> application;
  rv = NS_NewNativeLocalFile(nsDependentCString(applicationName.get()), PR_FALSE, getter_AddRefs(application));
  if (NS_FAILED(rv)) {
     
     char szAppPath[CCHMAXPATH];
     APIRET rc = DosSearchPath(SEARCH_IGNORENETERRS | SEARCH_ENVIRONMENT,
                               "PATH", applicationName.get(),
                               szAppPath, sizeof(szAppPath));
     if (rc == NO_ERROR) {
       rv = NS_NewNativeLocalFile(nsDependentCString(szAppPath), PR_FALSE, getter_AddRefs(application));
     }
     if (NS_FAILED(rv) || (rc != NO_ERROR)) {
       
       rv = NS_NewNativeLocalFile(nsDependentCString(getenv("COMSPEC")), PR_FALSE, getter_AddRefs(application));
       if (NS_FAILED(rv)) {
         return rv;
       }

       params[0] = "/c";
       params[1] = applicationName.get();
       params[2] = parameters.get();
       numParams = 3;
     }
  }

  nsCOMPtr<nsIProcess> process = do_CreateInstance(NS_PROCESS_CONTRACTID);

  if (NS_FAILED(rv = process->Init(application)))
     return rv;

  PRUint32 pid;
  if (NS_FAILED(rv = process->Run(PR_FALSE, params, numParams, &pid)))
    return rv;

  return NS_OK;
}





NS_IMETHODIMP
nsMIMEInfoOS2::GetEnumerator(nsISimpleEnumerator **_retval)
{
  nsCOMArray<nsIVariant> properties;

  nsCOMPtr<nsIVariant> variant;
  GetProperty(NS_LITERAL_STRING("defaultApplicationIconURL"), getter_AddRefs(variant));
  if (variant)
    properties.AppendObject(variant);

  GetProperty(NS_LITERAL_STRING("customApplicationIconURL"), getter_AddRefs(variant));
  if (variant)
    properties.AppendObject(variant);

  return NS_NewArrayEnumerator(_retval, properties);
}



NS_IMETHODIMP
nsMIMEInfoOS2::GetProperty(const nsAString& aName, nsIVariant **_retval)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (aName.EqualsLiteral(PROPERTY_DEFAULT_APP_ICON_URL)) {
    rv = GetIconURLVariant(mDefaultApplication, _retval);
  } else {
    if (aName.EqualsLiteral(PROPERTY_CUSTOM_APP_ICON_URL) &&
        mPreferredApplication) {
      
      nsCOMPtr<nsIFile> appFile;
      nsCOMPtr<nsILocalHandlerApp> localHandlerApp =
        do_QueryInterface(mPreferredApplication, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = localHandlerApp->GetExecutable(getter_AddRefs(appFile));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = GetIconURLVariant(appFile, _retval);
    }
  }

  return rv;
}



NS_IMETHODIMP
nsMIMEInfoOS2::GetIconURLVariant(nsIFile *aApplication, nsIVariant **_retval)
{
  nsresult rv = CallCreateInstance("@mozilla.org/variant;1", _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString fileURLSpec;
  if (aApplication)
    NS_GetURLSpecFromFile(aApplication, fileURLSpec);
  else {
    GetPrimaryExtension(fileURLSpec);
    fileURLSpec.Insert(NS_LITERAL_CSTRING("moztmp."), 0);
  }

  nsCAutoString iconURLSpec(NS_LITERAL_CSTRING("moz-icon://"));
  iconURLSpec += fileURLSpec;
  nsCOMPtr<nsIWritableVariant> writable(do_QueryInterface(*_retval));
  writable->SetAsAUTF8String(iconURLSpec);

  return NS_OK;
}

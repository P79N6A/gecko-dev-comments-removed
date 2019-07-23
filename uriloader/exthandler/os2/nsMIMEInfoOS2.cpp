









































#define INCL_DOS
#include <os2.h>

#include "nsMIMEInfoOS2.h"
#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsReadableUtils.h"
#include "nsIProcess.h"
#include <stdlib.h>

#define SALT_SIZE 8
#define TABLE_SIZE 36
static const PRUnichar table[] = 
  { 'a','b','c','d','e','f','g','h','i','j',
    'k','l','m','n','o','p','q','r','s','t',
    'u','v','w','x','y','z','0','1','2','3',
    '4','5','6','7','8','9'};


nsMIMEInfoOS2::~nsMIMEInfoOS2()
{
}

NS_IMETHODIMP nsMIMEInfoOS2::LaunchWithURI(nsIURI* aURI)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsILocalFile> docToLoad;
  rv = GetLocalFileFromURI(aURI, getter_AddRefs(docToLoad));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString path;
  docToLoad->GetNativePath(path);
  
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

  
  
  if (!application) {
    HOBJECT hobject = WinQueryObject(path.get());
    if (WinSetObjectData( hobject, "OPEN=DEFAULT" ))
      return NS_OK;
    else
      return NS_ERROR_FAILURE;
  }
  
  ULONG ulAppType;
  nsCAutoString apppath;
  application->GetNativePath(apppath);
  DosQueryAppType(apppath.get(), &ulAppType);
  if (ulAppType & (FAPPTYP_DOS |
                   FAPPTYP_WINDOWSPROT31 |
                   FAPPTYP_WINDOWSPROT |
                   FAPPTYP_WINDOWSREAL)) {
    
    
    nsCOMPtr<nsPIExternalAppLauncher> helperAppService (do_GetService(NS_EXTERNALHELPERAPPSERVICE_CONTRACTID));
    if (helperAppService)
    {
      nsCAutoString leafName; 
      docToLoad->GetNativeLeafName(leafName);
      const char* lastDot = strrchr(leafName.get(), '.');
      char suffix[CCHMAXPATH + 1] = "";
      if (lastDot)
      {
          strcpy(suffix, lastDot);
      }
      suffix[4] = '\0';
      
      nsAutoString saltedTempLeafName;
      do {
          saltedTempLeafName.Truncate();
          
          
          double fpTime;
          LL_L2D(fpTime, PR_Now());
          srand((uint)(fpTime * 1e-6 + 0.5));
          PRInt32 i;
          for (i=0;i<SALT_SIZE;i++) {
            saltedTempLeafName.Append(table[(rand()%TABLE_SIZE)]);
          }
          AppendASCIItoUTF16(suffix, saltedTempLeafName);
          rv = docToLoad->MoveTo(nsnull, saltedTempLeafName);
      } while (NS_FAILED(rv));
      helperAppService->DeleteTemporaryFileOnExit(docToLoad);
      docToLoad->GetNativePath(path);
    }
  } else {
    path.Insert('\"', 0);
    path.Append('\"');
  }
    
  const char * strPath = path.get();
  
  
  nsCOMPtr<nsIProcess> process = do_CreateInstance(NS_PROCESS_CONTRACTID);
  if (NS_FAILED(rv = process->Init(application)))
    return rv;
  PRUint32 pid;
  return process->Run(PR_FALSE, &strPath, 1, &pid);
}

nsresult nsMIMEInfoOS2::LoadUriInternal(nsIURI * aURL)
{
  LOG(("-- nsOSHelperAppService::LoadUriInternal\n"));
  nsCOMPtr<nsIPref> thePrefsService(do_GetService(NS_PREF_CONTRACTID));
  if (!thePrefsService) {
    return NS_ERROR_FAILURE;
  }

  
  nsresult rv;
  nsCOMPtr<nsIURI> uri = do_CreateInstance(kStandardURLCID, &rv);
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
  nsXPIDLCString prefString;

  nsCAutoString applicationName;
  nsCAutoString parameters;

  rv = thePrefsService->CopyCharPref(prefName.get(), getter_Copies(prefString));
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
  PRBool replaced = PR_FALSE;
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








#include "nsArrayEnumerator.h"
#include "nsCOMArray.h"
#include "nsIFile.h"
#include "nsIVariant.h"
#include "nsMIMEInfoWin.h"
#include "nsNetUtil.h"
#include <windows.h>
#include <shellapi.h>
#include "nsAutoPtr.h"
#include "nsIMutableArray.h"
#include "nsTArray.h"
#include "shlobj.h"
#include "windows.h"
#include "nsIWindowsRegKey.h"
#include "nsIProcess.h"
#include "nsOSHelperAppService.h"
#include "nsUnicharUtils.h"
#include "nsITextToSubURI.h"

#define RUNDLL32_EXE L"\\rundll32.exe"


NS_IMPL_ISUPPORTS_INHERITED1(nsMIMEInfoWin, nsMIMEInfoBase, nsIPropertyBag)

nsMIMEInfoWin::~nsMIMEInfoWin()
{
}

nsresult
nsMIMEInfoWin::LaunchDefaultWithFile(nsIFile* aFile)
{
  
  bool executable = true;
  aFile->IsExecutable(&executable);
  if (executable)
    return NS_ERROR_FAILURE;

  return aFile->Launch();
}

NS_IMETHODIMP
nsMIMEInfoWin::LaunchWithFile(nsIFile* aFile)
{
  nsresult rv;

  
  NS_ASSERTION(mClass == eMIMEInfo,
               "nsMIMEInfoBase should have mClass == eMIMEInfo");

  if (mPreferredAction == useSystemDefault) {
    return LaunchDefaultWithFile(aFile);
  }

  if (mPreferredAction == useHelperApp) {
    if (!mPreferredApplication)
      return NS_ERROR_FILE_NOT_FOUND;

    
    nsCOMPtr<nsILocalHandlerApp> localHandler = 
      do_QueryInterface(mPreferredApplication, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> executable;
    rv = localHandler->GetExecutable(getter_AddRefs(executable));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString path;
    aFile->GetPath(path);

    
    nsCString filename;
    executable->GetNativeLeafName(filename);
    if (filename.Length() > 4) {
      nsCString extension(Substring(filename, filename.Length() - 4, 4));

      if (extension.LowerCaseEqualsLiteral(".dll")) {
        nsAutoString args;

        
        
        if (!GetDllLaunchInfo(executable, aFile, args, false))
          return NS_ERROR_INVALID_ARG;

        WCHAR rundll32Path[MAX_PATH + sizeof(RUNDLL32_EXE) / sizeof(WCHAR) + 1] = {L'\0'};
        if (!GetSystemDirectoryW(rundll32Path, MAX_PATH)) {
          return NS_ERROR_FILE_NOT_FOUND;
        }
        lstrcatW(rundll32Path, RUNDLL32_EXE);

        SHELLEXECUTEINFOW seinfo;
        memset(&seinfo, 0, sizeof(seinfo));
        seinfo.cbSize = sizeof(SHELLEXECUTEINFOW);
        seinfo.fMask  = NULL;
        seinfo.hwnd   = NULL;
        seinfo.lpVerb = NULL;
        seinfo.lpFile = rundll32Path;
        seinfo.lpParameters =  args.get();
        seinfo.lpDirectory  = NULL;
        seinfo.nShow  = SW_SHOWNORMAL;
        if (ShellExecuteExW(&seinfo))
          return NS_OK;

        switch ((LONG_PTR)seinfo.hInstApp) {
          case 0:
          case SE_ERR_OOM:
            return NS_ERROR_OUT_OF_MEMORY;
          case SE_ERR_ACCESSDENIED:
            return NS_ERROR_FILE_ACCESS_DENIED;
          case SE_ERR_ASSOCINCOMPLETE:
          case SE_ERR_NOASSOC:
            return NS_ERROR_UNEXPECTED;
          case SE_ERR_DDEBUSY:
          case SE_ERR_DDEFAIL:
          case SE_ERR_DDETIMEOUT:
            return NS_ERROR_NOT_AVAILABLE;
          case SE_ERR_DLLNOTFOUND:
            return NS_ERROR_FAILURE;
          case SE_ERR_SHARE:
            return NS_ERROR_FILE_IS_LOCKED;
          default:
            switch(GetLastError()) {
              case ERROR_FILE_NOT_FOUND:
                return NS_ERROR_FILE_NOT_FOUND;
              case ERROR_PATH_NOT_FOUND:
                return NS_ERROR_FILE_UNRECOGNIZED_PATH;
              case ERROR_BAD_FORMAT:
                return NS_ERROR_FILE_CORRUPTED;
            }

        }
        return NS_ERROR_FILE_EXECUTION_FAILED;
      }
    }
    return LaunchWithIProcess(executable, path);
  }

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsMIMEInfoWin::GetHasDefaultHandler(bool * _retval)
{
  
  
  
  *_retval = !mDefaultAppDescription.IsEmpty();
  return NS_OK;
}

NS_IMETHODIMP
nsMIMEInfoWin::GetEnumerator(nsISimpleEnumerator* *_retval)
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

static nsresult GetIconURLVariant(nsIFile* aApplication, nsIVariant* *_retval)
{
  nsresult rv = CallCreateInstance("@mozilla.org/variant;1", _retval);
  if (NS_FAILED(rv))
    return rv;
  nsAutoCString fileURLSpec;
  NS_GetURLSpecFromFile(aApplication, fileURLSpec);
  nsAutoCString iconURLSpec; iconURLSpec.AssignLiteral("moz-icon://");
  iconURLSpec += fileURLSpec;
  nsCOMPtr<nsIWritableVariant> writable(do_QueryInterface(*_retval));
  writable->SetAsAUTF8String(iconURLSpec);
  return NS_OK;
}

NS_IMETHODIMP
nsMIMEInfoWin::GetProperty(const nsAString& aName, nsIVariant* *_retval)
{
  nsresult rv;
  if (mDefaultApplication && aName.EqualsLiteral(PROPERTY_DEFAULT_APP_ICON_URL)) {
    rv = GetIconURLVariant(mDefaultApplication, _retval);
    NS_ENSURE_SUCCESS(rv, rv);    
  } else if (mPreferredApplication && 
             aName.EqualsLiteral(PROPERTY_CUSTOM_APP_ICON_URL)) {
    nsCOMPtr<nsILocalHandlerApp> localHandler =
      do_QueryInterface(mPreferredApplication, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIFile> executable;
    rv = localHandler->GetExecutable(getter_AddRefs(executable));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetIconURLVariant(executable, _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}



nsresult
nsMIMEInfoWin::LoadUriInternal(nsIURI * aURL)
{
  nsresult rv = NS_OK;

  
  
  

  
  

  if (aURL)
  {
    
    nsAutoCString urlSpec;
    aURL->GetAsciiSpec(urlSpec);
 
    
    nsAutoCString urlCharset;
    nsAutoString utf16Spec;
    rv = aURL->GetOriginCharset(urlCharset);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsITextToSubURI> textToSubURI = do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = textToSubURI->UnEscapeNonAsciiURI(urlCharset, urlSpec, utf16Spec);
    NS_ENSURE_SUCCESS(rv, rv);

    static const PRUnichar cmdVerb[] = L"open";
    SHELLEXECUTEINFOW sinfo;
    memset(&sinfo, 0, sizeof(sinfo));
    sinfo.cbSize   = sizeof(sinfo);
    sinfo.fMask    = SEE_MASK_FLAG_DDEWAIT |
      SEE_MASK_FLAG_NO_UI;
    sinfo.hwnd     = NULL;
    sinfo.lpVerb   = (LPWSTR)&cmdVerb;
    sinfo.nShow    = SW_SHOWNORMAL;
    
    LPITEMIDLIST pidl = NULL;
    SFGAOF sfgao;
    
    
    if (SUCCEEDED(SHParseDisplayName(utf16Spec.get(),NULL, &pidl, 0, &sfgao))) {
      sinfo.lpIDList = pidl;
      sinfo.fMask |= SEE_MASK_INVOKEIDLIST;
    } else {
      
      
      rv = NS_ERROR_FAILURE;
    }
    if (NS_SUCCEEDED(rv)) {
      BOOL result = ShellExecuteExW(&sinfo);
      if (!result || ((LONG_PTR)sinfo.hInstApp) < 32)
        rv = NS_ERROR_FAILURE;
    }
    if (pidl)
      CoTaskMemFree(pidl);
  }
  
  return rv;
}


bool nsMIMEInfoWin::GetLocalHandlerApp(const nsAString& aCommandHandler,
                                         nsCOMPtr<nsILocalHandlerApp>& aApp)
{
  nsCOMPtr<nsIFile> locfile;
  nsresult rv = 
    NS_NewLocalFile(aCommandHandler, true, getter_AddRefs(locfile));
  if (NS_FAILED(rv))
    return false;

  aApp = do_CreateInstance("@mozilla.org/uriloader/local-handler-app;1");
  if (!aApp) 
    return false;

  aApp->SetExecutable(locfile);
  return true;
}



bool nsMIMEInfoWin::GetAppsVerbCommandHandler(const nsAString& appExeName,
                                                nsAString& applicationPath,
                                                bool edit)
{
  nsCOMPtr<nsIWindowsRegKey> appKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!appKey) 
    return false; 

  
  nsAutoString applicationsPath;
  applicationsPath.AppendLiteral("Applications\\");
  applicationsPath.Append(appExeName);

  nsresult rv = appKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                             applicationsPath,
                             nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv)) 
    return false;

  
  uint32_t value;
  if (NS_SUCCEEDED(appKey->ReadIntValue(
      NS_LITERAL_STRING("NoOpenWith"), &value)) &&
      value == 1)
    return false;

  nsAutoString dummy;
  if (NS_SUCCEEDED(appKey->ReadStringValue(
        NS_LITERAL_STRING("NoOpenWith"), dummy)))
    return false;

  appKey->Close();

  
  applicationsPath.AssignLiteral("Applications\\");
  applicationsPath.Append(appExeName);
  if (!edit)
    applicationsPath.AppendLiteral("\\shell\\open\\command");
  else
    applicationsPath.AppendLiteral("\\shell\\edit\\command");


  rv = appKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                    applicationsPath,
                    nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv)) 
    return false;

  nsAutoString appFilesystemCommand;
  if (NS_SUCCEEDED(appKey->ReadStringValue(EmptyString(), 
                                           appFilesystemCommand))) {
    
    
    if (!nsOSHelperAppService::CleanupCmdHandlerPath(appFilesystemCommand))
      return false;
    
    applicationPath = appFilesystemCommand;
    return true;
  }
  return false;
}





bool nsMIMEInfoWin::GetDllLaunchInfo(nsIFile * aDll,
                                       nsIFile * aFile,
                                       nsAString& args,
                                       bool edit)
{
  if (!aDll || !aFile) 
    return false;

  nsString appExeName;
  aDll->GetLeafName(appExeName);

  nsCOMPtr<nsIWindowsRegKey> appKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!appKey) 
    return false; 

  
  nsAutoString applicationsPath;
  applicationsPath.AppendLiteral("Applications\\");
  applicationsPath.Append(appExeName);

  nsresult rv = appKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                             applicationsPath,
                             nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv))
    return false;

  
  uint32_t value;
  rv = appKey->ReadIntValue(NS_LITERAL_STRING("NoOpenWith"), &value);
  if (NS_SUCCEEDED(rv) && value == 1)
    return false;

  nsAutoString dummy;
  if (NS_SUCCEEDED(appKey->ReadStringValue(NS_LITERAL_STRING("NoOpenWith"), 
                                           dummy)))
    return false;

  appKey->Close();

  
  applicationsPath.AssignLiteral("Applications\\");
  applicationsPath.Append(appExeName);
  if (!edit)
    applicationsPath.AppendLiteral("\\shell\\open\\command");
  else
    applicationsPath.AppendLiteral("\\shell\\edit\\command");

  rv = appKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                    applicationsPath,
                    nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv))
    return false;

  nsAutoString appFilesystemCommand;
  if (NS_SUCCEEDED(appKey->ReadStringValue(EmptyString(),
                                           appFilesystemCommand))) {
    
    uint32_t bufLength = 
      ::ExpandEnvironmentStringsW(appFilesystemCommand.get(),
                                  L"", 0);
    if (bufLength == 0) 
      return false;

    nsAutoArrayPtr<PRUnichar> destination(new PRUnichar[bufLength]);
    if (!destination)
      return false;
    if (!::ExpandEnvironmentStringsW(appFilesystemCommand.get(),
                                     destination,
                                     bufLength))
      return false;

    appFilesystemCommand = destination;

    
    
    nsAutoString params;
    NS_NAMED_LITERAL_STRING(rundllSegment, "rundll32.exe ");
    int32_t index = appFilesystemCommand.Find(rundllSegment);
    if (index > kNotFound) {
      params.Append(Substring(appFilesystemCommand,
                    index + rundllSegment.Length()));
    } else {
      params.Append(appFilesystemCommand);
    }

    
    NS_NAMED_LITERAL_STRING(percentOneParam, "%1");
    index = params.Find(percentOneParam);
    if (index == kNotFound) 
      return false;

    nsString target;
    aFile->GetTarget(target);
    params.Replace(index, 2, target);

    args = params;

    return true;
  }
  return false;
}



bool nsMIMEInfoWin::GetProgIDVerbCommandHandler(const nsAString& appProgIDName,
                                                  nsAString& applicationPath,
                                                  bool edit)
{
  nsCOMPtr<nsIWindowsRegKey> appKey =
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!appKey) 
    return false; 

  nsAutoString appProgId(appProgIDName);

  
  if (!edit)
    appProgId.AppendLiteral("\\shell\\open\\command");
  else
    appProgId.AppendLiteral("\\shell\\edit\\command");

  nsresult rv = appKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                             appProgId,
                             nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv))
    return false;

  nsAutoString appFilesystemCommand;
  if (NS_SUCCEEDED(appKey->ReadStringValue(EmptyString(), appFilesystemCommand))) {
    
    
    if (!nsOSHelperAppService::CleanupCmdHandlerPath(appFilesystemCommand))
      return false;
    
    applicationPath = appFilesystemCommand;
    return true;
  }
  return false;
}



void nsMIMEInfoWin::ProcessPath(nsCOMPtr<nsIMutableArray>& appList,
                                nsTArray<nsString>& trackList,
                                const nsAString& appFilesystemCommand)
{
  nsAutoString lower(appFilesystemCommand);
  ToLowerCase(lower);

  
  WCHAR exe[MAX_PATH+1];
  uint32_t len = GetModuleFileNameW(NULL, exe, MAX_PATH);
  if (len < MAX_PATH && len != 0) {
    uint32_t index = lower.Find(exe);
    if (index != -1)
      return;
  }

  nsCOMPtr<nsILocalHandlerApp> aApp;
  if (!GetLocalHandlerApp(appFilesystemCommand, aApp))
    return;

  
  appList->AppendElement(aApp, false);
  trackList.AppendElement(lower);
}



static bool IsPathInList(nsAString& appPath,
                           nsTArray<nsString>& trackList)
{
  
  
  nsAutoString tmp(appPath);
  ToLowerCase(tmp);

  for (uint32_t i = 0; i < trackList.Length(); i++) {
    if (tmp.Equals(trackList[i]))
      return true;
  }
  return false;
}










NS_IMETHODIMP
nsMIMEInfoWin::GetPossibleLocalHandlers(nsIArray **_retval)
{
  nsresult rv;

  *_retval = nullptr;

  nsCOMPtr<nsIMutableArray> appList =
    do_CreateInstance("@mozilla.org/array;1");

  if (!appList)
    return NS_ERROR_FAILURE;

  nsTArray<nsString> trackList;

  nsAutoCString fileExt;
  GetPrimaryExtension(fileExt);

  nsCOMPtr<nsIWindowsRegKey> regKey =
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!regKey) 
    return NS_ERROR_FAILURE; 
  nsCOMPtr<nsIWindowsRegKey> appKey =
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!appKey) 
    return NS_ERROR_FAILURE; 

  nsAutoString workingRegistryPath;

  bool extKnown = false;
  if (fileExt.IsEmpty()) {
    extKnown = true;
    
    
    
    
    
    nsAutoCString mimeType;
    GetMIMEType(mimeType);
    if (!mimeType.IsEmpty()) {
      workingRegistryPath.AppendLiteral("MIME\\Database\\Content Type\\");
      workingRegistryPath.Append(NS_ConvertASCIItoUTF16(mimeType));
            
      rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                        workingRegistryPath,
                        nsIWindowsRegKey::ACCESS_QUERY_VALUE);
      if(NS_SUCCEEDED(rv)) {
        nsAutoString mimeFileExt;
        if (NS_SUCCEEDED(regKey->ReadStringValue(EmptyString(), mimeFileExt))) {
          CopyUTF16toUTF8(mimeFileExt, fileExt);
          extKnown = false;
        }
      }
    }
  }

  nsAutoString fileExtToUse;
  if (fileExt.First() != '.')
    fileExtToUse = PRUnichar('.');
  fileExtToUse.Append(NS_ConvertUTF8toUTF16(fileExt));

  
  

  if (!extKnown) {
    
    workingRegistryPath = fileExtToUse;

    rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                      workingRegistryPath,
                      nsIWindowsRegKey::ACCESS_QUERY_VALUE);
    if (NS_SUCCEEDED(rv)) {
      nsAutoString appProgId;
      if (NS_SUCCEEDED(regKey->ReadStringValue(EmptyString(), appProgId))) {
        
        if (appProgId != NS_LITERAL_STRING("XPSViewer.Document")) {
          nsAutoString appFilesystemCommand;
          if (GetProgIDVerbCommandHandler(appProgId,
                                          appFilesystemCommand,
                                          false) &&
              !IsPathInList(appFilesystemCommand, trackList)) {
            ProcessPath(appList, trackList, appFilesystemCommand);
          }
        }
      }
      regKey->Close();
    }


    
    
    workingRegistryPath = fileExtToUse;
    workingRegistryPath.AppendLiteral("\\OpenWithList");

    rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                      workingRegistryPath,
                      nsIWindowsRegKey::ACCESS_QUERY_VALUE);
    if (NS_SUCCEEDED(rv)) {
      uint32_t count = 0;
      if (NS_SUCCEEDED(regKey->GetValueCount(&count)) && count > 0) {
        for (uint32_t index = 0; index < count; index++) {
          nsAutoString appName;
          if (NS_FAILED(regKey->GetValueName(index, appName)))
            continue;

          
          nsAutoString appFilesystemCommand;
          if (!GetAppsVerbCommandHandler(appName,
                                         appFilesystemCommand,
                                         false) ||
              IsPathInList(appFilesystemCommand, trackList))
            continue;
          ProcessPath(appList, trackList, appFilesystemCommand);
        }
      }
      regKey->Close();
    }


    
    

    workingRegistryPath = fileExtToUse;
    workingRegistryPath.AppendLiteral("\\OpenWithProgids");

    rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                      workingRegistryPath,
                      nsIWindowsRegKey::ACCESS_QUERY_VALUE);
    if (NS_SUCCEEDED(rv)) {
      uint32_t count = 0;
      if (NS_SUCCEEDED(regKey->GetValueCount(&count)) && count > 0) {
        for (uint32_t index = 0; index < count; index++) {
          
          nsAutoString appProgId;
          if (NS_FAILED(regKey->GetValueName(index, appProgId)))
            continue;

          nsAutoString appFilesystemCommand;
          if (!GetProgIDVerbCommandHandler(appProgId,
                                           appFilesystemCommand,
                                           false) ||
              IsPathInList(appFilesystemCommand, trackList))
            continue;
          ProcessPath(appList, trackList, appFilesystemCommand);
        }
      }
      regKey->Close();
    }


    

    

    workingRegistryPath =
      NS_LITERAL_STRING("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\");
    workingRegistryPath += fileExtToUse;
    workingRegistryPath.AppendLiteral("\\OpenWithList");

    rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                      workingRegistryPath,
                      nsIWindowsRegKey::ACCESS_QUERY_VALUE);
    if (NS_SUCCEEDED(rv)) {
      uint32_t count = 0;
      if (NS_SUCCEEDED(regKey->GetValueCount(&count)) && count > 0) {
        for (uint32_t index = 0; index < count; index++) {
          nsAutoString appName, appValue;
          if (NS_FAILED(regKey->GetValueName(index, appName)))
            continue;
          if (appName.EqualsLiteral("MRUList"))
            continue;
          if (NS_FAILED(regKey->ReadStringValue(appName, appValue)))
            continue;
          
          
          nsAutoString appFilesystemCommand;
          if (!GetAppsVerbCommandHandler(appValue,
                                         appFilesystemCommand,
                                         false) ||
              IsPathInList(appFilesystemCommand, trackList))
            continue;
          ProcessPath(appList, trackList, appFilesystemCommand);
        }
      }
    }
    

    
    
    
    

    workingRegistryPath =
      NS_LITERAL_STRING("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\");
    workingRegistryPath += fileExtToUse;
    workingRegistryPath.AppendLiteral("\\OpenWithProgids");

    regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                 workingRegistryPath,
                 nsIWindowsRegKey::ACCESS_QUERY_VALUE);
    if (NS_SUCCEEDED(rv)) {
      uint32_t count = 0;
      if (NS_SUCCEEDED(regKey->GetValueCount(&count)) && count > 0) {
        for (uint32_t index = 0; index < count; index++) {
          nsAutoString appIndex, appProgId;
          if (NS_FAILED(regKey->GetValueName(index, appProgId)))
            continue;

          nsAutoString appFilesystemCommand;
          if (!GetProgIDVerbCommandHandler(appProgId,
                                           appFilesystemCommand,
                                           false) ||
              IsPathInList(appFilesystemCommand, trackList))
            continue;
          ProcessPath(appList, trackList, appFilesystemCommand);
        }
      }
      regKey->Close();
    }


    
    
    

    workingRegistryPath = fileExtToUse;

    regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                 workingRegistryPath,
                 nsIWindowsRegKey::ACCESS_QUERY_VALUE);
    if (NS_SUCCEEDED(rv)) {
      nsAutoString perceivedType;
      rv = regKey->ReadStringValue(NS_LITERAL_STRING("PerceivedType"),
                                   perceivedType);
      if (NS_SUCCEEDED(rv)) {
        nsAutoString openWithListPath(NS_LITERAL_STRING("SystemFileAssociations\\"));
        openWithListPath.Append(perceivedType); 
        openWithListPath.Append(NS_LITERAL_STRING("\\OpenWithList"));

        nsresult rv = appKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                                   openWithListPath,
                                   nsIWindowsRegKey::ACCESS_QUERY_VALUE);
        if (NS_SUCCEEDED(rv)) {
          uint32_t count = 0;
          if (NS_SUCCEEDED(regKey->GetValueCount(&count)) && count > 0) {
            for (uint32_t index = 0; index < count; index++) {
              nsAutoString appName;
              if (NS_FAILED(regKey->GetValueName(index, appName)))
                continue;
              
              
              nsAutoString appFilesystemCommand;
              if (!GetAppsVerbCommandHandler(appName, appFilesystemCommand, 
                                             false) ||
                  IsPathInList(appFilesystemCommand, trackList))
                continue;
              ProcessPath(appList, trackList, appFilesystemCommand);
            }
          }
        }
      }
    }
  } 


  


  workingRegistryPath = NS_LITERAL_STRING("*\\OpenWithList");

  rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                    workingRegistryPath,
                    nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_SUCCEEDED(rv)) {
    uint32_t count = 0;
    if (NS_SUCCEEDED(regKey->GetValueCount(&count)) && count > 0) {
      for (uint32_t index = 0; index < count; index++) {
        nsAutoString appName;
        if (NS_FAILED(regKey->GetValueName(index, appName)))
          continue;

        
        nsAutoString appFilesystemCommand;
        if (!GetAppsVerbCommandHandler(appName, appFilesystemCommand,
                                       false) ||
            IsPathInList(appFilesystemCommand, trackList))
          continue;
        ProcessPath(appList, trackList, appFilesystemCommand);
      }
    }
    regKey->Close();
  }


  
  workingRegistryPath = NS_LITERAL_STRING("Applications");

  rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                    workingRegistryPath,
                    nsIWindowsRegKey::ACCESS_ENUMERATE_SUB_KEYS|
                    nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_SUCCEEDED(rv)) {
    uint32_t count = 0;
    if (NS_SUCCEEDED(regKey->GetChildCount(&count)) && count > 0) {
      for (uint32_t index = 0; index < count; index++) {
        nsAutoString appName;
        if (NS_FAILED(regKey->GetChildName(index, appName)))
          continue;

        
        nsAutoString appFilesystemCommand;
        if (!GetAppsVerbCommandHandler(appName, appFilesystemCommand,
                                       false) ||
            IsPathInList(appFilesystemCommand, trackList))
          continue;
        ProcessPath(appList, trackList, appFilesystemCommand);
      }
    }
  }

  
  *_retval = appList;
  NS_ADDREF(*_retval);

  return NS_OK;
}

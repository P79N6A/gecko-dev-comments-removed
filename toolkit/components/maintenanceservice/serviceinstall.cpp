



#include <windows.h>
#include <aclapi.h>
#include <stdlib.h>
#include <shlwapi.h>


#include <lm.h>

#include <nsWindowsHelpers.h>
#include "mozilla/UniquePtr.h"

#include "serviceinstall.h"
#include "servicebase.h"
#include "updatehelper.h"
#include "shellapi.h"
#include "readstrings.h"
#include "errors.h"

#pragma comment(lib, "version.lib")


#define MAINT_UNINSTALL_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MozillaMaintenanceService"

static BOOL
UpdateUninstallerVersionString(LPWSTR versionString)
{
  HKEY uninstallKey;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    MAINT_UNINSTALL_KEY, 0,
                    KEY_WRITE | KEY_WOW64_32KEY,
                    &uninstallKey) != ERROR_SUCCESS) {
    return FALSE;
  }

  LONG rv = RegSetValueExW(uninstallKey, L"DisplayVersion", 0, REG_SZ,
                           reinterpret_cast<const BYTE *>(versionString),
                           (wcslen(versionString) + 1) * sizeof(WCHAR));
  RegCloseKey(uninstallKey);
  return rv == ERROR_SUCCESS;
}








static int
ReadMaintenanceServiceStrings(LPCWSTR path, 
                              MaintenanceServiceStringTable *results)
{
  
  const unsigned int kNumStrings = 1;
  const char *kServiceKeys = "MozillaMaintenanceDescription\0";
  char serviceStrings[kNumStrings][MAX_TEXT_LEN];
  int result = ReadStrings(path, kServiceKeys, 
                           kNumStrings, serviceStrings);
  if (result != OK) {
    serviceStrings[0][0] = '\0';
  }
  strncpy(results->serviceDescription, 
          serviceStrings[0], MAX_TEXT_LEN - 1);
  results->serviceDescription[MAX_TEXT_LEN - 1] = '\0';
  return result;
}












static BOOL
GetVersionNumberFromPath(LPWSTR path, DWORD &A, DWORD &B, 
                         DWORD &C, DWORD &D) 
{
  DWORD fileVersionInfoSize = GetFileVersionInfoSizeW(path, 0);
  mozilla::UniquePtr<char[]> fileVersionInfo(new char[fileVersionInfoSize]);
  if (!GetFileVersionInfoW(path, 0, fileVersionInfoSize,
                           fileVersionInfo.get())) {
      LOG_WARN(("Could not obtain file info of old service.  (%d)", 
                GetLastError()));
      return FALSE;
  }

  VS_FIXEDFILEINFO *fixedFileInfo = 
    reinterpret_cast<VS_FIXEDFILEINFO *>(fileVersionInfo.get());
  UINT size;
  if (!VerQueryValueW(fileVersionInfo.get(), L"\\", 
    reinterpret_cast<LPVOID*>(&fixedFileInfo), &size)) {
      LOG_WARN(("Could not query file version info of old service.  (%d)", 
                GetLastError()));
      return FALSE;
  }  

  A = HIWORD(fixedFileInfo->dwFileVersionMS);
  B = LOWORD(fixedFileInfo->dwFileVersionMS);
  C = HIWORD(fixedFileInfo->dwFileVersionLS);
  D = LOWORD(fixedFileInfo->dwFileVersionLS);
  return TRUE;
}









BOOL
UpdateServiceDescription(SC_HANDLE serviceHandle)
{
  WCHAR updaterINIPath[MAX_PATH + 1];
  if (!GetModuleFileNameW(nullptr, updaterINIPath, 
                          sizeof(updaterINIPath) /
                          sizeof(updaterINIPath[0]))) {
    LOG_WARN(("Could not obtain module filename when attempting to "
              "modify service description.  (%d)", GetLastError()));
    return FALSE;
  }

  if (!PathRemoveFileSpecW(updaterINIPath)) {
    LOG_WARN(("Could not remove file spec when attempting to "
              "modify service description.  (%d)", GetLastError()));
    return FALSE;
  }

  if (!PathAppendSafe(updaterINIPath, L"updater.ini")) {
    LOG_WARN(("Could not append updater.ini filename when attempting to "
              "modify service description.  (%d)", GetLastError()));
    return FALSE;
  }

  if (GetFileAttributesW(updaterINIPath) == INVALID_FILE_ATTRIBUTES) {
    LOG_WARN(("updater.ini file does not exist, will not modify "
              "service description.  (%d)", GetLastError()));
    return FALSE;
  }
  
  MaintenanceServiceStringTable serviceStrings;
  int rv = ReadMaintenanceServiceStrings(updaterINIPath, &serviceStrings);
  if (rv != OK || !strlen(serviceStrings.serviceDescription)) {
    LOG_WARN(("updater.ini file does not contain a maintenance "
              "service description."));
    return FALSE;
  }

  WCHAR serviceDescription[MAX_TEXT_LEN];
  if (!MultiByteToWideChar(CP_UTF8, 0, 
                           serviceStrings.serviceDescription, -1,
                           serviceDescription,
                           sizeof(serviceDescription) / 
                           sizeof(serviceDescription[0]))) {
    LOG_WARN(("Could not convert description to wide string format.  (%d)",
              GetLastError()));
    return FALSE;
  }

  SERVICE_DESCRIPTIONW descriptionConfig;
  descriptionConfig.lpDescription = serviceDescription;
  if (!ChangeServiceConfig2W(serviceHandle, 
                             SERVICE_CONFIG_DESCRIPTION, 
                             &descriptionConfig)) {
    LOG_WARN(("Could not change service config.  (%d)", GetLastError()));
    return FALSE;
  }

  LOG(("The service description was updated successfully."));
  return TRUE;
}










BOOL
FixServicePath(SC_HANDLE service,
               LPCWSTR currentServicePath,
               BOOL &servicePathWasWrong)
{
  
  
  
  
  
  
  
  
  size_t currentServicePathLen = wcslen(currentServicePath);
  bool doesServiceHaveCorrectPath =
    currentServicePathLen > 2 &&
    !wcsstr(currentServicePath, L"maintenanceservice_tmp.exe") &&
    currentServicePath[0] == L'\"' &&
    currentServicePath[currentServicePathLen - 1] == L'\"';

  if (doesServiceHaveCorrectPath) {
    LOG(("The MozillaMaintenance service path is correct."));
    servicePathWasWrong = FALSE;
    return TRUE;
  }
  
  LOG(("The MozillaMaintenance path is NOT correct. It was: %ls",
       currentServicePath));

  servicePathWasWrong = TRUE;
  WCHAR fixedPath[MAX_PATH + 1] = { L'\0' };
  wcsncpy(fixedPath, currentServicePath, MAX_PATH);
  PathUnquoteSpacesW(fixedPath);
  if (!PathRemoveFileSpecW(fixedPath)) {
    LOG_WARN(("Couldn't remove file spec.  (%d)", GetLastError()));
    return FALSE;
  }
  if (!PathAppendSafe(fixedPath, L"maintenanceservice.exe")) {
    LOG_WARN(("Couldn't append file spec.  (%d)", GetLastError()));
    return FALSE;
  }
  PathQuoteSpacesW(fixedPath);


  if (!ChangeServiceConfigW(service, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE,
                            SERVICE_NO_CHANGE, fixedPath, nullptr, nullptr,
                            nullptr, nullptr, nullptr, nullptr)) {
    LOG_WARN(("Could not fix service path.  (%d)", GetLastError()));
    return FALSE;
  }

  LOG(("Fixed service path to: %ls.", fixedPath));
  return TRUE;
}









BOOL
SvcInstall(SvcInstallAction action)
{
  
  nsAutoServiceHandle schSCManager(OpenSCManager(nullptr, nullptr, 
                                                 SC_MANAGER_ALL_ACCESS));
  if (!schSCManager) {
    LOG_WARN(("Could not open service manager.  (%d)", GetLastError()));
    return FALSE;
  }

  WCHAR newServiceBinaryPath[MAX_PATH + 1];
  if (!GetModuleFileNameW(nullptr, newServiceBinaryPath, 
                          sizeof(newServiceBinaryPath) / 
                          sizeof(newServiceBinaryPath[0]))) {
    LOG_WARN(("Could not obtain module filename when attempting to "
              "install service.  (%d)",
              GetLastError()));
    return FALSE;
  }

  
  nsAutoServiceHandle schService(OpenServiceW(schSCManager, 
                                              SVC_NAME, 
                                              SERVICE_ALL_ACCESS));
  DWORD lastError = GetLastError();
  if (!schService && ERROR_SERVICE_DOES_NOT_EXIST != lastError) {
    
    LOG_WARN(("Could not open service.  (%d)", GetLastError()));
    return FALSE;
  }
  
  if (schService) {
    
    
    
    
    if (!SetUserAccessServiceDACL(schService)) {
      LOG_WARN(("Could not reset security ACE on service handle. It might not be "
                "possible to start the service. This error should never "
                "happen.  (%d)", GetLastError()));
    }

    
    DWORD bytesNeeded;
    if (!QueryServiceConfigW(schService, nullptr, 0, &bytesNeeded) && 
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      LOG_WARN(("Could not determine buffer size for query service config.  (%d)",
                GetLastError()));
      return FALSE;
    }

    
    
    mozilla::UniquePtr<char[]> serviceConfigBuffer(new char[bytesNeeded]);
    if (!QueryServiceConfigW(schService, 
        reinterpret_cast<QUERY_SERVICE_CONFIGW*>(serviceConfigBuffer.get()), 
        bytesNeeded, &bytesNeeded)) {
      LOG_WARN(("Could open service but could not query service config.  (%d)",
                GetLastError()));
      return FALSE;
    }
    QUERY_SERVICE_CONFIGW &serviceConfig = 
      *reinterpret_cast<QUERY_SERVICE_CONFIGW*>(serviceConfigBuffer.get());

    
    BOOL servicePathWasWrong;
    static BOOL alreadyCheckedFixServicePath = FALSE;
    if (!alreadyCheckedFixServicePath) {
      if (!FixServicePath(schService, serviceConfig.lpBinaryPathName,
                          servicePathWasWrong)) {
        LOG_WARN(("Could not fix service path. This should never happen.  (%d)",
                  GetLastError()));
        
        
        
        return TRUE;
      } else if (servicePathWasWrong) {
        
        
        
        
        
        alreadyCheckedFixServicePath = TRUE;
        LOG(("Restarting install action: %d", action));
        return SvcInstall(action);
      }
    }

    
    
    
    PathUnquoteSpacesW(serviceConfig.lpBinaryPathName);

    
    
    
    DWORD existingA, existingB, existingC, existingD;
    DWORD newA, newB, newC, newD; 
    BOOL obtainedExistingVersionInfo = 
      GetVersionNumberFromPath(serviceConfig.lpBinaryPathName, 
                               existingA, existingB, 
                               existingC, existingD);
    if (!GetVersionNumberFromPath(newServiceBinaryPath, newA, 
                                 newB, newC, newD)) {
      LOG_WARN(("Could not obtain version number from new path"));
      return FALSE;
    }

    
    
    
    if (ForceInstallSvc == action ||
        !obtainedExistingVersionInfo || 
        (existingA < newA) ||
        (existingA == newA && existingB < newB) ||
        (existingA == newA && existingB == newB && 
         existingC < newC) ||
        (existingA == newA && existingB == newB && 
         existingC == newC && existingD < newD)) {

      
      UpdateServiceDescription(schService);

      schService.reset();
      if (!StopService()) {
        return FALSE;
      }

      if (!wcscmp(newServiceBinaryPath, serviceConfig.lpBinaryPathName)) {
        LOG(("File is already in the correct location, no action needed for "
             "upgrade.  The path is: \"%ls\"", newServiceBinaryPath));
        return TRUE;
      }

      BOOL result = TRUE;

      
      
      
      if (!CopyFileW(newServiceBinaryPath, 
                     serviceConfig.lpBinaryPathName, FALSE)) {
        LOG_WARN(("Could not overwrite old service binary file. "
                  "This should never happen, but if it does the next "
                  "upgrade will fix it, the service is not a critical "
                  "component that needs to be installed for upgrades "
                  "to work.  (%d)", GetLastError()));

        
        
        const size_t len = wcslen(serviceConfig.lpBinaryPathName);
        if (len > 3) {
          
          
          LPWSTR oldServiceBinaryTempPath = 
            new WCHAR[len + 1];
          memset(oldServiceBinaryTempPath, 0, (len + 1) * sizeof (WCHAR));
          wcsncpy(oldServiceBinaryTempPath, serviceConfig.lpBinaryPathName, len);
          
          wcsncpy(oldServiceBinaryTempPath + len - 3, L"old", 3);

          
          if (MoveFileExW(serviceConfig.lpBinaryPathName, 
                          oldServiceBinaryTempPath, 
                          MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            
            if (!CopyFileW(newServiceBinaryPath, 
                           serviceConfig.lpBinaryPathName, FALSE)) {
              
              LOG_WARN(("The new service binary could not be copied in."
                        " The service will not be upgraded."));
              result = FALSE;
            } else {
              LOG(("The new service binary was copied in by first moving the"
                   " old one out of the way."));
            }

            
            if (DeleteFileW(oldServiceBinaryTempPath)) {
              LOG(("The old temp service path was deleted: %ls.",
                   oldServiceBinaryTempPath));
            } else {
              
              
              LOG_WARN(("The old temp service path was not deleted."));
            }
          } else {
            
            LOG_WARN(("Could not move old service file out of the way from:"
                      " \"%ls\" to \"%ls\". Service will not be upgraded.  (%d)",
                      serviceConfig.lpBinaryPathName,
                      oldServiceBinaryTempPath, GetLastError()));
            result = FALSE;
          }
          delete[] oldServiceBinaryTempPath;
        } else {
            
            LOG_WARN(("Service binary path was less than 3, service will"
                      " not be updated.  This should never happen."));
            result = FALSE;
        }
      } else {
        WCHAR versionStr[128] = { L'\0' };
        swprintf(versionStr, 128, L"%d.%d.%d.%d", newA, newB, newC, newD);
        if (!UpdateUninstallerVersionString(versionStr)) {
            LOG(("The uninstaller version string could not be updated."));
        }
        LOG(("The new service binary was copied in."));
      }

      
      
      
      if (MoveFileExW(newServiceBinaryPath, nullptr,
                      MOVEFILE_DELAY_UNTIL_REBOOT)) {
        LOG(("Deleting the old file path on the next reboot: %ls.",
             newServiceBinaryPath));
      } else {
        LOG_WARN(("Call to delete the old file path failed: %ls.",
                  newServiceBinaryPath));
      }
      
      return result;
    }

    
    
    
    MoveFileExW(newServiceBinaryPath, nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
    
    
    return TRUE; 
  }
  
  
  if (UpgradeSvc == action) {
    
    return TRUE;
  }

  
  PathQuoteSpacesW(newServiceBinaryPath);
  
  schService.own(CreateServiceW(schSCManager, SVC_NAME, SVC_DISPLAY_NAME,
                                SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
                                SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                                newServiceBinaryPath, nullptr, nullptr,
                                nullptr, nullptr, nullptr));
  if (!schService) {
    LOG_WARN(("Could not create Windows service. "
              "This error should never happen since a service install "
              "should only be called when elevated.  (%d)", GetLastError()));
    return FALSE;
  } 

  if (!SetUserAccessServiceDACL(schService)) {
    LOG_WARN(("Could not set security ACE on service handle, the service will not "
              "be able to be started from unelevated processes. "
              "This error should never happen.  (%d)",
              GetLastError()));
  }

  UpdateServiceDescription(schService);

  return TRUE;
}






BOOL
StopService()
{
  
  nsAutoServiceHandle schSCManager(OpenSCManager(nullptr, nullptr, 
                                                 SC_MANAGER_ALL_ACCESS));
  if (!schSCManager) {
    LOG_WARN(("Could not open service manager.  (%d)", GetLastError()));
    return FALSE;
  }

  
  nsAutoServiceHandle schService(OpenServiceW(schSCManager, SVC_NAME, 
                                              SERVICE_ALL_ACCESS));
  if (!schService) {
    LOG_WARN(("Could not open service.  (%d)", GetLastError()));
    return FALSE;
  } 

  LOG(("Sending stop request..."));
  SERVICE_STATUS status;
  SetLastError(ERROR_SUCCESS);
  if (!ControlService(schService, SERVICE_CONTROL_STOP, &status) &&
      GetLastError() != ERROR_SERVICE_NOT_ACTIVE) {
    LOG_WARN(("Error sending stop request.  (%d)", GetLastError()));
  }

  schSCManager.reset();
  schService.reset();

  LOG(("Waiting for service stop..."));
  DWORD lastState = WaitForServiceStop(SVC_NAME, 30);

  
  
  WaitForProcessExit(L"maintenanceservice.exe", 30);
  LOG(("Done waiting for service stop, last service state: %d", lastState));

  return lastState == SERVICE_STOPPED;
}






BOOL
SvcUninstall()
{
  
  nsAutoServiceHandle schSCManager(OpenSCManager(nullptr, nullptr, 
                                                 SC_MANAGER_ALL_ACCESS));
  if (!schSCManager) {
    LOG_WARN(("Could not open service manager.  (%d)", GetLastError()));
    return FALSE;
  }

  
  nsAutoServiceHandle schService(OpenServiceW(schSCManager, SVC_NAME, 
                                              SERVICE_ALL_ACCESS));
  if (!schService) {
    LOG_WARN(("Could not open service.  (%d)", GetLastError()));
    return FALSE;
  } 

  
  
  DWORD totalWaitTime = 0;
  SERVICE_STATUS status;
  static const int maxWaitTime = 1000 * 60; 
  if (ControlService(schService, SERVICE_CONTROL_STOP, &status)) {
    do {
      Sleep(status.dwWaitHint);
      totalWaitTime += (status.dwWaitHint + 10);
      if (status.dwCurrentState == SERVICE_STOPPED) {
        break;
      } else if (totalWaitTime > maxWaitTime) {
        break;
      }
    } while (QueryServiceStatus(schService, &status));
  }

  
  BOOL deleted = DeleteService(schService);
  if (!deleted) {
    deleted = (GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE);
  }

  return deleted;
}







BOOL
SetUserAccessServiceDACL(SC_HANDLE hService)
{
  PACL pNewAcl = nullptr;
  PSECURITY_DESCRIPTOR psd = nullptr;
  DWORD lastError = SetUserAccessServiceDACL(hService, pNewAcl, psd);
  if (pNewAcl) {
    LocalFree((HLOCAL)pNewAcl);
  }
  if (psd) {
    LocalFree((LPVOID)psd);
  }
  return ERROR_SUCCESS == lastError;
}









DWORD
SetUserAccessServiceDACL(SC_HANDLE hService, PACL &pNewAcl, 
                         PSECURITY_DESCRIPTOR psd)
{
  
  DWORD needed = 0;
  if (!QueryServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, 
                                  &psd, 0, &needed)) {
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      LOG_WARN(("Could not query service object security size.  (%d)",
                GetLastError()));
      return GetLastError();
    }

    DWORD size = needed;
    psd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, size);
    if (!psd) {
      LOG_WARN(("Could not allocate security descriptor.  (%d)",
                GetLastError()));
      return ERROR_INSUFFICIENT_BUFFER;
    }

    
    if (!QueryServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, 
                                    psd, size, &needed)) {
      LOG_WARN(("Could not allocate security descriptor.  (%d)",
                GetLastError()));
      return GetLastError();
    }
  }

  
  PACL pacl = nullptr;
  BOOL bDaclPresent = FALSE;
  BOOL bDaclDefaulted = FALSE;
  if ( !GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl, 
                                  &bDaclDefaulted)) {
    LOG_WARN(("Could not obtain DACL.  (%d)", GetLastError()));
    return GetLastError();
  }

  PSID sid;
  DWORD SIDSize = SECURITY_MAX_SID_SIZE;
  sid = LocalAlloc(LMEM_FIXED, SIDSize);
  if (!sid) {
    LOG_WARN(("Could not allocate SID memory.  (%d)", GetLastError()));
    return GetLastError();
  }

  if (!CreateWellKnownSid(WinBuiltinUsersSid, nullptr, sid, &SIDSize)) {
    DWORD lastError = GetLastError();
    LOG_WARN(("Could not create well known SID.  (%d)", lastError));
    LocalFree(sid);
    return lastError;
  }

  
  
  
  SID_NAME_USE accountType;
  WCHAR accountName[UNLEN + 1] = { L'\0' };
  WCHAR domainName[DNLEN + 1] = { L'\0' };
  DWORD accountNameSize = UNLEN + 1;
  DWORD domainNameSize = DNLEN + 1;
  if (!LookupAccountSidW(nullptr, sid, accountName, 
                         &accountNameSize, 
                         domainName, &domainNameSize, &accountType)) {
    LOG_WARN(("Could not lookup account Sid, will try Users.  (%d)",
              GetLastError()));
    wcsncpy(accountName, L"Users", UNLEN);
  }

  
  FreeSid(sid);
  sid = nullptr;

  
  EXPLICIT_ACCESS ea;
  BuildExplicitAccessWithNameW(&ea, accountName, 
                              SERVICE_START | SERVICE_STOP | GENERIC_READ, 
                              SET_ACCESS, NO_INHERITANCE);
  DWORD lastError = SetEntriesInAclW(1, (PEXPLICIT_ACCESS)&ea, pacl, &pNewAcl);
  if (ERROR_SUCCESS != lastError) {
    LOG_WARN(("Could not set entries in ACL.  (%d)", lastError));
    return lastError;
  }

  
  SECURITY_DESCRIPTOR sd;
  if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
    LOG_WARN(("Could not initialize security descriptor.  (%d)",
              GetLastError()));
    return GetLastError();
  }

  
  if (!SetSecurityDescriptorDacl(&sd, TRUE, pNewAcl, FALSE)) {
    LOG_WARN(("Could not set security descriptor DACL.  (%d)",
              GetLastError()));
    return GetLastError();
  }

  
  if (!SetServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, &sd)) {
    LOG_WARN(("Could not set object security.  (%d)",
              GetLastError()));
    return GetLastError();
  }

  
  LOG(("User access was set successfully on the service."));
  return ERROR_SUCCESS;
}






































#include <windows.h>
#include <aclapi.h>
#include <stdlib.h>

#include <nsAutoPtr.h>
#include <nsWindowsHelpers.h>
#include <nsMemory.h>

#include "serviceinstall.h"
#include "servicebase.h"
#include "updatehelper.h"
#include "shellapi.h"

#pragma comment(lib, "version.lib")












static BOOL
GetVersionNumberFromPath(LPWSTR path, DWORD &A, DWORD &B, 
                         DWORD &C, DWORD &D) 
{
  DWORD fileVersionInfoSize = GetFileVersionInfoSizeW(path, 0);
  nsAutoArrayPtr<char> fileVersionInfo = new char[fileVersionInfoSize];
  if (!GetFileVersionInfoW(path, 0, fileVersionInfoSize,
                           fileVersionInfo.get())) {
      LOG(("Could not obtain file info of old service.  (%d)\n", 
           GetLastError()));
      return FALSE;
  }

  VS_FIXEDFILEINFO *fixedFileInfo = 
    reinterpret_cast<VS_FIXEDFILEINFO *>(fileVersionInfo.get());
  UINT size;
  if (!VerQueryValueW(fileVersionInfo.get(), L"\\", 
    reinterpret_cast<LPVOID*>(&fixedFileInfo), &size)) {
      LOG(("Could not query file version info of old service.  (%d)\n", 
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
SvcInstall(SvcInstallAction action)
{
  
  nsAutoServiceHandle schSCManager(OpenSCManager(NULL, NULL, 
                                                 SC_MANAGER_ALL_ACCESS));
  if (!schSCManager) {
    LOG(("Could not open service manager.  (%d)\n", GetLastError()));
    return FALSE;
  }

  WCHAR newServiceBinaryPath[MAX_PATH + 1];
  if (!GetModuleFileNameW(NULL, newServiceBinaryPath, 
                          sizeof(newServiceBinaryPath) / 
                          sizeof(newServiceBinaryPath[0]))) {
    LOG(("Could not obtain module filename when attempting to "
         "install service. (%d)\n",
         GetLastError()));
    return FALSE;
  }

  
  BOOL serviceAlreadyExists = FALSE;
  nsAutoServiceHandle schService(OpenServiceW(schSCManager, 
                                              SVC_NAME, 
                                              SERVICE_ALL_ACCESS));
  DWORD lastError = GetLastError();
  if (!schService && ERROR_SERVICE_DOES_NOT_EXIST != lastError) {
    
    LOG(("Could not open service.  (%d)\n", GetLastError()));
    return FALSE;
  }
  
  if (schService) {
    serviceAlreadyExists = TRUE;

    
    DWORD bytesNeeded;
    if (!QueryServiceConfigW(schService, NULL, 0, &bytesNeeded) && 
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      LOG(("Could not determine buffer size for query service config.  (%d)\n", 
           GetLastError()));
      return FALSE;
    }

    
    
    nsAutoArrayPtr<char> serviceConfigBuffer = new char[bytesNeeded];
    if (!QueryServiceConfigW(schService, 
        reinterpret_cast<QUERY_SERVICE_CONFIGW*>(serviceConfigBuffer.get()), 
        bytesNeeded, &bytesNeeded)) {
      LOG(("Could open service but could not query service config.  (%d)\n", 
           GetLastError()));
      return FALSE;
    }
    QUERY_SERVICE_CONFIGW &serviceConfig = 
      *reinterpret_cast<QUERY_SERVICE_CONFIGW*>(serviceConfigBuffer.get());

    
    
    
    DWORD existingA, existingB, existingC, existingD;
    DWORD newA, newB, newC, newD; 
    BOOL obtainedExistingVersionInfo = 
      GetVersionNumberFromPath(serviceConfig.lpBinaryPathName, 
                               existingA, existingB, 
                               existingC, existingD);
    if (!GetVersionNumberFromPath(newServiceBinaryPath, newA, 
                                 newB, newC, newD)) {
      LOG(("Could not obtain version number from new path\n"));
      return FALSE;
    }

    schService.reset(); 

    
    
    
    if (ForceInstallSvc == action ||
        !obtainedExistingVersionInfo || 
        (existingA < newA) ||
        (existingA == newA && existingB < newB) ||
        (existingA == newA && existingB == newB && 
         existingC < newC) ||
        (existingA == newA && existingB == newB && 
         existingC == newC && existingD < newD)) {
      if (!StopService()) {
        return FALSE;
      }

      if (!DeleteFile(serviceConfig.lpBinaryPathName)) {
        LOG(("Could not delete old service binary file.  (%d)\n", GetLastError()));
        return FALSE;
      }

      if (!CopyFile(newServiceBinaryPath, 
                    serviceConfig.lpBinaryPathName, FALSE)) {
        LOG(("Could not overwrite old service binary file. "
             "This should never happen, but if it does the next upgrade will fix"
             " it, the service is not a critical component that needs to be "
             " installed for upgrades to work. (%d)\n", 
             GetLastError()));
        return FALSE;
      }

      
      
      
      MoveFileEx(newServiceBinaryPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
      
      
      wcsncpy(newServiceBinaryPath, serviceConfig.lpBinaryPathName, MAX_PATH);
      
    } else {
      
      
      
      MoveFileEx(newServiceBinaryPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
      
      return TRUE; 
    }
  } else if (UpgradeSvc == action) {
    
    return TRUE;
  }

  if (!serviceAlreadyExists) {
    
    schService.own(CreateServiceW(schSCManager, SVC_NAME, SVC_DISPLAY_NAME,
                                  SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
                                  SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                                  newServiceBinaryPath, NULL, NULL, NULL, 
                                  NULL, NULL));
    if (!schService) {
      LOG(("Could not create Windows service. "
           "This error should never happen since a service install "
           "should only be called when elevated. (%d)\n", GetLastError()));
      return FALSE;
    } 

    if (!SetUserAccessServiceDACL(schService)) {
      LOG(("Could not set security ACE on service handle, the service will not be "
           "able to be started from unelevated processes. "
           "This error should never happen.  (%d)\n", 
           GetLastError()));
    }
  }

  return TRUE;
}






BOOL
StopService()
{
  
  nsAutoServiceHandle schSCManager(OpenSCManager(NULL, NULL, 
                                                 SC_MANAGER_ALL_ACCESS));
  if (!schSCManager) {
    LOG(("Could not open service manager.  (%d)\n", GetLastError()));
    return FALSE;
  }

  
  nsAutoServiceHandle schService(OpenServiceW(schSCManager, SVC_NAME, 
                                              SERVICE_ALL_ACCESS));
  if (!schService) {
    LOG(("Could not open service.  (%d)\n", GetLastError()));
    return FALSE;
  } 

  
  LogFinish();

  return WaitForServiceStop(SVC_NAME, 60); 
}






BOOL
SvcUninstall()
{
  
  nsAutoServiceHandle schSCManager(OpenSCManager(NULL, NULL, 
                                                 SC_MANAGER_ALL_ACCESS));
  if (!schSCManager) {
    LOG(("Could not open service manager.  (%d)\n", GetLastError()));
    return FALSE;
  }

  
  nsAutoServiceHandle schService(OpenServiceW(schSCManager, SVC_NAME, 
                                              SERVICE_ALL_ACCESS));
  if (!schService) {
    LOG(("Could not open service.  (%d)\n", GetLastError()));
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
  PACL pNewAcl = NULL;
  PSECURITY_DESCRIPTOR psd = NULL;
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
      return GetLastError();
    }

    DWORD size = needed;
    psd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, size);
    if (!psd) {
      return ERROR_INSUFFICIENT_BUFFER;
    }

    
    if (!QueryServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, 
                                    psd, size, &needed)) {
      return GetLastError();
    }
  }

  
  PACL pacl = NULL;
  BOOL bDaclPresent = FALSE;
  BOOL bDaclDefaulted = FALSE;
  if ( !GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl, 
                                  &bDaclDefaulted)) {
    return GetLastError();
  }

  
  EXPLICIT_ACCESS ea;
  BuildExplicitAccessWithName(&ea, TEXT("Users"), 
                              SERVICE_START | SERVICE_STOP | GENERIC_READ, 
                              SET_ACCESS, NO_INHERITANCE);
  DWORD lastError = SetEntriesInAcl(1, (PEXPLICIT_ACCESS)&ea, pacl, &pNewAcl);
  if (ERROR_SUCCESS != lastError) {
    return lastError;
  }

  
  SECURITY_DESCRIPTOR sd;
  if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
    return GetLastError();
  }

  
  if (!SetSecurityDescriptorDacl(&sd, TRUE, pNewAcl, FALSE)) {
    return GetLastError();
  }

  
  if (!SetServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, &sd)) {
    return GetLastError();
  }

  
  return ERROR_SUCCESS;
}

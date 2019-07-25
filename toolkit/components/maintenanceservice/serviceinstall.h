




































#include "readstrings.h"

#define SVC_DISPLAY_NAME L"Mozilla Maintenance Service"

enum SvcInstallAction { UpgradeSvc, InstallSvc, ForceInstallSvc };
BOOL SvcInstall(SvcInstallAction action);
BOOL SvcUninstall();
BOOL StopService();
BOOL SetUserAccessServiceDACL(SC_HANDLE hService);
DWORD SetUserAccessServiceDACL(SC_HANDLE hService, PACL &pNewAcl, 
                               PSECURITY_DESCRIPTOR psd);

struct MaintenanceServiceStringTable
{
  char serviceDescription[MAX_TEXT_LEN];
};


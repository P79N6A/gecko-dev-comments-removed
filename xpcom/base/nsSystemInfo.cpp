





































#include "nsSystemInfo.h"
#include "prsystem.h"
#include "nsString.h"

nsSystemInfo::nsSystemInfo()
{
}

nsSystemInfo::~nsSystemInfo()
{
}

nsresult
nsSystemInfo::Init()
{
    nsresult rv = nsHashPropertyBag::Init();
    NS_ENSURE_SUCCESS(rv, rv);

    static const struct {
      PRSysInfo cmd;
      const char *name;
    } items[] = {
      { PR_SI_SYSNAME, "name" },
      { PR_SI_HOSTNAME, "host" },
      { PR_SI_ARCHITECTURE, "arch" },
      { PR_SI_RELEASE, "version" }
    };

    for (PRUint32 i = 0; i < (sizeof(items) / sizeof(items[0])); i++) {
      char buf[SYS_INFO_BUFFER_LENGTH];
      if (PR_GetSystemInfo(items[i].cmd, buf, sizeof(buf)) == PR_SUCCESS) {
        rv = SetPropertyAsACString(NS_ConvertASCIItoUTF16(items[i].name),
                                   nsDependentCString(buf));
        NS_ENSURE_SUCCESS(rv,rv);
      }
      else
        NS_WARNING("PR_GetSystemInfo failed");
    }
   
    return NS_OK;
}


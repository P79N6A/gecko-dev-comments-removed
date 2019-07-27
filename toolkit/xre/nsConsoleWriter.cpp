



#include "nsAppRunner.h"

#include "prio.h"
#include "prprf.h"
#include "prenv.h"

#include "nsCRT.h"
#include "nsNativeCharsetUtils.h"
#include "nsString.h"
#include "nsXREDirProvider.h"
#include "nsXULAppAPI.h"

#include "nsIConsoleService.h"
#include "nsIConsoleMessage.h"

void
WriteConsoleLog()
{
  nsresult rv;

  nsCOMPtr<nsIFile> lfile;

  char* logFileEnv = PR_GetEnv("XRE_CONSOLE_LOG");
  if (logFileEnv && *logFileEnv) {
    rv = XRE_GetFileFromPath(logFileEnv, getter_AddRefs(lfile));
    if (NS_FAILED(rv))
      return;
  }
  else {
    if (!gLogConsoleErrors)
      return;

    rv = gDirServiceProvider->GetUserAppDataDirectory(getter_AddRefs(lfile));
    if (NS_FAILED(rv))
      return;

    lfile->AppendNative(NS_LITERAL_CSTRING("console.log"));
  }

  PRFileDesc *file;
  rv = lfile->OpenNSPRFileDesc(PR_WRONLY | PR_APPEND | PR_CREATE_FILE,
                               0660, &file);
  if (NS_FAILED(rv))
    return;

  nsCOMPtr<nsIConsoleService> csrv
    (do_GetService(NS_CONSOLESERVICE_CONTRACTID));
  if (!csrv) {
    PR_Close(file);
    return;
  }

  nsIConsoleMessage** messages;
  uint32_t mcount;

  rv = csrv->GetMessageArray(&mcount, &messages);
  if (NS_FAILED(rv)) {
    PR_Close(file);
    return;
  }

  if (mcount) {
    PRExplodedTime etime;
    PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &etime);
    char datetime[512];
    PR_FormatTimeUSEnglish(datetime, sizeof(datetime),
                           "%Y-%m-%d %H:%M:%S", &etime);

    PR_fprintf(file, NS_LINEBREAK
                     "*** Console log: %s ***" NS_LINEBREAK,
               datetime);
  }

  
  

  nsXPIDLString msg;
  nsAutoCString nativemsg;

  for (uint32_t i = 0; i < mcount; ++i) {
    rv = messages[i]->GetMessageMoz(getter_Copies(msg));
    if (NS_SUCCEEDED(rv)) {
      NS_CopyUnicodeToNative(msg, nativemsg);
      PR_fprintf(file, "%s" NS_LINEBREAK, nativemsg.get());
    }
    NS_IF_RELEASE(messages[i]);
  }

  PR_Close(file);
  free(messages);
}







































#include "nsXULAppAPI.h"
#ifdef XP_WIN
#include <windows.h>
#include <stdlib.h>
#endif

#include <stdio.h>
#include <stdarg.h>

#include "nsCOMPtr.h"
#include "nsILocalFile.h"
#include "nsStringGlue.h"

static void Output(const char *fmt, ... )
{
  va_list ap;
  va_start(ap, fmt);

#if defined(XP_WIN) && !MOZ_WINCONSOLE
  char msg[2048];

  vsnprintf(msg, sizeof(msg), fmt, ap);

  MessageBox(NULL, msg, "XULRunner", MB_OK | MB_ICONERROR);
#else
  vfprintf(stderr, fmt, ap);
#endif

  va_end(ap);
}

int main(int argc, char* argv[])
{
  nsCOMPtr<nsILocalFile> appini;
  nsresult rv = XRE_GetBinaryPath(argv[0], getter_AddRefs(appini));
  if (NS_FAILED(rv)) {
    Output("Couldn't calculate the application directory.");
    return 255;
  }
  appini->SetNativeLeafName(NS_LITERAL_CSTRING("application.ini"));

  nsXREAppData *appData;
  rv = XRE_CreateAppData(appini, &appData);
  if (NS_FAILED(rv)) {
    Output("Couldn't read application.ini");
    return 255;
  }

  int result = XRE_main(argc, argv, appData);
  XRE_FreeAppData(appData);
  return result;
}

#if defined( XP_WIN ) && defined( WIN32 ) && !defined(__GNUC__)


int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR args, int )
{
    
    return main( __argc, __argv );
}
#endif







































#include "nsXULAppAPI.h"
#ifdef XP_WIN
#include <windows.h>
#include <stdlib.h>
#endif
#include "nsBuildID.h"

static const nsXREAppData kAppData = {
  sizeof(nsXREAppData),
  nsnull,
  "Mozilla",
  "Firefox",
  NS_STRINGIFY(APP_VERSION),
  NS_STRINGIFY(BUILD_ID),
  "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}",
  "Copyright (c) 1998 - 2007 mozilla.org",
  NS_XRE_ENABLE_PROFILE_MIGRATOR |
  NS_XRE_ENABLE_EXTENSION_MANAGER
#if defined(MOZILLA_OFFICIAL) && (defined(XP_WIN) || defined(XP_MACOSX))
  | NS_XRE_ENABLE_CRASH_REPORTER
#endif
,
  nsnull, 
  nsnull, 
  nsnull, 
  "https://crash-reports.mozilla.com/submit"
};

int main(int argc, char* argv[])
{
  return XRE_main(argc, argv, &kAppData);
}

#if defined( XP_WIN ) && defined( WIN32 ) && !defined(__GNUC__)


int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR args, int )
{
    
    return main( __argc, __argv );
}
#endif






































#include <stdio.h>
#include <stdlib.h>
#ifdef XP_WIN
#include <windows.h>
#endif

#include "nsXULAppAPI.h"
#include "nsXPCOMGlue.h"
#include "nsRegisterGRE.h"
#include "nsAppRunner.h"
#include "nsILocalFile.h"
#include "nsIXULAppInstall.h"
#include "nsCOMPtr.h"
#include "nsMemory.h"
#include "nsCRTGlue.h"
#include "nsStringAPI.h"
#include "nsServiceManagerUtils.h"
#include "plstr.h"
#include "prprf.h"
#include "prenv.h"
#include "nsINIParser.h"










static void Output(PRBool isError, const char *fmt, ... )
{
  va_list ap;
  va_start(ap, fmt);

#if defined(XP_WIN) && !MOZ_WINCONSOLE
  char *msg = PR_vsmprintf(fmt, ap);
  if (msg)
  {
    UINT flags = MB_OK;
    if (isError)
      flags |= MB_ICONERROR;
    else
      flags |= MB_ICONINFORMATION;
    MessageBox(NULL, msg, "XULRunner", flags);
    PR_smprintf_free(msg);
  }
#else
  vfprintf(stderr, fmt, ap);
#endif

  va_end(ap);
}




static PRBool IsArg(const char* arg, const char* s)
{
  if (*arg == '-')
  {
    if (*++arg == '-')
      ++arg;
    return !PL_strcasecmp(arg, s);
  }

#if defined(XP_WIN) || defined(XP_OS2)
  if (*arg == '/')
    return !PL_strcasecmp(++arg, s);
#endif

  return PR_FALSE;
}

static nsresult
GetGREVersion(const char *argv0,
              nsACString *aMilestone,
              nsACString *aVersion)
{
  if (aMilestone)
    aMilestone->Assign("<Error>");
  if (aVersion)
    aVersion->Assign("<Error>");

  nsCOMPtr<nsILocalFile> iniFile;
  nsresult rv = XRE_GetBinaryPath(argv0, getter_AddRefs(iniFile));
  if (NS_FAILED(rv))
    return rv;

  iniFile->SetNativeLeafName(NS_LITERAL_CSTRING("platform.ini"));

  nsINIParser parser;
  rv = parser.Init(iniFile);
  if (NS_FAILED(rv))
    return rv;

  if (aMilestone) {
    rv = parser.GetString("Build", "Milestone", *aMilestone);
    if (NS_FAILED(rv))
      return rv;
  }
  if (aVersion) {
    rv = parser.GetString("Build", "BuildID", *aVersion);
    if (NS_FAILED(rv))
      return rv;
  }
  return NS_OK;
}

static void Usage(const char *argv0)
{
    nsCAutoString milestone;
    GetGREVersion(argv0, &milestone, nsnull);

    
    Output(PR_FALSE,
           "Mozilla XULRunner %s\n\n"
           "Usage: " XULRUNNER_PROGNAME " [OPTIONS]\n"
           "       " XULRUNNER_PROGNAME " APP-FILE [APP-OPTIONS...]\n"
           "\n"
           "OPTIONS\n"
           "      --app                  specify APP-FILE (optional)\n"
           "  -h, --help                 show this message\n"
           "  -v, --version              show version\n"
           "  --gre-version              print the GRE version string on stdout\n"
           "  --register-global          register this GRE in the machine registry\n"
           "  --register-user            register this GRE in the user registry\n"
           "  --unregister-global        unregister this GRE formerly registered with\n"
           "                             --register-global\n"
           "  --unregister-user          unregister this GRE formely registered with\n"
           "                             --register-user\n"
           "  --find-gre <version>       Find a GRE with version <version> and print\n"
           "                             the path on stdout\n"
           "  --install-app <application> [<destination> [<directoryname>]]\n"
           "                             Install a XUL application.\n"
           "\n"
           "APP-FILE\n"
           "  Application initialization file.\n"
           "\n"
           "APP-OPTIONS\n"
           "  Application specific options.\n",
           milestone.get());
}

static nsresult
GetXULRunnerDir(const char *argv0, nsIFile* *aResult)
{
  nsresult rv;

  nsCOMPtr<nsILocalFile> appFile;
  rv = XRE_GetBinaryPath(argv0, getter_AddRefs(appFile));
  if (NS_FAILED(rv)) {
    Output(PR_TRUE, "Could not find XULRunner application path.\n");
    return rv;
  }

  rv = appFile->GetParent(aResult);
  if (NS_FAILED(rv)) {
    Output(PR_TRUE, "Could not find XULRunner installation dir.\n");
  }
  return rv;
}

static int
InstallXULApp(nsIFile* aXULRunnerDir,
              const char *aAppLocation,
              const char *aInstallTo,
              const char *aLeafName)
{
  nsCOMPtr<nsILocalFile> appLocation;
  nsCOMPtr<nsILocalFile> installTo;
  nsString leafName;

  nsresult rv = XRE_GetFileFromPath(aAppLocation, getter_AddRefs(appLocation));
  if (NS_FAILED(rv))
    return 2;

  if (aInstallTo) {
    rv = XRE_GetFileFromPath(aInstallTo, getter_AddRefs(installTo));
    if (NS_FAILED(rv))
      return 2;
  }

  if (aLeafName)
    NS_CStringToUTF16(nsDependentCString(aLeafName),
                      NS_CSTRING_ENCODING_NATIVE_FILESYSTEM, leafName);

  rv = NS_InitXPCOM2(nsnull, aXULRunnerDir, nsnull);
  if (NS_FAILED(rv))
    return 3;

  {
    
    nsCOMPtr<nsIXULAppInstall> install
      (do_GetService("@mozilla.org/xulrunner/app-install-service;1"));
    if (!install) {
      rv = NS_ERROR_FAILURE;
    }
    else {
      rv = install->InstallApplication(appLocation, installTo, leafName);
    }
  }

  NS_ShutdownXPCOM(nsnull);

  if (NS_FAILED(rv))
    return 3;

  return 0;
}

static const GREProperty kGREProperties[] = {
  { "xulrunner", "true" }
#ifdef MOZ_JAVAXPCOM
  , { "javaxpcom", "1" }
#endif
};

class AutoAppData
{
public:
  AutoAppData(nsILocalFile* aINIFile) : mAppData(nsnull) {
    nsresult rv = XRE_CreateAppData(aINIFile, &mAppData);
    if (NS_FAILED(rv))
      mAppData = nsnull;
  }
  ~AutoAppData() {
    if (mAppData)
      XRE_FreeAppData(mAppData);
  }

  nsresult
  Override(nsILocalFile* aINIFile) {
    return XRE_ParseAppData(aINIFile, mAppData);
  }

  operator nsXREAppData*() const { return mAppData; }
  nsXREAppData* operator -> () const { return mAppData; }

private:
  nsXREAppData* mAppData;
};

int main(int argc, char* argv[])
{
  if (argc > 1 && (IsArg(argv[1], "h") ||
                   IsArg(argv[1], "help") ||
                   IsArg(argv[1], "?")))
  {
    Usage(argv[0]);
    return 0;
  }

  if (argc == 2 && (IsArg(argv[1], "v") || IsArg(argv[1], "version")))
  {
    nsCAutoString milestone;
    nsCAutoString version;
    GetGREVersion(argv[0], &milestone, &version);
    Output(PR_FALSE, "Mozilla XULRunner %s - %s\n",
           milestone.get(), version.get());
    return 0;
  }

  if (argc > 1) {
    nsCAutoString milestone;
    nsresult rv = GetGREVersion(argv[0], &milestone, nsnull);
    if (NS_FAILED(rv))
      return 2;

    PRBool registerGlobal = IsArg(argv[1], "register-global");
    PRBool registerUser   = IsArg(argv[1], "register-user");
    if (registerGlobal || registerUser) {
      if (argc != 2) {
        Usage(argv[0]);
        return 1;
      }

      nsCOMPtr<nsIFile> regDir;
      rv = GetXULRunnerDir(argv[0], getter_AddRefs(regDir));
      if (NS_FAILED(rv))
        return 2;

      return RegisterXULRunner(registerGlobal, regDir,
                               kGREProperties,
                               NS_ARRAY_LENGTH(kGREProperties),
                               milestone.get()) ? 0 : 2;
    }

    registerGlobal = IsArg(argv[1], "unregister-global");
    registerUser   = IsArg(argv[1], "unregister-user");
    if (registerGlobal || registerUser) {
      if (argc != 2) {
        Usage(argv[0]);
        return 1;
      }

      nsCOMPtr<nsIFile> regDir;
      rv = GetXULRunnerDir(argv[0], getter_AddRefs(regDir));
      if (NS_FAILED(rv))
        return 2;

      UnregisterXULRunner(registerGlobal, regDir, milestone.get());
      return 0;
    }

    if (IsArg(argv[1], "find-gre")) {
      if (argc != 3) {
        Usage(argv[0]);
        return 1;
      }

      char path[MAXPATHLEN];
      static const GREVersionRange vr = {
        argv[2], PR_TRUE,
        argv[2], PR_TRUE
      };
      static const GREProperty kProperties[] = {
        { "xulrunner", "true" }
      };

      rv = GRE_GetGREPathWithProperties(&vr, 1, kProperties,
                                        NS_ARRAY_LENGTH(kProperties),
                                        path, sizeof(path));
      if (NS_FAILED(rv))
        return 1;

      printf("%s\n", path);
      return 0;
    }

    if (IsArg(argv[1], "gre-version")) {
      if (argc != 2) {
        Usage(argv[0]);
        return 1;
      }

      printf("%s\n", milestone.get());
      return 0;
    }

    if (IsArg(argv[1], "install-app")) {
      if (argc < 3 || argc > 5) {
        Usage(argv[0]);
        return 1;
      }

      char *appLocation = argv[2];

      char *installTo = nsnull;
      if (argc > 3) {
        installTo = argv[3];
        if (!*installTo) 
          installTo = nsnull;
      }

      char *leafName = nsnull;
      if (argc > 4) {
        leafName = argv[4];
        if (!*leafName)
          leafName = nsnull;
      }

      nsCOMPtr<nsIFile> regDir;
      rv = GetXULRunnerDir(argv[0], getter_AddRefs(regDir));
      if (NS_FAILED(rv))
        return 2;

      return InstallXULApp(regDir, appLocation, installTo, leafName);
    }
  }

  const char *appDataFile = PR_GetEnv("XUL_APP_FILE");

  if (!(appDataFile && *appDataFile)) {
    if (argc < 2) {
      Usage(argv[0]);
      return 1;
    }

    if (IsArg(argv[1], "app")) {
      if (argc == 2) {
        Usage(argv[0]);
        return 1;
      }
      argv[1] = argv[0];
      ++argv;
      --argc;
    }

    appDataFile = argv[1];
    argv[1] = argv[0];
    ++argv;
    --argc;

    static char kAppEnv[MAXPATHLEN];
    PR_snprintf(kAppEnv, MAXPATHLEN, "XUL_APP_FILE=%s", appDataFile);
    PR_SetEnv(kAppEnv);
  }

  nsCOMPtr<nsILocalFile> appDataLF;
  nsresult rv = XRE_GetFileFromPath(appDataFile, getter_AddRefs(appDataLF));
  if (NS_FAILED(rv)) {
    Output(PR_TRUE, "Error: unrecognized application.ini path.\n");
    return 2;
  }

  AutoAppData appData(appDataLF);
  if (!appData) {
    Output(PR_TRUE, "Error: couldn't parse application.ini.\n");
    return 2;
  }

  if (argc > 1 && IsArg(argv[1], "override")) {
    if (argc == 2) {
      Usage(argv[0]);
      return 1;
    }
    argv[1] = argv[0];
    ++argv;
    --argc;

    const char *ovrDataFile = argv[1];
    argv[1] = argv[0];
    ++argv;
    --argc;

    nsCOMPtr<nsILocalFile> ovrDataLF;
    nsresult rv = XRE_GetFileFromPath(ovrDataFile, getter_AddRefs(ovrDataLF));
    if (NS_FAILED(rv)) {
      Output(PR_TRUE, "Error: unrecognized override.ini path.\n");
      return 2;
    }

    appData.Override(ovrDataLF);
  }

  return XRE_main(argc, argv, appData);
}

#if defined( XP_WIN ) && defined( WIN32 ) && !defined(__GNUC__)


int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR args, int )
{
  
  return main( __argc, __argv );
}
#endif

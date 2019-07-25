









































#include "mozilla/Util.h"

#include "nsXRemoteService.h"
#include "nsIObserverService.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsICommandLineRunner.h"
#include "nsICommandLine.h"

#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsILocalFile.h"
#include "nsIServiceManager.h"
#include "nsIWeakReference.h"
#include "nsIWidget.h"
#include "nsIAppShellService.h"
#include "nsAppShellCID.h"
#include "nsPIDOMWindow.h"
#include "mozilla/X11Util.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "prprf.h"
#include "prenv.h"
#include "nsCRT.h"

#include "nsXULAppAPI.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

using namespace mozilla;

#define MOZILLA_VERSION_PROP   "_MOZILLA_VERSION"
#define MOZILLA_LOCK_PROP      "_MOZILLA_LOCK"
#define MOZILLA_COMMAND_PROP   "_MOZILLA_COMMAND"
#define MOZILLA_RESPONSE_PROP  "_MOZILLA_RESPONSE"
#define MOZILLA_USER_PROP      "_MOZILLA_USER"
#define MOZILLA_PROFILE_PROP   "_MOZILLA_PROFILE"
#define MOZILLA_PROGRAM_PROP   "_MOZILLA_PROGRAM"
#define MOZILLA_COMMANDLINE_PROP "_MOZILLA_COMMANDLINE"

const unsigned char kRemoteVersion[] = "5.1";

#ifdef IS_BIG_ENDIAN
#define TO_LITTLE_ENDIAN32(x) \
    ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | \
    (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))
#else
#define TO_LITTLE_ENDIAN32(x) (x)
#endif


static char *XAtomNames[] = {
  MOZILLA_VERSION_PROP,
  MOZILLA_LOCK_PROP,
  MOZILLA_COMMAND_PROP,
  MOZILLA_RESPONSE_PROP,
  MOZILLA_USER_PROP,
  MOZILLA_PROFILE_PROP,
  MOZILLA_PROGRAM_PROP,
  MOZILLA_COMMANDLINE_PROP
};
static Atom XAtoms[NS_ARRAY_LENGTH(XAtomNames)];

Atom nsXRemoteService::sMozVersionAtom;
Atom nsXRemoteService::sMozLockAtom;
Atom nsXRemoteService::sMozCommandAtom;
Atom nsXRemoteService::sMozResponseAtom;
Atom nsXRemoteService::sMozUserAtom;
Atom nsXRemoteService::sMozProfileAtom;
Atom nsXRemoteService::sMozProgramAtom;
Atom nsXRemoteService::sMozCommandLineAtom;

nsXRemoteService * nsXRemoteService::sRemoteImplementation = 0;


static bool
FindExtensionParameterInCommand(const char* aParameterName,
                                const nsACString& aCommand,
                                char aSeparator,
                                nsACString* aValue)
{
  nsCAutoString searchFor;
  searchFor.Append(aSeparator);
  searchFor.Append(aParameterName);
  searchFor.Append('=');

  nsACString::const_iterator start, end;
  aCommand.BeginReading(start);
  aCommand.EndReading(end);
  if (!FindInReadable(searchFor, start, end))
    return PR_FALSE;

  nsACString::const_iterator charStart, charEnd;
  charStart = end;
  aCommand.EndReading(charEnd);
  nsACString::const_iterator idStart = charStart, idEnd;
  if (FindCharInReadable(aSeparator, charStart, charEnd)) {
    idEnd = charStart;
  } else {
    idEnd = charEnd;
  }
  *aValue = nsDependentCSubstring(idStart, idEnd);
  return PR_TRUE;
}


nsXRemoteService::nsXRemoteService()
{    
}

void
nsXRemoteService::XRemoteBaseStartup(const char *aAppName, const char *aProfileName)
{
    EnsureAtoms();

    mAppName = aAppName;
    ToLowerCase(mAppName);

    mProfileName = aProfileName;

    nsCOMPtr<nsIObserverService> obs(do_GetService("@mozilla.org/observer-service;1"));
    if (obs) {
      obs->AddObserver(this, "xpcom-shutdown", PR_FALSE);
      obs->AddObserver(this, "quit-application", PR_FALSE);
    }
}

void 
nsXRemoteService::HandleCommandsFor(Window aWindowId)
{
  
  XChangeProperty(mozilla::DefaultXDisplay(), aWindowId, sMozVersionAtom, XA_STRING,
                  8, PropModeReplace, kRemoteVersion, sizeof(kRemoteVersion) - 1);

  
  unsigned char *logname;
  logname = (unsigned char*) PR_GetEnv("LOGNAME");
  if (logname) {
    
    XChangeProperty(mozilla::DefaultXDisplay(), aWindowId, sMozUserAtom, XA_STRING,
                    8, PropModeReplace, logname, strlen((char*) logname));
  }

  XChangeProperty(mozilla::DefaultXDisplay(), aWindowId, sMozProgramAtom, XA_STRING,
                  8, PropModeReplace, (unsigned char*) mAppName.get(), mAppName.Length());

  if (!mProfileName.IsEmpty()) {
    XChangeProperty(mozilla::DefaultXDisplay(),
                    aWindowId, sMozProfileAtom, XA_STRING,
                    8, PropModeReplace,
                    (unsigned char*) mProfileName.get(), mProfileName.Length());
  }

}

NS_IMETHODIMP
nsXRemoteService::Observe(nsISupports* aSubject,
                          const char *aTopic,
                          const PRUnichar *aData)
{
  
  
  Shutdown();
  return NS_OK;
}

bool
nsXRemoteService::HandleNewProperty(XID aWindowId, Display* aDisplay,
                                    Time aEventTime,
                                    Atom aChangedAtom,
                                    nsIWeakReference* aDomWindow)
{

  nsCOMPtr<nsIDOMWindow> window (do_QueryReferent(aDomWindow));

  if (aChangedAtom == sMozCommandAtom || aChangedAtom == sMozCommandLineAtom) {
    
    int result;
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    char *data = 0;

    result = XGetWindowProperty (aDisplay,
                                 aWindowId,
                                 aChangedAtom,
                                 0,                        
                                 (65536 / sizeof (long)),  
                                 True,                     
                                 XA_STRING,                
                                 &actual_type,             
                                 &actual_format,           
                                 &nitems,                  
                                 &bytes_after,             
                                 (unsigned char **)&data); 



    
    if (result != Success)
      return PR_FALSE;

    
    if (!data || !TO_LITTLE_ENDIAN32(*reinterpret_cast<PRInt32*>(data)))
      return PR_FALSE;

    
    const char *response = NULL;
    if (aChangedAtom == sMozCommandAtom)
      response = HandleCommand(data, window, aEventTime);
    else if (aChangedAtom == sMozCommandLineAtom)
      response = HandleCommandLine(data, window, aEventTime);

    
    XChangeProperty (aDisplay, aWindowId,
                     sMozResponseAtom, XA_STRING,
                     8, PropModeReplace,
                     (const unsigned char *)response,
                     strlen (response));
    XFree(data);
    return PR_TRUE;
  }

  else if (aChangedAtom == sMozResponseAtom) {
    
    return PR_TRUE;
  }

  else if (aChangedAtom == sMozLockAtom) {
    
    return PR_TRUE;
  }

  return PR_FALSE;
}

const char*
nsXRemoteService::HandleCommand(char* aCommand, nsIDOMWindow* aWindow,
                                PRUint32 aTimestamp)
{
  nsresult rv;

  nsCOMPtr<nsICommandLineRunner> cmdline
    (do_CreateInstance("@mozilla.org/toolkit/command-line;1", &rv));
  if (NS_FAILED(rv))
    return "509 internal error";

  
  

  nsCAutoString command(aCommand);
  PRInt32 p1, p2;
  p1 = command.FindChar('(');
  p2 = command.FindChar(')');

  if (p1 == kNotFound || p2 == kNotFound || p1 == 0 || p2 < p1) {
    return "500 command not parseable";
  }

  command.Truncate(p1);
  command.Trim(" ", PR_TRUE, PR_TRUE);
  ToLowerCase(command);

  if (!command.EqualsLiteral("ping")) {
    nsCAutoString desktopStartupID;
    nsDependentCString cmd(aCommand);
    FindExtensionParameterInCommand("DESKTOP_STARTUP_ID",
                                    cmd, '\n',
                                    &desktopStartupID);

    char* argv[3] = {"dummyappname", "-remote", aCommand};
    rv = cmdline->Init(3, argv, nsnull, nsICommandLine::STATE_REMOTE_EXPLICIT);
    if (NS_FAILED(rv))
      return "509 internal error";

    if (aWindow)
      cmdline->SetWindowContext(aWindow);

    if (sRemoteImplementation)
      sRemoteImplementation->SetDesktopStartupIDOrTimestamp(desktopStartupID, aTimestamp);

    rv = cmdline->Run();
    if (NS_ERROR_ABORT == rv)
      return "500 command not parseable";
    if (NS_FAILED(rv))
      return "509 internal error";
  }

  return "200 executed command";
}

const char*
nsXRemoteService::HandleCommandLine(char* aBuffer, nsIDOMWindow* aWindow,
                                    PRUint32 aTimestamp)
{
  nsresult rv;

  nsCOMPtr<nsICommandLineRunner> cmdline
    (do_CreateInstance("@mozilla.org/toolkit/command-line;1", &rv));
  if (NS_FAILED(rv))
    return "509 internal error";

  
  
  
  
  

  PRInt32 argc = TO_LITTLE_ENDIAN32(*reinterpret_cast<PRInt32*>(aBuffer));
  char *wd   = aBuffer + ((argc + 1) * sizeof(PRInt32));

  nsCOMPtr<nsILocalFile> lf;
  rv = NS_NewNativeLocalFile(nsDependentCString(wd), PR_TRUE,
                             getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return "509 internal error";

  nsCAutoString desktopStartupID;

  char **argv = (char**) malloc(sizeof(char*) * argc);
  if (!argv) return "509 internal error";

  PRInt32  *offset = reinterpret_cast<PRInt32*>(aBuffer) + 1;

  for (int i = 0; i < argc; ++i) {
    argv[i] = aBuffer + TO_LITTLE_ENDIAN32(offset[i]);

    if (i == 0) {
      nsDependentCString cmd(argv[0]);
      FindExtensionParameterInCommand("DESKTOP_STARTUP_ID",
                                      cmd, ' ',
                                      &desktopStartupID);
    }
  }

  rv = cmdline->Init(argc, argv, lf, nsICommandLine::STATE_REMOTE_AUTO);

  free (argv);
  if (NS_FAILED(rv)) {
    return "509 internal error";
  }

  if (aWindow)
    cmdline->SetWindowContext(aWindow);

  if (sRemoteImplementation)
    sRemoteImplementation->SetDesktopStartupIDOrTimestamp(desktopStartupID, aTimestamp);

  rv = cmdline->Run();

  if (NS_ERROR_ABORT == rv)
    return "500 command not parseable";

  if (NS_FAILED(rv))
    return "509 internal error";

  return "200 executed command";
}

void
nsXRemoteService::EnsureAtoms(void)
{
  if (sMozVersionAtom)
    return;

  XInternAtoms(mozilla::DefaultXDisplay(), XAtomNames, ArrayLength(XAtomNames),
               False, XAtoms);

  int i = 0;
  sMozVersionAtom     = XAtoms[i++];
  sMozLockAtom        = XAtoms[i++];
  sMozCommandAtom     = XAtoms[i++];
  sMozResponseAtom    = XAtoms[i++];
  sMozUserAtom        = XAtoms[i++];
  sMozProfileAtom     = XAtoms[i++];
  sMozProgramAtom     = XAtoms[i++];
  sMozCommandLineAtom = XAtoms[i++];
}










































#include "nsGTKRemoteService.h"

#include <X11/Xatom.h> 
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsPIDOMWindow.h"
#include "nsIGenericFactory.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsIWeakReference.h"
#include "nsIWidget.h"
#include "nsIAppShellService.h"
#include "nsAppShellCID.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "prprf.h"
#include "prenv.h"
#include "nsCRT.h"

#ifdef MOZ_WIDGET_GTK2
#include "nsGTKToolkit.h"
#endif

#include "nsICommandLineRunner.h"
#include "nsXULAppAPI.h"

#define MOZILLA_VERSION_PROP   "_MOZILLA_VERSION"
#define MOZILLA_LOCK_PROP      "_MOZILLA_LOCK"
#define MOZILLA_COMMAND_PROP   "_MOZILLA_COMMAND"
#define MOZILLA_RESPONSE_PROP  "_MOZILLA_RESPONSE"
#define MOZILLA_USER_PROP      "_MOZILLA_USER"
#define MOZILLA_PROFILE_PROP   "_MOZILLA_PROFILE"
#define MOZILLA_PROGRAM_PROP   "_MOZILLA_PROGRAM"
#define MOZILLA_COMMANDLINE_PROP "_MOZILLA_COMMANDLINE"

#ifdef IS_BIG_ENDIAN
#define TO_LITTLE_ENDIAN32(x) \
    ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | \
    (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))
#else
#define TO_LITTLE_ENDIAN32(x) (x)
#endif

const unsigned char kRemoteVersion[] = "5.1";

NS_IMPL_ISUPPORTS2(nsGTKRemoteService,
                   nsIRemoteService,
                   nsIObserver)

NS_IMETHODIMP
nsGTKRemoteService::Startup(const char* aAppName, const char* aProfileName)
{
  NS_ASSERTION(aAppName, "Don't pass a null appname!");

  EnsureAtoms();
  if (mServerWindow) return NS_ERROR_ALREADY_INITIALIZED;

  mAppName = aAppName;
  ToLowerCase(mAppName);

  mProfileName = aProfileName;

  mServerWindow = gtk_invisible_new();
  gtk_widget_realize(mServerWindow);
  HandleCommandsFor(mServerWindow, nsnull);

  if (!mWindows.IsInitialized())
    mWindows.Init();

  mWindows.EnumerateRead(StartupHandler, this);

  nsCOMPtr<nsIObserverService> obs
    (do_GetService("@mozilla.org/observer-service;1"));
  if (obs) {
    obs->AddObserver(this, "xpcom-shutdown", PR_FALSE);
    obs->AddObserver(this, "quit-application", PR_FALSE);
  }

  return NS_OK;
}

PLDHashOperator
nsGTKRemoteService::StartupHandler(const void* aKey,
                                   nsIWeakReference* aData,
                                   void* aClosure)
{
  GtkWidget* widget = (GtkWidget*) aKey;
  nsGTKRemoteService* aThis = (nsGTKRemoteService*) aClosure;

  aThis->HandleCommandsFor(widget, aData);
  return PL_DHASH_NEXT;
}

static nsIWidget* GetMainWidget(nsIDOMWindow* aWindow)
{
  
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));
  NS_ENSURE_TRUE(window, nsnull);

  nsCOMPtr<nsIBaseWindow> baseWindow
    (do_QueryInterface(window->GetDocShell()));
  NS_ENSURE_TRUE(baseWindow, nsnull);

  nsCOMPtr<nsIWidget> mainWidget;
  baseWindow->GetMainWidget(getter_AddRefs(mainWidget));
  return mainWidget;
}

#ifdef MOZ_WIDGET_GTK2
static nsGTKToolkit* GetGTKToolkit()
{
  nsCOMPtr<nsIAppShellService> svc = do_GetService(NS_APPSHELLSERVICE_CONTRACTID);
  if (!svc)
    return nsnull;
  nsCOMPtr<nsIDOMWindowInternal> window;
  svc->GetHiddenDOMWindow(getter_AddRefs(window));
  if (!window)
    return nsnull;
  nsIWidget* widget = GetMainWidget(window);
  if (!widget)
    return nsnull;
  nsIToolkit* toolkit = widget->GetToolkit();
  if (!toolkit)
    return nsnull;
  return static_cast<nsGTKToolkit*>(toolkit);
}
#endif

NS_IMETHODIMP
nsGTKRemoteService::RegisterWindow(nsIDOMWindow* aWindow)
{
  nsIWidget* mainWidget = GetMainWidget(aWindow);
  NS_ENSURE_TRUE(mainWidget, NS_ERROR_FAILURE);

  
  

  nsIWidget* tempWidget = mainWidget->GetParent();

  while (tempWidget) {
    tempWidget = tempWidget->GetParent();
    if (tempWidget)
      mainWidget = tempWidget;
  }

  GtkWidget* widget =
    (GtkWidget*) mainWidget->GetNativeData(NS_NATIVE_SHELLWIDGET);
  NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

  nsCOMPtr<nsIWeakReference> weak = do_GetWeakReference(aWindow);
  NS_ENSURE_TRUE(weak, NS_ERROR_FAILURE);

  if (!mWindows.IsInitialized())
    mWindows.Init();

  mWindows.Put(widget, weak);

  
  if (mServerWindow) {
    HandleCommandsFor(widget, weak);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGTKRemoteService::Shutdown()
{
  if (!mServerWindow)
    return NS_ERROR_NOT_INITIALIZED;

  gtk_widget_destroy(mServerWindow);
  mServerWindow = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsGTKRemoteService::Observe(nsISupports* aSubject,
                            const char *aTopic,
                            const PRUnichar *aData)
{
  
  
  Shutdown();
  return NS_OK;
}


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

void
nsGTKRemoteService::EnsureAtoms(void)
{
  if (sMozVersionAtom)
    return;

  XInternAtoms(GDK_DISPLAY(), XAtomNames, NS_ARRAY_LENGTH(XAtomNames),
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






static void
SetDesktopStartupIDOrTimestamp(const nsACString& aDesktopStartupID,
                               PRUint32 aTimestamp) {
#ifdef MOZ_WIDGET_GTK2
  nsGTKToolkit* toolkit = GetGTKToolkit();
  if (!toolkit)
    return;
  if (!aDesktopStartupID.IsEmpty()) {
    toolkit->SetDesktopStartupID(aDesktopStartupID);
  } else {
    toolkit->SetFocusTimestamp(aTimestamp);
  }
#endif
}

static PRBool
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

const char*
nsGTKRemoteService::HandleCommand(char* aCommand, nsIDOMWindow* aWindow,
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

#ifdef DEBUG_bsmedberg
  printf("Processing xremote command: %s\n", command.get());
#endif

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

    SetDesktopStartupIDOrTimestamp(desktopStartupID, aTimestamp);

    rv = cmdline->Run();
    if (NS_ERROR_ABORT == rv)
      return "500 command not parseable";
    if (NS_FAILED(rv))
      return "509 internal error";
  }

  return "200 executed command";
}

const char*
nsGTKRemoteService::HandleCommandLine(char* aBuffer, nsIDOMWindow* aWindow,
                                      PRUint32 aTimestamp)
{
  nsresult rv;

  nsCOMPtr<nsICommandLineRunner> cmdline
    (do_CreateInstance("@mozilla.org/toolkit/command-line;1", &rv));
  if (NS_FAILED(rv))
    return "509 internal error";

  
  
  
  
  

  PRInt32 argc = TO_LITTLE_ENDIAN32(*reinterpret_cast<PRInt32*>(aBuffer));
  char *wd   = aBuffer + ((argc + 1) * sizeof(PRInt32));

#ifdef DEBUG_bsmedberg
  printf("Receiving command line:\n"
         "  wd:\t%s\n"
         "  argc:\t%i\n",
         wd, argc);
#endif

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
#ifdef DEBUG_bsmedberg
    printf("  argv[%i]:\t%s\n", i, argv[i]);
#endif
  }

  rv = cmdline->Init(argc, argv, lf, nsICommandLine::STATE_REMOTE_AUTO);
  free (argv);
  if (NS_FAILED(rv)) {
    return "509 internal error";
  }

  if (aWindow)
    cmdline->SetWindowContext(aWindow);

  SetDesktopStartupIDOrTimestamp(desktopStartupID, aTimestamp);

  rv = cmdline->Run();
  
  if (NS_ERROR_ABORT == rv)
    return "500 command not parseable";
  
  if (NS_FAILED(rv))
    return "509 internal error";

  return "200 executed command";
}

void
nsGTKRemoteService::HandleCommandsFor(GtkWidget* widget,
                                      nsIWeakReference* aWindow)
{
#ifdef MOZ_WIDGET_GTK2
  g_signal_connect(G_OBJECT(widget), "property_notify_event",
                   G_CALLBACK(HandlePropertyChange), aWindow);
#else 
  gtk_signal_connect(GTK_OBJECT(widget), "property_notify_event",
                     GTK_SIGNAL_FUNC(HandlePropertyChange), aWindow);
#endif

  gtk_widget_add_events(widget, GDK_PROPERTY_CHANGE_MASK);

  Window window = GDK_WINDOW_XWINDOW(widget->window);

  
  XChangeProperty(GDK_DISPLAY(), window, sMozVersionAtom, XA_STRING,
                  8, PropModeReplace, kRemoteVersion, sizeof(kRemoteVersion) - 1);

  
  unsigned char *logname;
  logname = (unsigned char*) PR_GetEnv("LOGNAME");
  if (logname) {
    
    XChangeProperty(GDK_DISPLAY(), window, sMozUserAtom, XA_STRING,
                    8, PropModeReplace, logname, strlen((char*) logname));
  }

  XChangeProperty(GDK_DISPLAY(), window, sMozProgramAtom, XA_STRING,
                  8, PropModeReplace, (unsigned char*) mAppName.get(), mAppName.Length());

  if (!mProfileName.IsEmpty()) {
    XChangeProperty(GDK_DISPLAY(), window, sMozProfileAtom, XA_STRING,
                    8, PropModeReplace, (unsigned char*) mProfileName.get(), mProfileName.Length());
  }
}

#ifdef MOZ_WIDGET_GTK2
#define CMP_GATOM_XATOM(gatom,xatom) (gatom == gdk_x11_xatom_to_atom(xatom))
#else
#define CMP_GATOM_XATOM(gatom,xatom) (gatom == xatom)
#endif

gboolean
nsGTKRemoteService::HandlePropertyChange(GtkWidget *aWidget,
                                         GdkEventProperty *pevent,
                                         nsIWeakReference* aThis)
{
  nsCOMPtr<nsIDOMWindow> window (do_QueryReferent(aThis));

  if (pevent->state == GDK_PROPERTY_NEW_VALUE &&
      CMP_GATOM_XATOM(pevent->atom, sMozCommandAtom)) {

    
    int result;
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    char *data = 0;

    result = XGetWindowProperty (GDK_DISPLAY(),
                                 GDK_WINDOW_XWINDOW(pevent->window),
                                 sMozCommandAtom,
                                 0,                        
                                 (65536 / sizeof (long)),  
                                 True,                     
                                 XA_STRING,                
                                 &actual_type,             
                                 &actual_format,           
                                 &nitems,                  
                                 &bytes_after,             
                                 (unsigned char **)&data); 



#ifdef DEBUG_bsmedberg
    printf("Handling command: %s\n", data);
#endif

    
    if (result != Success)
      return FALSE;

    
    if (!data || !TO_LITTLE_ENDIAN32(*reinterpret_cast<PRInt32*>(data)))
      return FALSE;

    
    const char *response = HandleCommand(data, window, pevent->time);

    
    XChangeProperty (GDK_DISPLAY(), GDK_WINDOW_XWINDOW(pevent->window),
                     sMozResponseAtom, XA_STRING,
                     8, PropModeReplace, (const unsigned char *)response, strlen (response));
    XFree(data);
    return TRUE;
  }

  if (pevent->state == GDK_PROPERTY_NEW_VALUE &&
      CMP_GATOM_XATOM(pevent->atom, sMozCommandLineAtom)) {

    
    int result;
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    char *data = 0;

    result = XGetWindowProperty (GDK_DISPLAY(),
                                 GDK_WINDOW_XWINDOW(pevent->window),
                                 sMozCommandLineAtom,
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
      return FALSE;

    
    if (!data || !TO_LITTLE_ENDIAN32(*reinterpret_cast<PRInt32*>(data)))
      return FALSE;

    
    const char *response = HandleCommandLine(data, window, pevent->time);

    
    XChangeProperty (GDK_DISPLAY(), GDK_WINDOW_XWINDOW(pevent->window),
                     sMozResponseAtom, XA_STRING,
                     8, PropModeReplace, (const unsigned char *)response, strlen (response));
    XFree(data);
    return TRUE;
  }

  if (pevent->state == GDK_PROPERTY_NEW_VALUE && 
      CMP_GATOM_XATOM(pevent->atom, sMozResponseAtom)) {
    
    return TRUE;
  }

  if (pevent->state == GDK_PROPERTY_NEW_VALUE && 
      CMP_GATOM_XATOM(pevent->atom, sMozLockAtom)) {
    
    return TRUE;
  }

  return FALSE;
}

Atom nsGTKRemoteService::sMozVersionAtom;
Atom nsGTKRemoteService::sMozLockAtom;
Atom nsGTKRemoteService::sMozCommandAtom;
Atom nsGTKRemoteService::sMozResponseAtom;
Atom nsGTKRemoteService::sMozUserAtom;
Atom nsGTKRemoteService::sMozProfileAtom;
Atom nsGTKRemoteService::sMozProgramAtom;
Atom nsGTKRemoteService::sMozCommandLineAtom;


#define NS_REMOTESERVICE_CID \
  { 0xc0773e90, 0x5799, 0x4eff, { 0xad, 0x3, 0x3e, 0xbc, 0xd8, 0x56, 0x24, 0xac } }

NS_GENERIC_FACTORY_CONSTRUCTOR(nsGTKRemoteService)

static const nsModuleComponentInfo components[] =
{
  { "Remote Service",
    NS_REMOTESERVICE_CID,
    "@mozilla.org/toolkit/remote-service;1",
    nsGTKRemoteServiceConstructor
  }
};

NS_IMPL_NSGETMODULE(RemoteServiceModule, components)

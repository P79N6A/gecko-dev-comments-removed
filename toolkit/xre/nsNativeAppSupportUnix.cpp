






































#include "nsNativeAppSupportBase.h"
#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIObserverService.h"
#include "nsIAppStartup.h"
#include "nsServiceManagerUtils.h"
#include "prlink.h"
#include "nsXREDirProvider.h"
#include "nsReadableUtils.h"

#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define MIN_GTK_MAJOR_VERSION 2
#define MIN_GTK_MINOR_VERSION 10
#define UNSUPPORTED_GTK_MSG "We're sorry, this application requires a version of the GTK+ library that is not installed on your computer.\n\n\
You have GTK+ %d.%d.\nThis application requires GTK+ %d.%d or newer.\n\n\
Please upgrade your GTK+ library if you wish to use this application."

typedef struct _GnomeProgram GnomeProgram;
typedef struct _GnomeModuleInfo GnomeModuleInfo;
typedef struct _GnomeClient GnomeClient;

typedef enum {
  GNOME_SAVE_GLOBAL,
  GNOME_SAVE_LOCAL,
  GNOME_SAVE_BOTH
} GnomeSaveStyle;

typedef enum {
  GNOME_INTERACT_NONE,
  GNOME_INTERACT_ERRORS,
  GNOME_INTERACT_ANY
} GnomeInteractStyle;

typedef enum {
  GNOME_DIALOG_ERROR,
  GNOME_DIALOG_NORMAL
} GnomeDialogType;

typedef GnomeProgram * (*_gnome_program_init_fn)(const char *, const char *,
                                                 const GnomeModuleInfo *, int,
                                                 char **, const char *, ...);
typedef const GnomeModuleInfo * (*_libgnomeui_module_info_get_fn)();
typedef GnomeClient * (*_gnome_master_client_fn)(void);
typedef void (*GnomeInteractFunction)(GnomeClient *, gint, GnomeDialogType,
                                      gpointer);
typedef void (*_gnome_client_request_interaction_fn)(GnomeClient *,
                                                     GnomeDialogType,
                                                     GnomeInteractFunction,
                                                     gpointer);
typedef void (*_gnome_interaction_key_return_fn)(gint, gboolean);
typedef void (*_gnome_client_set_restart_command_fn)(GnomeClient*, gint, gchar*[]);

static _gnome_client_request_interaction_fn gnome_client_request_interaction;
static _gnome_interaction_key_return_fn gnome_interaction_key_return;
static _gnome_client_set_restart_command_fn gnome_client_set_restart_command;

void interact_cb(GnomeClient *client, gint key,
                 GnomeDialogType type, gpointer data)
{
  nsCOMPtr<nsIObserverService> obsServ =
    do_GetService("@mozilla.org/observer-service;1");
  nsCOMPtr<nsISupportsPRBool> cancelQuit =
    do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID);

  cancelQuit->SetData(PR_FALSE);

  obsServ->NotifyObservers(cancelQuit, "quit-application-requested", nsnull);

  PRBool abortQuit;
  cancelQuit->GetData(&abortQuit);

  gnome_interaction_key_return(key, abortQuit);
}

gboolean save_yourself_cb(GnomeClient *client, gint phase,
                          GnomeSaveStyle style, gboolean shutdown,
                          GnomeInteractStyle interact, gboolean fast,
                          gpointer user_data)
{
  if (!shutdown)
    return TRUE;

  nsCOMPtr<nsIObserverService> obsServ =
    do_GetService("@mozilla.org/observer-service;1");

  nsCOMPtr<nsISupportsPRBool> didSaveSession =
    do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID);

  if (!obsServ || !didSaveSession)
    return TRUE; 

  didSaveSession->SetData(PR_FALSE);
  obsServ->NotifyObservers(didSaveSession, "session-save", nsnull);

  PRBool status;
  didSaveSession->GetData(&status);

  
  if (!status) {
    if (interact == GNOME_INTERACT_ANY)
      gnome_client_request_interaction(client, GNOME_DIALOG_NORMAL,
                                       interact_cb, nsnull);
    return TRUE;
  }

  
  NS_ASSERTION(gDirServiceProvider, "gDirServiceProvider is NULL! This shouldn't happen!");
  nsCOMPtr<nsIFile> executablePath;
  nsresult rv;

  PRBool dummy;
  rv = gDirServiceProvider->GetFile(XRE_EXECUTABLE_FILE, &dummy, getter_AddRefs(executablePath));

  if (NS_SUCCEEDED(rv)) {
    nsCAutoString path;
    char* argv[1];

    
    nsCAutoString leafName;
    rv = executablePath->GetNativeLeafName(leafName);
    if (NS_SUCCEEDED(rv) && StringEndsWith(leafName, NS_LITERAL_CSTRING("-bin"))) {
      leafName.SetLength(leafName.Length() - strlen("-bin"));
      executablePath->SetNativeLeafName(leafName);
    }

    executablePath->GetNativePath(path);
    argv[0] = (char*)(path.get());

    gnome_client_set_restart_command(client, 1, argv);
  }

  return TRUE;
}

void die_cb(GnomeClient *client, gpointer user_data)
{
  nsCOMPtr<nsIAppStartup> appService =
    do_GetService("@mozilla.org/toolkit/app-startup;1");

  if (appService)
    appService->Quit(nsIAppStartup::eForceQuit);
}

class nsNativeAppSupportUnix : public nsNativeAppSupportBase
{
public:
  NS_IMETHOD Start(PRBool* aRetVal);
};

NS_IMETHODIMP
nsNativeAppSupportUnix::Start(PRBool *aRetVal)
{

  if (gtk_major_version < MIN_GTK_MAJOR_VERSION ||
      (gtk_major_version == MIN_GTK_MAJOR_VERSION && gtk_minor_version < MIN_GTK_MINOR_VERSION)) {
    GtkWidget* versionErrDialog = gtk_message_dialog_new(NULL,
                     GtkDialogFlags(GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT),
                     GTK_MESSAGE_ERROR,
                     GTK_BUTTONS_OK,
                     UNSUPPORTED_GTK_MSG,
                     gtk_major_version,
                     gtk_minor_version,
                     MIN_GTK_MAJOR_VERSION,
                     MIN_GTK_MINOR_VERSION);
    gtk_dialog_run(GTK_DIALOG(versionErrDialog));
    gtk_widget_destroy(versionErrDialog);
    exit(0);
  }

  *aRetVal = PR_TRUE;

  PRLibrary *gnomeuiLib = PR_LoadLibrary("libgnomeui-2.so.0");
  if (!gnomeuiLib)
    return NS_OK;

  PRLibrary *gnomeLib = PR_LoadLibrary("libgnome-2.so.0");
  if (!gnomeLib) {
    PR_UnloadLibrary(gnomeuiLib);
    return NS_OK;
  }

  _gnome_program_init_fn gnome_program_init =
    (_gnome_program_init_fn)PR_FindFunctionSymbol(gnomeLib, "gnome_program_init");
  _libgnomeui_module_info_get_fn libgnomeui_module_info_get = (_libgnomeui_module_info_get_fn)PR_FindFunctionSymbol(gnomeuiLib, "libgnomeui_module_info_get");
  if (!gnome_program_init || !libgnomeui_module_info_get) {
    PR_UnloadLibrary(gnomeuiLib);
    PR_UnloadLibrary(gnomeLib);
    return NS_OK;
  }

#ifdef ACCESSIBILITY
  
  
  
  static const char *accEnv = "GNOME_ACCESSIBILITY";
  const char *accOldValue = getenv(accEnv);
  setenv(accEnv, "0", 1);
#endif

  gnome_program_init("Gecko", "1.0", libgnomeui_module_info_get(), gArgc, gArgv, NULL);

#ifdef ACCESSIBILITY
  if (accOldValue) { 
    setenv(accEnv, accOldValue, 1);
  } else {
    unsetenv(accEnv);
  }
#endif

  
  
  

  gnome_client_request_interaction = (_gnome_client_request_interaction_fn)
    PR_FindFunctionSymbol(gnomeuiLib, "gnome_client_request_interaction");
  gnome_interaction_key_return = (_gnome_interaction_key_return_fn)
    PR_FindFunctionSymbol(gnomeuiLib, "gnome_interaction_key_return");
  gnome_client_set_restart_command = (_gnome_client_set_restart_command_fn)
    PR_FindFunctionSymbol(gnomeuiLib, "gnome_client_set_restart_command");

  _gnome_master_client_fn gnome_master_client = (_gnome_master_client_fn)
    PR_FindFunctionSymbol(gnomeuiLib, "gnome_master_client");

  GnomeClient *client = gnome_master_client();
  g_signal_connect(client, "save-yourself", G_CALLBACK(save_yourself_cb), NULL);
  g_signal_connect(client, "die", G_CALLBACK(die_cb), NULL);

  return NS_OK;
}

nsresult
NS_CreateNativeAppSupport(nsINativeAppSupport **aResult)
{
  nsNativeAppSupportBase* native = new nsNativeAppSupportUnix();
  if (!native)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = native;
  NS_ADDREF(*aResult);

  return NS_OK;
}

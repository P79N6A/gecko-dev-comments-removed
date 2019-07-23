








































#include "nsSystemSoundService.h"
#include "prlink.h"

#include <gtk/gtk.h>
#include <stdint.h>

static PRLibrary *libcanberra = nsnull;
static PRLibrary* libasound = nsnull;




typedef struct _ca_context ca_context;

typedef int (*ca_context_create_fn) (ca_context **);
typedef int (*ca_context_destroy_fn) (ca_context *);
typedef int (*ca_context_play_fn) (ca_context *c,
                                   uint32_t id,
                                   ...);
typedef int (*ca_context_change_props_fn) (ca_context *c,
                                           ...);

static ca_context_create_fn ca_context_create;
static ca_context_destroy_fn ca_context_destroy;
static ca_context_play_fn ca_context_play;
static ca_context_change_props_fn ca_context_change_props;



typedef void (*snd_lib_error_handler_t) (const char* file,
                                         int         line,
                                         const char* function,
                                         int         err,
                                         const char* format,
                                         ...);
typedef int (*snd_lib_error_set_handler_fn) (snd_lib_error_handler_t handler);

static void
quiet_error_handler(const char* file,
                    int         line,
                    const char* function,
                    int         err,
                    const char* format,
                    ...)
{
}





NS_IMPL_ISUPPORTS1(nsSystemSoundService, nsISystemSoundService)

NS_IMPL_ISYSTEMSOUNDSERVICE_GETINSTANCE(nsSystemSoundService)

nsSystemSoundService::nsSystemSoundService() :
  nsSystemSoundServiceBase()
{
}

nsSystemSoundService::~nsSystemSoundService()
{
}

nsresult
nsSystemSoundService::Init()
{
    PRFuncPtr func =
        PR_FindFunctionSymbolAndLibrary("snd_lib_error_set_handler",
                                        &libasound);
    if (libasound) {
        snd_lib_error_set_handler_fn snd_lib_error_set_handler =
             (snd_lib_error_set_handler_fn) func;
        snd_lib_error_set_handler(quiet_error_handler);
    }

    libcanberra = PR_LoadLibrary("libcanberra.so.0");
    if (libcanberra) {
        ca_context_create = (ca_context_create_fn)
             PR_FindFunctionSymbol(libcanberra, "ca_context_create");
        if (!ca_context_create) {
            PR_UnloadLibrary(libcanberra);
            libcanberra = nsnull;
        } else {
            
            ca_context_destroy = (ca_context_destroy_fn)
                PR_FindFunctionSymbol(libcanberra, "ca_context_destroy");
            ca_context_play = (ca_context_play_fn)
                PR_FindFunctionSymbol(libcanberra, "ca_context_play");
            ca_context_change_props = (ca_context_change_props_fn)
                PR_FindFunctionSymbol(libcanberra, "ca_context_change_props");
        }
    }

    return NS_OK;
}

void
nsSystemSoundService::OnShutdown()
{
    if (libcanberra) {
        PR_UnloadLibrary(libcanberra);
        libcanberra = nsnull;
    }
    if (libasound) {
        PR_UnloadLibrary(libasound);
        libasound = nsnull;
    }
}

NS_IMETHODIMP
nsSystemSoundService::Beep()
{
    nsresult rv = nsSystemSoundServiceBase::Beep();
    NS_ENSURE_SUCCESS(rv, rv);

    ::gdk_beep();
    return NS_OK;
}

NS_IMETHODIMP
nsSystemSoundService::PlayEventSound(PRUint32 aEventID)
{
    nsresult rv = nsSystemSoundServiceBase::PlayEventSound(aEventID);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!libcanberra)
        return NS_OK;

    
    
    GtkSettings* settings = gtk_settings_get_default();
    gchar* sound_theme_name = nsnull;

    if (g_object_class_find_property(G_OBJECT_GET_CLASS(settings),
                                     "gtk-sound-theme-name") &&
        g_object_class_find_property(G_OBJECT_GET_CLASS(settings),
                                     "gtk-enable-event-sounds")) {
        gboolean enable_sounds = TRUE;
        g_object_get(settings, "gtk-enable-event-sounds", &enable_sounds,
                               "gtk-sound-theme-name", &sound_theme_name,
                               NULL);

        if (!enable_sounds) {
            g_free(sound_theme_name);
            return NS_OK;
        }
    }

    
    
    ca_context* ctx = nsnull;
    static GStaticPrivate ctx_static_private = G_STATIC_PRIVATE_INIT;
    ctx = (ca_context*) g_static_private_get(&ctx_static_private);
    if (!ctx) {
        ca_context_create(&ctx);
        if (!ctx) {
            g_free(sound_theme_name);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        g_static_private_set(&ctx_static_private, ctx,
                             (GDestroyNotify) ca_context_destroy);
    }

    if (sound_theme_name) {
        ca_context_change_props(ctx, "canberra.xdg-theme.name",
                                sound_theme_name, NULL);
        g_free(sound_theme_name);
    }

    const char* eventID = nsnull;
    switch (aEventID) {
        case EVENT_ALERT_DIALOG_OPEN:
            eventID = "dialog-warning";
            break;
        case EVENT_CONFIRM_DIALOG_OPEN:
            eventID = "dialog-question";
            break;
        case EVENT_NEW_MAIL_RECEIVED:
            eventID = "message-new-email";
            break;
        case EVENT_MENU_EXECUTE:
            eventID = "menu-click";
            break;
        case EVENT_MENU_POPUP:
            eventID = "menu-popup";
            break;
        default:
            return NS_OK;
    }

    StopSoundPlayer();
    ca_context_play(ctx, 0, "event.id", eventID, NULL);

    return NS_OK;
}

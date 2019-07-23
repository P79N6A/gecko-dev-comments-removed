

















































#define TARGET              "_blank"
#define MIME_TYPES_HANDLED  "*:.*:All types"
#define PLUGIN_NAME         "Default Plugin"
#define PLUGIN_DESCRIPTION  "The default plugin handles plugin data for mimetypes and extensions that are not specified and facilitates downloading of new plugins."
#define CLICK_TO_GET        "Click here to get the plugin"       
#define CLICK_WHEN_DONE     "Click here after installing the plugin"

#define REFRESH_PLUGIN_LIST "javascript:navigator.plugins.refresh(true)"
#define PLUGINSPAGE_URL     "http://plugindoc.mozdev.org/winmime.html" /* XXX Branding: make configurable via .properties or prefs */
#define OK_BUTTON           "OK"
#define CANCEL_BUTTON       "CANCEL"
#if defined(HPUX)
#define JVM_SMARTUPDATE_URL "http://www.hp.com/go/java"
#elif defined(VMS)
#define JVM_SMARTUPDATE_URL "http://www.openvms.compaq.com/openvms/products/ips/mozilla_relnotes.html#java"
#else 
#define JVM_SMARTUPDATE_URL "http://java.com/download" /* XXX Branding: see above */
#endif 
#define JVM_MINETYPE        "application/x-java-vm"
#define MESSAGE "\
This page contains information of a type (%s) that can\n\
only be viewed with the appropriate Plug-in.\n\
\n\
Click OK to download Plugin."

#define GET 1
#define REFRESH 2
#include <gtk/gtk.h>

typedef struct _PluginInstance
{
    uint16 mode;
#ifdef MOZ_X11
    Window window;
    Display *display;
#endif
    uint32 x, y;
    uint32 width, height;
    NPMIMEType type;
    char *message;

    NPP instance;
    char *pluginsPageUrl;
    char *pluginsFileUrl;
    NPBool pluginsHidden;
#ifdef MOZ_X11
    Visual* visual;
    Colormap colormap;
#endif
    unsigned int depth;
    GtkWidget* dialogBox;

    NPBool exists;  
    int action;     

} PluginInstance;


typedef struct _MimeTypeElement
{
    PluginInstance *pinst;
    struct _MimeTypeElement *next;
} MimeTypeElement;


extern void makeWidget(PluginInstance *This);
extern NPMIMEType dupMimeType(NPMIMEType type);
extern void destroyWidget(PluginInstance *This);
extern void makePixmap(PluginInstance *This);
extern void destroyPixmap();



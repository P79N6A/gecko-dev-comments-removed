





































#ifndef UNIXPRINTING_SAMPLEPLUGIN_H
#define UNIXPRINTING_SAMPLEPLUGIN_H 1

#define MIME_TYPES_HANDLED "application/x-print-unix-nsplugin:.pnt:Demo Print Plugin for Unix/Linux"
#define PLUGIN_NAME         "Demo Print Plugin for unix/linux"
#define PLUGIN_DESCRIPTION   "The demo print plugin for unix."

typedef struct _PluginInstance
{
    uint16       mode;
#ifdef MOZ_X11
    Window       window;
    Display     *display;
#endif 
    uint32       x,
                 y;
    uint32       width,
                 height;
    NPMIMEType   type;
    char        *message;

    NPP          instance;
    char        *pluginsPrintMessage;
    NPBool       pluginsHidden;
#ifdef MOZ_X11
    Visual      *visual;
    Colormap     colormap;
#endif 
    unsigned int depth;

    NPBool       exists;  
} PluginInstance;



extern NPMIMEType dupMimeType(NPMIMEType type);
extern void printScreenMessage(PluginInstance *This);
extern void printEPSMessage(PluginInstance *This, FILE *output, NPWindow window);

#endif 


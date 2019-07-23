

























































#include <stdio.h>
#include <string.h>
#include "npapi.h"
#include "printplugin.h"
#include "strings.h"
#include "plstr.h"







char*
NPP_GetMIMEDescription(void)
{
    return(MIME_TYPES_HANDLED);
}

NPError
NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
    NPError err = NPERR_NO_ERROR;

    switch (variable) {
        case NPPVpluginNameString:
            *((char **)value) = PLUGIN_NAME;
            break;
        case NPPVpluginDescriptionString:
            *((char **)value) = PLUGIN_DESCRIPTION;
            break;
        default:
            err = NPERR_GENERIC_ERROR;
    }
    return err;
}

NPError
NPP_Initialize(void)
{
    return NPERR_NO_ERROR;
}

#ifdef OJI
jref
NPP_GetJavaClass()
{
    return NULL;
}
#endif

void
NPP_Shutdown(void)
{
}

NPError
NPP_New(NPMIMEType pluginType,
    NPP instance,
    uint16 mode,
    int16 argc,
    char* argn[],
    char* argv[],
    NPSavedData* saved)
{

    PluginInstance* This;

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    instance->pdata = NPN_MemAlloc(sizeof(PluginInstance));

    This = (PluginInstance*) instance->pdata;

    if (This == NULL)
    {
        return NPERR_OUT_OF_MEMORY_ERROR;
    }

    memset(This, 0, sizeof(PluginInstance));

    
    This->mode = mode;
    This->type = dupMimeType(pluginType);
    This->instance = instance;
    This->pluginsPrintMessage = NULL;
    This->exists = FALSE;

    
    


    while (argc > 0)
    {
        argc --;
        if (argv[argc] != NULL)
        {
        if (!PL_strcasecmp(argn[argc], "PRINTMSG"))
            This->pluginsPrintMessage = strdup(argv[argc]);
        else if (!PL_strcasecmp(argn[argc], "HIDDEN"))
            This->pluginsHidden = (!PL_strcasecmp(argv[argc],
            "TRUE"));
        }
    }

    return NPERR_NO_ERROR;
}

NPError
NPP_Destroy(NPP instance, NPSavedData** save)
{

    PluginInstance* This;

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    This = (PluginInstance*) instance->pdata;

    if (This != NULL) {
        if (This->type)
            NPN_MemFree(This->type);
        if (This->pluginsPrintMessage)
            NPN_MemFree(This->pluginsPrintMessage);
        NPN_MemFree(instance->pdata);
        instance->pdata = NULL;
    }

    return NPERR_NO_ERROR;
}


NPError
NPP_SetWindow(NPP instance, NPWindow* window)
{
    PluginInstance* This;
    NPSetWindowCallbackStruct *ws_info;

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    This = (PluginInstance*) instance->pdata;

    if (This == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    ws_info = (NPSetWindowCallbackStruct *)window->ws_info;

#ifdef MOZ_X11
    if (This->window == (Window) window->window) {
        




#ifdef DEBUG
        fprintf(stderr, "Printplugin: plugin received window resize.\n");
        fprintf(stderr, "Window=(%i)\n", (int)window);
        if (window) {
           fprintf(stderr, "W=(%i) H=(%i)\n",
               (int)window->width, (int)window->height);
        }
#endif
        return NPERR_NO_ERROR;
    } else {

      This->window = (Window) window->window;
      This->x = window->x;
      This->y = window->y;
      This->width = window->width;
      This->height = window->height;
      This->display = ws_info->display;
      This->visual = ws_info->visual;
      This->depth = ws_info->depth;
      This->colormap = ws_info->colormap;
      printScreenMessage(This);
    }
#endif  

    return NPERR_NO_ERROR;
}


NPError
NPP_NewStream(NPP instance,
          NPMIMEType type,
          NPStream *stream,
          NPBool seekable,
          uint16 *stype)
{
    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    return NPERR_NO_ERROR;
}


int32
NPP_WriteReady(NPP instance, NPStream *stream)
{
    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    
    NPN_DestroyStream(instance, stream, NPRES_DONE);

    
    return -1L;   
}


int32
NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    
    NPN_DestroyStream(instance, stream, NPRES_DONE);

    return -1L;   
}


NPError
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    




    return NPERR_NO_ERROR;
}


void
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
    




}


void
NPP_URLNotify(NPP instance, const char* url,
                NPReason reason, void* notifyData)
{
    




}


void
NPP_Print(NPP instance, NPPrint* printInfo)
{
    if(printInfo == NULL)
        return;

    if (instance != NULL) {
    



        if (printInfo->mode == NP_FULL) {
            
















    






            
            printInfo->print.fullPrint.pluginPrinted = FALSE;
        }
        else {  
            











    





            PluginInstance* This;
            NPPrintCallbackStruct* platformPrint;
            FILE *output;

            platformPrint =
                (NPPrintCallbackStruct *)(printInfo->print.embedPrint.platformPrint);

            output = (FILE*)(platformPrint->fp);
            if (output == NULL)
                return;

            
            This =  (PluginInstance*) instance->pdata;
            if (This == NULL)
                return;

            printEPSMessage(This, output, printInfo->print.embedPrint.window);

        }
    }
}

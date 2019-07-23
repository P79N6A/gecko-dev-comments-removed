






































#ifndef LEGACYPLUGIN_H
#define LEGACYPLUGIN_H

#include "npapi.h"
#include "nsISupports.h"

#ifdef MOZ_ACTIVEX_PLUGIN_XPCONNECT
#include "XPConnect.h"
#endif


enum PluginInstanceType
{
    itScript,
    itControl
};


struct PluginInstanceData {
    NPP pPluginInstance;
    PluginInstanceType nType;
    CControlSiteInstance *pControlSite;
#ifdef XPC_IDISPATCH_SUPPORT
    nsEventSinkInstance  *pControlEventSink;
#endif
    char *szUrl;
    char *szContentType;
    CLSID clsid;
#ifdef MOZ_ACTIVEX_PLUGIN_XPCONNECT
    nsISupports *pScriptingPeer;
#endif
};

#endif
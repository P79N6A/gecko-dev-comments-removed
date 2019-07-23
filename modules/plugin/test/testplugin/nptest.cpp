




































#include "npapi.h"
#include "npfunctions.h"

#if defined(XP_UNIX)

#define PLUGIN_NAME         "Test Plug-in"
#define PLUGIN_DESCRIPTION  "Plug-in for testing purposes."
#define PLUGIN_VERSION      "1.0.0.0"

NP_EXPORT(char*)
NP_GetPluginVersion(void) {
    return PLUGIN_VERSION;
}

NP_EXPORT(char*)
NP_GetMIMEDescription(void) {
    return "application/x-test:tst:Test mimetype";
}

NP_EXPORT(NPError)
NP_Initialize(NPNetscapeFuncs*, NPPluginFuncs*) {
    return NPERR_NO_ERROR;
}

NP_EXPORT(NPError)
NP_Shutdown(void) {
    return NPERR_NO_ERROR;
}

NP_EXPORT(NPError) 
NP_GetValue(void *future, NPPVariable aVariable, void *aValue) {
   switch (aVariable) {
     case NPPVpluginNameString:
       *((char **)aValue) = PLUGIN_NAME;
       break;
     case NPPVpluginDescriptionString:
       *((char **)aValue) = PLUGIN_DESCRIPTION;
       break;
     default:
       return NPERR_INVALID_PARAM;
       break;
   }
   return NPERR_NO_ERROR;
}

#endif

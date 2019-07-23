




































#ifndef __PROFILE_H__
#define __PROFILE_H__

#include "script.h"

ScriptItemStruct * readProfileSectionAndCreateScriptItemStruct(LPSTR szFileName, LPSTR szSection);

#define PROFILE_DEFAULT_STRING  "*"
#define PROFILE_NUMERIC_PREFIX  "_true_numeric_value_"

#define SECTION_OPTIONS "Options"
#define KEY_FIRST       "first"
#define KEY_LAST        "last"
#define KEY_REPETITIONS "repetitions"

#define KEY_ACTION      "APICall"
#define KEY_ARG1        "arg1"
#define KEY_ARG2        "arg2"
#define KEY_ARG3        "arg3"
#define KEY_ARG4        "arg4"
#define KEY_ARG5        "arg5"
#define KEY_ARG6        "arg6"
#define KEY_ARG7        "arg7"
#define KEY_DELAY       "delay"

#define KEY_WIDTH       "width"
#define KEY_HEIGHT      "height"
#define KEY_TOP         "top"
#define KEY_BOTTOM      "bottom"
#define KEY_LEFT        "left"
#define KEY_RIGHT       "right"

#define ENTRY_TRUE      "TRUE"
#define ENTRY_FALSE     "FALSE"


#define ENTRY_NPRES_DONE			             "NPRES_DONE"
#define ENTRY_NPRES_NETWORK_ERR            "NPRES_NETWORK_ERR"
#define ENTRY_NPRES_USER_BREAK             "NPRES_USER_BREAK"


#define ENTRY_NPPVPLUGINNAMESTRING         "NPPVpluginNameString"
#define ENTRY_NPPVPLUGINDESCRIPTIONSTRING  "NPPVpluginDescriptionString"
#define ENTRY_NPPVPLUGINWINDOWBOOL         "NPPVpluginWindowBool"
#define ENTRY_NPPVPLUGINTRANSPARENTBOOL    "NPPVpluginTransparentBool"
#define ENTRY_NPPVPLUGINWINDOWSIZE         "NPPVpluginWindowSize"
#define ENTRY_NPPVPLUGINKEEPLIBRARYINMEMORY "NPPVpluginKeepLibraryInMemory"


#define ENTRY_NPNVXDISPLAY                 "NPNVxDisplay"
#define ENTRY_NPNVXTAPPCONTEXT             "NPNVxtAppContext"
#define ENTRY_NPNVNETSCAPEWINDOW           "NPNVnetscapeWindow"
#define ENTRY_NPNVJAVASCRIPTENABLEDBOOL    "NPNVjavascriptEnabledBool"
#define ENTRY_NPNVASDENABLEDBOOL           "NPNVasdEnabledBool"
#define ENTRY_NPNVISOFFLINEBOOL            "NPNVisOfflineBool"

#endif 


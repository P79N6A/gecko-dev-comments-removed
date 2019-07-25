










































#ifdef XPCONNECT_STANDALONE
#define NO_SUBSCRIPT_LOADER
#endif

#include "mozilla/ModuleUtils.h"
#include "nsICategoryManager.h"
#include "mozJSComponentLoader.h"

#ifndef NO_SUBSCRIPT_LOADER
#include "mozJSSubScriptLoader.h"
const char mozJSSubScriptLoadContractID[] = "@mozilla.org/moz/jssubscript-loader;1";
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(mozJSComponentLoader)

#ifndef NO_SUBSCRIPT_LOADER
NS_GENERIC_FACTORY_CONSTRUCTOR(mozJSSubScriptLoader)
#endif

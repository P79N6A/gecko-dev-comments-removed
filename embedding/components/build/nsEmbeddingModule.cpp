




































#include "nsIGenericFactory.h"
#include "nsDialogParamBlock.h"
#include "nsWindowWatcher.h"
#include "nsAppStartupNotifier.h"
#include "nsFind.h"
#include "nsWebBrowserFind.h"
#include "nsWebBrowserPersist.h"
#include "nsCommandManager.h"
#include "nsControllerCommandTable.h"
#include "nsCommandParams.h"
#include "nsCommandGroup.h"
#include "nsBaseCommandController.h"
#include "nsNetCID.h"
#include "nsEmbedCID.h"

#ifdef NS_PRINTING
#ifndef WINCE
#include "nsPrintingPromptService.h"
#endif
#endif


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsWindowWatcher, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAppStartupNotifier)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFind)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWebBrowserFind)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWebBrowserPersist)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsControllerCommandTable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCommandManager)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsCommandParams, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsControllerCommandGroup)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBaseCommandController)

#ifdef MOZ_XUL
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDialogParamBlock)
#ifdef NS_PRINTING
#ifndef WINCE
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintingPromptService, Init)
#endif
#endif
#endif

static const nsModuleComponentInfo gComponents[] = {

#ifdef MOZ_XUL
  { "Dialog ParamBlock", NS_DIALOGPARAMBLOCK_CID, NS_DIALOGPARAMBLOCK_CONTRACTID, nsDialogParamBlockConstructor },
#ifdef NS_PRINTING
#ifndef WINCE
  { "Printing Prompt Service", NS_PRINTINGPROMPTSERVICE_CID, NS_PRINTINGPROMPTSERVICE_CONTRACTID, nsPrintingPromptServiceConstructor },
#endif
#endif
#endif
  { "Window Watcher", NS_WINDOWWATCHER_CID, NS_WINDOWWATCHER_CONTRACTID, nsWindowWatcherConstructor },
  { "Find",           NS_FIND_CID, NS_FIND_CONTRACTID, nsFindConstructor },
  { "WebBrowserFind",           NS_WEB_BROWSER_FIND_CID, NS_WEB_BROWSER_FIND_CONTRACTID, nsWebBrowserFindConstructor },
  { NS_APPSTARTUPNOTIFIER_CLASSNAME, NS_APPSTARTUPNOTIFIER_CID, NS_APPSTARTUPNOTIFIER_CONTRACTID, nsAppStartupNotifierConstructor },
  { "WebBrowserPersist Component", NS_WEBBROWSERPERSIST_CID, NS_WEBBROWSERPERSIST_CONTRACTID, nsWebBrowserPersistConstructor },
  { "Controller Command Table", NS_CONTROLLERCOMMANDTABLE_CID, NS_CONTROLLERCOMMANDTABLE_CONTRACTID, nsControllerCommandTableConstructor },
  { "Command Manager", NS_COMMAND_MANAGER_CID, NS_COMMAND_MANAGER_CONTRACTID, nsCommandManagerConstructor },
  { "Command Params", NS_COMMAND_PARAMS_CID, NS_COMMAND_PARAMS_CONTRACTID, nsCommandParamsConstructor },
  { "Command Group", NS_CONTROLLER_COMMAND_GROUP_CID, NS_CONTROLLER_COMMAND_GROUP_CONTRACTID, nsControllerCommandGroupConstructor },
  { "Base Command Controller", NS_BASECOMMANDCONTROLLER_CID, NS_BASECOMMANDCONTROLLER_CONTRACTID, nsBaseCommandControllerConstructor },
};

NS_IMPL_NSGETMODULE(embedcomponents, gComponents)

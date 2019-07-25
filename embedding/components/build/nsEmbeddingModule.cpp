




































#include "mozilla/ModuleUtils.h"
#include "nsDialogParamBlock.h"
#include "nsPromptService.h"
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
#include "nsPrompt.h"
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

#define NS_DEFAULTPROMPT_CLASSNAME \
    "nsDefaultPrompt"
#define NS_DEFAULTPROMPT_CID                         \
{ /* 2e41ada0-62b7-4902-b9a6-e4542aa458ba */         \
    0x2e41ada0,                                      \
    0x62b7,                                          \
    0x4902,                                          \
    {0xb9, 0xa6, 0xe4, 0x54, 0x2a, 0xa4, 0x58, 0xba} \
}

#define NS_DEFAULTAUTHPROMPT_CLASSNAME \
    "nsDefaultAuthPrompt"
#define NS_DEFAULTAUTHPROMPT_CID                     \
{ /* ca200860-4696-40d7-88fa-4490d423a8ef */         \
    0xca200860,                                      \
    0x4696,                                          \
    0x40d7,                                          \
    {0x88, 0xfa, 0x44, 0x90, 0xd4, 0x23, 0xa8, 0xef} \
}

static nsresult
nsDefaultPromptConstructor(nsISupports *outer, const nsIID &iid, void **result)
{
  if (outer)
    return NS_ERROR_NO_AGGREGATION;

  nsCOMPtr<nsIPrompt> prompt;
  nsresult rv = NS_NewPrompter(getter_AddRefs(prompt), nsnull);
  if (NS_FAILED(rv))
    return rv;

  return prompt->QueryInterface(iid, result);
}

static nsresult
nsDefaultAuthPromptConstructor(nsISupports *outer, const nsIID &iid, void **result)
{
  if (outer)
    return NS_ERROR_NO_AGGREGATION;

  nsCOMPtr<nsIAuthPrompt> prompt;
  nsresult rv = NS_NewAuthPrompter(getter_AddRefs(prompt), nsnull);
  if (NS_FAILED(rv))
    return rv;

  return prompt->QueryInterface(iid, result);
}

#ifdef MOZ_XUL
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDialogParamBlock)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPromptService, Init)
#ifdef NS_PRINTING
#ifndef WINCE
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintingPromptService, Init)
#endif
#endif
#endif

#ifdef MOZ_XUL
NS_DEFINE_NAMED_CID(NS_DIALOGPARAMBLOCK_CID);
NS_DEFINE_NAMED_CID(NS_PROMPTSERVICE_CID);
#ifdef NS_PRINTING
#ifndef WINCE
NS_DEFINE_NAMED_CID(NS_PRINTINGPROMPTSERVICE_CID);
#endif
#endif
#endif
NS_DEFINE_NAMED_CID(NS_WINDOWWATCHER_CID);
NS_DEFINE_NAMED_CID(NS_FIND_CID);
NS_DEFINE_NAMED_CID(NS_WEB_BROWSER_FIND_CID);
NS_DEFINE_NAMED_CID(NS_APPSTARTUPNOTIFIER_CID);
NS_DEFINE_NAMED_CID(NS_WEBBROWSERPERSIST_CID);
NS_DEFINE_NAMED_CID(NS_CONTROLLERCOMMANDTABLE_CID);
NS_DEFINE_NAMED_CID(NS_COMMAND_MANAGER_CID);
NS_DEFINE_NAMED_CID(NS_COMMAND_PARAMS_CID);
NS_DEFINE_NAMED_CID(NS_CONTROLLER_COMMAND_GROUP_CID);
NS_DEFINE_NAMED_CID(NS_BASECOMMANDCONTROLLER_CID);
NS_DEFINE_NAMED_CID(NS_DEFAULTPROMPT_CID);
NS_DEFINE_NAMED_CID(NS_DEFAULTAUTHPROMPT_CID);


static const mozilla::Module::CIDEntry kEmbeddingCIDs[] = {
#ifdef MOZ_XUL
    { &kNS_DIALOGPARAMBLOCK_CID, false, NULL, nsDialogParamBlockConstructor },
    { &kNS_PROMPTSERVICE_CID, false, NULL, nsPromptServiceConstructor },
#ifdef NS_PRINTING
#ifndef WINCE
    { &kNS_PRINTINGPROMPTSERVICE_CID, false, NULL, nsPrintingPromptServiceConstructor },
#endif
#endif
#endif
    { &kNS_WINDOWWATCHER_CID, false, NULL, nsWindowWatcherConstructor },
    { &kNS_FIND_CID, false, NULL, nsFindConstructor },
    { &kNS_WEB_BROWSER_FIND_CID, false, NULL, nsWebBrowserFindConstructor },
    { &kNS_APPSTARTUPNOTIFIER_CID, false, NULL, nsAppStartupNotifierConstructor },
    { &kNS_WEBBROWSERPERSIST_CID, false, NULL, nsWebBrowserPersistConstructor },
    { &kNS_CONTROLLERCOMMANDTABLE_CID, false, NULL, nsControllerCommandTableConstructor },
    { &kNS_COMMAND_MANAGER_CID, false, NULL, nsCommandManagerConstructor },
    { &kNS_COMMAND_PARAMS_CID, false, NULL, nsCommandParamsConstructor },
    { &kNS_CONTROLLER_COMMAND_GROUP_CID, false, NULL, nsControllerCommandGroupConstructor },
    { &kNS_BASECOMMANDCONTROLLER_CID, false, NULL, nsBaseCommandControllerConstructor },
    { &kNS_DEFAULTPROMPT_CID, false, NULL, nsDefaultPromptConstructor },
    { &kNS_DEFAULTAUTHPROMPT_CID, false, NULL, nsDefaultAuthPromptConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kEmbeddingContracts[] = {
#ifdef MOZ_XUL
    { NS_DIALOGPARAMBLOCK_CONTRACTID, &kNS_DIALOGPARAMBLOCK_CID },
    { NS_PROMPTSERVICE_CONTRACTID, &kNS_PROMPTSERVICE_CID },
#ifdef NS_PRINTING
#ifndef WINCE
    { NS_PRINTINGPROMPTSERVICE_CONTRACTID, &kNS_PRINTINGPROMPTSERVICE_CID },
#endif
#endif
#endif
    { NS_WINDOWWATCHER_CONTRACTID, &kNS_WINDOWWATCHER_CID },
    { NS_AUTHPROMPT_ADAPTER_FACTORY_CONTRACTID, &kNS_WINDOWWATCHER_CID },
    { NS_FIND_CONTRACTID, &kNS_FIND_CID },
    { NS_WEB_BROWSER_FIND_CONTRACTID, &kNS_WEB_BROWSER_FIND_CID },
    { NS_APPSTARTUPNOTIFIER_CONTRACTID, &kNS_APPSTARTUPNOTIFIER_CID },
    { NS_WEBBROWSERPERSIST_CONTRACTID, &kNS_WEBBROWSERPERSIST_CID },
    { NS_CONTROLLERCOMMANDTABLE_CONTRACTID, &kNS_CONTROLLERCOMMANDTABLE_CID },
    { NS_COMMAND_MANAGER_CONTRACTID, &kNS_COMMAND_MANAGER_CID },
    { NS_COMMAND_PARAMS_CONTRACTID, &kNS_COMMAND_PARAMS_CID },
    { NS_CONTROLLER_COMMAND_GROUP_CONTRACTID, &kNS_CONTROLLER_COMMAND_GROUP_CID },
    { NS_BASECOMMANDCONTROLLER_CONTRACTID, &kNS_BASECOMMANDCONTROLLER_CID },
    { NS_DEFAULTPROMPT_CONTRACTID, &kNS_DEFAULTPROMPT_CID },
    { NS_DEFAULTAUTHPROMPT_CONTRACTID, &kNS_DEFAULTAUTHPROMPT_CID },
    { NULL }
};

static const mozilla::Module kEmbeddingModule = {
    mozilla::Module::kVersion,
    kEmbeddingCIDs,
    kEmbeddingContracts
};

NSMODULE_DEFN(embedcomponents) = &kEmbeddingModule;

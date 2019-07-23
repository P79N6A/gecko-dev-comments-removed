






































#include "nsIGenericFactory.h"

#include "nsEditingSession.h"       
#include "nsComposerController.h"   
#include "nsEditorSpellCheck.h"     
#include "nsComposeTxtSrvFilter.h"
#include "nsIController.h"
#include "nsIControllerContext.h"
#include "nsIControllerCommandTable.h"

#include "nsServiceManagerUtils.h"

#define NS_HTMLEDITOR_COMMANDTABLE_CID \
{ 0x13e50d8d, 0x9cee, 0x4ad1, { 0xa3, 0xa2, 0x4a, 0x44, 0x2f, 0xdf, 0x7d, 0xfa } }

#define NS_HTMLEDITOR_DOCSTATE_COMMANDTABLE_CID \
{ 0xa33982d3, 0x1adf, 0x4162, { 0x99, 0x41, 0xf7, 0x34, 0xbc, 0x45, 0xe4, 0xed } }


static NS_DEFINE_CID(kHTMLEditorCommandTableCID, NS_HTMLEDITOR_COMMANDTABLE_CID);
static NS_DEFINE_CID(kHTMLEditorDocStateCommandTableCID, NS_HTMLEDITOR_DOCSTATE_COMMANDTABLE_CID);








NS_GENERIC_FACTORY_CONSTRUCTOR(nsEditingSession)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsEditorSpellCheck)







static NS_METHOD
nsComposeTxtSrvFilterConstructor(nsISupports *aOuter, REFNSIID aIID,
                                 void **aResult, PRBool aIsForMail)
{
    *aResult = NULL;
    if (NULL != aOuter) 
    {
        return NS_ERROR_NO_AGGREGATION;
    }
    nsComposeTxtSrvFilter * inst;
    NS_NEWXPCOM(inst, nsComposeTxtSrvFilter);
    if (NULL == inst) 
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(inst);
	  inst->Init(aIsForMail);
    nsresult rv = inst->QueryInterface(aIID, aResult);
    NS_RELEASE(inst);
    return rv;
}

static NS_METHOD
nsComposeTxtSrvFilterConstructorForComposer(nsISupports *aOuter, 
                                            REFNSIID aIID,
                                            void **aResult)
{
    return nsComposeTxtSrvFilterConstructor(aOuter, aIID, aResult, PR_FALSE);
}

static NS_METHOD
nsComposeTxtSrvFilterConstructorForMail(nsISupports *aOuter, 
                                        REFNSIID aIID,
                                        void **aResult)
{
    return nsComposeTxtSrvFilterConstructor(aOuter, aIID, aResult, PR_TRUE);
}









static nsresult
CreateControllerWithSingletonCommandTable(const nsCID& inCommandTableCID, nsIController **aResult)
{
  nsresult rv;
  nsCOMPtr<nsIController> controller = do_CreateInstance("@mozilla.org/embedcomp/base-command-controller;1", &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIControllerCommandTable> composerCommandTable = do_GetService(inCommandTableCID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  
  composerCommandTable->MakeImmutable();
  
  nsCOMPtr<nsIControllerContext> controllerContext = do_QueryInterface(controller, &rv);
  if (NS_FAILED(rv)) return rv;
  
  rv = controllerContext->Init(composerCommandTable);
  if (NS_FAILED(rv)) return rv;
  
  *aResult = controller;
  NS_ADDREF(*aResult);
  return NS_OK;
}




static NS_METHOD
nsHTMLEditorDocStateControllerConstructor(nsISupports *aOuter, REFNSIID aIID, 
                                              void **aResult)
{
  nsCOMPtr<nsIController> controller;
  nsresult rv = CreateControllerWithSingletonCommandTable(kHTMLEditorDocStateCommandTableCID, getter_AddRefs(controller));
  if (NS_FAILED(rv)) return rv;

  return controller->QueryInterface(aIID, aResult);
}



static NS_METHOD
nsHTMLEditorControllerConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsCOMPtr<nsIController> controller;
  nsresult rv = CreateControllerWithSingletonCommandTable(kHTMLEditorCommandTableCID, getter_AddRefs(controller));
  if (NS_FAILED(rv)) return rv;

  return controller->QueryInterface(aIID, aResult);
}


static NS_METHOD
nsHTMLEditorCommandTableConstructor(nsISupports *aOuter, REFNSIID aIID, 
                                              void **aResult)
{
  nsresult rv;
  nsCOMPtr<nsIControllerCommandTable> commandTable =
      do_CreateInstance(NS_CONTROLLERCOMMANDTABLE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  rv = nsComposerController::RegisterHTMLEditorCommands(commandTable);
  if (NS_FAILED(rv)) return rv;
  
  
  
  
  return commandTable->QueryInterface(aIID, aResult);
}



static NS_METHOD
nsHTMLEditorDocStateCommandTableConstructor(nsISupports *aOuter, REFNSIID aIID, 
                                              void **aResult)
{
  nsresult rv;
  nsCOMPtr<nsIControllerCommandTable> commandTable =
      do_CreateInstance(NS_CONTROLLERCOMMANDTABLE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  rv = nsComposerController::RegisterEditorDocStateCommands(commandTable);
  if (NS_FAILED(rv)) return rv;
  
  
  
  
  return commandTable->QueryInterface(aIID, aResult);
}






static const nsModuleComponentInfo components[] = {

    { "HTML Editor Controller", NS_HTMLEDITORCONTROLLER_CID,
      "@mozilla.org/editor/htmleditorcontroller;1",
      nsHTMLEditorControllerConstructor, },

    { "HTML Editor DocState Controller", NS_EDITORDOCSTATECONTROLLER_CID,
      "@mozilla.org/editor/editordocstatecontroller;1",
      nsHTMLEditorDocStateControllerConstructor, },

    { "HTML Editor command table", NS_HTMLEDITOR_COMMANDTABLE_CID,
      "", 
      nsHTMLEditorCommandTableConstructor, },

    { "HTML Editor doc state command table", NS_HTMLEDITOR_DOCSTATE_COMMANDTABLE_CID,
      "", 
      nsHTMLEditorDocStateCommandTableConstructor, },

    { "Editing Session", NS_EDITINGSESSION_CID,
      "@mozilla.org/editor/editingsession;1", nsEditingSessionConstructor, },

    { "Editor Spell Checker", NS_EDITORSPELLCHECK_CID,
      "@mozilla.org/editor/editorspellchecker;1",
      nsEditorSpellCheckConstructor,},

    { "TxtSrv Filter", NS_COMPOSERTXTSRVFILTER_CID,
      COMPOSER_TXTSRVFILTER_CONTRACTID,
      nsComposeTxtSrvFilterConstructorForComposer, },

    { "TxtSrv Filter For Mail", NS_COMPOSERTXTSRVFILTERMAIL_CID,
      COMPOSER_TXTSRVFILTERMAIL_CONTRACTID,
      nsComposeTxtSrvFilterConstructorForMail, },
};





NS_IMPL_NSGETMODULE(nsComposerModule, components)

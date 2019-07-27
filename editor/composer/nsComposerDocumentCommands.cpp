





#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsCRT.h"                      
#include "nsComposerCommands.h"         
#include "nsDebug.h"                    
#include "nsError.h"                    
#include "nsICommandParams.h"           
#include "nsIDOMDocument.h"             
#include "nsIDocShell.h"                
#include "nsIDocument.h"                
#include "nsIEditingSession.h"          
#include "nsIEditor.h"                  
#include "nsIHTMLEditor.h"              
#include "nsIHTMLInlineTableEditor.h"   
#include "nsIHTMLObjectResizer.h"       
#include "nsIPlaintextEditor.h"         
#include "nsIPresShell.h"               
#include "nsISelectionController.h"     
#include "nsISupportsImpl.h"            
#include "nsISupportsUtils.h"           
#include "nsIURI.h"                     
#include "nsPresContext.h"              
#include "nscore.h"                     

class nsISupports;


#define STATE_ENABLED  "state_enabled"
#define STATE_ALL "state_all"
#define STATE_ATTRIBUTE "state_attribute"
#define STATE_DATA "state_data"

static
nsresult
GetPresContextFromEditor(nsIEditor *aEditor, nsPresContext **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nullptr;
  NS_ENSURE_ARG_POINTER(aEditor);

  nsCOMPtr<nsISelectionController> selCon;
  nsresult rv = aEditor->GetSelectionController(getter_AddRefs(selCon));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  nsCOMPtr<nsIPresShell> presShell = do_QueryInterface(selCon);
  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

  NS_IF_ADDREF(*aResult = presShell->GetPresContext());
  return NS_OK;
}

NS_IMETHODIMP
nsSetDocumentOptionsCommand::IsCommandEnabled(const char * aCommandName,
                                              nsISupports *refCon,
                                              bool *outCmdEnabled)
{
  NS_ENSURE_ARG_POINTER(outCmdEnabled);
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(refCon);
  if (editor)
    return editor->GetIsSelectionEditable(outCmdEnabled);

  *outCmdEnabled = false;
  return NS_OK;
}

NS_IMETHODIMP
nsSetDocumentOptionsCommand::DoCommand(const char *aCommandName,
                                       nsISupports *refCon)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSetDocumentOptionsCommand::DoCommandParams(const char *aCommandName,
                                             nsICommandParams *aParams,
                                             nsISupports *refCon)
{
  NS_ENSURE_ARG_POINTER(aParams);

  nsCOMPtr<nsIEditor> editor = do_QueryInterface(refCon);
  NS_ENSURE_TRUE(editor, NS_ERROR_INVALID_ARG);

  nsRefPtr<nsPresContext> presContext;
  nsresult rv = GetPresContextFromEditor(editor, getter_AddRefs(presContext));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

  int32_t animationMode; 
  rv = aParams->GetLongValue("imageAnimation", &animationMode);
  if (NS_SUCCEEDED(rv))
  {
    
    
    presContext->SetImageAnimationMode(animationMode);
  }

  bool allowPlugins; 
  rv = aParams->GetBooleanValue("plugins", &allowPlugins);
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIDocShell> docShell(presContext->GetDocShell());
    NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

    rv = docShell->SetAllowPlugins(allowPlugins);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSetDocumentOptionsCommand::GetCommandStateParams(const char *aCommandName,
                                                   nsICommandParams *aParams,
                                                   nsISupports *refCon)
{
  NS_ENSURE_ARG_POINTER(aParams);
  NS_ENSURE_ARG_POINTER(refCon);

  
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(refCon);
  NS_ENSURE_TRUE(editor, NS_ERROR_INVALID_ARG);

  
  bool outCmdEnabled = false;
  IsCommandEnabled(aCommandName, refCon, &outCmdEnabled);
  nsresult rv = aParams->SetBooleanValue(STATE_ENABLED, outCmdEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsRefPtr<nsPresContext> presContext;
  rv = GetPresContextFromEditor(editor, getter_AddRefs(presContext));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

  int32_t animationMode;
  rv = aParams->GetLongValue("imageAnimation", &animationMode);
  if (NS_SUCCEEDED(rv))
  {
    
    
    rv = aParams->SetLongValue("imageAnimation",
                               presContext->ImageAnimationMode());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  bool allowPlugins = false; 
  rv = aParams->GetBooleanValue("plugins", &allowPlugins);
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIDocShell> docShell(presContext->GetDocShell());
    NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

    allowPlugins = docShell->PluginsAllowedInCurrentDoc();

    rv = aParams->SetBooleanValue("plugins", allowPlugins);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}










NS_IMETHODIMP
nsSetDocumentStateCommand::IsCommandEnabled(const char * aCommandName,
                                            nsISupports *refCon,
                                            bool *outCmdEnabled)
{
  
  NS_ENSURE_ARG_POINTER(outCmdEnabled);
  *outCmdEnabled = true;
  return NS_OK;
}

NS_IMETHODIMP
nsSetDocumentStateCommand::DoCommand(const char *aCommandName,
                                     nsISupports *refCon)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSetDocumentStateCommand::DoCommandParams(const char *aCommandName,
                                           nsICommandParams *aParams,
                                           nsISupports *refCon)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(refCon);
  NS_ENSURE_TRUE(editor, NS_ERROR_INVALID_ARG);

  if (!nsCRT::strcmp(aCommandName, "cmd_setDocumentModified"))
  {
    NS_ENSURE_ARG_POINTER(aParams);

    bool modified; 
    nsresult rv = aParams->GetBooleanValue(STATE_ATTRIBUTE, &modified);

    
    
    NS_ENSURE_SUCCESS(rv, rv);

    if (modified)
      return editor->IncrementModificationCount(1);

    return editor->ResetModificationCount();
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_setDocumentReadOnly"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    bool isReadOnly; 
    nsresult rvRO = aParams->GetBooleanValue(STATE_ATTRIBUTE, &isReadOnly);
    NS_ENSURE_SUCCESS(rvRO, rvRO);

    uint32_t flags;
    editor->GetFlags(&flags);
    if (isReadOnly)
      flags |= nsIPlaintextEditor::eEditorReadonlyMask;
    else
      flags &= ~(nsIPlaintextEditor::eEditorReadonlyMask);

    return editor->SetFlags(flags);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_setDocumentUseCSS"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLEditor> htmleditor = do_QueryInterface(refCon);
    NS_ENSURE_TRUE(htmleditor, NS_ERROR_INVALID_ARG);

    bool desireCSS;
    nsresult rvCSS = aParams->GetBooleanValue(STATE_ATTRIBUTE, &desireCSS);
    NS_ENSURE_SUCCESS(rvCSS, rvCSS);

    return htmleditor->SetIsCSSEnabled(desireCSS);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_insertBrOnReturn"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLEditor> htmleditor = do_QueryInterface(refCon);
    NS_ENSURE_TRUE(htmleditor, NS_ERROR_INVALID_ARG);

    bool insertBrOnReturn;
    nsresult rvBR = aParams->GetBooleanValue(STATE_ATTRIBUTE,
                                              &insertBrOnReturn);
    NS_ENSURE_SUCCESS(rvBR, rvBR);

    return htmleditor->SetReturnInParagraphCreatesNewParagraph(!insertBrOnReturn);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_enableObjectResizing"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLObjectResizer> resizer = do_QueryInterface(refCon);
    NS_ENSURE_TRUE(resizer, NS_ERROR_INVALID_ARG);

    bool enabled;
    nsresult rvOR = aParams->GetBooleanValue(STATE_ATTRIBUTE, &enabled);
    NS_ENSURE_SUCCESS(rvOR, rvOR);

    return resizer->SetObjectResizingEnabled(enabled);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_enableInlineTableEditing"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLInlineTableEditor> editor = do_QueryInterface(refCon);
    NS_ENSURE_TRUE(editor, NS_ERROR_INVALID_ARG);

    bool enabled;
    nsresult rvOR = aParams->GetBooleanValue(STATE_ATTRIBUTE, &enabled);
    NS_ENSURE_SUCCESS(rvOR, rvOR);

    return editor->SetInlineTableEditingEnabled(enabled);
  }

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSetDocumentStateCommand::GetCommandStateParams(const char *aCommandName,
                                                 nsICommandParams *aParams,
                                                 nsISupports *refCon)
{
  NS_ENSURE_ARG_POINTER(aParams);
  NS_ENSURE_ARG_POINTER(refCon);

  
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(refCon);
  NS_ENSURE_TRUE(editor, NS_ERROR_INVALID_ARG);

  
  bool outCmdEnabled = false;
  IsCommandEnabled(aCommandName, refCon, &outCmdEnabled);
  nsresult rv = aParams->SetBooleanValue(STATE_ENABLED, outCmdEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!nsCRT::strcmp(aCommandName, "cmd_setDocumentModified"))
  {
    bool modified;
    rv = editor->GetDocumentModified(&modified);
    NS_ENSURE_SUCCESS(rv, rv);

    return aParams->SetBooleanValue(STATE_ATTRIBUTE, modified);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_setDocumentReadOnly"))
  {
    NS_ENSURE_ARG_POINTER(aParams);

    uint32_t flags;
    editor->GetFlags(&flags);
    bool isReadOnly = flags & nsIPlaintextEditor::eEditorReadonlyMask;
    return aParams->SetBooleanValue(STATE_ATTRIBUTE, isReadOnly);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_setDocumentUseCSS"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLEditor> htmleditor = do_QueryInterface(refCon);
    NS_ENSURE_TRUE(htmleditor, NS_ERROR_INVALID_ARG);

    bool isCSS;
    htmleditor->GetIsCSSEnabled(&isCSS);
    return aParams->SetBooleanValue(STATE_ALL, isCSS);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_insertBrOnReturn"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLEditor> htmleditor = do_QueryInterface(refCon);
    NS_ENSURE_TRUE(htmleditor, NS_ERROR_INVALID_ARG);

    bool createPOnReturn;
    htmleditor->GetReturnInParagraphCreatesNewParagraph(&createPOnReturn);
    return aParams->SetBooleanValue(STATE_ATTRIBUTE, !createPOnReturn);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_enableObjectResizing"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLObjectResizer> resizer = do_QueryInterface(refCon);
    NS_ENSURE_TRUE(resizer, NS_ERROR_INVALID_ARG);

    bool enabled;
    resizer->GetObjectResizingEnabled(&enabled);
    return aParams->SetBooleanValue(STATE_ATTRIBUTE, enabled);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_enableInlineTableEditing"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLInlineTableEditor> editor = do_QueryInterface(refCon);
    NS_ENSURE_TRUE(editor, NS_ERROR_INVALID_ARG);

    bool enabled;
    editor->GetInlineTableEditingEnabled(&enabled);
    return aParams->SetBooleanValue(STATE_ATTRIBUTE, enabled);
  }

  return NS_ERROR_NOT_IMPLEMENTED;
}






































NS_IMETHODIMP
nsDocumentStateCommand::IsCommandEnabled(const char* aCommandName,
                                         nsISupports *refCon,
                                         bool *outCmdEnabled)
{
  NS_ENSURE_ARG_POINTER(outCmdEnabled);
  
  *outCmdEnabled = false;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentStateCommand::DoCommand(const char *aCommandName,
                                  nsISupports *refCon)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocumentStateCommand::DoCommandParams(const char *aCommandName,
                                        nsICommandParams *aParams,
                                        nsISupports *refCon)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocumentStateCommand::GetCommandStateParams(const char *aCommandName,
                                              nsICommandParams *aParams,
                                              nsISupports *refCon)
{
  NS_ENSURE_ARG_POINTER(aParams);
  NS_ENSURE_ARG_POINTER(aCommandName);
  nsresult rv;

  if (!nsCRT::strcmp(aCommandName, "obs_documentCreated"))
  {
    uint32_t editorStatus = nsIEditingSession::eEditorErrorUnknown;

    nsCOMPtr<nsIEditingSession> editingSession = do_QueryInterface(refCon);
    if (editingSession)
    {
      
      
      
      
      
      rv = editingSession->GetEditorStatus(&editorStatus);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else
    {
      
      nsCOMPtr<nsIEditor> editor = do_QueryInterface(refCon);
      if (editor)
        editorStatus = nsIEditingSession::eEditorOK;
    }

    
    
    aParams->SetLongValue(STATE_DATA, editorStatus);
    return NS_OK;
  }  
  else if (!nsCRT::strcmp(aCommandName, "obs_documentLocationChanged"))
  {
    nsCOMPtr<nsIEditor> editor = do_QueryInterface(refCon);
    if (editor)
    {
      nsCOMPtr<nsIDOMDocument> domDoc;
      editor->GetDocument(getter_AddRefs(domDoc));
      nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
      NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

      nsIURI *uri = doc->GetDocumentURI();
      NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);

      return aParams->SetISupportsValue(STATE_DATA, (nsISupports*)uri);
    }
    return NS_OK;
  }  

  return NS_ERROR_NOT_IMPLEMENTED;
}

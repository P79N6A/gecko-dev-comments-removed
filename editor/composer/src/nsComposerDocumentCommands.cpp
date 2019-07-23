









































#include "nsIEditor.h"
#include "nsIEditingSession.h"
#include "nsIPlaintextEditor.h"
#include "nsIHTMLEditor.h"
#include "nsIHTMLObjectResizer.h"
#include "nsIHTMLInlineTableEditor.h"

#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsISelectionController.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIDocShell.h"
#include "nsIURI.h"

#include "nsCOMPtr.h"

#include "nsComposerCommands.h"
#include "nsICommandParams.h"
#include "nsCRT.h"


#define STATE_ENABLED  "state_enabled"
#define STATE_ATTRIBUTE "state_attribute"
#define STATE_DATA "state_data"

static
nsresult
GetPresContextFromEditor(nsIEditor *aEditor, nsPresContext **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;
  NS_ENSURE_ARG_POINTER(aEditor);

  nsCOMPtr<nsISelectionController> selCon;
  nsresult rv = aEditor->GetSelectionController(getter_AddRefs(selCon));
  if (NS_FAILED(rv)) return rv;
  if (!selCon) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresShell> presShell = do_QueryInterface(selCon);
  if (!presShell) return NS_ERROR_FAILURE;

  NS_IF_ADDREF(*aResult = presShell->GetPresContext());
  return NS_OK;
}

NS_IMETHODIMP
nsSetDocumentOptionsCommand::IsCommandEnabled(const char * aCommandName,
                                              nsISupports *refCon,
                                              PRBool *outCmdEnabled)
{
  NS_ENSURE_ARG_POINTER(outCmdEnabled);
  nsCOMPtr<nsIHTMLEditor> editor = do_QueryInterface(refCon);
  *outCmdEnabled = (editor != nsnull);
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
  if (!editor) return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsPresContext> presContext;
  nsresult rv = GetPresContextFromEditor(editor, getter_AddRefs(presContext));
  if (NS_FAILED(rv)) return rv;
  if (!presContext) return NS_ERROR_FAILURE;

  PRInt32 animationMode; 
  rv = aParams->GetLongValue("imageAnimation", &animationMode);
  if (NS_SUCCEEDED(rv))
  {
    
    
    presContext->SetImageAnimationMode(animationMode);
  }

  PRBool allowPlugins; 
  rv = aParams->GetBooleanValue("plugins", &allowPlugins);
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsISupports> container = presContext->GetContainer();
    if (!container) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container, &rv));
    if (NS_FAILED(rv)) return rv;
    if (!docShell) return NS_ERROR_FAILURE;

    rv = docShell->SetAllowPlugins(allowPlugins);
    if (NS_FAILED(rv)) return rv;
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
  if (!editor) return NS_ERROR_INVALID_ARG;

  
  PRBool outCmdEnabled = PR_FALSE;
  IsCommandEnabled(aCommandName, refCon, &outCmdEnabled);
  nsresult rv = aParams->SetBooleanValue(STATE_ENABLED, outCmdEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsPresContext> presContext;
  rv = GetPresContextFromEditor(editor, getter_AddRefs(presContext));
  if (NS_FAILED(rv)) return rv;
  if (!presContext) return NS_ERROR_FAILURE;

  PRInt32 animationMode;
  rv = aParams->GetLongValue("imageAnimation", &animationMode);
  if (NS_SUCCEEDED(rv))
  {
    
    
    rv = aParams->SetLongValue("imageAnimation",
                               presContext->ImageAnimationMode());
    if (NS_FAILED(rv)) return rv;
  }

  PRBool allowPlugins; 
  rv = aParams->GetBooleanValue("plugins", &allowPlugins);
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsISupports> container = presContext->GetContainer();
    if (!container) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container, &rv));
    if (NS_FAILED(rv)) return rv;
    if (!docShell) return NS_ERROR_FAILURE;

    rv = docShell->GetAllowPlugins(&allowPlugins);
    if (NS_FAILED(rv)) return rv;

    rv = aParams->SetBooleanValue("plugins", allowPlugins);
    if (NS_FAILED(rv)) return rv;
  }

  return NS_OK;
}










NS_IMETHODIMP
nsSetDocumentStateCommand::IsCommandEnabled(const char * aCommandName,
                                            nsISupports *refCon,
                                            PRBool *outCmdEnabled)
{
  NS_ENSURE_ARG_POINTER(outCmdEnabled);
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(refCon);
  *outCmdEnabled = (editor != nsnull);
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
  if (!editor) 
    return NS_ERROR_INVALID_ARG;

  if (!nsCRT::strcmp(aCommandName, "cmd_setDocumentModified"))
  {
    NS_ENSURE_ARG_POINTER(aParams);

    PRBool modified; 
    nsresult rv = aParams->GetBooleanValue(STATE_ATTRIBUTE, &modified);

    
    
    if (NS_FAILED(rv))
      return rv;

    if (modified)
      return editor->IncrementModificationCount(1);

    return editor->ResetModificationCount();
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_setDocumentReadOnly"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    PRBool isReadOnly; 
    nsresult rvRO = aParams->GetBooleanValue(STATE_ATTRIBUTE, &isReadOnly);
    if (NS_FAILED(rvRO))
      return rvRO;

    PRUint32 flags;
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
    if (!htmleditor) 
      return NS_ERROR_INVALID_ARG;

    PRBool desireCSS;
    nsresult rvCSS = aParams->GetBooleanValue(STATE_ATTRIBUTE, &desireCSS);
    if (NS_FAILED(rvCSS))
      return rvCSS;

    return htmleditor->SetIsCSSEnabled(desireCSS);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_insertBrOnReturn"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLEditor> htmleditor = do_QueryInterface(refCon);
    if (!htmleditor) 
      return NS_ERROR_INVALID_ARG;

    PRBool insertBrOnReturn;
    nsresult rvBR = aParams->GetBooleanValue(STATE_ATTRIBUTE,
                                              &insertBrOnReturn);
    if (NS_FAILED(rvBR))
      return rvBR;

    return htmleditor->SetReturnInParagraphCreatesNewParagraph(!insertBrOnReturn);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_enableObjectResizing"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLObjectResizer> resizer = do_QueryInterface(refCon);
    if (!resizer)
      return NS_ERROR_INVALID_ARG;

    PRBool enabled;
    nsresult rvOR = aParams->GetBooleanValue(STATE_ATTRIBUTE, &enabled);
    if (NS_FAILED(rvOR))
      return rvOR;

    return resizer->SetObjectResizingEnabled(enabled);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_enableInlineTableEditing"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLInlineTableEditor> editor = do_QueryInterface(refCon);
    if (!editor)
      return NS_ERROR_INVALID_ARG;

    PRBool enabled;
    nsresult rvOR = aParams->GetBooleanValue(STATE_ATTRIBUTE, &enabled);
    if (NS_FAILED(rvOR))
      return rvOR;

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
  if (!editor) 
    return NS_ERROR_INVALID_ARG;

  
  PRBool outCmdEnabled = PR_FALSE;
  IsCommandEnabled(aCommandName, refCon, &outCmdEnabled);
  nsresult rv = aParams->SetBooleanValue(STATE_ENABLED, outCmdEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!nsCRT::strcmp(aCommandName, "cmd_setDocumentModified"))
  {
    PRBool modified;
    rv = editor->GetDocumentModified(&modified);
    NS_ENSURE_SUCCESS(rv, rv);

    return aParams->SetBooleanValue(STATE_ATTRIBUTE, modified);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_setDocumentReadOnly"))
  {
    NS_ENSURE_ARG_POINTER(aParams);

    PRUint32 flags;
    editor->GetFlags(&flags);
    PRBool isReadOnly = flags & nsIPlaintextEditor::eEditorReadonlyMask;
    return aParams->SetBooleanValue(STATE_ATTRIBUTE, isReadOnly);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_setDocumentUseCSS"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLEditor> htmleditor = do_QueryInterface(refCon);
    if (!htmleditor)
      return NS_ERROR_INVALID_ARG;

    PRBool isCSS;
    htmleditor->GetIsCSSEnabled(&isCSS);
    return aParams->SetBooleanValue(STATE_ATTRIBUTE, isCSS);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_insertBrOnReturn"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLEditor> htmleditor = do_QueryInterface(refCon);
    if (!htmleditor)
      return NS_ERROR_INVALID_ARG;

    PRBool createPOnReturn;
    htmleditor->GetReturnInParagraphCreatesNewParagraph(&createPOnReturn);
    return aParams->SetBooleanValue(STATE_ATTRIBUTE, !createPOnReturn);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_enableObjectResizing"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLObjectResizer> resizer = do_QueryInterface(refCon);
    if (!resizer)
      return NS_ERROR_INVALID_ARG;

    PRBool enabled;
    resizer->GetObjectResizingEnabled(&enabled);
    return aParams->SetBooleanValue(STATE_ATTRIBUTE, enabled);
  }

  if (!nsCRT::strcmp(aCommandName, "cmd_enableInlineTableEditing"))
  {
    NS_ENSURE_ARG_POINTER(aParams);
    nsCOMPtr<nsIHTMLInlineTableEditor> editor = do_QueryInterface(refCon);
    if (!editor)
      return NS_ERROR_INVALID_ARG;

    PRBool enabled;
    editor->GetInlineTableEditingEnabled(&enabled);
    return aParams->SetBooleanValue(STATE_ATTRIBUTE, enabled);
  }

  return NS_ERROR_NOT_IMPLEMENTED;
}






































#ifdef XP_MAC
#pragma mark -
#endif

NS_IMETHODIMP
nsDocumentStateCommand::IsCommandEnabled(const char* aCommandName,
                                         nsISupports *refCon,
                                         PRBool *outCmdEnabled)
{
  NS_ENSURE_ARG_POINTER(outCmdEnabled);
  
  *outCmdEnabled = PR_FALSE;
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
    PRUint32 editorStatus = nsIEditingSession::eEditorErrorUnknown;

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
      if (!doc) return NS_ERROR_FAILURE;

      nsIURI *uri = doc->GetDocumentURI();
      if (!uri) return NS_ERROR_FAILURE;

      return aParams->SetISupportsValue(STATE_DATA, (nsISupports*)uri);
    }
    return NS_OK;
  }  

  return NS_ERROR_NOT_IMPLEMENTED;
}

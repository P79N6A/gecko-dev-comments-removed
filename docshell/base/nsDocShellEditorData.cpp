







































#include "nsIComponentManager.h"
#include "nsIInterfaceRequestorUtils.h"

#include "nsIDOMWindow.h"
#include "nsIDocShellTreeItem.h"

#include "nsDocShellEditorData.h"








nsDocShellEditorData::nsDocShellEditorData(nsIDocShell* inOwningDocShell)
: mDocShell(inOwningDocShell)
, mMakeEditable(PR_FALSE)
{
  NS_ASSERTION(mDocShell, "Where is my docShell?");
}







nsDocShellEditorData::~nsDocShellEditorData()
{
  if (mEditingSession)
  {
    nsCOMPtr<nsIDOMWindow> domWindow = do_GetInterface(mDocShell);
    
    
    mEditingSession->TearDownEditorOnWindow(domWindow);
  }
  else if (mEditor) 
  {
    mEditor->PreDestroy();
    mEditor = nsnull;     
  }
}







nsresult
nsDocShellEditorData::MakeEditable(PRBool inWaitForUriLoad )
{
  if (mMakeEditable)
    return NS_OK;
  
  
  
  if (mEditor)
  {
    NS_WARNING("Destroying existing editor on frame");
    
    mEditor->PreDestroy();
    mEditor = nsnull;
  }
  
  mMakeEditable = PR_TRUE;
  return NS_OK;
}







PRBool
nsDocShellEditorData::GetEditable()
{
  return mMakeEditable || (mEditor != nsnull);
}






nsresult
nsDocShellEditorData::CreateEditor()
{
  nsCOMPtr<nsIEditingSession>   editingSession;    
  nsresult rv = GetEditingSession(getter_AddRefs(editingSession));
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIDOMWindow>    domWindow = do_GetInterface(mDocShell);
  rv = editingSession->SetupEditorOnWindow(domWindow);
  if (NS_FAILED(rv)) return rv;
  
  return NS_OK;
}







nsresult
nsDocShellEditorData::GetEditingSession(nsIEditingSession **outEditingSession)
{
  nsresult rv = EnsureEditingSession();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*outEditingSession = mEditingSession);

  return NS_OK;
}







nsresult
nsDocShellEditorData::GetEditor(nsIEditor **outEditor)
{
  NS_ENSURE_ARG_POINTER(outEditor);
  NS_IF_ADDREF(*outEditor = mEditor);
  return NS_OK;
}







nsresult
nsDocShellEditorData::SetEditor(nsIEditor *inEditor)
{
  
  
  
  if (mEditor.get() != inEditor)
  {
    if (mEditor)
    {
      mEditor->PreDestroy();
      mEditor = nsnull;
    }
      
    mEditor = inEditor;    
  }   
  
  return NS_OK;
}










nsresult
nsDocShellEditorData::EnsureEditingSession()
{
  NS_ASSERTION(mDocShell, "Should have docShell here");
  
  nsresult rv = NS_OK;
  
  if (!mEditingSession)
  {
    mEditingSession =
      do_CreateInstance("@mozilla.org/editor/editingsession;1", &rv);
  }

  return rv;
}


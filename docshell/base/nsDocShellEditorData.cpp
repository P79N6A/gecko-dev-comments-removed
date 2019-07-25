







































#include "nsIComponentManager.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDOMWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMDocument.h"
#include "nsDocShellEditorData.h"







nsDocShellEditorData::nsDocShellEditorData(nsIDocShell* inOwningDocShell)
: mDocShell(inOwningDocShell)
, mMakeEditable(PR_FALSE)
, mIsDetached(PR_FALSE)
, mDetachedMakeEditable(PR_FALSE)
, mDetachedEditingState(nsIHTMLDocument::eOff)
{
  NS_ASSERTION(mDocShell, "Where is my docShell?");
}







nsDocShellEditorData::~nsDocShellEditorData()
{
  TearDownEditor();
}

void
nsDocShellEditorData::TearDownEditor()
{
  if (mEditor) {
    mEditor->PreDestroy(PR_FALSE);
    mEditor = nsnull;
  }
  mEditingSession = nsnull;
  mIsDetached = PR_FALSE;
}







nsresult
nsDocShellEditorData::MakeEditable(bool inWaitForUriLoad)
{
  if (mMakeEditable)
    return NS_OK;
  
  
  
  if (mEditor)
  {
    NS_WARNING("Destroying existing editor on frame");
    
    mEditor->PreDestroy(PR_FALSE);
    mEditor = nsnull;
  }
  
  if (inWaitForUriLoad)
    mMakeEditable = PR_TRUE;
  return NS_OK;
}







bool
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
      mEditor->PreDestroy(PR_FALSE);
      mEditor = nsnull;
    }
      
    mEditor = inEditor;    
    if (!mEditor)
      mMakeEditable = PR_FALSE;
  }   
  
  return NS_OK;
}










nsresult
nsDocShellEditorData::EnsureEditingSession()
{
  NS_ASSERTION(mDocShell, "Should have docShell here");
  NS_ASSERTION(!mIsDetached, "This will stomp editing session!");
  
  nsresult rv = NS_OK;
  
  if (!mEditingSession)
  {
    mEditingSession =
      do_CreateInstance("@mozilla.org/editor/editingsession;1", &rv);
  }

  return rv;
}

nsresult
nsDocShellEditorData::DetachFromWindow()
{
  NS_ASSERTION(mEditingSession,
               "Can't detach when we don't have a session to detach!");
  
  nsCOMPtr<nsIDOMWindow> domWindow = do_GetInterface(mDocShell);
  nsresult rv = mEditingSession->DetachFromWindow(domWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  mIsDetached = PR_TRUE;
  mDetachedMakeEditable = mMakeEditable;
  mMakeEditable = PR_FALSE;

  nsCOMPtr<nsIDOMDocument> domDoc;
  domWindow->GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(domDoc);
  if (htmlDoc)
    mDetachedEditingState = htmlDoc->GetEditingState();

  mDocShell = nsnull;

  return NS_OK;
}

nsresult
nsDocShellEditorData::ReattachToWindow(nsIDocShell* aDocShell)
{
  mDocShell = aDocShell;

  nsCOMPtr<nsIDOMWindow> domWindow = do_GetInterface(mDocShell);
  nsresult rv = mEditingSession->ReattachToWindow(domWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  mIsDetached = PR_FALSE;
  mMakeEditable = mDetachedMakeEditable;

  nsCOMPtr<nsIDOMDocument> domDoc;
  domWindow->GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(domDoc);
  if (htmlDoc)
    htmlDoc->SetEditingState(mDetachedEditingState);
 
  return NS_OK;
}

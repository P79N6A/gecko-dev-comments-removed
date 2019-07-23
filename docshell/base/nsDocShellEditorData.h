





































#ifndef nsDocShellEditorData_h__
#define nsDocShellEditorData_h__

#ifndef nsCOMPtr_h___
#include "nsCOMPtr.h"
#endif

#ifndef __gen_nsIDocShell_h__
#include "nsIDocShell.h"
#endif

#ifndef __gen_nsIEditingSession_h__
#include "nsIEditingSession.h"
#endif


#include "nsIHTMLDocument.h"
#include "nsIEditor.h"

class nsIDOMWindow;

class nsDocShellEditorData
{
public:

  nsDocShellEditorData(nsIDocShell* inOwningDocShell);
  ~nsDocShellEditorData();

  nsresult MakeEditable(PRBool inWaitForUriLoad);
  PRBool GetEditable();
  nsresult CreateEditor();
  nsresult GetEditingSession(nsIEditingSession **outEditingSession);
  nsresult GetEditor(nsIEditor **outEditor);
  nsresult SetEditor(nsIEditor *inEditor);
  void TearDownEditor();
  nsresult DetachFromWindow();
  nsresult ReattachToWindow(nsIDocShell *aDocShell);
  PRBool WaitingForLoad() const { return mMakeEditable; }

protected:

  nsresult EnsureEditingSession();

  
  nsIDocShell* mDocShell;

  
  nsCOMPtr<nsIEditingSession> mEditingSession;

  
  PRBool mMakeEditable;
  
  
  nsCOMPtr<nsIEditor> mEditor;

  
  
  PRBool mIsDetached;

  
  PRBool mDetachedMakeEditable;

  
  
  nsIHTMLDocument::EditingState mDetachedEditingState;

};


#endif 

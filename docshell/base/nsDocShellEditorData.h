




#ifndef nsDocShellEditorData_h__
#define nsDocShellEditorData_h__

#ifndef nsCOMPtr_h___
#include "nsCOMPtr.h"
#endif

#include "nsIHTMLDocument.h"

class nsIDocShell;
class nsIEditingSession;
class nsIEditor;

class nsDocShellEditorData
{
public:
  explicit nsDocShellEditorData(nsIDocShell* aOwningDocShell);
  ~nsDocShellEditorData();

  nsresult MakeEditable(bool aWaitForUriLoad);
  bool GetEditable();
  nsresult CreateEditor();
  nsresult GetEditingSession(nsIEditingSession** aResult);
  nsresult GetEditor(nsIEditor** aResult);
  nsresult SetEditor(nsIEditor* aEditor);
  void TearDownEditor();
  nsresult DetachFromWindow();
  nsresult ReattachToWindow(nsIDocShell* aDocShell);
  bool WaitingForLoad() const { return mMakeEditable; }

protected:
  nsresult EnsureEditingSession();

  
  nsIDocShell* mDocShell;

  
  nsCOMPtr<nsIEditingSession> mEditingSession;

  
  bool mMakeEditable;

  
  nsCOMPtr<nsIEditor> mEditor;

  
  
  bool mIsDetached;

  
  bool mDetachedMakeEditable;

  
  
  nsIHTMLDocument::EditingState mDetachedEditingState;

};

#endif 

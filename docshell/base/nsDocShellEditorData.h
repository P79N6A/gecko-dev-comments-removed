





































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


#include "nsIEditor.h"





class nsDocShellEditorData
{
public:

              nsDocShellEditorData(nsIDocShell* inOwningDocShell);
              ~nsDocShellEditorData();
              

              
  nsresult    MakeEditable(PRBool inWaitForUriLoad);
  
  PRBool      GetEditable();
  
              
  nsresult    CreateEditor();
  
              
              
  nsresult    GetEditingSession(nsIEditingSession **outEditingSession);
  
              
  nsresult    GetEditor(nsIEditor **outEditor);
  
              
  nsresult    SetEditor(nsIEditor *inEditor);

protected:              

  nsresult    EnsureEditingSession();

protected:

  nsIDocShell*                mDocShell;        
  
  nsCOMPtr<nsIEditingSession> mEditingSession;  

  PRBool                      mMakeEditable;    
  nsCOMPtr<nsIEditor>         mEditor;          

};


#endif 







































#ifndef nsEditingSession_h__
#define nsEditingSession_h__


#ifndef nsWeakReference_h__
#include "nsWeakReference.h"
#endif

#include "nsITimer.h"

#ifndef __gen_nsIWebProgressListener_h__
#include "nsIWebProgressListener.h"
#endif

#ifndef __gen_nsIEditingSession_h__
#include "nsIEditingSession.h"
#endif

#include "nsString.h"

#define NS_EDITINGSESSION_CID                            \
{ 0xbc26ff01, 0xf2bd, 0x11d4, { 0xa7, 0x3c, 0xe5, 0xa4, 0xb5, 0xa8, 0xbd, 0xfc } }


class nsIWebProgress;
class nsIDocShell;
class nsIEditorDocShell;
class nsIChannel;
class nsIEditor;
class nsIControllers;

class nsComposerCommandsUpdater;

class nsEditingSession : public nsIEditingSession,
                         public nsIWebProgressListener,
                         public nsSupportsWeakReference
{
public:

                  nsEditingSession();
  virtual         ~nsEditingSession();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIWEBPROGRESSLISTENER
  
  
  NS_DECL_NSIEDITINGSESSION

protected:

  nsIDocShell *   GetDocShellFromWindow(nsIDOMWindow *aWindow);
  nsresult        GetEditorDocShellFromWindow(nsIDOMWindow *aWindow, 
                                              nsIEditorDocShell** outDocShell);
  
  nsresult        SetupEditorCommandController(const char *aControllerClassName,
                                               nsIDOMWindow *aWindow,
                                               nsISupports *aContext,
                                               PRUint32 *aControllerId);

  nsresult        SetContextOnControllerById(nsIControllers* aControllers, 
                                            nsISupports* aContext,
                                            PRUint32 aID);

  nsresult        PrepareForEditing(nsIDOMWindow *aWindow);

  static void     TimerCallback(nsITimer *aTimer, void *aClosure);
  nsCOMPtr<nsITimer>  mLoadBlankDocTimer;
  
  
  nsresult        StartDocumentLoad(nsIWebProgress *aWebProgress,
                                    PRBool isToBeMadeEditable);
  nsresult        EndDocumentLoad(nsIWebProgress *aWebProgress, 
                                  nsIChannel* aChannel, nsresult aStatus,
                                  PRBool isToBeMadeEditable);
  nsresult        StartPageLoad(nsIChannel *aChannel);
  nsresult        EndPageLoad(nsIWebProgress *aWebProgress, 
                              nsIChannel* aChannel, nsresult aStatus);
  
  PRBool          IsProgressForTargetDocument(nsIWebProgress *aWebProgress);

protected:

  PRPackedBool    mDoneSetup;    

  
  
  
  
  PRPackedBool    mCanCreateEditor; 

  PRPackedBool    mInteractive;

  
  
  PRPackedBool    mScriptsEnabled;

  
  
  PRPackedBool    mPluginsEnabled;

  PRPackedBool    mProgressListenerRegistered;

  
  PRUint16        mImageAnimationMode;

  
  
  nsCOMPtr<nsISupports> mStateMaintainer;  
  
  nsWeakPtr       mWindowToBeEdited;

  
  nsCString       mEditorType; 
  PRUint32        mEditorFlags;
  PRUint32        mEditorStatus;
  PRUint32        mBaseCommandControllerId;
  PRUint32        mDocStateControllerId;
  PRUint32        mHTMLCommandControllerId;
};



#endif 

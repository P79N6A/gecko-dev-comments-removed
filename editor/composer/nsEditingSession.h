




#ifndef nsEditingSession_h__
#define nsEditingSession_h__


#ifndef nsWeakReference_h__
#include "nsWeakReference.h"            
#endif

#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            
#include "nsIWeakReferenceUtils.h"      
#include "nsWeakReference.h"            
#include "nscore.h"                     

#ifndef __gen_nsIWebProgressListener_h__
#include "nsIWebProgressListener.h"
#endif

#ifndef __gen_nsIEditingSession_h__
#include "nsIEditingSession.h"          
#endif

#include "nsString.h"                   

class nsIDOMWindow;
class nsISupports;
class nsITimer;

#define NS_EDITINGSESSION_CID                            \
{ 0xbc26ff01, 0xf2bd, 0x11d4, { 0xa7, 0x3c, 0xe5, 0xa4, 0xb5, 0xa8, 0xbd, 0xfc } }


class nsComposerCommandsUpdater;
class nsIChannel;
class nsIControllers;
class nsIDocShell;
class nsIEditor;
class nsIWebProgress;

class nsEditingSession MOZ_FINAL : public nsIEditingSession,
                                   public nsIWebProgressListener,
                                   public nsSupportsWeakReference
{
public:

  nsEditingSession();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIWEBPROGRESSLISTENER
  
  
  NS_DECL_NSIEDITINGSESSION

protected:
  virtual         ~nsEditingSession();

  nsIDocShell *   GetDocShellFromWindow(nsIDOMWindow *aWindow);
  
  nsresult        SetupEditorCommandController(const char *aControllerClassName,
                                               nsIDOMWindow *aWindow,
                                               nsISupports *aContext,
                                               uint32_t *aControllerId);

  nsresult        SetContextOnControllerById(nsIControllers* aControllers, 
                                            nsISupports* aContext,
                                            uint32_t aID);

  nsresult        PrepareForEditing(nsIDOMWindow *aWindow);

  static void     TimerCallback(nsITimer *aTimer, void *aClosure);
  nsCOMPtr<nsITimer>  mLoadBlankDocTimer;
  
  
  nsresult        StartDocumentLoad(nsIWebProgress *aWebProgress,
                                    bool isToBeMadeEditable);
  nsresult        EndDocumentLoad(nsIWebProgress *aWebProgress, 
                                  nsIChannel* aChannel, nsresult aStatus,
                                  bool isToBeMadeEditable);
  nsresult        StartPageLoad(nsIChannel *aChannel);
  nsresult        EndPageLoad(nsIWebProgress *aWebProgress, 
                              nsIChannel* aChannel, nsresult aStatus);
  
  bool            IsProgressForTargetDocument(nsIWebProgress *aWebProgress);

  void            RemoveEditorControllers(nsIDOMWindow *aWindow);
  void            RemoveWebProgressListener(nsIDOMWindow *aWindow);
  void            RestoreAnimationMode(nsIDOMWindow *aWindow);
  void            RemoveListenersAndControllers(nsIDOMWindow *aWindow,
                                                nsIEditor *aEditor);

protected:

  bool            mDoneSetup;    

  
  
  
  
  bool            mCanCreateEditor; 

  bool            mInteractive;
  bool            mMakeWholeDocumentEditable;

  bool            mDisabledJSAndPlugins;

  
  
  bool            mScriptsEnabled;

  
  
  bool            mPluginsEnabled;

  bool            mProgressListenerRegistered;

  
  uint16_t        mImageAnimationMode;

  
  
  nsRefPtr<nsComposerCommandsUpdater> mStateMaintainer;
  
  
  nsCString       mEditorType; 
  uint32_t        mEditorFlags;
  uint32_t        mEditorStatus;
  uint32_t        mBaseCommandControllerId;
  uint32_t        mDocStateControllerId;
  uint32_t        mHTMLCommandControllerId;

  
  nsWeakPtr       mDocShell;

  
  nsWeakPtr       mExistingEditor;
};



#endif 

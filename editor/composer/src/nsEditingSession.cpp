









































#include "nsPIDOMWindow.h"
#include "nsIDOMWindowUtils.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMNSHTMLDocument.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMDocument.h"
#include "nsIURI.h"
#include "nsISelectionPrivate.h"
#include "nsITransactionManager.h"

#include "nsIEditorDocShell.h"
#include "nsIDocShell.h"

#include "nsIChannel.h"
#include "nsIWebProgress.h"
#include "nsIWebNavigation.h"
#include "nsIRefreshURI.h"

#include "nsIControllers.h"
#include "nsIController.h"
#include "nsIControllerContext.h"
#include "nsICommandManager.h"
#include "nsPICommandUpdater.h"

#include "nsIPresShell.h"

#include "nsComposerCommandsUpdater.h"
#include "nsEditingSession.h"

#include "nsComponentManagerUtils.h"
#include "nsIInterfaceRequestorUtils.h"

#include "nsIContentViewer.h"
#include "nsISelectionController.h"
#include "nsIPlaintextEditor.h"
#include "nsIEditor.h"

#include "nsIDOMNSDocument.h"
#include "nsIScriptContext.h"
#include "imgIContainer.h"
#include "nsPresContext.h"

#if DEBUG

#endif






nsEditingSession::nsEditingSession()
: mDoneSetup(PR_FALSE)
, mCanCreateEditor(PR_FALSE)
, mInteractive(PR_FALSE)
, mMakeWholeDocumentEditable(PR_TRUE)
, mScriptsEnabled(PR_TRUE)
, mPluginsEnabled(PR_TRUE)
, mProgressListenerRegistered(PR_FALSE)
, mImageAnimationMode(0)
, mEditorFlags(0)
, mEditorStatus(eEditorOK)
, mBaseCommandControllerId(0)
, mDocStateControllerId(0)
, mHTMLCommandControllerId(0)
{
}






nsEditingSession::~nsEditingSession()
{
  
  if (mLoadBlankDocTimer)
    mLoadBlankDocTimer->Cancel();
}

NS_IMPL_ISUPPORTS3(nsEditingSession, nsIEditingSession, nsIWebProgressListener, 
                   nsISupportsWeakReference)











#define DEFAULT_EDITOR_TYPE "html"

NS_IMETHODIMP
nsEditingSession::MakeWindowEditable(nsIDOMWindow *aWindow,
                                     const char *aEditorType, 
                                     PRBool aDoAfterUriLoad,
                                     PRBool aMakeWholeDocumentEditable,
                                     PRBool aInteractive)
{
  mEditorType.Truncate();
  mEditorFlags = 0;
  mWindowToBeEdited = do_GetWeakReference(aWindow);

  
  nsIDocShell *docShell = GetDocShellFromWindow(aWindow);
  if (!docShell) return NS_ERROR_FAILURE;

  mInteractive = aInteractive;
  mMakeWholeDocumentEditable = aMakeWholeDocumentEditable;

  nsresult rv;
  if (!mInteractive) {
    
    PRBool tmp;
    rv = docShell->GetAllowJavascript(&tmp);
    NS_ENSURE_SUCCESS(rv, rv);

    mScriptsEnabled = tmp;

    rv = docShell->SetAllowJavascript(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = docShell->GetAllowPlugins(&tmp);
    NS_ENSURE_SUCCESS(rv, rv);

    mPluginsEnabled = tmp;

    rv = docShell->SetAllowPlugins(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  TearDownEditorOnWindow(aWindow, PR_FALSE);
  
  
  mEditorStatus = eEditorCreationInProgress;

  
  if (!aEditorType)
    aEditorType = DEFAULT_EDITOR_TYPE;
  mEditorType = aEditorType;

  
  
  rv = PrepareForEditing(aWindow);
  if (NS_FAILED(rv)) return rv;  
  
  nsCOMPtr<nsIEditorDocShell> editorDocShell;
  rv = GetEditorDocShellFromWindow(aWindow, getter_AddRefs(editorDocShell));
  if (NS_FAILED(rv)) return rv;  
  
  
  rv = editorDocShell->MakeEditable(aDoAfterUriLoad);
  if (NS_FAILED(rv)) return rv;  

  
  
  
  rv = SetupEditorCommandController("@mozilla.org/editor/editorcontroller;1",
                                    aWindow,
                                    NS_STATIC_CAST(nsIEditingSession*, this),
                                    &mBaseCommandControllerId);
  if (NS_FAILED(rv)) return rv;

  
  
  rv = SetupEditorCommandController("@mozilla.org/editor/editordocstatecontroller;1",
                                    aWindow,
                                    NS_STATIC_CAST(nsIEditingSession*, this),
                                    &mDocStateControllerId);
  if (NS_FAILED(rv)) return rv;

  
  if (!aDoAfterUriLoad)
  {
    rv = SetupEditorOnWindow(aWindow);

    
    
    
    if (NS_FAILED(rv))
      TearDownEditorOnWindow(aWindow, PR_FALSE);
  }
  return rv;
}







NS_IMETHODIMP
nsEditingSession::WindowIsEditable(nsIDOMWindow *aWindow, PRBool *outIsEditable)
{
  nsCOMPtr<nsIEditorDocShell> editorDocShell;
  nsresult rv = GetEditorDocShellFromWindow(aWindow,
                                            getter_AddRefs(editorDocShell));
  if (NS_FAILED(rv)) return rv;  

  return editorDocShell->GetEditable(outIsEditable);
}







const char* const gSupportedTextTypes[] = {
  "text/plain",
  "text/css",
  "text/rdf",
  "text/xsl",
  "text/javascript",           
  "text/ecmascript",           
  "application/javascript",
  "application/ecmascript",
  "application/x-javascript",  
  "text/xul",                  
  "application/vnd.mozilla.xul+xml",
  NULL      
};

PRBool
IsSupportedTextType(const char* aMIMEType)
{
  if (!aMIMEType)
    return PR_FALSE;

  PRInt32 i = 0;
  while (gSupportedTextTypes[i])
  {
    if (strcmp(gSupportedTextTypes[i], aMIMEType) == 0)
    {
      return PR_TRUE;
    }

    i ++;
  }
  
  return PR_FALSE;
}







NS_IMETHODIMP
nsEditingSession::SetupEditorOnWindow(nsIDOMWindow *aWindow)
{
  mDoneSetup = PR_TRUE;

  nsresult rv;

  
  
  
  
  nsCOMPtr<nsIDOMDocument> doc;
  nsCAutoString mimeCType;

  
  if (NS_SUCCEEDED(aWindow->GetDocument(getter_AddRefs(doc))) && doc)
  {
    nsCOMPtr<nsIDOMNSDocument> nsdoc = do_QueryInterface(doc);
    if (nsdoc)
    {
      nsAutoString mimeType;
      if (NS_SUCCEEDED(nsdoc->GetContentType(mimeType)))
        AppendUTF16toUTF8(mimeType, mimeCType);

      if (IsSupportedTextType(mimeCType.get()))
      {
        mEditorType.AssignLiteral("text");
        mimeCType = "text/plain";
      }
      else if (!mimeCType.EqualsLiteral("text/html"))
      {
        
        mEditorStatus = eEditorErrorCantEditMimeType;

        
        mEditorType.AssignLiteral("html");
        mimeCType.AssignLiteral("text/html");
      }
    }

    
    
    nsCOMPtr<nsIDocument> document(do_QueryInterface(doc));
    if (document) {
      document->FlushPendingNotifications(Flush_Frames);
      if (mMakeWholeDocumentEditable) {
        document->SetEditableFlag(PR_TRUE);
      }
    }
  }
  PRBool needHTMLController = PR_FALSE;

  const char *classString = "@mozilla.org/editor/htmleditor;1";
  if (mEditorType.EqualsLiteral("textmail"))
  {
    mEditorFlags = nsIPlaintextEditor::eEditorPlaintextMask | 
                   nsIPlaintextEditor::eEditorEnableWrapHackMask | 
                   nsIPlaintextEditor::eEditorMailMask;
  }
  else if (mEditorType.EqualsLiteral("text"))
  {
    mEditorFlags = nsIPlaintextEditor::eEditorPlaintextMask | 
                   nsIPlaintextEditor::eEditorEnableWrapHackMask;
  }
  else if (mEditorType.EqualsLiteral("htmlmail"))
  {
    if (mimeCType.EqualsLiteral("text/html"))
    {
      needHTMLController = PR_TRUE;
      mEditorFlags = nsIPlaintextEditor::eEditorMailMask;
    }
    else 
      mEditorFlags = nsIPlaintextEditor::eEditorPlaintextMask | 
                     nsIPlaintextEditor::eEditorEnableWrapHackMask;
  }
  else 
  {
    needHTMLController = PR_TRUE;
  }

  if (mInteractive) {
    mEditorFlags |= nsIPlaintextEditor::eEditorAllowInteraction;
  }

  
  nsComposerCommandsUpdater *stateMaintainer;
  NS_NEWXPCOM(stateMaintainer, nsComposerCommandsUpdater);
  mStateMaintainer = NS_STATIC_CAST(nsISelectionListener*, stateMaintainer);

  if (!mStateMaintainer) return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  rv = stateMaintainer->Init(aWindow);
  if (NS_FAILED(rv)) return rv;

  if (mEditorStatus != eEditorCreationInProgress)
  {
    
    nsCOMPtr<nsIDocumentStateListener> docListener =
                                          do_QueryInterface(mStateMaintainer);
    if (docListener)
      docListener->NotifyDocumentCreated();

    return NS_ERROR_FAILURE;
  }

  
  
  nsIDocShell *docShell = GetDocShellFromWindow(aWindow);
  if (!docShell) return NS_ERROR_FAILURE;  

  if (!mInteractive) {
    
    nsCOMPtr<nsIDOMWindowUtils> utils(do_GetInterface(aWindow));
    if (!utils) return NS_ERROR_FAILURE;

    rv = utils->GetImageAnimationMode(&mImageAnimationMode);
    if (NS_FAILED(rv)) return rv;
    utils->SetImageAnimationMode(imgIContainer::kDontAnimMode);
  }

  
  nsCOMPtr<nsIEditorDocShell> editorDocShell = do_QueryInterface(docShell, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIEditor> editor = do_CreateInstance(classString, &rv);
  if (NS_FAILED(rv)) return rv;
  
  rv = editorDocShell->SetEditor(editor);
  if (NS_FAILED(rv)) return rv;

  
  if (needHTMLController)
  {
    
    rv = SetupEditorCommandController("@mozilla.org/editor/htmleditorcontroller;1",
                                      aWindow, editor,
                                      &mHTMLCommandControllerId);
    if (NS_FAILED(rv)) return rv;
  }

  
  rv = editor->SetContentsMIMEType(mimeCType.get());
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIContentViewer> contentViewer;
  rv = docShell->GetContentViewer(getter_AddRefs(contentViewer));
  if (NS_FAILED(rv)) return rv;
  if (!contentViewer) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMDocument> domDoc;  
  rv = contentViewer->GetDOMDocument(getter_AddRefs(domDoc));
  if (NS_FAILED(rv)) return rv;
  if (!domDoc) return NS_ERROR_FAILURE;

  
  
  rv = editor->AddDocumentStateListener(
      NS_STATIC_CAST(nsIDocumentStateListener*, stateMaintainer));
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIPresShell> presShell;
  rv = docShell->GetPresShell(getter_AddRefs(presShell));
  if (NS_FAILED(rv)) return rv;
  if (!presShell) return NS_ERROR_FAILURE;

  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(presShell);
  rv = editor->Init(domDoc, presShell, nsnull ,
                    selCon, mEditorFlags);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISelection> selection;
  editor->GetSelection(getter_AddRefs(selection));
  nsCOMPtr<nsISelectionPrivate> selPriv = do_QueryInterface(selection);
  if (!selPriv) return NS_ERROR_FAILURE;

  rv = selPriv->AddSelectionListener(stateMaintainer);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsITransactionManager> txnMgr;
  editor->GetTransactionManager(getter_AddRefs(txnMgr));
  if (txnMgr)
    txnMgr->AddListener(NS_STATIC_CAST(nsITransactionListener*,
                        stateMaintainer));

  
  rv = SetEditorOnControllers(aWindow, editor);
  if (NS_FAILED(rv)) return rv;

  
  mEditorStatus = eEditorOK;


  
  return editor->PostCreate();
}








NS_IMETHODIMP
nsEditingSession::TearDownEditorOnWindow(nsIDOMWindow *aWindow,
                                         PRBool aStopEditing)
{
  if (!mDoneSetup)
    return NS_OK;

  nsresult rv;
  
  
  if (mLoadBlankDocTimer)
  {
    mLoadBlankDocTimer->Cancel();
    mLoadBlankDocTimer = nsnull;
  }

  nsIDocShell *docShell = GetDocShellFromWindow(aWindow);

  mDoneSetup = PR_FALSE;

  if (aStopEditing) {
    nsCOMPtr<nsIWebProgress> webProgress = do_GetInterface(docShell);
    if (webProgress) {
      webProgress->RemoveProgressListener(this);

      mProgressListenerRegistered = PR_FALSE;
    }
  }

  nsCOMPtr<nsIEditorDocShell> editorDocShell;
  rv = GetEditorDocShellFromWindow(aWindow, getter_AddRefs(editorDocShell));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIEditor> editor;
  rv = editorDocShell->GetEditor(getter_AddRefs(editor));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (mStateMaintainer && editor)
  {
    
    SetEditorOnControllers(aWindow, nsnull);
  }

  
  
  editorDocShell->SetEditor(nsnull);

  if (mStateMaintainer)
  {
    if (editor)
    {
      

      
      nsCOMPtr<nsISelection> selection;
      editor->GetSelection(getter_AddRefs(selection));
      nsCOMPtr<nsISelectionPrivate> selPriv = do_QueryInterface(selection);
      if (selPriv)
      {
        nsCOMPtr<nsISelectionListener> listener = 
          do_QueryInterface(mStateMaintainer);
        selPriv->RemoveSelectionListener(listener);
      }

      nsCOMPtr<nsIDocumentStateListener> docListener =
        do_QueryInterface(mStateMaintainer);
      editor->RemoveDocumentStateListener(docListener);

      nsCOMPtr<nsITransactionManager> txnMgr;
      editor->GetTransactionManager(getter_AddRefs(txnMgr));
      if (txnMgr)
      {
        nsCOMPtr<nsITransactionListener> transactionListener =
          do_QueryInterface(mStateMaintainer);
        txnMgr->RemoveListener(transactionListener);
      }
    }

    
    

    nsCOMPtr<nsIDOMWindowInternal> domWindowInt(do_QueryInterface(aWindow));
  
    nsCOMPtr<nsIControllers> controllers;      
    domWindowInt->GetControllers(getter_AddRefs(controllers));

    if (controllers) {
      nsCOMPtr<nsIController> controller;

      if (mBaseCommandControllerId) {
        controllers->GetControllerById(mBaseCommandControllerId,
                                       getter_AddRefs(controller));

        if (controller) {
          controllers->RemoveController(controller);
        }
      }

      if (mDocStateControllerId) {
        controllers->GetControllerById(mDocStateControllerId,
                                       getter_AddRefs(controller));

        if (controller) {
          controllers->RemoveController(controller);
        }
      }

      if (mHTMLCommandControllerId) {
        controllers->GetControllerById(mHTMLCommandControllerId,
                                       getter_AddRefs(controller));

        if (controller) {
          controllers->RemoveController(controller);
        }
      }
    }

    
    mBaseCommandControllerId = 0;
    mDocStateControllerId = 0;
    mHTMLCommandControllerId = 0;
  }

  if (aStopEditing) {
    if (!mInteractive) {
      
      if (mScriptsEnabled) {
        docShell->SetAllowJavascript(PR_TRUE);
      }

      if (mPluginsEnabled) {
        docShell->SetAllowPlugins(PR_TRUE);
      }

      nsCOMPtr<nsIDOMWindowUtils> utils(do_GetInterface(aWindow));
      if (utils)
        utils->SetImageAnimationMode(mImageAnimationMode);
    }

    if (mMakeWholeDocumentEditable) {
      nsCOMPtr<nsIDOMDocument> domDoc;
      rv = aWindow->GetDocument(getter_AddRefs(domDoc));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      doc->SetEditableFlag(PR_FALSE);
    }
  }

  return rv;
}







NS_IMETHODIMP 
nsEditingSession::GetEditorForWindow(nsIDOMWindow *aWindow,
                                     nsIEditor **outEditor)
{
  nsCOMPtr<nsIEditorDocShell> editorDocShell;
  nsresult rv = GetEditorDocShellFromWindow(aWindow,
                                            getter_AddRefs(editorDocShell));
  if (NS_FAILED(rv)) return rv;  
  
  return editorDocShell->GetEditor(outEditor);
}

#ifdef XP_MAC
#pragma mark -
#endif






NS_IMETHODIMP
nsEditingSession::OnStateChange(nsIWebProgress *aWebProgress,
                                nsIRequest *aRequest,
                                PRUint32 aStateFlags, nsresult aStatus)
{

#ifdef NOISY_DOC_LOADING
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (channel)
  {
    nsCAutoString contentType;
    channel->GetContentType(contentType);
    if (!contentType.IsEmpty())
      printf(" ++++++ MIMETYPE = %s\n", contentType.get());
  }
#endif

  
  
  
  if (aStateFlags & nsIWebProgressListener::STATE_START)
  {
#ifdef NOISY_DOC_LOADING
  {
    nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
    if (channel)
    {
      nsCOMPtr<nsIURI> uri;
      channel->GetURI(getter_AddRefs(uri));
      if (uri)
      {
        nsXPIDLCString spec;
        uri->GetSpec(spec);
        printf(" **** STATE_START: CHANNEL URI=%s, flags=%x\n",
               spec.get(), aStateFlags);
      }
    }
    else
      printf("    STATE_START: NO CHANNEL flags=%x\n", aStateFlags);
  }
#endif
    
    if (aStateFlags & nsIWebProgressListener::STATE_IS_NETWORK)
    {
      nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
      StartPageLoad(channel);
#ifdef NOISY_DOC_LOADING
      printf("STATE_START & STATE_IS_NETWORK flags=%x\n", aStateFlags);
#endif
    }

    
    if (aStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT) {
#ifdef NOISY_DOC_LOADING
      printf("STATE_START & STATE_IS_DOCUMENT flags=%x\n", aStateFlags);
#endif

      PRBool progressIsForTargetDocument =
        IsProgressForTargetDocument(aWebProgress);

      if (progressIsForTargetDocument)
      {
        nsCOMPtr<nsIDOMWindow> window;
        aWebProgress->GetDOMWindow(getter_AddRefs(window));

        nsCOMPtr<nsIDOMDocument> doc;
        window->GetDocument(getter_AddRefs(doc));

        nsCOMPtr<nsIHTMLDocument> htmlDoc(do_QueryInterface(doc));

        if (htmlDoc && htmlDoc->IsWriting())
        {
          nsCOMPtr<nsIDOMNSHTMLDocument> htmlDomDoc(do_QueryInterface(doc));
          nsAutoString designMode;

          htmlDomDoc->GetDesignMode(designMode);

          if (designMode.EqualsLiteral("on"))
          {
            
            

            return NS_OK;
          }
        }

        mCanCreateEditor = PR_TRUE;
        StartDocumentLoad(aWebProgress, progressIsForTargetDocument);
      }
    }
  }
  
  
  
  else if (aStateFlags & nsIWebProgressListener::STATE_TRANSFERRING)
  {
    if (aStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT)
    {
      
    }
  }
  
  
  
  else if (aStateFlags & nsIWebProgressListener::STATE_REDIRECTING)
  {
    if (aStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT)
    {
      
    }
  }
  
  
  
  else if (aStateFlags & nsIWebProgressListener::STATE_STOP)
  {

#ifdef NOISY_DOC_LOADING
  {
    nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
    if (channel)
    {
      nsCOMPtr<nsIURI> uri;
      channel->GetURI(getter_AddRefs(uri));
      if (uri)
      {
        nsXPIDLCString spec;
        uri->GetSpec(spec);
        printf(" **** STATE_STOP: CHANNEL URI=%s, flags=%x\n",
               spec.get(), aStateFlags);
      }
    }
    else
      printf("     STATE_STOP: NO CHANNEL  flags=%x\n", aStateFlags);
  }
#endif

    
    if (aStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT)
    {
      nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
      EndDocumentLoad(aWebProgress, channel, aStatus,
                      IsProgressForTargetDocument(aWebProgress));
#ifdef NOISY_DOC_LOADING
      printf("STATE_STOP & STATE_IS_DOCUMENT flags=%x\n", aStateFlags);
#endif
    }

    
    if (aStateFlags & nsIWebProgressListener::STATE_IS_NETWORK)
    {
      nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
      (void)EndPageLoad(aWebProgress, channel, aStatus);
#ifdef NOISY_DOC_LOADING
      printf("STATE_STOP & STATE_IS_NETWORK flags=%x\n", aStateFlags);
#endif
    }
  }

  return NS_OK;
}






NS_IMETHODIMP
nsEditingSession::OnProgressChange(nsIWebProgress *aWebProgress,
                                   nsIRequest *aRequest,
                                   PRInt32 aCurSelfProgress,
                                   PRInt32 aMaxSelfProgress,
                                   PRInt32 aCurTotalProgress,
                                   PRInt32 aMaxTotalProgress)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}






NS_IMETHODIMP
nsEditingSession::OnLocationChange(nsIWebProgress *aWebProgress, 
                                   nsIRequest *aRequest, nsIURI *aURI)
{
  nsCOMPtr<nsIDOMWindow> domWindow;
  nsresult rv = aWebProgress->GetDOMWindow(getter_AddRefs(domWindow));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDOMDocument> domDoc;
  rv = domWindow->GetDocument(getter_AddRefs(domDoc));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if (!doc) return NS_ERROR_FAILURE;

  doc->SetDocumentURI(aURI);

  
  
  nsIDocShell *docShell = GetDocShellFromWindow(domWindow);
  if (!docShell) return NS_ERROR_FAILURE;

  nsCOMPtr<nsICommandManager> commandManager = do_GetInterface(docShell);
  nsCOMPtr<nsPICommandUpdater> commandUpdater =
                                  do_QueryInterface(commandManager);
  if (!commandUpdater) return NS_ERROR_FAILURE;

  return commandUpdater->CommandStatusChanged("obs_documentLocationChanged");
}






NS_IMETHODIMP
nsEditingSession::OnStatusChange(nsIWebProgress *aWebProgress,
                                 nsIRequest *aRequest,
                                 nsresult aStatus,
                                 const PRUnichar *aMessage)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}






NS_IMETHODIMP
nsEditingSession::OnSecurityChange(nsIWebProgress *aWebProgress,
                                   nsIRequest *aRequest, PRUint32 state)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}


#ifdef XP_MAC
#pragma mark -
#endif









PRBool
nsEditingSession::IsProgressForTargetDocument(nsIWebProgress *aWebProgress)
{
  nsCOMPtr<nsIDOMWindow> domWindow;
  if (aWebProgress)
    aWebProgress->GetDOMWindow(getter_AddRefs(domWindow));
  nsCOMPtr<nsIDOMWindow> editedDOMWindow = do_QueryReferent(mWindowToBeEdited);

  return (domWindow && (domWindow == editedDOMWindow));
}










NS_IMETHODIMP
nsEditingSession::GetEditorStatus(PRUint32 *aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);
  *aStatus = mEditorStatus;
  return NS_OK;
}







nsresult
nsEditingSession::StartDocumentLoad(nsIWebProgress *aWebProgress, 
                                    PRBool aIsToBeMadeEditable)
{
#ifdef NOISY_DOC_LOADING
  printf("======= StartDocumentLoad ========\n");
#endif

  NS_ENSURE_ARG_POINTER(aWebProgress);
  
  
  
  nsCOMPtr<nsIDOMWindow> domWindow;
  aWebProgress->GetDOMWindow(getter_AddRefs(domWindow));
  if (domWindow)
  {
    TearDownEditorOnWindow(domWindow, PR_FALSE);
  }
    
  if (aIsToBeMadeEditable)
    mEditorStatus = eEditorCreationInProgress;

  return NS_OK;
}







nsresult
nsEditingSession::EndDocumentLoad(nsIWebProgress *aWebProgress,
                                  nsIChannel* aChannel, nsresult aStatus,
                                  PRBool aIsToBeMadeEditable)
{
  NS_ENSURE_ARG_POINTER(aWebProgress);
  
#ifdef NOISY_DOC_LOADING
  printf("======= EndDocumentLoad ========\n");
  printf("with status %d, ", aStatus);
  nsCOMPtr<nsIURI> uri;
  nsXPIDLCString spec;
  if (NS_SUCCEEDED(aChannel->GetURI(getter_AddRefs(uri)))) {
    uri->GetSpec(spec);
    printf(" uri %s\n", spec.get());
  }
#endif

  
  
  
  
  
  nsCOMPtr<nsIDOMWindow> domWindow;
  aWebProgress->GetDOMWindow(getter_AddRefs(domWindow));
  
  
  
  if (aIsToBeMadeEditable) {
    if (aStatus == NS_ERROR_FILE_NOT_FOUND)
      mEditorStatus = eEditorErrorFileNotFound;
  }

  nsIDocShell *docShell = GetDocShellFromWindow(domWindow);
  if (!docShell) return NS_ERROR_FAILURE;       

  
  
  
  nsCOMPtr<nsIRefreshURI> refreshURI = do_QueryInterface(docShell);
  if (refreshURI)
    refreshURI->CancelRefreshURITimers();

  nsCOMPtr<nsIEditorDocShell> editorDocShell = do_QueryInterface(docShell);

  nsresult rv = NS_OK;

  
  if (aIsToBeMadeEditable && mCanCreateEditor && editorDocShell)
  {
    PRBool  makeEditable;
    editorDocShell->GetEditable(&makeEditable);
  
    if (makeEditable)
    {
      
      nsCOMPtr<nsIEditor> editor;
      rv = editorDocShell->GetEditor(getter_AddRefs(editor));
      if (NS_FAILED(rv))
        return rv;
      if (!editor)
      {
        mCanCreateEditor = PR_FALSE;
        rv = SetupEditorOnWindow(domWindow);
        if (NS_FAILED(rv))
        {
          
          if (mLoadBlankDocTimer)
          {
            
            mLoadBlankDocTimer->Cancel();
            mLoadBlankDocTimer = NULL;
          }
  
          mLoadBlankDocTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
          if (NS_FAILED(rv)) return rv;

          mEditorStatus = eEditorCreationInProgress;
          mLoadBlankDocTimer->InitWithFuncCallback(
                                          nsEditingSession::TimerCallback,
                                          (void*)docShell,
                                          10, nsITimer::TYPE_ONE_SHOT);
        }
      }
    }
  }
  return rv;
}


void
nsEditingSession::TimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsCOMPtr<nsIDocShell> docShell = (nsIDocShell*)aClosure;
  if (docShell)
  {
    nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(docShell));
    if (webNav)
      webNav->LoadURI(NS_LITERAL_STRING("about:blank").get(),
                      0, nsnull, nsnull, nsnull);
  }
}







nsresult
nsEditingSession::StartPageLoad(nsIChannel *aChannel)
{
#ifdef NOISY_DOC_LOADING
  printf("======= StartPageLoad ========\n");
#endif
  return NS_OK;
}







nsresult
nsEditingSession::EndPageLoad(nsIWebProgress *aWebProgress,
                              nsIChannel* aChannel, nsresult aStatus)
{
#ifdef NOISY_DOC_LOADING
  printf("======= EndPageLoad ========\n");
  printf("  with status %d, ", aStatus);
  nsCOMPtr<nsIURI> uri;
  nsXPIDLCString spec;
  if (NS_SUCCEEDED(aChannel->GetURI(getter_AddRefs(uri)))) {
    uri->GetSpec(spec);
    printf("uri %s\n", spec.get());
  }
 
  nsCAutoString contentType;
  aChannel->GetContentType(contentType);
  if (!contentType.IsEmpty())
    printf("   flags = %d, status = %d, MIMETYPE = %s\n", 
               mEditorFlags, mEditorStatus, contentType.get());
#endif

  
  
  if (aStatus == NS_ERROR_FILE_NOT_FOUND)
    mEditorStatus = eEditorErrorFileNotFound;

  nsCOMPtr<nsIDOMWindow> domWindow;
  aWebProgress->GetDOMWindow(getter_AddRefs(domWindow));

  nsIDocShell *docShell = GetDocShellFromWindow(domWindow);
  if (!docShell) return NS_ERROR_FAILURE;

  
  
  
  nsCOMPtr<nsIRefreshURI> refreshURI = do_QueryInterface(docShell);
  if (refreshURI)
    refreshURI->CancelRefreshURITimers();

#if 0
  
  return MakeWindowEditable(domWindow, "html", PR_FALSE, mInteractive);
#else
  return NS_OK;
#endif
}


#ifdef XP_MAC
#pragma mark -
#endif







nsIDocShell *
nsEditingSession::GetDocShellFromWindow(nsIDOMWindow *aWindow)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  if (!window)
    return nsnull;

  return window->GetDocShell();
}








nsresult
nsEditingSession::GetEditorDocShellFromWindow(nsIDOMWindow *aWindow,
                                              nsIEditorDocShell** outDocShell)
{
  nsIDocShell *docShell = GetDocShellFromWindow(aWindow);
  if (!docShell) return NS_ERROR_FAILURE;
  
  return docShell->QueryInterface(NS_GET_IID(nsIEditorDocShell), 
                                  (void **)outDocShell);
}







nsresult
nsEditingSession::PrepareForEditing(nsIDOMWindow *aWindow)
{
  if (mProgressListenerRegistered)
    return NS_OK;
    
  nsIDocShell *docShell = GetDocShellFromWindow(aWindow);
  
  
  nsCOMPtr<nsIWebProgress> webProgress = do_GetInterface(docShell);
  if (!webProgress) return NS_ERROR_FAILURE;

  nsresult rv =
    webProgress->AddProgressListener(this,
                                     (nsIWebProgress::NOTIFY_STATE_NETWORK  | 
                                      nsIWebProgress::NOTIFY_STATE_DOCUMENT |
                                      nsIWebProgress::NOTIFY_LOCATION));

  mProgressListenerRegistered = NS_SUCCEEDED(rv);

  return rv;
}








nsresult
nsEditingSession::SetupEditorCommandController(
                                  const char *aControllerClassName,
                                  nsIDOMWindow *aWindow,
                                  nsISupports *aContext,
                                  PRUint32 *aControllerId)
{
  NS_ENSURE_ARG_POINTER(aControllerClassName);
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aContext);
  NS_ENSURE_ARG_POINTER(aControllerId);

  nsresult rv;
  nsCOMPtr<nsIDOMWindowInternal> domWindowInt =
                                    do_QueryInterface(aWindow, &rv);
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIControllers> controllers;      
  rv = domWindowInt->GetControllers(getter_AddRefs(controllers));
  if (NS_FAILED(rv)) return rv;

  
  
  if (!*aControllerId)
  {
    nsresult rv;
    nsCOMPtr<nsIController> controller;
    controller = do_CreateInstance(aControllerClassName, &rv);
    if (NS_FAILED(rv)) return rv;  

    
    
    
    rv = controllers->InsertControllerAt(0, controller);
    if (NS_FAILED(rv)) return rv;  

    
    rv = controllers->GetControllerId(controller, aControllerId);
    if (NS_FAILED(rv)) return rv;  
  }  

  
  return SetContextOnControllerById(controllers, aContext, *aControllerId);
}







NS_IMETHODIMP
nsEditingSession::SetEditorOnControllers(nsIDOMWindow *aWindow,
                                         nsIEditor* aEditor)
{
  nsresult rv;
  
  
  nsCOMPtr<nsIDOMWindowInternal> domWindowInt =
                                     do_QueryInterface(aWindow, &rv);
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIControllers> controllers;      
  rv = domWindowInt->GetControllers(getter_AddRefs(controllers));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISupports> editorAsISupports = do_QueryInterface(aEditor);
  if (mBaseCommandControllerId)
  {
    rv = SetContextOnControllerById(controllers, editorAsISupports,
                                    mBaseCommandControllerId);
    if (NS_FAILED(rv)) return rv;
  }

  if (mDocStateControllerId)
  {
    rv = SetContextOnControllerById(controllers, editorAsISupports,
                                    mDocStateControllerId);
    if (NS_FAILED(rv)) return rv;
  }

  if (mHTMLCommandControllerId)
    rv = SetContextOnControllerById(controllers, editorAsISupports,
                                    mHTMLCommandControllerId);

  return rv;
}

nsresult
nsEditingSession::SetContextOnControllerById(nsIControllers* aControllers,
                                             nsISupports* aContext,
                                             PRUint32 aID)
{
  NS_ENSURE_ARG_POINTER(aControllers);

  
  nsCOMPtr<nsIController> controller;    
  aControllers->GetControllerById(aID, getter_AddRefs(controller));
  
  
  nsCOMPtr<nsIControllerContext> editorController =
                                       do_QueryInterface(controller);
  if (!editorController) return NS_ERROR_FAILURE;

  return editorController->SetCommandContext(aContext);
}

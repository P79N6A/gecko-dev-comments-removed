









































#include "nsPIDOMWindow.h"
#include "nsIDOMWindowUtils.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMHTMLDocument.h"
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

#include "nsIScriptContext.h"
#include "imgIContainer.h"

#if DEBUG

#endif






nsEditingSession::nsEditingSession()
: mDoneSetup(PR_FALSE)
, mCanCreateEditor(PR_FALSE)
, mInteractive(PR_FALSE)
, mMakeWholeDocumentEditable(PR_TRUE)
, mDisabledJSAndPlugins(PR_FALSE)
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

  
  nsIDocShell *docShell = GetDocShellFromWindow(aWindow);
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  mDocShell = do_GetWeakReference(docShell);
  mInteractive = aInteractive;
  mMakeWholeDocumentEditable = aMakeWholeDocumentEditable;

  nsresult rv;
  if (!mInteractive) {
    rv = DisableJSAndPlugins(aWindow);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  TearDownEditorOnWindow(aWindow);
  
  
  mEditorStatus = eEditorCreationInProgress;

  
  if (!aEditorType)
    aEditorType = DEFAULT_EDITOR_TYPE;
  mEditorType = aEditorType;

  
  
  rv = PrepareForEditing(aWindow);
  NS_ENSURE_SUCCESS(rv, rv);  
  
  nsCOMPtr<nsIEditorDocShell> editorDocShell;
  rv = GetEditorDocShellFromWindow(aWindow, getter_AddRefs(editorDocShell));
  NS_ENSURE_SUCCESS(rv, rv);  
  
  
  rv = editorDocShell->MakeEditable(aDoAfterUriLoad);
  NS_ENSURE_SUCCESS(rv, rv);  

  
  
  
  rv = SetupEditorCommandController("@mozilla.org/editor/editorcontroller;1",
                                    aWindow,
                                    static_cast<nsIEditingSession*>(this),
                                    &mBaseCommandControllerId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = SetupEditorCommandController("@mozilla.org/editor/editordocstatecontroller;1",
                                    aWindow,
                                    static_cast<nsIEditingSession*>(this),
                                    &mDocStateControllerId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!aDoAfterUriLoad)
  {
    rv = SetupEditorOnWindow(aWindow);

    
    
    
    if (NS_FAILED(rv))
      TearDownEditorOnWindow(aWindow);
  }
  return rv;
}

NS_IMETHODIMP
nsEditingSession::DisableJSAndPlugins(nsIDOMWindow *aWindow)
{
  nsIDocShell *docShell = GetDocShellFromWindow(aWindow);
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  PRBool tmp;
  nsresult rv = docShell->GetAllowJavascript(&tmp);
  NS_ENSURE_SUCCESS(rv, rv);

  mScriptsEnabled = tmp;

  rv = docShell->SetAllowJavascript(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = docShell->GetAllowPlugins(&tmp);
  NS_ENSURE_SUCCESS(rv, rv);

  mPluginsEnabled = tmp;

  rv = docShell->SetAllowPlugins(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  mDisabledJSAndPlugins = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsEditingSession::RestoreJSAndPlugins(nsIDOMWindow *aWindow)
{
  NS_ENSURE_TRUE(mDisabledJSAndPlugins, NS_OK);

  mDisabledJSAndPlugins = PR_FALSE;

  nsIDocShell *docShell = GetDocShellFromWindow(aWindow);
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  nsresult rv = docShell->SetAllowJavascript(mScriptsEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  
  return docShell->SetAllowPlugins(mPluginsEnabled);
}

NS_IMETHODIMP
nsEditingSession::GetJsAndPluginsDisabled(PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = mDisabledJSAndPlugins;
  return NS_OK;
}







NS_IMETHODIMP
nsEditingSession::WindowIsEditable(nsIDOMWindow *aWindow, PRBool *outIsEditable)
{
  nsCOMPtr<nsIEditorDocShell> editorDocShell;
  nsresult rv = GetEditorDocShellFromWindow(aWindow,
                                            getter_AddRefs(editorDocShell));
  NS_ENSURE_SUCCESS(rv, rv);  

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
  NS_ENSURE_TRUE(aMIMEType, PR_FALSE);

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
    nsAutoString mimeType;
    if (NS_SUCCEEDED(doc->GetContentType(mimeType)))
      AppendUTF16toUTF8(mimeType, mimeCType);

    if (IsSupportedTextType(mimeCType.get()))
    {
      mEditorType.AssignLiteral("text");
      mimeCType = "text/plain";
    }
    else if (!mimeCType.EqualsLiteral("text/html") &&
             !mimeCType.EqualsLiteral("application/xhtml+xml"))
    {
      
      mEditorStatus = eEditorErrorCantEditMimeType;

      
      mEditorType.AssignLiteral("html");
      mimeCType.AssignLiteral("text/html");
    }

    
    
    nsCOMPtr<nsIDocument> document = do_QueryInterface(doc);
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

  
  mStateMaintainer = new nsComposerCommandsUpdater();

  
  
  
  rv = mStateMaintainer->Init(aWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mEditorStatus != eEditorCreationInProgress)
  {
    mStateMaintainer->NotifyDocumentCreated();
    return NS_ERROR_FAILURE;
  }

  
  
  nsIDocShell *docShell = GetDocShellFromWindow(aWindow);
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);  

  if (!mInteractive) {
    
    nsCOMPtr<nsIDOMWindowUtils> utils(do_GetInterface(aWindow));
    NS_ENSURE_TRUE(utils, NS_ERROR_FAILURE);

    rv = utils->GetImageAnimationMode(&mImageAnimationMode);
    NS_ENSURE_SUCCESS(rv, rv);
    utils->SetImageAnimationMode(imgIContainer::kDontAnimMode);
  }

  
  nsCOMPtr<nsIEditorDocShell> editorDocShell = do_QueryInterface(docShell, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIEditor> editor = do_QueryReferent(mExistingEditor);
  if (editor) {
    editor->PreDestroy(PR_FALSE);
  } else {
    editor = do_CreateInstance(classString, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    mExistingEditor = do_GetWeakReference(editor);
  }
  
  rv = editorDocShell->SetEditor(editor);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (needHTMLController)
  {
    
    rv = SetupEditorCommandController("@mozilla.org/editor/htmleditorcontroller;1",
                                      aWindow, editor,
                                      &mHTMLCommandControllerId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = editor->SetContentsMIMEType(mimeCType.get());
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContentViewer> contentViewer;
  rv = docShell->GetContentViewer(getter_AddRefs(contentViewer));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(contentViewer, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMDocument> domDoc;  
  rv = contentViewer->GetDOMDocument(getter_AddRefs(domDoc));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(domDoc, NS_ERROR_FAILURE);

  
  
  rv = editor->AddDocumentStateListener(mStateMaintainer);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = editor->Init(domDoc, nsnull ,
                    nsnull, mEditorFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISelection> selection;
  editor->GetSelection(getter_AddRefs(selection));
  nsCOMPtr<nsISelectionPrivate> selPriv = do_QueryInterface(selection);
  NS_ENSURE_TRUE(selPriv, NS_ERROR_FAILURE);

  rv = selPriv->AddSelectionListener(mStateMaintainer);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsITransactionManager> txnMgr;
  editor->GetTransactionManager(getter_AddRefs(txnMgr));
  if (txnMgr)
    txnMgr->AddListener(mStateMaintainer);

  
  rv = SetEditorOnControllers(aWindow, editor);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mEditorStatus = eEditorOK;

  
  return editor->PostCreate();
}


void
nsEditingSession::RemoveListenersAndControllers(nsIDOMWindow *aWindow,
                                                nsIEditor *aEditor)
{
  if (!mStateMaintainer || !aEditor)
    return;

  
  nsCOMPtr<nsISelection> selection;
  aEditor->GetSelection(getter_AddRefs(selection));
  nsCOMPtr<nsISelectionPrivate> selPriv = do_QueryInterface(selection);
  if (selPriv)
    selPriv->RemoveSelectionListener(mStateMaintainer);

  aEditor->RemoveDocumentStateListener(mStateMaintainer);

  nsCOMPtr<nsITransactionManager> txnMgr;
  aEditor->GetTransactionManager(getter_AddRefs(txnMgr));
  if (txnMgr)
    txnMgr->RemoveListener(mStateMaintainer);

  
  
  RemoveEditorControllers(aWindow);
}







NS_IMETHODIMP
nsEditingSession::TearDownEditorOnWindow(nsIDOMWindow *aWindow)
{
  NS_ENSURE_TRUE(mDoneSetup, NS_OK);

  NS_ENSURE_TRUE(aWindow, NS_ERROR_NULL_POINTER);

  nsresult rv;
  
  
  if (mLoadBlankDocTimer)
  {
    mLoadBlankDocTimer->Cancel();
    mLoadBlankDocTimer = nsnull;
  }

  mDoneSetup = PR_FALSE;

  
  nsCOMPtr<nsIDOMDocument> domDoc;
  aWindow->GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(domDoc);
  PRBool stopEditing = htmlDoc && htmlDoc->IsEditingOn();
  if (stopEditing)
    RemoveWebProgressListener(aWindow);

  nsCOMPtr<nsIEditorDocShell> editorDocShell;
  rv = GetEditorDocShellFromWindow(aWindow, getter_AddRefs(editorDocShell));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIEditor> editor;
  rv = editorDocShell->GetEditor(getter_AddRefs(editor));
  NS_ENSURE_SUCCESS(rv, rv);

  if (stopEditing)
    htmlDoc->TearingDownEditor(editor);

  if (mStateMaintainer && editor)
  {
    
    
    SetEditorOnControllers(aWindow, nsnull);
  }

  
  
  editorDocShell->SetEditor(nsnull);

  RemoveListenersAndControllers(aWindow, editor);

  if (stopEditing)
  {
    
    RestoreJSAndPlugins(aWindow);
    RestoreAnimationMode(aWindow);

    if (mMakeWholeDocumentEditable)
    {
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
  NS_ENSURE_SUCCESS(rv, rv);  
  
  return editorDocShell->GetEditor(outEditor);
}






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

    
    if (aStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT &&
        !(aStateFlags & nsIWebProgressListener::STATE_RESTORING)) {
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
          nsCOMPtr<nsIDOMHTMLDocument> htmlDomDoc = do_QueryInterface(doc);
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
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocument> domDoc;
  rv = domWindow->GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  doc->SetDocumentURI(aURI);

  
  
  nsIDocShell *docShell = GetDocShellFromWindow(domWindow);
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsICommandManager> commandManager = do_GetInterface(docShell);
  nsCOMPtr<nsPICommandUpdater> commandUpdater =
                                  do_QueryInterface(commandManager);
  NS_ENSURE_TRUE(commandUpdater, NS_ERROR_FAILURE);

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









PRBool
nsEditingSession::IsProgressForTargetDocument(nsIWebProgress *aWebProgress)
{
  nsCOMPtr<nsIWebProgress> editedWebProgress = do_QueryReferent(mDocShell);
  return editedWebProgress == aWebProgress;
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
    nsIDocShell *docShell = GetDocShellFromWindow(domWindow);
    NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);
    docShell->DetachEditorFromWindow();
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
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);       

  
  
  
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
      
      
      bool needsSetup = false;
      if (mMakeWholeDocumentEditable) {
        needsSetup = true;
      } else {
        
        nsCOMPtr<nsIEditor> editor;
        rv = editorDocShell->GetEditor(getter_AddRefs(editor));
        NS_ENSURE_SUCCESS(rv, rv);

        needsSetup = !editor;
      }

      if (needsSetup)
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
          NS_ENSURE_SUCCESS(rv, rv);

          mEditorStatus = eEditorCreationInProgress;
          mLoadBlankDocTimer->InitWithFuncCallback(
                                          nsEditingSession::TimerCallback,
                                          static_cast<void*> (mDocShell.get()),
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
  nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(static_cast<nsIWeakReference*> (aClosure));
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
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  
  
  
  nsCOMPtr<nsIRefreshURI> refreshURI = do_QueryInterface(docShell);
  if (refreshURI)
    refreshURI->CancelRefreshURITimers();

#if 0
  
  return MakeWindowEditable(domWindow, "html", PR_FALSE, mInteractive);
#else
  return NS_OK;
#endif
}







nsIDocShell *
nsEditingSession::GetDocShellFromWindow(nsIDOMWindow *aWindow)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(window, nsnull);

  return window->GetDocShell();
}








nsresult
nsEditingSession::GetEditorDocShellFromWindow(nsIDOMWindow *aWindow,
                                              nsIEditorDocShell** outDocShell)
{
  nsIDocShell *docShell = GetDocShellFromWindow(aWindow);
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);
  
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
  NS_ENSURE_TRUE(webProgress, NS_ERROR_FAILURE);

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
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIControllers> controllers;      
  rv = domWindowInt->GetControllers(getter_AddRefs(controllers));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (!*aControllerId)
  {
    nsresult rv;
    nsCOMPtr<nsIController> controller;
    controller = do_CreateInstance(aControllerClassName, &rv);
    NS_ENSURE_SUCCESS(rv, rv);  

    
    
    
    rv = controllers->InsertControllerAt(0, controller);
    NS_ENSURE_SUCCESS(rv, rv);  

    
    rv = controllers->GetControllerId(controller, aControllerId);
    NS_ENSURE_SUCCESS(rv, rv);  
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
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIControllers> controllers;      
  rv = domWindowInt->GetControllers(getter_AddRefs(controllers));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> editorAsISupports = do_QueryInterface(aEditor);
  if (mBaseCommandControllerId)
  {
    rv = SetContextOnControllerById(controllers, editorAsISupports,
                                    mBaseCommandControllerId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDocStateControllerId)
  {
    rv = SetContextOnControllerById(controllers, editorAsISupports,
                                    mDocStateControllerId);
    NS_ENSURE_SUCCESS(rv, rv);
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
  NS_ENSURE_TRUE(editorController, NS_ERROR_FAILURE);

  return editorController->SetCommandContext(aContext);
}

void
nsEditingSession::RemoveEditorControllers(nsIDOMWindow *aWindow)
{
  
  
  nsCOMPtr<nsIDOMWindowInternal> domWindowInt(do_QueryInterface(aWindow));

  nsCOMPtr<nsIControllers> controllers;
  if (domWindowInt)
    domWindowInt->GetControllers(getter_AddRefs(controllers));

  if (controllers)
  {
    nsCOMPtr<nsIController> controller;
    if (mBaseCommandControllerId)
    {
      controllers->GetControllerById(mBaseCommandControllerId,
                                     getter_AddRefs(controller));
      if (controller)
        controllers->RemoveController(controller);
    }

    if (mDocStateControllerId)
    {
      controllers->GetControllerById(mDocStateControllerId,
                                     getter_AddRefs(controller));
      if (controller)
        controllers->RemoveController(controller);
    }

    if (mHTMLCommandControllerId)
    {
      controllers->GetControllerById(mHTMLCommandControllerId,
                                     getter_AddRefs(controller));
      if (controller)
        controllers->RemoveController(controller);
    }
  }

  
  mBaseCommandControllerId = 0;
  mDocStateControllerId = 0;
  mHTMLCommandControllerId = 0;
}

void
nsEditingSession::RemoveWebProgressListener(nsIDOMWindow *aWindow)
{
  nsIDocShell *docShell = GetDocShellFromWindow(aWindow);
  nsCOMPtr<nsIWebProgress> webProgress = do_GetInterface(docShell);
  if (webProgress)
  {
    webProgress->RemoveProgressListener(this);
    mProgressListenerRegistered = PR_FALSE;
  }
}

void
nsEditingSession::RestoreAnimationMode(nsIDOMWindow *aWindow)
{
  if (!mInteractive)
  {
    nsCOMPtr<nsIDOMWindowUtils> utils(do_GetInterface(aWindow));
    if (utils)
      utils->SetImageAnimationMode(mImageAnimationMode);
  }
}

nsresult
nsEditingSession::DetachFromWindow(nsIDOMWindow* aWindow)
{
  NS_ENSURE_TRUE(mDoneSetup, NS_OK);

  NS_ASSERTION(mStateMaintainer, "mStateMaintainer should exist.");

  
  if (mLoadBlankDocTimer)
  {
    mLoadBlankDocTimer->Cancel();
    mLoadBlankDocTimer = nsnull;
  }

  
  
  RemoveEditorControllers(aWindow);
  RemoveWebProgressListener(aWindow);
  RestoreJSAndPlugins(aWindow);
  RestoreAnimationMode(aWindow);

  
  
  mDocShell = nsnull;

  return NS_OK;
}

nsresult
nsEditingSession::ReattachToWindow(nsIDOMWindow* aWindow)
{
  NS_ENSURE_TRUE(mDoneSetup, NS_OK);

  NS_ASSERTION(mStateMaintainer, "mStateMaintainer should exist.");

  
  
  nsresult rv;

  nsIDocShell *docShell = GetDocShellFromWindow(aWindow);
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);
  mDocShell = do_GetWeakReference(docShell);

  
  if (!mInteractive)
  {
    rv = DisableJSAndPlugins(aWindow);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  mEditorStatus = eEditorCreationInProgress;

  
  rv = PrepareForEditing(aWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = SetupEditorCommandController("@mozilla.org/editor/editorcontroller;1",
                                    aWindow,
                                    static_cast<nsIEditingSession*>(this),
                                    &mBaseCommandControllerId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetupEditorCommandController("@mozilla.org/editor/editordocstatecontroller;1",
                                    aWindow,
                                    static_cast<nsIEditingSession*>(this),
                                    &mDocStateControllerId);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mStateMaintainer)
    mStateMaintainer->Init(aWindow);

  
  nsCOMPtr<nsIEditor> editor;
  rv = GetEditorForWindow(aWindow, getter_AddRefs(editor));
  NS_ENSURE_TRUE(editor, NS_ERROR_FAILURE);

  if (!mInteractive)
  {
    
    nsCOMPtr<nsIDOMWindowUtils> utils(do_GetInterface(aWindow));
    NS_ENSURE_TRUE(utils, NS_ERROR_FAILURE);

    rv = utils->GetImageAnimationMode(&mImageAnimationMode);
    NS_ENSURE_SUCCESS(rv, rv);
    utils->SetImageAnimationMode(imgIContainer::kDontAnimMode);
  }

  
  rv = SetupEditorCommandController("@mozilla.org/editor/htmleditorcontroller;1",
                                    aWindow, editor,
                                    &mHTMLCommandControllerId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = SetEditorOnControllers(aWindow, editor);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
  {
    PRBool isEditable;
    rv = WindowIsEditable(aWindow, &isEditable);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ASSERTION(isEditable, "Window is not editable after reattaching editor.");
  }
#endif 

  return NS_OK;
}












































#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsISupports.h"
#include "nsIContent.h"
#include "nsIContentViewerContainer.h"
#include "nsIDocumentViewer.h"
#include "nsIDocumentViewerPrint.h"

#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIEventStateManager.h"
#include "nsStyleSet.h"
#include "nsIStyleSheet.h"
#include "nsICSSStyleSheet.h"
#include "nsIFrame.h"

#include "nsILinkHandler.h"
#include "nsIDOMDocument.h"
#include "nsISelectionListener.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMNSHTMLDocument.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMRange.h"
#include "nsContentCID.h"
#include "nsLayoutCID.h"
#include "nsContentUtils.h"
#include "nsLayoutStylesheetCache.h"

#include "nsViewsCID.h"
#include "nsWidgetsCID.h"
#include "nsIDeviceContext.h"
#include "nsIDeviceContextSpec.h"
#include "nsIViewManager.h"
#include "nsIView.h"

#include "nsIPageSequenceFrame.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIContentViewerEdit.h"
#include "nsIContentViewerFile.h"
#include "nsICSSLoader.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShell.h"
#include "nsIBaseWindow.h"
#include "nsIFrameDebug.h"
#include "nsILayoutHistoryState.h"
#include "nsIParser.h"
#include "nsGUIEvent.h"
#include "nsHTMLReflowState.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIImageLoadingContent.h"
#include "nsCopySupport.h"
#include "nsIDOMHTMLFrameSetElement.h"
#ifdef MOZ_XUL
#include "nsIXULDocument.h"
#include "nsXULPopupManager.h"
#endif
#include "nsPrintfCString.h"

#include "nsIClipboardHelper.h"

#include "nsPIDOMWindow.h"
#include "nsJSEnvironment.h"
#include "nsIFocusController.h"
#include "nsIMenuParent.h"

#include "nsIScrollableView.h"
#include "nsIHTMLDocument.h"
#include "nsITimelineService.h"
#include "nsGfxCIID.h"
#include "nsStyleSheetService.h"

#include "nsIPrompt.h"
#include "imgIContainer.h" 




#ifdef NS_PRINTING

#include "nsIWebBrowserPrint.h"

#include "nsPrintEngine.h"


#include "nsIPrintSettings.h"
#include "nsIPrintSettingsService.h"
#include "nsIPrintOptions.h"
#include "nsIServiceManager.h"
#include "nsISimpleEnumerator.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"


static const char sPrintOptionsContractID[]         = "@mozilla.org/gfx/printsettings-service;1";


#include "nsPrintPreviewListener.h"

#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIPluginDocument.h"


#include "nsIPrintProgress.h"
#include "nsIPrintProgressParams.h"


#include "nsIWindowWatcher.h"


#include "nsPrintEngine.h"
#include "nsPagePrintTimer.h"

#endif 


#include "nsIDocument.h"


#include "nsIDOMEventTarget.h"
#include "nsIDOMFocusListener.h"
#include "nsISelectionController.h"

#include "nsBidiUtils.h"
#include "nsISHEntry.h"
#include "nsISHistory.h"
#include "nsISHistoryInternal.h"
#include "nsIWebNavigation.h"
#include "nsWeakPtr.h"
#include "nsEventDispatcher.h"


#include "prenv.h"
#include <stdio.h>


#include "nsGfxCIID.h"

#ifdef NS_DEBUG

#undef NOISY_VIEWER
#else
#undef NOISY_VIEWER
#endif



#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "prlog.h"

#ifdef PR_LOGGING

static PRLogModuleInfo * kPrintingLogMod = PR_NewLogModule("printing");
#define PR_PL(_p1)  PR_LOG(kPrintingLogMod, PR_LOG_DEBUG, _p1);

#define PRT_YESNO(_p) ((_p)?"YES":"NO")
#else
#define PRT_YESNO(_p)
#define PR_PL(_p1)
#endif


class DocumentViewerImpl;



#ifdef XP_MAC
#pragma mark ** nsDocViewerSelectionListener **
#endif

class nsDocViewerSelectionListener : public nsISelectionListener
{
public:

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSISELECTIONLISTENER

                       nsDocViewerSelectionListener()
                       : mDocViewer(NULL)
                       , mGotSelectionState(PR_FALSE)
                       , mSelectionWasCollapsed(PR_FALSE)
                       {
                       }

  virtual              ~nsDocViewerSelectionListener() {}

  nsresult             Init(DocumentViewerImpl *aDocViewer);

protected:

  DocumentViewerImpl*  mDocViewer;
  PRPackedBool         mGotSelectionState;
  PRPackedBool         mSelectionWasCollapsed;

};




class nsDocViewerFocusListener : public nsIDOMFocusListener
{
public:
  

  nsDocViewerFocusListener();
  

  virtual ~nsDocViewerFocusListener();



  NS_DECL_ISUPPORTS


  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);

  nsresult             Init(DocumentViewerImpl *aDocViewer);

private:
    DocumentViewerImpl*  mDocViewer;
};



#ifdef XP_MAC
#pragma mark ** DocumentViewerImpl **
#endif


class DocumentViewerImpl : public nsIDocumentViewer,
                           public nsIContentViewerEdit,
                           public nsIContentViewerFile,
                           public nsIMarkupDocumentViewer,
                           public nsIDocumentViewerPrint

#ifdef NS_PRINTING
                           , public nsIWebBrowserPrint
#endif

{
  friend class nsDocViewerSelectionListener;
  friend class nsPagePrintTimer;
  friend class nsPrintEngine;

public:
  DocumentViewerImpl();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICONTENTVIEWER

  
  NS_IMETHOD SetUAStyleSheet(nsIStyleSheet* aUAStyleSheet);
  NS_IMETHOD GetDocument(nsIDocument** aResult);
  NS_IMETHOD GetPresShell(nsIPresShell** aResult);
  NS_IMETHOD GetPresContext(nsPresContext** aResult);

  
  NS_DECL_NSICONTENTVIEWEREDIT

  
  NS_DECL_NSICONTENTVIEWERFILE

  
  NS_DECL_NSIMARKUPDOCUMENTVIEWER

#ifdef NS_PRINTING
  
  NS_DECL_NSIWEBBROWSERPRINT
#endif

  typedef void (*CallChildFunc)(nsIMarkupDocumentViewer* aViewer,
                                void* aClosure);
  void CallChildren(CallChildFunc aFunc, void* aClosure);

  
  NS_DECL_NSIDOCUMENTVIEWERPRINT

protected:
  virtual ~DocumentViewerImpl();

private:
  nsresult MakeWindow(nsIWidget* aParentWidget,
                      const nsRect& aBounds);
  nsresult InitInternal(nsIWidget* aParentWidget,
                        nsISupports *aState,
                        nsIDeviceContext* aDeviceContext,
                        const nsRect& aBounds,
                        PRBool aDoCreation,
                        PRBool aInPrintPreview,
                        PRBool aNeedMakeCX = PR_TRUE);
  nsresult InitPresentationStuff(PRBool aDoInitialReflow);

  nsresult GetPopupNode(nsIDOMNode** aNode);
  nsresult GetPopupLinkNode(nsIDOMNode** aNode);
  nsresult GetPopupImageNode(nsIImageLoadingContent** aNode);

  void DumpContentToPPM(const char* aFileName);

  void PrepareToStartLoad(void);

  nsresult SyncParentSubDocMap();

  nsresult GetDocumentSelection(nsISelection **aSelection);

  nsresult GetClipboardEventTarget(nsIDOMNode **aEventTarget);

#ifdef NS_PRINTING
  
  
  void SetIsPrintingInDocShellTree(nsIDocShellTreeNode* aParentNode, 
                                   PRBool               aIsPrintingOrPP, 
                                   PRBool               aStartAtTop);
#endif 

protected:
  
  
  
  
  

  nsWeakPtr mContainer; 
  nsCOMPtr<nsIDeviceContext> mDeviceContext;   

  
  
  nsCOMPtr<nsIDocument>    mDocument;
  nsCOMPtr<nsIWidget>      mWindow;      
  nsCOMPtr<nsIViewManager> mViewManager;
  nsCOMPtr<nsPresContext> mPresContext;
  nsCOMPtr<nsIPresShell>   mPresShell;

  nsCOMPtr<nsIStyleSheet>  mUAStyleSheet;

  nsCOMPtr<nsISelectionListener> mSelectionListener;
  nsCOMPtr<nsIDOMFocusListener> mFocusListener;

  nsCOMPtr<nsIContentViewer> mPreviousViewer;
  nsCOMPtr<nsISHEntry> mSHEntry;

  nsIWidget* mParentWidget;          

  float mTextZoom;      

  PRInt16 mNumURLStarts;
  PRInt16 mDestroyRefCount;    

  unsigned      mEnableRendering : 1;
  unsigned      mStopped : 1;
  unsigned      mLoaded : 1;
  unsigned      mDeferredWindowClose : 1;
  
  
  
  unsigned      mIsSticky : 1;
  unsigned      mInPermitUnload : 1;

#ifdef NS_PRINTING
  unsigned      mClosingWhilePrinting : 1;

#if NS_PRINT_PREVIEW
  
  unsigned                         mPrintIsPending : 1;
  unsigned                         mPrintDocIsFullyLoaded : 1;
  nsCOMPtr<nsIPrintSettings>       mCachedPrintSettings;
  nsCOMPtr<nsIWebProgressListener> mCachedPrintWebProgressListner;

  nsCOMPtr<nsPrintEngine>          mPrintEngine;
#endif 

#ifdef NS_DEBUG
  FILE* mDebugFile;
#endif 
#endif 

  
  PRInt32 mHintCharsetSource;
  nsCString mHintCharset;
  nsCString mDefaultCharacterSet;
  nsCString mForceCharacterSet;
  nsCString mPrevDocCharacterSet;
  
  PRPackedBool mIsPageMode;

};





static NS_DEFINE_CID(kViewManagerCID,       NS_VIEW_MANAGER_CID);
static NS_DEFINE_CID(kWidgetCID,            NS_CHILD_CID);


nsresult
NS_NewDocumentViewer(nsIDocumentViewer** aResult)
{
  *aResult = new DocumentViewerImpl();
  if (!*aResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aResult);

  return NS_OK;
}

void DocumentViewerImpl::PrepareToStartLoad()
{
  mEnableRendering  = PR_TRUE;
  mStopped          = PR_FALSE;
  mLoaded           = PR_FALSE;
  mDeferredWindowClose = PR_FALSE;

#ifdef NS_PRINTING
  mPrintIsPending        = PR_FALSE;
  mPrintDocIsFullyLoaded = PR_FALSE;
  mClosingWhilePrinting  = PR_FALSE;

  
  if (mPrintEngine) {
    mPrintEngine->Destroy();
    mPrintEngine = nsnull;
  }

#ifdef NS_PRINT_PREVIEW
  SetIsPrintPreview(PR_FALSE);
#endif

#ifdef NS_DEBUG
  mDebugFile = nsnull;
#endif

#endif 
}


DocumentViewerImpl::DocumentViewerImpl()
  : mTextZoom(1.0),
    mIsSticky(PR_TRUE),
    mHintCharsetSource(kCharsetUninitialized)
{
  PrepareToStartLoad();
}

NS_IMPL_ADDREF(DocumentViewerImpl)
NS_IMPL_RELEASE(DocumentViewerImpl)

NS_INTERFACE_MAP_BEGIN(DocumentViewerImpl)
    NS_INTERFACE_MAP_ENTRY(nsIContentViewer)
    NS_INTERFACE_MAP_ENTRY(nsIDocumentViewer)
    NS_INTERFACE_MAP_ENTRY(nsIMarkupDocumentViewer)
    NS_INTERFACE_MAP_ENTRY(nsIContentViewerFile)
    NS_INTERFACE_MAP_ENTRY(nsIContentViewerEdit)
    NS_INTERFACE_MAP_ENTRY(nsIDocumentViewerPrint)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContentViewer)
#ifdef NS_PRINTING
    NS_INTERFACE_MAP_ENTRY(nsIWebBrowserPrint)
#endif
NS_INTERFACE_MAP_END

DocumentViewerImpl::~DocumentViewerImpl()
{
  if (mDocument) {
    Close(nsnull);
    mDocument->Destroy();
  }

  NS_ASSERTION(!mPresShell && !mPresContext,
               "User did not call nsIContentViewer::Destroy");
  if (mPresShell || mPresContext) {
    
    
    mSHEntry = nsnull;

    Destroy();
  }

  
}









NS_IMETHODIMP
DocumentViewerImpl::LoadStart(nsISupports *aDoc)
{
#ifdef NOISY_VIEWER
  printf("DocumentViewerImpl::LoadStart\n");
#endif

  nsresult rv = NS_OK;
  if (!mDocument) {
    mDocument = do_QueryInterface(aDoc, &rv);
  }
  else if (mDocument == aDoc) {
    
    
    PrepareToStartLoad();
  }

  return rv;
}

nsresult
DocumentViewerImpl::SyncParentSubDocMap()
{
  nsCOMPtr<nsIDocShellTreeItem> item(do_QueryReferent(mContainer));
  nsCOMPtr<nsPIDOMWindow> pwin(do_GetInterface(item));
  nsCOMPtr<nsIContent> content;

  if (mDocument && pwin) {
    content = do_QueryInterface(pwin->GetFrameElementInternal());
  }

  if (content) {
    nsCOMPtr<nsIDocShellTreeItem> parent;
    item->GetParent(getter_AddRefs(parent));

    nsCOMPtr<nsIDOMWindow> parent_win(do_GetInterface(parent));

    if (parent_win) {
      nsCOMPtr<nsIDOMDocument> dom_doc;
      parent_win->GetDocument(getter_AddRefs(dom_doc));

      nsCOMPtr<nsIDocument> parent_doc(do_QueryInterface(dom_doc));

      if (parent_doc) {
        return parent_doc->SetSubDocumentFor(content, mDocument);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::SetContainer(nsISupports* aContainer)
{
  mContainer = do_GetWeakReference(aContainer);
  if (mPresContext) {
    mPresContext->SetContainer(aContainer);
  }

  
  
  

  return SyncParentSubDocMap();
}

NS_IMETHODIMP
DocumentViewerImpl::GetContainer(nsISupports** aResult)
{
   NS_ENSURE_ARG_POINTER(aResult);

   *aResult = nsnull;
   nsCOMPtr<nsISupports> container = do_QueryReferent(mContainer);
   container.swap(*aResult);
   return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Init(nsIWidget* aParentWidget,
                         nsIDeviceContext* aDeviceContext,
                         const nsRect& aBounds)
{
  return InitInternal(aParentWidget, nsnull, aDeviceContext, aBounds, PR_TRUE, PR_FALSE);
}

nsresult
DocumentViewerImpl::InitPresentationStuff(PRBool aDoInitialReflow)
{
  
  nsStyleSet *styleSet;
  nsresult rv = CreateStyleSet(mDocument, &styleSet);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDocument->CreateShell(mPresContext, mViewManager, styleSet,
                              getter_AddRefs(mPresShell));
  if (NS_FAILED(rv)) {
    delete styleSet;
    return rv;
  }

  
  styleSet->EndUpdate();

  if (aDoInitialReflow) {
    
    
    
    
    
    
    
    

    mDocument->FlushPendingNotifications(Flush_ContentAndNotify);
  }

  mPresShell->BeginObservingDocument();

  
  nsRect bounds;
  mWindow->GetBounds(bounds);

  nscoord width = mPresContext->DevPixelsToAppUnits(bounds.width);
  nscoord height = mPresContext->DevPixelsToAppUnits(bounds.height);

  mViewManager->DisableRefresh();
  mViewManager->SetWindowDimensions(width, height);
  mPresContext->SetTextZoom(mTextZoom);

  

  
  
  mViewManager->SetDefaultBackgroundColor(mPresContext->DefaultBackgroundColor());

  if (aDoInitialReflow) {
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(mDocument);
    if (htmlDoc) {
      nsCOMPtr<nsIDOMHTMLFrameSetElement> frameset =
        do_QueryInterface(mDocument->GetRootContent());
      htmlDoc->SetIsFrameset(frameset != nsnull);
    }

    nsCOMPtr<nsIPresShell> shellGrip = mPresShell;
    
    mPresShell->InitialReflow(width, height);

    
    if (mEnableRendering && mViewManager) {
      mViewManager->EnableRefresh(NS_VMREFRESH_IMMEDIATE);
    }
  } else {
    
    
    mPresContext->SetVisibleArea(nsRect(0, 0, width, height));
  }

  
  
  nsDocViewerSelectionListener *selectionListener =
    new nsDocViewerSelectionListener();
  NS_ENSURE_TRUE(selectionListener, NS_ERROR_OUT_OF_MEMORY);

  selectionListener->Init(this);

  
  mSelectionListener = selectionListener;

  nsCOMPtr<nsISelection> selection;
  rv = GetDocumentSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(selection));
  rv = selPrivate->AddSelectionListener(mSelectionListener);
  if (NS_FAILED(rv))
    return rv;

  
  nsCOMPtr<nsIDOMFocusListener> mOldFocusListener = mFocusListener;

  
  
  
  
  nsDocViewerFocusListener *focusListener;
  NS_NEWXPCOM(focusListener, nsDocViewerFocusListener);
  NS_ENSURE_TRUE(focusListener, NS_ERROR_OUT_OF_MEMORY);

  focusListener->Init(this);

  
  mFocusListener = focusListener;

  if (mDocument) {
    rv = mDocument->AddEventListenerByIID(mFocusListener,
                                          NS_GET_IID(nsIDOMFocusListener));
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to register focus listener");
    if (mOldFocusListener) {
      rv = mDocument->RemoveEventListenerByIID(mOldFocusListener,
                                               NS_GET_IID(nsIDOMFocusListener));
      NS_ASSERTION(NS_SUCCEEDED(rv), "failed to remove focus listener");
    }
  }

  return NS_OK;
}





nsresult
DocumentViewerImpl::InitInternal(nsIWidget* aParentWidget,
                                 nsISupports *aState,
                                 nsIDeviceContext* aDeviceContext,
                                 const nsRect& aBounds,
                                 PRBool aDoCreation,
                                 PRBool aInPrintPreview,
                                 PRBool aNeedMakeCX )
{
  mParentWidget = aParentWidget; 

  nsresult rv = NS_OK;
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NULL_POINTER);

  mDeviceContext = aDeviceContext;

  PRBool makeCX = PR_FALSE;
  if (aDoCreation) {
    if (aParentWidget && !mPresContext) {
      
      if (mIsPageMode) {
        
      }
      else
        mPresContext =
            new nsPresContext(mDocument, nsPresContext::eContext_Galley);
      NS_ENSURE_TRUE(mPresContext, NS_ERROR_OUT_OF_MEMORY);

      nsresult rv = mPresContext->Init(aDeviceContext); 
      if (NS_FAILED(rv)) {
        mPresContext = nsnull;
        return rv;
      }

#if defined(NS_PRINTING) && defined(NS_PRINT_PREVIEW)
      makeCX = !GetIsPrintPreview() && aNeedMakeCX; 
#else
      makeCX = PR_TRUE;
#endif
    }

    if (mPresContext) {
      

      
      
      
      

      rv = MakeWindow(aParentWidget, aBounds);
      NS_ENSURE_SUCCESS(rv, rv);
      Hide();

#ifdef NS_PRINT_PREVIEW
      if (mIsPageMode) {
        
        
        
        
#if 0
        nsCOMPtr<nsIDeviceContextSpec> devspec =
          do_CreateInstance("@mozilla.org/gfx/devicecontextspec;1", &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        
        rv = devspec->Init(mWindow, mPresContext->GetPrintSettings(), PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
        nsCOMPtr<nsIDeviceContext> devctx =
          do_CreateInstance("@mozilla.org/gfx/devicecontext;1", &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = devctx->InitForPrinting(devspec);
        NS_ENSURE_SUCCESS(rv, rv);
        
        
        
        
#endif
        double pageWidth = 0, pageHeight = 0;
        mPresContext->GetPrintSettings()->GetEffectivePageSize(&pageWidth,
                                                               &pageHeight);
        mPresContext->SetPageSize(
          nsSize(mPresContext->TwipsToAppUnits(pageWidth),
                 mPresContext->TwipsToAppUnits(pageHeight)));
        mPresContext->SetIsRootPaginatedDocument(PR_TRUE);
        mPresContext->SetPageScale(1.0f);
      }
#endif
    }
  }

  nsCOMPtr<nsIInterfaceRequestor> requestor(do_QueryReferent(mContainer));
  if (requestor) {
    if (mPresContext) {
      nsCOMPtr<nsILinkHandler> linkHandler;
      requestor->GetInterface(NS_GET_IID(nsILinkHandler),
                              getter_AddRefs(linkHandler));

      mPresContext->SetContainer(requestor);
      mPresContext->SetLinkHandler(linkHandler);
    }

    if (!aInPrintPreview) {
      

      nsCOMPtr<nsPIDOMWindow> window;
      requestor->GetInterface(NS_GET_IID(nsPIDOMWindow),
                              getter_AddRefs(window));

      if (window) {
        window->SetNewDocument(mDocument, aState, PR_TRUE);

        nsJSContext::LoadStart();
      }
    }
  }

  if (aDoCreation && mPresContext) {
    
    

    rv = InitPresentationStuff(!makeCX);
  }

  return rv;
}









NS_IMETHODIMP
DocumentViewerImpl::LoadComplete(nsresult aStatus)
{
  




  nsCOMPtr<nsIDocumentViewer> kungFuDeathGrip(this);

  
  
  
  if (mPresShell && !mStopped) {
    
    nsCOMPtr<nsIPresShell> shell = mPresShell;
    shell->FlushPendingNotifications(Flush_Layout);
  }
  
  nsresult rv = NS_OK;
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);

  
  nsPIDOMWindow *window = mDocument->GetWindow();

  
  NS_ENSURE_TRUE(window, NS_ERROR_NULL_POINTER);

  mLoaded = PR_TRUE;

  
  PRBool restoring = PR_FALSE;
  if(NS_SUCCEEDED(aStatus)) {
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event(PR_TRUE, NS_LOAD);
    event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
     
    event.target = mDocument;

    
    
    

    nsIDocShell *docShell = window->GetDocShell();
    NS_ENSURE_TRUE(docShell, NS_ERROR_UNEXPECTED);

    docShell->GetRestoringDocument(&restoring);
    if (!restoring) {
      nsEventDispatcher::Dispatch(window, mPresContext, &event, nsnull,
                                  &status);
#ifdef MOZ_TIMELINE
      
      

      

      nsIURI *uri = mDocument ? mDocument->GetDocumentURI() : nsnull;

      if (uri) {
        
        nsCAutoString spec;
        uri->GetSpec(spec);
        if (spec.EqualsLiteral("chrome://navigator/content/navigator.xul") ||
            spec.EqualsLiteral("chrome://browser/content/browser.xul")) {
          NS_TIMELINE_MARK("Navigator Window visible now");
        }
      }
#endif 
    }
  } else {
    
  }

  
  
  
  if (mDocument)
    mDocument->OnPageShow(restoring);

  
  
  if (mPresShell && !mStopped) {
    nsCOMPtr<nsIPresShell> shellDeathGrip(mPresShell); 
    mPresShell->UnsuppressPainting();
  }

  nsJSContext::LoadEnd();

#ifdef NS_PRINTING
  
  if (mPrintIsPending) {
    mPrintIsPending        = PR_FALSE;
    mPrintDocIsFullyLoaded = PR_TRUE;
    Print(mCachedPrintSettings, mCachedPrintWebProgressListner);
    mCachedPrintSettings           = nsnull;
    mCachedPrintWebProgressListner = nsnull;
  }
#endif

  return rv;
}

NS_IMETHODIMP
DocumentViewerImpl::PermitUnload(PRBool *aPermitUnload)
{
  *aPermitUnload = PR_TRUE;

  if (!mDocument || mInPermitUnload) {
    return NS_OK;
  }

  
  nsPIDOMWindow *window = mDocument->GetWindow();

  if (!window) {
    
    NS_WARNING("window not set for document!");
    return NS_OK;
  }

  
  
  nsEventStatus status = nsEventStatus_eIgnore;
  nsBeforePageUnloadEvent event(PR_TRUE, NS_BEFORE_PAGE_UNLOAD);
  event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
  
  event.target = mDocument;
  nsresult rv = NS_OK;

  
  
  nsRefPtr<DocumentViewerImpl> kungFuDeathGrip(this);

  {
    
    
    nsAutoPopupStatePusher popupStatePusher(openAbused, PR_TRUE);

    mInPermitUnload = PR_TRUE;
    nsEventDispatcher::Dispatch(window, mPresContext, &event, nsnull, &status);
    mInPermitUnload = PR_FALSE;
  }

  nsCOMPtr<nsIDocShellTreeNode> docShellNode(do_QueryReferent(mContainer));

  if (NS_SUCCEEDED(rv) && (event.flags & NS_EVENT_FLAG_NO_DEFAULT ||
                           !event.text.IsEmpty())) {
    

    nsCOMPtr<nsIPrompt> prompt = do_GetInterface(docShellNode);

    if (prompt) {
      nsXPIDLString preMsg, postMsg;
      rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                              "OnBeforeUnloadPreMessage",
                                              preMsg);
      rv |= nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                               "OnBeforeUnloadPostMessage",
                                               postMsg);

      
      if (NS_FAILED(rv) || preMsg.IsEmpty() || postMsg.IsEmpty()) {
        NS_ERROR("Failed to get strings from dom.properties!");
        return NS_OK;
      }

      
      
      PRInt32 len = PR_MIN(event.text.Length(), 1024);

      nsAutoString msg;
      if (len == 0) {
        msg = preMsg + NS_LITERAL_STRING("\n\n") + postMsg;
      } else {
        msg = preMsg + NS_LITERAL_STRING("\n\n") +
              StringHead(event.text, len) +
              NS_LITERAL_STRING("\n\n") + postMsg;
      } 

      
      
      
      if (NS_FAILED(prompt->Confirm(nsnull, msg.get(), aPermitUnload))) {
        *aPermitUnload = PR_TRUE;
      }
    }
  }

  if (docShellNode) {
    PRInt32 childCount;
    docShellNode->GetChildCount(&childCount);

    for (PRInt32 i = 0; i < childCount && *aPermitUnload; ++i) {
      nsCOMPtr<nsIDocShellTreeItem> item;
      docShellNode->GetChildAt(i, getter_AddRefs(item));

      nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(item));

      if (docShell) {
        nsCOMPtr<nsIContentViewer> cv;
      docShell->GetContentViewer(getter_AddRefs(cv));

      if (cv) {
        cv->PermitUnload(aPermitUnload);
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::PageHide(PRBool aIsUnload)
{
  mEnableRendering = PR_FALSE;

  if (!mDocument) {
    return NS_ERROR_NULL_POINTER;
  }

  mDocument->OnPageHide(!aIsUnload);
  if (aIsUnload) {
    
    NS_ENSURE_STATE(mDocument);

    
    nsPIDOMWindow *window = mDocument->GetWindow();

    if (!window) {
      
      NS_ERROR("window not set for document!");
      return NS_ERROR_NULL_POINTER;
    }

    
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event(PR_TRUE, NS_PAGE_UNLOAD);
    event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
    
    event.target = mDocument;

    
    
    nsAutoPopupStatePusher popupStatePusher(openAbused, PR_TRUE);

    nsEventDispatcher::Dispatch(window, mPresContext, &event, nsnull, &status);
  }

#ifdef MOZ_XUL
  
  
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && mDocument)
    pm->HidePopupsInDocument(mDocument);
#endif

  return NS_OK;
}

static void
AttachContainerRecurse(nsIDocShell* aShell)
{
  nsCOMPtr<nsIContentViewer> viewer;
  aShell->GetContentViewer(getter_AddRefs(viewer));
  nsCOMPtr<nsIDocumentViewer> docViewer = do_QueryInterface(viewer);
  if (docViewer) {
    nsCOMPtr<nsIDocument> doc;
    docViewer->GetDocument(getter_AddRefs(doc));
    if (doc) {
      doc->SetContainer(aShell);
    }
    nsCOMPtr<nsPresContext> pc;
    docViewer->GetPresContext(getter_AddRefs(pc));
    if (pc) {
      pc->SetContainer(aShell);
      pc->SetLinkHandler(nsCOMPtr<nsILinkHandler>(do_QueryInterface(aShell)));
    }
    nsCOMPtr<nsIPresShell> presShell;
    docViewer->GetPresShell(getter_AddRefs(presShell));
    if (presShell) {
      presShell->SetForwardingContainer(nsnull);
    }
  }

  
  nsCOMPtr<nsIDocShellTreeNode> node = do_QueryInterface(aShell);
  NS_ASSERTION(node, "docshells must implement nsIDocShellTreeNode");

  PRInt32 childCount;
  node->GetChildCount(&childCount);
  for (PRInt32 i = 0; i < childCount; ++i) {
    nsCOMPtr<nsIDocShellTreeItem> childItem;
    node->GetChildAt(i, getter_AddRefs(childItem));
    AttachContainerRecurse(nsCOMPtr<nsIDocShell>(do_QueryInterface(childItem)));
  }
}

NS_IMETHODIMP
DocumentViewerImpl::Open(nsISupports *aState, nsISHEntry *aSHEntry)
{
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);

  
  
  nsCOMPtr<nsISupports> container = do_QueryReferent(mContainer);
  if (!container)
    return NS_ERROR_NOT_AVAILABLE;

  nsRect bounds;
  mWindow->GetBounds(bounds);

  nsresult rv = InitInternal(mParentWidget, aState, mDeviceContext, bounds,
                             PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mDocument)
    mDocument->SetContainer(nsCOMPtr<nsISupports>(do_QueryReferent(mContainer)));

  if (mPresShell)
    mPresShell->SetForwardingContainer(nsnull);

  
  

  nsCOMPtr<nsIDocShellTreeItem> item;
  PRInt32 itemIndex = 0;
  while (NS_SUCCEEDED(aSHEntry->ChildShellAt(itemIndex++,
                                             getter_AddRefs(item))) && item) {
    AttachContainerRecurse(nsCOMPtr<nsIDocShell>(do_QueryInterface(item)));
  }
  
  SyncParentSubDocMap();

  if (mFocusListener && mDocument) {
    mDocument->AddEventListenerByIID(mFocusListener,
                                     NS_GET_IID(nsIDOMFocusListener));
  }

  

  PrepareToStartLoad();
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Close(nsISHEntry *aSHEntry)
{
  
  
  
  

  mSHEntry = aSHEntry;

  
  
  
  
  

  if (!mDocument)
    return NS_OK;

#if defined(NS_PRINTING) && defined(NS_PRINT_PREVIEW)
  
  
  if (GetIsPrintPreview() && mPrintEngine) {
    mPrintEngine->TurnScriptingOn(PR_TRUE);
  }
#endif

#ifdef NS_PRINTING
  
  
  
  
  if (mPrintEngine && !mClosingWhilePrinting) {
    mClosingWhilePrinting = PR_TRUE;
    NS_ADDREF_THIS();
  } else
#endif
    {
      
      mDocument->SetScriptGlobalObject(nsnull);

      if (!mSHEntry)
        mDocument->Destroy();
    }

  if (mFocusListener && mDocument) {
    mDocument->RemoveEventListenerByIID(mFocusListener,
                                        NS_GET_IID(nsIDOMFocusListener));
  }

  return NS_OK;
}

static void
DetachContainerRecurse(nsIDocShell *aShell)
{
  
  nsCOMPtr<nsIContentViewer> viewer;
  aShell->GetContentViewer(getter_AddRefs(viewer));
  nsCOMPtr<nsIDocumentViewer> docViewer = do_QueryInterface(viewer);
  if (docViewer) {
    nsCOMPtr<nsIDocument> doc;
    docViewer->GetDocument(getter_AddRefs(doc));
    if (doc) {
      doc->SetContainer(nsnull);
    }
    nsCOMPtr<nsPresContext> pc;
    docViewer->GetPresContext(getter_AddRefs(pc));
    if (pc) {
      pc->SetContainer(nsnull);
      pc->SetLinkHandler(nsnull);
    }
    nsCOMPtr<nsIPresShell> presShell;
    docViewer->GetPresShell(getter_AddRefs(presShell));
    if (presShell) {
      presShell->SetForwardingContainer(nsWeakPtr(do_GetWeakReference(aShell)));
    }
  }

  
  nsCOMPtr<nsIDocShellTreeNode> node = do_QueryInterface(aShell);
  NS_ASSERTION(node, "docshells must implement nsIDocShellTreeNode");

  PRInt32 childCount;
  node->GetChildCount(&childCount);
  for (PRInt32 i = 0; i < childCount; ++i) {
    nsCOMPtr<nsIDocShellTreeItem> childItem;
    node->GetChildAt(i, getter_AddRefs(childItem));
    DetachContainerRecurse(nsCOMPtr<nsIDocShell>(do_QueryInterface(childItem)));
  }
}

NS_IMETHODIMP
DocumentViewerImpl::Destroy()
{
  NS_ASSERTION(mDocument, "No document in Destroy()!");

#ifdef NS_PRINTING
  
  
  
  
  
  
  if (mPrintEngine) {
    if (mPrintEngine->CheckBeforeDestroy()) {
      return NS_OK;
    }
  }
#endif

  
  
  
  
  if (mDestroyRefCount != 0) {
    --mDestroyRefCount;
    return NS_OK;
  }

  
  
  if (mSHEntry) {
    if (mPresShell)
      mPresShell->Freeze();

    
    mSHEntry->SetSticky(mIsSticky);
    mIsSticky = PR_TRUE;

    PRBool savePresentation = PR_TRUE;

    
    if (mPresShell) {
      nsIViewManager *vm = mPresShell->GetViewManager();
      if (vm) {
        nsIView *rootView = nsnull;
        vm->GetRootView(rootView);

        if (rootView) {
          nsIView *rootViewParent = rootView->GetParent();
          if (rootViewParent) {
            nsIViewManager *parentVM = rootViewParent->GetViewManager();
            if (parentVM) {
              parentVM->RemoveChild(rootView);
            }
          }
        }
      }
    }

    Hide();

    
    if (mDocument) {
      nsresult rv = mDocument->Sanitize();
      if (NS_FAILED(rv)) {
        
        savePresentation = PR_FALSE;
      }
    }


    
    
    if (savePresentation) {
      mSHEntry->SetContentViewer(this);
    }
    else {
      mSHEntry->SyncPresentationState();
    }
    nsCOMPtr<nsISHEntry> shEntry = mSHEntry; 
    mSHEntry = nsnull;

    
    
    
    

    if (mDocument)
      mDocument->SetContainer(nsnull);
    if (mPresContext) {
      mPresContext->SetLinkHandler(nsnull);
      mPresContext->SetContainer(nsnull);
    }
    if (mPresShell)
      mPresShell->SetForwardingContainer(mContainer);

    
    
    nsCOMPtr<nsIDocShellTreeItem> item;
    PRInt32 itemIndex = 0;
    while (NS_SUCCEEDED(shEntry->ChildShellAt(itemIndex++,
                                              getter_AddRefs(item))) && item) {
      DetachContainerRecurse(nsCOMPtr<nsIDocShell>(do_QueryInterface(item)));
    }

    return NS_OK;
  }

  if (mDocument) {
    mDocument->Destroy();
    mDocument = nsnull;
  }

  
  
  
  

#ifdef NS_PRINTING
  if (mPrintEngine) {
    mPrintEngine->Destroy();
    mPrintEngine = nsnull;
  }
#endif

  
  if (mPreviousViewer) {
    mPreviousViewer->Destroy();
    mPreviousViewer = nsnull;
  }

  if (mDeviceContext) {
    mDeviceContext->FlushFontCache();
    mDeviceContext = nsnull;
  }

  if (mPresShell) {
    
    mPresShell->EndObservingDocument();

    nsCOMPtr<nsISelection> selection;
    GetDocumentSelection(getter_AddRefs(selection));

    nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(selection));

    if (selPrivate && mSelectionListener)
      selPrivate->RemoveSelectionListener(mSelectionListener);

    mPresShell->Destroy();
    mPresShell = nsnull;
  }

  if (mPresContext) {
    mPresContext->SetContainer(nsnull);
    mPresContext->SetLinkHandler(nsnull);
    mPresContext = nsnull;
  }

  mContainer = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Stop(void)
{
  NS_ASSERTION(mDocument, "Stop called too early or too late");
  if (mDocument) {
    mDocument->StopDocumentLoad();
  }

  if (mEnableRendering && (mLoaded || mStopped) && mPresContext && !mSHEntry)
    mPresContext->SetImageAnimationMode(imgIContainer::kDontAnimMode);

  mStopped = PR_TRUE;

  if (!mLoaded && mPresShell) {
    
    nsCOMPtr<nsIPresShell> shellDeathGrip(mPresShell); 
    mPresShell->UnsuppressPainting();
  }

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetDOMDocument(nsIDOMDocument **aResult)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  return CallQueryInterface(mDocument, aResult);
}

NS_IMETHODIMP
DocumentViewerImpl::SetDOMDocument(nsIDOMDocument *aDocument)
{
  
  
  
  
  

  
  
  
  
  

  nsresult rv;
  if (!aDocument)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDocument> newDoc = do_QueryInterface(aDocument, &rv);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsISupports> container = do_QueryReferent(mContainer);
  newDoc->SetContainer(container);

  if (mDocument != newDoc) {
    
    
    mDocument = newDoc;

    
    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(container);
    if (window) {
      window->SetNewDocument(newDoc, nsnull, PR_TRUE);
    }

    
    
    nsCOMPtr<nsIDocShellTreeNode> node = do_QueryInterface(container);
    if (node) {
      PRInt32 count;
      node->GetChildCount(&count);
      for (PRInt32 i = 0; i < count; ++i) {
        nsCOMPtr<nsIDocShellTreeItem> child;
        node->GetChildAt(0, getter_AddRefs(child));
        node->RemoveChild(child);
      }
    }
  }

  rv = SyncParentSubDocMap();
  NS_ENSURE_SUCCESS(rv, rv);

  

  nsCOMPtr<nsILinkHandler> linkHandler;
  if (mPresShell) {
    if (mPresContext) {
      
      
      linkHandler = mPresContext->GetLinkHandler();
    }

    mPresShell->EndObservingDocument();
    mPresShell->Destroy();

    mPresShell = nsnull;
  }

  
  if (mPresContext) {
    
    if (linkHandler) {
      mPresContext->SetLinkHandler(linkHandler);
    }

    

    nsStyleSet *styleSet;
    rv = CreateStyleSet(mDocument, &styleSet);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = newDoc->CreateShell(mPresContext, mViewManager, styleSet,
                             getter_AddRefs(mPresShell));
    if (NS_FAILED(rv)) {
      delete styleSet;
      return rv;
    }

    
    styleSet->EndUpdate();

    
    mPresShell->BeginObservingDocument();

    
    if (mDocument) {
      rv = mDocument->AddEventListenerByIID(mFocusListener,
                                            NS_GET_IID(nsIDOMFocusListener));
      NS_ASSERTION(NS_SUCCEEDED(rv), "failed to register focus listener");
    }
  }

  return rv;
}

NS_IMETHODIMP
DocumentViewerImpl::SetUAStyleSheet(nsIStyleSheet* aUAStyleSheet)
{
  NS_ASSERTION(aUAStyleSheet, "unexpected null pointer");
  nsCOMPtr<nsICSSStyleSheet> sheet(do_QueryInterface(aUAStyleSheet));
  if (sheet) {
    nsCOMPtr<nsICSSStyleSheet> newSheet;
    sheet->Clone(nsnull, nsnull, nsnull, nsnull, getter_AddRefs(newSheet));
    mUAStyleSheet = newSheet;
  }
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetDocument(nsIDocument** aResult)
{
  NS_IF_ADDREF(*aResult = mDocument);

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetPresShell(nsIPresShell** aResult)
{
  NS_IF_ADDREF(*aResult = mPresShell);

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetPresContext(nsPresContext** aResult)
{
  NS_IF_ADDREF(*aResult = mPresContext);

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetBounds(nsRect& aResult)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  NS_PRECONDITION(mWindow, "null window");
  if (mWindow) {
    mWindow->GetBounds(aResult);
  }
  else {
    aResult.SetRect(0, 0, 0, 0);
  }
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetPreviousViewer(nsIContentViewer** aViewer)
{
  *aViewer = mPreviousViewer;
  NS_IF_ADDREF(*aViewer);
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::SetPreviousViewer(nsIContentViewer* aViewer)
{
  
  

  if (aViewer) {
    NS_ASSERTION(!mPreviousViewer,
                 "can't set previous viewer when there already is one");

    
    
    
    
    
    
    
    nsCOMPtr<nsIContentViewer> prevViewer;
    aViewer->GetPreviousViewer(getter_AddRefs(prevViewer));
    if (prevViewer) {
      aViewer->SetPreviousViewer(nsnull);
      aViewer->Destroy();
      return SetPreviousViewer(prevViewer);
    }
  }

  mPreviousViewer = aViewer;
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::SetBounds(const nsRect& aBounds)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);

  if (mWindow) {
    
    
    mWindow->Resize(aBounds.x, aBounds.y, aBounds.width, aBounds.height,
                    PR_FALSE);
  }

  
  
  
  
  
  
  
  if (mPreviousViewer)
    mPreviousViewer->SetBounds(aBounds);

#if defined(NS_PRINTING) && defined(NS_PRINT_PREVIEW)
  if (GetIsPrintPreview() && !mPrintEngine->GetIsCreatingPrintPreview()) {
    mPrintEngine->GetPrintPreviewWindow()->Resize(aBounds.x, aBounds.y,
                                                  aBounds.width, aBounds.height,
                                                  PR_FALSE);
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Move(PRInt32 aX, PRInt32 aY)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  NS_PRECONDITION(mWindow, "null window");
  if (mWindow) {
    mWindow->Move(aX, aY);
  }
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Show(void)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);

  
  
  if (mPreviousViewer) {
    
    
    nsCOMPtr<nsIContentViewer> prevViewer(mPreviousViewer);
    mPreviousViewer = nsnull;
    prevViewer->Destroy();

    
    nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryReferent(mContainer);
    if (treeItem) {
      
      
      nsCOMPtr<nsIDocShellTreeItem> root;
      treeItem->GetSameTypeRootTreeItem(getter_AddRefs(root));
      nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(root);
      nsCOMPtr<nsISHistory> history;
      webNav->GetSessionHistory(getter_AddRefs(history));
      nsCOMPtr<nsISHistoryInternal> historyInt = do_QueryInterface(history);
      if (historyInt) {
        PRInt32 prevIndex,loadedIndex;
        nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(treeItem);
        docShell->GetPreviousTransIndex(&prevIndex);
        docShell->GetLoadedTransIndex(&loadedIndex);
#ifdef DEBUG_PAGE_CACHE
        printf("About to evict content viewers: prev=%d, loaded=%d\n",
               prevIndex, loadedIndex);
#endif
        historyInt->EvictContentViewers(prevIndex, loadedIndex);
      }
    }
  }

  if (mWindow) {
    mWindow->Show(PR_TRUE);
  }

  if (mDocument && !mPresShell && !mWindow) {
    nsresult rv;

    nsCOMPtr<nsIBaseWindow> base_win(do_QueryReferent(mContainer));
    NS_ENSURE_TRUE(base_win, NS_ERROR_UNEXPECTED);

    base_win->GetParentWidget(&mParentWidget);
    NS_ENSURE_TRUE(mParentWidget, NS_ERROR_UNEXPECTED);

    mDeviceContext = mParentWidget->GetDeviceContext();

    
    NS_ASSERTION(!mPresContext, "Shouldn't have a prescontext if we have no shell!");
    mPresContext = new nsPresContext(mDocument, nsPresContext::eContext_Galley);
    NS_ENSURE_TRUE(mPresContext, NS_ERROR_OUT_OF_MEMORY);

    rv = mPresContext->Init(mDeviceContext);
    if (NS_FAILED(rv)) {
      mPresContext = nsnull;
      return rv;
    }

    nsRect tbounds;
    mParentWidget->GetBounds(tbounds);

    rv = MakeWindow(mParentWidget, tbounds);
    if (NS_FAILED(rv))
      return rv;

    if (mPresContext && base_win) {
      nsCOMPtr<nsILinkHandler> linkHandler(do_GetInterface(base_win));

      if (linkHandler) {
        mPresContext->SetLinkHandler(linkHandler);
      }

      mPresContext->SetContainer(base_win);
    }

    if (mPresContext) {
      Hide();

      rv = InitPresentationStuff(PR_TRUE);
    }

    
    
    

    nsCOMPtr<nsIPresShell> shellDeathGrip(mPresShell); 
    mPresShell->UnsuppressPainting();
  }

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Hide(void)
{
  NS_PRECONDITION(mWindow, "null window");
  if (mWindow) {
    mWindow->Show(PR_FALSE);
  }

  if (!mPresShell)
    return NS_OK;

  NS_ASSERTION(mPresContext, "Can't have a presshell and no prescontext!");

  
  if (mPreviousViewer) {
    mPreviousViewer->Destroy();
    mPreviousViewer = nsnull;
  }

  if (mIsSticky) {
    
    
    

    return NS_OK;
  }

  if (mDeviceContext) {
    mDeviceContext->FlushFontCache();
  }

  
  mPresShell->EndObservingDocument();
  nsCOMPtr<nsISelection> selection;

  GetDocumentSelection(getter_AddRefs(selection));

  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(selection));

  if (selPrivate && mSelectionListener) {
    selPrivate->RemoveSelectionListener(mSelectionListener);
  }

  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mContainer));
  if (docShell) {
    PRBool saveLayoutState = PR_FALSE;
    docShell->GetShouldSaveLayoutState(&saveLayoutState);
    if (saveLayoutState) {
      nsCOMPtr<nsILayoutHistoryState> layoutState;
      mPresShell->CaptureHistoryState(getter_AddRefs(layoutState), PR_TRUE);
    }
  }

  mPresShell->Destroy();
  
  mPresContext->SetContainer(nsnull);
  mPresContext->SetLinkHandler(nsnull);                             

  mPresShell     = nsnull;
  mPresContext   = nsnull;
  mViewManager   = nsnull;
  mWindow        = nsnull;
  mDeviceContext = nsnull;
  mParentWidget  = nsnull;

  nsCOMPtr<nsIBaseWindow> base_win(do_QueryReferent(mContainer));

  if (base_win) {
    base_win->SetParentWidget(nsnull);
  }

  return NS_OK;
}
NS_IMETHODIMP
DocumentViewerImpl::SetEnableRendering(PRBool aOn)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  mEnableRendering = aOn;
  if (mViewManager) {
    if (aOn) {
      mViewManager->EnableRefresh(NS_VMREFRESH_IMMEDIATE);
      nsIView* view;
      mViewManager->GetRootView(view);   
      if (view) {
        mViewManager->UpdateView(view, NS_VMREFRESH_IMMEDIATE);
      }
    }
    else {
      mViewManager->DisableRefresh();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetSticky(PRBool *aSticky)
{
  *aSticky = mIsSticky;

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::SetSticky(PRBool aSticky)
{
  mIsSticky = aSticky;

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetEnableRendering(PRBool* aResult)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  NS_PRECONDITION(nsnull != aResult, "null OUT ptr");
  if (aResult) {
    *aResult = mEnableRendering;
  }
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::RequestWindowClose(PRBool* aCanClose)
{
#ifdef NS_PRINTING
  if (mPrintIsPending || (mPrintEngine && mPrintEngine->GetIsPrinting())) {
    *aCanClose = PR_FALSE;
    mDeferredWindowClose = PR_TRUE;
  } else
#endif
    *aCanClose = PR_TRUE;

  return NS_OK;
}

PR_STATIC_CALLBACK(PRBool)
AppendAgentSheet(nsIStyleSheet *aSheet, void *aData)
{
  nsStyleSet *styleSet = static_cast<nsStyleSet*>(aData);
  styleSet->AppendStyleSheet(nsStyleSet::eAgentSheet, aSheet);
  return PR_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
PrependUserSheet(nsIStyleSheet *aSheet, void *aData)
{
  nsStyleSet *styleSet = static_cast<nsStyleSet*>(aData);
  styleSet->PrependStyleSheet(nsStyleSet::eUserSheet, aSheet);
  return PR_TRUE;
}

nsresult
DocumentViewerImpl::CreateStyleSet(nsIDocument* aDocument,
                                   nsStyleSet** aStyleSet)
{
  

  
  
  if (!mUAStyleSheet) {
    NS_WARNING("unable to load UA style sheet");
  }

  nsStyleSet *styleSet = new nsStyleSet();
  if (!styleSet) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  styleSet->BeginUpdate();
  
  
  
  
  nsCOMPtr<nsIDocShellTreeItem> docShell(do_QueryReferent(mContainer));
  PRInt32 shellType;
  docShell->GetItemType(&shellType);
  nsICSSStyleSheet* sheet = nsnull;
  if (shellType == nsIDocShellTreeItem::typeChrome) {
    sheet = nsLayoutStylesheetCache::UserChromeSheet();
  }
  else {
    sheet = nsLayoutStylesheetCache::UserContentSheet();
  }

  if (sheet)
    styleSet->AppendStyleSheet(nsStyleSet::eUserSheet, sheet);

  
  PRBool shouldOverride = PR_FALSE;
  nsCOMPtr<nsIDocShell> ds(do_QueryInterface(docShell));
  nsCOMPtr<nsIDOMEventTarget> chromeHandler;
  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<nsICSSStyleSheet> csssheet;

  ds->GetChromeEventHandler(getter_AddRefs(chromeHandler));
  if (chromeHandler) {
    nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(chromeHandler));
    nsCOMPtr<nsIContent> content(do_QueryInterface(elt));
    if (elt && content) {
      nsCOMPtr<nsIURI> baseURI = content->GetBaseURI();

      nsAutoString sheets;
      elt->GetAttribute(NS_LITERAL_STRING("usechromesheets"), sheets);
      if (!sheets.IsEmpty() && baseURI) {
        nsCOMPtr<nsICSSLoader> cssLoader;
        NS_NewCSSLoader(getter_AddRefs(cssLoader));

        char *str = ToNewCString(sheets);
        char *newStr = str;
        char *token;
        while ( (token = nsCRT::strtok(newStr, ", ", &newStr)) ) {
          NS_NewURI(getter_AddRefs(uri), nsDependentCString(token), nsnull,
                    baseURI);
          if (!uri) continue;

          cssLoader->LoadSheetSync(uri, getter_AddRefs(csssheet));
          if (!sheet) continue;

          styleSet->PrependStyleSheet(nsStyleSet::eAgentSheet, csssheet);
          shouldOverride = PR_TRUE;
        }
        nsMemory::Free(str);
      }
    }
  }

  if (!shouldOverride) {
    sheet = nsLayoutStylesheetCache::ScrollbarsSheet();
    if (sheet) {
      styleSet->PrependStyleSheet(nsStyleSet::eAgentSheet, sheet);
    }
  }

  sheet = nsLayoutStylesheetCache::FormsSheet();
  if (sheet) {
    styleSet->PrependStyleSheet(nsStyleSet::eAgentSheet, sheet);
  }

  if (mUAStyleSheet) {
    styleSet->PrependStyleSheet(nsStyleSet::eAgentSheet, mUAStyleSheet);
  }

  nsCOMPtr<nsIStyleSheetService> dummy =
    do_GetService(NS_STYLESHEETSERVICE_CONTRACTID);

  nsStyleSheetService *sheetService = nsStyleSheetService::gInstance;
  if (sheetService) {
    sheetService->AgentStyleSheets()->EnumerateForwards(AppendAgentSheet,
                                                        styleSet);
    sheetService->UserStyleSheets()->EnumerateBackwards(PrependUserSheet,
                                                        styleSet);
  }

  
  *aStyleSet = styleSet;
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::ClearHistoryEntry()
{
  mSHEntry = nsnull;
  return NS_OK;
}



nsresult
DocumentViewerImpl::MakeWindow(nsIWidget* aParentWidget,
                               const nsRect& aBounds)
{
  nsresult rv;

  mViewManager = do_CreateInstance(kViewManagerCID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsIDeviceContext *dx = mPresContext->DeviceContext();

  nsRect tbounds = aBounds;
  tbounds *= mPresContext->AppUnitsPerDevPixel();

   
   
  rv = mViewManager->Init(dx);
  if (NS_FAILED(rv))
    return rv;

  
  
  
  
  
  tbounds.x = 0;
  tbounds.y = 0;

  
  
  nsIView* containerView = nsIView::GetViewFor(aParentWidget);

  if (containerView) {
    
    
    nsIViewManager* containerVM = containerView->GetViewManager();
    nsIView* pView = containerView;
    do {
      pView = pView->GetParent();
    } while (pView && pView->GetViewManager() == containerVM);

    if (!pView) {
      
      
      
      
      
      
      
      
      nsCOMPtr<nsIDocShellTreeItem> container(do_QueryReferent(mContainer));
      nsCOMPtr<nsIDocShellTreeItem> parentContainer;
      PRInt32 itemType;
      if (nsnull == container
          || NS_FAILED(container->GetParent(getter_AddRefs(parentContainer)))
          || nsnull == parentContainer
          || NS_FAILED(parentContainer->GetItemType(&itemType))
          || itemType != nsIDocShellTreeItem::typeContent) {
        containerView = nsnull;
      }
    }
  }

  
  nsIView* view = mViewManager->CreateView(tbounds, containerView);
  if (!view)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  rv = view->CreateWidget(kWidgetCID, nsnull,
                          containerView != nsnull ? nsnull : aParentWidget->GetNativeData(NS_NATIVE_WIDGET),
                          PR_TRUE, PR_FALSE);
  if (NS_FAILED(rv))
    return rv;

  
  mViewManager->SetRootView(view);

  mWindow = view->GetWidget();

  
  
  
  

  return rv;
}




nsresult DocumentViewerImpl::GetDocumentSelection(nsISelection **aSelection)
{
  NS_ENSURE_ARG_POINTER(aSelection);
  if (!mPresShell) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsCOMPtr<nsISelectionController> selcon;
  selcon = do_QueryInterface(mPresShell);
  if (selcon)
    return selcon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                                aSelection);
  return NS_ERROR_FAILURE;
}





NS_IMETHODIMP DocumentViewerImpl::Search()
{
  
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetSearchable(PRBool *aSearchable)
{
  
  *aSearchable = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::ClearSelection()
{
  nsresult rv;
  nsCOMPtr<nsISelection> selection;

  
  rv = GetDocumentSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;

  return selection->CollapseToStart();
}

NS_IMETHODIMP DocumentViewerImpl::SelectAll()
{
  
  
  
  nsCOMPtr<nsISelection> selection;
  nsresult rv;

  
  rv = GetDocumentSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDOMHTMLDocument> htmldoc = do_QueryInterface(mDocument);
  nsCOMPtr<nsIDOMNode> bodyNode;

  if (htmldoc)
  {
    nsCOMPtr<nsIDOMHTMLElement>bodyElement;
    rv = htmldoc->GetBody(getter_AddRefs(bodyElement));
    if (NS_FAILED(rv) || !bodyElement) return rv;

    bodyNode = do_QueryInterface(bodyElement);
  }
  else if (mDocument)
  {
    bodyNode = do_QueryInterface(mDocument->GetRootContent());
  }
  if (!bodyNode) return NS_ERROR_FAILURE;

  rv = selection->RemoveAllRanges();
  if (NS_FAILED(rv)) return rv;

  rv = selection->SelectAllChildren(bodyNode);
  return rv;
}

NS_IMETHODIMP DocumentViewerImpl::CopySelection()
{
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);

  
  nsresult rv;
  nsCOMPtr<nsIDOMNode> eventTarget;
  rv = GetClipboardEventTarget(getter_AddRefs(eventTarget));
  
  if (NS_SUCCEEDED(rv)) {
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent evt(PR_TRUE, NS_COPY);
    nsEventDispatcher::Dispatch(eventTarget, mPresContext, &evt, nsnull,
                                &status);
    
    if (status == nsEventStatus_eConsumeNoDefault)
      
      return NS_OK;
    
    if (!mPresShell)
      return NS_OK;
  }

  return mPresShell->DoCopy();
}

NS_IMETHODIMP DocumentViewerImpl::CopyLinkLocation()
{
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIDOMNode> node;
  GetPopupLinkNode(getter_AddRefs(node));
  
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  nsAutoString locationText;
  nsresult rv = mPresShell->GetLinkLocation(node, locationText);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIClipboardHelper> clipboard(do_GetService("@mozilla.org/widget/clipboardhelper;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  return clipboard->CopyString(locationText);
}

NS_IMETHODIMP DocumentViewerImpl::CopyImage(PRInt32 aCopyFlags)
{
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIImageLoadingContent> node;
  GetPopupImageNode(getter_AddRefs(node));
  
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  return nsCopySupport::ImageCopy(node, aCopyFlags);
}

nsresult DocumentViewerImpl::GetClipboardEventTarget(nsIDOMNode** aEventTarget)
{
  NS_ENSURE_ARG_POINTER(aEventTarget);
  *aEventTarget = nsnull;

  if (!mPresShell)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsISelection> sel;
  nsresult rv = mPresShell->GetSelectionForCopy(getter_AddRefs(sel));
  if (NS_FAILED(rv))
    return rv;
  if (!sel)
    return NS_ERROR_FAILURE;

  return nsCopySupport::GetClipboardEventTarget(sel, aEventTarget);
}

NS_IMETHODIMP DocumentViewerImpl::GetCopyable(PRBool *aCopyable)
{
  NS_ENSURE_ARG_POINTER(aCopyable);
  *aCopyable = PR_FALSE;

  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);

  
  
  nsCOMPtr<nsIDOMNode> eventTarget;
  nsresult rv = GetClipboardEventTarget(getter_AddRefs(eventTarget));
  
  if (NS_SUCCEEDED(rv)) {
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent evt(PR_TRUE, NS_BEFORECOPY);
    nsEventDispatcher::Dispatch(eventTarget, mPresContext, &evt, nsnull,
                                &status);
    
    if (status == nsEventStatus_eConsumeNoDefault) {
      *aCopyable = PR_TRUE;
      return NS_OK;
    }
    
    if (!mPresShell)
      return NS_OK;
  }

  nsCOMPtr<nsISelection> selection;
  rv = mPresShell->GetSelectionForCopy(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;

  PRBool isCollapsed;
  selection->GetIsCollapsed(&isCollapsed);

  *aCopyable = !isCollapsed;
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::CutSelection()
{
  NS_ENSURE_TRUE(mPresContext, NS_ERROR_NOT_INITIALIZED);

  
  nsresult rv;
  nsCOMPtr<nsIDOMNode> eventTarget;
  rv = GetClipboardEventTarget(getter_AddRefs(eventTarget));
  
  if (NS_SUCCEEDED(rv)) {
    nsEvent evt(PR_TRUE, NS_CUT);
    nsEventDispatcher::Dispatch(eventTarget, mPresContext, &evt);
    
    
  }

  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetCutable(PRBool *aCutable)
{
  NS_ENSURE_ARG_POINTER(aCutable);
  *aCutable = PR_FALSE;

  NS_ENSURE_TRUE(mPresContext, NS_ERROR_NOT_INITIALIZED);

  
  
  nsCOMPtr<nsIDOMNode> eventTarget;
  nsresult rv = GetClipboardEventTarget(getter_AddRefs(eventTarget));
  
  if (NS_SUCCEEDED(rv)) {
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent evt(PR_TRUE, NS_BEFORECUT);
    nsEventDispatcher::Dispatch(eventTarget, mPresContext, &evt, nsnull,
                                &status);
    
    if (status == nsEventStatus_eConsumeNoDefault) {
      *aCutable = PR_TRUE;
      return NS_OK;
    }
  }

  *aCutable = PR_FALSE;  
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::Paste()
{
  NS_ENSURE_TRUE(mPresContext, NS_ERROR_NOT_INITIALIZED);

  
  nsresult rv;
  nsCOMPtr<nsIDOMNode> eventTarget;
  rv = GetClipboardEventTarget(getter_AddRefs(eventTarget));
  
  if (NS_SUCCEEDED(rv)) {
    nsEvent evt(PR_TRUE, NS_PASTE);
    nsEventDispatcher::Dispatch(eventTarget, mPresContext, &evt);
    
    
  }

  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetPasteable(PRBool *aPasteable)
{
  NS_ENSURE_ARG_POINTER(aPasteable);
  *aPasteable = PR_FALSE;

  NS_ENSURE_TRUE(mPresContext, NS_ERROR_NOT_INITIALIZED);

  
  
  nsCOMPtr<nsIDOMNode> eventTarget;
  nsresult rv = GetClipboardEventTarget(getter_AddRefs(eventTarget));
  
  if (NS_SUCCEEDED(rv)) {
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent evt(PR_TRUE, NS_BEFOREPASTE);
    nsEventDispatcher::Dispatch(eventTarget, mPresContext, &evt, nsnull,
                                &status);
    
    if (status == nsEventStatus_eConsumeNoDefault) {
      *aPasteable = PR_TRUE;
      return NS_OK;
    }
  }

  *aPasteable = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP DocumentViewerImpl::GetContents(const char *mimeType, PRBool selectionOnly, nsAString& aOutValue)
{
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);
  return mPresShell->DoGetContents(nsDependentCString(mimeType), 0, selectionOnly, aOutValue);
}


NS_IMETHODIMP DocumentViewerImpl::GetCanGetContents(PRBool *aCanGetContents)
{
  return GetCopyable(aCanGetContents);
}

#ifdef XP_MAC
#pragma mark -
#endif








NS_IMETHODIMP
DocumentViewerImpl::Print(PRBool            aSilent,
                          FILE *            aDebugFile,
                          nsIPrintSettings* aPrintSettings)
{
#ifdef NS_PRINTING
  nsCOMPtr<nsIPrintSettings> printSettings;

#ifdef NS_DEBUG
  nsresult rv = NS_ERROR_FAILURE;

  mDebugFile = aDebugFile;
  
  
  printSettings = aPrintSettings;
  nsCOMPtr<nsIPrintOptions> printOptions = do_GetService(sPrintOptionsContractID, &rv);
  if (NS_SUCCEEDED(rv)) {
    
    if (printSettings == nsnull) {
      printOptions->CreatePrintSettings(getter_AddRefs(printSettings));
    }
    NS_ASSERTION(printSettings, "You can't PrintPreview without a PrintSettings!");
  }
  if (printSettings) printSettings->SetPrintSilent(aSilent);
  if (printSettings) printSettings->SetShowPrintProgress(PR_FALSE);
#endif


  return Print(printSettings, nsnull);
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP 
DocumentViewerImpl::PrintWithParent(nsIDOMWindowInternal *aParentWin, nsIPrintSettings *aThePrintSettings, nsIWebProgressListener *aWPListener)
{
#ifdef NS_PRINTING
  return Print(aThePrintSettings, aWPListener);
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP
DocumentViewerImpl::GetPrintable(PRBool *aPrintable)
{
  NS_ENSURE_ARG_POINTER(aPrintable);

  *aPrintable = !GetIsPrinting();

  return NS_OK;
}





NS_IMETHODIMP DocumentViewerImpl::ScrollToNode(nsIDOMNode* aNode)
{
   NS_ENSURE_ARG(aNode);
   NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
   nsCOMPtr<nsIPresShell> presShell;
   NS_ENSURE_SUCCESS(GetPresShell(getter_AddRefs(presShell)), NS_ERROR_FAILURE);

   
   

   nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
   NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

   
   NS_ENSURE_SUCCESS(presShell->ScrollContentIntoView(content,
                                                      NS_PRESSHELL_SCROLL_TOP,
                                                      NS_PRESSHELL_SCROLL_ANYWHERE),
                     NS_ERROR_FAILURE);
   return NS_OK;
}

void
DocumentViewerImpl::CallChildren(CallChildFunc aFunc, void* aClosure)
{
  nsCOMPtr<nsIDocShellTreeNode> docShellNode(do_QueryReferent(mContainer));
  if (docShellNode)
  {
    PRInt32 i;
    PRInt32 n;
    docShellNode->GetChildCount(&n);
    for (i=0; i < n; i++)
    {
      nsCOMPtr<nsIDocShellTreeItem> child;
      docShellNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));
      NS_ASSERTION(childAsShell, "null child in docshell");
      if (childAsShell)
      {
        nsCOMPtr<nsIContentViewer> childCV;
        childAsShell->GetContentViewer(getter_AddRefs(childCV));
        if (childCV)
        {
          nsCOMPtr<nsIMarkupDocumentViewer> markupCV = do_QueryInterface(childCV);
          if (markupCV) {
            (*aFunc)(markupCV, aClosure);
          }
        }
      }
    }
  }
}

struct TextZoomInfo
{
  float mTextZoom;
};

static void
SetChildTextZoom(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  struct TextZoomInfo* textZoomInfo = (struct TextZoomInfo*) aClosure;
  aChild->SetTextZoom(textZoomInfo->mTextZoom);
}

NS_IMETHODIMP
DocumentViewerImpl::SetTextZoom(float aTextZoom)
{
  mTextZoom = aTextZoom;

  if (mViewManager) {
    mViewManager->BeginUpdateViewBatch();
  }
      
  
  
  
  
  struct TextZoomInfo textZoomInfo = { aTextZoom };
  CallChildren(SetChildTextZoom, &textZoomInfo);

  
  if (mPresContext && aTextZoom != mPresContext->TextZoom()) {
      mPresContext->SetTextZoom(aTextZoom);
  }

  if (mViewManager) {
    mViewManager->EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetTextZoom(float* aTextZoom)
{
  NS_ENSURE_ARG_POINTER(aTextZoom);
  NS_ASSERTION(!mPresContext || mPresContext->TextZoom() == mTextZoom, 
               "mPresContext->TextZoom() != mTextZoom");

  *aTextZoom = mTextZoom;
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::SetFullZoom(float aFullZoom)
{
  if (mPresContext) {
      mPresContext->SetFullZoom(aFullZoom);
  }

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetFullZoom(float* aFullZoom)
{
  NS_ENSURE_ARG_POINTER(aFullZoom);
  *aFullZoom = mPresContext ? mPresContext->GetFullZoom() : 1.0;
  return NS_OK;
}

static void
SetChildAuthorStyleDisabled(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  PRBool styleDisabled  = *static_cast<PRBool*>(aClosure);
  aChild->SetAuthorStyleDisabled(styleDisabled);
}


NS_IMETHODIMP
DocumentViewerImpl::SetAuthorStyleDisabled(PRBool aStyleDisabled)
{
  if (mPresShell) {
    mPresShell->SetAuthorStyleDisabled(aStyleDisabled);
  }
  CallChildren(SetChildAuthorStyleDisabled, &aStyleDisabled);
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetAuthorStyleDisabled(PRBool* aStyleDisabled)
{
  if (mPresShell) {
    *aStyleDisabled = mPresShell->GetAuthorStyleDisabled();
  } else {
    *aStyleDisabled = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetDefaultCharacterSet(nsACString& aDefaultCharacterSet)
{
  NS_ENSURE_STATE(nsCOMPtr<nsISupports>(do_QueryReferent(mContainer)));

  if (mDefaultCharacterSet.IsEmpty())
  {
    const nsAdoptingString& defCharset =
      nsContentUtils::GetLocalizedStringPref("intl.charset.default");

    if (!defCharset.IsEmpty())
      LossyCopyUTF16toASCII(defCharset, mDefaultCharacterSet);
    else
      mDefaultCharacterSet.AssignLiteral("ISO-8859-1");
  }
  aDefaultCharacterSet = mDefaultCharacterSet;
  return NS_OK;
}

static void
SetChildDefaultCharacterSet(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  const nsACString* charset = static_cast<nsACString*>(aClosure);
  aChild->SetDefaultCharacterSet(*charset);
}

NS_IMETHODIMP
DocumentViewerImpl::SetDefaultCharacterSet(const nsACString& aDefaultCharacterSet)
{
  mDefaultCharacterSet = aDefaultCharacterSet;  
  
  CallChildren(SetChildDefaultCharacterSet, (void*) &aDefaultCharacterSet);
  return NS_OK;
}




NS_IMETHODIMP DocumentViewerImpl::GetForceCharacterSet(nsACString& aForceCharacterSet)
{
  aForceCharacterSet = mForceCharacterSet;
  return NS_OK;
}

static void
SetChildForceCharacterSet(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  const nsACString* charset = static_cast<nsACString*>(aClosure);
  aChild->SetForceCharacterSet(*charset);
}

NS_IMETHODIMP
DocumentViewerImpl::SetForceCharacterSet(const nsACString& aForceCharacterSet)
{
  mForceCharacterSet = aForceCharacterSet;
  
  CallChildren(SetChildForceCharacterSet, (void*) &aForceCharacterSet);
  return NS_OK;
}




NS_IMETHODIMP DocumentViewerImpl::GetHintCharacterSet(nsACString& aHintCharacterSet)
{

  if(kCharsetUninitialized == mHintCharsetSource) {
    aHintCharacterSet.Truncate();
  } else {
    aHintCharacterSet = mHintCharset;
    
    
  }
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetHintCharacterSetSource(PRInt32 *aHintCharacterSetSource)
{
  NS_ENSURE_ARG_POINTER(aHintCharacterSetSource);

  *aHintCharacterSetSource = mHintCharsetSource;
  return NS_OK;
}


NS_IMETHODIMP DocumentViewerImpl::GetPrevDocCharacterSet(nsACString& aPrevDocCharacterSet)
{
  aPrevDocCharacterSet = mPrevDocCharacterSet;

  return NS_OK;
}

static void
SetChildPrevDocCharacterSet(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  const nsACString* charset = static_cast<nsACString*>(aClosure);
  aChild->SetPrevDocCharacterSet(*charset);
}


NS_IMETHODIMP
DocumentViewerImpl::SetPrevDocCharacterSet(const nsACString& aPrevDocCharacterSet)
{
  mPrevDocCharacterSet = aPrevDocCharacterSet;  
  CallChildren(SetChildPrevDocCharacterSet, (void*) &aPrevDocCharacterSet);
  return NS_OK;
}


static void
SetChildHintCharacterSetSource(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  aChild->SetHintCharacterSetSource(NS_PTR_TO_INT32(aClosure));
}

NS_IMETHODIMP
DocumentViewerImpl::SetHintCharacterSetSource(PRInt32 aHintCharacterSetSource)
{
  mHintCharsetSource = aHintCharacterSetSource;
  
  CallChildren(SetChildHintCharacterSetSource,
                      (void*) aHintCharacterSetSource);
  return NS_OK;
}

static void
SetChildHintCharacterSet(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  const nsACString* charset = static_cast<nsACString*>(aClosure);
  aChild->SetHintCharacterSet(*charset);
}

NS_IMETHODIMP
DocumentViewerImpl::SetHintCharacterSet(const nsACString& aHintCharacterSet)
{
  mHintCharset = aHintCharacterSet;
  
  CallChildren(SetChildHintCharacterSet, (void*) &aHintCharacterSet);
  return NS_OK;
}

static void
SetChildBidiOptions(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  aChild->SetBidiOptions(NS_PTR_TO_INT32(aClosure));
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiTextDirection(PRUint8 aTextDirection)
{
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_DIRECTION(bidiOptions, aTextDirection);
  SetBidiOptions(bidiOptions);
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiTextDirection(PRUint8* aTextDirection)
{
  PRUint32 bidiOptions;

  if (aTextDirection) {
    GetBidiOptions(&bidiOptions);
    *aTextDirection = GET_BIDI_OPTION_DIRECTION(bidiOptions);
  }
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiTextType(PRUint8 aTextType)
{
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_TEXTTYPE(bidiOptions, aTextType);
  SetBidiOptions(bidiOptions);
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiTextType(PRUint8* aTextType)
{
  PRUint32 bidiOptions;

  if (aTextType) {
    GetBidiOptions(&bidiOptions);
    *aTextType = GET_BIDI_OPTION_TEXTTYPE(bidiOptions);
  }
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiControlsTextMode(PRUint8 aControlsTextMode)
{
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_CONTROLSTEXTMODE(bidiOptions, aControlsTextMode);
  SetBidiOptions(bidiOptions);
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiControlsTextMode(PRUint8* aControlsTextMode)
{
  PRUint32 bidiOptions;

  if (aControlsTextMode) {
    GetBidiOptions(&bidiOptions);
    *aControlsTextMode = GET_BIDI_OPTION_CONTROLSTEXTMODE(bidiOptions);
  }
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiNumeral(PRUint8 aNumeral)
{
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_NUMERAL(bidiOptions, aNumeral);
  SetBidiOptions(bidiOptions);
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiNumeral(PRUint8* aNumeral)
{
  PRUint32 bidiOptions;

  if (aNumeral) {
    GetBidiOptions(&bidiOptions);
    *aNumeral = GET_BIDI_OPTION_NUMERAL(bidiOptions);
  }
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiSupport(PRUint8 aSupport)
{
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_SUPPORT(bidiOptions, aSupport);
  SetBidiOptions(bidiOptions);
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiSupport(PRUint8* aSupport)
{
  PRUint32 bidiOptions;

  if (aSupport) {
    GetBidiOptions(&bidiOptions);
    *aSupport = GET_BIDI_OPTION_SUPPORT(bidiOptions);
  }
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiCharacterSet(PRUint8 aCharacterSet)
{
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_CHARACTERSET(bidiOptions, aCharacterSet);
  SetBidiOptions(bidiOptions);
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiCharacterSet(PRUint8* aCharacterSet)
{
  PRUint32 bidiOptions;

  if (aCharacterSet) {
    GetBidiOptions(&bidiOptions);
    *aCharacterSet = GET_BIDI_OPTION_CHARACTERSET(bidiOptions);
  }
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiOptions(PRUint32 aBidiOptions)
{
  if (mPresContext) {
    mPresContext->SetBidi(aBidiOptions, PR_TRUE); 
  }
  
  CallChildren(SetChildBidiOptions, (void*) aBidiOptions);
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiOptions(PRUint32* aBidiOptions)
{
  if (aBidiOptions) {
    if (mPresContext) {
      *aBidiOptions = mPresContext->GetBidi();
    }
    else
      *aBidiOptions = IBMBIDI_DEFAULT_BIDI_OPTIONS;
  }
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SizeToContent()
{
   NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);

   nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryReferent(mContainer));
   NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

   nsCOMPtr<nsIDocShellTreeItem> docShellParent;
   docShellAsItem->GetSameTypeParent(getter_AddRefs(docShellParent));

   
   
   NS_ENSURE_TRUE(!docShellParent, NS_ERROR_FAILURE);

   nsCOMPtr<nsIPresShell> presShell;
   GetPresShell(getter_AddRefs(presShell));
   NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

   
   
   mDocument->FlushPendingNotifications(Flush_Layout);

  nsIFrame *root = presShell->GetRootFrame();
  NS_ENSURE_TRUE(root, NS_ERROR_FAILURE);

  nscoord prefWidth;
  {
    nsCOMPtr<nsIRenderingContext> rcx;
    presShell->CreateRenderingContext(root, getter_AddRefs(rcx));
    NS_ENSURE_TRUE(rcx, NS_ERROR_FAILURE);
    prefWidth = root->GetPrefWidth(rcx);
  }

  nsresult rv = presShell->ResizeReflow(prefWidth, NS_UNCONSTRAINEDSIZE);
  NS_ENSURE_SUCCESS(rv, rv);

   nsCOMPtr<nsPresContext> presContext;
   GetPresContext(getter_AddRefs(presContext));
   NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

   PRInt32 width, height;

   
   nsRect shellArea = presContext->GetVisibleArea();
   if (shellArea.width == NS_UNCONSTRAINEDSIZE ||
       shellArea.height == NS_UNCONSTRAINEDSIZE) {
     
     return NS_ERROR_FAILURE;
   }
   width = presContext->AppUnitsToDevPixels(shellArea.width);
   height = presContext->AppUnitsToDevPixels(shellArea.height);

   nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
   docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
   NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

   








   NS_ENSURE_SUCCESS(treeOwner->SizeShellTo(docShellAsItem, width+1, height),
      NS_ERROR_FAILURE);

   return NS_OK;
}


NS_IMPL_ISUPPORTS1(nsDocViewerSelectionListener, nsISelectionListener)

nsresult nsDocViewerSelectionListener::Init(DocumentViewerImpl *aDocViewer)
{
  mDocViewer = aDocViewer;
  return NS_OK;
}










nsresult
DocumentViewerImpl::GetPopupNode(nsIDOMNode** aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);

  nsresult rv;

  
  nsCOMPtr<nsIDocument> document;
  rv = GetDocument(getter_AddRefs(document));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);


  
  nsPIDOMWindow *privateWin = document->GetWindow();
  NS_ENSURE_TRUE(privateWin, NS_ERROR_NOT_AVAILABLE);

  
  nsIFocusController *focusController = privateWin->GetRootFocusController();
  NS_ENSURE_TRUE(focusController, NS_ERROR_FAILURE);

  
  focusController->GetPopupNode(aNode); 

  return rv;
}


nsresult
DocumentViewerImpl::GetPopupLinkNode(nsIDOMNode** aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);

  
  *aNode = nsnull;

  
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupNode(getter_AddRefs(node));
  NS_ENSURE_SUCCESS(rv, rv);

  
  while (node) {

    
    nsCOMPtr<nsIDOMHTMLAnchorElement> anchor(do_QueryInterface(node));
    nsCOMPtr<nsIDOMHTMLAreaElement> area;
    nsCOMPtr<nsIDOMHTMLLinkElement> link;
    nsAutoString xlinkType;
    if (!anchor) {
      
      area = do_QueryInterface(node);
      if (!area) {
        
        link = do_QueryInterface(node);
        if (!link) {
          
          nsCOMPtr<nsIDOMElement> element(do_QueryInterface(node));
          if (element) {
            element->GetAttributeNS(NS_LITERAL_STRING("http://www.w3.org/1999/xlink"),NS_LITERAL_STRING("type"),xlinkType);
          }
        }
      }
    }
    if (anchor || area || link || xlinkType.EqualsLiteral("simple")) {
      *aNode = node;
      NS_IF_ADDREF(*aNode); 
      return NS_OK;
    }
    else {
      
      nsCOMPtr<nsIDOMNode> parentNode;
      node->GetParentNode(getter_AddRefs(parentNode));
      node = parentNode;
    }
  }

  
  return NS_ERROR_FAILURE;
}


nsresult
DocumentViewerImpl::GetPopupImageNode(nsIImageLoadingContent** aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);

  
  *aNode = nsnull;

  
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupNode(getter_AddRefs(node));
  NS_ENSURE_SUCCESS(rv, rv);

  if (node)
    CallQueryInterface(node, aNode);

  return NS_OK;
}












NS_IMETHODIMP DocumentViewerImpl::GetInLink(PRBool* aInLink)
{
#ifdef DEBUG_dr
  printf("dr :: DocumentViewerImpl::GetInLink\n");
#endif

  NS_ENSURE_ARG_POINTER(aInLink);

  
  *aInLink = PR_FALSE;

  
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupLinkNode(getter_AddRefs(node));
  if (NS_FAILED(rv)) return rv;
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  
  *aInLink = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetInImage(PRBool* aInImage)
{
#ifdef DEBUG_dr
  printf("dr :: DocumentViewerImpl::GetInImage\n");
#endif

  NS_ENSURE_ARG_POINTER(aInImage);

  
  *aInImage = PR_FALSE;

  
  nsCOMPtr<nsIImageLoadingContent> node;
  nsresult rv = GetPopupImageNode(getter_AddRefs(node));
  if (NS_FAILED(rv)) return rv;
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  
  *aInImage = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsDocViewerSelectionListener::NotifySelectionChanged(nsIDOMDocument *, nsISelection *, PRInt16)
{
  NS_ASSERTION(mDocViewer, "Should have doc viewer!");

  
  nsCOMPtr<nsISelection> selection;
  nsresult rv = mDocViewer->GetDocumentSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;

  PRBool selectionCollapsed;
  selection->GetIsCollapsed(&selectionCollapsed);
  
  
  
  if (!mGotSelectionState || mSelectionWasCollapsed != selectionCollapsed)
  {
    nsCOMPtr<nsIDocument> theDoc;
    mDocViewer->GetDocument(getter_AddRefs(theDoc));
    if (!theDoc) return NS_ERROR_FAILURE;

    nsPIDOMWindow *domWindow = theDoc->GetWindow();
    if (!domWindow) return NS_ERROR_FAILURE;

    domWindow->UpdateCommands(NS_LITERAL_STRING("select"));
    mGotSelectionState = PR_TRUE;
    mSelectionWasCollapsed = selectionCollapsed;
  }

  return NS_OK;
}


NS_IMPL_ISUPPORTS2(nsDocViewerFocusListener,
                   nsIDOMFocusListener,
                   nsIDOMEventListener)

nsDocViewerFocusListener::nsDocViewerFocusListener()
:mDocViewer(nsnull)
{
}

nsDocViewerFocusListener::~nsDocViewerFocusListener(){}

nsresult
nsDocViewerFocusListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDocViewerFocusListener::Focus(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIPresShell> shell;
  if(!mDocViewer)
    return NS_ERROR_FAILURE;

  nsresult result = mDocViewer->GetPresShell(getter_AddRefs(shell));
  if(NS_FAILED(result) || !shell)
    return result?result:NS_ERROR_FAILURE;
  nsCOMPtr<nsISelectionController> selCon;
  selCon = do_QueryInterface(shell);
  PRInt16 selectionStatus;
  selCon->GetDisplaySelection(&selectionStatus);

  
  if(selectionStatus == nsISelectionController::SELECTION_DISABLED ||
     selectionStatus == nsISelectionController::SELECTION_HIDDEN)
  {
    selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
    selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
  }
  return result;
}

NS_IMETHODIMP
nsDocViewerFocusListener::Blur(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIPresShell> shell;
  if(!mDocViewer)
    return NS_ERROR_FAILURE;

  nsresult result = mDocViewer->GetPresShell(getter_AddRefs(shell));
  if(NS_FAILED(result) || !shell)
    return result?result:NS_ERROR_FAILURE;
  nsCOMPtr<nsISelectionController> selCon;
  selCon = do_QueryInterface(shell);
  PRInt16 selectionStatus;
  selCon->GetDisplaySelection(&selectionStatus);

  
  if(selectionStatus == nsISelectionController::SELECTION_ON ||
     selectionStatus == nsISelectionController::SELECTION_ATTENTION)
  {
    selCon->SetDisplaySelection(nsISelectionController::SELECTION_DISABLED);
    selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
  }
  return result;
}


nsresult
nsDocViewerFocusListener::Init(DocumentViewerImpl *aDocViewer)
{
  mDocViewer = aDocViewer;
  return NS_OK;
}





#ifdef NS_PRINTING

NS_IMETHODIMP
DocumentViewerImpl::Print(nsIPrintSettings*       aPrintSettings,
                          nsIWebProgressListener* aWebProgressListener)
{

#ifdef MOZ_XUL
  
  nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocument));
  if (xulDoc) {
    nsPrintEngine::ShowPrintErrorDialog(NS_ERROR_GFX_PRINTER_NO_XUL);
    return NS_ERROR_FAILURE;
  }
#endif

  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mContainer));
  NS_ASSERTION(docShell, "This has to be a docshell");

  
  
  
  PRUint32 busyFlags = nsIDocShell::BUSY_FLAGS_NONE;
  if ((NS_FAILED(docShell->GetBusyFlags(&busyFlags)) ||
       (busyFlags != nsIDocShell::BUSY_FLAGS_NONE && busyFlags & nsIDocShell::BUSY_FLAGS_PAGE_LOADING)) && 
      !mPrintDocIsFullyLoaded) {
    if (!mPrintIsPending) {
      mCachedPrintSettings           = aPrintSettings;
      mCachedPrintWebProgressListner = aWebProgressListener;
      mPrintIsPending                = PR_TRUE;
    }
    PR_PL(("Printing Stopped - document is still busy!"));
    return NS_ERROR_GFX_PRINTER_DOC_IS_BUSY;
  }

  nsCOMPtr<nsIPresShell> presShell;
  docShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell || !mDocument || !mDeviceContext || !mParentWidget) {
    PR_PL(("Can't Print without pres shell, document etc"));
    return NS_ERROR_FAILURE;
  }

  nsresult rv;

  
  
  
  
  if (GetIsPrinting()) {
    
    rv = NS_ERROR_NOT_AVAILABLE;
    nsPrintEngine::ShowPrintErrorDialog(rv);
    return rv;
  }

  
  
  nsCOMPtr<nsIPluginDocument> pDoc(do_QueryInterface(mDocument));
  if (pDoc)
    return pDoc->Print();

  if (!mPrintEngine) {
    mPrintEngine = new nsPrintEngine();
    NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_OUT_OF_MEMORY);

    rv = mPrintEngine->Initialize(this, docShell, mDocument, 
                                  mDeviceContext, mParentWidget,
#ifdef NS_DEBUG
                                  mDebugFile
#else
                                  nsnull
#endif
                                  );
    if (NS_FAILED(rv)) {
      mPrintEngine->Destroy();
      mPrintEngine = nsnull;
      return rv;
    }
  }

  rv = mPrintEngine->Print(aPrintSettings, aWebProgressListener);
  if (NS_FAILED(rv)) {
    OnDonePrinting();
  }
  return rv;
}








NS_IMETHODIMP
DocumentViewerImpl::PrintPreview(nsIPrintSettings* aPrintSettings, 
                                 nsIDOMWindow *aChildDOMWin, 
                                 nsIWebProgressListener* aWebProgressListener)
{
#if defined(NS_PRINTING) && defined(NS_PRINT_PREVIEW)
  nsresult rv = NS_OK;

  if (GetIsPrinting()) {
    nsPrintEngine::CloseProgressDialog(aWebProgressListener);
    return NS_ERROR_FAILURE;
  }

#ifdef MOZ_XUL
  
  nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocument));
  if (xulDoc) {
    nsPrintEngine::CloseProgressDialog(aWebProgressListener);
    nsPrintEngine::ShowPrintErrorDialog(NS_ERROR_GFX_PRINTER_NO_XUL, PR_FALSE);
    return NS_ERROR_FAILURE;
  }
#endif

  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mContainer));
  NS_ASSERTION(docShell, "This has to be a docshell");
  nsCOMPtr<nsIPresShell> presShell;
  docShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell || !mDocument || !mDeviceContext || !mParentWidget) {
    PR_PL(("Can't Print Preview without pres shell, document etc"));
    return NS_ERROR_FAILURE;
  }

  if (!mPrintEngine) {
    mPrintEngine = new nsPrintEngine();
    NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_OUT_OF_MEMORY);

    rv = mPrintEngine->Initialize(this, docShell, mDocument,
                                  mDeviceContext, mParentWidget,
#ifdef NS_DEBUG
                                  mDebugFile
#else
                                  nsnull
#endif
                                  );
    if (NS_FAILED(rv)) {
      mPrintEngine->Destroy();
      mPrintEngine = nsnull;
      return rv;
    }
  }

  rv = mPrintEngine->PrintPreview(aPrintSettings, aChildDOMWin, aWebProgressListener);
  if (NS_FAILED(rv)) {
    OnDonePrinting();
  }
  return rv;
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP
DocumentViewerImpl::PrintPreviewNavigate(PRInt16 aType, PRInt32 aPageNum)
{
  if (!GetIsPrintPreview() ||
      mPrintEngine->GetIsCreatingPrintPreview())
    return NS_ERROR_FAILURE;

  nsIScrollableView* scrollableView = nsnull;
  mPrintEngine->GetPrintPreviewViewManager()->GetRootScrollableView(&scrollableView);
  if (scrollableView == nsnull)
    return NS_OK;

  
  if (aType == nsIWebBrowserPrint::PRINTPREVIEW_HOME ||
      (aType == nsIWebBrowserPrint::PRINTPREVIEW_GOTO_PAGENUM && aPageNum == 1)) {
    scrollableView->ScrollTo(0, 0, PR_TRUE);
    return NS_OK;
  }

  
  
  nsIFrame* seqFrame  = nsnull;
  PRInt32   pageCount = 0;
  if (NS_FAILED(mPrintEngine->GetSeqFrameAndCountPages(seqFrame, pageCount))) {
    return NS_ERROR_FAILURE;
  }

  
  nscoord x;
  nscoord y;
  scrollableView->GetScrollPosition(x, y);

  PRInt32    pageNum = 1;
  nsIFrame * fndPageFrame  = nsnull;
  nsIFrame * currentPage   = nsnull;

  
  if (aType == nsIWebBrowserPrint::PRINTPREVIEW_END) {
    aType    = nsIWebBrowserPrint::PRINTPREVIEW_GOTO_PAGENUM;
    aPageNum = pageCount;
  }

  
  
  nscoord gap = 0;
  nsIFrame* pageFrame = seqFrame->GetFirstChild(nsnull);
  while (pageFrame != nsnull) {
    nsRect pageRect = pageFrame->GetRect();
    if (pageNum == 1) {
      gap = pageRect.y;
    }
    if (pageRect.Contains(pageRect.x, y)) {
      currentPage = pageFrame;
    }
    if (pageNum == aPageNum) {
      fndPageFrame = pageFrame;
      break;
    }
    pageNum++;
    pageFrame = pageFrame->GetNextSibling();
  }

  if (aType == nsIWebBrowserPrint::PRINTPREVIEW_PREV_PAGE) {
    if (currentPage) {
      fndPageFrame = currentPage->GetPrevInFlow();
      if (!fndPageFrame) {
        return NS_OK;
      }
    } else {
      return NS_OK;
    }
  } else if (aType == nsIWebBrowserPrint::PRINTPREVIEW_NEXT_PAGE) {
    if (currentPage) {
      fndPageFrame = currentPage->GetNextInFlow();
      if (!fndPageFrame) {
        return NS_OK;
      }
    } else {
      return NS_OK;
    }
  } else { 
    if (aPageNum < 0 || aPageNum > pageCount) {
      return NS_OK;
    }
  }

  if (fndPageFrame && scrollableView) {
    nscoord deadSpaceGap = 0;
    nsIPageSequenceFrame * sqf;
    if (NS_SUCCEEDED(CallQueryInterface(seqFrame, &sqf))) {
      sqf->GetDeadSpaceValue(&deadSpaceGap);
    }

    
    scrollableView->ScrollTo(0, fndPageFrame->GetPosition().y-deadSpaceGap, PR_TRUE);
  }
  return NS_OK;

}


NS_IMETHODIMP
DocumentViewerImpl::GetGlobalPrintSettings(nsIPrintSettings * *aGlobalPrintSettings)
{
  return nsPrintEngine::GetGlobalPrintSettings(aGlobalPrintSettings);
}



NS_IMETHODIMP
DocumentViewerImpl::GetDoingPrint(PRBool *aDoingPrint)
{
  NS_ENSURE_ARG_POINTER(aDoingPrint);
  
  *aDoingPrint = PR_FALSE;
  if (mPrintEngine) {
    
    return mPrintEngine->GetDoingPrintPreview(aDoingPrint);
  } 
  return NS_OK;
}



NS_IMETHODIMP
DocumentViewerImpl::GetDoingPrintPreview(PRBool *aDoingPrintPreview)
{
  NS_ENSURE_ARG_POINTER(aDoingPrintPreview);

  *aDoingPrintPreview = PR_FALSE;
  if (mPrintEngine) {
    return mPrintEngine->GetDoingPrintPreview(aDoingPrintPreview);
  }
  return NS_OK;
}


NS_IMETHODIMP
DocumentViewerImpl::GetCurrentPrintSettings(nsIPrintSettings * *aCurrentPrintSettings)
{
  NS_ENSURE_ARG_POINTER(aCurrentPrintSettings);

  *aCurrentPrintSettings = nsnull;
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  return mPrintEngine->GetCurrentPrintSettings(aCurrentPrintSettings);
}



NS_IMETHODIMP 
DocumentViewerImpl::GetCurrentChildDOMWindow(nsIDOMWindow * *aCurrentChildDOMWindow)
{
  NS_ENSURE_ARG_POINTER(aCurrentChildDOMWindow);
  *aCurrentChildDOMWindow = nsnull;
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
DocumentViewerImpl::Cancel()
{
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);
  return mPrintEngine->Cancelled();
}


NS_IMETHODIMP
DocumentViewerImpl::ExitPrintPreview()
{
  if (GetIsPrinting()) return NS_ERROR_FAILURE;
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  if (GetIsPrintPreview()) {
    ReturnToGalleyPresentation();
  }
  return NS_OK;
}



NS_IMETHODIMP
DocumentViewerImpl::EnumerateDocumentNames(PRUint32* aCount,
                                           PRUnichar*** aResult)
{
#ifdef NS_PRINTING
  NS_ENSURE_ARG(aCount);
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  return mPrintEngine->EnumerateDocumentNames(aCount, aResult);
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP 
DocumentViewerImpl::GetIsFramesetFrameSelected(PRBool *aIsFramesetFrameSelected)
{
#ifdef NS_PRINTING
  *aIsFramesetFrameSelected = PR_FALSE;
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  return mPrintEngine->GetIsFramesetFrameSelected(aIsFramesetFrameSelected);
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP
DocumentViewerImpl::GetPrintPreviewNumPages(PRInt32 *aPrintPreviewNumPages)
{
#ifdef NS_PRINTING
  NS_ENSURE_ARG_POINTER(aPrintPreviewNumPages);
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  return mPrintEngine->GetPrintPreviewNumPages(aPrintPreviewNumPages);
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP
DocumentViewerImpl::GetIsFramesetDocument(PRBool *aIsFramesetDocument)
{
#ifdef NS_PRINTING
  *aIsFramesetDocument = PR_FALSE;
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  return mPrintEngine->GetIsFramesetDocument(aIsFramesetDocument);
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP 
DocumentViewerImpl::GetIsIFrameSelected(PRBool *aIsIFrameSelected)
{
#ifdef NS_PRINTING
  *aIsIFrameSelected = PR_FALSE;
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  return mPrintEngine->GetIsIFrameSelected(aIsIFrameSelected);
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP 
DocumentViewerImpl::GetIsRangeSelection(PRBool *aIsRangeSelection)
{
#ifdef NS_PRINTING
  *aIsRangeSelection = PR_FALSE;
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  return mPrintEngine->GetIsRangeSelection(aIsRangeSelection);
#else
  return NS_ERROR_FAILURE;
#endif
}







void 
DocumentViewerImpl::SetIsPrintingInDocShellTree(nsIDocShellTreeNode* aParentNode, 
                                                PRBool               aIsPrintingOrPP, 
                                                PRBool               aStartAtTop)
{
  NS_ASSERTION(aParentNode, "Parent can't be NULL!");

  nsCOMPtr<nsIDocShellTreeItem> parentItem(do_QueryInterface(aParentNode));

  
  if (aStartAtTop) {
    while (parentItem) {
      nsCOMPtr<nsIDocShellTreeItem> parent;
      parentItem->GetSameTypeParent(getter_AddRefs(parent));
      if (!parent) {
        break;
      }
      parentItem = do_QueryInterface(parent);
    }
  }
  NS_ASSERTION(parentItem, "parentItem can't be null");

  
  nsCOMPtr<nsIContentViewerContainer> viewerContainer(do_QueryInterface(parentItem));
  if (viewerContainer) {
    viewerContainer->SetIsPrinting(aIsPrintingOrPP);
  }

  
  PRInt32 n;
  aParentNode->GetChildCount(&n);
  for (PRInt32 i=0; i < n; i++) {
    nsCOMPtr<nsIDocShellTreeItem> child;
    aParentNode->GetChildAt(i, getter_AddRefs(child));
    nsCOMPtr<nsIDocShellTreeNode> childAsNode(do_QueryInterface(child));
    NS_ASSERTION(childAsNode, "child isn't nsIDocShellTreeNode");
    if (childAsNode) {
      SetIsPrintingInDocShellTree(childAsNode, aIsPrintingOrPP, PR_FALSE);
    }
  }

}
#endif 



PRBool
DocumentViewerImpl::GetIsPrinting()
{
#ifdef NS_PRINTING
  if (mPrintEngine) {
    return mPrintEngine->GetIsPrinting();
  }
#endif
  return PR_FALSE; 
}



void
DocumentViewerImpl::SetIsPrinting(PRBool aIsPrinting)
{
#ifdef NS_PRINTING
  
  
  if (mContainer) {
    nsCOMPtr<nsIDocShellTreeNode> docShellTreeNode(do_QueryReferent(mContainer));
    NS_ASSERTION(docShellTreeNode, "mContainer has to be a nsIDocShellTreeNode");
    SetIsPrintingInDocShellTree(docShellTreeNode, aIsPrinting, PR_TRUE);
  }
#endif
}





PRBool
DocumentViewerImpl::GetIsPrintPreview()
{
#ifdef NS_PRINTING
  if (mPrintEngine) {
    return mPrintEngine->GetIsPrintPreview();
  }
#endif
  return PR_FALSE; 
}



void
DocumentViewerImpl::SetIsPrintPreview(PRBool aIsPrintPreview)
{
#ifdef NS_PRINTING
  
  
  if (mContainer) {
    nsCOMPtr<nsIDocShellTreeNode> docShellTreeNode(do_QueryReferent(mContainer));
    NS_ASSERTION(docShellTreeNode, "mContainer has to be a nsIDocShellTreeNode");
    SetIsPrintingInDocShellTree(docShellTreeNode, aIsPrintPreview, PR_TRUE);
  }
#endif
}






void
DocumentViewerImpl::IncrementDestroyRefCount()
{
  ++mDestroyRefCount;
}



static void ResetFocusState(nsIDocShell* aDocShell);

void
DocumentViewerImpl::ReturnToGalleyPresentation()
{
#if defined(NS_PRINTING) && defined(NS_PRINT_PREVIEW)
  if (!GetIsPrintPreview()) {
    NS_ERROR("Wow, we should never get here!");
    return;
  }

  SetIsPrintPreview(PR_FALSE);

  mPrintEngine->TurnScriptingOn(PR_TRUE);
  mPrintEngine->Destroy();
  mPrintEngine = nsnull;

  mViewManager->EnableRefresh(NS_VMREFRESH_DEFERRED);

  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mContainer));
  ResetFocusState(docShell);

  Show();

#endif 
}



static void
ResetFocusState(nsIDocShell* aDocShell)
{
  nsCOMPtr<nsISimpleEnumerator> docShellEnumerator;
  aDocShell->GetDocShellEnumerator(nsIDocShellTreeItem::typeContent,
                                   nsIDocShell::ENUMERATE_FORWARDS,
                                   getter_AddRefs(docShellEnumerator));
  
  nsCOMPtr<nsIDocShell> currentDocShell;
  nsCOMPtr<nsISupports> currentContainer;
  PRBool hasMoreDocShells;
  while (NS_SUCCEEDED(docShellEnumerator->HasMoreElements(&hasMoreDocShells))
         && hasMoreDocShells) {
    docShellEnumerator->GetNext(getter_AddRefs(currentContainer));
    currentDocShell = do_QueryInterface(currentContainer);
    if (!currentDocShell) {
      break;
    }
    nsCOMPtr<nsPresContext> presContext;
    currentDocShell->GetPresContext(getter_AddRefs(presContext));
    nsIEventStateManager* esm =
      presContext ? presContext->EventStateManager() : nsnull;
    if (esm) {
       esm->SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
       esm->SetFocusedContent(nsnull);
    }
  }
}













void
DocumentViewerImpl::OnDonePrinting() 
{
#if defined(NS_PRINTING) && defined(NS_PRINT_PREVIEW)
  if (mPrintEngine) {
    if (GetIsPrintPreview()) {
      mPrintEngine->DestroyPrintingData();
    } else {
      mPrintEngine->Destroy();
      mPrintEngine = nsnull;
    }

    
    if (mDeferredWindowClose) {
      mDeferredWindowClose = PR_FALSE;
      nsCOMPtr<nsISupports> container = do_QueryReferent(mContainer);
      nsCOMPtr<nsIDOMWindowInternal> win = do_GetInterface(container);
      if (win)
        win->Close();
    } else if (mClosingWhilePrinting) {
      if (mDocument) {
        mDocument->SetScriptGlobalObject(nsnull);
        mDocument->Destroy();
        mDocument = nsnull;
      }
      mClosingWhilePrinting = PR_FALSE;
      NS_RELEASE_THIS();
    }
  }
#endif 
}

NS_IMETHODIMP DocumentViewerImpl::SetPageMode(PRBool aPageMode, nsIPrintSettings* aPrintSettings)
{
  
  
  mIsPageMode = aPageMode;
  
  nsRect bounds;
  mWindow->GetBounds(bounds);

  if (mPresShell) {
    
    mPresShell->EndObservingDocument();
    nsCOMPtr<nsISelection> selection;
    nsresult rv = GetDocumentSelection(getter_AddRefs(selection));
    nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(selection));
    if (NS_SUCCEEDED(rv) && selPrivate && mSelectionListener)
      selPrivate->RemoveSelectionListener(mSelectionListener);
    mPresShell->Destroy();
  }

  if (mPresContext) {
    mPresContext->SetContainer(nsnull);
    mPresContext->SetLinkHandler(nsnull);
  }

  mPresShell    = nsnull;
  mPresContext  = nsnull;
  mViewManager  = nsnull;
  mWindow       = nsnull;

  NS_ENSURE_STATE(mDocument);
  if (aPageMode)
  {    
    mPresContext =
      new nsPresContext(mDocument, nsPresContext::eContext_PageLayout);
    NS_ENSURE_TRUE(mPresContext, NS_ERROR_OUT_OF_MEMORY);
    mPresContext->SetPaginatedScrolling(PR_TRUE);
    mPresContext->SetPrintSettings(aPrintSettings);
    nsresult rv = mPresContext->Init(mDeviceContext);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  InitInternal(mParentWidget, nsnull, mDeviceContext, bounds, PR_TRUE, PR_FALSE, PR_FALSE);
  mViewManager->EnableRefresh(NS_VMREFRESH_NO_SYNC);

  Show();
  return NS_OK;
}

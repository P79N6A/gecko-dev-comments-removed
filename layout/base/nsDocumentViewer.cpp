










































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
#include "nsIDOMDocumentEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMBeforeUnloadEvent.h"
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
#include "nsFocusManager.h"

#include "nsIScrollableView.h"
#include "nsIHTMLDocument.h"
#include "nsITimelineService.h"
#include "nsGfxCIID.h"
#include "nsStyleSheetService.h"
#include "nsURILoader.h"

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
#include "nsPresShellIterator.h"


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
  






  nsresult MakeWindow(const nsSize& aSize, nsIView* aContainerView);

  





  nsIView* FindContainerView();

  


  nsresult CreateDeviceContext(nsIView* aContainerView);

  



  nsresult InitInternal(nsIWidget* aParentWidget,
                        nsISupports *aState,
                        const nsIntRect& aBounds,
                        PRBool aDoCreation,
                        PRBool aInPrintPreview,
                        PRBool aNeedMakeCX = PR_TRUE);
  






  nsresult InitPresentationStuff(PRBool aDoInitialReflow, PRBool aReenableRefresh);

  nsresult GetPopupNode(nsIDOMNode** aNode);
  nsresult GetPopupLinkNode(nsIDOMNode** aNode);
  nsresult GetPopupImageNode(nsIImageLoadingContent** aNode);

  void PrepareToStartLoad(void);

  nsresult SyncParentSubDocMap();

  nsresult GetDocumentSelection(nsISelection **aSelection);

  nsresult GetClipboardEventTarget(nsIDOMNode **aEventTarget);
  nsresult FireClipboardEvent(PRUint32 msg, PRBool* aPreventDefault);

  void DestroyPresShell();

#ifdef NS_PRINTING
  
  
  void SetIsPrintingInDocShellTree(nsIDocShellTreeNode* aParentNode, 
                                   PRBool               aIsPrintingOrPP, 
                                   PRBool               aStartAtTop);
#endif 

protected:
  
  nsIPresShell* GetPresShell();
  nsPresContext* GetPresContext();
  nsIViewManager* GetViewManager();

  
  
  
  
  

  nsWeakPtr mContainer; 
  nsCOMPtr<nsIDeviceContext> mDeviceContext;  

  
  
  nsCOMPtr<nsIDocument>    mDocument;
  nsCOMPtr<nsIWidget>      mWindow;      
  nsCOMPtr<nsIViewManager> mViewManager;
  nsCOMPtr<nsPresContext> mPresContext;
  nsCOMPtr<nsIPresShell>   mPresShell;

  nsCOMPtr<nsISelectionListener> mSelectionListener;
  nsCOMPtr<nsIDOMFocusListener> mFocusListener;

  nsCOMPtr<nsIContentViewer> mPreviousViewer;
  nsCOMPtr<nsISHEntry> mSHEntry;

  nsIWidget* mParentWidget; 

  nsIntRect mBounds;

  
  
  float mTextZoom;      
  float mPageZoom;

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
  unsigned                         mPrintPreviewZoomed : 1;

  
  unsigned                         mPrintIsPending : 1;
  unsigned                         mPrintDocIsFullyLoaded : 1;
  nsCOMPtr<nsIPrintSettings>       mCachedPrintSettings;
  nsCOMPtr<nsIWebProgressListener> mCachedPrintWebProgressListner;

  nsCOMPtr<nsPrintEngine>          mPrintEngine;
  float                            mOriginalPrintPreviewScale;
  float                            mPrintPreviewZoom;
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
  PRPackedBool mCallerIsClosingWindow;

};





static NS_DEFINE_CID(kViewManagerCID,       NS_VIEW_MANAGER_CID);
static NS_DEFINE_CID(kWidgetCID,            NS_CHILD_CID);
static NS_DEFINE_CID(kDeviceContextCID,     NS_DEVICE_CONTEXT_CID);


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
  mCallerIsClosingWindow = PR_FALSE;

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
  : mTextZoom(1.0), mPageZoom(1.0),
    mIsSticky(PR_TRUE),
#ifdef NS_PRINT_PREVIEW
    mPrintPreviewZoom(1.0),
#endif
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
        if (mDocument && parent_doc->GetSubDocumentFor(content) != mDocument) {
          mDocument->SuppressEventHandling(parent_doc->EventHandlingSuppressed());
        }
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
                         const nsIntRect& aBounds)
{
  return InitInternal(aParentWidget, nsnull, aBounds, PR_TRUE, PR_FALSE);
}

nsresult
DocumentViewerImpl::InitPresentationStuff(PRBool aDoInitialReflow, PRBool aReenableRefresh)
{
  NS_ASSERTION(!mPresShell,
               "Someone should have destroyed the presshell!");

  
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

  
  nscoord width = mPresContext->DeviceContext()->UnscaledAppUnitsPerDevPixel() * mBounds.width;
  nscoord height = mPresContext->DeviceContext()->UnscaledAppUnitsPerDevPixel() * mBounds.height;

  mViewManager->DisableRefresh();
  mViewManager->SetWindowDimensions(width, height);
  mPresContext->SetTextZoom(mTextZoom);
  mPresContext->SetFullZoom(mPageZoom);

  if (aDoInitialReflow) {
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(mDocument);
    if (htmlDoc) {
      nsCOMPtr<nsIDOMHTMLFrameSetElement> frameset =
        do_QueryInterface(mDocument->GetRootContent());
      htmlDoc->SetIsFrameset(frameset != nsnull);
    }

    nsCOMPtr<nsIPresShell> shellGrip = mPresShell;
    
    mPresShell->InitialReflow(width, height);
  } else {
    
    
    mPresContext->SetVisibleArea(nsRect(0, 0, width, height));
  }

  
  if (aReenableRefresh && mEnableRendering && mViewManager) {
    mViewManager->EnableRefresh(NS_VMREFRESH_IMMEDIATE);
  }

  
  
  if (!mSelectionListener) {
    nsDocViewerSelectionListener *selectionListener =
      new nsDocViewerSelectionListener();
    NS_ENSURE_TRUE(selectionListener, NS_ERROR_OUT_OF_MEMORY);

    selectionListener->Init(this);

    
    mSelectionListener = selectionListener;
  }

  nsCOMPtr<nsISelection> selection;
  rv = GetDocumentSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(selection));
  rv = selPrivate->AddSelectionListener(mSelectionListener);
  if (NS_FAILED(rv))
    return rv;

  
  nsCOMPtr<nsIDOMFocusListener> oldFocusListener = mFocusListener;

  
  
  
  
  nsDocViewerFocusListener *focusListener;
  NS_NEWXPCOM(focusListener, nsDocViewerFocusListener);
  NS_ENSURE_TRUE(focusListener, NS_ERROR_OUT_OF_MEMORY);

  focusListener->Init(this);

  
  mFocusListener = focusListener;

  if (mDocument) {
    rv = mDocument->AddEventListenerByIID(mFocusListener,
                                          NS_GET_IID(nsIDOMFocusListener));
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to register focus listener");
    if (oldFocusListener) {
      rv = mDocument->RemoveEventListenerByIID(oldFocusListener,
                                               NS_GET_IID(nsIDOMFocusListener));
      NS_ASSERTION(NS_SUCCEEDED(rv), "failed to remove focus listener");
    }
  }

  return NS_OK;
}

static nsPresContext*
CreatePresContext(nsIDocument* aDocument,
                  nsPresContext::nsPresContextType aType,
                  nsIView* aContainerView)
{
  if (aContainerView)
    return new nsPresContext(aDocument, aType);
  return new nsRootPresContext(aDocument, aType);
}





nsresult
DocumentViewerImpl::InitInternal(nsIWidget* aParentWidget,
                                 nsISupports *aState,
                                 const nsIntRect& aBounds,
                                 PRBool aDoCreation,
                                 PRBool aInPrintPreview,
                                 PRBool aNeedMakeCX )
{
  
  
  
  nsAutoScriptBlocker blockScripts;

  mParentWidget = aParentWidget; 
  mBounds = aBounds;

  nsresult rv = NS_OK;
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NULL_POINTER);

  nsIView* containerView = FindContainerView();

  PRBool makeCX = PR_FALSE;
  if (aDoCreation) {
    nsresult rv = CreateDeviceContext(containerView);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    if (!mPresContext &&
        (aParentWidget || containerView || mDocument->GetDisplayDocument())) {
      
      if (mIsPageMode) {
        
      } else {
        mPresContext = CreatePresContext(mDocument,
            nsPresContext::eContext_Galley, containerView);
      }
      NS_ENSURE_TRUE(mPresContext, NS_ERROR_OUT_OF_MEMORY);

      nsresult rv = mPresContext->Init(mDeviceContext); 
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
      

      
      
      
      

      rv = MakeWindow(nsSize(mPresContext->DevPixelsToAppUnits(aBounds.width),
                             mPresContext->DevPixelsToAppUnits(aBounds.height)),
                      containerView);
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
          nsSize(mPresContext->TwipsToAppUnits(NSToIntFloor(pageWidth)),
                 mPresContext->TwipsToAppUnits(NSToIntFloor(pageHeight))));
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
    
    

    rv = InitPresentationStuff(!makeCX, !makeCX);
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

  mLoaded = PR_TRUE;

  
  PRBool restoring = PR_FALSE;
  
  
  
  
  if(window &&
     (NS_SUCCEEDED(aStatus) || aStatus == NS_ERROR_PARSED_DATA_CACHED)) {
    if (mDocument)
      mDocument->SetReadyStateInternal(nsIDocument::READYSTATE_COMPLETE);
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

  
  
  
  if (mDocument) {
    
    window = mDocument->GetWindow();
    if (window) {
      nsIDocShell *docShell = window->GetDocShell();
      PRBool beingDestroyed;
      if (docShell &&
          NS_SUCCEEDED(docShell->IsBeingDestroyed(&beingDestroyed)) &&
          !beingDestroyed) {
        mDocument->OnPageShow(restoring, nsnull);
      }
    }
  }

  
  
  if (mPresShell && !mStopped) {
    nsCOMPtr<nsIPresShell> shellDeathGrip(mPresShell);
    mPresShell->UnsuppressPainting();
    
    if (mPresShell) {
      mPresShell->ScrollToAnchor();
    }
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
DocumentViewerImpl::PermitUnload(PRBool aCallerClosesWindow, PRBool *aPermitUnload)
{
  *aPermitUnload = PR_TRUE;

  if (!mDocument || mInPermitUnload || mCallerIsClosingWindow) {
    return NS_OK;
  }

  
  nsPIDOMWindow *window = mDocument->GetWindow();

  if (!window) {
    
    NS_WARNING("window not set for document!");
    return NS_OK;
  }

  NS_ASSERTION(nsContentUtils::IsSafeToRunScript(), "This is unsafe");

  
  
  nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(mDocument);
  nsCOMPtr<nsIDOMEvent> event;
  docEvent->CreateEvent(NS_LITERAL_STRING("beforeunloadevent"),
                        getter_AddRefs(event));
  nsCOMPtr<nsIDOMBeforeUnloadEvent> beforeUnload = do_QueryInterface(event);
  nsCOMPtr<nsIPrivateDOMEvent> pEvent = do_QueryInterface(beforeUnload);
  NS_ENSURE_STATE(pEvent);
  nsresult rv = event->InitEvent(NS_LITERAL_STRING("beforeunload"),
                                 PR_FALSE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mDocument);
  pEvent->SetTarget(target);
  pEvent->SetTrusted(PR_TRUE);

  
  
  nsRefPtr<DocumentViewerImpl> kungFuDeathGrip(this);

  {
    
    
    nsAutoPopupStatePusher popupStatePusher(openAbused, PR_TRUE);

    mInPermitUnload = PR_TRUE;
    nsEventDispatcher::DispatchDOMEvent(window, nsnull, event, mPresContext,
                                        nsnull);
    mInPermitUnload = PR_FALSE;
  }

  nsCOMPtr<nsIDocShellTreeNode> docShellNode(do_QueryReferent(mContainer));
  nsAutoString text;
  beforeUnload->GetReturnValue(text);
  if (pEvent->GetInternalNSEvent()->flags & NS_EVENT_FLAG_NO_DEFAULT ||
      !text.IsEmpty()) {
    

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

      
      
      PRInt32 len = NS_MIN(text.Length(), 1024U);

      nsAutoString msg;
      if (len == 0) {
        msg = preMsg + NS_LITERAL_STRING("\n\n") + postMsg;
      } else {
        msg = preMsg + NS_LITERAL_STRING("\n\n") +
              StringHead(text, len) +
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
          cv->PermitUnload(aCallerClosesWindow, aPermitUnload);
        }
      }
    }
  }

  if (aCallerClosesWindow && *aPermitUnload)
    mCallerIsClosingWindow = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::ResetCloseWindow()
{
  mCallerIsClosingWindow = PR_FALSE;

  nsCOMPtr<nsIDocShellTreeNode> docShellNode(do_QueryReferent(mContainer));
  if (docShellNode) {
    PRInt32 childCount;
    docShellNode->GetChildCount(&childCount);

    for (PRInt32 i = 0; i < childCount; ++i) {
      nsCOMPtr<nsIDocShellTreeItem> item;
      docShellNode->GetChildAt(i, getter_AddRefs(item));

      nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(item));

      if (docShell) {
        nsCOMPtr<nsIContentViewer> cv;
        docShell->GetContentViewer(getter_AddRefs(cv));

        if (cv) {
          cv->ResetCloseWindow();
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

  mDocument->OnPageHide(!aIsUnload, nsnull);

  
  NS_ENSURE_STATE(mDocument);
  nsPIDOMWindow *window = mDocument->GetWindow();
  if (window)
    window->PageHidden();

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
  
  
  nsContentUtils::HidePopupsInDocument(mDocument);
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

  if (mDocument)
    mDocument->SetContainer(nsCOMPtr<nsISupports>(do_QueryReferent(mContainer)));

  nsresult rv = InitInternal(mParentWidget, aState, mBounds, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mPresShell)
    mPresShell->SetForwardingContainer(nsnull);

  
  

  if (aSHEntry) {
    nsCOMPtr<nsIDocShellTreeItem> item;
    PRInt32 itemIndex = 0;
    while (NS_SUCCEEDED(aSHEntry->ChildShellAt(itemIndex++,
                                               getter_AddRefs(item))) && item) {
      AttachContainerRecurse(nsCOMPtr<nsIDocShell>(do_QueryInterface(item)));
    }
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
  } else
#endif
    {
      
      mDocument->SetScriptGlobalObject(nsnull);

      if (!mSHEntry && mDocument)
        mDocument->RemovedFromDocShell();
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
#ifdef NS_PRINT_PREVIEW
    PRBool doingPrintPreview;
    mPrintEngine->GetDoingPrintPreview(&doingPrintPreview);
    if (doingPrintPreview) {
      mPrintEngine->FinishPrintPreview();
    }
#endif

    mPrintEngine->Destroy();
    mPrintEngine = nsnull;
  }
#endif

  
  if (mPreviousViewer) {
    mPreviousViewer->Destroy();
    mPreviousViewer = nsnull;
  }

  mDeviceContext = nsnull;

  if (mPresShell) {
    DestroyPresShell();
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
    nsSize currentSize(0, 0);

    if (mViewManager) {
      mViewManager->GetWindowDimensions(&currentSize.width, &currentSize.height);
    }

    if (mPresContext) {
      
      
      linkHandler = mPresContext->GetLinkHandler();
    }

    DestroyPresShell();

    nsIView* containerView = FindContainerView();

    
    
    MakeWindow(currentSize, containerView);
  }

  
  if (mPresContext) {
    
    if (linkHandler) {
      mPresContext->SetLinkHandler(linkHandler);
    }

    rv = InitPresentationStuff(PR_FALSE, PR_FALSE);
    if (NS_SUCCEEDED(rv) && mEnableRendering && mViewManager) {
      mViewManager->EnableRefresh(NS_VMREFRESH_IMMEDIATE);
    }
  }

  return rv;
}

NS_IMETHODIMP
DocumentViewerImpl::GetDocument(nsIDocument** aResult)
{
  NS_IF_ADDREF(*aResult = mDocument);

  return NS_OK;
}

nsIPresShell*
DocumentViewerImpl::GetPresShell()
{
  if (!GetIsPrintPreview()) {
    return mPresShell;
  }
  NS_ENSURE_TRUE(mDocument, nsnull);
  nsCOMPtr<nsIPresShell> shell;
  nsCOMPtr<nsIPresShell> currentShell;
  nsPresShellIterator iter(mDocument);
  while ((shell = iter.GetNextShell())) {
    currentShell.swap(shell);
  }
  return currentShell.get();
}

nsPresContext*
DocumentViewerImpl::GetPresContext()
{
  if (!GetIsPrintPreview()) {
    return mPresContext;
  }
  nsIPresShell* shell = GetPresShell();
  return shell ? shell->GetPresContext() : nsnull;
}

nsIViewManager*
DocumentViewerImpl::GetViewManager()
{
  if (!GetIsPrintPreview()) {
    return mViewManager;
  }
  nsIPresShell* shell = GetPresShell();
  return shell ? shell->GetViewManager() : nsnull;
}

NS_IMETHODIMP
DocumentViewerImpl::GetPresShell(nsIPresShell** aResult)
{
  nsIPresShell* shell = GetPresShell();
  NS_IF_ADDREF(*aResult = shell);
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetPresContext(nsPresContext** aResult)
{
  nsPresContext* pc = GetPresContext();
  NS_IF_ADDREF(*aResult = pc);
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetBounds(nsIntRect& aResult)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  aResult = mBounds;
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
DocumentViewerImpl::SetBounds(const nsIntRect& aBounds)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);

  mBounds = aBounds;
  if (mWindow) {
    
    
    mWindow->Resize(aBounds.x, aBounds.y, aBounds.width, aBounds.height,
                    PR_FALSE);
  } else if (mPresContext && mViewManager) {
    PRInt32 p2a = mPresContext->AppUnitsPerDevPixel();
    mViewManager->SetWindowDimensions(NSIntPixelsToAppUnits(mBounds.width, p2a),
                                      NSIntPixelsToAppUnits(mBounds.height, p2a));
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
  mBounds.MoveTo(aX, aY);
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

  if (mDocument && !mPresShell) {
    NS_ASSERTION(!mWindow, "Window already created but no presshell?");

    nsCOMPtr<nsIBaseWindow> base_win(do_QueryReferent(mContainer));
    if (base_win) {
      base_win->GetParentWidget(&mParentWidget);
      if (mParentWidget) {
        mParentWidget->Release(); 
      }
    }

    nsIView* containerView = FindContainerView();

    nsresult rv = CreateDeviceContext(containerView);
    NS_ENSURE_SUCCESS(rv, rv);

    
    NS_ASSERTION(!mPresContext, "Shouldn't have a prescontext if we have no shell!");
    mPresContext = CreatePresContext(mDocument,
        nsPresContext::eContext_Galley, containerView);
    NS_ENSURE_TRUE(mPresContext, NS_ERROR_OUT_OF_MEMORY);

    rv = mPresContext->Init(mDeviceContext);
    if (NS_FAILED(rv)) {
      mPresContext = nsnull;
      return rv;
    }

    rv = MakeWindow(nsSize(mPresContext->DevPixelsToAppUnits(mBounds.width),
                           mPresContext->DevPixelsToAppUnits(mBounds.height)),
                           containerView);
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

      rv = InitPresentationStuff(mDocument->MayStartLayout(),
                                 mDocument->MayStartLayout());
    }

    
    
    

    nsCOMPtr<nsIPresShell> shellDeathGrip(mPresShell); 
    mPresShell->UnsuppressPainting();
  }

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Hide(void)
{
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

  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mContainer));
  if (docShell) {
    nsCOMPtr<nsILayoutHistoryState> layoutState;
    mPresShell->CaptureHistoryState(getter_AddRefs(layoutState), PR_TRUE);
  }

  DestroyPresShell();

  
  mPresContext->SetContainer(nsnull);
  mPresContext->SetLinkHandler(nsnull);                             

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

static PRBool
AppendAgentSheet(nsIStyleSheet *aSheet, void *aData)
{
  nsStyleSet *styleSet = static_cast<nsStyleSet*>(aData);
  styleSet->AppendStyleSheet(nsStyleSet::eAgentSheet, aSheet);
  return PR_TRUE;
}

static PRBool
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
  

  
  
  nsStyleSet *styleSet = new nsStyleSet();
  if (!styleSet) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  styleSet->BeginUpdate();
  
  
  
  
#ifdef DEBUG
  nsCOMPtr<nsISupports> debugDocContainer = aDocument->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> debugDocShell(do_QueryReferent(mContainer));
  NS_ASSERTION(SameCOMIdentity(debugDocContainer, debugDocShell),
               "Unexpected containers");
#endif
  nsICSSStyleSheet* sheet = nsnull;
  if (nsContentUtils::IsInChromeDocshell(aDocument)) {
    sheet = nsLayoutStylesheetCache::UserChromeSheet();
  }
  else {
    sheet = nsLayoutStylesheetCache::UserContentSheet();
  }

  if (sheet)
    styleSet->AppendStyleSheet(nsStyleSet::eUserSheet, sheet);

  
  PRBool shouldOverride = PR_FALSE;
  
  
  nsCOMPtr<nsIDocShell> ds(do_QueryReferent(mContainer));
  nsCOMPtr<nsIDOMEventTarget> chromeHandler;
  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<nsICSSStyleSheet> csssheet;

  if (ds) {
    ds->GetChromeEventHandler(getter_AddRefs(chromeHandler));
  }
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

  
  
  nsCOMPtr<nsICSSStyleSheet> quirkClone;
  if (!nsLayoutStylesheetCache::UASheet() ||
      !nsLayoutStylesheetCache::QuirkSheet() ||
      NS_FAILED(nsLayoutStylesheetCache::QuirkSheet()->
                Clone(nsnull, nsnull, nsnull, nsnull,
                      getter_AddRefs(quirkClone))) ||
      !sheet) {
    delete styleSet;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  
  styleSet->PrependStyleSheet(nsStyleSet::eAgentSheet, quirkClone);
  styleSet->SetQuirkStyleSheet(quirkClone);
  styleSet->PrependStyleSheet(nsStyleSet::eAgentSheet,
                              nsLayoutStylesheetCache::UASheet());

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
DocumentViewerImpl::MakeWindow(const nsSize& aSize, nsIView* aContainerView)
{
  nsresult rv;

  mViewManager = do_CreateInstance(kViewManagerCID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsIDeviceContext *dx = mPresContext->DeviceContext();

  rv = mViewManager->Init(dx);
  if (NS_FAILED(rv))
    return rv;

  
  nsRect tbounds(nsPoint(0, 0), aSize);
  
  nsIView* view = mViewManager->CreateView(tbounds, aContainerView);
  if (!view)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  if (mParentWidget || !aContainerView) {
    
    
    
    nsWidgetInitData initData;
    nsWidgetInitData* initDataPtr;
    if (!mParentWidget) {
      initDataPtr = &initData;
      initData.mWindowType = eWindowType_invisible;

      initData.mContentType =
        nsContentUtils::IsInChromeDocshell(mDocument) ?
          eContentTypeUI : eContentTypeContent;
    } else {
      initDataPtr = nsnull;
    }
    rv = view->CreateWidget(kWidgetCID, initDataPtr,
                            (aContainerView != nsnull || !mParentWidget) ?
                              nsnull : mParentWidget->GetNativeData(NS_NATIVE_WIDGET),
                            PR_TRUE, PR_FALSE);
    if (NS_FAILED(rv))
      return rv;
  }

  
  mViewManager->SetRootView(view);

  mWindow = view->GetWidget();

  
  
  
  

  return rv;
}

nsIView*
DocumentViewerImpl::FindContainerView()
{
  nsIView* containerView = nsnull;

  if (mParentWidget) {
    containerView = nsIView::GetViewFor(mParentWidget);
  } else if (mContainer) {
    nsCOMPtr<nsIDocShellTreeItem> docShellItem = do_QueryReferent(mContainer);
    nsCOMPtr<nsPIDOMWindow> pwin(do_GetInterface(docShellItem));
    if (pwin) {
      nsCOMPtr<nsIContent> content = do_QueryInterface(pwin->GetFrameElementInternal());
      nsCOMPtr<nsIPresShell> parentPresShell;
      if (docShellItem) {
        nsCOMPtr<nsIDocShellTreeItem> parentDocShellItem;
        docShellItem->GetParent(getter_AddRefs(parentDocShellItem));
        if (parentDocShellItem) {
          nsCOMPtr<nsIDocShell> parentDocShell = do_QueryInterface(parentDocShellItem);
          parentDocShell->GetPresShell(getter_AddRefs(parentPresShell));
        }
      }
      if (!content) {
        NS_WARNING("Subdocument container has no content");
      } else if (!parentPresShell) {
        NS_WARNING("Subdocument container has no presshell");
      } else {
        nsIFrame* f = parentPresShell->GetRealPrimaryFrameFor(content);
        if (f) {
          nsIFrame* subdocFrame = f->GetContentInsertionFrame();
          
          
          
          
          if (subdocFrame->GetType() == nsGkAtoms::subDocumentFrame) {
            nsIView* subdocFrameView = subdocFrame->GetView();
            NS_ASSERTION(subdocFrameView, "Subdoc frames must have views");
            nsIView* innerView = subdocFrameView->GetFirstChild();
            NS_ASSERTION(innerView, "Subdoc frames must have an inner view too");
            containerView = innerView;
          } else {
            NS_WARNING("Subdocument container has non-subdocument frame");
          }
        } else {
          NS_WARNING("Subdocument container has no frame");
        }
      }
    }
  }

  if (!containerView)
    return nsnull;

  nsIWidget* outerWidget = containerView->GetNearestWidget(nsnull);
  if (outerWidget &&
      outerWidget->GetTransparencyMode() == eTransparencyTransparent)
    return containerView;

  
  
  nsIViewManager* containerVM = containerView->GetViewManager();
  nsIView* pView = containerView;
  do {
    pView = pView->GetParent();
  } while (pView && pView->GetViewManager() == containerVM);
  if (pView)
    return containerView;

  
  
  
  
  
  
  
  
  
  nsCOMPtr<nsIDocShellTreeItem> container(do_QueryReferent(mContainer));
  if (container) {
    nsCOMPtr<nsIDocShellTreeItem> sameTypeParent;
    container->GetSameTypeParent(getter_AddRefs(sameTypeParent));
    if (sameTypeParent)
      return containerView;
  }
  return nsnull;
}

nsresult
DocumentViewerImpl::CreateDeviceContext(nsIView* aContainerView)
{
  NS_PRECONDITION(!mPresShell && !mPresContext && !mWindow,
                  "This will screw up our existing presentation");
  NS_PRECONDITION(mDocument, "Gotta have a document here");
  
  nsIDocument* doc = mDocument->GetDisplayDocument();
  if (doc) {
    NS_ASSERTION(!aContainerView, "External resource document embedded somewhere?");
    
    nsIPresShell* shell = doc->GetPrimaryShell();
    if (shell) {
      nsPresContext* ctx = shell->GetPresContext();
      if (ctx) {
        mDeviceContext = ctx->DeviceContext();
        return NS_OK;
      }
    }
  }
  
  
  
  mDeviceContext = do_CreateInstance(kDeviceContextCID);
  NS_ENSURE_TRUE(mDeviceContext, NS_ERROR_FAILURE);
  nsIWidget* widget = nsnull;
  if (aContainerView) {
    widget = aContainerView->GetNearestWidget(nsnull);
  }
  
  if (!widget) {
    widget = mParentWidget;
  }
  if (widget) {
    widget = widget->GetTopLevelWidget();
  }

  mDeviceContext->Init(widget);
  return NS_OK;
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
  PRBool preventDefault;
  nsresult rv = FireClipboardEvent(NS_COPY, &preventDefault);
  if (NS_FAILED(rv) || preventDefault)
    return rv;

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

nsresult DocumentViewerImpl::FireClipboardEvent(PRUint32 msg,
                                                PRBool* aPreventDefault)
{
  *aPreventDefault = PR_FALSE;

  NS_ENSURE_TRUE(mPresContext, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);

  
  PRBool isReflowing = PR_TRUE;
  nsresult rv = mPresShell->IsReflowLocked(&isReflowing);
  if (NS_FAILED(rv) || isReflowing)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> eventTarget;
  rv = GetClipboardEventTarget(getter_AddRefs(eventTarget));
  if (NS_FAILED(rv))
    
    return NS_OK;

  nsEventStatus status = nsEventStatus_eIgnore;
  nsEvent evt(PR_TRUE, msg);
  nsEventDispatcher::Dispatch(eventTarget, mPresContext, &evt, nsnull,
                              &status);
  
  if (status == nsEventStatus_eConsumeNoDefault)
    *aPreventDefault = PR_TRUE;

  
  
  NS_ENSURE_STATE(mPresShell);

  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetCopyable(PRBool *aCopyable)
{
  NS_ENSURE_ARG_POINTER(aCopyable);
  *aCopyable = PR_FALSE;

  NS_ENSURE_STATE(mPresShell);
  nsCOMPtr<nsISelection> selection;
  nsresult rv = mPresShell->GetSelectionForCopy(getter_AddRefs(selection));
  if (NS_FAILED(rv))
    return rv;

  PRBool isCollapsed;
  selection->GetIsCollapsed(&isCollapsed);

  *aCopyable = !isCollapsed;
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::CutSelection()
{
  
  
  PRBool preventDefault;
  return FireClipboardEvent(NS_CUT, &preventDefault);
}

NS_IMETHODIMP DocumentViewerImpl::GetCutable(PRBool *aCutable)
{
  NS_ENSURE_ARG_POINTER(aCutable);
  *aCutable = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::Paste()
{
  
  
  PRBool preventDefault;
  return FireClipboardEvent(NS_PASTE, &preventDefault);
}

NS_IMETHODIMP DocumentViewerImpl::GetPasteable(PRBool *aPasteable)
{
  NS_ENSURE_ARG_POINTER(aPasteable);
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

struct ZoomInfo
{
  float mZoom;
};

static void
SetChildTextZoom(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  struct ZoomInfo* ZoomInfo = (struct ZoomInfo*) aClosure;
  aChild->SetTextZoom(ZoomInfo->mZoom);
}

static void
SetChildFullZoom(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  struct ZoomInfo* ZoomInfo = (struct ZoomInfo*) aClosure;
  aChild->SetFullZoom(ZoomInfo->mZoom);
}

static PRBool
SetExtResourceTextZoom(nsIDocument* aDocument, void* aClosure)
{
  
  nsIPresShell* shell = aDocument->GetPrimaryShell();
  if (shell) {
    nsPresContext* ctxt = shell->GetPresContext();
    if (ctxt) {
      struct ZoomInfo* ZoomInfo = static_cast<struct ZoomInfo*>(aClosure);
      ctxt->SetTextZoom(ZoomInfo->mZoom);
    }
  }

  return PR_TRUE;
}

static PRBool
SetExtResourceFullZoom(nsIDocument* aDocument, void* aClosure)
{
  
  nsIPresShell* shell = aDocument->GetPrimaryShell();
  if (shell) {
    nsPresContext* ctxt = shell->GetPresContext();
    if (ctxt) {
      struct ZoomInfo* ZoomInfo = static_cast<struct ZoomInfo*>(aClosure);
      ctxt->SetFullZoom(ZoomInfo->mZoom);
    }
  }

  return PR_TRUE;
}

NS_IMETHODIMP
DocumentViewerImpl::SetTextZoom(float aTextZoom)
{
  if (GetIsPrintPreview()) {
    return NS_OK;
  }

  mTextZoom = aTextZoom;

  nsIViewManager::UpdateViewBatch batch(GetViewManager());
      
  
  
  
  
  struct ZoomInfo ZoomInfo = { aTextZoom };
  CallChildren(SetChildTextZoom, &ZoomInfo);

  
  nsPresContext* pc = GetPresContext();
  if (pc && aTextZoom != mPresContext->TextZoom()) {
      pc->SetTextZoom(aTextZoom);
  }

  
  mDocument->EnumerateExternalResources(SetExtResourceTextZoom, &ZoomInfo);

  batch.EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
  
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetTextZoom(float* aTextZoom)
{
  NS_ENSURE_ARG_POINTER(aTextZoom);
  nsPresContext* pc = GetPresContext();
  *aTextZoom = pc ? pc->TextZoom() : 1.0f;
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::SetFullZoom(float aFullZoom)
{
#ifdef NS_PRINT_PREVIEW
  if (GetIsPrintPreview()) {
    nsPresContext* pc = GetPresContext();
    NS_ENSURE_TRUE(pc, NS_OK);
    nsCOMPtr<nsIPresShell> shell = pc->GetPresShell();
    NS_ENSURE_TRUE(shell, NS_OK);

    nsIViewManager::UpdateViewBatch batch(shell->GetViewManager());
    if (!mPrintPreviewZoomed) {
      mOriginalPrintPreviewScale = pc->GetPrintPreviewScale();
      mPrintPreviewZoomed = PR_TRUE;
    }

    mPrintPreviewZoom = aFullZoom;
    pc->SetPrintPreviewScale(aFullZoom * mOriginalPrintPreviewScale);
    nsIPageSequenceFrame* pf = nsnull;
    shell->GetPageSequenceFrame(&pf);
    if (pf) {
      nsIFrame* f = do_QueryFrame(pf);
      shell->FrameNeedsReflow(f, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);
    }

    nsIFrame* rootFrame = shell->GetRootFrame();
    if (rootFrame) {
      nsRect rect(nsPoint(0, 0), rootFrame->GetSize());
      rootFrame->Invalidate(rect);
    }
    batch.EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
    return NS_OK;
  }
#endif

  mPageZoom = aFullZoom;

  nsIViewManager::UpdateViewBatch batch(GetViewManager());

  struct ZoomInfo ZoomInfo = { aFullZoom };
  CallChildren(SetChildFullZoom, &ZoomInfo);

  nsPresContext* pc = GetPresContext();
  if (pc) {
    pc->SetFullZoom(aFullZoom);
  }

  
  mDocument->EnumerateExternalResources(SetExtResourceFullZoom, &ZoomInfo);

  batch.EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetFullZoom(float* aFullZoom)
{
  NS_ENSURE_ARG_POINTER(aFullZoom);
#ifdef NS_PRINT_PREVIEW
  if (GetIsPrintPreview()) {
    *aFullZoom = mPrintPreviewZoom;
    return NS_OK;
  }
#endif
  
  
  nsPresContext* pc = GetPresContext();
  *aFullZoom = pc ? pc->GetFullZoom() : mPageZoom;
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
   NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_NOT_AVAILABLE);
   
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
   
   NS_ENSURE_TRUE(shellArea.width != NS_UNCONSTRAINEDSIZE &&
                  shellArea.height != NS_UNCONSTRAINEDSIZE,
                  NS_ERROR_FAILURE);
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

  if (!mContainer) {
    PR_PL(("Container was destroyed yet we are still trying to use it!"));
    return NS_ERROR_FAILURE;
  }

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
  if (!presShell || !mDocument || !mDeviceContext) {
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
                                  float(mDeviceContext->AppUnitsPerInch()) /
                                  float(mDeviceContext->AppUnitsPerDevPixel()) /
                                  mPageZoom,
                                  mParentWidget,
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

  if (!mContainer) {
    PR_PL(("Container was destroyed yet we are still trying to use it!"));
    return NS_ERROR_FAILURE;
  }

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
                                  float(mDeviceContext->AppUnitsPerInch()) /
                                  float(mDeviceContext->AppUnitsPerDevPixel()) /
                                  mPageZoom,
                                  mParentWidget,
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
  mPrintPreviewZoomed = PR_FALSE;
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
    scrollableView->ScrollTo(0, 0, 0);
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
    nscoord deadSpaceGapTwips = 0;
    nsIPageSequenceFrame * sqf = do_QueryFrame(seqFrame);
    if (sqf) {
      sqf->GetDeadSpaceValue(&deadSpaceGapTwips);
    }

    
    
    
    nscoord deadSpaceGap = 
      seqFrame->PresContext()->TwipsToAppUnits(deadSpaceGapTwips);

    nscoord newYPosn = 
      nscoord(mPrintEngine->GetPrintPreviewScale() * 
              float(fndPageFrame->GetPosition().y - deadSpaceGap));
    scrollableView->ScrollTo(0, newYPosn, 0);
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
  printf("TEST-INFO ExitPrintPreview: mPrintEngine=%p, GetIsPrinting()=%d\n",
         static_cast<void*>(mPrintEngine.get()), GetIsPrinting());
  if (GetIsPrinting())
    return NS_ERROR_FAILURE;
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

  if (mPresContext)
    mPresContext->RestoreImageAnimationMode();

  SetTextZoom(mTextZoom);
  SetFullZoom(mPageZoom);
  Show();

#endif 
}



static void
ResetFocusState(nsIDocShell* aDocShell)
{
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm)
    return;

  nsCOMPtr<nsISimpleEnumerator> docShellEnumerator;
  aDocShell->GetDocShellEnumerator(nsIDocShellTreeItem::typeContent,
                                   nsIDocShell::ENUMERATE_FORWARDS,
                                   getter_AddRefs(docShellEnumerator));
  
  nsCOMPtr<nsISupports> currentContainer;
  PRBool hasMoreDocShells;
  while (NS_SUCCEEDED(docShellEnumerator->HasMoreElements(&hasMoreDocShells))
         && hasMoreDocShells) {
    docShellEnumerator->GetNext(getter_AddRefs(currentContainer));
    nsCOMPtr<nsIDOMWindow> win = do_GetInterface(currentContainer);
    if (win)
      fm->ClearFocus(win);
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
    }
    if (mPresContext)
      mPresContext->RestoreImageAnimationMode();
  }
#endif 
}

NS_IMETHODIMP DocumentViewerImpl::SetPageMode(PRBool aPageMode, nsIPrintSettings* aPrintSettings)
{
  
  
  mIsPageMode = aPageMode;

  if (mPresShell) {
    DestroyPresShell();
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
    mPresContext = CreatePresContext(mDocument,
        nsPresContext::eContext_PageLayout, FindContainerView());
    NS_ENSURE_TRUE(mPresContext, NS_ERROR_OUT_OF_MEMORY);
    mPresContext->SetPaginatedScrolling(PR_TRUE);
    mPresContext->SetPrintSettings(aPrintSettings);
    nsresult rv = mPresContext->Init(mDeviceContext);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  InitInternal(mParentWidget, nsnull, mBounds, PR_TRUE, PR_FALSE, PR_FALSE);
  mViewManager->EnableRefresh(NS_VMREFRESH_NO_SYNC);

  Show();
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetHistoryEntry(nsISHEntry **aHistoryEntry)
{
  NS_IF_ADDREF(*aHistoryEntry = mSHEntry);
  return NS_OK;
}

void
DocumentViewerImpl::DestroyPresShell()
{
  
  mPresShell->EndObservingDocument();

  nsCOMPtr<nsISelection> selection;
  GetDocumentSelection(getter_AddRefs(selection));
  nsCOMPtr<nsISelectionPrivate> selPrivate = do_QueryInterface(selection);
  if (selPrivate && mSelectionListener)
    selPrivate->RemoveSelectionListener(mSelectionListener);

  nsAutoScriptBlocker scriptBlocker;
  mPresShell->Destroy();
  mPresShell = nsnull;
}

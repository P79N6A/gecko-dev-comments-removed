







#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsISupports.h"
#include "nsIContent.h"
#include "nsIContentViewerContainer.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewerPrint.h"
#include "nsIDOMBeforeUnloadEvent.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsStyleSet.h"
#include "nsIStyleSheet.h"
#include "nsCSSStyleSheet.h"
#include "nsIFrame.h"
#include "nsSubDocumentFrame.h"

#include "nsILinkHandler.h"
#include "nsIDOMDocument.h"
#include "nsISelectionListener.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMRange.h"
#include "nsContentCID.h"
#include "nsLayoutCID.h"
#include "nsContentUtils.h"
#include "nsLayoutStylesheetCache.h"
#include "mozilla/Preferences.h"

#include "nsIDeviceContextSpec.h"
#include "nsViewManager.h"
#include "nsView.h"

#include "nsIPageSequenceFrame.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIContentViewerEdit.h"
#include "nsIContentViewerFile.h"
#include "mozilla/css/Loader.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShell.h"
#include "nsIBaseWindow.h"
#include "nsILayoutHistoryState.h"
#include "nsCharsetSource.h"
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

#include "nsIClipboardHelper.h"

#include "nsPIDOMWindow.h"
#include "nsDOMNavigationTiming.h"
#include "nsPIWindowRoot.h"
#include "nsJSEnvironment.h"
#include "nsFocusManager.h"

#include "nsIScrollableFrame.h"
#include "nsIHTMLDocument.h"
#include "nsGfxCIID.h"
#include "nsStyleSheetService.h"
#include "nsURILoader.h"
#include "nsRenderingContext.h"
#include "nsILoadContext.h"

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


#include "nsPagePrintTimer.h"

#endif 


#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
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

#include "nsObserverService.h"

#include "mozilla/dom/Element.h"

#include "jsfriendapi.h"

using namespace mozilla;

#ifdef DEBUG

#undef NOISY_VIEWER
#else
#undef NOISY_VIEWER
#endif



#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "prlog.h"

#ifdef PR_LOGGING

#ifdef NS_PRINTING
static PRLogModuleInfo *
GetPrintingLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("printing");
  return sLog;
}
#define PR_PL(_p1)  PR_LOG(GetPrintingLog(), PR_LOG_DEBUG, _p1);
#endif 

#define PRT_YESNO(_p) ((_p)?"YES":"NO")
#else
#define PRT_YESNO(_p)
#define PR_PL(_p1)
#endif


class nsDocumentViewer;
class nsPrintEventDispatcher;



class nsDocViewerSelectionListener : public nsISelectionListener
{
public:

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSISELECTIONLISTENER

                       nsDocViewerSelectionListener()
                       : mDocViewer(NULL)
                       , mGotSelectionState(false)
                       , mSelectionWasCollapsed(false)
                       {
                       }

  virtual              ~nsDocViewerSelectionListener() {}

  nsresult             Init(nsDocumentViewer *aDocViewer);

protected:

  nsDocumentViewer*  mDocViewer;
  bool                 mGotSelectionState;
  bool                 mSelectionWasCollapsed;

};




class nsDocViewerFocusListener : public nsIDOMEventListener
{
public:
  

  nsDocViewerFocusListener();
  

  virtual ~nsDocViewerFocusListener();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  nsresult             Init(nsDocumentViewer *aDocViewer);

private:
    nsDocumentViewer*  mDocViewer;
};



class nsDocumentViewer : public nsIContentViewer,
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
  nsDocumentViewer();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICONTENTVIEWER

  
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


  static void DispatchBeforePrint(nsIDocument* aTop)
  {
    DispatchEventToWindowTree(aTop, NS_LITERAL_STRING("beforeprint"));
  }
  static void DispatchAfterPrint(nsIDocument* aTop)
  {
    DispatchEventToWindowTree(aTop, NS_LITERAL_STRING("afterprint"));
  }
  static void DispatchEventToWindowTree(nsIDocument* aTop,
                                        const nsAString& aEvent);

protected:
  virtual ~nsDocumentViewer();

private:
  






  nsresult MakeWindow(const nsSize& aSize, nsView* aContainerView);

  


  nsresult CreateDeviceContext(nsView* aContainerView);

  






  nsresult InitInternal(nsIWidget* aParentWidget,
                        nsISupports *aState,
                        const nsIntRect& aBounds,
                        bool aDoCreation,
                        bool aNeedMakeCX = true,
                        bool aForceSetNewDocument = true);
  



  nsresult InitPresentationStuff(bool aDoInitialReflow);

  nsresult GetPopupNode(nsIDOMNode** aNode);
  nsresult GetPopupLinkNode(nsIDOMNode** aNode);
  nsresult GetPopupImageNode(nsIImageLoadingContent** aNode);

  void PrepareToStartLoad(void);

  nsresult SyncParentSubDocMap();

  nsresult GetDocumentSelection(nsISelection **aSelection);

  void DestroyPresShell();
  void DestroyPresContext();

#ifdef NS_PRINTING
  
  
  void SetIsPrintingInDocShellTree(nsIDocShellTreeNode* aParentNode, 
                                   bool                 aIsPrintingOrPP, 
                                   bool                 aStartAtTop);
#endif 

  
  
  
  bool ShouldAttachToTopLevel();

protected:
  
  nsIPresShell* GetPresShell();
  nsPresContext* GetPresContext();
  nsViewManager* GetViewManager();

  void DetachFromTopLevelWidget();

  
  
  
  
  

  nsWeakPtr mContainer; 
  nsWeakPtr mTopContainerWhilePrinting;
  nsRefPtr<nsDeviceContext> mDeviceContext;  

  
  
  nsCOMPtr<nsIDocument>    mDocument;
  nsCOMPtr<nsIWidget>      mWindow;      
  nsRefPtr<nsViewManager> mViewManager;
  nsRefPtr<nsPresContext>  mPresContext;
  nsCOMPtr<nsIPresShell>   mPresShell;

  nsCOMPtr<nsISelectionListener> mSelectionListener;
  nsRefPtr<nsDocViewerFocusListener> mFocusListener;

  nsCOMPtr<nsIContentViewer> mPreviousViewer;
  nsCOMPtr<nsISHEntry> mSHEntry;

  nsIWidget* mParentWidget; 
  bool mAttachedToParent; 

  nsIntRect mBounds;

  
  
  float mTextZoom;      
  float mPageZoom;
  int mMinFontSize;

  int16_t mNumURLStarts;
  int16_t mDestroyRefCount;    

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

  nsRefPtr<nsPrintEngine>          mPrintEngine;
  float                            mOriginalPrintPreviewScale;
  float                            mPrintPreviewZoom;
  nsAutoPtr<nsPrintEventDispatcher> mBeforeAndAfterPrint;
#endif 

#ifdef DEBUG
  FILE* mDebugFile;
#endif 
#endif 

  
  int32_t mHintCharsetSource;
  nsCString mHintCharset;
  nsCString mDefaultCharacterSet;
  nsCString mForceCharacterSet;
  nsCString mPrevDocCharacterSet;
  
  bool mIsPageMode;
  bool mCallerIsClosingWindow;
  bool mInitializedForPrintPreview;
  bool mHidden;
};

class nsPrintEventDispatcher
{
public:
  nsPrintEventDispatcher(nsIDocument* aTop) : mTop(aTop)
  {
    nsDocumentViewer::DispatchBeforePrint(mTop);
  }
  ~nsPrintEventDispatcher()
  {
    nsDocumentViewer::DispatchAfterPrint(mTop);
  }

  nsCOMPtr<nsIDocument> mTop;
};

class nsDocumentShownDispatcher : public nsRunnable
{
public:
  nsDocumentShownDispatcher(nsCOMPtr<nsIDocument> aDocument)
  : mDocument(aDocument) {}

  NS_IMETHOD Run();

private:
  nsCOMPtr<nsIDocument> mDocument;
};







nsresult
NS_NewContentViewer(nsIContentViewer** aResult)
{
  *aResult = new nsDocumentViewer();

  NS_ADDREF(*aResult);

  return NS_OK;
}

void nsDocumentViewer::PrepareToStartLoad()
{
  mStopped          = false;
  mLoaded           = false;
  mAttachedToParent = false;
  mDeferredWindowClose = false;
  mCallerIsClosingWindow = false;

#ifdef NS_PRINTING
  mPrintIsPending        = false;
  mPrintDocIsFullyLoaded = false;
  mClosingWhilePrinting  = false;

  
  if (mPrintEngine) {
    mPrintEngine->Destroy();
    mPrintEngine = nullptr;
#ifdef NS_PRINT_PREVIEW
    SetIsPrintPreview(false);
#endif
  }

#ifdef DEBUG
  mDebugFile = nullptr;
#endif

#endif 
}


nsDocumentViewer::nsDocumentViewer()
  : mTextZoom(1.0), mPageZoom(1.0), mMinFontSize(0),
    mIsSticky(true),
#ifdef NS_PRINT_PREVIEW
    mPrintPreviewZoom(1.0),
#endif
    mHintCharsetSource(kCharsetUninitialized),
    mInitializedForPrintPreview(false),
    mHidden(false)
{
  PrepareToStartLoad();
}

NS_IMPL_ADDREF(nsDocumentViewer)
NS_IMPL_RELEASE(nsDocumentViewer)

NS_INTERFACE_MAP_BEGIN(nsDocumentViewer)
    NS_INTERFACE_MAP_ENTRY(nsIContentViewer)
    NS_INTERFACE_MAP_ENTRY(nsIMarkupDocumentViewer)
    NS_INTERFACE_MAP_ENTRY(nsIContentViewerFile)
    NS_INTERFACE_MAP_ENTRY(nsIContentViewerEdit)
    NS_INTERFACE_MAP_ENTRY(nsIDocumentViewerPrint)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContentViewer)
#ifdef NS_PRINTING
    NS_INTERFACE_MAP_ENTRY(nsIWebBrowserPrint)
#endif
NS_INTERFACE_MAP_END

nsDocumentViewer::~nsDocumentViewer()
{
  if (mDocument) {
    Close(nullptr);
    mDocument->Destroy();
  }

  NS_ASSERTION(!mPresShell && !mPresContext,
               "User did not call nsIContentViewer::Destroy");
  if (mPresShell || mPresContext) {
    
    
    mSHEntry = nullptr;

    Destroy();
  }

  
}









NS_IMETHODIMP
nsDocumentViewer::LoadStart(nsISupports *aDoc)
{
#ifdef NOISY_VIEWER
  printf("nsDocumentViewer::LoadStart\n");
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
nsDocumentViewer::SyncParentSubDocMap()
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
        if (mDocument &&
            parent_doc->GetSubDocumentFor(content) != mDocument) {
          mDocument->SuppressEventHandling(parent_doc->EventHandlingSuppressed());
        }
        return parent_doc->SetSubDocumentFor(content->AsElement(), mDocument);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::SetContainer(nsISupports* aContainer)
{
  mContainer = do_GetWeakReference(aContainer);
  if (mPresContext) {
    mPresContext->SetContainer(aContainer);
  }

  
  
  

  return SyncParentSubDocMap();
}

NS_IMETHODIMP
nsDocumentViewer::GetContainer(nsISupports** aResult)
{
   NS_ENSURE_ARG_POINTER(aResult);

   *aResult = nullptr;
   nsCOMPtr<nsISupports> container = do_QueryReferent(mContainer);
   container.swap(*aResult);
   return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::Init(nsIWidget* aParentWidget,
                         const nsIntRect& aBounds)
{
  return InitInternal(aParentWidget, nullptr, aBounds, true);
}

nsresult
nsDocumentViewer::InitPresentationStuff(bool aDoInitialReflow)
{
  if (GetIsPrintPreview())
    return NS_OK;

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

  mViewManager->SetWindowDimensions(width, height);
  mPresContext->SetTextZoom(mTextZoom);
  mPresContext->SetFullZoom(mPageZoom);
  mPresContext->SetMinFontSize(mMinFontSize);

  if (aDoInitialReflow) {
    nsCOMPtr<nsIPresShell> shellGrip = mPresShell;
    
    mPresShell->Initialize(width, height);
  } else {
    
    
    mPresContext->SetVisibleArea(nsRect(0, 0, width, height));
  }

  
  
  if (!mSelectionListener) {
    nsDocViewerSelectionListener *selectionListener =
      new nsDocViewerSelectionListener();

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

  
  nsRefPtr<nsDocViewerFocusListener> oldFocusListener = mFocusListener;

  
  
  
  
  nsDocViewerFocusListener *focusListener = new nsDocViewerFocusListener();

  focusListener->Init(this);

  
  mFocusListener = focusListener;

  if (mDocument) {
    mDocument->AddEventListener(NS_LITERAL_STRING("focus"),
                                mFocusListener,
                                false, false);
    mDocument->AddEventListener(NS_LITERAL_STRING("blur"),
                                mFocusListener,
                                false, false);

    if (oldFocusListener) {
      mDocument->RemoveEventListener(NS_LITERAL_STRING("focus"),
                                     oldFocusListener, false);
      mDocument->RemoveEventListener(NS_LITERAL_STRING("blur"),
                                     oldFocusListener, false);
    }
  }

  if (aDoInitialReflow && mDocument) {
    mDocument->ScrollToRef();
  }

  return NS_OK;
}

static nsPresContext*
CreatePresContext(nsIDocument* aDocument,
                  nsPresContext::nsPresContextType aType,
                  nsView* aContainerView)
{
  if (aContainerView)
    return new nsPresContext(aDocument, aType);
  return new nsRootPresContext(aDocument, aType);
}





nsresult
nsDocumentViewer::InitInternal(nsIWidget* aParentWidget,
                                 nsISupports *aState,
                                 const nsIntRect& aBounds,
                                 bool aDoCreation,
                                 bool aNeedMakeCX ,
                                 bool aForceSetNewDocument )
{
  if (mIsPageMode) {
    
    
    aForceSetNewDocument = false;
  }

  
  
  
  nsAutoScriptBlocker blockScripts;

  mParentWidget = aParentWidget; 
  mBounds = aBounds;

  nsresult rv = NS_OK;
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NULL_POINTER);

  nsView* containerView = FindContainerView();

  bool makeCX = false;
  if (aDoCreation) {
    nsresult rv = CreateDeviceContext(containerView);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    if (!mPresContext &&
        (aParentWidget || containerView || mDocument->IsBeingUsedAsImage() ||
         (mDocument->GetDisplayDocument() &&
          mDocument->GetDisplayDocument()->GetShell()))) {
      
      if (mIsPageMode) {
        
      } else {
        mPresContext = CreatePresContext(mDocument,
            nsPresContext::eContext_Galley, containerView);
      }
      NS_ENSURE_TRUE(mPresContext, NS_ERROR_OUT_OF_MEMORY);

      nsresult rv = mPresContext->Init(mDeviceContext); 
      if (NS_FAILED(rv)) {
        mPresContext = nullptr;
        return rv;
      }

#if defined(NS_PRINTING) && defined(NS_PRINT_PREVIEW)
      makeCX = !GetIsPrintPreview() && aNeedMakeCX; 
#else
      makeCX = true;
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
        
        
        
        
        double pageWidth = 0, pageHeight = 0;
        mPresContext->GetPrintSettings()->GetEffectivePageSize(&pageWidth,
                                                               &pageHeight);
        mPresContext->SetPageSize(
          nsSize(mPresContext->CSSTwipsToAppUnits(NSToIntFloor(pageWidth)),
                 mPresContext->CSSTwipsToAppUnits(NSToIntFloor(pageHeight))));
        mPresContext->SetIsRootPaginatedDocument(true);
        mPresContext->SetPageScale(1.0f);
      }
#endif
    } else {
      
      if (mPreviousViewer) {
        mPreviousViewer->Destroy();
        mPreviousViewer = nullptr;
      }
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

    

    nsCOMPtr<nsPIDOMWindow> window;
    requestor->GetInterface(NS_GET_IID(nsPIDOMWindow),
                            getter_AddRefs(window));

    if (window) {
      nsCOMPtr<nsIDocument> curDoc =
        do_QueryInterface(window->GetExtantDocument());
      if (aForceSetNewDocument || curDoc != mDocument) {
        window->SetNewDocument(mDocument, aState, false);
        nsJSContext::LoadStart();
      }
    }
  }

  if (aDoCreation && mPresContext) {
    
    

    rv = InitPresentationStuff(!makeCX);
  }

  return rv;
}

void nsDocumentViewer::SetNavigationTiming(nsDOMNavigationTiming* timing)
{
  NS_ASSERTION(mDocument, "Must have a document to set navigation timing.");
  if (mDocument) {
    mDocument->SetNavigationTiming(timing);
  }
}









NS_IMETHODIMP
nsDocumentViewer::LoadComplete(nsresult aStatus)
{
  




  nsRefPtr<nsDocumentViewer> kungFuDeathGrip(this);

  
  
  
  if (mPresShell && !mStopped) {
    
    nsCOMPtr<nsIPresShell> shell = mPresShell;
    shell->FlushPendingNotifications(Flush_Layout);
  }

  nsresult rv = NS_OK;
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);

  
  nsPIDOMWindow *window = mDocument->GetWindow();

  mLoaded = true;

  
  bool restoring = false;
  
  
  
  
  if(window &&
     (NS_SUCCEEDED(aStatus) || aStatus == NS_ERROR_PARSED_DATA_CACHED)) {
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event(true, NS_LOAD);
    event.mFlags.mBubbles = false;
     
    event.target = mDocument;

    
    
    

    nsIDocShell *docShell = window->GetDocShell();
    NS_ENSURE_TRUE(docShell, NS_ERROR_UNEXPECTED);

    docShell->GetRestoringDocument(&restoring);
    if (!restoring) {
      MOZ_ASSERT(mDocument->IsXUL() || 
                 mDocument->GetReadyStateEnum() ==
                   nsIDocument::READYSTATE_INTERACTIVE ||
                 
                 
                 (mDocument->GetReadyStateEnum() ==
                    nsIDocument::READYSTATE_UNINITIALIZED &&
                  NS_IsAboutBlank(mDocument->GetDocumentURI())),
                 "Bad readystate");
      mDocument->SetReadyStateInternal(nsIDocument::READYSTATE_COMPLETE);

      nsRefPtr<nsDOMNavigationTiming> timing(mDocument->GetNavigationTiming());
      if (timing) {
        timing->NotifyLoadEventStart();
      }
      nsEventDispatcher::Dispatch(window, mPresContext, &event, nullptr,
                                  &status);
      if (timing) {
        timing->NotifyLoadEventEnd();
      }
    }
  } else {
    
  }

  
  
  
  if (mDocument) {
    
    window = mDocument->GetWindow();
    if (window) {
      nsIDocShell *docShell = window->GetDocShell();
      bool isInUnload;
      if (docShell && NS_SUCCEEDED(docShell->GetIsInUnload(&isInUnload)) &&
          !isInUnload) {
        mDocument->OnPageShow(restoring, nullptr);
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
    mPrintIsPending        = false;
    mPrintDocIsFullyLoaded = true;
    Print(mCachedPrintSettings, mCachedPrintWebProgressListner);
    mCachedPrintSettings           = nullptr;
    mCachedPrintWebProgressListner = nullptr;
  }
#endif

  return rv;
}

NS_IMETHODIMP
nsDocumentViewer::PermitUnload(bool aCallerClosesWindow, bool *aPermitUnload)
{
  *aPermitUnload = true;

  if (!mDocument || mInPermitUnload || mCallerIsClosingWindow) {
    return NS_OK;
  }

  
  nsPIDOMWindow *window = mDocument->GetWindow();

  if (!window) {
    
    NS_WARNING("window not set for document!");
    return NS_OK;
  }

  NS_ASSERTION(nsContentUtils::IsSafeToRunScript(), "This is unsafe");

  
  
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(mDocument);
  nsCOMPtr<nsIDOMEvent> event;
  domDoc->CreateEvent(NS_LITERAL_STRING("beforeunloadevent"),
                      getter_AddRefs(event));
  nsCOMPtr<nsIDOMBeforeUnloadEvent> beforeUnload = do_QueryInterface(event);
  NS_ENSURE_STATE(beforeUnload);
  nsresult rv = event->InitEvent(NS_LITERAL_STRING("beforeunload"),
                                 false, true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  event->SetTarget(mDocument);
  event->SetTrusted(true);

  
  
  nsRefPtr<nsDocumentViewer> kungFuDeathGrip(this);

  {
    
    
    nsAutoPopupStatePusher popupStatePusher(openAbused, true);

    mInPermitUnload = true;
    nsEventDispatcher::DispatchDOMEvent(window, nullptr, event, mPresContext,
                                        nullptr);
    mInPermitUnload = false;
  }

  nsCOMPtr<nsIDocShellTreeNode> docShellNode(do_QueryReferent(mContainer));
  nsAutoString text;
  beforeUnload->GetReturnValue(text);
  if (event->GetInternalNSEvent()->mFlags.mDefaultPrevented ||
      !text.IsEmpty()) {
    

    nsCOMPtr<nsIPrompt> prompt = do_GetInterface(docShellNode);

    if (prompt) {
      nsXPIDLString title, message, stayLabel, leaveLabel;
      rv  = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                               "OnBeforeUnloadTitle",
                                               title);
      nsresult tmp = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                               "OnBeforeUnloadMessage",
                                               message);
      if (NS_FAILED(tmp)) {
        rv = tmp;
      }
      tmp = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                               "OnBeforeUnloadLeaveButton",
                                               leaveLabel);
      if (NS_FAILED(tmp)) {
        rv = tmp;
      }
      tmp = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                               "OnBeforeUnloadStayButton",
                                               stayLabel);
      if (NS_FAILED(tmp)) {
        rv = tmp;
      }

      if (NS_FAILED(rv) || !title || !message || !stayLabel || !leaveLabel) {
        NS_ERROR("Failed to get strings from dom.properties!");
        return NS_OK;
      }

      
      
      bool dummy = false;
      int32_t buttonPressed = 0;
      uint32_t buttonFlags = (nsIPrompt::BUTTON_POS_0_DEFAULT |
                             (nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_0) |
                             (nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_1));

      nsAutoSyncOperation sync(mDocument);
      rv = prompt->ConfirmEx(title, message, buttonFlags,
                             leaveLabel, stayLabel, nullptr, nullptr,
                             &dummy, &buttonPressed);
      NS_ENSURE_SUCCESS(rv, rv);

      
      *aPermitUnload = (buttonPressed == 0);
    }
  }

  if (docShellNode) {
    int32_t childCount;
    docShellNode->GetChildCount(&childCount);

    for (int32_t i = 0; i < childCount && *aPermitUnload; ++i) {
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
    mCallerIsClosingWindow = true;

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::ResetCloseWindow()
{
  mCallerIsClosingWindow = false;

  nsCOMPtr<nsIDocShellTreeNode> docShellNode(do_QueryReferent(mContainer));
  if (docShellNode) {
    int32_t childCount;
    docShellNode->GetChildCount(&childCount);

    for (int32_t i = 0; i < childCount; ++i) {
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
nsDocumentViewer::PageHide(bool aIsUnload)
{
  mHidden = true;

  if (!mDocument) {
    return NS_ERROR_NULL_POINTER;
  }

  mDocument->OnPageHide(!aIsUnload, nullptr);

  
  NS_ENSURE_STATE(mDocument);
  nsPIDOMWindow *window = mDocument->GetWindow();
  if (window)
    window->PageHidden();

  if (aIsUnload) {
    
    nsJSContext::PokeGC(js::gcreason::PAGE_HIDE, NS_GC_DELAY * 2);

    
    NS_ENSURE_STATE(mDocument);

    
    nsPIDOMWindow *window = mDocument->GetWindow();

    if (!window) {
      
      NS_WARNING("window not set for document!");
      return NS_ERROR_NULL_POINTER;
    }

    
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event(true, NS_PAGE_UNLOAD);
    event.mFlags.mBubbles = false;
    
    event.target = mDocument;

    
    
    nsAutoPopupStatePusher popupStatePusher(openAbused, true);

    nsEventDispatcher::Dispatch(window, mPresContext, &event, nullptr, &status);
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
  if (viewer) {
    nsIDocument* doc = viewer->GetDocument();
    if (doc) {
      doc->SetContainer(aShell);
    }
    nsRefPtr<nsPresContext> pc;
    viewer->GetPresContext(getter_AddRefs(pc));
    if (pc) {
      pc->SetContainer(aShell);
      pc->SetLinkHandler(nsCOMPtr<nsILinkHandler>(do_QueryInterface(aShell)));
    }
    nsCOMPtr<nsIPresShell> presShell;
    viewer->GetPresShell(getter_AddRefs(presShell));
    if (presShell) {
      presShell->SetForwardingContainer(nullptr);
    }
  }

  
  int32_t childCount;
  aShell->GetChildCount(&childCount);
  for (int32_t i = 0; i < childCount; ++i) {
    nsCOMPtr<nsIDocShellTreeItem> childItem;
    aShell->GetChildAt(i, getter_AddRefs(childItem));
    AttachContainerRecurse(nsCOMPtr<nsIDocShell>(do_QueryInterface(childItem)));
  }
}

NS_IMETHODIMP
nsDocumentViewer::Open(nsISupports *aState, nsISHEntry *aSHEntry)
{
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);

  if (mDocument)
    mDocument->SetContainer(nsCOMPtr<nsISupports>(do_QueryReferent(mContainer)));

  nsresult rv = InitInternal(mParentWidget, aState, mBounds, false);
  NS_ENSURE_SUCCESS(rv, rv);

  mHidden = false;

  if (mPresShell)
    mPresShell->SetForwardingContainer(nullptr);

  
  

  if (aSHEntry) {
    nsCOMPtr<nsIDocShellTreeItem> item;
    int32_t itemIndex = 0;
    while (NS_SUCCEEDED(aSHEntry->ChildShellAt(itemIndex++,
                                               getter_AddRefs(item))) && item) {
      AttachContainerRecurse(nsCOMPtr<nsIDocShell>(do_QueryInterface(item)));
    }
  }
  
  SyncParentSubDocMap();

  if (mFocusListener && mDocument) {
    mDocument->AddEventListener(NS_LITERAL_STRING("focus"), mFocusListener,
                                false, false);
    mDocument->AddEventListener(NS_LITERAL_STRING("blur"), mFocusListener,
                                false, false);
  }

  

  PrepareToStartLoad();

  
  
  
  
  
  
  
  
  if (nsIWidget::UsePuppetWidgets() && mPresContext &&
      ShouldAttachToTopLevel()) {
    
    DetachFromTopLevelWidget();

    nsViewManager *vm = GetViewManager();
    NS_ABORT_IF_FALSE(vm, "no view manager");
    nsView* v = vm->GetRootView();
    NS_ABORT_IF_FALSE(v, "no root view");
    NS_ABORT_IF_FALSE(mParentWidget, "no mParentWidget to set");
    v->AttachToTopLevelWidget(mParentWidget);

    mAttachedToParent = true;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::Close(nsISHEntry *aSHEntry)
{
  
  
  
  

  mSHEntry = aSHEntry;

  
  
  
  
  

  if (!mDocument)
    return NS_OK;

#if defined(NS_PRINTING) && defined(NS_PRINT_PREVIEW)
  
  
  if (GetIsPrintPreview() && mPrintEngine) {
    mPrintEngine->TurnScriptingOn(true);
  }
#endif

#ifdef NS_PRINTING
  
  
  
  if (mPrintEngine && !mClosingWhilePrinting) {
    mClosingWhilePrinting = true;
  } else
#endif
    {
      
      mDocument->SetScriptGlobalObject(nullptr);

      if (!mSHEntry && mDocument)
        mDocument->RemovedFromDocShell();
    }

  if (mFocusListener && mDocument) {
    mDocument->RemoveEventListener(NS_LITERAL_STRING("focus"), mFocusListener,
                                   false);
    mDocument->RemoveEventListener(NS_LITERAL_STRING("blur"), mFocusListener,
                                   false);
  }

  return NS_OK;
}

static void
DetachContainerRecurse(nsIDocShell *aShell)
{
  
  nsCOMPtr<nsIContentViewer> viewer;
  aShell->GetContentViewer(getter_AddRefs(viewer));
  if (viewer) {
    nsIDocument* doc = viewer->GetDocument();
    if (doc) {
      doc->SetContainer(nullptr);
    }
    nsRefPtr<nsPresContext> pc;
    viewer->GetPresContext(getter_AddRefs(pc));
    if (pc) {
      pc->SetContainer(nullptr);
      pc->SetLinkHandler(nullptr);
    }
    nsCOMPtr<nsIPresShell> presShell;
    viewer->GetPresShell(getter_AddRefs(presShell));
    if (presShell) {
      presShell->SetForwardingContainer(nsWeakPtr(do_GetWeakReference(aShell)));
    }
  }

  
  int32_t childCount;
  aShell->GetChildCount(&childCount);
  for (int32_t i = 0; i < childCount; ++i) {
    nsCOMPtr<nsIDocShellTreeItem> childItem;
    aShell->GetChildAt(i, getter_AddRefs(childItem));
    DetachContainerRecurse(nsCOMPtr<nsIDocShell>(do_QueryInterface(childItem)));
  }
}

NS_IMETHODIMP
nsDocumentViewer::Destroy()
{
  NS_ASSERTION(mDocument, "No document in Destroy()!");

#ifdef NS_PRINTING
  
  
  
  
  
  
  
  if (mPrintEngine) {
    if (mPrintEngine->CheckBeforeDestroy()) {
      return NS_OK;
    }
  }
  mBeforeAndAfterPrint = nullptr;
#endif

  
  
  
  
  if (mDestroyRefCount != 0) {
    --mDestroyRefCount;
    return NS_OK;
  }

  
  
  if (mSHEntry) {
    if (mPresShell)
      mPresShell->Freeze();

    
    mSHEntry->SetSticky(mIsSticky);
    mIsSticky = true;

    bool savePresentation = mDocument ? mDocument->IsBFCachingAllowed() : true;

    
    if (mPresShell) {
      nsViewManager *vm = mPresShell->GetViewManager();
      if (vm) {
        nsView *rootView = vm->GetRootView();

        if (rootView) {
          
          
          
          vm->InvalidateViewNoSuppression(rootView,
            rootView->GetBounds() - rootView->GetPosition());

          nsView *rootViewParent = rootView->GetParent();
          if (rootViewParent) {
            nsViewManager *parentVM = rootViewParent->GetViewManager();
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
        
        
        savePresentation = false;
      }
    }


    
    

    
    
    nsCOMPtr<nsISHEntry> shEntry = mSHEntry; 
    mSHEntry = nullptr;

    if (savePresentation) {
      shEntry->SetContentViewer(this);
    }

    
    
    
    shEntry->SyncPresentationState();

    
    
    
    

    if (mDocument) {
      mDocument->SetContainer(nullptr);
    }
    if (mPresContext) {
      mPresContext->SetLinkHandler(nullptr);
      mPresContext->SetContainer(nullptr);
    }
    if (mPresShell)
      mPresShell->SetForwardingContainer(mContainer);

    
    
    nsCOMPtr<nsIDocShellTreeItem> item;
    int32_t itemIndex = 0;
    while (NS_SUCCEEDED(shEntry->ChildShellAt(itemIndex++,
                                              getter_AddRefs(item))) && item) {
      DetachContainerRecurse(nsCOMPtr<nsIDocShell>(do_QueryInterface(item)));
    }

    return NS_OK;
  }

  

  if (mPresShell) {
    DestroyPresShell();
  }
  if (mDocument) {
    mDocument->Destroy();
    mDocument = nullptr;
  }

  
  
  
  

#ifdef NS_PRINTING
  if (mPrintEngine) {
#ifdef NS_PRINT_PREVIEW
    bool doingPrintPreview;
    mPrintEngine->GetDoingPrintPreview(&doingPrintPreview);
    if (doingPrintPreview) {
      mPrintEngine->FinishPrintPreview();
    }
#endif

    mPrintEngine->Destroy();
    mPrintEngine = nullptr;
  }
#endif

  
  if (mPreviousViewer) {
    mPreviousViewer->Destroy();
    mPreviousViewer = nullptr;
  }

  mDeviceContext = nullptr;

  if (mPresContext) {
    DestroyPresContext();
  }

  mWindow = nullptr;
  mViewManager = nullptr;
  mContainer = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::Stop(void)
{
  NS_ASSERTION(mDocument, "Stop called too early or too late");
  if (mDocument) {
    mDocument->StopDocumentLoad();
  }

  if (!mHidden && (mLoaded || mStopped) && mPresContext && !mSHEntry)
    mPresContext->SetImageAnimationMode(imgIContainer::kDontAnimMode);

  mStopped = true;

  if (!mLoaded && mPresShell) {
    
    nsCOMPtr<nsIPresShell> shellDeathGrip(mPresShell); 
    mPresShell->UnsuppressPainting();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetDOMDocument(nsIDOMDocument **aResult)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  return CallQueryInterface(mDocument, aResult);
}

NS_IMETHODIMP_(nsIDocument *)
nsDocumentViewer::GetDocument()
{
  return mDocument;
}

NS_IMETHODIMP
nsDocumentViewer::SetDOMDocument(nsIDOMDocument *aDocument)
{
  
  
  
  
  

  
  
  
  
  

  if (!aDocument)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDocument> newDoc = do_QueryInterface(aDocument);
  NS_ENSURE_TRUE(newDoc, NS_ERROR_UNEXPECTED);

  return SetDocumentInternal(newDoc, false);
}

NS_IMETHODIMP
nsDocumentViewer::SetDocumentInternal(nsIDocument* aDocument,
                                        bool aForceReuseInnerWindow)
{
  MOZ_ASSERT(aDocument);

  
  nsCOMPtr<nsISupports> container = do_QueryReferent(mContainer);
  aDocument->SetContainer(container);

  if (mDocument != aDocument) {
    if (mDocument->IsStaticDocument()) {
      mDocument->SetScriptGlobalObject(nullptr);
      mDocument->Destroy();
    }
    
    
    mDocument = aDocument;

    
    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(container);
    if (window) {
      window->SetNewDocument(aDocument, nullptr, aForceReuseInnerWindow);
    }

    
    
    if (!aDocument->IsStaticDocument()) {
      nsCOMPtr<nsIDocShellTreeNode> node = do_QueryInterface(container);
      if (node) {
        int32_t count;
        node->GetChildCount(&count);
        for (int32_t i = 0; i < count; ++i) {
          nsCOMPtr<nsIDocShellTreeItem> child;
          node->GetChildAt(0, getter_AddRefs(child));
          node->RemoveChild(child);
        }
      }
    }
  }

  nsresult rv = SyncParentSubDocMap();
  NS_ENSURE_SUCCESS(rv, rv);

  

  if (mPresShell) {
    DestroyPresShell();
  }

  if (mPresContext) {
    DestroyPresContext();

    mWindow = nullptr;
    InitInternal(mParentWidget, nullptr, mBounds, true, true, false);
  }

  return rv;
}

nsIPresShell*
nsDocumentViewer::GetPresShell()
{
  return mPresShell;
}

nsPresContext*
nsDocumentViewer::GetPresContext()
{
  return mPresContext;
}

nsViewManager*
nsDocumentViewer::GetViewManager()
{
  return mViewManager;
}

NS_IMETHODIMP
nsDocumentViewer::GetPresShell(nsIPresShell** aResult)
{
  nsIPresShell* shell = GetPresShell();
  NS_IF_ADDREF(*aResult = shell);
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetPresContext(nsPresContext** aResult)
{
  nsPresContext* pc = GetPresContext();
  NS_IF_ADDREF(*aResult = pc);
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetBounds(nsIntRect& aResult)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  aResult = mBounds;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetPreviousViewer(nsIContentViewer** aViewer)
{
  *aViewer = mPreviousViewer;
  NS_IF_ADDREF(*aViewer);
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::SetPreviousViewer(nsIContentViewer* aViewer)
{
  
  

  if (aViewer) {
    NS_ASSERTION(!mPreviousViewer,
                 "can't set previous viewer when there already is one");

    
    
    
    
    
    
    
    
    
    
    nsCOMPtr<nsIContentViewer> prevViewer;
    aViewer->GetPreviousViewer(getter_AddRefs(prevViewer));
    if (prevViewer) {
      aViewer->SetPreviousViewer(nullptr);
      aViewer->Destroy();
      return SetPreviousViewer(prevViewer);
    }
  }

  mPreviousViewer = aViewer;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::SetBounds(const nsIntRect& aBounds)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);

  mBounds = aBounds;
  if (mWindow) {
    if (!mAttachedToParent) {
      
      
      mWindow->Resize(aBounds.x, aBounds.y,
                      aBounds.width, aBounds.height,
                      false);
    }
  } else if (mPresContext && mViewManager) {
    int32_t p2a = mPresContext->AppUnitsPerDevPixel();
    mViewManager->SetWindowDimensions(NSIntPixelsToAppUnits(mBounds.width, p2a),
                                      NSIntPixelsToAppUnits(mBounds.height, p2a));
  }

  
  
  
  
  
  
  
  if (mPreviousViewer) {
    nsCOMPtr<nsIContentViewer> previousViewer = mPreviousViewer;
    previousViewer->SetBounds(aBounds);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::Move(int32_t aX, int32_t aY)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  mBounds.MoveTo(aX, aY);
  if (mWindow) {
    mWindow->Move(aX, aY);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::Show(void)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);

  
  
  if (mPreviousViewer) {
    
    
    nsCOMPtr<nsIContentViewer> prevViewer(mPreviousViewer);
    mPreviousViewer = nullptr;
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
        int32_t prevIndex,loadedIndex;
        nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(treeItem);
        docShell->GetPreviousTransIndex(&prevIndex);
        docShell->GetLoadedTransIndex(&loadedIndex);
#ifdef DEBUG_PAGE_CACHE
        printf("About to evict content viewers: prev=%d, loaded=%d\n",
               prevIndex, loadedIndex);
#endif
        historyInt->EvictOutOfRangeContentViewers(loadedIndex);
      }
    }
  }

  if (mWindow) {
    
    
    
    if (!mAttachedToParent) {
      mWindow->Show(true);
    }
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

    nsView* containerView = FindContainerView();

    nsresult rv = CreateDeviceContext(containerView);
    NS_ENSURE_SUCCESS(rv, rv);

    
    NS_ASSERTION(!mPresContext, "Shouldn't have a prescontext if we have no shell!");
    mPresContext = CreatePresContext(mDocument,
        nsPresContext::eContext_Galley, containerView);
    NS_ENSURE_TRUE(mPresContext, NS_ERROR_OUT_OF_MEMORY);

    rv = mPresContext->Init(mDeviceContext);
    if (NS_FAILED(rv)) {
      mPresContext = nullptr;
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

      rv = InitPresentationStuff(mDocument->MayStartLayout());
    }

    
    
    

    if (mPresShell) {
      nsCOMPtr<nsIPresShell> shellDeathGrip(mPresShell); 
      mPresShell->UnsuppressPainting();
    }
  }

  
  
  NS_DispatchToMainThread(new nsDocumentShownDispatcher(mDocument));

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::Hide(void)
{
  if (!mAttachedToParent && mWindow) {
    mWindow->Show(false);
  }

  if (!mPresShell)
    return NS_OK;

  NS_ASSERTION(mPresContext, "Can't have a presshell and no prescontext!");

  
  if (mPreviousViewer) {
    mPreviousViewer->Destroy();
    mPreviousViewer = nullptr;
  }

  if (mIsSticky) {
    
    
    

    return NS_OK;
  }

  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mContainer));
  if (docShell) {
    nsCOMPtr<nsILayoutHistoryState> layoutState;
    mPresShell->CaptureHistoryState(getter_AddRefs(layoutState));
  }

  DestroyPresShell();

  DestroyPresContext();

  mViewManager   = nullptr;
  mWindow        = nullptr;
  mDeviceContext = nullptr;
  mParentWidget  = nullptr;

  nsCOMPtr<nsIBaseWindow> base_win(do_QueryReferent(mContainer));

  if (base_win && !mAttachedToParent) {
    base_win->SetParentWidget(nullptr);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetSticky(bool *aSticky)
{
  *aSticky = mIsSticky;

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::SetSticky(bool aSticky)
{
  mIsSticky = aSticky;

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::RequestWindowClose(bool* aCanClose)
{
#ifdef NS_PRINTING
  if (mPrintIsPending || (mPrintEngine && mPrintEngine->GetIsPrinting())) {
    *aCanClose = false;
    mDeferredWindowClose = true;
  } else
#endif
    *aCanClose = true;

  return NS_OK;
}

static bool
AppendAgentSheet(nsIStyleSheet *aSheet, void *aData)
{
  nsStyleSet *styleSet = static_cast<nsStyleSet*>(aData);
  styleSet->AppendStyleSheet(nsStyleSet::eAgentSheet, aSheet);
  return true;
}

static bool
PrependUserSheet(nsIStyleSheet *aSheet, void *aData)
{
  nsStyleSet *styleSet = static_cast<nsStyleSet*>(aData);
  styleSet->PrependStyleSheet(nsStyleSet::eUserSheet, aSheet);
  return true;
}

nsresult
nsDocumentViewer::CreateStyleSet(nsIDocument* aDocument,
                                   nsStyleSet** aStyleSet)
{
  

  
  
  nsStyleSet *styleSet = new nsStyleSet();

  styleSet->BeginUpdate();
  
  
  
  
  nsCSSStyleSheet* sheet = nullptr;
  if (nsContentUtils::IsInChromeDocshell(aDocument)) {
    sheet = nsLayoutStylesheetCache::UserChromeSheet();
  }
  else {
    sheet = nsLayoutStylesheetCache::UserContentSheet();
  }

  if (sheet)
    styleSet->AppendStyleSheet(nsStyleSet::eUserSheet, sheet);

  
  bool shouldOverride = false;
  
  
  nsCOMPtr<nsIDocShell> ds(do_QueryReferent(mContainer));
  nsCOMPtr<nsIDOMEventTarget> chromeHandler;
  nsCOMPtr<nsIURI> uri;
  nsRefPtr<nsCSSStyleSheet> csssheet;

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
        nsRefPtr<mozilla::css::Loader> cssLoader = new mozilla::css::Loader();

        char *str = ToNewCString(sheets);
        char *newStr = str;
        char *token;
        while ( (token = nsCRT::strtok(newStr, ", ", &newStr)) ) {
          NS_NewURI(getter_AddRefs(uri), nsDependentCString(token), nullptr,
                    baseURI);
          if (!uri) continue;

          cssLoader->LoadSheetSync(uri, getter_AddRefs(csssheet));
          if (!csssheet) continue;

          styleSet->PrependStyleSheet(nsStyleSet::eAgentSheet, csssheet);
          shouldOverride = true;
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

  sheet = nsLayoutStylesheetCache::FullScreenOverrideSheet();
  if (sheet) {
    styleSet->PrependStyleSheet(nsStyleSet::eOverrideSheet, sheet);
  }

  
  
  nsRefPtr<nsCSSStyleSheet> quirkClone;
  nsCSSStyleSheet* quirkSheet;
  if (!nsLayoutStylesheetCache::UASheet() ||
      !(quirkSheet = nsLayoutStylesheetCache::QuirkSheet()) ||
      !(quirkClone = quirkSheet->Clone(nullptr, nullptr, nullptr, nullptr)) ||
      !sheet) {
    delete styleSet;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  
  styleSet->PrependStyleSheet(nsStyleSet::eAgentSheet, quirkClone);
  styleSet->SetQuirkStyleSheet(quirkClone);
  styleSet->PrependStyleSheet(nsStyleSet::eAgentSheet,
                              nsLayoutStylesheetCache::UASheet());

  nsStyleSheetService *sheetService = nsStyleSheetService::GetInstance();
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
nsDocumentViewer::ClearHistoryEntry()
{
  mSHEntry = nullptr;
  return NS_OK;
}



nsresult
nsDocumentViewer::MakeWindow(const nsSize& aSize, nsView* aContainerView)
{
  if (GetIsPrintPreview())
    return NS_OK;

  bool shouldAttach = ShouldAttachToTopLevel();

  if (shouldAttach) {
    
    DetachFromTopLevelWidget();
  }

  mViewManager = new nsViewManager();

  nsDeviceContext *dx = mPresContext->DeviceContext();

  nsresult rv = mViewManager->Init(dx);
  if (NS_FAILED(rv))
    return rv;

  
  nsRect tbounds(nsPoint(0, 0), aSize);
  
  nsView* view = mViewManager->CreateView(tbounds, aContainerView);
  if (!view)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  
  
  if (!mDocument->IsResourceDoc() &&
      (mParentWidget || !aContainerView)) {
    
    
    
    nsWidgetInitData initData;
    nsWidgetInitData* initDataPtr;
    if (!mParentWidget) {
      initDataPtr = &initData;
      initData.mWindowType = eWindowType_invisible;
    } else {
      initDataPtr = nullptr;
    }

    if (shouldAttach) {
      
      rv = view->AttachToTopLevelWidget(mParentWidget);
      mAttachedToParent = true;
    }
    else if (!aContainerView && mParentWidget) {
      rv = view->CreateWidgetForParent(mParentWidget, initDataPtr,
                                       true, false);
    }
    else {
      rv = view->CreateWidget(initDataPtr, true, false);
    }
    if (NS_FAILED(rv))
      return rv;
  }

  
  mViewManager->SetRootView(view);

  mWindow = view->GetWidget();

  
  
  
  

  return rv;
}

void
nsDocumentViewer::DetachFromTopLevelWidget()
{
  if (mViewManager) {
    nsView* oldView = mViewManager->GetRootView();
    if (oldView && oldView->IsAttachedToTopLevel()) {
      oldView->DetachFromTopLevelWidget();
    }
  }
  mAttachedToParent = false;
}

nsView*
nsDocumentViewer::FindContainerView()
{
  nsView* containerView = nullptr;

  if (mContainer) {
    nsCOMPtr<nsIDocShellTreeItem> docShellItem = do_QueryReferent(mContainer);
    nsCOMPtr<nsPIDOMWindow> pwin(do_GetInterface(docShellItem));
    if (pwin) {
      nsCOMPtr<nsIContent> containerElement = do_QueryInterface(pwin->GetFrameElementInternal());
      if (!containerElement) {
        return nullptr;
      }
      nsCOMPtr<nsIPresShell> parentPresShell;
      if (docShellItem) {
        nsCOMPtr<nsIDocShellTreeItem> parentDocShellItem;
        docShellItem->GetParent(getter_AddRefs(parentDocShellItem));
        if (parentDocShellItem) {
          nsCOMPtr<nsIDocShell> parentDocShell = do_QueryInterface(parentDocShellItem);
          parentPresShell = parentDocShell->GetPresShell();
        }
      }
      if (!parentPresShell) {
        nsCOMPtr<nsIDocument> parentDoc = containerElement->GetCurrentDoc();
        if (parentDoc) {
          parentPresShell = parentDoc->GetShell();
        }
      }
      if (!parentPresShell) {
        NS_WARNING("Subdocument container has no presshell");
      } else {
        nsIFrame* f = parentPresShell->GetRealPrimaryFrameFor(containerElement);
        if (f) {
          nsIFrame* subdocFrame = f->GetContentInsertionFrame();
          
          
          
          
          if (subdocFrame->GetType() == nsGkAtoms::subDocumentFrame) {
            NS_ASSERTION(subdocFrame->GetView(), "Subdoc frames must have views");
            nsView* innerView =
              static_cast<nsSubDocumentFrame*>(subdocFrame)->EnsureInnerView();
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

  return containerView;
}

nsresult
nsDocumentViewer::CreateDeviceContext(nsView* aContainerView)
{
  NS_PRECONDITION(!mPresShell && !mWindow,
                  "This will screw up our existing presentation");
  NS_PRECONDITION(mDocument, "Gotta have a document here");
  
  nsIDocument* doc = mDocument->GetDisplayDocument();
  if (doc) {
    NS_ASSERTION(!aContainerView, "External resource document embedded somewhere?");
    
    nsIPresShell* shell = doc->GetShell();
    if (shell) {
      nsPresContext* ctx = shell->GetPresContext();
      if (ctx) {
        mDeviceContext = ctx->DeviceContext();
        return NS_OK;
      }
    }
  }
  
  
  
  nsIWidget* widget = nullptr;
  if (aContainerView) {
    widget = aContainerView->GetNearestWidget(nullptr);
  }
  if (!widget) {
    widget = mParentWidget;
  }
  if (widget) {
    widget = widget->GetTopLevelWidget();
  }

  mDeviceContext = new nsDeviceContext();
  mDeviceContext->Init(widget);
  return NS_OK;
}



nsresult nsDocumentViewer::GetDocumentSelection(nsISelection **aSelection)
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





NS_IMETHODIMP nsDocumentViewer::ClearSelection()
{
  nsresult rv;
  nsCOMPtr<nsISelection> selection;

  
  rv = GetDocumentSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;

  return selection->CollapseToStart();
}

NS_IMETHODIMP nsDocumentViewer::SelectAll()
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
    bodyNode = do_QueryInterface(mDocument->GetRootElement());
  }
  if (!bodyNode) return NS_ERROR_FAILURE;

  rv = selection->RemoveAllRanges();
  if (NS_FAILED(rv)) return rv;

  rv = selection->SelectAllChildren(bodyNode);
  return rv;
}

NS_IMETHODIMP nsDocumentViewer::CopySelection()
{
  nsCopySupport::FireClipboardEvent(NS_COPY, mPresShell, nullptr);
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::CopyLinkLocation()
{
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIDOMNode> node;
  GetPopupLinkNode(getter_AddRefs(node));
  
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  nsCOMPtr<dom::Element> elm(do_QueryInterface(node));
  NS_ENSURE_TRUE(elm, NS_ERROR_FAILURE);

  nsAutoString locationText;
  nsContentUtils::GetLinkLocation(elm, locationText);
  if (locationText.IsEmpty())
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  nsCOMPtr<nsIClipboardHelper> clipboard(do_GetService("@mozilla.org/widget/clipboardhelper;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(mDocument);
  return clipboard->CopyString(locationText, doc);
}

NS_IMETHODIMP nsDocumentViewer::CopyImage(int32_t aCopyFlags)
{
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIImageLoadingContent> node;
  GetPopupImageNode(getter_AddRefs(node));
  
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  nsCOMPtr<nsILoadContext> loadContext(do_QueryReferent(mContainer));
  return nsCopySupport::ImageCopy(node, loadContext, aCopyFlags);
}


NS_IMETHODIMP nsDocumentViewer::GetCopyable(bool *aCopyable)
{
  NS_ENSURE_ARG_POINTER(aCopyable);
  *aCopyable = nsCopySupport::CanCopy(mDocument);
  return NS_OK;
}


NS_IMETHODIMP nsDocumentViewer::GetContents(const char *mimeType, bool selectionOnly, nsAString& aOutValue)
{
  aOutValue.Truncate();

  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_INITIALIZED);

  
  nsCOMPtr<nsISelection> sel;
  if (selectionOnly) {
    nsCopySupport::GetSelectionForCopy(mDocument, getter_AddRefs(sel));
    NS_ENSURE_TRUE(sel, NS_ERROR_FAILURE);
  
    bool isCollapsed;
    sel->GetIsCollapsed(&isCollapsed);
    if (isCollapsed)
      return NS_OK;
  }

  
  return nsCopySupport::GetContents(nsDependentCString(mimeType), 0, sel,
                                    mDocument, aOutValue);
}


NS_IMETHODIMP nsDocumentViewer::GetCanGetContents(bool *aCanGetContents)
{
  NS_ENSURE_ARG_POINTER(aCanGetContents);
  *aCanGetContents = false;
  NS_ENSURE_STATE(mDocument);
  *aCanGetContents = nsCopySupport::CanCopy(mDocument);
  return NS_OK;
}









NS_IMETHODIMP
nsDocumentViewer::Print(bool              aSilent,
                          FILE *            aDebugFile,
                          nsIPrintSettings* aPrintSettings)
{
#ifdef NS_PRINTING
  nsCOMPtr<nsIPrintSettings> printSettings;

#ifdef DEBUG
  nsresult rv = NS_ERROR_FAILURE;

  mDebugFile = aDebugFile;
  
  
  printSettings = aPrintSettings;
  nsCOMPtr<nsIPrintOptions> printOptions = do_GetService(sPrintOptionsContractID, &rv);
  if (NS_SUCCEEDED(rv)) {
    
    if (printSettings == nullptr) {
      printOptions->CreatePrintSettings(getter_AddRefs(printSettings));
    }
    NS_ASSERTION(printSettings, "You can't PrintPreview without a PrintSettings!");
  }
  if (printSettings) printSettings->SetPrintSilent(aSilent);
  if (printSettings) printSettings->SetShowPrintProgress(false);
#endif


  return Print(printSettings, nullptr);
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP 
nsDocumentViewer::PrintWithParent(nsIDOMWindow*, nsIPrintSettings *aThePrintSettings, nsIWebProgressListener *aWPListener)
{
#ifdef NS_PRINTING
  return Print(aThePrintSettings, aWPListener);
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP
nsDocumentViewer::GetPrintable(bool *aPrintable)
{
  NS_ENSURE_ARG_POINTER(aPrintable);

  *aPrintable = !GetIsPrinting();

  return NS_OK;
}





NS_IMETHODIMP nsDocumentViewer::ScrollToNode(nsIDOMNode* aNode)
{
  NS_ENSURE_ARG(aNode);
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  nsCOMPtr<nsIPresShell> presShell;
  NS_ENSURE_SUCCESS(GetPresShell(getter_AddRefs(presShell)), NS_ERROR_FAILURE);

  
  

  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

  
  NS_ENSURE_SUCCESS(
    presShell->ScrollContentIntoView(content,
                                     nsIPresShell::ScrollAxis(
                                       nsIPresShell::SCROLL_TOP,
                                       nsIPresShell::SCROLL_ALWAYS),
                                     nsIPresShell::ScrollAxis(),
                                     nsIPresShell::SCROLL_OVERFLOW_HIDDEN),
    NS_ERROR_FAILURE);
  return NS_OK;
}

void
nsDocumentViewer::CallChildren(CallChildFunc aFunc, void* aClosure)
{
  nsCOMPtr<nsIDocShellTreeNode> docShellNode(do_QueryReferent(mContainer));
  if (docShellNode)
  {
    int32_t i;
    int32_t n;
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

struct LineBoxInfo
{
  nscoord mMaxLineBoxWidth;
};

static void
ChangeChildMaxLineBoxWidth(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  struct LineBoxInfo* lbi = (struct LineBoxInfo*) aClosure;
  aChild->ChangeMaxLineBoxWidth(lbi->mMaxLineBoxWidth);
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
SetChildMinFontSize(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  nsCOMPtr<nsIMarkupDocumentViewer> branch =
    do_QueryInterface(aChild);
  branch->SetMinFontSize(NS_PTR_TO_INT32(aClosure));
}

static void
SetChildFullZoom(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  struct ZoomInfo* ZoomInfo = (struct ZoomInfo*) aClosure;
  aChild->SetFullZoom(ZoomInfo->mZoom);
}

static bool
SetExtResourceTextZoom(nsIDocument* aDocument, void* aClosure)
{
  
  nsIPresShell* shell = aDocument->GetShell();
  if (shell) {
    nsPresContext* ctxt = shell->GetPresContext();
    if (ctxt) {
      struct ZoomInfo* ZoomInfo = static_cast<struct ZoomInfo*>(aClosure);
      ctxt->SetTextZoom(ZoomInfo->mZoom);
    }
  }

  return true;
}

static bool
SetExtResourceMinFontSize(nsIDocument* aDocument, void* aClosure)
{
  nsIPresShell* shell = aDocument->GetShell();
  if (shell) {
    nsPresContext* ctxt = shell->GetPresContext();
    if (ctxt) {
      ctxt->SetMinFontSize(NS_PTR_TO_INT32(aClosure));
    }
  }

  return true;
}

static bool
SetExtResourceFullZoom(nsIDocument* aDocument, void* aClosure)
{
  
  nsIPresShell* shell = aDocument->GetShell();
  if (shell) {
    nsPresContext* ctxt = shell->GetPresContext();
    if (ctxt) {
      struct ZoomInfo* ZoomInfo = static_cast<struct ZoomInfo*>(aClosure);
      ctxt->SetFullZoom(ZoomInfo->mZoom);
    }
  }

  return true;
}

NS_IMETHODIMP
nsDocumentViewer::SetTextZoom(float aTextZoom)
{
  if (GetIsPrintPreview()) {
    return NS_OK;
  }

  mTextZoom = aTextZoom;

  
  
  
  
  struct ZoomInfo ZoomInfo = { aTextZoom };
  CallChildren(SetChildTextZoom, &ZoomInfo);

  
  nsPresContext* pc = GetPresContext();
  if (pc && aTextZoom != mPresContext->TextZoom()) {
      pc->SetTextZoom(aTextZoom);
  }

  
  mDocument->EnumerateExternalResources(SetExtResourceTextZoom, &ZoomInfo);

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetTextZoom(float* aTextZoom)
{
  NS_ENSURE_ARG_POINTER(aTextZoom);
  nsPresContext* pc = GetPresContext();
  *aTextZoom = pc ? pc->TextZoom() : 1.0f;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::SetMinFontSize(int32_t aMinFontSize)
{
  if (GetIsPrintPreview()) {
    return NS_OK;
  }

  mMinFontSize = aMinFontSize;

  
  
  
  
  CallChildren(SetChildMinFontSize, NS_INT32_TO_PTR(aMinFontSize));

  
  nsPresContext* pc = GetPresContext();
  if (pc && aMinFontSize != mPresContext->MinFontSize(nullptr)) {
    pc->SetMinFontSize(aMinFontSize);
  }

  
  mDocument->EnumerateExternalResources(SetExtResourceMinFontSize,
                                        NS_INT32_TO_PTR(aMinFontSize));

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetMinFontSize(int32_t* aMinFontSize)
{
  NS_ENSURE_ARG_POINTER(aMinFontSize);
  nsPresContext* pc = GetPresContext();
  *aMinFontSize = pc ? pc->MinFontSize(nullptr) : 0;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::SetFullZoom(float aFullZoom)
{
#ifdef NS_PRINT_PREVIEW
  if (GetIsPrintPreview()) {
    nsPresContext* pc = GetPresContext();
    NS_ENSURE_TRUE(pc, NS_OK);
    nsCOMPtr<nsIPresShell> shell = pc->GetPresShell();
    NS_ENSURE_TRUE(shell, NS_OK);

    if (!mPrintPreviewZoomed) {
      mOriginalPrintPreviewScale = pc->GetPrintPreviewScale();
      mPrintPreviewZoomed = true;
    }

    mPrintPreviewZoom = aFullZoom;
    pc->SetPrintPreviewScale(aFullZoom * mOriginalPrintPreviewScale);
    nsIPageSequenceFrame* pf = shell->GetPageSequenceFrame();
    if (pf) {
      nsIFrame* f = do_QueryFrame(pf);
      shell->FrameNeedsReflow(f, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);
    }

    nsIFrame* rootFrame = shell->GetRootFrame();
    if (rootFrame) {
      rootFrame->InvalidateFrame();
    }
    return NS_OK;
  }
#endif

  mPageZoom = aFullZoom;

  struct ZoomInfo ZoomInfo = { aFullZoom };
  CallChildren(SetChildFullZoom, &ZoomInfo);

  nsPresContext* pc = GetPresContext();
  if (pc) {
    pc->SetFullZoom(aFullZoom);
  }

  
  mDocument->EnumerateExternalResources(SetExtResourceFullZoom, &ZoomInfo);

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetFullZoom(float* aFullZoom)
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
  bool styleDisabled  = *static_cast<bool*>(aClosure);
  aChild->SetAuthorStyleDisabled(styleDisabled);
}


NS_IMETHODIMP
nsDocumentViewer::SetAuthorStyleDisabled(bool aStyleDisabled)
{
  if (mPresShell) {
    mPresShell->SetAuthorStyleDisabled(aStyleDisabled);
  }
  CallChildren(SetChildAuthorStyleDisabled, &aStyleDisabled);
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetAuthorStyleDisabled(bool* aStyleDisabled)
{
  if (mPresShell) {
    *aStyleDisabled = mPresShell->GetAuthorStyleDisabled();
  } else {
    *aStyleDisabled = false;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetDefaultCharacterSet(nsACString& aDefaultCharacterSet)
{
  if (mDefaultCharacterSet.IsEmpty())
  {
    const nsAdoptingCString& defCharset =
      Preferences::GetLocalizedCString("intl.charset.default");

    if (!defCharset.IsEmpty()) {
      mDefaultCharacterSet = defCharset;
    } else {
      mDefaultCharacterSet.AssignLiteral("ISO-8859-1");
    }
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
nsDocumentViewer::SetDefaultCharacterSet(const nsACString& aDefaultCharacterSet)
{
  mDefaultCharacterSet = aDefaultCharacterSet;  
  
  CallChildren(SetChildDefaultCharacterSet, (void*) &aDefaultCharacterSet);
  return NS_OK;
}




NS_IMETHODIMP nsDocumentViewer::GetForceCharacterSet(nsACString& aForceCharacterSet)
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
nsDocumentViewer::SetForceCharacterSet(const nsACString& aForceCharacterSet)
{
  mForceCharacterSet = aForceCharacterSet;
  
  CallChildren(SetChildForceCharacterSet, (void*) &aForceCharacterSet);
  return NS_OK;
}




NS_IMETHODIMP nsDocumentViewer::GetHintCharacterSet(nsACString& aHintCharacterSet)
{

  if(kCharsetUninitialized == mHintCharsetSource) {
    aHintCharacterSet.Truncate();
  } else {
    aHintCharacterSet = mHintCharset;
    
    
  }
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::GetHintCharacterSetSource(int32_t *aHintCharacterSetSource)
{
  NS_ENSURE_ARG_POINTER(aHintCharacterSetSource);

  *aHintCharacterSetSource = mHintCharsetSource;
  return NS_OK;
}


NS_IMETHODIMP nsDocumentViewer::GetPrevDocCharacterSet(nsACString& aPrevDocCharacterSet)
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
nsDocumentViewer::SetPrevDocCharacterSet(const nsACString& aPrevDocCharacterSet)
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
nsDocumentViewer::SetHintCharacterSetSource(int32_t aHintCharacterSetSource)
{
  mHintCharsetSource = aHintCharacterSetSource;
  
  CallChildren(SetChildHintCharacterSetSource,
                      NS_INT32_TO_PTR(aHintCharacterSetSource));
  return NS_OK;
}

static void
SetChildHintCharacterSet(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  const nsACString* charset = static_cast<nsACString*>(aClosure);
  aChild->SetHintCharacterSet(*charset);
}

NS_IMETHODIMP
nsDocumentViewer::SetHintCharacterSet(const nsACString& aHintCharacterSet)
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

NS_IMETHODIMP nsDocumentViewer::SetBidiTextDirection(uint8_t aTextDirection)
{
  uint32_t bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_DIRECTION(bidiOptions, aTextDirection);
  SetBidiOptions(bidiOptions);
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::GetBidiTextDirection(uint8_t* aTextDirection)
{
  uint32_t bidiOptions;

  if (aTextDirection) {
    GetBidiOptions(&bidiOptions);
    *aTextDirection = GET_BIDI_OPTION_DIRECTION(bidiOptions);
  }
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::SetBidiTextType(uint8_t aTextType)
{
  uint32_t bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_TEXTTYPE(bidiOptions, aTextType);
  SetBidiOptions(bidiOptions);
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::GetBidiTextType(uint8_t* aTextType)
{
  uint32_t bidiOptions;

  if (aTextType) {
    GetBidiOptions(&bidiOptions);
    *aTextType = GET_BIDI_OPTION_TEXTTYPE(bidiOptions);
  }
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::SetBidiNumeral(uint8_t aNumeral)
{
  uint32_t bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_NUMERAL(bidiOptions, aNumeral);
  SetBidiOptions(bidiOptions);
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::GetBidiNumeral(uint8_t* aNumeral)
{
  uint32_t bidiOptions;

  if (aNumeral) {
    GetBidiOptions(&bidiOptions);
    *aNumeral = GET_BIDI_OPTION_NUMERAL(bidiOptions);
  }
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::SetBidiSupport(uint8_t aSupport)
{
  uint32_t bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_SUPPORT(bidiOptions, aSupport);
  SetBidiOptions(bidiOptions);
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::GetBidiSupport(uint8_t* aSupport)
{
  uint32_t bidiOptions;

  if (aSupport) {
    GetBidiOptions(&bidiOptions);
    *aSupport = GET_BIDI_OPTION_SUPPORT(bidiOptions);
  }
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::SetBidiOptions(uint32_t aBidiOptions)
{
  if (mPresContext) {
    mPresContext->SetBidi(aBidiOptions, true); 
  }
  
  CallChildren(SetChildBidiOptions, NS_INT32_TO_PTR(aBidiOptions));
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::GetBidiOptions(uint32_t* aBidiOptions)
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

static void
AppendChildSubtree(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  nsTArray<nsCOMPtr<nsIMarkupDocumentViewer> >& array =
    *static_cast<nsTArray<nsCOMPtr<nsIMarkupDocumentViewer> >*>(aClosure);
  aChild->AppendSubtree(array);
}

NS_IMETHODIMP nsDocumentViewer::AppendSubtree(nsTArray<nsCOMPtr<nsIMarkupDocumentViewer> >& aArray)
{
  aArray.AppendElement(this);
  CallChildren(AppendChildSubtree, &aArray);
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::ChangeMaxLineBoxWidth(int32_t aMaxLineBoxWidth)
{
  
  struct LineBoxInfo lbi = { aMaxLineBoxWidth };
  CallChildren(ChangeChildMaxLineBoxWidth, &lbi);

  
  
  nscoord mlbw = nsPresContext::CSSPixelsToAppUnits(aMaxLineBoxWidth);
  nsIPresShell* presShell = GetPresShell();
  if (presShell) {
    presShell->SetMaxLineBoxWidth(mlbw);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetContentSize(int32_t* aWidth, int32_t* aHeight)
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
    nsRefPtr<nsRenderingContext> rcx =
      presShell->GetReferenceRenderingContext();
    NS_ENSURE_TRUE(rcx, NS_ERROR_FAILURE);
    prefWidth = root->GetPrefWidth(rcx);
  }

  nsresult rv = presShell->ResizeReflow(prefWidth, NS_UNCONSTRAINEDSIZE);
  NS_ENSURE_SUCCESS(rv, rv);

   nsRefPtr<nsPresContext> presContext;
   GetPresContext(getter_AddRefs(presContext));
   NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

   
   nsRect shellArea = presContext->GetVisibleArea();
   
   NS_ENSURE_TRUE(shellArea.width != NS_UNCONSTRAINEDSIZE &&
                  shellArea.height != NS_UNCONSTRAINEDSIZE,
                  NS_ERROR_FAILURE);

   *aWidth = presContext->AppUnitsToDevPixels(shellArea.width);
   *aHeight = presContext->AppUnitsToDevPixels(shellArea.height);

   return NS_OK;
}


NS_IMPL_ISUPPORTS1(nsDocViewerSelectionListener, nsISelectionListener)

nsresult nsDocViewerSelectionListener::Init(nsDocumentViewer *aDocViewer)
{
  mDocViewer = aDocViewer;
  return NS_OK;
}










nsresult
nsDocumentViewer::GetPopupNode(nsIDOMNode** aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);

  *aNode = nullptr;

  
  nsIDocument* document = GetDocument();
  NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsPIDOMWindow> window(document->GetWindow());
  NS_ENSURE_TRUE(window, NS_ERROR_NOT_AVAILABLE);
  if (window) {
    nsCOMPtr<nsPIWindowRoot> root = window->GetTopWindowRoot();
    NS_ENSURE_TRUE(root, NS_ERROR_FAILURE);

    
    nsCOMPtr<nsIDOMNode> node = root->GetPopupNode();
#ifdef MOZ_XUL
    if (!node) {
      nsPIDOMWindow* rootWindow = root->GetWindow();
      if (rootWindow) {
        nsCOMPtr<nsIDocument> rootDoc = do_QueryInterface(rootWindow->GetExtantDocument());
        if (rootDoc) {
          nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
          if (pm) {
            node = pm->GetLastTriggerPopupNode(rootDoc);
          }
        }
      }
    }
#endif
    node.swap(*aNode);
  }

  return NS_OK;
}


nsresult
nsDocumentViewer::GetPopupLinkNode(nsIDOMNode** aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);

  
  *aNode = nullptr;

  
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupNode(getter_AddRefs(node));
  NS_ENSURE_SUCCESS(rv, rv);

  
  while (node) {

    nsCOMPtr<nsIContent> content(do_QueryInterface(node));
    if (content) {
      nsCOMPtr<nsIURI> hrefURI = content->GetHrefURI();
      if (hrefURI) {
        *aNode = node;
        NS_IF_ADDREF(*aNode); 
        return NS_OK;
      }
    }

    
    nsCOMPtr<nsIDOMNode> parentNode;
    node->GetParentNode(getter_AddRefs(parentNode));
    node = parentNode;
  }

  
  return NS_ERROR_FAILURE;
}


nsresult
nsDocumentViewer::GetPopupImageNode(nsIImageLoadingContent** aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);

  
  *aNode = nullptr;

  
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupNode(getter_AddRefs(node));
  NS_ENSURE_SUCCESS(rv, rv);

  if (node)
    CallQueryInterface(node, aNode);

  return NS_OK;
}












NS_IMETHODIMP nsDocumentViewer::GetInLink(bool* aInLink)
{
#ifdef DEBUG_dr
  printf("dr :: nsDocumentViewer::GetInLink\n");
#endif

  NS_ENSURE_ARG_POINTER(aInLink);

  
  *aInLink = false;

  
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupLinkNode(getter_AddRefs(node));
  if (NS_FAILED(rv)) return rv;
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  
  *aInLink = true;
  return NS_OK;
}

NS_IMETHODIMP nsDocumentViewer::GetInImage(bool* aInImage)
{
#ifdef DEBUG_dr
  printf("dr :: nsDocumentViewer::GetInImage\n");
#endif

  NS_ENSURE_ARG_POINTER(aInImage);

  
  *aInImage = false;

  
  nsCOMPtr<nsIImageLoadingContent> node;
  nsresult rv = GetPopupImageNode(getter_AddRefs(node));
  if (NS_FAILED(rv)) return rv;
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  
  *aInImage = true;
  return NS_OK;
}

NS_IMETHODIMP nsDocViewerSelectionListener::NotifySelectionChanged(nsIDOMDocument *, nsISelection *, int16_t)
{
  NS_ASSERTION(mDocViewer, "Should have doc viewer!");

  
  nsCOMPtr<nsISelection> selection;
  nsresult rv = mDocViewer->GetDocumentSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;

  bool selectionCollapsed;
  selection->GetIsCollapsed(&selectionCollapsed);
  
  
  
  if (!mGotSelectionState || mSelectionWasCollapsed != selectionCollapsed)
  {
    nsIDocument* theDoc = mDocViewer->GetDocument();
    if (!theDoc) return NS_ERROR_FAILURE;

    nsPIDOMWindow *domWindow = theDoc->GetWindow();
    if (!domWindow) return NS_ERROR_FAILURE;

    domWindow->UpdateCommands(NS_LITERAL_STRING("select"));
    mGotSelectionState = true;
    mSelectionWasCollapsed = selectionCollapsed;
  }

  return NS_OK;
}


NS_IMPL_ISUPPORTS1(nsDocViewerFocusListener,
                   nsIDOMEventListener)

nsDocViewerFocusListener::nsDocViewerFocusListener()
:mDocViewer(nullptr)
{
}

nsDocViewerFocusListener::~nsDocViewerFocusListener(){}

nsresult
nsDocViewerFocusListener::HandleEvent(nsIDOMEvent* aEvent)
{
  NS_ENSURE_STATE(mDocViewer);

  nsCOMPtr<nsIPresShell> shell;
  mDocViewer->GetPresShell(getter_AddRefs(shell));
  NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(shell);
  int16_t selectionStatus;
  selCon->GetDisplaySelection(&selectionStatus);

  nsAutoString eventType;
  aEvent->GetType(eventType);
  if (eventType.EqualsLiteral("focus")) {
    
    if(selectionStatus == nsISelectionController::SELECTION_DISABLED ||
       selectionStatus == nsISelectionController::SELECTION_HIDDEN) {
      selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
      selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
    }
  } else {
    NS_ABORT_IF_FALSE(eventType.EqualsLiteral("blur"),
                      "Unexpected event type");
    
    if(selectionStatus == nsISelectionController::SELECTION_ON ||
       selectionStatus == nsISelectionController::SELECTION_ATTENTION) {
      selCon->SetDisplaySelection(nsISelectionController::SELECTION_DISABLED);
      selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
    }
  }

  return NS_OK;
}

nsresult
nsDocViewerFocusListener::Init(nsDocumentViewer *aDocViewer)
{
  mDocViewer = aDocViewer;
  return NS_OK;
}





#ifdef NS_PRINTING

NS_IMETHODIMP
nsDocumentViewer::Print(nsIPrintSettings*       aPrintSettings,
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
  NS_ENSURE_STATE(docShell);

  
  
  
  uint32_t busyFlags = nsIDocShell::BUSY_FLAGS_NONE;
  if ((NS_FAILED(docShell->GetBusyFlags(&busyFlags)) ||
       (busyFlags != nsIDocShell::BUSY_FLAGS_NONE && busyFlags & nsIDocShell::BUSY_FLAGS_PAGE_LOADING)) && 
      !mPrintDocIsFullyLoaded) {
    if (!mPrintIsPending) {
      mCachedPrintSettings           = aPrintSettings;
      mCachedPrintWebProgressListner = aWebProgressListener;
      mPrintIsPending                = true;
    }
    PR_PL(("Printing Stopped - document is still busy!"));
    return NS_ERROR_GFX_PRINTER_DOC_IS_BUSY;
  }

  if (!mDocument || !mDeviceContext) {
    PR_PL(("Can't Print without a document and a device context"));
    return NS_ERROR_FAILURE;
  }

  nsresult rv;

  
  
  
  
  if (GetIsPrinting()) {
    
    rv = NS_ERROR_NOT_AVAILABLE;
    nsPrintEngine::ShowPrintErrorDialog(rv);
    return rv;
  }

  nsAutoPtr<nsPrintEventDispatcher> beforeAndAfterPrint(
    new nsPrintEventDispatcher(mDocument));
  NS_ENSURE_STATE(!GetIsPrinting());
  
  
  nsCOMPtr<nsIPluginDocument> pDoc(do_QueryInterface(mDocument));
  if (pDoc)
    return pDoc->Print();

  if (!mPrintEngine) {
    NS_ENSURE_STATE(mDeviceContext);
    mPrintEngine = new nsPrintEngine();

    rv = mPrintEngine->Initialize(this, mContainer, mDocument, 
                                  float(mDeviceContext->AppUnitsPerCSSInch()) /
                                  float(mDeviceContext->AppUnitsPerDevPixel()) /
                                  mPageZoom,
#ifdef DEBUG
                                  mDebugFile
#else
                                  nullptr
#endif
                                  );
    if (NS_FAILED(rv)) {
      mPrintEngine->Destroy();
      mPrintEngine = nullptr;
      return rv;
    }
  }
  if (mPrintEngine->HasPrintCallbackCanvas()) {
    mBeforeAndAfterPrint = beforeAndAfterPrint;
  }
  dom::Element* root = mDocument->GetRootElement();
  if (root && root->HasAttr(kNameSpaceID_None, nsGkAtoms::mozdisallowselectionprint)) {
    mPrintEngine->SetDisallowSelectionPrint(true);
  }
  rv = mPrintEngine->Print(aPrintSettings, aWebProgressListener);
  if (NS_FAILED(rv)) {
    OnDonePrinting();
  }
  return rv;
}

NS_IMETHODIMP
nsDocumentViewer::PrintPreview(nsIPrintSettings* aPrintSettings, 
                                 nsIDOMWindow *aChildDOMWin, 
                                 nsIWebProgressListener* aWebProgressListener)
{
#if defined(NS_PRINTING) && defined(NS_PRINT_PREVIEW)
  NS_WARN_IF_FALSE(IsInitializedForPrintPreview(),
                   "Using docshell.printPreview is the preferred way for print previewing!");

  NS_ENSURE_ARG_POINTER(aChildDOMWin);
  nsresult rv = NS_OK;

  if (GetIsPrinting()) {
    nsPrintEngine::CloseProgressDialog(aWebProgressListener);
    return NS_ERROR_FAILURE;
  }

#ifdef MOZ_XUL
  
  nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocument));
  if (xulDoc) {
    nsPrintEngine::CloseProgressDialog(aWebProgressListener);
    nsPrintEngine::ShowPrintErrorDialog(NS_ERROR_GFX_PRINTER_NO_XUL, false);
    return NS_ERROR_FAILURE;
  }
#endif

  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mContainer));
  if (!docShell || !mDeviceContext) {
    PR_PL(("Can't Print Preview without device context and docshell"));
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMDocument> domDoc;
  aChildDOMWin->GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  NS_ENSURE_STATE(doc);

  nsAutoPtr<nsPrintEventDispatcher> beforeAndAfterPrint(
    new nsPrintEventDispatcher(doc));
  NS_ENSURE_STATE(!GetIsPrinting());
  if (!mPrintEngine) {
    mPrintEngine = new nsPrintEngine();

    rv = mPrintEngine->Initialize(this, mContainer, doc,
                                  float(mDeviceContext->AppUnitsPerCSSInch()) /
                                  float(mDeviceContext->AppUnitsPerDevPixel()) /
                                  mPageZoom,
#ifdef DEBUG
                                  mDebugFile
#else
                                  nullptr
#endif
                                  );
    if (NS_FAILED(rv)) {
      mPrintEngine->Destroy();
      mPrintEngine = nullptr;
      return rv;
    }
  }
  if (mPrintEngine->HasPrintCallbackCanvas()) {
    mBeforeAndAfterPrint = beforeAndAfterPrint;
  }
  rv = mPrintEngine->PrintPreview(aPrintSettings, aChildDOMWin, aWebProgressListener);
  mPrintPreviewZoomed = false;
  if (NS_FAILED(rv)) {
    OnDonePrinting();
  }
  return rv;
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP
nsDocumentViewer::PrintPreviewNavigate(int16_t aType, int32_t aPageNum)
{
  if (!GetIsPrintPreview() ||
      mPrintEngine->GetIsCreatingPrintPreview())
    return NS_ERROR_FAILURE;

  nsIScrollableFrame* sf =
    mPrintEngine->GetPrintPreviewPresShell()->GetRootScrollFrameAsScrollable();
  if (!sf)
    return NS_OK;

  
  if (aType == nsIWebBrowserPrint::PRINTPREVIEW_HOME ||
      (aType == nsIWebBrowserPrint::PRINTPREVIEW_GOTO_PAGENUM && aPageNum == 1)) {
    sf->ScrollTo(nsPoint(0, 0), nsIScrollableFrame::INSTANT);
    return NS_OK;
  }

  
  
  nsIFrame* seqFrame  = nullptr;
  int32_t   pageCount = 0;
  if (NS_FAILED(mPrintEngine->GetSeqFrameAndCountPages(seqFrame, pageCount))) {
    return NS_ERROR_FAILURE;
  }

  
  nsPoint pt = sf->GetScrollPosition();

  int32_t    pageNum = 1;
  nsIFrame * fndPageFrame  = nullptr;
  nsIFrame * currentPage   = nullptr;

  
  if (aType == nsIWebBrowserPrint::PRINTPREVIEW_END) {
    aType    = nsIWebBrowserPrint::PRINTPREVIEW_GOTO_PAGENUM;
    aPageNum = pageCount;
  }

  
  
  nsIFrame* pageFrame = seqFrame->GetFirstPrincipalChild();
  while (pageFrame != nullptr) {
    nsRect pageRect = pageFrame->GetRect();
    if (pageRect.Contains(pageRect.x, pt.y)) {
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

  if (fndPageFrame) {
    nscoord newYPosn =
      nscoord(mPrintEngine->GetPrintPreviewScale() * fndPageFrame->GetPosition().y);
    sf->ScrollTo(nsPoint(pt.x, newYPosn), nsIScrollableFrame::INSTANT);
  }
  return NS_OK;

}


NS_IMETHODIMP
nsDocumentViewer::GetGlobalPrintSettings(nsIPrintSettings * *aGlobalPrintSettings)
{
  return nsPrintEngine::GetGlobalPrintSettings(aGlobalPrintSettings);
}



NS_IMETHODIMP
nsDocumentViewer::GetDoingPrint(bool *aDoingPrint)
{
  NS_ENSURE_ARG_POINTER(aDoingPrint);
  
  *aDoingPrint = false;
  if (mPrintEngine) {
    
    return mPrintEngine->GetDoingPrintPreview(aDoingPrint);
  } 
  return NS_OK;
}



NS_IMETHODIMP
nsDocumentViewer::GetDoingPrintPreview(bool *aDoingPrintPreview)
{
  NS_ENSURE_ARG_POINTER(aDoingPrintPreview);

  *aDoingPrintPreview = false;
  if (mPrintEngine) {
    return mPrintEngine->GetDoingPrintPreview(aDoingPrintPreview);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsDocumentViewer::GetCurrentPrintSettings(nsIPrintSettings * *aCurrentPrintSettings)
{
  NS_ENSURE_ARG_POINTER(aCurrentPrintSettings);

  *aCurrentPrintSettings = nullptr;
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  return mPrintEngine->GetCurrentPrintSettings(aCurrentPrintSettings);
}



NS_IMETHODIMP 
nsDocumentViewer::GetCurrentChildDOMWindow(nsIDOMWindow * *aCurrentChildDOMWindow)
{
  NS_ENSURE_ARG_POINTER(aCurrentChildDOMWindow);
  *aCurrentChildDOMWindow = nullptr;
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDocumentViewer::Cancel()
{
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);
  return mPrintEngine->Cancelled();
}


NS_IMETHODIMP
nsDocumentViewer::ExitPrintPreview()
{
  if (GetIsPrinting())
    return NS_ERROR_FAILURE;
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  if (GetIsPrintPreview()) {
    ReturnToGalleyPresentation();
  }
  return NS_OK;
}



NS_IMETHODIMP
nsDocumentViewer::EnumerateDocumentNames(uint32_t* aCount,
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
nsDocumentViewer::GetIsFramesetFrameSelected(bool *aIsFramesetFrameSelected)
{
#ifdef NS_PRINTING
  *aIsFramesetFrameSelected = false;
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  return mPrintEngine->GetIsFramesetFrameSelected(aIsFramesetFrameSelected);
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP
nsDocumentViewer::GetPrintPreviewNumPages(int32_t *aPrintPreviewNumPages)
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
nsDocumentViewer::GetIsFramesetDocument(bool *aIsFramesetDocument)
{
#ifdef NS_PRINTING
  *aIsFramesetDocument = false;
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  return mPrintEngine->GetIsFramesetDocument(aIsFramesetDocument);
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP 
nsDocumentViewer::GetIsIFrameSelected(bool *aIsIFrameSelected)
{
#ifdef NS_PRINTING
  *aIsIFrameSelected = false;
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  return mPrintEngine->GetIsIFrameSelected(aIsIFrameSelected);
#else
  return NS_ERROR_FAILURE;
#endif
}


NS_IMETHODIMP 
nsDocumentViewer::GetIsRangeSelection(bool *aIsRangeSelection)
{
#ifdef NS_PRINTING
  *aIsRangeSelection = false;
  NS_ENSURE_TRUE(mPrintEngine, NS_ERROR_FAILURE);

  return mPrintEngine->GetIsRangeSelection(aIsRangeSelection);
#else
  return NS_ERROR_FAILURE;
#endif
}







void 
nsDocumentViewer::SetIsPrintingInDocShellTree(nsIDocShellTreeNode* aParentNode, 
                                                bool                 aIsPrintingOrPP, 
                                                bool                 aStartAtTop)
{
  nsCOMPtr<nsIDocShellTreeItem> parentItem(do_QueryInterface(aParentNode));

  
  if (aStartAtTop) {
    if (aIsPrintingOrPP) {
      while (parentItem) {
        nsCOMPtr<nsIDocShellTreeItem> parent;
        parentItem->GetSameTypeParent(getter_AddRefs(parent));
        if (!parent) {
          break;
        }
        parentItem = do_QueryInterface(parent);
      }
      mTopContainerWhilePrinting = do_GetWeakReference(parentItem);
    } else {
      parentItem = do_QueryReferent(mTopContainerWhilePrinting);
    }
  }

  
  nsCOMPtr<nsIContentViewerContainer> viewerContainer(do_QueryInterface(parentItem));
  if (viewerContainer) {
    viewerContainer->SetIsPrinting(aIsPrintingOrPP);
  }

  if (!aParentNode) {
    return;
  }

  
  int32_t n;
  aParentNode->GetChildCount(&n);
  for (int32_t i=0; i < n; i++) {
    nsCOMPtr<nsIDocShellTreeItem> child;
    aParentNode->GetChildAt(i, getter_AddRefs(child));
    nsCOMPtr<nsIDocShellTreeNode> childAsNode(do_QueryInterface(child));
    NS_ASSERTION(childAsNode, "child isn't nsIDocShellTreeNode");
    if (childAsNode) {
      SetIsPrintingInDocShellTree(childAsNode, aIsPrintingOrPP, false);
    }
  }

}
#endif 

bool
nsDocumentViewer::ShouldAttachToTopLevel()
{
  if (!mParentWidget)
    return false;

  nsCOMPtr<nsIDocShellTreeItem> containerItem = do_QueryReferent(mContainer);
  if (!containerItem)
    return false;

  
  if (nsIWidget::UsePuppetWidgets())
    return true;

#if defined(XP_WIN) || defined(MOZ_WIDGET_GTK)
  
  
  int32_t docType;
  nsWindowType winType;
  containerItem->GetItemType(&docType);
  mParentWidget->GetWindowType(winType);
  if ((winType == eWindowType_toplevel ||
       winType == eWindowType_dialog ||
       winType == eWindowType_invisible) &&
      docType == nsIDocShellTreeItem::typeChrome)
    return true;
#endif

  return false;
}

bool CollectDocuments(nsIDocument* aDocument, void* aData)
{
  if (aDocument) {
    static_cast<nsCOMArray<nsIDocument>*>(aData)->AppendObject(aDocument);
    aDocument->EnumerateSubDocuments(CollectDocuments, aData);
  }
  return true;
}

void
nsDocumentViewer::DispatchEventToWindowTree(nsIDocument* aDoc,
                                              const nsAString& aEvent)
{
  nsCOMArray<nsIDocument> targets;
  CollectDocuments(aDoc, &targets);
  for (int32_t i = 0; i < targets.Count(); ++i) {
    nsIDocument* d = targets[i];
    nsContentUtils::DispatchTrustedEvent(d, d->GetWindow(),
                                         aEvent, false, false, nullptr);
  }
}



bool
nsDocumentViewer::GetIsPrinting()
{
#ifdef NS_PRINTING
  if (mPrintEngine) {
    return mPrintEngine->GetIsPrinting();
  }
#endif
  return false; 
}



void
nsDocumentViewer::SetIsPrinting(bool aIsPrinting)
{
#ifdef NS_PRINTING
  
  
  nsCOMPtr<nsIDocShellTreeNode> docShellTreeNode(do_QueryReferent(mContainer));
  if (docShellTreeNode || !aIsPrinting) {
    SetIsPrintingInDocShellTree(docShellTreeNode, aIsPrinting, true);
  } else {
    NS_WARNING("Did you close a window before printing?");
  }

  if (!aIsPrinting) {
    mBeforeAndAfterPrint = nullptr;
  }
#endif
}





bool
nsDocumentViewer::GetIsPrintPreview()
{
#ifdef NS_PRINTING
  if (mPrintEngine) {
    return mPrintEngine->GetIsPrintPreview();
  }
#endif
  return false; 
}



void
nsDocumentViewer::SetIsPrintPreview(bool aIsPrintPreview)
{
#ifdef NS_PRINTING
  
  
  nsCOMPtr<nsIDocShellTreeNode> docShellTreeNode(do_QueryReferent(mContainer));
  if (docShellTreeNode || !aIsPrintPreview) {
    SetIsPrintingInDocShellTree(docShellTreeNode, aIsPrintPreview, true);
  }
  if (!aIsPrintPreview) {
    mBeforeAndAfterPrint = nullptr;
  }
#endif
  if (!aIsPrintPreview) {
    if (mPresShell) {
      DestroyPresShell();
    }
    mWindow = nullptr;
    mViewManager = nullptr;
    mPresContext = nullptr;
    mPresShell = nullptr;
  }
}






void
nsDocumentViewer::IncrementDestroyRefCount()
{
  ++mDestroyRefCount;
}



#if defined(NS_PRINTING) && defined(NS_PRINT_PREVIEW)


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
  bool hasMoreDocShells;
  while (NS_SUCCEEDED(docShellEnumerator->HasMoreElements(&hasMoreDocShells))
         && hasMoreDocShells) {
    docShellEnumerator->GetNext(getter_AddRefs(currentContainer));
    nsCOMPtr<nsIDOMWindow> win = do_GetInterface(currentContainer);
    if (win)
      fm->ClearFocus(win);
  }
}
#endif 

void
nsDocumentViewer::ReturnToGalleyPresentation()
{
#if defined(NS_PRINTING) && defined(NS_PRINT_PREVIEW)
  if (!GetIsPrintPreview()) {
    NS_ERROR("Wow, we should never get here!");
    return;
  }

  SetIsPrintPreview(false);

  mPrintEngine->TurnScriptingOn(true);
  mPrintEngine->Destroy();
  mPrintEngine = nullptr;

  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mContainer));
  ResetFocusState(docShell);

  SetTextZoom(mTextZoom);
  SetFullZoom(mPageZoom);
  SetMinFontSize(mMinFontSize);
  Show();

#endif 
}













void
nsDocumentViewer::OnDonePrinting() 
{
#if defined(NS_PRINTING) && defined(NS_PRINT_PREVIEW)
  if (mPrintEngine) {
    nsRefPtr<nsPrintEngine> pe = mPrintEngine;
    if (GetIsPrintPreview()) {
      pe->DestroyPrintingData();
    } else {
      mPrintEngine = nullptr;
      pe->Destroy();
    }

    
    if (mDeferredWindowClose) {
      mDeferredWindowClose = false;
      nsCOMPtr<nsISupports> container = do_QueryReferent(mContainer);
      nsCOMPtr<nsIDOMWindow> win = do_GetInterface(container);
      if (win)
        win->Close();
    } else if (mClosingWhilePrinting) {
      if (mDocument) {
        mDocument->SetScriptGlobalObject(nullptr);
        mDocument->Destroy();
        mDocument = nullptr;
      }
      mClosingWhilePrinting = false;
    }
  }
#endif 
}

NS_IMETHODIMP nsDocumentViewer::SetPageMode(bool aPageMode, nsIPrintSettings* aPrintSettings)
{
  
  
  mIsPageMode = aPageMode;

  if (mPresShell) {
    DestroyPresShell();
  }

  if (mPresContext) {
    DestroyPresContext();
  }

  mViewManager  = nullptr;
  mWindow       = nullptr;

  NS_ENSURE_STATE(mDocument);
  if (aPageMode)
  {    
    mPresContext = CreatePresContext(mDocument,
        nsPresContext::eContext_PageLayout, FindContainerView());
    NS_ENSURE_TRUE(mPresContext, NS_ERROR_OUT_OF_MEMORY);
    mPresContext->SetPaginatedScrolling(true);
    mPresContext->SetPrintSettings(aPrintSettings);
    nsresult rv = mPresContext->Init(mDeviceContext);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  InitInternal(mParentWidget, nullptr, mBounds, true, false);

  Show();
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetHistoryEntry(nsISHEntry **aHistoryEntry)
{
  NS_IF_ADDREF(*aHistoryEntry = mSHEntry);
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetIsTabModalPromptAllowed(bool *aAllowed)
{
  *aAllowed = !(mInPermitUnload || mHidden);
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentViewer::GetIsHidden(bool *aHidden)
{
  *aHidden = mHidden;
  return NS_OK;
}

void
nsDocumentViewer::DestroyPresShell()
{
  
  mPresShell->EndObservingDocument();

  nsCOMPtr<nsISelection> selection;
  GetDocumentSelection(getter_AddRefs(selection));
  nsCOMPtr<nsISelectionPrivate> selPrivate = do_QueryInterface(selection);
  if (selPrivate && mSelectionListener)
    selPrivate->RemoveSelectionListener(mSelectionListener);

  nsAutoScriptBlocker scriptBlocker;
  mPresShell->Destroy();
  mPresShell = nullptr;
}

void
nsDocumentViewer::DestroyPresContext()
{
  mPresContext->SetContainer(nullptr);
  mPresContext->SetLinkHandler(nullptr);
  mPresContext = nullptr;
}

bool
nsDocumentViewer::IsInitializedForPrintPreview()
{
  return mInitializedForPrintPreview;
}

void
nsDocumentViewer::InitializeForPrintPreview()
{
  mInitializedForPrintPreview = true;
}

void
nsDocumentViewer::SetPrintPreviewPresentation(nsViewManager* aViewManager,
                                                nsPresContext* aPresContext,
                                                nsIPresShell* aPresShell)
{
  if (mPresShell) {
    DestroyPresShell();
  }

  mWindow = nullptr;
  mViewManager = aViewManager;
  mPresContext = aPresContext;
  mPresShell = aPresShell;
}


NS_IMETHODIMP
nsDocumentShownDispatcher::Run()
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->NotifyObservers(mDocument, "document-shown", NULL);
  }
  return NS_OK;
}


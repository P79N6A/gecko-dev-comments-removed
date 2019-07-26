




#include "nsPrintEngine.h"

#include "nsIStringBundle.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"

#include "nsISelection.h"
#include "nsIScriptGlobalObject.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIFrame.h"
#include "nsIURI.h"
#include "nsITextToSubURI.h"
#include "nsError.h"

#include "nsView.h"
#include "nsAsyncDOMEvent.h"


#include "nsIPrintSettings.h"
#include "nsIPrintSettingsService.h"
#include "nsIPrintOptions.h"
#include "nsIPrintSession.h"
#include "nsGfxCIID.h"
#include "nsIServiceManager.h"
#include "nsGkAtoms.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"

static const char sPrintSettingsServiceContractID[] = "@mozilla.org/gfx/printsettings-service;1";


#include "nsPrintPreviewListener.h"
#include "nsThreadUtils.h"


#include "nsIWebBrowserPrint.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMHTMLFrameSetElement.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLEmbedElement.h"


#include "imgIContainer.h" 
#include "nsIWebBrowserPrint.h" 


#include "nsIPrintProgress.h"
#include "nsIPrintProgressParams.h"
#include "nsIObserver.h"


#include "nsIPrompt.h"
#include "nsIWindowWatcher.h"


#include "nsIPrintingPromptService.h"
static const char kPrintingPromptService[] = "@mozilla.org/embedcomp/printingprompt-service;1";


#include "nsPagePrintTimer.h"


#include "nsIDocument.h"


#include "nsIDOMEventTarget.h"
#include "nsISelectionController.h"


#include "nsISupportsUtils.h"
#include "nsIScriptContext.h"
#include "nsIDOMDocument.h"
#include "nsISelectionListener.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMRange.h"
#include "nsContentCID.h"
#include "nsLayoutCID.h"
#include "nsContentUtils.h"
#include "nsIPresShell.h"
#include "nsLayoutUtils.h"
#include "mozilla/Preferences.h"

#include "nsViewsCID.h"
#include "nsWidgetsCID.h"
#include "nsIDeviceContextSpec.h"
#include "nsIViewManager.h"
#include "nsIView.h"
#include "nsRenderingContext.h"

#include "nsIPageSequenceFrame.h"
#include "nsIURL.h"
#include "nsIContentViewerEdit.h"
#include "nsIContentViewerFile.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIWebBrowserChrome.h"
#include "nsIBaseWindow.h"
#include "nsILayoutHistoryState.h"
#include "nsFrameManager.h"
#include "nsGUIEvent.h"
#include "nsHTMLReflowState.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIContentViewerContainer.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewerPrint.h"

#include "nsFocusManager.h"
#include "nsRange.h"
#include "nsCDefaultURIFixup.h"
#include "nsIURIFixup.h"
#include "mozilla/dom/Element.h"
#include "nsContentList.h"

using namespace mozilla;
using namespace mozilla::dom;



#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "prlog.h"

#ifdef PR_LOGGING

#ifdef DEBUG



#endif

#define DUMP_LAYOUT_LEVEL 9 // this turns on the dumping of each doucment's layout info

static PRLogModuleInfo * kPrintingLogMod = PR_NewLogModule("printing");
#define PR_PL(_p1)  PR_LOG(kPrintingLogMod, PR_LOG_DEBUG, _p1);

#ifdef EXTENDED_DEBUG_PRINTING
static uint32_t gDumpFileNameCnt   = 0;
static uint32_t gDumpLOFileNameCnt = 0;
#endif

#define PRT_YESNO(_p) ((_p)?"YES":"NO")
static const char * gFrameTypesStr[]       = {"eDoc", "eFrame", "eIFrame", "eFrameSet"};
static const char * gPrintFrameTypeStr[]   = {"kNoFrames", "kFramesAsIs", "kSelectedFrame", "kEachFrameSep"};
static const char * gFrameHowToEnableStr[] = {"kFrameEnableNone", "kFrameEnableAll", "kFrameEnableAsIsAndEach"};
static const char * gPrintRangeStr[]       = {"kRangeAllPages", "kRangeSpecifiedPageRange", "kRangeSelection", "kRangeFocusFrame"};
#else
#define PRT_YESNO(_p)
#define PR_PL(_p1)
#endif

#ifdef EXTENDED_DEBUG_PRINTING

static void DumpPrintObjectsListStart(const char * aStr, nsTArray<nsPrintObject*> * aDocList);
static void DumpPrintObjectsTree(nsPrintObject * aPO, int aLevel= 0, FILE* aFD = nullptr);
static void DumpPrintObjectsTreeLayout(nsPrintObject * aPO,nsDeviceContext * aDC, int aLevel= 0, FILE * aFD = nullptr);

#define DUMP_DOC_LIST(_title) DumpPrintObjectsListStart((_title), mPrt->mPrintDocList);
#define DUMP_DOC_TREE DumpPrintObjectsTree(mPrt->mPrintObject);
#define DUMP_DOC_TREELAYOUT DumpPrintObjectsTreeLayout(mPrt->mPrintObject, mPrt->mPrintDC);
#else
#define DUMP_DOC_LIST(_title)
#define DUMP_DOC_TREE
#define DUMP_DOC_TREELAYOUT
#endif

class nsScriptSuppressor
{
public:
  nsScriptSuppressor(nsPrintEngine* aPrintEngine)
  : mPrintEngine(aPrintEngine), mSuppressed(false) {}

  ~nsScriptSuppressor() { Unsuppress(); }

  void Suppress()
  {
    if (mPrintEngine) {
      mSuppressed = true;
      mPrintEngine->TurnScriptingOn(false);
    }
  }
  
  void Unsuppress()
  {
    if (mPrintEngine && mSuppressed) {
      mPrintEngine->TurnScriptingOn(true);
    }
    mSuppressed = false;
  }

  void Disconnect() { mPrintEngine = nullptr; }
protected:
  nsRefPtr<nsPrintEngine> mPrintEngine;
  bool                    mSuppressed;
};


static NS_DEFINE_CID(kViewManagerCID,       NS_VIEW_MANAGER_CID);

NS_IMPL_ISUPPORTS3(nsPrintEngine, nsIWebProgressListener,
                   nsISupportsWeakReference, nsIObserver)




nsPrintEngine::nsPrintEngine() :
  mIsCreatingPrintPreview(false),
  mIsDoingPrinting(false),
  mIsDoingPrintPreview(false),
  mProgressDialogIsShown(false),
  mScreenDPI(115.0f),
  mPrt(nullptr),
  mPagePrintTimer(nullptr),
  mPageSeqFrame(nullptr),
  mPrtPreview(nullptr),
  mOldPrtPreview(nullptr),
  mDebugFile(nullptr),
  mLoadCounter(0),
  mDidLoadDataForPrinting(false)
{
}


nsPrintEngine::~nsPrintEngine()
{
  Destroy(); 
}


void nsPrintEngine::Destroy()
{
  if (mPrt) {
    delete mPrt;
    mPrt = nullptr;
  }

#ifdef NS_PRINT_PREVIEW
  if (mPrtPreview) {
    delete mPrtPreview;
    mPrtPreview = nullptr;
  }

  
  if (mOldPrtPreview) {
    delete mOldPrtPreview;
    mOldPrtPreview = nullptr;
  }

#endif
  mDocViewerPrint = nullptr;
}


void nsPrintEngine::DestroyPrintingData()
{
  if (mPrt) {
    delete mPrt;
    mPrt = nullptr;
  }
}






nsresult nsPrintEngine::Initialize(nsIDocumentViewerPrint* aDocViewerPrint, 
                                   nsIWeakReference*       aContainer,
                                   nsIDocument*            aDocument,
                                   float                   aScreenDPI,
                                   FILE*                   aDebugFile)
{
  NS_ENSURE_ARG_POINTER(aDocViewerPrint);
  NS_ENSURE_ARG_POINTER(aContainer);
  NS_ENSURE_ARG_POINTER(aDocument);

  mDocViewerPrint = aDocViewerPrint;
  mContainer      = aContainer;
  mDocument       = aDocument;
  mScreenDPI      = aScreenDPI;

  mDebugFile      = aDebugFile;      

  return NS_OK;
}


bool
nsPrintEngine::CheckBeforeDestroy()
{
  if (mPrt && mPrt->mPreparingForPrint) {
    mPrt->mDocWasToBeDestroyed = true;
    return true;
  }
  return false;
}


nsresult
nsPrintEngine::Cancelled()
{
  if (mPrt && mPrt->mPrintSettings) {
    return mPrt->mPrintSettings->SetIsCancelled(true);
  }
  return NS_ERROR_FAILURE;
}






void
nsPrintEngine::InstallPrintPreviewListener()
{
  if (!mPrt->mPPEventListeners) {
    nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mContainer);
    nsCOMPtr<nsPIDOMWindow> win(do_GetInterface(docShell));
    if (win) {
      nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(win->GetFrameElementInternal()));
      mPrt->mPPEventListeners = new nsPrintPreviewListener(target);
      mPrt->mPPEventListeners->AddListeners();
    }
  }
}


nsresult 
nsPrintEngine::GetSeqFrameAndCountPagesInternal(nsPrintObject*  aPO,
                                                nsIFrame*&    aSeqFrame,
                                                int32_t&      aCount)
{
  NS_ENSURE_ARG_POINTER(aPO);

  
  nsIPageSequenceFrame* seqFrame = aPO->mPresShell->GetPageSequenceFrame();
  if (seqFrame) {
    aSeqFrame = do_QueryFrame(seqFrame);
  } else {
    aSeqFrame = nullptr;
  }
  if (aSeqFrame == nullptr) return NS_ERROR_FAILURE;

  
  aCount = 0;
  nsIFrame* pageFrame = aSeqFrame->GetFirstPrincipalChild();
  while (pageFrame != nullptr) {
    aCount++;
    pageFrame = pageFrame->GetNextSibling();
  }

  return NS_OK;

}


nsresult nsPrintEngine::GetSeqFrameAndCountPages(nsIFrame*& aSeqFrame, int32_t& aCount)
{
  NS_ASSERTION(mPrtPreview, "mPrtPreview can't be null!");
  return GetSeqFrameAndCountPagesInternal(mPrtPreview->mPrintObject, aSeqFrame, aCount);
}










#ifdef EXTENDED_DEBUG_PRINTING
static int RemoveFilesInDir(const char * aDir);
static void GetDocTitleAndURL(nsPrintObject* aPO, char *& aDocStr, char *& aURLStr);
static void DumpPrintObjectsTree(nsPrintObject * aPO, int aLevel, FILE* aFD);
static void DumpPrintObjectsList(nsTArray<nsPrintObject*> * aDocList);
static void RootFrameList(nsPresContext* aPresContext, FILE* out, int32_t aIndent);
static void DumpViews(nsIDocShell* aDocShell, FILE* out);
static void DumpLayoutData(char* aTitleStr, char* aURLStr,
                           nsPresContext* aPresContext,
                           nsDeviceContext * aDC, nsIFrame * aRootFrame,
                           nsIDocShell * aDocShell, FILE* aFD);
#endif



nsresult
nsPrintEngine::CommonPrint(bool                    aIsPrintPreview,
                           nsIPrintSettings*       aPrintSettings,
                           nsIWebProgressListener* aWebProgressListener,
                           nsIDOMDocument* aDoc) {
  nsresult rv = DoCommonPrint(aIsPrintPreview, aPrintSettings,
                              aWebProgressListener, aDoc);
  if (NS_FAILED(rv)) {
    if (aIsPrintPreview) {
      SetIsCreatingPrintPreview(false);
      SetIsPrintPreview(false);
    } else {
      SetIsPrinting(false);
    }
    if (mProgressDialogIsShown)
      CloseProgressDialog(aWebProgressListener);
    if (rv != NS_ERROR_ABORT && rv != NS_ERROR_OUT_OF_MEMORY)
      ShowPrintErrorDialog(rv, !aIsPrintPreview);
    delete mPrt;
    mPrt = nullptr;
  }

  return rv;
}

nsresult
nsPrintEngine::DoCommonPrint(bool                    aIsPrintPreview,
                             nsIPrintSettings*       aPrintSettings,
                             nsIWebProgressListener* aWebProgressListener,
                             nsIDOMDocument*         aDoc)
{
  nsresult rv;

  if (aIsPrintPreview) {
    
    
    nsCOMPtr<nsIPrintingPromptService> pps(do_QueryInterface(aWebProgressListener));
    mProgressDialogIsShown = pps != nullptr;

    if (mIsDoingPrintPreview) {
      mOldPrtPreview = mPrtPreview;
      mPrtPreview = nullptr;
    }
  } else {
    mProgressDialogIsShown = false;
  }

  mPrt = new nsPrintData(aIsPrintPreview ? nsPrintData::eIsPrintPreview :
                                           nsPrintData::eIsPrinting);
  NS_ENSURE_TRUE(mPrt, NS_ERROR_OUT_OF_MEMORY);

  
  mPrt->mPrintSettings = aPrintSettings;
  if (!mPrt->mPrintSettings) {
    rv = GetGlobalPrintSettings(getter_AddRefs(mPrt->mPrintSettings));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = CheckForPrinters(mPrt->mPrintSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  mPrt->mPrintSettings->SetIsCancelled(false);
  mPrt->mPrintSettings->GetShrinkToFit(&mPrt->mShrinkToFit);

  if (aIsPrintPreview) {
    SetIsCreatingPrintPreview(true);
    SetIsPrintPreview(true);
    nsCOMPtr<nsIMarkupDocumentViewer> viewer =
      do_QueryInterface(mDocViewerPrint);
    if (viewer) {
      viewer->SetTextZoom(1.0f);
      viewer->SetFullZoom(1.0f);
      viewer->SetMinFontSize(0);
    }
  }

  
  
  
  
  nsCOMPtr<nsIPrintSession> printSession;
  if (!aIsPrintPreview) {
    printSession = do_CreateInstance("@mozilla.org/gfx/printsession;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    mPrt->mPrintSettings->SetPrintSession(printSession);
  }

  if (aWebProgressListener != nullptr) {
    mPrt->mPrintProgressListeners.AppendObject(aWebProgressListener);
  }

  
  
  
  mPrt->mCurrentFocusWin = FindFocusedDOMWindow();

  
  bool isSelection = IsThereARangeSelection(mPrt->mCurrentFocusWin);

  
  nsCOMPtr<nsIDocShell> webContainer(do_QueryReferent(mContainer, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  mPrt->mPrintObject = new nsPrintObject();
  NS_ENSURE_TRUE(mPrt->mPrintObject, NS_ERROR_OUT_OF_MEMORY);
  rv = mPrt->mPrintObject->Init(webContainer, aDoc, aIsPrintPreview);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(mPrt->mPrintDocList.AppendElement(mPrt->mPrintObject),
                 NS_ERROR_OUT_OF_MEMORY);

  mPrt->mIsParentAFrameSet = IsParentAFrameSet(webContainer);
  mPrt->mPrintObject->mFrameType = mPrt->mIsParentAFrameSet ? eFrameSet : eDoc;

  
  nsCOMPtr<nsIDocShellTreeNode> parentAsNode =
    do_QueryInterface(mPrt->mPrintObject->mDocShell);
  BuildDocTree(parentAsNode, &mPrt->mPrintDocList, mPrt->mPrintObject);

  if (!aIsPrintPreview) {
    SetIsPrinting(true);
  }

  
  if (!mPrt->mPrintObject->mDocument ||
      !mPrt->mPrintObject->mDocument->GetRootElement())
    return NS_ERROR_GFX_PRINTER_STARTDOC;

  
  
  MapContentToWebShells(mPrt->mPrintObject, mPrt->mPrintObject);

  mPrt->mIsIFrameSelected = IsThereAnIFrameSelected(webContainer, mPrt->mCurrentFocusWin, mPrt->mIsParentAFrameSet);

  
  if (mPrt->mIsParentAFrameSet) {
    if (mPrt->mCurrentFocusWin) {
      mPrt->mPrintSettings->SetHowToEnableFrameUI(nsIPrintSettings::kFrameEnableAll);
    } else {
      mPrt->mPrintSettings->SetHowToEnableFrameUI(nsIPrintSettings::kFrameEnableAsIsAndEach);
    }
  } else {
    mPrt->mPrintSettings->SetHowToEnableFrameUI(nsIPrintSettings::kFrameEnableNone);
  }
  
  mPrt->mPrintSettings->SetPrintOptions(nsIPrintSettings::kEnableSelectionRB, isSelection || mPrt->mIsIFrameSelected);

  nsCOMPtr<nsIDeviceContextSpec> devspec
    (do_CreateInstance("@mozilla.org/gfx/devicecontextspec;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsScriptSuppressor scriptSuppressor(this);
  if (!aIsPrintPreview) {
#ifdef DEBUG
    mPrt->mDebugFilePtr = mDebugFile;
#endif

    scriptSuppressor.Suppress();
    bool printSilently;
    mPrt->mPrintSettings->GetPrintSilent(&printSilently);

    
    printSilently =
      Preferences::GetBool("print.always_print_silent", printSilently);

    
    
    
    if (!printSilently) {
      nsCOMPtr<nsIPrintingPromptService> printPromptService(do_GetService(kPrintingPromptService));
      if (printPromptService) {
        nsIDOMWindow *domWin = mDocument->GetWindow(); 
        NS_ENSURE_TRUE(domWin, NS_ERROR_FAILURE);

        
        
        
        
        
        
        nsCOMPtr<nsIWebBrowserPrint> wbp(do_QueryInterface(mDocViewerPrint));
        rv = printPromptService->ShowPrintDialog(domWin, wbp,
                                                 mPrt->mPrintSettings);
        
        
        
        

        if (NS_SUCCEEDED(rv)) {
          
          
          printSilently = true;

          if (mPrt && mPrt->mPrintSettings) {
            
            mPrt->mPrintSettings->GetShrinkToFit(&mPrt->mShrinkToFit);
          }
        } else if (rv == NS_ERROR_NOT_IMPLEMENTED) {
          
          
          
          rv = NS_OK;
        }
      } else {
        rv = NS_ERROR_GFX_NO_PRINTROMPTSERVICE;
      }
    } else {
      
      rv = mPrt->mPrintSettings->SetupSilentPrinting();
    }
    
    if (rv == NS_ERROR_ABORT) 
      return rv;
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = devspec->Init(nullptr, mPrt->mPrintSettings, aIsPrintPreview);
  NS_ENSURE_SUCCESS(rv, rv);

  mPrt->mPrintDC = new nsDeviceContext();
  rv = mPrt->mPrintDC->InitForPrinting(devspec);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aIsPrintPreview) {
    mPrt->mPrintSettings->SetPrintFrameType(nsIPrintSettings::kFramesAsIs);

    
    
    mPrt->mPrintSettings->SetPrintRange(nsIPrintSettings::kRangeAllPages);
  } else {
    
    
    
    
    
    
    
    
    int16_t printFrameTypeUsage = nsIPrintSettings::kUseSettingWhenPossible;
    mPrt->mPrintSettings->GetPrintFrameTypeUsage(&printFrameTypeUsage);

    
    if (printFrameTypeUsage == nsIPrintSettings::kUseSettingWhenPossible) {
      
      int16_t printFrameType = nsIPrintSettings::kEachFrameSep;
      mPrt->mPrintSettings->GetPrintFrameType(&printFrameType);

      
      
      if (printFrameType == nsIPrintSettings::kNoFrames) {
        mPrt->mPrintFrameType = nsIPrintSettings::kEachFrameSep;
        mPrt->mPrintSettings->SetPrintFrameType(mPrt->mPrintFrameType);
      } else {
        
        
        int16_t howToEnableFrameUI;
        mPrt->mPrintSettings->GetHowToEnableFrameUI(&howToEnableFrameUI);
        if (howToEnableFrameUI != nsIPrintSettings::kFrameEnableNone) {
          switch (howToEnableFrameUI) {
          case nsIPrintSettings::kFrameEnableAll:
            mPrt->mPrintFrameType = printFrameType;
            break;

          case nsIPrintSettings::kFrameEnableAsIsAndEach:
            if (printFrameType != nsIPrintSettings::kSelectedFrame) {
              mPrt->mPrintFrameType = printFrameType;
            } else { 
              mPrt->mPrintFrameType = nsIPrintSettings::kEachFrameSep;
            }
            break;
          } 
          mPrt->mPrintSettings->SetPrintFrameType(mPrt->mPrintFrameType);
        }
      }
    } else {
      mPrt->mPrintSettings->GetPrintFrameType(&mPrt->mPrintFrameType);
    }
  }

  if (mPrt->mPrintFrameType == nsIPrintSettings::kEachFrameSep) {
    CheckForChildFrameSets(mPrt->mPrintObject);
  }

  if (NS_FAILED(EnablePOsForPrinting())) {
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIWebProgress> webProgress = do_QueryInterface(mPrt->mPrintObject->mDocShell);
  webProgress->AddProgressListener(
    static_cast<nsIWebProgressListener*>(this),
    nsIWebProgress::NOTIFY_STATE_REQUEST);

  mLoadCounter = 0;
  mDidLoadDataForPrinting = false;

  if (aIsPrintPreview) {
    bool notifyOnInit = false;
    ShowPrintProgress(false, notifyOnInit);

    
    TurnScriptingOn(false);

    if (!notifyOnInit) {
      InstallPrintPreviewListener();
      rv = InitPrintDocConstruction(false);
    } else {
      rv = NS_OK;
    }
  } else {
    bool doNotify;
    ShowPrintProgress(true, doNotify);
    if (!doNotify) {
      
      mPrt->OnStartPrinting();

      rv = InitPrintDocConstruction(false);
    }
  }

  
  scriptSuppressor.Disconnect();

  return NS_OK;
}


NS_IMETHODIMP
nsPrintEngine::Print(nsIPrintSettings*       aPrintSettings,
                     nsIWebProgressListener* aWebProgressListener)
{
  
  
  
  nsCOMPtr<nsIDOMDocument> doc =
    do_QueryInterface(mPrtPreview && mPrtPreview->mPrintObject ?
                        mPrtPreview->mPrintObject->mDocument : mDocument);

  return CommonPrint(false, aPrintSettings, aWebProgressListener, doc);
}

NS_IMETHODIMP
nsPrintEngine::PrintPreview(nsIPrintSettings* aPrintSettings, 
                                 nsIDOMWindow *aChildDOMWin, 
                                 nsIWebProgressListener* aWebProgressListener)
{
  
  
  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mContainer));
  NS_ENSURE_STATE(docShell);

  uint32_t busyFlags = nsIDocShell::BUSY_FLAGS_NONE;
  if (NS_FAILED(docShell->GetBusyFlags(&busyFlags)) ||
      busyFlags != nsIDocShell::BUSY_FLAGS_NONE) {
    CloseProgressDialog(aWebProgressListener);
    ShowPrintErrorDialog(NS_ERROR_GFX_PRINTER_DOC_IS_BUSY_PP, false);
    return NS_ERROR_FAILURE;
  }

  NS_ENSURE_STATE(aChildDOMWin);
  nsCOMPtr<nsIDOMDocument> doc;
  aChildDOMWin->GetDocument(getter_AddRefs(doc));
  NS_ENSURE_STATE(doc);

  
  return CommonPrint(true, aPrintSettings, aWebProgressListener, doc);
}



NS_IMETHODIMP
nsPrintEngine::GetIsFramesetDocument(bool *aIsFramesetDocument)
{
  nsCOMPtr<nsIDocShell> webContainer(do_QueryReferent(mContainer));
  *aIsFramesetDocument = IsParentAFrameSet(webContainer);
  return NS_OK;
}



NS_IMETHODIMP 
nsPrintEngine::GetIsIFrameSelected(bool *aIsIFrameSelected)
{
  *aIsIFrameSelected = false;

  
  nsCOMPtr<nsIDocShell> webContainer(do_QueryReferent(mContainer));
  
  nsCOMPtr<nsIDOMWindow> currentFocusWin = FindFocusedDOMWindow();
  if (currentFocusWin && webContainer) {
    
    
    
    bool isParentFrameSet;
    *aIsIFrameSelected = IsThereAnIFrameSelected(webContainer, currentFocusWin, isParentFrameSet);
  }
  return NS_OK;
}



NS_IMETHODIMP 
nsPrintEngine::GetIsRangeSelection(bool *aIsRangeSelection)
{
  
  nsCOMPtr<nsIDOMWindow> currentFocusWin = FindFocusedDOMWindow();
  *aIsRangeSelection = IsThereARangeSelection(currentFocusWin);
  return NS_OK;
}



NS_IMETHODIMP 
nsPrintEngine::GetIsFramesetFrameSelected(bool *aIsFramesetFrameSelected)
{
  
  nsCOMPtr<nsIDOMWindow> currentFocusWin = FindFocusedDOMWindow();
  *aIsFramesetFrameSelected = currentFocusWin != nullptr;
  return NS_OK;
}



NS_IMETHODIMP
nsPrintEngine::GetPrintPreviewNumPages(int32_t *aPrintPreviewNumPages)
{
  NS_ENSURE_ARG_POINTER(aPrintPreviewNumPages);

  nsPrintData* prt = nullptr;
  nsIFrame* seqFrame  = nullptr;
  *aPrintPreviewNumPages = 0;

  
  
  if (mPrtPreview) {
    prt = mPrtPreview;
  } else {
    prt = mPrt;
  }
  if ((!prt) ||
      NS_FAILED(GetSeqFrameAndCountPagesInternal(prt->mPrintObject, seqFrame, *aPrintPreviewNumPages))) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}



NS_IMETHODIMP
nsPrintEngine::EnumerateDocumentNames(uint32_t* aCount,
                                      PRUnichar*** aResult)
{
  NS_ENSURE_ARG(aCount);
  NS_ENSURE_ARG_POINTER(aResult);

  *aCount = 0;
  *aResult = nullptr;

  int32_t     numDocs = mPrt->mPrintDocList.Length();
  PRUnichar** array   = (PRUnichar**) nsMemory::Alloc(numDocs * sizeof(PRUnichar*));
  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;

  for (int32_t i=0;i<numDocs;i++) {
    nsPrintObject* po = mPrt->mPrintDocList.ElementAt(i);
    NS_ASSERTION(po, "nsPrintObject can't be null!");
    PRUnichar * docTitleStr;
    PRUnichar * docURLStr;
    GetDocumentTitleAndURL(po->mDocument, &docTitleStr, &docURLStr);

    
    if (!docTitleStr || !*docTitleStr) {
      if (docURLStr && *docURLStr) {
        nsMemory::Free(docTitleStr);
        docTitleStr = docURLStr;
      } else {
        nsMemory::Free(docURLStr);
      }
      docURLStr = nullptr;
      if (!docTitleStr || !*docTitleStr) {
        CleanupDocTitleArray(array, i);
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    array[i] = docTitleStr;
    if (docURLStr) nsMemory::Free(docURLStr);
  }
  *aCount  = numDocs;
  *aResult = array;

  return NS_OK;

}



nsresult
nsPrintEngine::GetGlobalPrintSettings(nsIPrintSettings **aGlobalPrintSettings)
{
  NS_ENSURE_ARG_POINTER(aGlobalPrintSettings);

  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIPrintSettingsService> printSettingsService =
    do_GetService(sPrintSettingsServiceContractID, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = printSettingsService->GetGlobalPrintSettings(aGlobalPrintSettings);
  }
  return rv;
}



NS_IMETHODIMP
nsPrintEngine::GetDoingPrint(bool *aDoingPrint)
{
  NS_ENSURE_ARG_POINTER(aDoingPrint);
  *aDoingPrint = mIsDoingPrinting;
  return NS_OK;
}



NS_IMETHODIMP
nsPrintEngine::GetDoingPrintPreview(bool *aDoingPrintPreview)
{
  NS_ENSURE_ARG_POINTER(aDoingPrintPreview);
  *aDoingPrintPreview = mIsDoingPrintPreview;
  return NS_OK;
}



NS_IMETHODIMP
nsPrintEngine::GetCurrentPrintSettings(nsIPrintSettings * *aCurrentPrintSettings)
{
  NS_ENSURE_ARG_POINTER(aCurrentPrintSettings);

  if (mPrt) {
    *aCurrentPrintSettings = mPrt->mPrintSettings;

  } else if (mPrtPreview) {
    *aCurrentPrintSettings = mPrtPreview->mPrintSettings;

  } else {
    *aCurrentPrintSettings = nullptr;
  }
  NS_IF_ADDREF(*aCurrentPrintSettings);
  return NS_OK;
}









nsresult
nsPrintEngine::CheckForPrinters(nsIPrintSettings* aPrintSettings)
{
#if defined(XP_MACOSX) || defined(ANDROID)
  
  return NS_OK;
#else
  NS_ENSURE_ARG_POINTER(aPrintSettings);

  
  nsXPIDLString printerName;
  nsresult rv = aPrintSettings->GetPrinterName(getter_Copies(printerName));
  if (NS_SUCCEEDED(rv) && !printerName.IsEmpty()) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIPrintSettingsService> printSettingsService =
    do_GetService(sPrintSettingsServiceContractID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = printSettingsService->GetDefaultPrinterName(getter_Copies(printerName));
  if (NS_SUCCEEDED(rv) && !printerName.IsEmpty()) {
    rv = aPrintSettings->SetPrinterName(printerName.get());
  }
  return rv;
#endif
}



void
nsPrintEngine::ShowPrintProgress(bool aIsForPrinting, bool& aDoNotify)
{
  
  
  
  aDoNotify = false;

  
  bool showProgresssDialog = false;

  
  
  if (!mProgressDialogIsShown) {
    showProgresssDialog = Preferences::GetBool("print.show_print_progress");
  }

  
  
  
  if (showProgresssDialog) {
    mPrt->mPrintSettings->GetShowPrintProgress(&showProgresssDialog);
  }

  
  
  if (showProgresssDialog) {
    nsCOMPtr<nsIPrintingPromptService> printPromptService(do_GetService(kPrintingPromptService));
    if (printPromptService) {
      nsPIDOMWindow *domWin = mDocument->GetWindow(); 
      if (!domWin) return;

      nsCOMPtr<nsIDocShellTreeItem> docShellItem =
        do_QueryInterface(domWin->GetDocShell());
      if (!docShellItem) return;
      nsCOMPtr<nsIDocShellTreeOwner> owner;
      docShellItem->GetTreeOwner(getter_AddRefs(owner));
      nsCOMPtr<nsIWebBrowserChrome> browserChrome = do_GetInterface(owner);
      if (!browserChrome) return;
      bool isModal = true;
      browserChrome->IsWindowModal(&isModal);
      if (isModal) {
        
        
        return;
      }

      nsCOMPtr<nsIWebProgressListener> printProgressListener;

      nsCOMPtr<nsIWebBrowserPrint> wbp(do_QueryInterface(mDocViewerPrint));
      nsresult rv = printPromptService->ShowProgress(domWin, wbp, mPrt->mPrintSettings, this, aIsForPrinting,
                                                     getter_AddRefs(printProgressListener), 
                                                     getter_AddRefs(mPrt->mPrintProgressParams), 
                                                     &aDoNotify);
      if (NS_SUCCEEDED(rv)) {
        if (printProgressListener && mPrt->mPrintProgressParams) {
          mPrt->mPrintProgressListeners.AppendObject(printProgressListener);
          SetDocAndURLIntoProgress(mPrt->mPrintObject, mPrt->mPrintProgressParams);
        }
      }
    }
  }
}


bool
nsPrintEngine::IsThereARangeSelection(nsIDOMWindow* aDOMWin)
{
  nsCOMPtr<nsIPresShell> presShell;
  if (aDOMWin) {
    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aDOMWin));
    window->GetDocShell()->GetPresShell(getter_AddRefs(presShell));
  }

  if (!presShell)
    return false;

  
  
  nsCOMPtr<nsISelection> selection;
  selection = presShell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL);
  if (selection) {
    int32_t count;
    selection->GetRangeCount(&count);
    if (count == 1) {
      nsCOMPtr<nsIDOMRange> range;
      if (NS_SUCCEEDED(selection->GetRangeAt(0, getter_AddRefs(range)))) {
        
        bool isCollapsed;
        selection->GetIsCollapsed(&isCollapsed);
        return !isCollapsed;
      }
    }
    if (count > 1) return true;
  }
  return false;
}


bool
nsPrintEngine::IsParentAFrameSet(nsIDocShell * aParent)
{
  
  nsCOMPtr<nsIDocShellTreeItem> parentAsItem(do_QueryInterface(aParent));
  if (!parentAsItem) return false;

  
  
  
  
  
  
  
  
  
  
  
  
  bool isFrameSet = false;
  
  
  nsCOMPtr<nsIDOMDocument> domDoc = do_GetInterface(aParent);
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if (doc) {
    nsIContent *rootElement = doc->GetRootElement();
    if (rootElement) {
      isFrameSet = HasFramesetChild(rootElement);
    }
  }
  return isFrameSet;
}





void
nsPrintEngine::BuildDocTree(nsIDocShellTreeNode *      aParentNode,
                            nsTArray<nsPrintObject*> * aDocList,
                            nsPrintObject *            aPO)
{
  NS_ASSERTION(aParentNode, "Pointer is null!");
  NS_ASSERTION(aDocList, "Pointer is null!");
  NS_ASSERTION(aPO, "Pointer is null!");

  int32_t childWebshellCount;
  aParentNode->GetChildCount(&childWebshellCount);
  if (childWebshellCount > 0) {
    for (int32_t i=0;i<childWebshellCount;i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      aParentNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));

      nsCOMPtr<nsIContentViewer>  viewer;
      childAsShell->GetContentViewer(getter_AddRefs(viewer));
      if (viewer) {
        nsCOMPtr<nsIContentViewerFile> viewerFile(do_QueryInterface(viewer));
        if (viewerFile) {
          nsCOMPtr<nsIDocShell> childDocShell(do_QueryInterface(child));
          nsCOMPtr<nsIDocShellTreeNode> childNode(do_QueryInterface(child));
          nsCOMPtr<nsIDOMDocument> doc = do_GetInterface(childDocShell);
          nsPrintObject * po = new nsPrintObject();
          po->mParent = aPO;
          nsresult rv = po->Init(childDocShell, doc, aPO->mPrintPreview);
          if (NS_FAILED(rv))
            NS_NOTREACHED("Init failed?");
          aPO->mKids.AppendElement(po);
          aDocList->AppendElement(po);
          BuildDocTree(childNode, aDocList, po);
        }
      }
    }
  }
}


void
nsPrintEngine::GetDocumentTitleAndURL(nsIDocument* aDoc,
                                      PRUnichar**  aTitle,
                                      PRUnichar**  aURLStr)
{
  NS_ASSERTION(aDoc,      "Pointer is null!");
  NS_ASSERTION(aTitle,    "Pointer is null!");
  NS_ASSERTION(aURLStr,   "Pointer is null!");

  *aTitle  = nullptr;
  *aURLStr = nullptr;

  nsAutoString docTitle;
  nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(aDoc);
  doc->GetTitle(docTitle);
  if (!docTitle.IsEmpty()) {
    *aTitle = ToNewUnicode(docTitle);
  }

  nsIURI* url = aDoc->GetDocumentURI();
  if (!url) return;

  nsCOMPtr<nsIURIFixup> urifixup(do_GetService(NS_URIFIXUP_CONTRACTID));
  if (!urifixup) return;

  nsCOMPtr<nsIURI> exposableURI;
  urifixup->CreateExposableURI(url, getter_AddRefs(exposableURI));

  if (!exposableURI) return;

  nsAutoCString urlCStr;
  exposableURI->GetSpec(urlCStr);

  nsresult rv;
  nsCOMPtr<nsITextToSubURI> textToSubURI = 
    do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return;

  nsAutoString unescapedURI;
  rv = textToSubURI->UnEscapeURIForUI(NS_LITERAL_CSTRING("UTF-8"),
                                      urlCStr, unescapedURI);
  if (NS_FAILED(rv)) return;

  *aURLStr = ToNewUnicode(unescapedURI);
}







void
nsPrintEngine::MapContentToWebShells(nsPrintObject* aRootPO,
                                     nsPrintObject* aPO)
{
  NS_ASSERTION(aRootPO, "Pointer is null!");
  NS_ASSERTION(aPO, "Pointer is null!");

  
  
  
  nsCOMPtr<nsIContentViewer> viewer;
  aPO->mDocShell->GetContentViewer(getter_AddRefs(viewer));
  if (!viewer) return;

  nsCOMPtr<nsIDOMDocument> domDoc;
  viewer->GetDOMDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if (!doc) return;

  Element* rootElement = doc->GetRootElement();
  if (rootElement) {
    MapContentForPO(aPO, rootElement);
  } else {
    NS_WARNING("Null root content on (sub)document.");
  }

  
  for (uint32_t i=0;i<aPO->mKids.Length();i++) {
    MapContentToWebShells(aRootPO, aPO->mKids[i]);
  }

}















void
nsPrintEngine::CheckForChildFrameSets(nsPrintObject* aPO)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  
  bool hasChildFrames = false;
  for (uint32_t i=0;i<aPO->mKids.Length();i++) {
    nsPrintObject* po = aPO->mKids[i];
    if (po->mFrameType == eFrame) {
      hasChildFrames = true;
      CheckForChildFrameSets(po);
    }
  }

  if (hasChildFrames && aPO->mFrameType == eFrame) {
    aPO->mFrameType = eFrameSet;
  }
}
















void
nsPrintEngine::MapContentForPO(nsPrintObject*   aPO,
                               nsIContent*      aContent)
{
  NS_PRECONDITION(aPO && aContent, "Null argument");

  nsIDocument* doc = aContent->GetDocument();

  NS_ASSERTION(doc, "Content without a document from a document tree?");

  nsIDocument* subDoc = doc->GetSubDocumentFor(aContent);

  if (subDoc) {
    nsCOMPtr<nsISupports> container = subDoc->GetContainer();
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));

    if (docShell) {
      nsPrintObject * po = nullptr;
      int32_t cnt = aPO->mKids.Length();
      for (int32_t i=0;i<cnt;i++) {
        nsPrintObject* kid = aPO->mKids.ElementAt(i);
        if (kid->mDocument == subDoc) {
          po = kid;
          break;
        }
      }

      
      
      if (po) {

        nsCOMPtr<nsIDOMHTMLFrameElement> frame(do_QueryInterface(aContent));
        
        
        if (frame && po->mParent->mFrameType == eFrameSet) {
          po->mFrameType = eFrame;
        } else {
          
          po->mFrameType = eIFrame;
          SetPrintAsIs(po, true);
          NS_ASSERTION(po->mParent, "The root must be a parent");
          po->mParent->mPrintAsIs = true;
        }
      }
    }
  }

  
  for (nsIContent* child = aContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    MapContentForPO(aPO, child);
  }
}


bool
nsPrintEngine::IsThereAnIFrameSelected(nsIDocShell* aDocShell,
                                       nsIDOMWindow* aDOMWin,
                                       bool& aIsParentFrameSet)
{
  aIsParentFrameSet = IsParentAFrameSet(aDocShell);
  bool iFrameIsSelected = false;
  if (mPrt && mPrt->mPrintObject) {
    nsPrintObject* po = FindPrintObjectByDOMWin(mPrt->mPrintObject, aDOMWin);
    iFrameIsSelected = po && po->mFrameType == eIFrame;
  } else {
    
    if (!aIsParentFrameSet) {
      
      
      
      if (aDOMWin) {
        
        
        nsCOMPtr<nsIDOMWindow> domWin = do_GetInterface(aDocShell);
        if (domWin != aDOMWin) {
          iFrameIsSelected = true; 
        }
      }
    }
  }

  return iFrameIsSelected;
}




void
nsPrintEngine::SetPrintPO(nsPrintObject* aPO, bool aPrint)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  
  aPO->mDontPrint = !aPrint;

  for (uint32_t i=0;i<aPO->mKids.Length();i++) {
    SetPrintPO(aPO->mKids[i], aPrint);
  } 
}






void
nsPrintEngine::GetDisplayTitleAndURL(nsPrintObject*    aPO,
                                     PRUnichar**       aTitle, 
                                     PRUnichar**       aURLStr,
                                     eDocTitleDefault  aDefType)
{
  NS_ASSERTION(aPO, "Pointer is null!");
  NS_ASSERTION(aTitle, "Pointer is null!");
  NS_ASSERTION(aURLStr, "Pointer is null!");

  *aTitle  = nullptr;
  *aURLStr = nullptr;

  if (!mPrt)
    return;

  
  
  PRUnichar * docTitleStrPS = nullptr;
  PRUnichar * docURLStrPS   = nullptr;
  if (mPrt->mPrintSettings) {
    mPrt->mPrintSettings->GetTitle(&docTitleStrPS);
    mPrt->mPrintSettings->GetDocURL(&docURLStrPS);

    if (docTitleStrPS && *docTitleStrPS) {
      *aTitle  = docTitleStrPS;
    }

    if (docURLStrPS && *docURLStrPS) {
      *aURLStr  = docURLStrPS;
    }

    
    if (docTitleStrPS && docURLStrPS) {
      return;
    }
  }

  PRUnichar* docTitle;
  PRUnichar* docUrl;
  GetDocumentTitleAndURL(aPO->mDocument, &docTitle, &docUrl);

  if (docUrl) {
    if (!docURLStrPS)
      *aURLStr = docUrl;
    else
      nsMemory::Free(docUrl);
  }

  if (docTitle) {
    if (!docTitleStrPS)
      *aTitle = docTitle;
    else
      nsMemory::Free(docTitle);
  } else if (!docTitleStrPS) {
    switch (aDefType) {
      case eDocTitleDefBlank: *aTitle = ToNewUnicode(EmptyString());
        break;

      case eDocTitleDefURLDoc:
        if (*aURLStr) {
          *aTitle = NS_strdup(*aURLStr);
        } else if (mPrt->mBrandName) {
          *aTitle = NS_strdup(mPrt->mBrandName);
        }
        break;
      case eDocTitleDefNone:
        
        break;
    }
  }
}


nsresult nsPrintEngine::DocumentReadyForPrinting()
{
  if (mPrt->mPrintFrameType == nsIPrintSettings::kEachFrameSep) {
    CheckForChildFrameSets(mPrt->mPrintObject);
  }

  
  
  
  nsresult rv = SetupToPrintContent();
  if (NS_FAILED(rv)) {
    
    
    DonePrintingPages(nullptr, rv);
  }
  return rv;
}




nsresult nsPrintEngine::CleanupOnFailure(nsresult aResult, bool aIsPrinting)
{
  PR_PL(("****  Failed %s - rv 0x%X", aIsPrinting?"Printing":"Print Preview", aResult));

  
  if (mPagePrintTimer) {
    mPagePrintTimer->Stop();
    NS_RELEASE(mPagePrintTimer);
  }
  
  if (aIsPrinting) {
    SetIsPrinting(false);
  } else {
    SetIsPrintPreview(false);
    SetIsCreatingPrintPreview(false);
  }

  





  if (aResult != NS_ERROR_ABORT) {
    ShowPrintErrorDialog(aResult, aIsPrinting);
  }

  FirePrintCompletionEvent();

  return aResult;

}


void
nsPrintEngine::ShowPrintErrorDialog(nsresult aPrintError, bool aIsPrinting)
{

  PR_PL(("nsPrintEngine::ShowPrintErrorDialog(nsresult aPrintError=%lx, bool aIsPrinting=%d)\n", (long)aPrintError, (int)aIsPrinting));

  nsAutoCString stringName;

  switch(aPrintError)
  {
#define NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(nserr) case nserr: stringName.AssignLiteral(#nserr); break;
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_CMD_NOT_FOUND)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_CMD_FAILURE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_NO_PRINTER_AVAILABLE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_NAME_NOT_FOUND)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_ACCESS_DENIED)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_INVALID_ATTRIBUTE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_PRINTER_NOT_READY)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_OUT_OF_PAPER)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_PRINTER_IO_ERROR)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_FILE_IO_ERROR)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_PRINTPREVIEW)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_UNEXPECTED)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_OUT_OF_MEMORY)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_NOT_IMPLEMENTED)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_NOT_AVAILABLE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_ABORT)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_STARTDOC)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_ENDDOC)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_STARTPAGE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_ENDPAGE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_PRINT_WHILE_PREVIEW)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_PAPER_SIZE_NOT_SUPPORTED)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_ORIENTATION_NOT_SUPPORTED)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_COLORSPACE_NOT_SUPPORTED)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_TOO_MANY_COPIES)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_DRIVER_CONFIGURATION_ERROR)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_DOC_IS_BUSY_PP)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_DOC_WAS_DESTORYED)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_NO_PRINTDIALOG_IN_TOOLKIT)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_NO_PRINTROMPTSERVICE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_NO_XUL)   
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_PLEX_NOT_SUPPORTED)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_DOC_IS_BUSY)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTING_NOT_IMPLEMENTED)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_COULD_NOT_LOAD_PRINT_MODULE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_RESOLUTION_NOT_SUPPORTED)

    default:
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_FAILURE)
#undef NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG
  }

  PR_PL(("ShowPrintErrorDialog:  stringName='%s'\n", stringName.get()));

  nsXPIDLString msg, title;
  nsresult rv =
    nsContentUtils::GetLocalizedString(nsContentUtils::ePRINTING_PROPERTIES,
                                       stringName.get(), msg);
  if (NS_FAILED(rv)) {
    PR_PL(("GetLocalizedString failed\n"));
    return;
  }

  rv = nsContentUtils::GetLocalizedString(nsContentUtils::ePRINTING_PROPERTIES,
      aIsPrinting ? "print_error_dialog_title"
                  : "printpreview_error_dialog_title",
      title);

  nsCOMPtr<nsIWindowWatcher> wwatch = do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    PR_PL(("ShowPrintErrorDialog(): wwatch==nullptr\n"));
    return;
  }

  nsCOMPtr<nsIDOMWindow> active;
  wwatch->GetActiveWindow(getter_AddRefs(active));

  nsCOMPtr<nsIPrompt> dialog;
  

  wwatch->GetNewPrompter(active, getter_AddRefs(dialog));
  if (!dialog) {
    PR_PL(("ShowPrintErrorDialog(): dialog==nullptr\n"));
    return;
  }

  dialog->Alert(title.get(), msg.get());
  PR_PL(("ShowPrintErrorDialog(): alert displayed successfully.\n"));
}





nsresult
nsPrintEngine::ReconstructAndReflow(bool doSetPixelScale)
{
#if (defined(XP_WIN) || defined(XP_OS2)) && defined(EXTENDED_DEBUG_PRINTING)
  
  
  if (kPrintingLogMod && kPrintingLogMod->level == DUMP_LAYOUT_LEVEL) {
    RemoveFilesInDir(".\\");
    gDumpFileNameCnt   = 0;
    gDumpLOFileNameCnt = 0;
  }
#endif

  for (uint32_t i = 0; i < mPrt->mPrintDocList.Length(); ++i) {
    nsPrintObject* po = mPrt->mPrintDocList.ElementAt(i);
    NS_ASSERTION(po, "nsPrintObject can't be null!");

    if (po->mDontPrint || po->mInvisible) {
      continue;
    }

    UpdateZoomRatio(po, doSetPixelScale);

    po->mPresContext->SetPageScale(po->mZoomRatio);

    
    float printDPI = float(mPrt->mPrintDC->AppUnitsPerCSSInch()) /
                     float(mPrt->mPrintDC->AppUnitsPerDevPixel());
    po->mPresContext->SetPrintPreviewScale(mScreenDPI / printDPI);

    po->mPresShell->ReconstructFrames();

    
    
    bool documentIsTopLevel = true;
    if (i != 0) {
      nsSize adjSize;
      bool doReturn; 
      nsresult rv = SetRootView(po, doReturn, documentIsTopLevel, adjSize);

      MOZ_ASSERT(!documentIsTopLevel, "How could this happen?");
      
      if (NS_FAILED(rv) || doReturn) {
        return rv; 
      }
    }

    po->mPresShell->FlushPendingNotifications(Flush_Layout);

    nsresult rv = UpdateSelectionAndShrinkPrintObject(po, documentIsTopLevel);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


nsresult
nsPrintEngine::SetupToPrintContent()
{
  nsresult rv;

  bool didReconstruction = false;
  
  
  
  if (mDidLoadDataForPrinting) {
    rv = ReconstructAndReflow(DoSetPixelScale());
    didReconstruction = true;
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  bool ppIsShrinkToFit = mPrtPreview && mPrtPreview->mShrinkToFit;
  if (mPrt->mShrinkToFit && !ppIsShrinkToFit) {
    
    if (mPrt->mPrintDocList.Length() > 1 && mPrt->mPrintObject->mFrameType == eFrameSet) {
      nsPrintObject* smallestPO = FindSmallestSTF();
      NS_ASSERTION(smallestPO, "There must always be an XMost PO!");
      if (smallestPO) {
        
        mPrt->mShrinkRatio = smallestPO->mShrinkRatio;
      }
    } else {
      
      mPrt->mShrinkRatio = mPrt->mPrintObject->mShrinkRatio;
    }

    if (mPrt->mShrinkRatio < 0.998f) {
      rv = ReconstructAndReflow(true);
      didReconstruction = true;
      NS_ENSURE_SUCCESS(rv, rv);
    }

#ifdef PR_LOGGING
    float calcRatio = 0.0f;
    if (mPrt->mPrintDocList.Length() > 1 && mPrt->mPrintObject->mFrameType == eFrameSet) {
      nsPrintObject* smallestPO = FindSmallestSTF();
      NS_ASSERTION(smallestPO, "There must always be an XMost PO!");
      if (smallestPO) {
        
        calcRatio = smallestPO->mShrinkRatio;
      }
    } else {
      
      calcRatio = mPrt->mPrintObject->mShrinkRatio;
    }
    PR_PL(("**************************************************************************\n"));
    PR_PL(("STF Ratio is: %8.5f Effective Ratio: %8.5f Diff: %8.5f\n", mPrt->mShrinkRatio, calcRatio,  mPrt->mShrinkRatio-calcRatio));
    PR_PL(("**************************************************************************\n"));
#endif
  }
  
  
  
  if (didReconstruction) {
    FirePrintPreviewUpdateEvent();
  }
  
  DUMP_DOC_LIST(("\nAfter Reflow------------------------------------------"));
  PR_PL(("\n"));
  PR_PL(("-------------------------------------------------------\n"));
  PR_PL(("\n"));

  CalcNumPrintablePages(mPrt->mNumPrintablePages);

  PR_PL(("--- Printing %d pages\n", mPrt->mNumPrintablePages));
  DUMP_DOC_TREELAYOUT;

  
  if (mPrt != nullptr) {
    mPrt->OnStartPrinting();    
  }

  PRUnichar* fileName = nullptr;
  
  bool isPrintToFile = false;
  mPrt->mPrintSettings->GetPrintToFile(&isPrintToFile);
  if (isPrintToFile) {
  
  
    mPrt->mPrintSettings->GetToFileName(&fileName);
  }

  PRUnichar * docTitleStr;
  PRUnichar * docURLStr;
  GetDisplayTitleAndURL(mPrt->mPrintObject, &docTitleStr, &docURLStr, eDocTitleDefURLDoc); 

  int32_t startPage = 1;
  int32_t endPage   = mPrt->mNumPrintablePages;

  int16_t printRangeType = nsIPrintSettings::kRangeAllPages;
  mPrt->mPrintSettings->GetPrintRange(&printRangeType);
  if (printRangeType == nsIPrintSettings::kRangeSpecifiedPageRange) {
    mPrt->mPrintSettings->GetStartPageRange(&startPage);
    mPrt->mPrintSettings->GetEndPageRange(&endPage);
    if (endPage > mPrt->mNumPrintablePages) {
      endPage = mPrt->mNumPrintablePages;
    }
  }

  rv = NS_OK;
  
  
  
  
  if (!mPrt->mDebugFilePtr && mIsDoingPrinting) {
    rv = mPrt->mPrintDC->BeginDocument(docTitleStr, fileName, startPage, endPage);
  } 

  if (mIsCreatingPrintPreview) {
    
    
    nsIPageSequenceFrame *seqFrame = mPrt->mPrintObject->mPresShell->GetPageSequenceFrame();
    if (seqFrame) {
      seqFrame->StartPrint(mPrt->mPrintObject->mPresContext, 
                           mPrt->mPrintSettings, docTitleStr, docURLStr);
      docTitleStr = nullptr;
      docURLStr = nullptr;
    }
  }
  if (docTitleStr) nsMemory::Free(docTitleStr);
  if (docURLStr) nsMemory::Free(docURLStr);

  PR_PL(("****************** Begin Document ************************\n"));

  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  

  if (mIsDoingPrinting) {
    PrintDocContent(mPrt->mPrintObject, rv); 
  }

  return rv;
}




nsresult
nsPrintEngine::ReflowDocList(nsPrintObject* aPO, bool aSetPixelScale)
{
  NS_ENSURE_ARG_POINTER(aPO);

  
  if (aPO->mParent && aPO->mParent->mPresShell) {
    nsIFrame* frame = aPO->mContent ? aPO->mContent->GetPrimaryFrame() : nullptr;
    if (!frame || !frame->GetStyleVisibility()->IsVisible()) {
      SetPrintPO(aPO, false);
      aPO->mInvisible = true;
      return NS_OK;
    }
  }

  UpdateZoomRatio(aPO, aSetPixelScale);

  nsresult rv;
  
  rv = ReflowPrintObject(aPO);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t cnt = aPO->mKids.Length();
  for (int32_t i=0;i<cnt;i++) {
    rv = ReflowDocList(aPO->mKids[i], aSetPixelScale);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

void
nsPrintEngine::FirePrintPreviewUpdateEvent()
{
  
  
  if (mIsDoingPrintPreview && !mIsDoingPrinting) {
    nsCOMPtr<nsIContentViewer> cv = do_QueryInterface(mDocViewerPrint);
    (new nsAsyncDOMEvent(
       cv->GetDocument(), NS_LITERAL_STRING("printPreviewUpdate"), true, true)
    )->RunDOMEventWhenSafe();
  }
}

nsresult
nsPrintEngine::InitPrintDocConstruction(bool aHandleError)
{
  nsresult rv;
  rv = ReflowDocList(mPrt->mPrintObject, DoSetPixelScale());
  NS_ENSURE_SUCCESS(rv, rv);

  FirePrintPreviewUpdateEvent();

  if (mLoadCounter == 0) {
    AfterNetworkPrint(aHandleError);
  }
  return rv;
}

nsresult
nsPrintEngine::AfterNetworkPrint(bool aHandleError)
{
  nsCOMPtr<nsIWebProgress> webProgress = do_QueryInterface(mPrt->mPrintObject->mDocShell);

  webProgress->RemoveProgressListener(
    static_cast<nsIWebProgressListener*>(this));

  nsresult rv;
  if (mIsDoingPrinting) {
    rv = DocumentReadyForPrinting();
  } else {
    rv = FinishPrintPreview();
  }

  
  if (aHandleError && NS_FAILED(rv)) {
    CleanupOnFailure(rv, !mIsDoingPrinting);
  }

  return rv;
}




NS_IMETHODIMP
nsPrintEngine::OnStateChange(nsIWebProgress* aWebProgress,
                             nsIRequest* aRequest,
                             uint32_t aStateFlags,
                             nsresult aStatus)
{
  nsAutoCString name;
  aRequest->GetName(name);
  if (name.Equals("about:document-onload-blocker")) {
    return NS_OK;
  }
  if (aStateFlags & STATE_START) {
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);

    ++mLoadCounter;
  } else if (aStateFlags & STATE_STOP) {
    mDidLoadDataForPrinting = true;
    --mLoadCounter;
   
    
    
    if (mLoadCounter == 0) {
      AfterNetworkPrint(true);
    }
  }
  return NS_OK;
}



NS_IMETHODIMP
nsPrintEngine::OnProgressChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 int32_t aCurSelfProgress,
                                 int32_t aMaxSelfProgress,
                                 int32_t aCurTotalProgress,
                                 int32_t aMaxTotalProgress)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsPrintEngine::OnLocationChange(nsIWebProgress* aWebProgress,
                                nsIRequest* aRequest,
                                nsIURI* aLocation,
                                uint32_t aFlags)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsPrintEngine::OnStatusChange(nsIWebProgress *aWebProgress,
                              nsIRequest *aRequest,
                              nsresult aStatus,
                              const PRUnichar *aMessage)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsPrintEngine::OnSecurityChange(nsIWebProgress *aWebProgress,
                                  nsIRequest *aRequest,
                                  uint32_t aState)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}



void
nsPrintEngine::UpdateZoomRatio(nsPrintObject* aPO, bool aSetPixelScale)
{
  
  
  if (aSetPixelScale && aPO->mFrameType != eIFrame) {
    float ratio;
    if (mPrt->mPrintFrameType == nsIPrintSettings::kFramesAsIs || mPrt->mPrintFrameType == nsIPrintSettings::kNoFrames) {
      ratio = mPrt->mShrinkRatio - 0.005f; 
    } else {
      ratio = aPO->mShrinkRatio - 0.005f; 
    }
    aPO->mZoomRatio = ratio;
  } else if (!mPrt->mShrinkToFit) {
    double scaling;
    mPrt->mPrintSettings->GetScaling(&scaling);
    aPO->mZoomRatio = float(scaling);
  } 
}

nsresult
nsPrintEngine::UpdateSelectionAndShrinkPrintObject(nsPrintObject* aPO,
                                                   bool aDocumentIsTopLevel)
{
  nsCOMPtr<nsIPresShell> displayShell;
  aPO->mDocShell->GetPresShell(getter_AddRefs(displayShell));
  
  nsCOMPtr<nsISelection> selection, selectionPS;
  
  if (displayShell) {
    selection = displayShell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL);
  }
  selectionPS = aPO->mPresShell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL);

  
  
  if (selectionPS) {
    selectionPS->RemoveAllRanges();
  }
  if (selection && selectionPS) {
    int32_t cnt;
    selection->GetRangeCount(&cnt);
    int32_t inx;
    for (inx = 0; inx < cnt; ++inx) {
      nsCOMPtr<nsIDOMRange> range;
      if (NS_SUCCEEDED(selection->GetRangeAt(inx, getter_AddRefs(range))))
        selectionPS->AddRange(range);
    }
  }

  
  
  
  
  
  if (mPrt->mShrinkToFit && aDocumentIsTopLevel) {
    nsIPageSequenceFrame* pageSequence = aPO->mPresShell->GetPageSequenceFrame();
    NS_ENSURE_STATE(pageSequence);
    pageSequence->GetSTFPercent(aPO->mShrinkRatio);
  }
  return NS_OK;
}

bool
nsPrintEngine::DoSetPixelScale()
{
  
  
  
  
  
  
  bool doSetPixelScale = false;
  bool ppIsShrinkToFit = mPrtPreview && mPrtPreview->mShrinkToFit;
  if (ppIsShrinkToFit) {
    mPrt->mShrinkRatio = mPrtPreview->mShrinkRatio;
    doSetPixelScale = true;
  }
  return doSetPixelScale;
}

nsIView*
nsPrintEngine::GetParentViewForRoot()
{
  if (mIsCreatingPrintPreview) {
    nsCOMPtr<nsIContentViewer> cv = do_QueryInterface(mDocViewerPrint);
    if (cv) {
      return cv->FindContainerView();
    }
  }
  return nullptr;
}

nsresult
nsPrintEngine::SetRootView(
    nsPrintObject* aPO, 
    bool& doReturn, 
    bool& documentIsTopLevel, 
    nsSize& adjSize
)
{
  bool canCreateScrollbars = true;

  nsIView* rootView;
  nsIView* parentView = nullptr;

  doReturn = false;

  if (aPO->mParent && aPO->mParent->IsPrintable()) {
    nsIFrame* frame = aPO->mContent ? aPO->mContent->GetPrimaryFrame() : nullptr;
    
    
    if (!frame) {
      SetPrintPO(aPO, false);
      doReturn = true;
      return NS_OK;
    }

    
    
    
    adjSize = frame->GetContentRect().Size();
    documentIsTopLevel = false;
    

    
    if (frame && frame->GetType() == nsGkAtoms::subDocumentFrame) {
      nsIView* view = frame->GetView();
      NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);
      view = view->GetFirstChild();
      NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);
      parentView = view;
      canCreateScrollbars = false;
    }
  } else {
    nscoord pageWidth, pageHeight;
    mPrt->mPrintDC->GetDeviceSurfaceDimensions(pageWidth, pageHeight);
    adjSize = nsSize(pageWidth, pageHeight);
    documentIsTopLevel = true;
    parentView = GetParentViewForRoot();
  }

  if (aPO->mViewManager->GetRootView()) {
    
    rootView = aPO->mViewManager->GetRootView();
    
    aPO->mViewManager->RemoveChild(rootView);
    reinterpret_cast<nsView*>(rootView)->SetParent(reinterpret_cast<nsView*>(parentView));
  } else {
    
    nsRect tbounds = nsRect(nsPoint(0, 0), adjSize);
    rootView = aPO->mViewManager->CreateView(tbounds, parentView);
    NS_ENSURE_TRUE(rootView, NS_ERROR_OUT_OF_MEMORY);
  }
    
  if (mIsCreatingPrintPreview && documentIsTopLevel) {
    aPO->mPresContext->SetPaginatedScrolling(canCreateScrollbars);
  }

  
  aPO->mViewManager->SetRootView(rootView);

  return NS_OK;
}


nsresult
nsPrintEngine::ReflowPrintObject(nsPrintObject * aPO)
{
  NS_ENSURE_STATE(aPO);

  if (!aPO->IsPrintable()) {
    return NS_OK;
  }
  
  NS_ASSERTION(!aPO->mPresContext, "Recreating prescontext");

  
  nsPresContext::nsPresContextType type =
      mIsCreatingPrintPreview ? nsPresContext::eContext_PrintPreview:
                                nsPresContext::eContext_Print;
  nsIView* parentView =
    aPO->mParent && aPO->mParent->IsPrintable() ? nullptr : GetParentViewForRoot();
  aPO->mPresContext = parentView ?
      new nsPresContext(aPO->mDocument, type) :
      new nsRootPresContext(aPO->mDocument, type);
  NS_ENSURE_TRUE(aPO->mPresContext, NS_ERROR_OUT_OF_MEMORY);
  aPO->mPresContext->SetPrintSettings(mPrt->mPrintSettings);

  
  bool printBGColors;
  mPrt->mPrintSettings->GetPrintBGColors(&printBGColors);
  aPO->mPresContext->SetBackgroundColorDraw(printBGColors);
  mPrt->mPrintSettings->GetPrintBGImages(&printBGColors);
  aPO->mPresContext->SetBackgroundImageDraw(printBGColors);

  
  nsresult rv = aPO->mPresContext->Init(mPrt->mPrintDC);
  NS_ENSURE_SUCCESS(rv, rv);

  aPO->mViewManager = do_CreateInstance(kViewManagerCID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = aPO->mViewManager->Init(mPrt->mPrintDC);
  NS_ENSURE_SUCCESS(rv,rv);

  nsStyleSet* styleSet;
  rv = mDocViewerPrint->CreateStyleSet(aPO->mDocument, &styleSet);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aPO->mDocument->CreateShell(aPO->mPresContext, aPO->mViewManager,
                                   styleSet, getter_AddRefs(aPO->mPresShell));
  if (NS_FAILED(rv)) {
    delete styleSet;
    return rv;
  }

  styleSet->EndUpdate();
  
  


  bool doReturn = false;;
  bool documentIsTopLevel = false;
  nsSize adjSize; 

  rv = SetRootView(aPO, doReturn, documentIsTopLevel, adjSize);

  if (NS_FAILED(rv) || doReturn) {
    return rv; 
  }

  PR_PL(("In DV::ReflowPrintObject PO: %p pS: %p (%9s) Setting w,h to %d,%d\n", aPO, aPO->mPresShell.get(),
         gFrameTypesStr[aPO->mFrameType], adjSize.width, adjSize.height));


  
  
  nsCOMPtr<nsISupports> supps(do_QueryInterface(aPO->mDocShell));
  aPO->mPresContext->SetContainer(supps);

  aPO->mPresShell->BeginObservingDocument();

  aPO->mPresContext->SetPageSize(adjSize);
  aPO->mPresContext->SetIsRootPaginatedDocument(documentIsTopLevel);
  aPO->mPresContext->SetPageScale(aPO->mZoomRatio);
  
  float printDPI = float(mPrt->mPrintDC->AppUnitsPerCSSInch()) /
                   float(mPrt->mPrintDC->AppUnitsPerDevPixel());
  aPO->mPresContext->SetPrintPreviewScale(mScreenDPI / printDPI);

  if (mIsCreatingPrintPreview && documentIsTopLevel) {
    mDocViewerPrint->SetPrintPreviewPresentation(aPO->mViewManager,
                                                 aPO->mPresContext,
                                                 aPO->mPresShell);
  }

  rv = aPO->mPresShell->Initialize(adjSize.width, adjSize.height);

  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(aPO->mPresShell, "Presshell should still be here");

  
  aPO->mPresShell->FlushPendingNotifications(Flush_Layout);

  rv = UpdateSelectionAndShrinkPrintObject(aPO, documentIsTopLevel);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef EXTENDED_DEBUG_PRINTING
    if (kPrintingLogMod && kPrintingLogMod->level == DUMP_LAYOUT_LEVEL) {
      char * docStr;
      char * urlStr;
      GetDocTitleAndURL(aPO, docStr, urlStr);
      char filename[256];
      sprintf(filename, "print_dump_%d.txt", gDumpFileNameCnt++);
      
      FILE * fd = fopen(filename, "w");
      if (fd) {
        nsIFrame *theRootFrame =
          aPO->mPresShell->FrameManager()->GetRootFrame();
        fprintf(fd, "Title: %s\n", docStr?docStr:"");
        fprintf(fd, "URL:   %s\n", urlStr?urlStr:"");
        fprintf(fd, "--------------- Frames ----------------\n");
        nsRefPtr<nsRenderingContext> renderingContext;
        mPrt->mPrintDocDC->CreateRenderingContext(*getter_AddRefs(renderingContext));
        RootFrameList(aPO->mPresContext, fd, 0);
        
        fprintf(fd, "---------------------------------------\n\n");
        fprintf(fd, "--------------- Views From Root Frame----------------\n");
        nsIView* v = theRootFrame->GetView();
        if (v) {
          v->List(fd);
        } else {
          printf("View is null!\n");
        }
        if (docShell) {
          fprintf(fd, "--------------- All Views ----------------\n");
          DumpViews(docShell, fd);
          fprintf(fd, "---------------------------------------\n\n");
        }
        fclose(fd);
      }
      if (docStr) nsMemory::Free(docStr);
      if (urlStr) nsMemory::Free(urlStr);
    }
#endif

  return NS_OK;
}



void
nsPrintEngine::CalcNumPrintablePages(int32_t& aNumPages)
{
  aNumPages = 0;
  
  
  for (uint32_t i=0; i<mPrt->mPrintDocList.Length(); i++) {
    nsPrintObject* po = mPrt->mPrintDocList.ElementAt(i);
    NS_ASSERTION(po, "nsPrintObject can't be null!");
    if (po->mPresContext && po->mPresContext->IsRootPaginatedDocument()) {
      nsIPageSequenceFrame* pageSequence = po->mPresShell->GetPageSequenceFrame();
      nsIFrame * seqFrame = do_QueryFrame(pageSequence);
      if (seqFrame) {
        nsIFrame* frame = seqFrame->GetFirstPrincipalChild();
        while (frame) {
          aNumPages++;
          frame = frame->GetNextSibling();
        }
      }
    }
  }
}










bool
nsPrintEngine::PrintDocContent(nsPrintObject* aPO, nsresult& aStatus)
{
  NS_ASSERTION(aPO, "Pointer is null!");
  aStatus = NS_OK;

  if (!aPO->mHasBeenPrinted && aPO->IsPrintable()) {
    aStatus = DoPrint(aPO);
    return true;
  }

  
  
  if (!aPO->mInvisible && !(aPO->mPrintAsIs && aPO->mHasBeenPrinted)) {
    for (uint32_t i=0;i<aPO->mKids.Length();i++) {
      nsPrintObject* po = aPO->mKids[i];
      bool printed = PrintDocContent(po, aStatus);
      if (printed || NS_FAILED(aStatus)) {
        return true;
      }
    }
  }
  return false;
}

static already_AddRefed<nsIDOMNode>
GetEqualNodeInCloneTree(nsIDOMNode* aNode, nsIDocument* aDoc)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  
  if (content && content->IsInAnonymousSubtree()) {
    return nullptr;
  }

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, nullptr);

  nsTArray<int32_t> indexArray;
  nsINode* current = node;
  NS_ENSURE_TRUE(current, nullptr);
  while (current) {
    nsINode* parent = current->GetNodeParent();
    if (!parent) {
     break;
    }
    int32_t index = parent->IndexOf(current);
    NS_ENSURE_TRUE(index >= 0, nullptr);
    indexArray.AppendElement(index);
    current = parent;
  }
  NS_ENSURE_TRUE(current->IsNodeOfType(nsINode::eDOCUMENT), nullptr);

  current = aDoc;
  for (int32_t i = indexArray.Length() - 1; i >= 0; --i) {
    current = current->GetChildAt(indexArray[i]);
    NS_ENSURE_TRUE(current, nullptr);
  }
  nsCOMPtr<nsIDOMNode> result = do_QueryInterface(current);
  return result.forget();
}

static nsresult CloneRangeToSelection(nsIDOMRange* aRange,
                                      nsIDocument* aDoc,
                                      nsISelection* aSelection)
{
  bool collapsed = false;
  aRange->GetCollapsed(&collapsed);
  if (collapsed) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> startContainer, endContainer;
  int32_t startOffset = -1, endOffset = -1;
  aRange->GetStartContainer(getter_AddRefs(startContainer));
  aRange->GetStartOffset(&startOffset);
  aRange->GetEndContainer(getter_AddRefs(endContainer));
  aRange->GetEndOffset(&endOffset);
  NS_ENSURE_STATE(startContainer && endContainer);

  nsCOMPtr<nsIDOMNode> newStart = GetEqualNodeInCloneTree(startContainer, aDoc);
  nsCOMPtr<nsIDOMNode> newEnd = GetEqualNodeInCloneTree(endContainer, aDoc);
  NS_ENSURE_STATE(newStart && newEnd);

  nsRefPtr<nsRange> range = new nsRange();
  nsresult rv = range->SetStart(newStart, startOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = range->SetEnd(newEnd, endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  return aSelection->AddRange(range);
}

static nsresult CloneSelection(nsIDocument* aOrigDoc, nsIDocument* aDoc)
{
  nsIPresShell* origShell = aOrigDoc->GetShell();
  nsIPresShell* shell = aDoc->GetShell();
  NS_ENSURE_STATE(origShell && shell);

  nsCOMPtr<nsISelection> origSelection =
    origShell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL);
  nsCOMPtr<nsISelection> selection =
    shell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL);
  NS_ENSURE_STATE(origSelection && selection);

  int32_t rangeCount = 0;
  origSelection->GetRangeCount(&rangeCount);
  for (int32_t i = 0; i < rangeCount; ++i) {
    nsCOMPtr<nsIDOMRange> range;
    origSelection->GetRangeAt(i, getter_AddRefs(range));
    if (range) {
      CloneRangeToSelection(range, aDoc, selection);
    }
  }
  return NS_OK;
}


nsresult
nsPrintEngine::DoPrint(nsPrintObject * aPO)
{
  PR_PL(("\n"));
  PR_PL(("**************************** %s ****************************\n", gFrameTypesStr[aPO->mFrameType]));
  PR_PL(("****** In DV::DoPrint   PO: %p \n", aPO));

  nsIPresShell*   poPresShell   = aPO->mPresShell;
  nsPresContext*  poPresContext = aPO->mPresContext;

  NS_ASSERTION(poPresContext, "PrintObject has not been reflowed");
  NS_ASSERTION(poPresContext->Type() != nsPresContext::eContext_PrintPreview,
               "How did this context end up here?");

  if (mPrt->mPrintProgressParams) {
    SetDocAndURLIntoProgress(aPO, mPrt->mPrintProgressParams);
  }

  {
    int16_t printRangeType = nsIPrintSettings::kRangeAllPages;
    nsresult rv;
    if (mPrt->mPrintSettings != nullptr) {
      mPrt->mPrintSettings->GetPrintRange(&printRangeType);
    }

    
    nsIPageSequenceFrame* pageSequence = poPresShell->GetPageSequenceFrame();
    NS_ASSERTION(nullptr != pageSequence, "no page sequence frame");

    
    mPrt->mPreparingForPrint = false;

    
    if (nullptr != mPrt->mDebugFilePtr) {
#ifdef DEBUG
      
      nsIFrame* root = poPresShell->FrameManager()->GetRootFrame();
      root->DumpRegressionData(poPresContext, mPrt->mDebugFilePtr, 0);
      fclose(mPrt->mDebugFilePtr);
      SetIsPrinting(false);
#endif
    } else {
#ifdef EXTENDED_DEBUG_PRINTING
      nsIFrame* rootFrame = poPresShell->FrameManager()->GetRootFrame();
      if (aPO->IsPrintable()) {
        char * docStr;
        char * urlStr;
        GetDocTitleAndURL(aPO, docStr, urlStr);
        DumpLayoutData(docStr, urlStr, poPresContext, mPrt->mPrintDocDC, rootFrame, docShell, nullptr);
        if (docStr) nsMemory::Free(docStr);
        if (urlStr) nsMemory::Free(urlStr);
      }
#endif

      if (!mPrt->mPrintSettings) {
        
        SetIsPrinting(false);
        return NS_ERROR_FAILURE;
      }

      PRUnichar * docTitleStr = nullptr;
      PRUnichar * docURLStr   = nullptr;

      GetDisplayTitleAndURL(aPO, &docTitleStr, &docURLStr, eDocTitleDefBlank);

      if (nsIPrintSettings::kRangeSelection == printRangeType) {
        CloneSelection(aPO->mDocument->GetOriginalDocument(), aPO->mDocument);

        poPresContext->SetIsRenderingOnlySelection(true);
        
        
        nsRefPtr<nsRenderingContext> rc;
        mPrt->mPrintDC->CreateRenderingContext(*getter_AddRefs(rc));

        
        
        nsIFrame* startFrame;
        nsIFrame* endFrame;
        int32_t   startPageNum;
        int32_t   endPageNum;
        nsRect    startRect;
        nsRect    endRect;

        nsCOMPtr<nsISelection> selectionPS;
        selectionPS = poPresShell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL);

        rv = GetPageRangeForSelection(poPresShell, poPresContext, *rc, selectionPS, pageSequence,
                                      &startFrame, startPageNum, startRect,
                                      &endFrame, endPageNum, endRect);
        if (NS_SUCCEEDED(rv)) {
          mPrt->mPrintSettings->SetStartPageRange(startPageNum);
          mPrt->mPrintSettings->SetEndPageRange(endPageNum);
          nsIntMargin marginTwips(0,0,0,0);
          nsIntMargin unwrtMarginTwips(0,0,0,0);
          mPrt->mPrintSettings->GetMarginInTwips(marginTwips);
          mPrt->mPrintSettings->GetUnwriteableMarginInTwips(unwrtMarginTwips);
          nsMargin totalMargin = poPresContext->CSSTwipsToAppUnits(marginTwips +
                                                                   unwrtMarginTwips);
          if (startPageNum == endPageNum) {
            startRect.y -= totalMargin.top;
            endRect.y   -= totalMargin.top;

            
            if (startRect.y < 0) {
              
              
              startRect.height = NS_MAX(0, startRect.YMost());
              startRect.y = 0;
            }
            if (endRect.y < 0) {
              
              
              endRect.height = NS_MAX(0, endRect.YMost());
              endRect.y = 0;
            }
            NS_ASSERTION(endRect.y >= startRect.y,
                         "Selection end point should be after start point");
            NS_ASSERTION(startRect.height >= 0,
                         "rect should have non-negative height.");
            NS_ASSERTION(endRect.height >= 0,
                         "rect should have non-negative height.");

            nscoord selectionHgt = endRect.y + endRect.height - startRect.y;
            
            pageSequence->SetSelectionHeight(startRect.y * aPO->mZoomRatio,
                                             selectionHgt * aPO->mZoomRatio);

            
            
            nscoord pageWidth, pageHeight;
            mPrt->mPrintDC->GetDeviceSurfaceDimensions(pageWidth, pageHeight);
            pageHeight -= totalMargin.top + totalMargin.bottom;
            int32_t totalPages = NSToIntCeil(float(selectionHgt) * aPO->mZoomRatio / float(pageHeight));
            pageSequence->SetTotalNumPages(totalPages);
          }
        }
      }

      nsIFrame * seqFrame = do_QueryFrame(pageSequence);
      if (!seqFrame) {
        SetIsPrinting(false);
        if (docTitleStr) nsMemory::Free(docTitleStr);
        if (docURLStr) nsMemory::Free(docURLStr);
        return NS_ERROR_FAILURE;
      }

      mPageSeqFrame = pageSequence;
      mPageSeqFrame->StartPrint(poPresContext, mPrt->mPrintSettings, docTitleStr, docURLStr);

      
      PR_PL(("Scheduling Print of PO: %p (%s) \n", aPO, gFrameTypesStr[aPO->mFrameType]));
      StartPagePrintTimer(aPO);
    }
  }

  return NS_OK;
}


void
nsPrintEngine::SetDocAndURLIntoProgress(nsPrintObject* aPO,
                                        nsIPrintProgressParams* aParams)
{
  NS_ASSERTION(aPO, "Must have vaild nsPrintObject");
  NS_ASSERTION(aParams, "Must have vaild nsIPrintProgressParams");

  if (!aPO || !aPO->mDocShell || !aParams) {
    return;
  }
  const uint32_t kTitleLength = 64;

  PRUnichar * docTitleStr;
  PRUnichar * docURLStr;
  GetDisplayTitleAndURL(aPO, &docTitleStr, &docURLStr, eDocTitleDefURLDoc);

  
  ElipseLongString(docTitleStr, kTitleLength, false);
  ElipseLongString(docURLStr, kTitleLength, true);

  aParams->SetDocTitle(docTitleStr);
  aParams->SetDocURL(docURLStr);

  if (docTitleStr != nullptr) nsMemory::Free(docTitleStr);
  if (docURLStr != nullptr) nsMemory::Free(docURLStr);
}


void
nsPrintEngine::ElipseLongString(PRUnichar *& aStr, const uint32_t aLen, bool aDoFront)
{
  
  if (aStr && NS_strlen(aStr) > aLen) {
    if (aDoFront) {
      PRUnichar * ptr = &aStr[NS_strlen(aStr) - aLen + 3];
      nsAutoString newStr;
      newStr.AppendLiteral("...");
      newStr += ptr;
      nsMemory::Free(aStr);
      aStr = ToNewUnicode(newStr);
    } else {
      nsAutoString newStr(aStr);
      newStr.SetLength(aLen-3);
      newStr.AppendLiteral("...");
      nsMemory::Free(aStr);
      aStr = ToNewUnicode(newStr);
    }
  }
}

static bool
DocHasPrintCallbackCanvas(nsIDocument* aDoc, void* aData)
{
  if (!aDoc) {
    return true;
  }
  Element* root = aDoc->GetRootElement();
  if (!root) {
    return true;
  }
  nsRefPtr<nsContentList> canvases = NS_GetContentList(root,
                                                       kNameSpaceID_XHTML,
                                                       NS_LITERAL_STRING("canvas"));
  uint32_t canvasCount = canvases->Length(true);
  for (uint32_t i = 0; i < canvasCount; ++i) {
    nsCOMPtr<nsIDOMHTMLCanvasElement> canvas = do_QueryInterface(canvases->Item(i, false));
    nsCOMPtr<nsIPrintCallback> printCallback;
    if (canvas && NS_SUCCEEDED(canvas->GetMozPrintCallback(getter_AddRefs(printCallback))) &&
        printCallback) {
      
      
      *static_cast<bool*>(aData) = true;
      return false;
    }
  }
  return true;
}

static bool
DocHasPrintCallbackCanvas(nsIDocument* aDoc)
{
  bool result = false;
  aDoc->EnumerateSubDocuments(&DocHasPrintCallbackCanvas, static_cast<void*>(&result));
  return result;
}





bool
nsPrintEngine::HasPrintCallbackCanvas()
{
  if (!mDocument) {
    return false;
  }
  
  bool result = false;
  DocHasPrintCallbackCanvas(mDocument, static_cast<void*>(&result));
  
  return result || DocHasPrintCallbackCanvas(mDocument);
}


bool
nsPrintEngine::PrePrintPage()
{
  NS_ASSERTION(mPageSeqFrame,  "mPageSeqFrame is null!");
  NS_ASSERTION(mPrt,           "mPrt is null!");

  
  
  if (!mPrt || !mPageSeqFrame) {
    return true; 
  }

  
  bool isCancelled = false;
  mPrt->mPrintSettings->GetIsCancelled(&isCancelled);
  if (isCancelled)
    return true;

  
  
  bool done = false;
  nsresult rv = mPageSeqFrame->PrePrintNextPage(mPagePrintTimer, &done);
  if (NS_FAILED(rv)) {
    
    
    
    
    if (rv != NS_ERROR_ABORT) {
      ShowPrintErrorDialog(rv);
      mPrt->mIsAborted = true;
    }
    done = true;
  }
  return done;
}

bool
nsPrintEngine::PrintPage(nsPrintObject*    aPO,
                         bool&           aInRange)
{
  NS_ASSERTION(aPO,            "aPO is null!");
  NS_ASSERTION(mPageSeqFrame,  "mPageSeqFrame is null!");
  NS_ASSERTION(mPrt,           "mPrt is null!");

  
  
  if (!mPrt || !aPO || !mPageSeqFrame) {
    ShowPrintErrorDialog(NS_ERROR_FAILURE);
    return true; 
  }

  PR_PL(("-----------------------------------\n"));
  PR_PL(("------ In DV::PrintPage PO: %p (%s)\n", aPO, gFrameTypesStr[aPO->mFrameType]));

  
  bool isCancelled = false;
  mPrt->mPrintSettings->GetIsCancelled(&isCancelled);
  if (isCancelled || mPrt->mIsAborted)
    return true;

  int32_t pageNum, numPages, endPage;
  mPageSeqFrame->GetCurrentPageNum(&pageNum);
  mPageSeqFrame->GetNumPages(&numPages);

  bool donePrinting;
  bool isDoingPrintRange;
  mPageSeqFrame->IsDoingPrintRange(&isDoingPrintRange);
  if (isDoingPrintRange) {
    int32_t fromPage;
    int32_t toPage;
    mPageSeqFrame->GetPrintRange(&fromPage, &toPage);

    if (fromPage > numPages) {
      return true;
    }
    if (toPage > numPages) {
      toPage = numPages;
    }

    PR_PL(("****** Printing Page %d printing from %d to page %d\n", pageNum, fromPage, toPage));

    donePrinting = pageNum >= toPage;
    aInRange = pageNum >= fromPage && pageNum <= toPage;
    endPage = (toPage - fromPage)+1;
  } else {
    PR_PL(("****** Printing Page %d of %d page(s)\n", pageNum, numPages));

    donePrinting = pageNum >= numPages;
    endPage = numPages;
    aInRange = true;
  }

  
  
  if (mPrt->mPrintFrameType == nsIPrintSettings::kEachFrameSep)
    endPage = mPrt->mNumPrintablePages;
  
  mPrt->DoOnProgressChange(++mPrt->mNumPagesPrinted, endPage, false, 0);

  
  
  
  
  
  
  
  nsresult rv = mPageSeqFrame->PrintNextPage();
  if (NS_FAILED(rv)) {
    if (rv != NS_ERROR_ABORT) {
      ShowPrintErrorDialog(rv);
      mPrt->mIsAborted = true;
    }
    return true;
  }

  mPageSeqFrame->DoPageEnd();

  return donePrinting;
}




nsresult 
nsPrintEngine::FindSelectionBoundsWithList(nsPresContext* aPresContext,
                                           nsRenderingContext& aRC,
                                           nsFrameList::Enumerator& aChildFrames,
                                           nsIFrame *      aParentFrame,
                                           nsRect&         aRect,
                                           nsIFrame *&     aStartFrame,
                                           nsRect&         aStartRect,
                                           nsIFrame *&     aEndFrame,
                                           nsRect&         aEndRect)
{
  NS_ASSERTION(aPresContext, "Pointer is null!");
  NS_ASSERTION(aParentFrame, "Pointer is null!");

  aRect += aParentFrame->GetPosition();
  for (; !aChildFrames.AtEnd(); aChildFrames.Next()) {
    nsIFrame* child = aChildFrames.get();
    if (child->IsSelected() && child->IsVisibleForPainting()) {
      nsRect r = child->GetRect();
      if (aStartFrame == nullptr) {
        aStartFrame = child;
        aStartRect.SetRect(aRect.x + r.x, aRect.y + r.y, r.width, r.height);
      } else {
        aEndFrame = child;
        aEndRect.SetRect(aRect.x + r.x, aRect.y + r.y, r.width, r.height);
      }
    }
    FindSelectionBounds(aPresContext, aRC, child, aRect, aStartFrame, aStartRect, aEndFrame, aEndRect);
    child = child->GetNextSibling();
  }
  aRect -= aParentFrame->GetPosition();
  return NS_OK;
}



nsresult 
nsPrintEngine::FindSelectionBounds(nsPresContext* aPresContext,
                                   nsRenderingContext& aRC,
                                   nsIFrame *      aParentFrame,
                                   nsRect&         aRect,
                                   nsIFrame *&     aStartFrame,
                                   nsRect&         aStartRect,
                                   nsIFrame *&     aEndFrame,
                                   nsRect&         aEndRect)
{
  NS_ASSERTION(aPresContext, "Pointer is null!");
  NS_ASSERTION(aParentFrame, "Pointer is null!");

  
  nsIFrame::ChildListIterator lists(aParentFrame);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    nsresult rv = FindSelectionBoundsWithList(aPresContext, aRC, childFrames, aParentFrame, aRect, aStartFrame, aStartRect, aEndFrame, aEndRect);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}







nsresult 
nsPrintEngine::GetPageRangeForSelection(nsIPresShell *        aPresShell,
                                        nsPresContext*       aPresContext,
                                        nsRenderingContext&  aRC,
                                        nsISelection*         aSelection,
                                        nsIPageSequenceFrame* aPageSeqFrame,
                                        nsIFrame**            aStartFrame,
                                        int32_t&              aStartPageNum,
                                        nsRect&               aStartRect,
                                        nsIFrame**            aEndFrame,
                                        int32_t&              aEndPageNum,
                                        nsRect&               aEndRect)
{
  NS_ASSERTION(aPresShell, "Pointer is null!");
  NS_ASSERTION(aPresContext, "Pointer is null!");
  NS_ASSERTION(aSelection, "Pointer is null!");
  NS_ASSERTION(aPageSeqFrame, "Pointer is null!");
  NS_ASSERTION(aStartFrame, "Pointer is null!");
  NS_ASSERTION(aEndFrame, "Pointer is null!");

  nsIFrame * seqFrame = do_QueryFrame(aPageSeqFrame);
  if (!seqFrame) {
    return NS_ERROR_FAILURE;
  }

  nsIFrame * startFrame = nullptr;
  nsIFrame * endFrame   = nullptr;

  
  
  
  nsRect r = seqFrame->GetRect();
  FindSelectionBounds(aPresContext, aRC, seqFrame, r,
                      startFrame, aStartRect, endFrame, aEndRect);

#ifdef DEBUG_rodsX
  printf("Start Frame: %p\n", startFrame);
  printf("End Frame:   %p\n", endFrame);
#endif

  
  
  aStartPageNum = -1;
  aEndPageNum   = -1;

  nsIFrame * startPageFrame;
  nsIFrame * endPageFrame;

  
  if (startFrame != nullptr) {
    
    
    
    
    
    if (endFrame == nullptr) {
      
      
      
      
      startPageFrame = nsLayoutUtils::GetPageFrame(startFrame);
      endPageFrame   = startPageFrame;
      aEndRect       = aStartRect;
    } else {
      startPageFrame = nsLayoutUtils::GetPageFrame(startFrame);
      endPageFrame   = nsLayoutUtils::GetPageFrame(endFrame);
    }
  } else {
    return NS_ERROR_FAILURE;
  }

#ifdef DEBUG_rodsX
  printf("Start Page: %p\n", startPageFrame);
  printf("End Page:   %p\n", endPageFrame);

  
  {
  int32_t pageNum = 1;
  nsIFrame* child = seqFrame->GetFirstPrincipalChild();
  while (child != nullptr) {
    printf("Page: %d - %p\n", pageNum, child);
    pageNum++;
    child = child->GetNextSibling();
  }
  }
#endif

  
  
  int32_t pageNum = 1;
  nsIFrame* page = seqFrame->GetFirstPrincipalChild();
  while (page != nullptr) {
    if (page == startPageFrame) {
      aStartPageNum = pageNum;
    }
    if (page == endPageFrame) {
      aEndPageNum = pageNum;
    }
    pageNum++;
    page = page->GetNextSibling();
  }

#ifdef DEBUG_rodsX
  printf("Start Page No: %d\n", aStartPageNum);
  printf("End Page No:   %d\n", aEndPageNum);
#endif

  *aStartFrame = startPageFrame;
  *aEndFrame   = endPageFrame;

  return NS_OK;
}











void nsPrintEngine::SetIsPrinting(bool aIsPrinting)
{ 
  mIsDoingPrinting = aIsPrinting;
  
  
  if (!mIsDoingPrintPreview && mDocViewerPrint) {
    mDocViewerPrint->SetIsPrinting(aIsPrinting);
  }
  if (mPrt && aIsPrinting) {
    mPrt->mPreparingForPrint = true;
  }
}


void nsPrintEngine::SetIsPrintPreview(bool aIsPrintPreview) 
{ 
  mIsDoingPrintPreview = aIsPrintPreview; 

  if (mDocViewerPrint) {
    mDocViewerPrint->SetIsPrintPreview(aIsPrintPreview);
  }
}


void
nsPrintEngine::CleanupDocTitleArray(PRUnichar**& aArray, int32_t& aCount)
{
  for (int32_t i = aCount - 1; i >= 0; i--) {
    nsMemory::Free(aArray[i]);
  }
  nsMemory::Free(aArray);
  aArray = NULL;
  aCount = 0;
}



bool nsPrintEngine::HasFramesetChild(nsIContent* aContent)
{
  if (!aContent) {
    return false;
  }

  
  for (nsIContent* child = aContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->IsHTML(nsGkAtoms::frameset)) {
      return true;
    }
  }

  return false;
}
 





already_AddRefed<nsIDOMWindow>
nsPrintEngine::FindFocusedDOMWindow()
{
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, nullptr);

  nsCOMPtr<nsPIDOMWindow> window(mDocument->GetWindow());
  NS_ENSURE_TRUE(window, nullptr);

  nsCOMPtr<nsPIDOMWindow> rootWindow = window->GetPrivateRoot();
  NS_ENSURE_TRUE(rootWindow, nullptr);

  nsPIDOMWindow* focusedWindow;
  nsFocusManager::GetFocusedDescendant(rootWindow, true, &focusedWindow);
  NS_ENSURE_TRUE(focusedWindow, nullptr);

  if (IsWindowsInOurSubTree(focusedWindow)) {
    return focusedWindow;
  }

  NS_IF_RELEASE(focusedWindow);
  return nullptr;
}


bool
nsPrintEngine::IsWindowsInOurSubTree(nsPIDOMWindow * window)
{
  bool found = false;

  
  if (window) {
    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem =
      do_QueryInterface(window->GetDocShell());

    if (docShellAsItem) {
      
      nsCOMPtr<nsIDocShell> thisDVDocShell(do_QueryReferent(mContainer));
      while (!found) {
        nsCOMPtr<nsIDocShell> parentDocshell(do_QueryInterface(docShellAsItem));
        if (parentDocshell) {
          if (parentDocshell == thisDVDocShell) {
            found = true;
            break;
          }
        } else {
          break; 
        }
        nsCOMPtr<nsIDocShellTreeItem> docShellParent;
        docShellAsItem->GetSameTypeParent(getter_AddRefs(docShellParent));
        docShellAsItem = docShellParent;
      } 
    }
  } 

  return found;
}


bool
nsPrintEngine::DonePrintingPages(nsPrintObject* aPO, nsresult aResult)
{
  
  PR_PL(("****** In DV::DonePrintingPages PO: %p (%s)\n", aPO, aPO?gFrameTypesStr[aPO->mFrameType]:""));

  
  
  
  if (mPageSeqFrame) {
    mPageSeqFrame->ResetPrintCanvasList();
  }

  if (aPO && !mPrt->mIsAborted) {
    aPO->mHasBeenPrinted = true;
    nsresult rv;
    bool didPrint = PrintDocContent(mPrt->mPrintObject, rv);
    if (NS_SUCCEEDED(rv) && didPrint) {
      PR_PL(("****** In DV::DonePrintingPages PO: %p (%s) didPrint:%s (Not Done Printing)\n", aPO, gFrameTypesStr[aPO->mFrameType], PRT_YESNO(didPrint)));
      return false;
    }
  }

  if (NS_SUCCEEDED(aResult)) {
    FirePrintCompletionEvent();
  }

  TurnScriptingOn(true);
  SetIsPrinting(false);

  
  
  NS_IF_RELEASE(mPagePrintTimer);

  return true;
}




void
nsPrintEngine::SetPrintAsIs(nsPrintObject* aPO, bool aAsIs)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  aPO->mPrintAsIs = aAsIs;
  for (uint32_t i=0;i<aPO->mKids.Length();i++) {
    SetPrintAsIs(aPO->mKids[i], aAsIs);
  }
}



nsPrintObject*
nsPrintEngine::FindPrintObjectByDOMWin(nsPrintObject* aPO,
                                       nsIDOMWindow* aDOMWin)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  
  
  if (!aDOMWin) {
    return nullptr;
  }

  nsCOMPtr<nsIDOMDocument> domDoc;
  aDOMWin->GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if (aPO->mDocument && aPO->mDocument->GetOriginalDocument() == doc) {
    return aPO;
  }

  int32_t cnt = aPO->mKids.Length();
  for (int32_t i = 0; i < cnt; ++i) {
    nsPrintObject* po = FindPrintObjectByDOMWin(aPO->mKids[i], aDOMWin);
    if (po) {
      return po;
    }
  }

  return nullptr;
}


nsresult
nsPrintEngine::EnablePOsForPrinting()
{
  
  
  mPrt->mSelectedPO = nullptr;

  if (mPrt->mPrintSettings == nullptr) {
    return NS_ERROR_FAILURE;
  }

  mPrt->mPrintFrameType = nsIPrintSettings::kNoFrames;
  mPrt->mPrintSettings->GetPrintFrameType(&mPrt->mPrintFrameType);

  int16_t printHowEnable = nsIPrintSettings::kFrameEnableNone;
  mPrt->mPrintSettings->GetHowToEnableFrameUI(&printHowEnable);

  int16_t printRangeType = nsIPrintSettings::kRangeAllPages;
  mPrt->mPrintSettings->GetPrintRange(&printRangeType);

  PR_PL(("\n"));
  PR_PL(("********* nsPrintEngine::EnablePOsForPrinting *********\n"));
  PR_PL(("PrintFrameType:     %s \n", gPrintFrameTypeStr[mPrt->mPrintFrameType]));
  PR_PL(("HowToEnableFrameUI: %s \n", gFrameHowToEnableStr[printHowEnable]));
  PR_PL(("PrintRange:         %s \n", gPrintRangeStr[printRangeType]));
  PR_PL(("----\n"));

  
  
  
  if (printRangeType == nsIPrintSettings::kRangeSelection) {
    mPrt->mPrintFrameType = nsIPrintSettings::kSelectedFrame;
    printHowEnable        = nsIPrintSettings::kFrameEnableNone;
  }

  
  
  
  
  
  if (printHowEnable == nsIPrintSettings::kFrameEnableNone) {

    
    if (printRangeType == nsIPrintSettings::kRangeAllPages ||
        printRangeType == nsIPrintSettings::kRangeSpecifiedPageRange) {
      SetPrintPO(mPrt->mPrintObject, true);

      
      
      if (mPrt->mPrintObject->mKids.Length() > 0) {
        for (uint32_t i=0;i<mPrt->mPrintObject->mKids.Length();i++) {
          nsPrintObject* po = mPrt->mPrintObject->mKids[i];
          NS_ASSERTION(po, "nsPrintObject can't be null!");
          SetPrintAsIs(po);
        }

        
        mPrt->mPrintFrameType = nsIPrintSettings::kFramesAsIs;
      }
      PR_PL(("PrintFrameType:     %s \n", gPrintFrameTypeStr[mPrt->mPrintFrameType]));
      PR_PL(("HowToEnableFrameUI: %s \n", gFrameHowToEnableStr[printHowEnable]));
      PR_PL(("PrintRange:         %s \n", gPrintRangeStr[printRangeType]));
      return NS_OK;
    }

    
    
    if (printRangeType == nsIPrintSettings::kRangeSelection) {

      
      if (mPrt->mCurrentFocusWin) {
        
        nsPrintObject * po = FindPrintObjectByDOMWin(mPrt->mPrintObject, mPrt->mCurrentFocusWin);
        if (po != nullptr) {
          mPrt->mSelectedPO = po;
          
          SetPrintAsIs(po);

          
          SetPrintPO(po, true);

          
          
          
          
          
          
          
          nsCOMPtr<nsIDOMWindow> domWin =
            do_QueryInterface(po->mDocument->GetOriginalDocument()->GetWindow());
          if (!IsThereARangeSelection(domWin)) {
            printRangeType = nsIPrintSettings::kRangeAllPages;
            mPrt->mPrintSettings->SetPrintRange(printRangeType);
          }
          PR_PL(("PrintFrameType:     %s \n", gPrintFrameTypeStr[mPrt->mPrintFrameType]));
          PR_PL(("HowToEnableFrameUI: %s \n", gFrameHowToEnableStr[printHowEnable]));
          PR_PL(("PrintRange:         %s \n", gPrintRangeStr[printRangeType]));
          return NS_OK;
        }
      } else {
        for (uint32_t i=0;i<mPrt->mPrintDocList.Length();i++) {
          nsPrintObject* po = mPrt->mPrintDocList.ElementAt(i);
          NS_ASSERTION(po, "nsPrintObject can't be null!");
          nsCOMPtr<nsIDOMWindow> domWin = do_GetInterface(po->mDocShell);
          if (IsThereARangeSelection(domWin)) {
            mPrt->mCurrentFocusWin = domWin;
            SetPrintPO(po, true);
            break;
          }
        }
        return NS_OK;
      }
    }
  }

  
  if (printRangeType == nsIPrintSettings::kRangeSelection) {
    
    if (mPrt->mCurrentFocusWin) {
      
      nsPrintObject * po = FindPrintObjectByDOMWin(mPrt->mPrintObject, mPrt->mCurrentFocusWin);
      if (po != nullptr) {
        mPrt->mSelectedPO = po;
        
        SetPrintAsIs(po);

        
        SetPrintPO(po, true);

        
        
        
        
        
        
        
        nsCOMPtr<nsIDOMWindow> domWin =
          do_QueryInterface(po->mDocument->GetOriginalDocument()->GetWindow());
        if (!IsThereARangeSelection(domWin)) {
          printRangeType = nsIPrintSettings::kRangeAllPages;
          mPrt->mPrintSettings->SetPrintRange(printRangeType);
        }
        PR_PL(("PrintFrameType:     %s \n", gPrintFrameTypeStr[mPrt->mPrintFrameType]));
        PR_PL(("HowToEnableFrameUI: %s \n", gFrameHowToEnableStr[printHowEnable]));
        PR_PL(("PrintRange:         %s \n", gPrintRangeStr[printRangeType]));
        return NS_OK;
      }
    }
  }

  
  if (mPrt->mPrintFrameType == nsIPrintSettings::kFramesAsIs) {
    SetPrintAsIs(mPrt->mPrintObject);
    SetPrintPO(mPrt->mPrintObject, true);
    return NS_OK;
  }

  
  
  
  if (mPrt->mPrintFrameType == nsIPrintSettings::kSelectedFrame) {

    if ((mPrt->mIsParentAFrameSet && mPrt->mCurrentFocusWin) || mPrt->mIsIFrameSelected) {
      nsPrintObject * po = FindPrintObjectByDOMWin(mPrt->mPrintObject, mPrt->mCurrentFocusWin);
      if (po != nullptr) {
        mPrt->mSelectedPO = po;
        
        
        
        if (po->mKids.Length() > 0) {
          
          SetPrintAsIs(po);
        }

        
        SetPrintPO(po, true);
      }
    }
    return NS_OK;
  }

  
  
  if (mPrt->mPrintFrameType == nsIPrintSettings::kEachFrameSep) {
    SetPrintPO(mPrt->mPrintObject, true);
    int32_t cnt = mPrt->mPrintDocList.Length();
    for (int32_t i=0;i<cnt;i++) {
      nsPrintObject* po = mPrt->mPrintDocList.ElementAt(i);
      NS_ASSERTION(po, "nsPrintObject can't be null!");
      if (po->mFrameType == eFrameSet) {
        po->mDontPrint = true;
      }
    }
  }

  return NS_OK;
}




nsPrintObject*
nsPrintEngine::FindSmallestSTF()
{
  float smallestRatio = 1.0f;
  nsPrintObject* smallestPO = nullptr;

  for (uint32_t i=0;i<mPrt->mPrintDocList.Length();i++) {
    nsPrintObject* po = mPrt->mPrintDocList.ElementAt(i);
    NS_ASSERTION(po, "nsPrintObject can't be null!");
    if (po->mFrameType != eFrameSet && po->mFrameType != eIFrame) {
      if (po->mShrinkRatio < smallestRatio) {
        smallestRatio = po->mShrinkRatio;
        smallestPO    = po;
      }
    }
  }

#ifdef EXTENDED_DEBUG_PRINTING
  if (smallestPO) printf("*PO: %p  Type: %d  %10.3f\n", smallestPO, smallestPO->mFrameType, smallestPO->mShrinkRatio);
#endif
  return smallestPO;
}


void
nsPrintEngine::TurnScriptingOn(bool aDoTurnOn)
{
  if (mIsDoingPrinting && aDoTurnOn && mDocViewerPrint &&
      mDocViewerPrint->GetIsPrintPreview()) {
    
    
    return;
  }

  nsPrintData* prt = mPrt;
#ifdef NS_PRINT_PREVIEW
  if (!prt) {
    prt = mPrtPreview;
  }
#endif
  if (!prt) {
    return;
  }

  NS_ASSERTION(mDocument, "We MUST have a document.");
  

  for (uint32_t i=0;i<prt->mPrintDocList.Length();i++) {
    nsPrintObject* po = prt->mPrintDocList.ElementAt(i);
    NS_ASSERTION(po, "nsPrintObject can't be null!");

    nsIDocument* doc = po->mDocument;
    if (!doc) {
      continue;
    }

    
    nsIScriptGlobalObject *scriptGlobalObj = doc->GetScriptGlobalObject();

    if (scriptGlobalObj) {
      nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(scriptGlobalObj);
      NS_ASSERTION(window, "Can't get nsPIDOMWindow");
      nsIScriptContext *scx = scriptGlobalObj->GetContext();
      NS_WARN_IF_FALSE(scx, "Can't get nsIScriptContext");
      nsresult propThere = NS_PROPTABLE_PROP_NOT_THERE;
      doc->GetProperty(nsGkAtoms::scriptEnabledBeforePrintOrPreview,
                       &propThere);
      if (aDoTurnOn) {
        if (propThere != NS_PROPTABLE_PROP_NOT_THERE) {
          doc->DeleteProperty(nsGkAtoms::scriptEnabledBeforePrintOrPreview);
          if (scx) {
            scx->SetScriptsEnabled(true, false);
          }
          window->ResumeTimeouts(false);
        }
      } else {
        
        
        
        if (propThere == NS_PROPTABLE_PROP_NOT_THERE) {
          
          
          doc->SetProperty(nsGkAtoms::scriptEnabledBeforePrintOrPreview,
                           NS_INT32_TO_PTR(doc->IsScriptEnabled()));
          if (scx) {
            scx->SetScriptsEnabled(false, false);
          }
          window->SuspendTimeouts(1, false);
        }
      }
    }
  }
}











void
nsPrintEngine::CloseProgressDialog(nsIWebProgressListener* aWebProgressListener)
{
  if (aWebProgressListener) {
    aWebProgressListener->OnStateChange(nullptr, nullptr, nsIWebProgressListener::STATE_STOP|nsIWebProgressListener::STATE_IS_DOCUMENT, NS_OK);
  }
}


nsresult
nsPrintEngine::FinishPrintPreview()
{
  nsresult rv = NS_OK;

#ifdef NS_PRINT_PREVIEW

  if (!mPrt) {
    
    return rv;
  }

  rv = DocumentReadyForPrinting();

  SetIsCreatingPrintPreview(false);

  
  if (NS_FAILED(rv)) {
    


    mPrt->OnEndPrinting();
    TurnScriptingOn(true);

    return rv;
  }

  
  


  if (mIsDoingPrintPreview && mOldPrtPreview) {
    delete mOldPrtPreview;
    mOldPrtPreview = nullptr;
  }


  mPrt->OnEndPrinting();

  
  
  mPrtPreview = mPrt;
  mPrt        = nullptr;

#endif 

  return NS_OK;
}







nsresult
nsPrintEngine::StartPagePrintTimer(nsPrintObject* aPO)
{
  if (!mPagePrintTimer) {
    
    
    int32_t printPageDelay = 50;
    mPrt->mPrintSettings->GetPrintPageDelay(&printPageDelay);

    nsRefPtr<nsPagePrintTimer> timer =
      new nsPagePrintTimer(this, mDocViewerPrint, printPageDelay);
    timer.forget(&mPagePrintTimer);
  }

  return mPagePrintTimer->Start(aPO);
}


NS_IMETHODIMP 
nsPrintEngine::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
  nsresult rv = NS_ERROR_FAILURE;

  rv = InitPrintDocConstruction(true);
  if (!mIsDoingPrinting && mPrtPreview) {
      mPrtPreview->OnEndPrinting();
  }

  return rv;

}




class nsPrintCompletionEvent : public nsRunnable {
public:
  nsPrintCompletionEvent(nsIDocumentViewerPrint *docViewerPrint)
    : mDocViewerPrint(docViewerPrint) {
    NS_ASSERTION(mDocViewerPrint, "mDocViewerPrint is null.");
  }

  NS_IMETHOD Run() {
    if (mDocViewerPrint)
      mDocViewerPrint->OnDonePrinting();
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDocumentViewerPrint> mDocViewerPrint;
};


void
nsPrintEngine::FirePrintCompletionEvent()
{
  nsCOMPtr<nsIRunnable> event = new nsPrintCompletionEvent(mDocViewerPrint);
  if (NS_FAILED(NS_DispatchToCurrentThread(event)))
    NS_WARNING("failed to dispatch print completion event");
}






#if (defined(XP_WIN) || defined(XP_OS2)) && defined(EXTENDED_DEBUG_PRINTING)
#include "windows.h"
#include "process.h"
#include "direct.h"

#define MY_FINDFIRST(a,b) FindFirstFile(a,b)
#define MY_FINDNEXT(a,b) FindNextFile(a,b)
#define ISDIR(a) (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#define MY_FINDCLOSE(a) FindClose(a)
#define MY_FILENAME(a) a.cFileName
#define MY_FILESIZE(a) (a.nFileSizeHigh * MAXDWORD) + a.nFileSizeLow

int RemoveFilesInDir(const char * aDir)
{
	WIN32_FIND_DATA data_ptr;
	HANDLE find_handle;

  char path[MAX_PATH];

  strcpy(path, aDir);

	
	if (path[strlen(path)-1] != '\\')
    strcat(path, "\\");

  char findPath[MAX_PATH];
  strcpy(findPath, path);
  strcat(findPath, "*.*");

	find_handle = MY_FINDFIRST(findPath, &data_ptr);

	if (find_handle != INVALID_HANDLE_VALUE) {
		do  {
			if (ISDIR(data_ptr)
				&& (stricmp(MY_FILENAME(data_ptr),"."))
				&& (stricmp(MY_FILENAME(data_ptr),".."))) {
					
			}
			else if (!ISDIR(data_ptr)) {
        if (!strncmp(MY_FILENAME(data_ptr), "print_dump", 10)) {
          char fileName[MAX_PATH];
          strcpy(fileName, aDir);
          strcat(fileName, "\\");
          strcat(fileName, MY_FILENAME(data_ptr));
				  printf("Removing %s\n", fileName);
          remove(fileName);
        }
			}
		} while(MY_FINDNEXT(find_handle,&data_ptr));
		MY_FINDCLOSE(find_handle);
	}
	return TRUE;
}
#endif

#ifdef EXTENDED_DEBUG_PRINTING




static void RootFrameList(nsPresContext* aPresContext, FILE* out, int32_t aIndent)
{
  if (!aPresContext || !out)
    return;

  nsIPresShell *shell = aPresContext->GetPresShell();
  if (shell) {
    nsIFrame* frame = shell->FrameManager()->GetRootFrame();
    if (frame) {
      frame->List(aPresContext, out, aIndent);
    }
  }
}




static void DumpFrames(FILE*                 out,
                       nsPresContext*       aPresContext,
                       nsRenderingContext * aRendContext,
                       nsIFrame *            aFrame,
                       int32_t               aLevel)
{
  NS_ASSERTION(out, "Pointer is null!");
  NS_ASSERTION(aPresContext, "Pointer is null!");
  NS_ASSERTION(aRendContext, "Pointer is null!");
  NS_ASSERTION(aFrame, "Pointer is null!");

  nsIFrame* child = aFrame->GetFirstPrincipalChild();
  while (child != nullptr) {
    for (int32_t i=0;i<aLevel;i++) {
     fprintf(out, "  ");
    }
    nsAutoString tmp;
    child->GetFrameName(tmp);
    fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
    bool isSelected;
    if (NS_SUCCEEDED(child->IsVisibleForPainting(aPresContext, *aRendContext, true, &isSelected))) {
      fprintf(out, " %p %s", child, isSelected?"VIS":"UVS");
      nsRect rect = child->GetRect();
      fprintf(out, "[%d,%d,%d,%d] ", rect.x, rect.y, rect.width, rect.height);
      fprintf(out, "v: %p ", (void*)child->GetView());
      fprintf(out, "\n");
      DumpFrames(out, aPresContext, aRendContext, child, aLevel+1);
      child = child->GetNextSibling();
    }
  }
}





static void
DumpViews(nsIDocShell* aDocShell, FILE* out)
{
  NS_ASSERTION(aDocShell, "Pointer is null!");
  NS_ASSERTION(out, "Pointer is null!");

  if (nullptr != aDocShell) {
    fprintf(out, "docshell=%p \n", aDocShell);
    nsIPresShell* shell = nsPrintEngine::GetPresShellFor(aDocShell);
    if (shell) {
      nsIViewManager* vm = shell->GetViewManager();
      if (vm) {
        nsIView* root = vm->GetRootView();
        if (root) {
          root->List(out);
        }
      }
    }
    else {
      fputs("null pres shell\n", out);
    }

    
    int32_t i, n;
    nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(aDocShell));
    docShellAsNode->GetChildCount(&n);
    for (i = 0; i < n; i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      docShellAsNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));
      if (childAsShell) {
        DumpViews(childAsShell, out);
      }
    }
  }
}




void DumpLayoutData(char*              aTitleStr,
                    char*              aURLStr,
                    nsPresContext*    aPresContext,
                    nsDeviceContext * aDC,
                    nsIFrame *         aRootFrame,
                    nsIDocShekk *      aDocShell,
                    FILE*              aFD = nullptr)
{
  if (!kPrintingLogMod || kPrintingLogMod->level != DUMP_LAYOUT_LEVEL) return;

  if (aPresContext == nullptr || aDC == nullptr) {
    return;
  }

#ifdef NS_PRINT_PREVIEW
  if (aPresContext->Type() == nsPresContext::eContext_PrintPreview) {
    return;
  }
#endif

  NS_ASSERTION(aRootFrame, "Pointer is null!");
  NS_ASSERTION(aDocShell, "Pointer is null!");

  
  char filename[256];
  sprintf(filename, "print_dump_layout_%d.txt", gDumpLOFileNameCnt++);
  FILE * fd = aFD?aFD:fopen(filename, "w");
  if (fd) {
    fprintf(fd, "Title: %s\n", aTitleStr?aTitleStr:"");
    fprintf(fd, "URL:   %s\n", aURLStr?aURLStr:"");
    fprintf(fd, "--------------- Frames ----------------\n");
    fprintf(fd, "--------------- Frames ----------------\n");
    nsRefPtr<nsRenderingContext> renderingContext;
    aDC->CreateRenderingContext(*getter_AddRefs(renderingContext));
    RootFrameList(aPresContext, fd, 0);
    
    fprintf(fd, "---------------------------------------\n\n");
    fprintf(fd, "--------------- Views From Root Frame----------------\n");
    nsIView* v = aRootFrame->GetView();
    if (v) {
      v->List(fd);
    } else {
      printf("View is null!\n");
    }
    if (aDocShell) {
      fprintf(fd, "--------------- All Views ----------------\n");
      DumpViews(aDocShell, fd);
      fprintf(fd, "---------------------------------------\n\n");
    }
    if (aFD == nullptr) {
      fclose(fd);
    }
  }
}


static void DumpPrintObjectsList(nsTArray<nsPrintObject*> * aDocList)
{
  if (!kPrintingLogMod || kPrintingLogMod->level != DUMP_LAYOUT_LEVEL) return;

  NS_ASSERTION(aDocList, "Pointer is null!");

  const char types[][3] = {"DC", "FR", "IF", "FS"};
  PR_PL(("Doc List\n***************************************************\n"));
  PR_PL(("T  P A H    PO    DocShell   Seq     Page      Root     Page#    Rect\n"));
  int32_t cnt = aDocList->Length();
  for (int32_t i=0;i<cnt;i++) {
    nsPrintObject* po = aDocList->ElementAt(i);
    NS_ASSERTION(po, "nsPrintObject can't be null!");
    nsIFrame* rootFrame = nullptr;
    if (po->mPresShell) {
      rootFrame = po->mPresShell->FrameManager()->GetRootFrame();
      while (rootFrame != nullptr) {
        nsIPageSequenceFrame * sqf = do_QueryFrame(rootFrame);
        if (sqf) {
          break;
        }
        rootFrame = rootFrame->GetFirstPrincipalChild();
      }
    }

    PR_PL(("%s %d %d %d %p %p %p %p %p   %d   %d,%d,%d,%d\n", types[po->mFrameType],
            po->IsPrintable(), po->mPrintAsIs, po->mHasBeenPrinted, po, po->mDocShell.get(), po->mSeqFrame,
            po->mPageFrame, rootFrame, po->mPageNum, po->mRect.x, po->mRect.y, po->mRect.width, po->mRect.height));
  }
}


static void DumpPrintObjectsTree(nsPrintObject * aPO, int aLevel, FILE* aFD)
{
  if (!kPrintingLogMod || kPrintingLogMod->level != DUMP_LAYOUT_LEVEL) return;

  NS_ASSERTION(aPO, "Pointer is null!");

  FILE * fd = aFD?aFD:stdout;
  const char types[][3] = {"DC", "FR", "IF", "FS"};
  if (aLevel == 0) {
    fprintf(fd, "DocTree\n***************************************************\n");
    fprintf(fd, "T     PO    DocShell   Seq      Page     Page#    Rect\n");
  }
  int32_t cnt = aPO->mKids.Length();
  for (int32_t i=0;i<cnt;i++) {
    nsPrintObject* po = aPO->mKids.ElementAt(i);
    NS_ASSERTION(po, "nsPrintObject can't be null!");
    for (int32_t k=0;k<aLevel;k++) fprintf(fd, "  ");
    fprintf(fd, "%s %p %p %p %p %d %d,%d,%d,%d\n", types[po->mFrameType], po, po->mDocShell.get(), po->mSeqFrame,
           po->mPageFrame, po->mPageNum, po->mRect.x, po->mRect.y, po->mRect.width, po->mRect.height);
  }
}


static void GetDocTitleAndURL(nsPrintObject* aPO, char *& aDocStr, char *& aURLStr)
{
  aDocStr = nullptr;
  aURLStr = nullptr;

  PRUnichar * docTitleStr;
  PRUnichar * docURLStr;
  nsPrintEngine::GetDisplayTitleAndURL(aPO,
                                            &docTitleStr, &docURLStr,
                                            nsPrintEngine::eDocTitleDefURLDoc); 

  if (docTitleStr) {
    nsAutoString strDocTitle(docTitleStr);
    aDocStr = ToNewCString(strDocTitle);
    nsMemory::Free(docTitleStr);
  }

  if (docURLStr) {
    nsAutoString strURL(docURLStr);
    aURLStr = ToNewCString(strURL);
    nsMemory::Free(docURLStr);
  }
}


static void DumpPrintObjectsTreeLayout(nsPrintObject * aPO,
                                       nsDeviceContext * aDC,
                                       int aLevel, FILE * aFD)
{
  if (!kPrintingLogMod || kPrintingLogMod->level != DUMP_LAYOUT_LEVEL) return;

  NS_ASSERTION(aPO, "Pointer is null!");
  NS_ASSERTION(aDC, "Pointer is null!");

  const char types[][3] = {"DC", "FR", "IF", "FS"};
  FILE * fd = nullptr;
  if (aLevel == 0) {
    fd = fopen("tree_layout.txt", "w");
    fprintf(fd, "DocTree\n***************************************************\n");
    fprintf(fd, "***************************************************\n");
    fprintf(fd, "T     PO    DocShell   Seq      Page     Page#    Rect\n");
  } else {
    fd = aFD;
  }
  if (fd) {
    nsIFrame* rootFrame = nullptr;
    if (aPO->mPresShell) {
      rootFrame = aPO->mPresShell->FrameManager()->GetRootFrame();
    }
    for (int32_t k=0;k<aLevel;k++) fprintf(fd, "  ");
    fprintf(fd, "%s %p %p %p %p %d %d,%d,%d,%d\n", types[aPO->mFrameType], aPO, aPO->mDocShell.get(), aPO->mSeqFrame,
           aPO->mPageFrame, aPO->mPageNum, aPO->mRect.x, aPO->mRect.y, aPO->mRect.width, aPO->mRect.height);
    if (aPO->IsPrintable()) {
      char * docStr;
      char * urlStr;
      GetDocTitleAndURL(aPO, docStr, urlStr);
      DumpLayoutData(docStr, urlStr, aPO->mPresContext, aDC, rootFrame, aPO->mDocShell, fd);
      if (docStr) nsMemory::Free(docStr);
      if (urlStr) nsMemory::Free(urlStr);
    }
    fprintf(fd, "<***************************************************>\n");

    int32_t cnt = aPO->mKids.Length();
    for (int32_t i=0;i<cnt;i++) {
      nsPrintObject* po = aPO->mKids.ElementAt(i);
      NS_ASSERTION(po, "nsPrintObject can't be null!");
      DumpPrintObjectsTreeLayout(po, aDC, aLevel+1, fd);
    }
  }
  if (aLevel == 0 && fd) {
    fclose(fd);
  }
}


static void DumpPrintObjectsListStart(const char * aStr, nsTArray<nsPrintObject*> * aDocList)
{
  if (!kPrintingLogMod || kPrintingLogMod->level != DUMP_LAYOUT_LEVEL) return;

  NS_ASSERTION(aStr, "Pointer is null!");
  NS_ASSERTION(aDocList, "Pointer is null!");

  PR_PL(("%s\n", aStr));
  DumpPrintObjectsList(aDocList);
}

#define DUMP_DOC_LIST(_title) DumpPrintObjectsListStart((_title), mPrt->mPrintDocList);
#define DUMP_DOC_TREE DumpPrintObjectsTree(mPrt->mPrintObject);
#define DUMP_DOC_TREELAYOUT DumpPrintObjectsTreeLayout(mPrt->mPrintObject, mPrt->mPrintDC);

#else
#define DUMP_DOC_LIST(_title)
#define DUMP_DOC_TREE
#define DUMP_DOC_TREELAYOUT
#endif






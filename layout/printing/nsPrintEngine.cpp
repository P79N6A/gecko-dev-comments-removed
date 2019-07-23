





































#include "nsPrintEngine.h"

#include "nsIStringBundle.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"

#include "nsISelection.h"
#include "nsIScriptGlobalObject.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIURI.h"
#include "nsContentErrors.h"


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
#include "nsIStringBundle.h"


#include "nsIPrintingPromptService.h"
static const char kPrintingPromptService[] = "@mozilla.org/embedcomp/printingprompt-service;1";


#include "nsPagePrintTimer.h"


#include "nsIDocument.h"


#include "nsIDOMEventTarget.h"
#include "nsIDOMFocusListener.h"
#include "nsISelectionController.h"


#include "nsISupportsUtils.h"
#include "nsIFrame.h"
#include "nsIScriptContext.h"
#include "nsILinkHandler.h"
#include "nsIDOMDocument.h"
#include "nsISelectionListener.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMNSHTMLDocument.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMRange.h"
#include "nsContentCID.h"
#include "nsLayoutCID.h"
#include "nsContentUtils.h"
#include "nsIPresShell.h"
#include "nsLayoutUtils.h"

#include "nsViewsCID.h"
#include "nsWidgetsCID.h"
#include "nsIDeviceContext.h"
#include "nsIDeviceContextSpec.h"
#include "nsIViewManager.h"
#include "nsIView.h"

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
#include "nsIDocShell.h"
#include "nsIBaseWindow.h"
#include "nsILayoutHistoryState.h"
#include "nsFrameManager.h"
#include "nsIParser.h"
#include "nsGUIEvent.h"
#include "nsHTMLReflowState.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIContentViewerContainer.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewerPrint.h"

#include "nsPIDOMWindow.h"
#include "nsFocusManager.h"

#include "nsCDefaultURIFixup.h"
#include "nsIURIFixup.h"



#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "prlog.h"

#ifdef PR_LOGGING

#ifdef NS_DEBUG



#endif

#define DUMP_LAYOUT_LEVEL 9 // this turns on the dumping of each doucment's layout info

static PRLogModuleInfo * kPrintingLogMod = PR_NewLogModule("printing");
#define PR_PL(_p1)  PR_LOG(kPrintingLogMod, PR_LOG_DEBUG, _p1);

#ifdef EXTENDED_DEBUG_PRINTING
static PRUint32 gDumpFileNameCnt   = 0;
static PRUint32 gDumpLOFileNameCnt = 0;
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
static void DumpPrintObjectsTree(nsPrintObject * aPO, int aLevel= 0, FILE* aFD = nsnull);
static void DumpPrintObjectsTreeLayout(nsPrintObject * aPO,nsIDeviceContext * aDC, int aLevel= 0, FILE * aFD = nsnull);

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
  : mPrintEngine(aPrintEngine), mSuppressed(PR_FALSE) {}

  ~nsScriptSuppressor() { Unsuppress(); }

  void Suppress()
  {
    if (mPrintEngine) {
      mSuppressed = PR_TRUE;
      mPrintEngine->TurnScriptingOn(PR_FALSE);
    }
  }
  
  void Unsuppress()
  {
    if (mPrintEngine && mSuppressed) {
      mPrintEngine->TurnScriptingOn(PR_TRUE);
    }
    mSuppressed = PR_FALSE;
  }

  void Disconnect() { mPrintEngine = nsnull; }
protected:
  nsRefPtr<nsPrintEngine> mPrintEngine;
  PRBool                  mSuppressed;
};


static NS_DEFINE_CID(kViewManagerCID,       NS_VIEW_MANAGER_CID);
static NS_DEFINE_CID(kWidgetCID,            NS_CHILD_CID);

NS_IMPL_ISUPPORTS1(nsPrintEngine, nsIObserver)




nsPrintEngine::nsPrintEngine() :
  mIsCreatingPrintPreview(PR_FALSE),
  mIsDoingPrinting(PR_FALSE),
  mIsDoingPrintPreview(PR_FALSE),
  mProgressDialogIsShown(PR_FALSE),
  mContainer(nsnull),
  mScreenDPI(115.0f),
  mPrt(nsnull),
  mPagePrintTimer(nsnull),
  mPageSeqFrame(nsnull),
  mParentWidget(nsnull),
  mPrtPreview(nsnull),
  mOldPrtPreview(nsnull),
  mDebugFile(nsnull)
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
    mPrt = nsnull;
  }

#ifdef NS_PRINT_PREVIEW
  if (mPrtPreview) {
    delete mPrtPreview;
    mPrtPreview = nsnull;
  }

  
  if (mOldPrtPreview) {
    delete mOldPrtPreview;
    mOldPrtPreview = nsnull;
  }

#endif
  mDocViewerPrint = nsnull;
}


void nsPrintEngine::DestroyPrintingData()
{
  if (mPrt) {
    delete mPrt;
    mPrt = nsnull;
  }
}






nsresult nsPrintEngine::Initialize(nsIDocumentViewerPrint* aDocViewerPrint, 
                                   nsISupports*            aContainer,
                                   nsIDocument*            aDocument,
                                   float                   aScreenDPI,
                                   nsIWidget*              aParentWidget,
                                   FILE*                   aDebugFile)
{
  NS_ENSURE_ARG_POINTER(aDocViewerPrint);
  NS_ENSURE_ARG_POINTER(aContainer);
  NS_ENSURE_ARG_POINTER(aDocument);

  mDocViewerPrint = aDocViewerPrint;
  mContainer      = aContainer;      
  mDocument       = aDocument;
  mScreenDPI      = aScreenDPI;
  mParentWidget   = aParentWidget;    

  mDebugFile      = aDebugFile;      

  return NS_OK;
}


PRBool
nsPrintEngine::CheckBeforeDestroy()
{
  if (mPrt && mPrt->mPreparingForPrint) {
    mPrt->mDocWasToBeDestroyed = PR_TRUE;
    return PR_TRUE;
  }
  return PR_FALSE;
}


nsresult
nsPrintEngine::Cancelled()
{
  if (mPrt && mPrt->mPrintSettings) {
    return mPrt->mPrintSettings->SetIsCancelled(PR_TRUE);
  }
  return NS_ERROR_FAILURE;
}






void
nsPrintEngine::InstallPrintPreviewListener()
{
  if (!mPrt->mPPEventListeners) {
    nsCOMPtr<nsPIDOMWindow> win(do_GetInterface(mContainer));
    nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(win->GetFrameElementInternal()));
    mPrt->mPPEventListeners = new nsPrintPreviewListener(target);

    if (mPrt->mPPEventListeners) {
      mPrt->mPPEventListeners->AddListeners();
    }
  }
}


nsresult 
nsPrintEngine::GetSeqFrameAndCountPagesInternal(nsPrintObject*  aPO,
                                                nsIFrame*&    aSeqFrame,
                                                PRInt32&      aCount)
{
  NS_ENSURE_ARG_POINTER(aPO);

  
  nsIPageSequenceFrame* seqFrame = nsnull;
  aPO->mPresShell->GetPageSequenceFrame(&seqFrame);
  if (seqFrame) {
    aSeqFrame = do_QueryFrame(seqFrame);
  } else {
    aSeqFrame = nsnull;
  }
  if (aSeqFrame == nsnull) return NS_ERROR_FAILURE;

  
  aCount = 0;
  nsIFrame* pageFrame = aSeqFrame->GetFirstChild(nsnull);
  while (pageFrame != nsnull) {
    aCount++;
    pageFrame = pageFrame->GetNextSibling();
  }

  return NS_OK;

}


nsresult nsPrintEngine::GetSeqFrameAndCountPages(nsIFrame*& aSeqFrame, PRInt32& aCount)
{
  NS_ASSERTION(mPrtPreview, "mPrtPreview can't be null!");
  return GetSeqFrameAndCountPagesInternal(mPrtPreview->mPrintObject, aSeqFrame, aCount);
}










#ifdef EXTENDED_DEBUG_PRINTING
static int RemoveFilesInDir(const char * aDir);
static void GetDocTitleAndURL(nsPrintObject* aPO, char *& aDocStr, char *& aURLStr);
static void DumpPrintObjectsTree(nsPrintObject * aPO, int aLevel, FILE* aFD);
static void DumpPrintObjectsList(nsTArray<nsPrintObject*> * aDocList);
static void RootFrameList(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent);
static void DumpViews(nsIDocShell* aDocShell, FILE* out);
static void DumpLayoutData(char* aTitleStr, char* aURLStr,
                           nsPresContext* aPresContext,
                           nsIDeviceContext * aDC, nsIFrame * aRootFrame,
                           nsIDocShell * aDocShell, FILE* aFD);
#endif



nsresult
nsPrintEngine::CommonPrint(PRBool                  aIsPrintPreview,
                           nsIPrintSettings*       aPrintSettings,
                           nsIWebProgressListener* aWebProgressListener) {
  nsresult rv = DoCommonPrint(aIsPrintPreview, aPrintSettings,
                              aWebProgressListener);
  if (NS_FAILED(rv)) {
    if (aIsPrintPreview) {
      SetIsCreatingPrintPreview(PR_FALSE);
      SetIsPrintPreview(PR_FALSE);
    } else {
      SetIsPrinting(PR_FALSE);
    }
    if (mProgressDialogIsShown)
      CloseProgressDialog(aWebProgressListener);
    if (rv != NS_ERROR_ABORT && rv != NS_ERROR_OUT_OF_MEMORY)
      ShowPrintErrorDialog(rv, !aIsPrintPreview);
    delete mPrt;
    mPrt = nsnull;
  }

  return rv;
}

nsresult
nsPrintEngine::DoCommonPrint(PRBool                  aIsPrintPreview,
                             nsIPrintSettings*       aPrintSettings,
                             nsIWebProgressListener* aWebProgressListener)
{
  nsresult rv;

  if (aIsPrintPreview) {
    
    
    nsCOMPtr<nsIPrintingPromptService> pps(do_QueryInterface(aWebProgressListener));
    mProgressDialogIsShown = pps != nsnull;

    if (mIsDoingPrintPreview) {
      mOldPrtPreview = mPrtPreview;
      mPrtPreview = nsnull;
    }
  } else {
    mProgressDialogIsShown = PR_FALSE;
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

  mPrt->mPrintSettings->SetIsCancelled(PR_FALSE);
  mPrt->mPrintSettings->GetShrinkToFit(&mPrt->mShrinkToFit);

  if (aIsPrintPreview) {
    SetIsCreatingPrintPreview(PR_TRUE);
    SetIsPrintPreview(PR_TRUE);
    nsCOMPtr<nsIMarkupDocumentViewer> viewer =
      do_QueryInterface(mDocViewerPrint);
    if (viewer) {
      viewer->SetTextZoom(1.0f);
      viewer->SetFullZoom(1.0f);
    }
  } else {
    SetIsPrinting(PR_TRUE);
  }

  
  
  
  
  nsCOMPtr<nsIPrintSession> printSession;
  if (!aIsPrintPreview) {
    printSession = do_CreateInstance("@mozilla.org/gfx/printsession;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    mPrt->mPrintSettings->SetPrintSession(printSession);
  }

  if (aWebProgressListener != nsnull) {
    mPrt->mPrintProgressListeners.AppendObject(aWebProgressListener);
  }

  
  
  
  mPrt->mCurrentFocusWin = FindFocusedDOMWindow();

  
  PRBool isSelection = IsThereARangeSelection(mPrt->mCurrentFocusWin);

  
  nsCOMPtr<nsIDocShell> webContainer(do_QueryInterface(mContainer, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  mPrt->mPrintObject = new nsPrintObject();
  NS_ENSURE_TRUE(mPrt->mPrintObject, NS_ERROR_OUT_OF_MEMORY);
  rv = mPrt->mPrintObject->Init(webContainer);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(mPrt->mPrintDocList.AppendElement(mPrt->mPrintObject),
                 NS_ERROR_OUT_OF_MEMORY);

  mPrt->mIsParentAFrameSet = IsParentAFrameSet(webContainer);
  mPrt->mPrintObject->mFrameType = mPrt->mIsParentAFrameSet ? eFrameSet : eDoc;

  
  nsCOMPtr<nsIDocShellTreeNode>  parentAsNode(do_QueryInterface(webContainer));
  BuildDocTree(parentAsNode, &mPrt->mPrintDocList, mPrt->mPrintObject);

  
  if (!mPrt->mPrintObject->mDocument->GetRootContent())
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
#ifdef NS_DEBUG
    mPrt->mDebugFilePtr = mDebugFile;
#endif

    scriptSuppressor.Suppress();
    PRBool printSilently;
    mPrt->mPrintSettings->GetPrintSilent(&printSilently);

    
    printSilently = nsContentUtils::GetBoolPref("print.always_print_silent",
                                                printSilently);

    
    
    
    if (!printSilently) {
      nsCOMPtr<nsIPrintingPromptService> printPromptService(do_GetService(kPrintingPromptService));
      if (printPromptService) {
        nsIDOMWindow *domWin = mDocument->GetWindow(); 
        NS_ENSURE_TRUE(domWin, NS_ERROR_FAILURE);

        
        
        
        
        
        
        nsCOMPtr<nsIWebBrowserPrint> wbp(do_QueryInterface(mDocViewerPrint));
        rv = printPromptService->ShowPrintDialog(domWin, wbp,
                                                 mPrt->mPrintSettings);
        if (rv == NS_ERROR_NOT_IMPLEMENTED) {
          
          
          
          rv = NS_OK;
        } else if (NS_SUCCEEDED(rv)) {
          
          
          printSilently = PR_TRUE;
        }
        
        mPrt->mPrintSettings->GetShrinkToFit(&mPrt->mShrinkToFit);
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

  rv = devspec->Init(nsnull, mPrt->mPrintSettings, aIsPrintPreview);
  NS_ENSURE_SUCCESS(rv, rv);

  mPrt->mPrintDC = do_CreateInstance("@mozilla.org/gfx/devicecontext;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mPrt->mPrintDC->InitForPrinting(devspec);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (aIsPrintPreview) {
    mPrt->mPrintSettings->SetPrintFrameType(nsIPrintSettings::kFramesAsIs);

    
    
    mPrt->mPrintSettings->SetPrintRange(nsIPrintSettings::kRangeAllPages);
  } else {
    
    
    
    
    
    
    
    
    PRInt16 printFrameTypeUsage = nsIPrintSettings::kUseSettingWhenPossible;
    mPrt->mPrintSettings->GetPrintFrameTypeUsage(&printFrameTypeUsage);

    
    if (printFrameTypeUsage == nsIPrintSettings::kUseSettingWhenPossible) {
      
      PRInt16 printFrameType = nsIPrintSettings::kEachFrameSep;
      mPrt->mPrintSettings->GetPrintFrameType(&printFrameType);

      
      
      if (printFrameType == nsIPrintSettings::kNoFrames) {
        mPrt->mPrintFrameType = nsIPrintSettings::kEachFrameSep;
        mPrt->mPrintSettings->SetPrintFrameType(mPrt->mPrintFrameType);
      } else {
        
        
        PRInt16 howToEnableFrameUI;
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

  if (aIsPrintPreview) {
    PRBool notifyOnInit = PR_FALSE;
    ShowPrintProgress(PR_FALSE, notifyOnInit);

    
    TurnScriptingOn(PR_FALSE);

    if (!notifyOnInit) {
      rv = FinishPrintPreview();
    } else {
      rv = NS_OK;
    }
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    PRUnichar * docTitleStr;
    PRUnichar * docURLStr;

    GetDisplayTitleAndURL(mPrt->mPrintObject, &docTitleStr, &docURLStr, eDocTitleDefURLDoc); 

    
    rv = mPrt->mPrintDC->PrepareDocument(docTitleStr, nsnull);

    if (docTitleStr) nsMemory::Free(docTitleStr);
    if (docURLStr) nsMemory::Free(docURLStr);

    NS_ENSURE_SUCCESS(rv, rv);

    PRBool doNotify;
    ShowPrintProgress(PR_TRUE, doNotify);
    if (!doNotify) {
      
      mPrt->OnStartPrinting();    
      rv = DocumentReadyForPrinting();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  scriptSuppressor.Disconnect();

  return NS_OK;
}


NS_IMETHODIMP
nsPrintEngine::Print(nsIPrintSettings*       aPrintSettings,
                     nsIWebProgressListener* aWebProgressListener)
{
  return CommonPrint(PR_FALSE, aPrintSettings, aWebProgressListener);
}

NS_IMETHODIMP
nsPrintEngine::PrintPreview(nsIPrintSettings* aPrintSettings, 
                                 nsIDOMWindow *aChildDOMWin, 
                                 nsIWebProgressListener* aWebProgressListener)
{
  
  
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(mContainer));
  NS_ASSERTION(docShell, "This has to be a docshell");

  PRUint32 busyFlags = nsIDocShell::BUSY_FLAGS_NONE;
  if (NS_FAILED(docShell->GetBusyFlags(&busyFlags)) ||
      busyFlags != nsIDocShell::BUSY_FLAGS_NONE) {
    CloseProgressDialog(aWebProgressListener);
    ShowPrintErrorDialog(NS_ERROR_GFX_PRINTER_DOC_IS_BUSY_PP, PR_FALSE);
    return NS_ERROR_FAILURE;
  }

  
  return CommonPrint(PR_TRUE, aPrintSettings, aWebProgressListener);
}



NS_IMETHODIMP
nsPrintEngine::GetIsFramesetDocument(PRBool *aIsFramesetDocument)
{
  nsCOMPtr<nsIDocShell> webContainer(do_QueryInterface(mContainer));
  *aIsFramesetDocument = IsParentAFrameSet(webContainer);
  return NS_OK;
}



NS_IMETHODIMP 
nsPrintEngine::GetIsIFrameSelected(PRBool *aIsIFrameSelected)
{
  *aIsIFrameSelected = PR_FALSE;

  
  nsCOMPtr<nsIDocShell> webContainer(do_QueryInterface(mContainer));
  
  nsCOMPtr<nsIDOMWindow> currentFocusWin = FindFocusedDOMWindow();
  if (currentFocusWin && webContainer) {
    
    
    
    PRPackedBool isParentFrameSet;
    *aIsIFrameSelected = IsThereAnIFrameSelected(webContainer, currentFocusWin, isParentFrameSet);
  }
  return NS_OK;
}



NS_IMETHODIMP 
nsPrintEngine::GetIsRangeSelection(PRBool *aIsRangeSelection)
{
  
  nsCOMPtr<nsIDOMWindow> currentFocusWin = FindFocusedDOMWindow();
  *aIsRangeSelection = IsThereARangeSelection(currentFocusWin);
  return NS_OK;
}



NS_IMETHODIMP 
nsPrintEngine::GetIsFramesetFrameSelected(PRBool *aIsFramesetFrameSelected)
{
  
  nsCOMPtr<nsIDOMWindow> currentFocusWin = FindFocusedDOMWindow();
  *aIsFramesetFrameSelected = currentFocusWin != nsnull;
  return NS_OK;
}



NS_IMETHODIMP
nsPrintEngine::GetPrintPreviewNumPages(PRInt32 *aPrintPreviewNumPages)
{
  NS_ENSURE_ARG_POINTER(aPrintPreviewNumPages);

  nsIFrame* seqFrame  = nsnull;
  *aPrintPreviewNumPages = 0;
  if (!mPrtPreview ||
      NS_FAILED(GetSeqFrameAndCountPagesInternal(mPrtPreview->mPrintObject, seqFrame, *aPrintPreviewNumPages))) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}



NS_IMETHODIMP
nsPrintEngine::EnumerateDocumentNames(PRUint32* aCount,
                                      PRUnichar*** aResult)
{
  NS_ENSURE_ARG(aCount);
  NS_ENSURE_ARG_POINTER(aResult);

  *aCount = 0;
  *aResult = nsnull;

  PRInt32     numDocs = mPrt->mPrintDocList.Length();
  PRUnichar** array   = (PRUnichar**) nsMemory::Alloc(numDocs * sizeof(PRUnichar*));
  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;

  for (PRInt32 i=0;i<numDocs;i++) {
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
      docURLStr = nsnull;
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
nsPrintEngine::GetDoingPrint(PRBool *aDoingPrint)
{
  NS_ENSURE_ARG_POINTER(aDoingPrint);
  *aDoingPrint = mIsDoingPrinting;
  return NS_OK;
}



NS_IMETHODIMP
nsPrintEngine::GetDoingPrintPreview(PRBool *aDoingPrintPreview)
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
    *aCurrentPrintSettings = nsnull;
  }
  NS_IF_ADDREF(*aCurrentPrintSettings);
  return NS_OK;
}









nsresult
nsPrintEngine::CheckForPrinters(nsIPrintSettings* aPrintSettings)
{
#ifdef XP_MACOSX
  
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
nsPrintEngine::ShowPrintProgress(PRBool aIsForPrinting, PRBool& aDoNotify)
{
  
  
  
  aDoNotify = PR_FALSE;

  
  PRBool showProgresssDialog = PR_FALSE;

  
  
  if (!mProgressDialogIsShown) {
    showProgresssDialog =
      nsContentUtils::GetBoolPref("print.show_print_progress");
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
      PRBool isModal = PR_TRUE;
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


PRBool
nsPrintEngine::IsThereARangeSelection(nsIDOMWindow* aDOMWin)
{
  nsCOMPtr<nsIPresShell> presShell;
  if (aDOMWin) {
    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aDOMWin));
    window->GetDocShell()->GetPresShell(getter_AddRefs(presShell));
  }

  if (!presShell)
    return PR_FALSE;

  
  
  nsCOMPtr<nsISelection> selection;
  selection = presShell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL);
  if (selection) {
    PRInt32 count;
    selection->GetRangeCount(&count);
    if (count == 1) {
      nsCOMPtr<nsIDOMRange> range;
      if (NS_SUCCEEDED(selection->GetRangeAt(0, getter_AddRefs(range)))) {
        
        PRBool isCollapsed;
        selection->GetIsCollapsed(&isCollapsed);
        return !isCollapsed;
      }
    }
    if (count > 1) return PR_TRUE;
  }
  return PR_FALSE;
}


PRBool
nsPrintEngine::IsParentAFrameSet(nsIDocShell * aParent)
{
  NS_ASSERTION(aParent, "Pointer is null!");

  nsCOMPtr<nsIPresShell> shell;
  aParent->GetPresShell(getter_AddRefs(shell));
  NS_ASSERTION(shell, "shell can't be null");

  
  nsCOMPtr<nsIDocShellTreeItem> parentAsItem(do_QueryInterface(aParent));
  if (!parentAsItem) return PR_FALSE;

  
  
  
  
  
  
  
  
  
  
  
  
  PRBool isFrameSet = PR_FALSE;
  
  
  if (shell) {
    nsIDocument *doc = shell->GetDocument();
    if (doc) {
      nsIContent *rootContent = doc->GetRootContent();
      if (rootContent) {
        isFrameSet = HasFramesetChild(rootContent);
      }
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

  PRInt32 childWebshellCount;
  aParentNode->GetChildCount(&childWebshellCount);
  if (childWebshellCount > 0) {
    for (PRInt32 i=0;i<childWebshellCount;i++) {
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
          nsPrintObject * po = new nsPrintObject();
          nsresult rv = po->Init(childDocShell);
          if (NS_FAILED(rv))
            NS_NOTREACHED("Init failed?");
          po->mParent = aPO;
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

  *aTitle  = nsnull;
  *aURLStr = nsnull;

  nsAutoString docTitle;
  nsCOMPtr<nsIDOMNSDocument> doc = do_QueryInterface(aDoc);
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

  nsCAutoString urlCStr;
  exposableURI->GetSpec(urlCStr);
  *aURLStr = UTF8ToNewUnicode(urlCStr);
}







void
nsPrintEngine::MapContentToWebShells(nsPrintObject* aRootPO,
                                     nsPrintObject* aPO)
{
  NS_ASSERTION(aRootPO, "Pointer is null!");
  NS_ASSERTION(aPO, "Pointer is null!");

  
  
  
  nsIContent *rootContent = aPO->mDocument->GetRootContent();
  if (rootContent) {
    MapContentForPO(aPO, rootContent);
  } else {
    NS_WARNING("Null root content on (sub)document.");
  }

  
  for (PRUint32 i=0;i<aPO->mKids.Length();i++) {
    MapContentToWebShells(aRootPO, aPO->mKids[i]);
  }

}















void
nsPrintEngine::CheckForChildFrameSets(nsPrintObject* aPO)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  
  PRBool hasChildFrames = PR_FALSE;
  for (PRUint32 i=0;i<aPO->mKids.Length();i++) {
    nsPrintObject* po = aPO->mKids[i];
    if (po->mFrameType == eFrame) {
      hasChildFrames = PR_TRUE;
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
      nsPrintObject * po = nsnull;
      PRInt32 cnt = aPO->mKids.Length();
      for (PRInt32 i=0;i<cnt;i++) {
        nsPrintObject* kid = aPO->mKids.ElementAt(i);
        if (kid->mDocument == subDoc) {
          po = kid;
          break;
        }
      }

      
      
      if (po) {
        po->mContent  = aContent;

        nsCOMPtr<nsIDOMHTMLFrameElement> frame(do_QueryInterface(aContent));
        
        
        if (frame && po->mParent->mFrameType == eFrameSet) {
          po->mFrameType = eFrame;
        } else {
          
          po->mFrameType = eIFrame;
          SetPrintAsIs(po, PR_TRUE);
          NS_ASSERTION(po->mParent, "The root must be a parent");
          po->mParent->mPrintAsIs = PR_TRUE;
        }
      }
    }
  }

  
  PRUint32 count = aContent->GetChildCount();
  for (PRUint32 i = 0; i < count; ++i) {
    nsIContent *child = aContent->GetChildAt(i);
    MapContentForPO(aPO, child);
  }
}


PRBool
nsPrintEngine::IsThereAnIFrameSelected(nsIDocShell* aDocShell,
                                       nsIDOMWindow* aDOMWin,
                                       PRPackedBool& aIsParentFrameSet)
{
  aIsParentFrameSet = IsParentAFrameSet(aDocShell);
  PRBool iFrameIsSelected = PR_FALSE;
  if (mPrt && mPrt->mPrintObject) {
    nsPrintObject* po = FindPrintObjectByDOMWin(mPrt->mPrintObject, aDOMWin);
    iFrameIsSelected = po && po->mFrameType == eIFrame;
  } else {
    
    if (!aIsParentFrameSet) {
      
      
      
      if (aDOMWin) {
        
        
        nsCOMPtr<nsIDOMWindow> domWin = do_GetInterface(aDocShell);
        if (domWin != aDOMWin) {
          iFrameIsSelected = PR_TRUE; 
        }
      }
    }
  }

  return iFrameIsSelected;
}




void
nsPrintEngine::SetPrintPO(nsPrintObject* aPO, PRBool aPrint)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  
  aPO->mDontPrint = !aPrint;

  for (PRUint32 i=0;i<aPO->mKids.Length();i++) {
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

  *aTitle  = nsnull;
  *aURLStr = nsnull;

  if (!mPrt)
    return;

  
  
  PRUnichar * docTitleStrPS = nsnull;
  PRUnichar * docURLStrPS   = nsnull;
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
    
    
    DonePrintingPages(nsnull, rv);
  }
  return rv;
}




nsresult nsPrintEngine::CleanupOnFailure(nsresult aResult, PRBool aIsPrinting)
{
  PR_PL(("****  Failed %s - rv 0x%X", aIsPrinting?"Printing":"Print Preview", aResult));

  
  if (mPagePrintTimer) {
    mPagePrintTimer->Stop();
    NS_RELEASE(mPagePrintTimer);
  }
  
  if (aIsPrinting) {
    SetIsPrinting(PR_FALSE);
  } else {
    SetIsPrintPreview(PR_FALSE);
    SetIsCreatingPrintPreview(PR_FALSE);
  }

  





  if (aResult != NS_ERROR_ABORT) {
    ShowPrintErrorDialog(aResult, aIsPrinting);
  }

  FirePrintCompletionEvent();

  return aResult;

}


void
nsPrintEngine::ShowPrintErrorDialog(nsresult aPrintError, PRBool aIsPrinting)
{

  PR_PL(("nsPrintEngine::ShowPrintErrorDialog(nsresult aPrintError=%lx, PRBool aIsPrinting=%d)\n", (long)aPrintError, (int)aIsPrinting));

  nsCAutoString stringName;

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
    PR_PL(("ShowPrintErrorDialog(): wwatch==nsnull\n"));
    return;
  }

  nsCOMPtr<nsIDOMWindow> active;
  wwatch->GetActiveWindow(getter_AddRefs(active));

  nsCOMPtr<nsIPrompt> dialog;
  

  wwatch->GetNewPrompter(active, getter_AddRefs(dialog));
  if (!dialog) {
    PR_PL(("ShowPrintErrorDialog(): dialog==nsnull\n"));
    return;
  }

  dialog->Alert(title.get(), msg.get());
  PR_PL(("ShowPrintErrorDialog(): alert displayed successfully.\n"));
}






nsresult
nsPrintEngine::SetupToPrintContent()
{
  
  
  
  if (NS_FAILED(EnablePOsForPrinting())) {
    return NS_ERROR_FAILURE;
  }
  DUMP_DOC_LIST("\nAfter Enable------------------------------------------");

  
  
  
  
  
  
  PRBool doSetPixelScale = PR_FALSE;
  PRBool ppIsShrinkToFit = mPrtPreview && mPrtPreview->mShrinkToFit;
  if (ppIsShrinkToFit) {
    mPrt->mShrinkRatio = mPrtPreview->mShrinkRatio;
    doSetPixelScale = PR_TRUE;
  }

  
  nsresult rv = ReflowDocList(mPrt->mPrintObject, doSetPixelScale);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  
  
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
      
      mPrt->mShrinkRatio = NS_MAX(mPrt->mShrinkRatio, 0.60f);

      for (PRUint32 i=0;i<mPrt->mPrintDocList.Length();i++) {
        nsPrintObject* po = mPrt->mPrintDocList.ElementAt(i);
        NS_ASSERTION(po, "nsPrintObject can't be null!");
        
        po->DestroyPresentation();
      }

#if (defined(XP_WIN) || defined(XP_OS2)) && defined(EXTENDED_DEBUG_PRINTING)
      
      
      if (kPrintingLogMod && kPrintingLogMod->level == DUMP_LAYOUT_LEVEL) {
        RemoveFilesInDir(".\\");
        gDumpFileNameCnt   = 0;
        gDumpLOFileNameCnt = 0;
      }
#endif

      
      
      
      if (NS_FAILED(ReflowDocList(mPrt->mPrintObject, PR_TRUE))) {
        return NS_ERROR_FAILURE;
      }
    }

#ifdef PR_LOGGING
    {
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
    }
#endif
  }

  DUMP_DOC_LIST(("\nAfter Reflow------------------------------------------"));
  PR_PL(("\n"));
  PR_PL(("-------------------------------------------------------\n"));
  PR_PL(("\n"));

  CalcNumPrintablePages(mPrt->mNumPrintablePages);

  PR_PL(("--- Printing %d pages\n", mPrt->mNumPrintablePages));
  DUMP_DOC_TREELAYOUT;

  
  if (mPrt != nsnull) {
    mPrt->OnStartPrinting();    
  }

  PRUnichar* fileName = nsnull;
  
  PRBool isPrintToFile = PR_FALSE;
  mPrt->mPrintSettings->GetPrintToFile(&isPrintToFile);
  if (isPrintToFile) {
  
  
    mPrt->mPrintSettings->GetToFileName(&fileName);
  }

  PRUnichar * docTitleStr;
  PRUnichar * docURLStr;
  GetDisplayTitleAndURL(mPrt->mPrintObject, &docTitleStr, &docURLStr, eDocTitleDefURLDoc); 

  PRInt32 startPage = 1;
  PRInt32 endPage   = mPrt->mNumPrintablePages;

  PRInt16 printRangeType = nsIPrintSettings::kRangeAllPages;
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
    
    
    nsIPageSequenceFrame *seqFrame = nsnull;
    mPrt->mPrintObject->mPresShell->GetPageSequenceFrame(&seqFrame);
    if (seqFrame) {
      seqFrame->StartPrint(mPrt->mPrintObject->mPresContext, 
                           mPrt->mPrintSettings, docTitleStr, docURLStr);
    }
  } else {
    if (docTitleStr) nsMemory::Free(docTitleStr);
    if (docURLStr) nsMemory::Free(docURLStr);
  }

  PR_PL(("****************** Begin Document ************************\n"));

  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  

  if (mIsDoingPrinting) {
    PrintDocContent(mPrt->mPrintObject, rv); 
  }

  return rv;
}




nsresult
nsPrintEngine::ReflowDocList(nsPrintObject* aPO, PRBool aSetPixelScale)
{
  NS_ENSURE_ARG_POINTER(aPO);

  
  if (aPO->mParent && aPO->mParent->mPresShell) {
    nsIFrame * frame =
      aPO->mParent->mPresShell->GetPrimaryFrameFor(aPO->mContent);
    if (frame) {
      if (!frame->GetStyleVisibility()->IsVisible()) {
        aPO->mDontPrint = PR_TRUE;
        aPO->mInvisible = PR_TRUE;
        return NS_OK;
      }
    }
  }

  
  
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

  nsresult rv;
  
  rv = ReflowPrintObject(aPO);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 cnt = aPO->mKids.Length();
  for (PRInt32 i=0;i<cnt;i++) {
    rv = ReflowDocList(aPO->mKids[i], aSetPixelScale);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}



nsresult
nsPrintEngine::ReflowPrintObject(nsPrintObject * aPO)
{
  NS_ASSERTION(aPO, "Pointer is null!");
  if (!aPO) return NS_ERROR_FAILURE;

  nsSize adjSize;
  PRBool documentIsTopLevel;
  nsIFrame* frame = nsnull;
  if (!aPO->IsPrintable())
    return NS_OK;

  if (aPO->mParent && aPO->mParent->IsPrintable()) {
    if (aPO->mParent->mPresShell) {
      frame = aPO->mParent->mPresShell->FrameManager()->
        GetPrimaryFrameFor(aPO->mContent, -1);
    }
    
    
    if (!frame)
      return NS_OK;

    adjSize = frame->GetContentRect().Size();
    documentIsTopLevel = PR_FALSE;
    
  } else {
    nscoord pageWidth, pageHeight;
    mPrt->mPrintDC->GetDeviceSurfaceDimensions(pageWidth, pageHeight);
#if defined(XP_UNIX) && !defined(XP_MACOSX)
    
    
    
    PRInt32 orientation;
    mPrt->mPrintSettings->GetOrientation(&orientation);
    if (nsIPrintSettings::kLandscapeOrientation == orientation) {
      adjSize = nsSize(pageHeight, pageWidth);
    } else {
      adjSize = nsSize(pageWidth, pageHeight);
    }
#else
    adjSize = nsSize(pageWidth, pageHeight);
#endif 
    documentIsTopLevel = PR_TRUE;
  }

  
  
  
  
  
  
  
  PRBool canCreateScrollbars = PR_TRUE;
  nsIView* parentView = nsnull;
  
  if (frame && frame->GetType() == nsGkAtoms::subDocumentFrame) {
    nsIView* view = frame->GetView();
    NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);
    view = view->GetFirstChild();
    NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);
    parentView = view;
    canCreateScrollbars = PR_FALSE;
  }

  
  aPO->mPresContext = new nsRootPresContext(aPO->mDocument,
    mIsCreatingPrintPreview ? nsPresContext::eContext_PrintPreview:
                              nsPresContext::eContext_Print);
  NS_ENSURE_TRUE(aPO->mPresContext, NS_ERROR_OUT_OF_MEMORY);
  aPO->mPresContext->SetPrintSettings(mPrt->mPrintSettings);

  
  PRBool printBGColors;
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
  
  

  PR_PL(("In DV::ReflowPrintObject PO: %p (%9s) Setting w,h to %d,%d\n", aPO,
         gFrameTypesStr[aPO->mFrameType], adjSize.width, adjSize.height));

  
  nsRect tbounds = nsRect(nsPoint(0, 0), adjSize);
  nsIView* rootView = aPO->mViewManager->CreateView(tbounds, parentView);
  NS_ENSURE_TRUE(rootView, NS_ERROR_OUT_OF_MEMORY);

  
  
  
  if (mIsCreatingPrintPreview && documentIsTopLevel) {
    nsNativeWidget widget = nsnull;
    if (!frame)
      widget = mParentWidget->GetNativeData(NS_NATIVE_WIDGET);
    rv = rootView->CreateWidget(kWidgetCID, nsnull,
                                widget, PR_TRUE, PR_TRUE,
                                eContentTypeContent);
    NS_ENSURE_SUCCESS(rv, rv);
    aPO->mWindow = rootView->GetWidget();
    aPO->mPresContext->SetPaginatedScrolling(canCreateScrollbars);
  }

  
  aPO->mViewManager->SetRootView(rootView);

  
  
  nsCOMPtr<nsISupports> supps(do_QueryInterface(aPO->mDocShell));
  aPO->mPresContext->SetContainer(supps);

  aPO->mPresShell->BeginObservingDocument();

  aPO->mPresContext->SetPageSize(adjSize);
  aPO->mPresContext->SetIsRootPaginatedDocument(documentIsTopLevel);
  aPO->mPresContext->SetPageScale(aPO->mZoomRatio);
  
  float printDPI = float(mPrt->mPrintDC->AppUnitsPerInch()) /
                   float(mPrt->mPrintDC->AppUnitsPerDevPixel());
  aPO->mPresContext->SetPrintPreviewScale(mScreenDPI / printDPI);

  rv = aPO->mPresShell->InitialReflow(adjSize.width, adjSize.height);

  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(aPO->mPresShell, "Presshell should still be here");

  
  aPO->mPresShell->FlushPendingNotifications(Flush_Layout);

  nsCOMPtr<nsIPresShell> displayShell;
  aPO->mDocShell->GetPresShell(getter_AddRefs(displayShell));
  
  nsCOMPtr<nsISelection> selection, selectionPS;
  
  if (displayShell) {
    selection = displayShell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL);
  }
  selectionPS = aPO->mPresShell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL);
  if (selection && selectionPS) {
    PRInt32 cnt;
    selection->GetRangeCount(&cnt);
    PRInt32 inx;
    for (inx=0;inx<cnt;inx++) {
      nsCOMPtr<nsIDOMRange> range;
      if (NS_SUCCEEDED(selection->GetRangeAt(inx, getter_AddRefs(range))))
        selectionPS->AddRange(range);
    }
  }

  
  
  
  
  
  if (mPrt->mShrinkToFit && documentIsTopLevel) {
    nsIPageSequenceFrame* pageSequence;
    aPO->mPresShell->GetPageSequenceFrame(&pageSequence);
    pageSequence->GetSTFPercent(aPO->mShrinkRatio);
  }

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
        nsCOMPtr<nsIRenderingContext> renderingContext;
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
nsPrintEngine::CalcNumPrintablePages(PRInt32& aNumPages)
{
  aNumPages = 0;
  
  
  for (PRUint32 i=0; i<mPrt->mPrintDocList.Length(); i++) {
    nsPrintObject* po = mPrt->mPrintDocList.ElementAt(i);
    NS_ASSERTION(po, "nsPrintObject can't be null!");
    if (po->mPresContext && po->mPresContext->IsRootPaginatedDocument()) {
      nsIPageSequenceFrame* pageSequence;
      po->mPresShell->GetPageSequenceFrame(&pageSequence);
      nsIFrame * seqFrame = do_QueryFrame(pageSequence);
      if (seqFrame) {
        nsIFrame* frame = seqFrame->GetFirstChild(nsnull);
        while (frame) {
          aNumPages++;
          frame = frame->GetNextSibling();
        }
      }
    }
  }
}










PRBool
nsPrintEngine::PrintDocContent(nsPrintObject* aPO, nsresult& aStatus)
{
  NS_ASSERTION(aPO, "Pointer is null!");
  aStatus = NS_OK;

  if (!aPO->mHasBeenPrinted && aPO->IsPrintable()) {
    aStatus = DoPrint(aPO);
    return PR_TRUE;
  }

  
  
  if (!aPO->mInvisible && !(aPO->mPrintAsIs && aPO->mHasBeenPrinted)) {
    for (PRUint32 i=0;i<aPO->mKids.Length();i++) {
      nsPrintObject* po = aPO->mKids[i];
      PRBool printed = PrintDocContent(po, aStatus);
      if (printed || NS_FAILED(aStatus)) {
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
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
    PRInt16 printRangeType = nsIPrintSettings::kRangeAllPages;
    nsresult rv;
    if (mPrt->mPrintSettings != nsnull) {
      mPrt->mPrintSettings->GetPrintRange(&printRangeType);
    }

    
    nsIPageSequenceFrame* pageSequence;
    poPresShell->GetPageSequenceFrame(&pageSequence);
    NS_ASSERTION(nsnull != pageSequence, "no page sequence frame");

    
    mPrt->mPreparingForPrint = PR_FALSE;

    
    if (nsnull != mPrt->mDebugFilePtr) {
#ifdef NS_DEBUG
      
      nsIFrame* root = poPresShell->FrameManager()->GetRootFrame();
      root->DumpRegressionData(poPresContext, mPrt->mDebugFilePtr, 0);
      fclose(mPrt->mDebugFilePtr);
      SetIsPrinting(PR_FALSE);
#endif
    } else {
#ifdef EXTENDED_DEBUG_PRINTING
      nsIFrame* rootFrame = poPresShell->FrameManager()->GetRootFrame();
      if (aPO->IsPrintable()) {
        char * docStr;
        char * urlStr;
        GetDocTitleAndURL(aPO, docStr, urlStr);
        DumpLayoutData(docStr, urlStr, poPresContext, mPrt->mPrintDocDC, rootFrame, docShell, nsnull);
        if (docStr) nsMemory::Free(docStr);
        if (urlStr) nsMemory::Free(urlStr);
      }
#endif

      if (mPrt->mPrintSettings) {
        PRUnichar * docTitleStr = nsnull;
        PRUnichar * docURLStr   = nsnull;

        GetDisplayTitleAndURL(aPO, &docTitleStr, &docURLStr, eDocTitleDefBlank); 

        if (nsIPrintSettings::kRangeSelection == printRangeType) {
          poPresContext->SetIsRenderingOnlySelection(PR_TRUE);
          
          
          nsCOMPtr<nsIRenderingContext> rc;
          mPrt->mPrintDC->CreateRenderingContext(*getter_AddRefs(rc));

          
          
          nsIFrame* startFrame;
          nsIFrame* endFrame;
          PRInt32   startPageNum;
          PRInt32   endPageNum;
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
            nsMargin totalMargin = poPresContext->TwipsToAppUnits(marginTwips + 
                                                              unwrtMarginTwips);
            if (startPageNum == endPageNum) {
              {
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
                PRInt32 totalPages = NSToIntCeil(float(selectionHgt) * aPO->mZoomRatio / float(pageHeight));
                pageSequence->SetTotalNumPages(totalPages);
              }
            }
          }
        }

        nsIFrame * seqFrame = do_QueryFrame(pageSequence);
        if (!seqFrame) {
          SetIsPrinting(PR_FALSE);
          return NS_ERROR_FAILURE;
        }

        mPageSeqFrame = pageSequence;
        mPageSeqFrame->StartPrint(poPresContext, mPrt->mPrintSettings, docTitleStr, docURLStr);

        
        PR_PL(("Scheduling Print of PO: %p (%s) \n", aPO, gFrameTypesStr[aPO->mFrameType]));
        StartPagePrintTimer(aPO);
      } else {
        
        SetIsPrinting(PR_FALSE);
        return NS_ERROR_FAILURE;
      }
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
  const PRUint32 kTitleLength = 64;

  PRUnichar * docTitleStr;
  PRUnichar * docURLStr;
  GetDisplayTitleAndURL(aPO, &docTitleStr, &docURLStr, eDocTitleDefURLDoc);

  
  ElipseLongString(docTitleStr, kTitleLength, PR_FALSE);
  ElipseLongString(docURLStr, kTitleLength, PR_TRUE);

  aParams->SetDocTitle(docTitleStr);
  aParams->SetDocURL(docURLStr);

  if (docTitleStr != nsnull) nsMemory::Free(docTitleStr);
  if (docURLStr != nsnull) nsMemory::Free(docURLStr);
}


void
nsPrintEngine::ElipseLongString(PRUnichar *& aStr, const PRUint32 aLen, PRBool aDoFront)
{
  
  if (aStr && nsCRT::strlen(aStr) > aLen) {
    if (aDoFront) {
      PRUnichar * ptr = &aStr[nsCRT::strlen(aStr)-aLen+3];
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


PRBool
nsPrintEngine::PrintPage(nsPrintObject*    aPO,
                         PRBool&           aInRange)
{
  NS_ASSERTION(aPO,            "aPO is null!");
  NS_ASSERTION(mPageSeqFrame,  "mPageSeqFrame is null!");
  NS_ASSERTION(mPrt,           "mPrt is null!");

  
  
  if (!mPrt || !aPO || !mPageSeqFrame) {
    ShowPrintErrorDialog(NS_ERROR_FAILURE);
    return PR_TRUE; 
  }

  PR_PL(("-----------------------------------\n"));
  PR_PL(("------ In DV::PrintPage PO: %p (%s)\n", aPO, gFrameTypesStr[aPO->mFrameType]));

  
  PRBool isCancelled = PR_FALSE;
  mPrt->mPrintSettings->GetIsCancelled(&isCancelled);
  if (isCancelled)
    return PR_TRUE;

  PRInt32 pageNum, numPages, endPage;
  mPageSeqFrame->GetCurrentPageNum(&pageNum);
  mPageSeqFrame->GetNumPages(&numPages);

  PRBool donePrinting;
  PRBool isDoingPrintRange;
  mPageSeqFrame->IsDoingPrintRange(&isDoingPrintRange);
  if (isDoingPrintRange) {
    PRInt32 fromPage;
    PRInt32 toPage;
    mPageSeqFrame->GetPrintRange(&fromPage, &toPage);

    if (fromPage > numPages) {
      return PR_TRUE;
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
    aInRange = PR_TRUE;
  }

  
  
  if (mPrt->mPrintFrameType == nsIPrintSettings::kEachFrameSep)
    endPage = mPrt->mNumPrintablePages;
  
  mPrt->DoOnProgressChange(++mPrt->mNumPagesPrinted, endPage, PR_FALSE, 0);

  
  
  
  
  
  
  
  nsresult rv = mPageSeqFrame->PrintNextPage();
  if (NS_FAILED(rv)) {
    if (rv != NS_ERROR_ABORT) {
      ShowPrintErrorDialog(rv);
      mPrt->mIsAborted = PR_TRUE;
    }
    return PR_TRUE;
  }

  mPageSeqFrame->DoPageEnd();

  return donePrinting;
}




nsresult 
nsPrintEngine::FindSelectionBoundsWithList(nsPresContext* aPresContext,
                                           nsIRenderingContext& aRC,
                                           nsIAtom*        aList,
                                           nsIFrame *      aParentFrame,
                                           nsRect&         aRect,
                                           nsIFrame *&     aStartFrame,
                                           nsRect&         aStartRect,
                                           nsIFrame *&     aEndFrame,
                                           nsRect&         aEndRect)
{
  NS_ASSERTION(aPresContext, "Pointer is null!");
  NS_ASSERTION(aParentFrame, "Pointer is null!");

  nsIFrame* child = aParentFrame->GetFirstChild(aList);
  aRect += aParentFrame->GetPosition();
  while (child) {
    
    
    PRBool isSelected = (child->GetStateBits() & NS_FRAME_SELECTED_CONTENT)
      == NS_FRAME_SELECTED_CONTENT;
    if (isSelected) {
      isSelected = child->IsVisibleForPainting();
    }

    if (isSelected) {
      nsRect r = child->GetRect();
      if (aStartFrame == nsnull) {
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
                                   nsIRenderingContext& aRC,
                                   nsIFrame *      aParentFrame,
                                   nsRect&         aRect,
                                   nsIFrame *&     aStartFrame,
                                   nsRect&         aStartRect,
                                   nsIFrame *&     aEndFrame,
                                   nsRect&         aEndRect)
{
  NS_ASSERTION(aPresContext, "Pointer is null!");
  NS_ASSERTION(aParentFrame, "Pointer is null!");

  
  nsIAtom* childListName = nsnull;
  PRInt32  childListIndex = 0;
  do {
    nsresult rv = FindSelectionBoundsWithList(aPresContext, aRC, childListName, aParentFrame, aRect, aStartFrame, aStartRect, aEndFrame, aEndRect);
    NS_ENSURE_SUCCESS(rv, rv);
    childListName = aParentFrame->GetAdditionalChildListName(childListIndex++);
  } while (childListName);
  return NS_OK;
}







nsresult 
nsPrintEngine::GetPageRangeForSelection(nsIPresShell *        aPresShell,
                                        nsPresContext*       aPresContext,
                                        nsIRenderingContext&  aRC,
                                        nsISelection*         aSelection,
                                        nsIPageSequenceFrame* aPageSeqFrame,
                                        nsIFrame**            aStartFrame,
                                        PRInt32&              aStartPageNum,
                                        nsRect&               aStartRect,
                                        nsIFrame**            aEndFrame,
                                        PRInt32&              aEndPageNum,
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

  nsIFrame * startFrame = nsnull;
  nsIFrame * endFrame   = nsnull;

  
  
  
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

  
  if (startFrame != nsnull) {
    
    
    
    
    
    if (endFrame == nsnull) {
      
      
      
      
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
  PRInt32 pageNum = 1;
  nsIFrame* child = seqFrame->GetFirstChild(nsnull);
  while (child != nsnull) {
    printf("Page: %d - %p\n", pageNum, child);
    pageNum++;
    child = child->GetNextSibling();
  }
  }
#endif

  
  
  PRInt32 pageNum = 1;
  nsIFrame* page = seqFrame->GetFirstChild(nsnull);
  while (page != nsnull) {
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











void nsPrintEngine::SetIsPrinting(PRBool aIsPrinting)
{ 
  mIsDoingPrinting = aIsPrinting;
  
  
  if (mDocViewerPrint && !mIsDoingPrintPreview) {
    mDocViewerPrint->SetIsPrinting(aIsPrinting);
  }
  if (mPrt && aIsPrinting) {
    mPrt->mPreparingForPrint = PR_TRUE;
  }
}


void nsPrintEngine::SetIsPrintPreview(PRBool aIsPrintPreview) 
{ 
  mIsDoingPrintPreview = aIsPrintPreview; 

  if (mDocViewerPrint) {
    mDocViewerPrint->SetIsPrintPreview(aIsPrintPreview);
  }
}


void
nsPrintEngine::CleanupDocTitleArray(PRUnichar**& aArray, PRInt32& aCount)
{
  for (PRInt32 i = aCount - 1; i >= 0; i--) {
    nsMemory::Free(aArray[i]);
  }
  nsMemory::Free(aArray);
  aArray = NULL;
  aCount = 0;
}



PRBool nsPrintEngine::HasFramesetChild(nsIContent* aContent)
{
  if (!aContent) {
    return PR_FALSE;
  }

  PRUint32 numChildren = aContent->GetChildCount();

  
  for (PRUint32 i = 0; i < numChildren; ++i) {
    nsIContent *child = aContent->GetChildAt(i);
    if (child->Tag() == nsGkAtoms::frameset &&
        child->IsHTML()) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}
 





already_AddRefed<nsIDOMWindow>
nsPrintEngine::FindFocusedDOMWindow()
{
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm) {
    nsCOMPtr<nsIDOMWindow> domWin;
    fm->GetFocusedWindow(getter_AddRefs(domWin));
    if (domWin && IsWindowsInOurSubTree(domWin))
      return domWin.forget();
  }

  return nsnull;
}


PRBool
nsPrintEngine::IsWindowsInOurSubTree(nsIDOMWindow * aDOMWindow)
{
  PRBool found = PR_FALSE;

  
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aDOMWindow));
  if (window) {
    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem =
      do_QueryInterface(window->GetDocShell());

    if (docShellAsItem) {
      
      nsCOMPtr<nsIDocShell> thisDVDocShell(do_QueryInterface(mContainer));
      while (!found) {
        nsCOMPtr<nsIDocShell> parentDocshell(do_QueryInterface(docShellAsItem));
        if (parentDocshell) {
          if (parentDocshell == thisDVDocShell) {
            found = PR_TRUE;
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


PRBool
nsPrintEngine::DonePrintingPages(nsPrintObject* aPO, nsresult aResult)
{
  
  PR_PL(("****** In DV::DonePrintingPages PO: %p (%s)\n", aPO, aPO?gFrameTypesStr[aPO->mFrameType]:""));

  if (aPO != nsnull) {
    aPO->mHasBeenPrinted = PR_TRUE;
    nsresult rv;
    PRBool didPrint = PrintDocContent(mPrt->mPrintObject, rv);
    if (NS_SUCCEEDED(rv) && didPrint) {
      PR_PL(("****** In DV::DonePrintingPages PO: %p (%s) didPrint:%s (Not Done Printing)\n", aPO, gFrameTypesStr[aPO->mFrameType], PRT_YESNO(didPrint)));
      return PR_FALSE;
    }
  }

  if (NS_SUCCEEDED(aResult)) {
    FirePrintCompletionEvent();
  }

  TurnScriptingOn(PR_TRUE);
  SetIsPrinting(PR_FALSE);

  
  
  NS_IF_RELEASE(mPagePrintTimer);

  return PR_TRUE;
}




void
nsPrintEngine::SetPrintAsIs(nsPrintObject* aPO, PRBool aAsIs)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  aPO->mPrintAsIs = aAsIs;
  for (PRUint32 i=0;i<aPO->mKids.Length();i++) {
    SetPrintAsIs(aPO->mKids[i], aAsIs);
  }
}



nsPrintObject*
nsPrintEngine::FindPrintObjectByDOMWin(nsPrintObject* aPO,
                                       nsIDOMWindow* aDOMWin)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  
  
  if (!aDOMWin) {
    return nsnull;
  }

  nsCOMPtr<nsIDOMWindow> domWin(do_GetInterface(aPO->mDocShell));
  if (domWin && domWin == aDOMWin) {
    return aPO;
  }

  PRInt32 cnt = aPO->mKids.Length();
  for (PRInt32 i = 0; i < cnt; ++i) {
    nsPrintObject* po = FindPrintObjectByDOMWin(aPO->mKids[i], aDOMWin);
    if (po) {
      return po;
    }
  }

  return nsnull;
}


nsresult
nsPrintEngine::EnablePOsForPrinting()
{
  
  
  mPrt->mSelectedPO = nsnull;

  if (mPrt->mPrintSettings == nsnull) {
    return NS_ERROR_FAILURE;
  }

  mPrt->mPrintFrameType = nsIPrintSettings::kNoFrames;
  mPrt->mPrintSettings->GetPrintFrameType(&mPrt->mPrintFrameType);

  PRInt16 printHowEnable = nsIPrintSettings::kFrameEnableNone;
  mPrt->mPrintSettings->GetHowToEnableFrameUI(&printHowEnable);

  PRInt16 printRangeType = nsIPrintSettings::kRangeAllPages;
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
      SetPrintPO(mPrt->mPrintObject, PR_TRUE);

      
      
      if (mPrt->mPrintObject->mKids.Length() > 0) {
        for (PRUint32 i=0;i<mPrt->mPrintObject->mKids.Length();i++) {
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
        if (po != nsnull) {
          mPrt->mSelectedPO = po;
          
          SetPrintAsIs(po);

          
          SetPrintPO(po, PR_TRUE);

          
          
          
          
          
          
          
          nsCOMPtr<nsIDOMWindow> domWin = do_GetInterface(po->mDocShell);
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
        for (PRUint32 i=0;i<mPrt->mPrintDocList.Length();i++) {
          nsPrintObject* po = mPrt->mPrintDocList.ElementAt(i);
          NS_ASSERTION(po, "nsPrintObject can't be null!");
          nsCOMPtr<nsIDOMWindow> domWin = do_GetInterface(po->mDocShell);
          if (IsThereARangeSelection(domWin)) {
            mPrt->mCurrentFocusWin = domWin;
            SetPrintPO(po, PR_TRUE);
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
      if (po != nsnull) {
        mPrt->mSelectedPO = po;
        
        SetPrintAsIs(po);

        
        SetPrintPO(po, PR_TRUE);

        
        
        
        
        
        
        
        nsCOMPtr<nsIDOMWindow> domWin = do_GetInterface(po->mDocShell);
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
    SetPrintPO(mPrt->mPrintObject, PR_TRUE);
    return NS_OK;
  }

  
  
  
  if (mPrt->mPrintFrameType == nsIPrintSettings::kSelectedFrame) {

    if ((mPrt->mIsParentAFrameSet && mPrt->mCurrentFocusWin) || mPrt->mIsIFrameSelected) {
      nsPrintObject * po = FindPrintObjectByDOMWin(mPrt->mPrintObject, mPrt->mCurrentFocusWin);
      if (po != nsnull) {
        mPrt->mSelectedPO = po;
        
        
        
        if (po->mKids.Length() > 0) {
          
          SetPrintAsIs(po);
        }

        
        SetPrintPO(po, PR_TRUE);
      }
    }
    return NS_OK;
  }

  
  
  if (mPrt->mPrintFrameType == nsIPrintSettings::kEachFrameSep) {
    SetPrintPO(mPrt->mPrintObject, PR_TRUE);
    PRInt32 cnt = mPrt->mPrintDocList.Length();
    for (PRInt32 i=0;i<cnt;i++) {
      nsPrintObject* po = mPrt->mPrintDocList.ElementAt(i);
      NS_ASSERTION(po, "nsPrintObject can't be null!");
      if (po->mFrameType == eFrameSet) {
        po->mDontPrint = PR_TRUE;
      }
    }
  }

  return NS_OK;
}




nsPrintObject*
nsPrintEngine::FindSmallestSTF()
{
  float smallestRatio = 1.0f;
  nsPrintObject* smallestPO = nsnull;

  for (PRUint32 i=0;i<mPrt->mPrintDocList.Length();i++) {
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
nsPrintEngine::TurnScriptingOn(PRBool aDoTurnOn)
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
  

  for (PRUint32 i=0;i<prt->mPrintDocList.Length();i++) {
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
            scx->SetScriptsEnabled(PR_TRUE, PR_FALSE);
          }
          window->ResumeTimeouts(PR_FALSE);
        }
      } else {
        
        
        
        if (propThere == NS_PROPTABLE_PROP_NOT_THERE) {
          
          
          doc->SetProperty(nsGkAtoms::scriptEnabledBeforePrintOrPreview,
                           NS_INT32_TO_PTR(doc->IsScriptEnabled()));
          if (scx) {
            scx->SetScriptsEnabled(PR_FALSE, PR_FALSE);
          }
          window->SuspendTimeouts(1, PR_FALSE);
        }
      }
    }
  }
}











void
nsPrintEngine::CloseProgressDialog(nsIWebProgressListener* aWebProgressListener)
{
  if (aWebProgressListener) {
    aWebProgressListener->OnStateChange(nsnull, nsnull, nsIWebProgressListener::STATE_STOP|nsIWebProgressListener::STATE_IS_DOCUMENT, nsnull);
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

  SetIsCreatingPrintPreview(PR_FALSE);

  
  if (NS_FAILED(rv)) {
    


    mPrt->OnEndPrinting();
    TurnScriptingOn(PR_TRUE);

    return rv;
  }

  
  


  if (mIsDoingPrintPreview && mOldPrtPreview) {
    delete mOldPrtPreview;
    mOldPrtPreview = nsnull;
  }

  InstallPrintPreviewListener();

  mPrt->OnEndPrinting();

  
  
  mPrtPreview = mPrt;
  mPrt        = nsnull;

#endif 

  return NS_OK;
}







nsresult
nsPrintEngine::StartPagePrintTimer(nsPrintObject* aPO)
{
  if (!mPagePrintTimer) {
    nsresult rv = NS_NewPagePrintTimer(&mPagePrintTimer);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    PRInt32 printPageDelay = 500;
    mPrt->mPrintSettings->GetPrintPageDelay(&printPageDelay);

    mPagePrintTimer->Init(this, mDocViewerPrint, printPageDelay);
  }

  return mPagePrintTimer->Start(aPO);
}


NS_IMETHODIMP 
nsPrintEngine::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (mIsDoingPrinting) {
    rv = DocumentReadyForPrinting();
 
    
    if (NS_FAILED(rv)) {
      CleanupOnFailure(rv, PR_TRUE);
    }
  } else {
    rv = FinishPrintPreview();
    if (NS_FAILED(rv)) {
      CleanupOnFailure(rv, PR_FALSE);
    }
    if (mPrtPreview) {
      mPrtPreview->OnEndPrinting();
    }
    rv = NS_OK;
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




static void RootFrameList(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent)
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
                       nsIRenderingContext * aRendContext,
                       nsIFrame *            aFrame,
                       PRInt32               aLevel)
{
  NS_ASSERTION(out, "Pointer is null!");
  NS_ASSERTION(aPresContext, "Pointer is null!");
  NS_ASSERTION(aRendContext, "Pointer is null!");
  NS_ASSERTION(aFrame, "Pointer is null!");

  nsIFrame* child = aFrame->GetFirstChild(nsnull);
  while (child != nsnull) {
    for (PRInt32 i=0;i<aLevel;i++) {
     fprintf(out, "  ");
    }
    nsAutoString tmp;
    child->GetFrameName(tmp);
    fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
    PRBool isSelected;
    if (NS_SUCCEEDED(child->IsVisibleForPainting(aPresContext, *aRendContext, PR_TRUE, &isSelected))) {
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

  if (nsnull != aDocShell) {
    fprintf(out, "docshell=%p \n", aDocShell);
    nsIPresShell* shell = nsPrintEngine::GetPresShellFor(aDocShell);
    if (shell) {
      nsIViewManager* vm = shell->GetViewManager();
      if (vm) {
        nsIView* root;
        vm->GetRootView(root);
        if (nsnull != root) {
          root->List(out);
        }
      }
    }
    else {
      fputs("null pres shell\n", out);
    }

    
    PRInt32 i, n;
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
                    nsIDeviceContext * aDC,
                    nsIFrame *         aRootFrame,
                    nsIDocShekk *      aDocShell,
                    FILE*              aFD = nsnull)
{
  if (!kPrintingLogMod || kPrintingLogMod->level != DUMP_LAYOUT_LEVEL) return;

  if (aPresContext == nsnull || aDC == nsnull) {
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
    nsCOMPtr<nsIRenderingContext> renderingContext;
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
    if (aFD == nsnull) {
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
  PRInt32 cnt = aDocList->Length();
  for (PRInt32 i=0;i<cnt;i++) {
    nsPrintObject* po = aDocList->ElementAt(i);
    NS_ASSERTION(po, "nsPrintObject can't be null!");
    nsIFrame* rootFrame = nsnull;
    if (po->mPresShell) {
      rootFrame = po->mPresShell->FrameManager()->GetRootFrame();
      while (rootFrame != nsnull) {
        nsIPageSequenceFrame * sqf = do_QueryFrame(rootFrame);
        if (sqf) {
          break;
        }
        rootFrame = rootFrame->GetFirstChild(nsnull);
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
  PRInt32 cnt = aPO->mKids.Length();
  for (PRInt32 i=0;i<cnt;i++) {
    nsPrintObject* po = aPO->mKids.ElementAt(i);
    NS_ASSERTION(po, "nsPrintObject can't be null!");
    for (PRInt32 k=0;k<aLevel;k++) fprintf(fd, "  ");
    fprintf(fd, "%s %p %p %p %p %d %d,%d,%d,%d\n", types[po->mFrameType], po, po->mDocShell.get(), po->mSeqFrame,
           po->mPageFrame, po->mPageNum, po->mRect.x, po->mRect.y, po->mRect.width, po->mRect.height);
  }
}


static void GetDocTitleAndURL(nsPrintObject* aPO, char *& aDocStr, char *& aURLStr)
{
  aDocStr = nsnull;
  aURLStr = nsnull;

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
                                       nsIDeviceContext * aDC,
                                       int aLevel, FILE * aFD)
{
  if (!kPrintingLogMod || kPrintingLogMod->level != DUMP_LAYOUT_LEVEL) return;

  NS_ASSERTION(aPO, "Pointer is null!");
  NS_ASSERTION(aDC, "Pointer is null!");

  const char types[][3] = {"DC", "FR", "IF", "FS"};
  FILE * fd = nsnull;
  if (aLevel == 0) {
    fd = fopen("tree_layout.txt", "w");
    fprintf(fd, "DocTree\n***************************************************\n");
    fprintf(fd, "***************************************************\n");
    fprintf(fd, "T     PO    DocShell   Seq      Page     Page#    Rect\n");
  } else {
    fd = aFD;
  }
  if (fd) {
    nsIFrame* rootFrame = nsnull;
    if (aPO->mPresShell) {
      rootFrame = aPO->mPresShell->FrameManager()->GetRootFrame();
    }
    for (PRInt32 k=0;k<aLevel;k++) fprintf(fd, "  ");
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

    PRInt32 cnt = aPO->mKids.Length();
    for (PRInt32 i=0;i<cnt;i++) {
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






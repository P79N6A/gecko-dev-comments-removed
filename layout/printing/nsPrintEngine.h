



































#ifndef nsPrintEngine_h___
#define nsPrintEngine_h___

#include "nsCOMPtr.h"

#include "nsPrintObject.h"
#include "nsPrintData.h"


#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsIObserver.h"


class nsPagePrintTimer;
class nsIDocShellTreeNode;
class nsIDeviceContext;
class nsIDocumentViewerPrint;
class nsPrintObject;
class nsIDocShell;
class nsIPageSequenceFrame;





class nsPrintEngine : public nsIObserver
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

  
  NS_IMETHOD Print(nsIPrintSettings*       aPrintSettings,
                   nsIWebProgressListener* aWebProgressListener);
  NS_IMETHOD PrintPreview(nsIPrintSettings* aPrintSettings,
                          nsIDOMWindow *aChildDOMWin,
                          nsIWebProgressListener* aWebProgressListener);
  NS_IMETHOD GetIsFramesetDocument(PRBool *aIsFramesetDocument);
  NS_IMETHOD GetIsIFrameSelected(PRBool *aIsIFrameSelected);
  NS_IMETHOD GetIsRangeSelection(PRBool *aIsRangeSelection);
  NS_IMETHOD GetIsFramesetFrameSelected(PRBool *aIsFramesetFrameSelected);
  NS_IMETHOD GetPrintPreviewNumPages(PRInt32 *aPrintPreviewNumPages);
  NS_IMETHOD EnumerateDocumentNames(PRUint32* aCount, PRUnichar*** aResult);
  static nsresult GetGlobalPrintSettings(nsIPrintSettings** aPrintSettings);
  NS_IMETHOD GetDoingPrint(PRBool *aDoingPrint);
  NS_IMETHOD GetDoingPrintPreview(PRBool *aDoingPrintPreview);
  NS_IMETHOD GetCurrentPrintSettings(nsIPrintSettings **aCurrentPrintSettings);


  
  
  enum eDocTitleDefault {
    eDocTitleDefNone,
    eDocTitleDefBlank,
    eDocTitleDefURLDoc
  };

  nsPrintEngine();
  ~nsPrintEngine();

  void Destroy();
  void DestroyPrintingData();

  nsresult Initialize(nsIDocumentViewerPrint* aDocViewerPrint, 
                      nsISupports*            aContainer,
                      nsIDocument*            aDocument,
                      float                   aScreenDPI,
                      nsIWidget*              aParentWidget,
                      FILE*                   aDebugFile);

  nsresult GetSeqFrameAndCountPages(nsIFrame*& aSeqFrame, PRInt32& aCount);

  
  
  
  nsresult DocumentReadyForPrinting();
  nsresult GetSelectionDocument(nsIDeviceContextSpec * aDevSpec,
                                nsIDocument ** aNewDoc);

  nsresult SetupToPrintContent();
  nsresult EnablePOsForPrinting();
  nsPrintObject* FindSmallestSTF();

  PRBool   PrintDocContent(nsPrintObject* aPO, nsresult& aStatus);
  nsresult DoPrint(nsPrintObject * aPO);

  void SetPrintPO(nsPrintObject* aPO, PRBool aPrint);

  void TurnScriptingOn(PRBool aDoTurnOn);
  PRBool CheckDocumentForPPCaching();
  void InstallPrintPreviewListener();

  
  PRBool   PrintPage(nsPrintObject* aPOect, PRBool& aInRange);
  PRBool   DonePrintingPages(nsPrintObject* aPO, nsresult aResult);

  
  void BuildDocTree(nsIDocShellTreeNode *      aParentNode,
                    nsTArray<nsPrintObject*> * aDocList,
                    nsPrintObject *            aPO);
  nsresult ReflowDocList(nsPrintObject * aPO, PRBool aSetPixelScale);

  nsresult ReflowPrintObject(nsPrintObject * aPO);

  void CheckForChildFrameSets(nsPrintObject* aPO);

  void CalcNumPrintablePages(PRInt32& aNumPages);
  void ShowPrintProgress(PRBool aIsForPrinting, PRBool& aDoNotify);
  nsresult CleanupOnFailure(nsresult aResult, PRBool aIsPrinting);
  
  
  nsresult FinishPrintPreview();
  static void CloseProgressDialog(nsIWebProgressListener* aWebProgressListener);
  void SetDocAndURLIntoProgress(nsPrintObject* aPO,
                                nsIPrintProgressParams* aParams);
  void ElipseLongString(PRUnichar *& aStr, const PRUint32 aLen, PRBool aDoFront);
  nsresult CheckForPrinters(nsIPrintSettings* aPrintSettings);
  void CleanupDocTitleArray(PRUnichar**& aArray, PRInt32& aCount);

  PRBool IsThereARangeSelection(nsIDOMWindow * aDOMWin);

  


  
  nsresult StartPagePrintTimer(nsPrintObject* aPO);

  PRBool IsWindowsInOurSubTree(nsIDOMWindow * aDOMWindow);
  static PRBool IsParentAFrameSet(nsIDocShell * aParent);
  PRBool IsThereAnIFrameSelected(nsIDocShell* aDocShell,
                                 nsIDOMWindow* aDOMWin,
                                 PRPackedBool& aIsParentFrameSet);

  static nsPrintObject* FindPrintObjectByDOMWin(nsPrintObject* aParentObject,
                                                nsIDOMWindow* aDOMWin);

  
  already_AddRefed<nsIDOMWindow> FindFocusedDOMWindow();

  
  
  
  static void GetDocumentTitleAndURL(nsIDocument* aDoc,
                                     PRUnichar** aTitle,
                                     PRUnichar** aURLStr);
  void GetDisplayTitleAndURL(nsPrintObject*    aPO,
                             PRUnichar**       aTitle,
                             PRUnichar**       aURLStr,
                             eDocTitleDefault  aDefType);
  static void ShowPrintErrorDialog(nsresult printerror,
                                   PRBool aIsPrinting = PR_TRUE);

  static PRBool HasFramesetChild(nsIContent* aContent);

  PRBool   CheckBeforeDestroy();
  nsresult Cancelled();

  nsIWidget* GetPrintPreviewWindow() {return mPrtPreview->mPrintObject->mWindow;}

  nsIViewManager* GetPrintPreviewViewManager() {return mPrtPreview->mPrintObject->mViewManager;}

  float GetPrintPreviewScale() { return mPrtPreview->mPrintObject->
                                        mPresContext->GetPrintPreviewScale(); }
  
  static nsIPresShell* GetPresShellFor(nsIDocShell* aDocShell);

  
  void SetIsPrinting(PRBool aIsPrinting);
  PRBool GetIsPrinting()
  {
    return mIsDoingPrinting;
  }
  void SetIsPrintPreview(PRBool aIsPrintPreview);
  PRBool GetIsPrintPreview()
  {
    return mIsDoingPrintPreview;
  }
  void SetIsCreatingPrintPreview(PRBool aIsCreatingPrintPreview)
  {
    mIsCreatingPrintPreview = aIsCreatingPrintPreview;
  }
  PRBool GetIsCreatingPrintPreview()
  {
    return mIsCreatingPrintPreview;
  }

protected:

  nsresult CommonPrint(PRBool aIsPrintPreview, nsIPrintSettings* aPrintSettings,
                       nsIWebProgressListener* aWebProgressListener,
                       nsIDOMDocument* aDoc);

  nsresult DoCommonPrint(PRBool aIsPrintPreview, nsIPrintSettings* aPrintSettings,
                         nsIWebProgressListener* aWebProgressListener,
                         nsIDOMDocument* aDoc);

  void FirePrintCompletionEvent();
  static nsresult GetSeqFrameAndCountPagesInternal(nsPrintObject*  aPO,
                                                   nsIFrame*&      aSeqFrame,
                                                   PRInt32&        aCount);

  static nsresult FindSelectionBoundsWithList(nsPresContext* aPresContext,
                                              nsIRenderingContext& aRC,
                                              nsIAtom*        aList,
                                              nsIFrame *      aParentFrame,
                                              nsRect&         aRect,
                                              nsIFrame *&     aStartFrame,
                                              nsRect&         aStartRect,
                                              nsIFrame *&     aEndFrame,
                                              nsRect&         aEndRect);

  static nsresult FindSelectionBounds(nsPresContext* aPresContext,
                                      nsIRenderingContext& aRC,
                                      nsIFrame *      aParentFrame,
                                      nsRect&         aRect,
                                      nsIFrame *&     aStartFrame,
                                      nsRect&         aStartRect,
                                      nsIFrame *&     aEndFrame,
                                      nsRect&         aEndRect);

  static nsresult GetPageRangeForSelection(nsIPresShell *        aPresShell,
                                           nsPresContext*       aPresContext,
                                           nsIRenderingContext&  aRC,
                                           nsISelection*         aSelection,
                                           nsIPageSequenceFrame* aPageSeqFrame,
                                           nsIFrame**            aStartFrame,
                                           PRInt32&              aStartPageNum,
                                           nsRect&               aStartRect,
                                           nsIFrame**            aEndFrame,
                                           PRInt32&              aEndPageNum,
                                           nsRect&               aEndRect);

  static void MapContentForPO(nsPrintObject* aPO, nsIContent* aContent);

  static void MapContentToWebShells(nsPrintObject* aRootPO, nsPrintObject* aPO);

  static void SetPrintAsIs(nsPrintObject* aPO, PRBool aAsIs = PR_TRUE);

  
  PRPackedBool mIsCreatingPrintPreview;
  PRPackedBool mIsDoingPrinting;
  PRPackedBool mIsDoingPrintPreview; 
  PRPackedBool mProgressDialogIsShown;

  nsCOMPtr<nsIDocumentViewerPrint> mDocViewerPrint;
  nsISupports*            mContainer;      
  float                   mScreenDPI;
  
  nsPrintData*            mPrt;
  nsPagePrintTimer*       mPagePrintTimer;
  nsIPageSequenceFrame*   mPageSeqFrame;

  
  nsCOMPtr<nsIWidget>     mParentWidget;        
  nsPrintData*            mPrtPreview;
  nsPrintData*            mOldPrtPreview;

  nsCOMPtr<nsIDocument>   mDocument;

  FILE* mDebugFile;

private:
  nsPrintEngine& operator=(const nsPrintEngine& aOther); 

};

#endif 

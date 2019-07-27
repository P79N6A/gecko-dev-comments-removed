



#ifndef nsPrintEngine_h___
#define nsPrintEngine_h___

#include "mozilla/Attributes.h"

#include "nsCOMPtr.h"

#include "nsPrintObject.h"
#include "nsPrintData.h"
#include "nsFrameList.h"
#include "mozilla/Attributes.h"
#include "nsIWebProgress.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"


#include "nsIDOMWindow.h"
#include "nsIObserver.h"


class nsPagePrintTimer;
class nsIDocShell;
class nsIDocument;
class nsIDocumentViewerPrint;
class nsPrintObject;
class nsIDocShell;
class nsIPageSequenceFrame;





class nsPrintEngine final : public nsIObserver,
                            public nsIWebProgressListener,
                            public nsSupportsWeakReference
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

  NS_DECL_NSIWEBPROGRESSLISTENER

  
  NS_IMETHOD Print(nsIPrintSettings*       aPrintSettings,
                   nsIWebProgressListener* aWebProgressListener);
  NS_IMETHOD PrintPreview(nsIPrintSettings* aPrintSettings,
                          nsIDOMWindow *aChildDOMWin,
                          nsIWebProgressListener* aWebProgressListener);
  NS_IMETHOD GetIsFramesetDocument(bool *aIsFramesetDocument);
  NS_IMETHOD GetIsIFrameSelected(bool *aIsIFrameSelected);
  NS_IMETHOD GetIsRangeSelection(bool *aIsRangeSelection);
  NS_IMETHOD GetIsFramesetFrameSelected(bool *aIsFramesetFrameSelected);
  NS_IMETHOD GetPrintPreviewNumPages(int32_t *aPrintPreviewNumPages);
  NS_IMETHOD EnumerateDocumentNames(uint32_t* aCount, char16_t*** aResult);
  static nsresult GetGlobalPrintSettings(nsIPrintSettings** aPrintSettings);
  NS_IMETHOD GetDoingPrint(bool *aDoingPrint);
  NS_IMETHOD GetDoingPrintPreview(bool *aDoingPrintPreview);
  NS_IMETHOD GetCurrentPrintSettings(nsIPrintSettings **aCurrentPrintSettings);


  
  
  enum eDocTitleDefault {
    eDocTitleDefBlank,
    eDocTitleDefURLDoc
  };

  nsPrintEngine();

  void Destroy();
  void DestroyPrintingData();

  nsresult Initialize(nsIDocumentViewerPrint* aDocViewerPrint, 
                      nsIDocShell*            aContainer,
                      nsIDocument*            aDocument,
                      float                   aScreenDPI,
                      FILE*                   aDebugFile);

  nsresult GetSeqFrameAndCountPages(nsIFrame*& aSeqFrame, int32_t& aCount);

  
  
  
  nsresult DocumentReadyForPrinting();
  nsresult GetSelectionDocument(nsIDeviceContextSpec * aDevSpec,
                                nsIDocument ** aNewDoc);

  nsresult SetupToPrintContent();
  nsresult EnablePOsForPrinting();
  nsPrintObject* FindSmallestSTF();

  bool     PrintDocContent(nsPrintObject* aPO, nsresult& aStatus);
  nsresult DoPrint(nsPrintObject * aPO);

  void SetPrintPO(nsPrintObject* aPO, bool aPrint);

  void TurnScriptingOn(bool aDoTurnOn);
  bool CheckDocumentForPPCaching();
  void InstallPrintPreviewListener();

  
  bool     HasPrintCallbackCanvas();
  bool     PrePrintPage();
  bool     PrintPage(nsPrintObject* aPOect, bool& aInRange);
  bool     DonePrintingPages(nsPrintObject* aPO, nsresult aResult);

  
  void BuildDocTree(nsIDocShell *      aParentNode,
                    nsTArray<nsPrintObject*> * aDocList,
                    nsPrintObject *            aPO);
  nsresult ReflowDocList(nsPrintObject * aPO, bool aSetPixelScale);

  nsresult ReflowPrintObject(nsPrintObject * aPO);

  void CheckForChildFrameSets(nsPrintObject* aPO);

  void CalcNumPrintablePages(int32_t& aNumPages);
  void ShowPrintProgress(bool aIsForPrinting, bool& aDoNotify);
  nsresult CleanupOnFailure(nsresult aResult, bool aIsPrinting);
  
  
  nsresult FinishPrintPreview();
  static void CloseProgressDialog(nsIWebProgressListener* aWebProgressListener);
  void SetDocAndURLIntoProgress(nsPrintObject* aPO,
                                nsIPrintProgressParams* aParams);
  void EllipseLongString(nsAString& aStr, const uint32_t aLen, bool aDoFront);
  nsresult CheckForPrinters(nsIPrintSettings* aPrintSettings);
  void CleanupDocTitleArray(char16_t**& aArray, int32_t& aCount);

  bool IsThereARangeSelection(nsIDOMWindow * aDOMWin);

  


  
  nsresult StartPagePrintTimer(nsPrintObject* aPO);

  bool IsWindowsInOurSubTree(nsPIDOMWindow * aDOMWindow);
  static bool IsParentAFrameSet(nsIDocShell * aParent);
  bool IsThereAnIFrameSelected(nsIDocShell* aDocShell,
                                 nsIDOMWindow* aDOMWin,
                                 bool& aIsParentFrameSet);

  static nsPrintObject* FindPrintObjectByDOMWin(nsPrintObject* aParentObject,
                                                nsIDOMWindow* aDOMWin);

  
  already_AddRefed<nsIDOMWindow> FindFocusedDOMWindow();

  
  
  
  static void GetDocumentTitleAndURL(nsIDocument* aDoc,
                                     nsAString&   aTitle,
                                     nsAString&   aURLStr);
  void GetDisplayTitleAndURL(nsPrintObject*   aPO,
                             nsAString&       aTitle,
                             nsAString&       aURLStr,
                             eDocTitleDefault aDefType);
  static void ShowPrintErrorDialog(nsresult printerror,
                                   bool aIsPrinting = true);

  static bool HasFramesetChild(nsIContent* aContent);

  bool     CheckBeforeDestroy();
  nsresult Cancelled();

  nsIPresShell* GetPrintPreviewPresShell() {return mPrtPreview->mPrintObject->mPresShell;}

  float GetPrintPreviewScale() { return mPrtPreview->mPrintObject->
                                        mPresContext->GetPrintPreviewScale(); }
  
  static nsIPresShell* GetPresShellFor(nsIDocShell* aDocShell);

  
  void SetIsPrinting(bool aIsPrinting);
  bool GetIsPrinting()
  {
    return mIsDoingPrinting;
  }
  void SetIsPrintPreview(bool aIsPrintPreview);
  bool GetIsPrintPreview()
  {
    return mIsDoingPrintPreview;
  }
  void SetIsCreatingPrintPreview(bool aIsCreatingPrintPreview)
  {
    mIsCreatingPrintPreview = aIsCreatingPrintPreview;
  }
  bool GetIsCreatingPrintPreview()
  {
    return mIsCreatingPrintPreview;
  }

  void SetDisallowSelectionPrint(bool aDisallowSelectionPrint)
  {
    mDisallowSelectionPrint = aDisallowSelectionPrint;
  }

  void SetNoMarginBoxes(bool aNoMarginBoxes) {
    mNoMarginBoxes = aNoMarginBoxes;
  }

protected:
  ~nsPrintEngine();

  nsresult CommonPrint(bool aIsPrintPreview, nsIPrintSettings* aPrintSettings,
                       nsIWebProgressListener* aWebProgressListener,
                       nsIDOMDocument* aDoc);

  nsresult DoCommonPrint(bool aIsPrintPreview, nsIPrintSettings* aPrintSettings,
                         nsIWebProgressListener* aWebProgressListener,
                         nsIDOMDocument* aDoc);

  void FirePrintCompletionEvent();
  static nsresult GetSeqFrameAndCountPagesInternal(nsPrintObject*  aPO,
                                                   nsIFrame*&      aSeqFrame,
                                                   int32_t&        aCount);

  static nsresult FindSelectionBoundsWithList(nsPresContext* aPresContext,
                                              nsRenderingContext& aRC,
                                              nsFrameList::Enumerator& aChildFrames,
                                              nsIFrame *      aParentFrame,
                                              nsRect&         aRect,
                                              nsIFrame *&     aStartFrame,
                                              nsRect&         aStartRect,
                                              nsIFrame *&     aEndFrame,
                                              nsRect&         aEndRect);

  static nsresult FindSelectionBounds(nsPresContext* aPresContext,
                                      nsRenderingContext& aRC,
                                      nsIFrame *      aParentFrame,
                                      nsRect&         aRect,
                                      nsIFrame *&     aStartFrame,
                                      nsRect&         aStartRect,
                                      nsIFrame *&     aEndFrame,
                                      nsRect&         aEndRect);

  static nsresult GetPageRangeForSelection(nsIPresShell *        aPresShell,
                                           nsPresContext*        aPresContext,
                                           nsRenderingContext&   aRC,
                                           nsISelection*         aSelection,
                                           nsIPageSequenceFrame* aPageSeqFrame,
                                           nsIFrame**            aStartFrame,
                                           int32_t&              aStartPageNum,
                                           nsRect&               aStartRect,
                                           nsIFrame**            aEndFrame,
                                           int32_t&              aEndPageNum,
                                           nsRect&               aEndRect);

  static void MapContentForPO(nsPrintObject* aPO, nsIContent* aContent);

  static void MapContentToWebShells(nsPrintObject* aRootPO, nsPrintObject* aPO);

  static void SetPrintAsIs(nsPrintObject* aPO, bool aAsIs = true);

  
  bool mIsCreatingPrintPreview;
  bool mIsDoingPrinting;
  bool mIsDoingPrintPreview; 
  bool mProgressDialogIsShown;

  nsCOMPtr<nsIDocumentViewerPrint> mDocViewerPrint;
  nsWeakPtr               mContainer;
  float                   mScreenDPI;
  
  nsPrintData*            mPrt;
  nsPagePrintTimer*       mPagePrintTimer;
  nsIPageSequenceFrame*   mPageSeqFrame;

  
  nsPrintData*            mPrtPreview;
  nsPrintData*            mOldPrtPreview;

  nsCOMPtr<nsIDocument>   mDocument;

  FILE* mDebugFile;

  int32_t mLoadCounter;
  bool mDidLoadDataForPrinting;
  bool mIsDestroying;
  bool mDisallowSelectionPrint;
  bool mNoMarginBoxes;

  nsresult AfterNetworkPrint(bool aHandleError);

  nsresult SetRootView(nsPrintObject* aPO,
                       bool& aDoReturn,
                       bool& aDocumentIsTopLevel,
                       nsSize& aAdjSize);
  nsView* GetParentViewForRoot();
  bool DoSetPixelScale();
  void UpdateZoomRatio(nsPrintObject* aPO, bool aSetPixelScale);
  nsresult ReconstructAndReflow(bool aDoSetPixelScale);
  nsresult UpdateSelectionAndShrinkPrintObject(nsPrintObject* aPO,
                                               bool aDocumentIsTopLevel);
  nsresult InitPrintDocConstruction(bool aHandleError);
  void FirePrintPreviewUpdateEvent();
private:
  nsPrintEngine& operator=(const nsPrintEngine& aOther) = delete;
};

#endif 

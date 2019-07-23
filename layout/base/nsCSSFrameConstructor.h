









































#ifndef nsCSSFrameConstructor_h___
#define nsCSSFrameConstructor_h___

#include "nsCOMPtr.h"
#include "nsILayoutHistoryState.h"
#include "nsIXBLService.h"
#include "nsQuoteList.h"
#include "nsCounterManager.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsThreadUtils.h"
#include "nsPageContentFrame.h"

class nsIDocument;
struct nsFrameItems;
struct nsAbsoluteItems;
class nsStyleContext;
struct nsStyleContent;
struct nsStyleDisplay;
class nsIPresShell;
class nsVoidArray;
class nsFrameManager;
class nsIDOMHTMLSelectElement;
class nsPresContext;
class nsStyleChangeList;
class nsIFrame;

struct nsFindFrameHint
{
  nsIFrame *mPrimaryFrameForPrevSibling;  
  nsFindFrameHint() : mPrimaryFrameForPrevSibling(nsnull) { }
};

typedef void (PR_CALLBACK nsLazyFrameConstructionCallback)
             (nsIContent* aContent, nsIFrame* aFrame, void* aArg);

class nsFrameConstructorState;
class nsFrameConstructorSaveState;
  
class nsCSSFrameConstructor
{
public:
  nsCSSFrameConstructor(nsIDocument *aDocument, nsIPresShell* aPresShell);
  ~nsCSSFrameConstructor(void) { }

  
  static nsIXBLService * GetXBLService();
  static void ReleaseGlobals() { NS_IF_RELEASE(gXBLService); }

  
  static void GetAlternateTextFor(nsIContent*    aContent,
                                  nsIAtom*       aTag,  
                                  nsXPIDLString& aAltText);
private: 
  
  nsCSSFrameConstructor(const nsCSSFrameConstructor& aCopy); 
  nsCSSFrameConstructor& operator=(const nsCSSFrameConstructor& aCopy); 

public:
  
  nsresult ConstructRootFrame(nsIContent*     aDocElement,
                              nsIFrame**      aNewFrame);

  nsresult ReconstructDocElementHierarchy();

  nsresult ContentAppended(nsIContent*     aContainer,
                           PRInt32         aNewIndexInContainer);

  nsresult ContentInserted(nsIContent*            aContainer,
                           nsIContent*            aChild,
                           PRInt32                aIndexInContainer,
                           nsILayoutHistoryState* aFrameState);

  nsresult ContentRemoved(nsIContent*     aContainer,
                          nsIContent*     aChild,
                          PRInt32         aIndexInContainer,
                          PRBool          aInReinsertContent);

  nsresult CharacterDataChanged(nsIContent*     aContent,
                                PRBool          aAppend);

  nsresult ContentStatesChanged(nsIContent*     aContent1,
                                nsIContent*     aContent2,
                                PRInt32         aStateMask);

  
  
  
  
  
  
  
  nsresult AddLazyChildren(nsIContent* aContent,
                           nsLazyFrameConstructionCallback* aCallback,
                           void* aArg);

  
  
  void NotifyDestroyingFrame(nsIFrame* aFrame);

  nsresult AttributeChanged(nsIContent* aContent,
                            PRInt32     aNameSpaceID,
                            nsIAtom*    aAttribute,
                            PRInt32     aModType,
                            PRUint32    aStateMask);

  void BeginUpdate() { ++mUpdateCount; }
  void EndUpdate();
  void RecalcQuotesAndCounters();

  void WillDestroyFrameTree();

  
  
  
  
  
  nsresult ProcessRestyledFrames(nsStyleChangeList& aRestyleArray);

private:
  
  
  
  
  
  void ProcessOneRestyle(nsIContent* aContent, nsReStyleHint aRestyleHint,
                         nsChangeHint aChangeHint);

public:
  
  
  
  
  void ProcessPendingRestyles();

  void PostRestyleEvent(nsIContent* aContent, nsReStyleHint aRestyleHint,
                        nsChangeHint aMinChangeHint);

  
  nsresult CreateContinuingFrame(nsPresContext* aPresContext,
                                 nsIFrame*       aFrame,
                                 nsIFrame*       aParentFrame,
                                 nsIFrame**      aContinuingFrame,
                                 PRBool          aIsFluid = PR_TRUE);

  
  nsresult ReplicateFixedFrames(nsPageContentFrame* aParentFrame);

  
  
  
  nsresult FindPrimaryFrameFor(nsFrameManager*  aFrameManager,
                               nsIContent*      aContent,
                               nsIFrame**       aFrame,
                               nsFindFrameHint* aHint);

  
  nsresult GetInsertionPoint(nsIFrame*     aParentFrame,
                             nsIContent*   aChildContent,
                             nsIFrame**    aInsertionPoint,
                             PRBool*       aMultiple = nsnull);

  nsresult CreateListBoxContent(nsPresContext* aPresContext,
                                nsIFrame*       aParentFrame,
                                nsIFrame*       aPrevFrame,
                                nsIContent*     aChild,
                                nsIFrame**      aResult,
                                PRBool          aIsAppend,
                                PRBool          aIsScrollbar,
                                nsILayoutHistoryState* aFrameState);

  nsresult RemoveMappingsForFrameSubtree(nsIFrame* aRemovedFrame);

  nsIFrame* GetInitialContainingBlock() { return mInitialContainingBlock; }
  nsIFrame* GetPageSequenceFrame() { return mPageSequenceFrame; }

private:

  nsresult ReconstructDocElementHierarchyInternal();

  nsresult ReinsertContent(nsIContent*    aContainer,
                           nsIContent*    aChild);

  nsresult ConstructPageFrame(nsIPresShell*  aPresShell, 
                              nsPresContext* aPresContext,
                              nsIFrame*      aParentFrame,
                              nsIFrame*      aPrevPageFrame,
                              nsIFrame*&     aPageFrame,
                              nsIFrame*&     aPageContentFrame);

  void DoContentStateChanged(nsIContent*     aContent,
                             PRInt32         aStateMask);

  
  void RestyleElement(nsIContent*     aContent,
                      nsIFrame*       aPrimaryFrame,
                      nsChangeHint    aMinHint);

  void RestyleLaterSiblings(nsIContent*     aContent);

  nsresult InitAndRestoreFrame (const nsFrameConstructorState& aState,
                                nsIContent*                    aContent,
                                nsIFrame*                      aParentFrame,
                                nsIFrame*                      aPrevInFlow,
                                nsIFrame*                      aNewFrame,
                                PRBool                         aAllowCounters = PR_TRUE);

  already_AddRefed<nsStyleContext>
  ResolveStyleContext(nsIFrame*         aParentFrame,
                      nsIContent*       aContent);

  nsresult ConstructFrame(nsFrameConstructorState& aState,
                          nsIContent*              aContent,
                          nsIFrame*                aParentFrame,
                          nsFrameItems&            aFrameItems);

  nsresult ConstructDocElementFrame(nsFrameConstructorState& aState,
                                    nsIContent*              aDocElement,
                                    nsIFrame*                aParentFrame,
                                    nsIFrame**               aNewFrame);

  nsresult ConstructDocElementTableFrame(nsIContent*            aDocElement,
                                         nsIFrame*              aParentFrame,
                                         nsIFrame**             aNewTableFrame,
                                         nsFrameConstructorState& aState);

  













  nsresult CreateAttributeContent(nsIContent* aParentContent,
                                  nsIFrame* aParentFrame,
                                  PRInt32 aAttrNamespace,
                                  nsIAtom* aAttrName,
                                  nsStyleContext* aStyleContext,
                                  nsCOMArray<nsIContent>& aGeneratedContent,
                                  nsIContent** aNewContent,
                                  nsIFrame** aNewFrame);
  
  nsresult CreateGeneratedFrameFor(nsIFrame*             aParentFrame,
                                   nsIContent*           aContent,
                                   nsStyleContext*       aStyleContext,
                                   const nsStyleContent* aStyleContent,
                                   PRUint32              aContentIndex,
                                   nsCOMArray<nsIContent>& aGeneratedContent,
                                   nsIFrame**            aFrame);

  PRBool CreateGeneratedContentFrame(nsFrameConstructorState& aState,
                                     nsIFrame*                aFrame,
                                     nsIContent*              aContent,
                                     nsStyleContext*          aStyleContext,
                                     nsIAtom*                 aPseudoElement,
                                     nsIFrame**               aResult);

  
  
  
  nsresult AppendFrames(nsFrameConstructorState&       aState,
                        nsIContent*                    aContainer,
                        nsIFrame*                      aParentFrame,
                        nsFrameItems&                  aFrameList,
                        nsIFrame*                      aAfterFrame);

  
  








 
  nsresult ConstructTableFrame(nsFrameConstructorState& aState,
                               nsIContent*              aContent,
                               nsIFrame*                aContentParent,
                               nsStyleContext*          aStyleContext,
                               PRInt32                  aNameSpaceID,
                               PRBool                   aIsPseudo,
                               nsFrameItems&            aChildItems,
                               PRBool                   aAllowOutOfFlow,
                               nsIFrame*&               aNewOuterFrame,
                               nsIFrame*&               aNewInnerFrame);

  nsresult ConstructTableCaptionFrame(nsFrameConstructorState& aState,
                                      nsIContent*              aContent,
                                      nsIFrame*                aParent,
                                      nsStyleContext*          aStyleContext,
                                      PRInt32                  aNameSpaceID,
                                      nsFrameItems&            aChildItems,
                                      nsIFrame*&               aNewFrame,
                                      PRBool&                  aIsPseudoParent);

  nsresult ConstructTableRowGroupFrame(nsFrameConstructorState& aState,
                                       nsIContent*              aContent,
                                       nsIFrame*                aParent,
                                       nsStyleContext*          aStyleContext,
                                       PRInt32                  aNameSpaceID,
                                       PRBool                   aIsPseudo,
                                       nsFrameItems&            aChildItems,
                                       nsIFrame*&               aNewFrame,
                                       PRBool&                  aIsPseudoParent);

  nsresult ConstructTableColGroupFrame(nsFrameConstructorState& aState,
                                       nsIContent*              aContent,
                                       nsIFrame*                aParent,
                                       nsStyleContext*          aStyleContext,
                                       PRInt32                  aNameSpaceID,
                                       PRBool                   aIsPseudo,
                                       nsFrameItems&            aChildItems,
                                       nsIFrame*&               aNewFrame,
                                       PRBool&                  aIsPseudoParent);

  nsresult ConstructTableRowFrame(nsFrameConstructorState& aState,
                                  nsIContent*              aContent,
                                  nsIFrame*                aParent,
                                  nsStyleContext*          aStyleContext,
                                  PRInt32                  aNameSpaceID,
                                  PRBool                   aIsPseudo,
                                  nsFrameItems&            aChildItems,
                                  nsIFrame*&               aNewFrame,
                                  PRBool&                  aIsPseudoParent);

  nsresult ConstructTableColFrame(nsFrameConstructorState& aState,
                                  nsIContent*              aContent,
                                  nsIFrame*                aParent,
                                  nsStyleContext*          aStyleContext,
                                  PRInt32                  aNameSpaceID,
                                  PRBool                   aIsPseudo,
                                  nsFrameItems&            aChildItems,
                                  nsIFrame*&               aNewFrame,
                                  PRBool&                  aIsPseudoParent);

  nsresult ConstructTableCellFrame(nsFrameConstructorState& aState,
                                   nsIContent*              aContent,
                                   nsIFrame*                aParentFrame,
                                   nsStyleContext*          aStyleContext,
                                   PRInt32                  aNameSpaceID,
                                   PRBool                   aIsPseudo,
                                   nsFrameItems&            aChildItems,
                                   nsIFrame*&               aNewCellOuterFrame,
                                   nsIFrame*&               aNewCellInnerFrame,
                                   PRBool&                  aIsPseudoParent);

  nsresult CreatePseudoTableFrame(PRInt32                  aNameSpaceID,
                                  nsFrameConstructorState& aState, 
                                  nsIFrame*                aParentFrameIn = nsnull);

  nsresult CreatePseudoRowGroupFrame(PRInt32                  aNameSpaceID,
                                     nsFrameConstructorState& aState, 
                                     nsIFrame*                aParentFrameIn = nsnull);

  nsresult CreatePseudoColGroupFrame(PRInt32                  aNameSpaceID,
                                     nsFrameConstructorState& aState, 
                                     nsIFrame*                aParentFrameIn = nsnull);

  nsresult CreatePseudoRowFrame(PRInt32                  aNameSpaceID,
                                nsFrameConstructorState& aState, 
                                nsIFrame*                aParentFrameIn = nsnull);

  nsresult CreatePseudoCellFrame(PRInt32                  aNameSpaceID,
                                 nsFrameConstructorState& aState, 
                                 nsIFrame*                aParentFrameIn = nsnull);

  nsresult GetPseudoTableFrame(PRInt32                  aNameSpaceID,
                               nsFrameConstructorState& aState, 
                               nsIFrame&                aParentFrameIn);

  nsresult GetPseudoColGroupFrame(PRInt32                  aNameSpaceID,
                                  nsFrameConstructorState& aState, 
                                  nsIFrame&                aParentFrameIn);

  nsresult GetPseudoRowGroupFrame(PRInt32                  aNameSpaceID,
                                  nsFrameConstructorState& aState, 
                                  nsIFrame&                aParentFrameIn);

  nsresult GetPseudoRowFrame(PRInt32                  aNameSpaceID,
                             nsFrameConstructorState& aState, 
                             nsIFrame&                aParentFrameIn);

  nsresult GetPseudoCellFrame(PRInt32                  aNameSpaceID,
                              nsFrameConstructorState& aState, 
                              nsIFrame&                aParentFrameIn);

  nsresult GetParentFrame(PRInt32                  aNameSpaceID,
                          nsIFrame&                aParentFrameIn, 
                          nsIAtom*                 aChildFrameType, 
                          nsFrameConstructorState& aState, 
                          nsIFrame*&               aParentFrame,
                          PRBool&                  aIsPseudoParent);

  



















  
  
  
  
  nsresult AdjustParentFrame(nsFrameConstructorState&     aState,
                             nsIContent*                  aChildContent,
                             nsIFrame* &                  aParentFrame,
                             nsIAtom*                     aTag,
                             PRInt32                      aNameSpaceID,
                             nsStyleContext*              aChildStyle,
                             nsFrameItems* &              aFrameItems,
                             nsFrameConstructorSaveState& aSaveState,
                             PRBool&                      aSuppressFrame,
                             PRBool&                      aCreatedPseudo);

  const nsStyleDisplay* GetDisplay(nsIFrame* aFrame);

  

protected:
  static nsresult CreatePlaceholderFrameFor(nsIPresShell*    aPresShell, 
                                            nsPresContext*  aPresContext,
                                            nsFrameManager*  aFrameManager,
                                            nsIContent*      aContent,
                                            nsIFrame*        aFrame,
                                            nsStyleContext*  aStyleContext,
                                            nsIFrame*        aParentFrame,
                                            nsIFrame**       aPlaceholderFrame);

private:
  
  nsresult ConstructRadioControlFrame(nsIFrame**         aNewFrame,
                                      nsIContent*        aContent,
                                      nsStyleContext*    aStyleContext);

  
  nsresult ConstructCheckboxControlFrame(nsIFrame**       aNewFrame,
                                         nsIContent*      aContent,
                                         nsStyleContext*  aStyleContext);
  
  
  nsresult ConstructButtonFrame(nsFrameConstructorState& aState,
                                nsIContent*              aContent,
                                nsIFrame*                aParentFrame,
                                nsIAtom*                 aTag,
                                nsStyleContext*          aStyleContext,
                                nsIFrame**               aNewFrame,
                                const nsStyleDisplay*    aStyleDisplay,
                                nsFrameItems&            aFrameItems,
                                PRBool                   aHasPseudoParent);

  
  
  nsresult ConstructSelectFrame(nsFrameConstructorState& aState,
                                nsIContent*              aContent,
                                nsIFrame*                aParentFrame,
                                nsIAtom*                 aTag,
                                nsStyleContext*          aStyleContext,
                                nsIFrame*&               aNewFrame,
                                const nsStyleDisplay*    aStyleDisplay,
                                PRBool&                  aFrameHasBeenInitialized,
                                nsFrameItems&            aFrameItems);

  
  
  nsresult ConstructFieldSetFrame(nsFrameConstructorState& aState,
                                  nsIContent*              aContent,
                                  nsIFrame*                aParentFrame,
                                  nsIAtom*                 aTag,
                                  nsStyleContext*          aStyleContext,
                                  nsIFrame*&               aNewFrame,
                                  nsFrameItems&            aFrameItems,
                                  const nsStyleDisplay*    aStyleDisplay,
                                  PRBool&                  aFrameHasBeenInitialized);

  nsresult ConstructTextFrame(nsFrameConstructorState& aState,
                              nsIContent*              aContent,
                              nsIFrame*                aParentFrame,
                              nsStyleContext*          aStyleContext,
                              nsFrameItems&            aFrameItems,
                              PRBool                   aPseudoParent);

  nsresult ConstructPageBreakFrame(nsFrameConstructorState& aState,
                                   nsIContent*              aContent,
                                   nsIFrame*                aParentFrame,
                                   nsStyleContext*          aStyleContext,
                                   nsFrameItems&            aFrameItems);

  
  
  
  PRBool PageBreakBefore(nsFrameConstructorState& aState,
                         nsIContent*              aContent,
                         nsIFrame*                aParentFrame,
                         nsStyleContext*          aStyleContext,
                         nsFrameItems&            aFrameItems);

  nsresult ConstructHTMLFrame(nsFrameConstructorState& aState,
                              nsIContent*              aContent,
                              nsIFrame*                aParentFrame,
                              nsIAtom*                 aTag,
                              PRInt32                  aNameSpaceID,
                              nsStyleContext*          aStyleContext,
                              nsFrameItems&            aFrameItems,
                              PRBool                   aHasPseudoParent);

  nsresult ConstructFrameInternal( nsFrameConstructorState& aState,
                                   nsIContent*              aContent,
                                   nsIFrame*                aParentFrame,
                                   nsIAtom*                 aTag,
                                   PRInt32                  aNameSpaceID,
                                   nsStyleContext*          aStyleContext,
                                   nsFrameItems&            aFrameItems,
                                   PRBool                   aXBLBaseTag);

  nsresult CreateAnonymousFrames(nsIAtom*                 aTag,
                                 nsFrameConstructorState& aState,
                                 nsIContent*              aParent,
                                 nsIFrame*                aNewFrame,
                                 PRBool                   aAppendToExisting,
                                 nsFrameItems&            aChildItems,
                                 PRBool                   aIsRoot = PR_FALSE);

  nsresult CreateAnonymousFrames(nsFrameConstructorState& aState,
                                 nsIContent*              aParent,
                                 nsIDocument*             aDocument,
                                 nsIFrame*                aNewFrame,
                                 PRBool                   aAppendToExisting,
                                 nsFrameItems&            aChildItems);


#ifdef MOZ_MATHML
  nsresult ConstructMathMLFrame(nsFrameConstructorState& aState,
                                nsIContent*              aContent,
                                nsIFrame*                aParentFrame,
                                nsIAtom*                 aTag,
                                PRInt32                  aNameSpaceID,
                                nsStyleContext*          aStyleContext,
                                nsFrameItems&            aFrameItems,
                                PRBool                   aHasPseudoParent);
#endif

  nsresult ConstructXULFrame(nsFrameConstructorState& aState,
                             nsIContent*              aContent,
                             nsIFrame*                aParentFrame,
                             nsIAtom*                 aTag,
                             PRInt32                  aNameSpaceID,
                             nsStyleContext*          aStyleContext,
                             nsFrameItems&            aFrameItems,
                             PRBool                   aXBLBaseTag,
                             PRBool                   aHasPseudoParent,
                             PRBool*                  aHaltProcessing);



#ifdef MOZ_XTF
  nsresult ConstructXTFFrame(nsFrameConstructorState& aState,
                             nsIContent*              aContent,
                             nsIFrame*                aParentFrame,
                             nsIAtom*                 aTag,
                             PRInt32                  aNameSpaceID,
                             nsStyleContext*          aStyleContext,
                             nsFrameItems&            aFrameItems,
                             PRBool                   aHasPseudoParent);
#endif


#ifdef MOZ_SVG
  nsresult TestSVGConditions(nsIContent* aContent,
                             PRBool&     aHasRequiredExtensions,
                             PRBool&     aHasRequiredFeatures,
                             PRBool&     aHasSystemLanguage);
 
  nsresult SVGSwitchProcessChildren(nsFrameConstructorState& aState,
                                    nsIContent*              aContent,
                                    nsIFrame*                aFrame,
                                    nsFrameItems&            aFrameItems);

  nsresult ConstructSVGFrame(nsFrameConstructorState& aState,
                             nsIContent*              aContent,
                             nsIFrame*                aParentFrame,
                             nsIAtom*                 aTag,
                             PRInt32                  aNameSpaceID,
                             nsStyleContext*          aStyleContext,
                             nsFrameItems&            aFrameItems,
                             PRBool                   aHasPseudoParent,
                             PRBool*                  aHaltProcessing);
#endif

  nsresult ConstructFrameByDisplayType(nsFrameConstructorState& aState,
                                       const nsStyleDisplay*    aDisplay,
                                       nsIContent*              aContent,
                                       PRInt32                  aNameSpaceID,
                                       nsIAtom*                 aTag,
                                       nsIFrame*                aParentFrame,
                                       nsStyleContext*          aStyleContext,
                                       nsFrameItems&            aFrameItems,
                                       PRBool                   aHasPseudoParent);

  nsresult ProcessChildren(nsFrameConstructorState& aState,
                           nsIContent*              aContent,
                           nsIFrame*                aFrame,
                           PRBool                   aCanHaveGeneratedContent,
                           nsFrameItems&            aFrameItems,
                           PRBool                   aParentIsBlock);

  
  nsresult CreateInputFrame(nsFrameConstructorState& aState,
                            nsIContent*              aContent,
                            nsIFrame*                aParentFrame,
                            nsIAtom*                 aTag,
                            nsStyleContext*          aStyleContext,
                            nsIFrame**               aFrame,
                            const nsStyleDisplay*    aStyleDisplay,
                            PRBool&                  aFrameHasBeenInitialized,
                            PRBool&                  aAddedToFrameList,
                            nsFrameItems&            aFrameItems,
                            PRBool                   aHasPseudoParent);

  
  typedef nsIFrame* (* ImageFrameCreatorFunc)(nsIPresShell*, nsStyleContext*);

  





  nsresult CreateHTMLImageFrame(nsIContent*           aContent,
                                nsStyleContext*       aStyleContext,
                                ImageFrameCreatorFunc aFunc,
                                nsIFrame**            aFrame);

  nsIFrame* GetFrameFor(nsIContent* aContent);

  







  nsIFrame* GetAbsoluteContainingBlock(nsIFrame* aFrame);
  nsIFrame* GetFloatContainingBlock(nsIFrame* aFrame);

  nsIContent* PropagateScrollToViewport();

  
  
  
  
  nsresult
  BuildScrollFrame(nsFrameConstructorState& aState,
                   nsIContent*              aContent,
                   nsStyleContext*          aContentStyle,
                   nsIFrame*                aScrolledFrame,
                   nsIFrame*                aParentFrame,
                   nsIFrame*                aContentParentFrame,
                   nsIFrame*&               aNewFrame,
                   nsStyleContext*&         aScrolledChildStyle);

  
  already_AddRefed<nsStyleContext>
  BeginBuildingScrollFrame(nsFrameConstructorState& aState,
                           nsIContent*              aContent,
                           nsStyleContext*          aContentStyle,
                           nsIFrame*                aParentFrame,
                           nsIFrame*                aContentParentFrame,
                           nsIAtom*                 aScrolledPseudo,
                           PRBool                   aIsRoot,
                           nsIFrame*&               aNewFrame);

  
  
  void
  FinishBuildingScrollFrame(nsIFrame* aScrollFrame,
                            nsIFrame* aScrolledFrame);

  
  nsresult
  InitializeSelectFrame(nsFrameConstructorState& aState,
                        nsIFrame*                scrollFrame,
                        nsIFrame*                scrolledFrame,
                        nsIContent*              aContent,
                        nsIFrame*                aParentFrame,
                        nsStyleContext*          aStyleContext,
                        PRBool                   aBuildCombobox,
                        nsFrameItems&            aFrameItems);

  nsresult MaybeRecreateFramesForContent(nsIContent*      aContent);

  nsresult RecreateFramesForContent(nsIContent*      aContent);

  
  
  
  
  
  
  
  
  PRBool MaybeRecreateContainerForIBSplitterFrame(nsIFrame* aFrame,
                                                  nsresult* aResult);

  nsresult CreateContinuingOuterTableFrame(nsIPresShell*    aPresShell, 
                                           nsPresContext*  aPresContext,
                                           nsIFrame*        aFrame,
                                           nsIFrame*        aParentFrame,
                                           nsIContent*      aContent,
                                           nsStyleContext*  aStyleContext,
                                           nsIFrame**       aContinuingFrame);

  nsresult CreateContinuingTableFrame(nsIPresShell*    aPresShell, 
                                      nsPresContext*  aPresContext,
                                      nsIFrame*        aFrame,
                                      nsIFrame*        aParentFrame,
                                      nsIContent*      aContent,
                                      nsStyleContext*  aStyleContext,
                                      nsIFrame**       aContinuingFrame);

  

  

  already_AddRefed<nsStyleContext>
  GetFirstLetterStyle(nsIContent*      aContent,
                      nsStyleContext*  aStyleContext);

  already_AddRefed<nsStyleContext>
  GetFirstLineStyle(nsIContent*      aContent,
                    nsStyleContext*  aStyleContext);

  PRBool ShouldHaveFirstLetterStyle(nsIContent*      aContent,
                                    nsStyleContext*  aStyleContext);

  
  
  PRBool HasFirstLetterStyle(nsIFrame* aBlockFrame);

  PRBool ShouldHaveFirstLineStyle(nsIContent*      aContent,
                                  nsStyleContext*  aStyleContext);

  void ShouldHaveSpecialBlockStyle(nsIContent*      aContent,
                                   nsStyleContext*  aStyleContext,
                                   PRBool*          aHaveFirstLetterStyle,
                                   PRBool*          aHaveFirstLineStyle);

  
  
  
  
  
  
  
  
  
  
  
  nsresult ConstructBlock(nsFrameConstructorState& aState,
                          const nsStyleDisplay*    aDisplay,
                          nsIContent*              aContent,
                          nsIFrame*                aParentFrame,
                          nsIFrame*                aContentParentFrame,
                          nsStyleContext*          aStyleContext,
                          nsIFrame**               aNewFrame,
                          nsFrameItems&            aFrameItems,
                          PRBool                   aAbsPosContainer);

  nsresult ConstructInline(nsFrameConstructorState& aState,
                           const nsStyleDisplay*    aDisplay,
                           nsIContent*              aContent,
                           nsIFrame*                aParentFrame,
                           nsStyleContext*          aStyleContext,
                           PRBool                   aIsPositioned,
                           nsIFrame*                aNewFrame);

  


















  nsIFrame* MoveFramesToEndOfIBSplit(nsFrameConstructorState& aState,
                                     nsIFrame* aExistingEndFrame,
                                     PRBool aIsPositioned,
                                     nsIContent* aContent,
                                     nsStyleContext* aStyleContext,
                                     nsIFrame* aFramesToMove,
                                     nsIFrame* aBlockPart,
                                     nsFrameConstructorState* aTargetState);

  nsresult ProcessInlineChildren(nsFrameConstructorState& aState,
                                 nsIContent*              aContent,
                                 nsIFrame*                aFrame,
                                 PRBool                   aCanHaveGeneratedContent,
                                 nsFrameItems&            aFrameItems,
                                 PRBool*                  aKidsAllInline);

  PRBool AreAllKidsInline(nsIFrame* aFrameList);

  
  
  
  
  
  
  
  PRBool WipeContainingBlock(nsFrameConstructorState& aState,
                             nsIFrame*                aContainingBlock,
                             nsIFrame*                aFrame,
                             const nsFrameItems&      aFrameList,
                             PRBool                   aIsAppend,
                             nsIFrame*                aPrevSibling);

  nsresult ReframeContainingBlock(nsIFrame* aFrame);

  nsresult StyleChangeReflow(nsIFrame* aFrame);

  








  nsIFrame* FindFrameWithContent(nsFrameManager*  aFrameManager,
                                 nsIFrame*        aParentFrame,
                                 nsIContent*      aParentContent,
                                 nsIContent*      aContent,
                                 nsFindFrameHint* aHint);

  

  

  void CreateFloatingLetterFrame(nsFrameConstructorState& aState,
                                 nsIFrame*                aBlockFrame,
                                 nsIContent*              aTextContent,
                                 nsIFrame*                aTextFrame,
                                 nsIContent*              aBlockContent,
                                 nsIFrame*                aParentFrame,
                                 nsStyleContext*          aStyleContext,
                                 nsFrameItems&            aResult);

  nsresult CreateLetterFrame(nsFrameConstructorState& aState,
                             nsIFrame*                aBlockFrame,
                             nsIContent*              aTextContent,
                             nsIFrame*                aParentFrame,
                             nsFrameItems&            aResult);

  nsresult WrapFramesInFirstLetterFrame(nsFrameConstructorState& aState,
                                        nsIContent*              aBlockContent,
                                        nsIFrame*                aBlockFrame,
                                        nsFrameItems&            aBlockFrames);

  nsresult WrapFramesInFirstLetterFrame(nsFrameConstructorState& aState,
                                        nsIFrame*                aBlockFrame,
                                        nsIFrame*                aParentFrame,
                                        nsIFrame*                aParentFrameList,
                                        nsIFrame**               aModifiedParent,
                                        nsIFrame**               aTextFrame,
                                        nsIFrame**               aPrevFrame,
                                        nsFrameItems&            aLetterFrame,
                                        PRBool*                  aStopLooking);

  nsresult RecoverLetterFrames(nsFrameConstructorState& aState,
                               nsIFrame*                aBlockFrame);

  
  nsresult RemoveLetterFrames(nsPresContext*  aPresContext,
                              nsIPresShell*    aPresShell,
                              nsFrameManager*  aFrameManager,
                              nsIFrame*        aBlockFrame);

  
  nsresult RemoveFirstLetterFrames(nsPresContext*  aPresContext,
                                   nsIPresShell*    aPresShell,
                                   nsFrameManager*  aFrameManager,
                                   nsIFrame*        aFrame,
                                   PRBool*          aStopLooking);

  
  nsresult RemoveFloatingFirstLetterFrames(nsPresContext*  aPresContext,
                                           nsIPresShell*    aPresShell,
                                           nsFrameManager*  aFrameManager,
                                           nsIFrame*        aBlockFrame,
                                           PRBool*          aStopLooking);

  
  
  nsresult CaptureStateForFramesOf(nsIContent* aContent,
                                   nsILayoutHistoryState* aHistoryState);

  
  nsresult CaptureStateFor(nsIFrame*              aFrame,
                           nsILayoutHistoryState* aHistoryState);

  

  

  nsresult WrapFramesInFirstLineFrame(nsFrameConstructorState& aState,
                                      nsIContent*              aBlockContent,
                                      nsIFrame*                aBlockFrame,
                                      nsFrameItems&            aFrameItems);

  nsresult AppendFirstLineFrames(nsFrameConstructorState& aState,
                                 nsIContent*              aContent,
                                 nsIFrame*                aBlockFrame,
                                 nsFrameItems&            aFrameItems);

  nsresult InsertFirstLineFrames(nsFrameConstructorState& aState,
                                 nsIContent*              aContent,
                                 nsIFrame*                aBlockFrame,
                                 nsIFrame**               aParentFrame,
                                 nsIFrame*                aPrevSibling,
                                 nsFrameItems&            aFrameItems);

  nsresult RemoveFixedItems(const nsFrameConstructorState& aState);

  
  
  
  nsIFrame* FindPreviousSibling(nsIContent*       aContainer,
                                nsIFrame*         aContainerFrame,
                                PRInt32           aIndexInContainer,
                                const nsIContent* aChild = nsnull);

  
  
  nsIFrame* FindNextSibling(nsIContent*       aContainer,
                            nsIFrame*         aContainerFrame,
                            PRInt32           aIndexInContainer,
                            const nsIContent* aChild = nsnull);

  
  
  
  
  PRBool IsValidSibling(nsIFrame*              aParentFrame,
                        nsIFrame*              aSibling,
                        PRUint8                aSiblingDisplay,
                        nsIContent&            aContent,
                        PRUint8&               aDisplay);

  void QuotesDirty() {
    if (mUpdateCount != 0)
      mQuotesDirty = PR_TRUE;
    else
      mQuoteList.RecalcAll();
  }

  void CountersDirty() {
    if (mUpdateCount != 0)
      mCountersDirty = PR_TRUE;
    else
      mCounterManager.RecalcAll();
  }

public:
  struct RestyleData;
  friend struct RestyleData;

  struct RestyleData {
    nsReStyleHint mRestyleHint;  
    nsChangeHint  mChangeHint;   
  };

  struct RestyleEnumerateData : public RestyleData {
    nsCOMPtr<nsIContent> mContent;
  };

  class RestyleEvent;
  friend class RestyleEvent;

  class RestyleEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    RestyleEvent(nsCSSFrameConstructor *aConstructor)
      : mConstructor(aConstructor) {
      NS_PRECONDITION(aConstructor, "Must have a constructor!");
    }
    void Revoke() { mConstructor = nsnull; }
  private:
    nsCSSFrameConstructor *mConstructor;
  };

  friend class nsFrameConstructorState;

private:

  class LazyGenerateChildrenEvent;
  friend class LazyGenerateChildrenEvent;

  class LazyGenerateChildrenEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    LazyGenerateChildrenEvent(nsIContent *aContent,
                              nsIPresShell *aPresShell,
                              nsLazyFrameConstructionCallback* aCallback,
                              void* aArg)
      : mContent(aContent), mPresShell(aPresShell), mCallback(aCallback), mArg(aArg)
    {}

  private:
    nsCOMPtr<nsIContent> mContent;
    nsCOMPtr<nsIPresShell> mPresShell;
    nsLazyFrameConstructionCallback* mCallback;
    void* mArg;
  };

  nsIDocument*        mDocument;  
  nsIPresShell*       mPresShell; 

  nsIFrame*           mInitialContainingBlock;
  nsIFrame*           mFixedContainingBlock;
  nsIFrame*           mDocElementContainingBlock;
  nsIFrame*           mGfxScrollFrame;
  nsIFrame*           mPageSequenceFrame;
  nsQuoteList         mQuoteList;
  nsCounterManager    mCounterManager;
  PRUint16            mUpdateCount;
  PRPackedBool        mQuotesDirty : 1;
  PRPackedBool        mCountersDirty : 1;
  PRPackedBool        mInitialContainingBlockIsAbsPosContainer : 1;
  PRPackedBool        mIsDestroyingFrameTree : 1;

  nsRevocableEventPtr<RestyleEvent> mRestyleEvent;

  nsCOMPtr<nsILayoutHistoryState> mTempFrameTreeState;

  nsDataHashtable<nsISupportsHashKey, RestyleData> mPendingRestyles;

  static nsIXBLService * gXBLService;
};

#endif 





#ifndef nsTableOuterFrame_h__
#define nsTableOuterFrame_h__

#include "mozilla/Attributes.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsCellMap.h"
#include "nsBlockFrame.h"
#include "nsTableFrame.h"

class nsTableCaptionFrame : public nsBlockFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsIAtom* GetType() const;
  friend nsIFrame* NS_NewTableCaptionFrame(nsIPresShell* aPresShell, nsStyleContext*  aContext);

  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap);

  virtual nsIFrame* GetParentStyleContextFrame() const;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  nsTableCaptionFrame(nsStyleContext*  aContext);
  virtual ~nsTableCaptionFrame();
};











class nsTableOuterFrame : public nsContainerFrame
{
public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  NS_DECL_QUERYFRAME_TARGET(nsTableOuterFrame)

  




  friend nsIFrame* NS_NewTableOuterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
  
  

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList) MOZ_OVERRIDE;
 
  virtual const nsFrameList& GetChildList(ChildListID aListID) const;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const MOZ_OVERRIDE;

  NS_IMETHOD AppendFrames(ChildListID     aListID,
                          nsFrameList&    aFrameList) MOZ_OVERRIDE;

  NS_IMETHOD InsertFrames(ChildListID     aListID,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList) MOZ_OVERRIDE;

  NS_IMETHOD RemoveFrame(ChildListID     aListID,
                         nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  virtual nsIFrame* GetContentInsertionFrame() {
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  nsresult BuildDisplayListForInnerTable(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);

  virtual nscoord GetBaseline() const MOZ_OVERRIDE;

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap) MOZ_OVERRIDE;

  


  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  virtual nsIFrame* GetParentStyleContextFrame() const MOZ_OVERRIDE;

  


  nsIContent* GetCellAt(uint32_t aRowIdx, uint32_t aColIdx) const;

  


  int32_t GetRowCount() const
  {
    return InnerTableFrame()->GetRowCount();
  }

  


  int32_t GetColCount() const
  {
    return InnerTableFrame()->GetColCount();
  }

  


  int32_t GetIndexByRowAndColumn(int32_t aRowIdx, int32_t aColIdx) const
  {
    nsTableCellMap* cellMap = InnerTableFrame()->GetCellMap();
    if (!cellMap)
      return -1;

    return cellMap->GetIndexByRowAndColumn(aRowIdx, aColIdx);
  }

  


  void GetRowAndColumnByIndex(int32_t aCellIdx, int32_t* aRowIdx,
                              int32_t* aColIdx) const
  {
    *aRowIdx = *aColIdx = 0;
    nsTableCellMap* cellMap = InnerTableFrame()->GetCellMap();
    if (cellMap) {
      cellMap->GetRowAndColumnByIndex(aCellIdx, aRowIdx, aColIdx);
    }
  }

  


  nsTableCellFrame* GetCellFrameAt(uint32_t aRowIdx, uint32_t aColIdx) const
  {
    nsTableCellMap* map = InnerTableFrame()->GetCellMap();
    if (!map) {
      return nullptr;
    }

    return map->GetCellInfoAt(aRowIdx, aColIdx);
  }

  


  uint32_t GetEffectiveColSpanAt(uint32_t aRowIdx, uint32_t aColIdx) const
  {
    nsTableCellMap* map = InnerTableFrame()->GetCellMap();
    return map->GetEffectiveColSpan(aRowIdx, aColIdx);
  }

  


  uint32_t GetEffectiveRowSpanAt(uint32_t aRowIdx, uint32_t aColIdx) const
  {
    nsTableCellMap* map = InnerTableFrame()->GetCellMap();
    return map->GetEffectiveRowSpan(aRowIdx, aColIdx);
  }

protected:


  nsTableOuterFrame(nsStyleContext* aContext);
  virtual ~nsTableOuterFrame();

  void InitChildReflowState(nsPresContext&    aPresContext,                     
                            nsHTMLReflowState& aReflowState);

  uint8_t GetCaptionSide(); 

  bool HasSideCaption() {
    uint8_t captionSide = GetCaptionSide();
    return captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
           captionSide == NS_STYLE_CAPTION_SIDE_RIGHT;
  }
  
  uint8_t GetCaptionVerticalAlign();

  void SetDesiredSize(uint8_t         aCaptionSide,
                      const nsMargin& aInnerMargin,
                      const nsMargin& aCaptionMargin,
                      nscoord&        aWidth,
                      nscoord&        aHeight);

  nsresult   GetCaptionOrigin(uint32_t         aCaptionSide,
                              const nsSize&    aContainBlockSize,
                              const nsSize&    aInnerSize, 
                              const nsMargin&  aInnerMargin,
                              const nsSize&    aCaptionSize,
                              nsMargin&        aCaptionMargin,
                              nsPoint&         aOrigin);

  nsresult   GetInnerOrigin(uint32_t         aCaptionSide,
                            const nsSize&    aContainBlockSize,
                            const nsSize&    aCaptionSize, 
                            const nsMargin&  aCaptionMargin,
                            const nsSize&    aInnerSize,
                            nsMargin&        aInnerMargin,
                            nsPoint&         aOrigin);
  
  
  void OuterBeginReflowChild(nsPresContext*           aPresContext,
                             nsIFrame*                aChildFrame,
                             const nsHTMLReflowState& aOuterRS,
                             void*                    aChildRSSpace,
                             nscoord                  aAvailWidth);

  nsresult OuterDoReflowChild(nsPresContext*           aPresContext,
                              nsIFrame*                aChildFrame,
                              const nsHTMLReflowState& aChildRS,
                              nsHTMLReflowMetrics&     aMetrics,
                              nsReflowStatus&          aStatus);

  
  void UpdateReflowMetrics(uint8_t              aCaptionSide,
                           nsHTMLReflowMetrics& aMet,
                           const nsMargin&      aInnerMargin,
                           const nsMargin&      aCaptionMargin);

  
  
  void GetChildMargin(nsPresContext*           aPresContext,
                      const nsHTMLReflowState& aOuterRS,
                      nsIFrame*                aChildFrame,
                      nscoord                  aAvailableWidth,
                      nsMargin&                aMargin);

  nsTableFrame* InnerTableFrame() const {
    return static_cast<nsTableFrame*>(mFrames.FirstChild());
  }
  
private:
  nsFrameList   mCaptionFrames;
};

#endif

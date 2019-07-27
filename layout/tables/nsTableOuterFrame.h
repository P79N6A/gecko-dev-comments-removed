



#ifndef nsTableOuterFrame_h__
#define nsTableOuterFrame_h__

#include "mozilla/Attributes.h"
#include "mozilla/Maybe.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsCellMap.h"
#include "nsTableFrame.h"






class nsTableOuterFrame : public nsContainerFrame
{
public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  NS_DECL_QUERYFRAME_TARGET(nsTableOuterFrame)

  




  friend nsTableOuterFrame* NS_NewTableOuterFrame(nsIPresShell* aPresShell,
                                                  nsStyleContext* aContext);
  
  

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  virtual const nsFrameList& GetChildList(ChildListID aListID) const override;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const override;

  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) override;
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) override;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) override;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) override;

  virtual nsContainerFrame* GetContentInsertionFrame() override {
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() override;
#endif

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  void BuildDisplayListForInnerTable(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists);

  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode) const override;

  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) override;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) override;

  virtual mozilla::LogicalSize
  ComputeAutoSize(nsRenderingContext *aRenderingContext,
                  mozilla::WritingMode aWritingMode,
                  const mozilla::LogicalSize& aCBSize,
                  nscoord aAvailableISize,
                  const mozilla::LogicalSize& aMargin,
                  const mozilla::LogicalSize& aBorder,
                  const mozilla::LogicalSize& aPadding,
                  bool aShrinkWrap) override;

  


  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  




  virtual nsIAtom* GetType() const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  virtual nsStyleContext* GetParentStyleContext(nsIFrame** aProviderFrame) const override;

  


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


  explicit nsTableOuterFrame(nsStyleContext* aContext);
  virtual ~nsTableOuterFrame();

  void InitChildReflowState(nsPresContext&    aPresContext,                     
                            nsHTMLReflowState& aReflowState);

  
  
  uint8_t GetLogicalCaptionSide(mozilla::WritingMode aWM);

  bool HasSideCaption(mozilla::WritingMode aWM) {
    uint8_t captionSide = GetLogicalCaptionSide(aWM);
    return captionSide == NS_STYLE_CAPTION_SIDE_ISTART ||
           captionSide == NS_STYLE_CAPTION_SIDE_IEND;
  }
  
  uint8_t GetCaptionVerticalAlign();

  void SetDesiredSize(uint8_t                       aCaptionSide,
                      const mozilla::LogicalSize&   aInnerSize,
                      const mozilla::LogicalSize&   aCaptionSize,
                      const mozilla::LogicalMargin& aInnerMargin,
                      const mozilla::LogicalMargin& aCaptionMargin,
                      nscoord&                      aISize,
                      nscoord&                      aBSize,
                      mozilla::WritingMode          aWM);

  nsresult   GetCaptionOrigin(uint32_t         aCaptionSide,
                              const mozilla::LogicalSize&    aContainBlockSize,
                              const mozilla::LogicalSize&    aInnerSize, 
                              const mozilla::LogicalMargin&  aInnerMargin,
                              const mozilla::LogicalSize&    aCaptionSize,
                              mozilla::LogicalMargin&        aCaptionMargin,
                              mozilla::LogicalPoint&         aOrigin,
                              mozilla::WritingMode           aWM);

  nsresult   GetInnerOrigin(uint32_t         aCaptionSide,
                            const mozilla::LogicalSize&    aContainBlockSize,
                            const mozilla::LogicalSize&    aCaptionSize, 
                            const mozilla::LogicalMargin&  aCaptionMargin,
                            const mozilla::LogicalSize&    aInnerSize,
                            mozilla::LogicalMargin&        aInnerMargin,
                            mozilla::LogicalPoint&         aOrigin,
                            mozilla::WritingMode           aWM);
  
  
  void OuterBeginReflowChild(nsPresContext*                     aPresContext,
                             nsIFrame*                          aChildFrame,
                             const nsHTMLReflowState&           aOuterRS,
                             mozilla::Maybe<nsHTMLReflowState>& aChildRS,
                             nscoord                            aAvailISize);

  void OuterDoReflowChild(nsPresContext*           aPresContext,
                          nsIFrame*                aChildFrame,
                          const nsHTMLReflowState& aChildRS,
                          nsHTMLReflowMetrics&     aMetrics,
                          nsReflowStatus&          aStatus);

  
  void UpdateOverflowAreas(nsHTMLReflowMetrics& aMet);

  
  void GetChildMargin(nsPresContext*           aPresContext,
                      const nsHTMLReflowState& aOuterRS,
                      nsIFrame*                aChildFrame,
                      nscoord                  aAvailableWidth,
                      mozilla::LogicalMargin&  aMargin);

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
                                           (~eCanContainOverflowContainers));
  }

  nsTableFrame* InnerTableFrame() const {
    return static_cast<nsTableFrame*>(mFrames.FirstChild());
  }
  
private:
  nsFrameList   mCaptionFrames;
};

#endif

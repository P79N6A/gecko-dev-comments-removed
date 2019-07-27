



#ifndef nsTableColGroupFrame_h__
#define nsTableColGroupFrame_h__

#include "mozilla/Attributes.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsTableFrame.h"
#include "mozilla/WritingModes.h"

class nsTableColFrame;







class nsTableColGroupFrame final : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  

  




  friend nsTableColGroupFrame* NS_NewTableColGroupFrame(nsIPresShell* aPresShell,
                                                        nsStyleContext* aContext);

  nsTableFrame* GetTableFrame() const
  {
    nsIFrame* parent = GetParent();
    MOZ_ASSERT(parent && parent->GetType() == nsGkAtoms::tableFrame);
    return static_cast<nsTableFrame*>(parent);
  }

  


  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override {}

  







  nsTableColGroupType GetColType() const;

  


  void SetColType(nsTableColGroupType aType);
  
  







  static nsTableColGroupFrame* GetLastRealColGroup(nsTableFrame* aTableFrame);

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) override;

  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) override;
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) override;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) override;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) override;

  






  void RemoveChild(nsTableColFrame& aChild,
                   bool             aResetSubsequentColIndices);

  





  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  




  virtual nsIAtom* GetType() const override;

  virtual mozilla::WritingMode GetWritingMode() const override
    { return GetTableFrame()->GetWritingMode(); }

  













  nsresult AddColsToTable(int32_t                   aFirstColIndex,
                          bool                      aResetSubsequentColIndices,
                          const nsFrameList::Slice& aCols);

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
  void Dump(int32_t aIndent);
#endif

  



  virtual int32_t GetColCount() const;

  
  nsTableColFrame * GetFirstColumn();
  


  nsTableColFrame * GetNextColumn(nsIFrame *aChildFrame);

  


  int32_t GetStartColumnIndex();
  
  


  void SetStartColumnIndex(int32_t aIndex);

  
  int32_t GetSpan();

  

  nsFrameList& GetWritableChildList();

  







  static void ResetColIndices(nsIFrame*       aFirstColGroup,
                              int32_t         aFirstColIndex,
                              nsIFrame*       aStartColFrame = nullptr);

  





  void GetContinuousBCBorderWidth(nsMargin& aBorder);
  



  void SetContinuousBCBorderWidth(uint8_t     aForSide,
                                  BCPixelSize aPixelValue);

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsContainerFrame::IsFrameOfType(aFlags & ~(nsIFrame::eTablePart));
  }
  
  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) override;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) override;
  virtual void InvalidateFrameForRemoval() override { InvalidateFrameSubtree(); }

protected:
  explicit nsTableColGroupFrame(nsStyleContext* aContext);

  void InsertColsReflow(int32_t                   aColIndex,
                        const nsFrameList::Slice& aCols);

  virtual LogicalSides GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const override;

  
  int32_t mColCount;
  
  int32_t mStartColIndex;

  
  BCPixelSize mTopContBorderWidth;
  BCPixelSize mBottomContBorderWidth;
};

inline nsTableColGroupFrame::nsTableColGroupFrame(nsStyleContext *aContext)
: nsContainerFrame(aContext), mColCount(0), mStartColIndex(0)
{ 
  SetColType(eColGroupContent);
}
  
inline int32_t nsTableColGroupFrame::GetStartColumnIndex()
{  
  return mStartColIndex;
}

inline void nsTableColGroupFrame::SetStartColumnIndex (int32_t aIndex)
{
  mStartColIndex = aIndex;
}

inline int32_t nsTableColGroupFrame::GetColCount() const
{  
  return mColCount;
}

inline nsFrameList& nsTableColGroupFrame::GetWritableChildList()
{  
  return mFrames;
}

#endif


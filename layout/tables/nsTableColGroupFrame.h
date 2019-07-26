



#ifndef nsTableColGroupFrame_h__
#define nsTableColGroupFrame_h__

#include "mozilla/Attributes.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsTableColFrame.h"

class nsTableFrame;
class nsTableColFrame;

enum nsTableColGroupType {
  eColGroupContent            = 0, 
  eColGroupAnonymousCol       = 1, 
  eColGroupAnonymousCell      = 2  
};







class nsTableColGroupFrame : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  

  




  friend nsIFrame* NS_NewTableColGroupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  


  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList) MOZ_OVERRIDE;

  


  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE {}

  







  nsTableColGroupType GetColType() const;

  


  void SetColType(nsTableColGroupType aType);
  
  







  static nsTableColGroupFrame* GetLastRealColGroup(nsTableFrame* aTableFrame);

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;

  

  NS_IMETHOD AppendFrames(ChildListID     aListID,
                          nsFrameList&    aFrameList) MOZ_OVERRIDE;
  NS_IMETHOD InsertFrames(ChildListID     aListID,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList) MOZ_OVERRIDE;
  NS_IMETHOD RemoveFrame(ChildListID     aListID,
                         nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  






  void RemoveChild(nsTableColFrame& aChild,
                   bool             aResetSubsequentColIndices);

  





  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  













  nsresult AddColsToTable(int32_t                   aFirstColIndex,
                          bool                      aResetSubsequentColIndices,
                          const nsFrameList::Slice& aCols);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
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
  
  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) MOZ_OVERRIDE;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) MOZ_OVERRIDE;
  virtual void InvalidateFrameForRemoval() MOZ_OVERRIDE { InvalidateFrameSubtree(); }

protected:
  nsTableColGroupFrame(nsStyleContext* aContext);

  void InsertColsReflow(int32_t                   aColIndex,
                        const nsFrameList::Slice& aCols);

  virtual int GetSkipSides() const MOZ_OVERRIDE;

  
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


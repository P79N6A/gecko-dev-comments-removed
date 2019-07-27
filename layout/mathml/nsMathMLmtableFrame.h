




#ifndef nsMathMLmtableFrame_h___
#define nsMathMLmtableFrame_h___

#include "mozilla/Attributes.h"
#include "nsMathMLContainerFrame.h"
#include "nsBlockFrame.h"
#include "nsTableOuterFrame.h"
#include "nsTableRowFrame.h"
#include "nsTableCellFrame.h"





class nsMathMLmtableOuterFrame : public nsTableOuterFrame,
                                 public nsMathMLFrame
{
public:
  friend nsContainerFrame* NS_NewMathMLmtableOuterFrame(nsIPresShell* aPresShell,
                                                        nsStyleContext* aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  

  virtual void
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus) override;

  virtual nsresult
  AttributeChanged(int32_t  aNameSpaceID,
                   nsIAtom* aAttribute,
                   int32_t  aModType) override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsTableOuterFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

protected:
  explicit nsMathMLmtableOuterFrame(nsStyleContext* aContext) : nsTableOuterFrame(aContext) {}
  virtual ~nsMathMLmtableOuterFrame();

  
  
  
  nsIFrame*
  GetRowFrameAt(nsPresContext* aPresContext,
                int32_t         aRowIndex);
}; 



class nsMathMLmtableFrame : public nsTableFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsMathMLmtableFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  friend nsContainerFrame* NS_NewMathMLmtableFrame(nsIPresShell* aPresShell,
                                                   nsStyleContext* aContext);

  

  virtual void
  SetInitialChildList(ChildListID  aListID,
                      nsFrameList& aChildList) override;

  virtual void
  AppendFrames(ChildListID  aListID,
               nsFrameList& aFrameList) override
  {
    nsTableFrame::AppendFrames(aListID, aFrameList);
    RestyleTable();
  }

  virtual void
  InsertFrames(ChildListID aListID,
               nsIFrame* aPrevFrame,
               nsFrameList& aFrameList) override
  {
    nsTableFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
    RestyleTable();
  }

  virtual void
  RemoveFrame(ChildListID aListID,
              nsIFrame* aOldFrame) override
  {
    nsTableFrame::RemoveFrame(aListID, aOldFrame);
    RestyleTable();
  }

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsTableFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

  
  
  
  void RestyleTable();

  
  nscoord GetColSpacing(int32_t aColIndex) override;

  


  nscoord GetColSpacing(int32_t aStartColIndex,
                        int32_t aEndColIndex) override;

  
  nscoord GetRowSpacing(int32_t aRowIndex) override;

  


  nscoord GetRowSpacing(int32_t aStartRowIndex,
                        int32_t aEndRowIndex) override;

  void SetColSpacingArray(const nsTArray<nscoord>& aColSpacing)
  {
    mColSpacing = aColSpacing;
  }

  void SetRowSpacingArray(const nsTArray<nscoord>& aRowSpacing)
  {
    mRowSpacing = aRowSpacing;
  }

  void SetFrameSpacing(nscoord aSpacingX, nscoord aSpacingY)
  {
    mFrameSpacingX = aSpacingX;
    mFrameSpacingY = aSpacingY;
  }

  




  void SetUseCSSSpacing();

  bool GetUseCSSSpacing()
  {
    return mUseCSSSpacing;
  }

protected:
  explicit nsMathMLmtableFrame(nsStyleContext* aContext) : nsTableFrame(aContext) {}
  virtual ~nsMathMLmtableFrame();

private:
  nsTArray<nscoord> mColSpacing;
  nsTArray<nscoord> mRowSpacing;
  nscoord mFrameSpacingX;
  nscoord mFrameSpacingY;
  bool mUseCSSSpacing;
}; 



class nsMathMLmtrFrame : public nsTableRowFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsContainerFrame* NS_NewMathMLmtrFrame(nsIPresShell* aPresShell,
                                                nsStyleContext* aContext);

  

  virtual nsresult
  AttributeChanged(int32_t  aNameSpaceID,
                   nsIAtom* aAttribute,
                   int32_t  aModType) override;

  virtual void
  AppendFrames(ChildListID  aListID,
               nsFrameList& aFrameList) override
  {
    nsTableRowFrame::AppendFrames(aListID, aFrameList);
    RestyleTable();
  }

  virtual void
  InsertFrames(ChildListID aListID,
               nsIFrame* aPrevFrame,
               nsFrameList& aFrameList) override
  {
    nsTableRowFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
    RestyleTable();
  }

  virtual void
  RemoveFrame(ChildListID aListID,
              nsIFrame* aOldFrame) override
  {
    nsTableRowFrame::RemoveFrame(aListID, aOldFrame);
    RestyleTable();
  }

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsTableRowFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

  
  void RestyleTable()
  {
    nsTableFrame* tableFrame = GetTableFrame();
    if (tableFrame && tableFrame->IsFrameOfType(nsIFrame::eMathML)) {
      
      ((nsMathMLmtableFrame*)tableFrame)->RestyleTable();
    }
  }

protected:
  explicit nsMathMLmtrFrame(nsStyleContext* aContext) : nsTableRowFrame(aContext) {}
  virtual ~nsMathMLmtrFrame();
}; 



class nsMathMLmtdFrame : public nsTableCellFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsContainerFrame* NS_NewMathMLmtdFrame(nsIPresShell* aPresShell,
                                                nsStyleContext* aContext,
                                                nsTableFrame* aTableFrame);

  

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual nsresult
  AttributeChanged(int32_t  aNameSpaceID,
                   nsIAtom* aAttribute,
                   int32_t  aModType) override;

  virtual uint8_t GetVerticalAlign() const override;
  virtual nsresult ProcessBorders(nsTableFrame* aFrame,
                                  nsDisplayListBuilder* aBuilder,
                                  const nsDisplayListSet& aLists) override;

  virtual int32_t GetRowSpan() override;
  virtual int32_t GetColSpan() override;
  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsTableCellFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

  virtual nsMargin* GetBorderWidth(nsMargin& aBorder) const override;

  virtual nsMargin GetBorderOverflow() override;

protected:
  nsMathMLmtdFrame(nsStyleContext* aContext, nsTableFrame* aTableFrame)
    : nsTableCellFrame(aContext, aTableFrame) {}
  virtual ~nsMathMLmtdFrame();
}; 



class nsMathMLmtdInnerFrame : public nsBlockFrame,
                              public nsMathMLFrame {
public:
  friend nsContainerFrame* NS_NewMathMLmtdInnerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(int32_t         aFirstIndex,
                                    int32_t         aLastIndex,
                                    uint32_t        aFlagsValues,
                                    uint32_t        aFlagsToUpdate) override
  {
    nsMathMLContainerFrame::PropagatePresentationDataFromChildAt(this,
      aFirstIndex, aLastIndex, aFlagsValues, aFlagsToUpdate);
    return NS_OK;
  }

  virtual void
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus) override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsBlockFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eMathML | nsIFrame::eExcludesIgnorableWhitespace));
  }

  virtual const nsStyleText* StyleTextForLineLayout() override;
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) override;

  bool
  IsMrowLike() override {
    return mFrames.FirstChild() != mFrames.LastChild() ||
           !mFrames.FirstChild();
  }

protected:
  explicit nsMathMLmtdInnerFrame(nsStyleContext* aContext);
  virtual ~nsMathMLmtdInnerFrame();

  nsStyleText* mUniqueStyleText;

};  

#endif 

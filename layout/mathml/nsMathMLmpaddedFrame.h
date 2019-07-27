




#ifndef nsMathMLmpaddedFrame_h___
#define nsMathMLmpaddedFrame_h___

#include "mozilla/Attributes.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmpaddedFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmpaddedFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent) override;

  NS_IMETHOD
  TransmitAutomaticData() override {
    return TransmitAutomaticDataForMrowLikeElement();
  }

  virtual void
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus) override;
  
  virtual nsresult
  Place(nsRenderingContext& aRenderingContext,
        bool                 aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize) override;

  bool
  IsMrowLike() override {
    return mFrames.FirstChild() != mFrames.LastChild() ||
           !mFrames.FirstChild();
  }

protected:
  explicit nsMathMLmpaddedFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmpaddedFrame();
  
  virtual nsresult
  MeasureForWidth(nsRenderingContext& aRenderingContext,
                  nsHTMLReflowMetrics& aDesiredSize) override;

private:
  nsCSSValue mWidth;
  nsCSSValue mHeight;
  nsCSSValue mDepth;
  nsCSSValue mLeadingSpace;
  nsCSSValue mVerticalOffset;

  int32_t    mWidthSign;
  int32_t    mHeightSign;
  int32_t    mDepthSign;
  int32_t    mLeadingSpaceSign;
  int32_t    mVerticalOffsetSign;

  int32_t    mWidthPseudoUnit;
  int32_t    mHeightPseudoUnit;
  int32_t    mDepthPseudoUnit;
  int32_t    mLeadingSpacePseudoUnit;
  int32_t    mVerticalOffsetPseudoUnit;

  
  void
  ProcessAttributes();

  static bool
  ParseAttribute(nsString&   aString,
                 int32_t&    aSign,
                 nsCSSValue& aCSSValue,
                 int32_t&    aPseudoUnit);

  void
  UpdateValue(int32_t                  aSign,
              int32_t                  aPseudoUnit,
              const nsCSSValue&        aCSSValue,
              const nsHTMLReflowMetrics& aDesiredSize,
              nscoord&                 aValueToUpdate,
              float                    aFontSizeInflation) const;
};

#endif 

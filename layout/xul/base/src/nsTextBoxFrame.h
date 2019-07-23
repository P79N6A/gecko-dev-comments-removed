




































#ifndef nsTextBoxFrame_h___
#define nsTextBoxFrame_h___

#include "nsLeafBoxFrame.h"

class nsAccessKeyInfo;
class nsAsyncAccesskeyUpdate;

typedef nsLeafBoxFrame nsTextBoxFrameSuper;
class nsTextBoxFrame : public nsTextBoxFrameSuper
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState);
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);
  virtual void MarkIntrinsicWidthsDirty();

  enum CroppingStyle { CropNone, CropLeft, CropRight, CropCenter };

  friend nsIFrame* NS_NewTextBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD  Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        asPrevInFlow);

  virtual void Destroy();

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  void UpdateAttributes(nsIAtom*         aAttribute,
                        PRBool&          aResize,
                        PRBool&          aRedraw);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual ~nsTextBoxFrame();

  void PaintTitle(nsIRenderingContext& aRenderingContext,
                  const nsRect&        aDirtyRect,
                  nsPoint              aPt);

  virtual PRBool ComputesOwnOverflowArea();

protected:
  friend class nsAsyncAccesskeyUpdate;
  
  
  PRBool UpdateAccesskey(nsWeakFrame& aWeakThis);
  void UpdateAccessTitle();
  void UpdateAccessIndex();

  
  void LayoutTitle(nsPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aRect);

  void CalculateUnderline(nsIRenderingContext& aRenderingContext);

  void CalcTextSize(nsBoxLayoutState& aBoxLayoutState);

  nsRect CalcTextRect(nsIRenderingContext &aRenderingContext, const nsPoint &aTextOrigin);

  nsTextBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext);

  void CalculateTitleForWidth(nsPresContext*      aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              nscoord              aWidth);

  void GetTextSize(nsPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsString&      aString,
                   nsSize&              aSize,
                   nscoord&             aAscent);

  nsresult RegUnregAccessKey(PRBool aDoReg);

private:

  PRBool AlwaysAppendAccessKey();
  PRBool InsertSeparatorBeforeAccessKey();

  void DrawText(nsIRenderingContext& aRenderingContext,
                         const nsRect&        aTextRect,
                         const nscolor*       aOverrideColor);

  void PaintOneShadow(gfxContext *     aCtx,
                      const nsRect&    aTextRect,
                      nsCSSShadowItem* aShadowDetails,
                      const nscolor&   aForegroundColor,
                      const nsRect&    aDirtyRect);

  nsString mTitle;
  nsString mCroppedTitle;
  nsString mAccessKey;
  nsSize mTextSize;
  nsAccessKeyInfo* mAccessKeyInfo;

  CroppingStyle mCropType;
  nscoord mTitleWidth;
  nscoord mAscent;
  PRPackedBool mNeedsRecalc;
  PRPackedBool mNeedsReflowCallback;

  static PRBool gAlwaysAppendAccessKey;
  static PRBool gAccessKeyPrefInitialized;
  static PRBool gInsertSeparatorBeforeAccessKey;
  static PRBool gInsertSeparatorPrefInitialized;

}; 

#endif 

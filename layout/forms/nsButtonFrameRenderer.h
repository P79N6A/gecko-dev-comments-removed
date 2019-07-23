









































#ifndef nsButtonFrameRenderer_h___
#define nsButtonFrameRenderer_h___

#include "nsCoord.h"
#include "nsAutoPtr.h"
#include "nsFrame.h"

class nsStyleChangeList;


#define NS_BUTTON_RENDERER_FOCUS_INNER_CONTEXT_INDEX  0
#define NS_BUTTON_RENDERER_FOCUS_OUTER_CONTEXT_INDEX  1
#define NS_BUTTON_RENDERER_LAST_CONTEXT_INDEX   NS_BUTTON_RENDERER_FOCUS_OUTER_CONTEXT_INDEX

class nsButtonFrameRenderer {
public:

  nsButtonFrameRenderer();
  ~nsButtonFrameRenderer();

  


  nsresult DisplayButton(nsDisplayListBuilder* aBuilder,
                         nsDisplayList* aBackground, nsDisplayList* aForeground);


  void PaintOutlineAndFocusBorders(nsPresContext* aPresContext,
                                   nsIRenderingContext& aRenderingContext,
                                   const nsRect& aDirtyRect,
                                   const nsRect& aRect);

  void PaintBorderAndBackground(nsPresContext* aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                const nsRect& aDirtyRect,
                                const nsRect& aRect,
                                PRUint32 aBGFlags);

  void SetFrame(nsFrame* aFrame, nsPresContext* aPresContext);
 
  void SetDisabled(PRBool aDisabled, PRBool notify);

  PRBool isActive();
  PRBool isDisabled();

  void GetButtonOuterFocusRect(const nsRect& aRect, nsRect& aResult);
  void GetButtonRect(const nsRect& aRect, nsRect& aResult);
  void GetButtonInnerFocusRect(const nsRect& aRect, nsRect& aResult);
  nsMargin GetButtonOuterFocusBorderAndPadding();
  nsMargin GetButtonBorderAndPadding();
  nsMargin GetButtonInnerFocusMargin();
  nsMargin GetButtonInnerFocusBorderAndPadding();
  nsMargin GetButtonOutlineBorderAndPadding();
  nsMargin GetFullButtonBorderAndPadding();
  nsMargin GetAddedButtonBorderAndPadding();

  nsStyleContext* GetStyleContext(PRInt32 aIndex) const;
  void SetStyleContext(PRInt32 aIndex, nsStyleContext* aStyleContext);
  void ReResolveStyles(nsPresContext* aPresContext);

  nsIFrame* GetFrame();

protected:

private:

  
  nsRefPtr<nsStyleContext> mBorderStyle;
  nsRefPtr<nsStyleContext> mInnerFocusStyle;
  nsRefPtr<nsStyleContext> mOuterFocusStyle;

  nsFrame* mFrame;
};


#endif


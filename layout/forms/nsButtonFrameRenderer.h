









































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
   virtual ~nsButtonFrameRenderer();

   


   nsresult DisplayButton(nsDisplayListBuilder* aBuilder,
                          nsDisplayList* aBackground, nsDisplayList* aForeground);


        void PaintOutlineAndFocusBorders(nsPresContext* aPresContext,
						  nsIRenderingContext& aRenderingContext,
						  const nsRect& aDirtyRect,
						  const nsRect& aRect);

        void PaintBorderAndBackground(nsPresContext* aPresContext,
						  nsIRenderingContext& aRenderingContext,
						  const nsRect& aDirtyRect,
						  const nsRect& aRect);

	virtual void SetFrame(nsFrame* aFrame, nsPresContext* aPresContext);
 
	virtual void SetDisabled(PRBool aDisabled, PRBool notify);

	PRBool isActive();
	PRBool isDisabled();

	virtual void GetButtonOutlineRect(const nsRect& aRect, nsRect& aResult);
	virtual void GetButtonOuterFocusRect(const nsRect& aRect, nsRect& aResult);
	virtual void GetButtonRect(const nsRect& aRect, nsRect& aResult);
	virtual void GetButtonInnerFocusRect(const nsRect& aRect, nsRect& aResult);
	virtual void GetButtonContentRect(const nsRect& aRect, nsRect& aResult);
  virtual nsMargin GetButtonOuterFocusBorderAndPadding();
  virtual nsMargin GetButtonBorderAndPadding();
  virtual nsMargin GetButtonInnerFocusMargin();
  virtual nsMargin GetButtonInnerFocusBorderAndPadding();
  virtual nsMargin GetButtonOutlineBorderAndPadding();
  virtual nsMargin GetFullButtonBorderAndPadding();
  virtual nsMargin GetAddedButtonBorderAndPadding();

  virtual nsStyleContext* GetStyleContext(PRInt32 aIndex) const;
  virtual void SetStyleContext(PRInt32 aIndex, nsStyleContext* aStyleContext);
	virtual void ReResolveStyles(nsPresContext* aPresContext);

  virtual nsIFrame* GetFrame();

protected:

private:

	
  nsRefPtr<nsStyleContext> mBorderStyle;
  nsRefPtr<nsStyleContext> mInnerFocusStyle;
  nsRefPtr<nsStyleContext> mOuterFocusStyle;

	nsFrame* mFrame;
};


#endif


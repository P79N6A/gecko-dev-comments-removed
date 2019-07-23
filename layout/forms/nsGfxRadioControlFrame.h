




































#ifndef nsGfxRadioControlFrame_h___
#define nsGfxRadioControlFrame_h___

#include "nsFormControlFrame.h"
#include "nsIRadioControlFrame.h"

#ifdef ACCESSIBILITY
class nsIAccessible;
#endif



#define NS_GFX_RADIO_CONTROL_FRAME_FACE_CONTEXT_INDEX   0 // for additional style contexts
#define NS_GFX_RADIO_CONTROL_FRAME_LAST_CONTEXT_INDEX   0

class nsGfxRadioControlFrame : public nsFormControlFrame,
                               public nsIRadioControlFrame

{
private:

public:
  nsGfxRadioControlFrame(nsStyleContext* aContext);
  ~nsGfxRadioControlFrame();

   
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  NS_IMETHOD SetRadioButtonFaceStyleContext(nsStyleContext *aRadioButtonFaceStyleContext);
#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif
  NS_IMETHOD OnChecked(nsPresContext* aPresContext, PRBool aChecked);

  virtual nsStyleContext* GetAdditionalStyleContext(PRInt32 aIndex) const;
  virtual void SetAdditionalStyleContext(PRInt32 aIndex,
                                         nsStyleContext* aStyleContext);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  void PaintRadioButtonFromStyle(nsIRenderingContext& aRenderingContext, nsPoint aPt,
                                 const nsRect& aDirtyRect);

protected:
  nsRefPtr<nsStyleContext> mRadioButtonFaceStyle;

private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }
};

#endif


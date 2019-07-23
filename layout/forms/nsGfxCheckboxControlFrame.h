



































#ifndef nsGfxCheckboxControlFrame_h___
#define nsGfxCheckboxControlFrame_h___

#include "nsFormControlFrame.h"
#include "nsICheckboxControlFrame.h"

#ifdef ACCESSIBILITY
class nsIAccessible;
#endif


#define NS_GFX_CHECKBOX_CONTROL_FRAME_FACE_CONTEXT_INDEX   0 // for additional style contexts
#define NS_GFX_CHECKBOX_CONTROL_FRAME_LAST_CONTEXT_INDEX   0

class nsGfxCheckboxControlFrame : public nsFormControlFrame,
                                  public nsICheckboxControlFrame
{
public:
  nsGfxCheckboxControlFrame(nsStyleContext* aContext);
  virtual ~nsGfxCheckboxControlFrame();
  
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("CheckboxControl"), aResult);
  }
#endif

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif


  
  NS_IMETHOD SetCheckboxFaceStyleContext(nsStyleContext *aCheckboxFaceStyleContext);
  NS_IMETHOD OnChecked(nsPresContext* aPresContext, PRBool aChecked);

  virtual nsStyleContext* GetAdditionalStyleContext(PRInt32 aIndex) const;
  virtual void SetAdditionalStyleContext(PRInt32 aIndex,
                                         nsStyleContext* aStyleContext);

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  void PaintCheckBox(nsIRenderingContext& aRenderingContext,
                     nsPoint aPt, const nsRect& aDirtyRect);

  void PaintCheckBoxFromStyle(nsIRenderingContext& aRenderingContext,
                              nsPoint aPt, const nsRect& aDirtyRect);

protected:

  PRBool GetCheckboxState();

  nsRefPtr<nsStyleContext> mCheckButtonFaceStyle;

private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }
 
};

#endif


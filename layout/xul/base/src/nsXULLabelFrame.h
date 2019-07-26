






#ifndef nsXULLabelFrame_h_
#define nsXULLabelFrame_h_

#include "mozilla/Attributes.h"
#include "nsBlockFrame.h"

#ifndef MOZ_XUL
#error "This file should not be included"
#endif

class nsXULLabelFrame : public nsBlockFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewXULLabelFrame(nsIPresShell* aPresShell,
                                       nsStyleContext *aContext);
  
  
  virtual void Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  NS_IMETHOD AttributeChanged(int32_t aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t aModType) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  nsXULLabelFrame(nsStyleContext *aContext) : nsBlockFrame(aContext) {}

  nsresult RegUnregAccessKey(bool aDoReg);
};

nsIFrame*
NS_NewXULLabelFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

#endif 

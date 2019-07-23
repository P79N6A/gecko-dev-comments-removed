






































#ifndef nsXULLabelFrame_h_
#define nsXULLabelFrame_h_

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
  
  
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual void Destroy();

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  




  virtual nsIAtom* GetType() const;
  
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

protected:
  nsXULLabelFrame(nsStyleContext *aContext) : nsBlockFrame(aContext) {}

  nsresult RegUnregAccessKey(PRBool aDoReg);
};

nsIFrame*
NS_NewXULLabelFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

#endif 

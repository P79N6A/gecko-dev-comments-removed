






































#ifndef nsAreaFrame_h___
#define nsAreaFrame_h___

#include "nsBlockFrame.h"
#include "nsAbsoluteContainingBlock.h"

struct nsStyleDisplay;
struct nsStylePosition;








class nsAreaFrame : public nsBlockFrame
{
public:
  friend nsIFrame* NS_NewAreaFrame(nsIPresShell* aPresShell, nsStyleContext *aContext, PRUint32 aFlags);
  
  

#ifdef MOZ_XUL
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual void Destroy();

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);
#endif

  




  virtual nsIAtom* GetType() const;
  
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

protected:
  nsAreaFrame(nsStyleContext *aContext) : nsBlockFrame(aContext) {}

#ifdef MOZ_XUL
  nsresult RegUnregAccessKey(PRBool aDoReg);
#endif
};

#endif 

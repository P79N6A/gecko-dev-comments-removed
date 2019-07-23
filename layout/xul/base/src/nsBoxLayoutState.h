











































#ifndef nsBoxLayoutState_h___
#define nsBoxLayoutState_h___

#include "nsIFrame.h"
#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"

class nsIRenderingContext;
class nsCalculatedBoxInfo;
struct nsHTMLReflowMetrics;
class nsString;
class nsHTMLReflowCommand;

class nsBoxLayoutState
{
public:
  nsBoxLayoutState(nsPresContext* aPresContext, nsIRenderingContext* aRenderingContext = nsnull) NS_HIDDEN;
  nsBoxLayoutState(const nsBoxLayoutState& aState) NS_HIDDEN;

  nsPresContext* PresContext() const { return mPresContext; }
  nsIPresShell* PresShell() const { return mPresContext->PresShell(); }

  PRUint32 LayoutFlags() const { return mLayoutFlags; }
  void SetLayoutFlags(PRUint32 aFlags) { mLayoutFlags = aFlags; }

  
  void SetPaintingDisabled(PRBool aDisable) { mPaintingDisabled = aDisable; }
  PRBool PaintingDisabled() const { return mPaintingDisabled; }

  
  
  
  
  nsIRenderingContext* GetRenderingContext() const { return mRenderingContext; }

  void PushStackMemory() { PresShell()->PushStackMemory(); }
  void PopStackMemory()  { PresShell()->PopStackMemory(); }
  void* AllocateStackMemory(size_t aSize)
  { return PresShell()->AllocateStackMemory(aSize); }

private:
  nsCOMPtr<nsPresContext> mPresContext;
  nsIRenderingContext *mRenderingContext;
  PRUint32 mLayoutFlags;
  PRBool mPaintingDisabled;
};

#endif


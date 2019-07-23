











































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

class NS_STACK_CLASS nsBoxLayoutState
{
public:
  nsBoxLayoutState(nsPresContext* aPresContext, nsIRenderingContext* aRenderingContext = nsnull,
                   PRUint16 aReflowDepth = 0) NS_HIDDEN;
  nsBoxLayoutState(const nsBoxLayoutState& aState) NS_HIDDEN;

  nsPresContext* PresContext() const { return mPresContext; }
  nsIPresShell* PresShell() const { return mPresContext->PresShell(); }

  PRUint32 LayoutFlags() const { return mLayoutFlags; }
  void SetLayoutFlags(PRUint32 aFlags) { mLayoutFlags = aFlags; }

  
  void SetPaintingDisabled(PRBool aDisable) { mPaintingDisabled = aDisable; }
  PRBool PaintingDisabled() const { return mPaintingDisabled; }

  
  
  
  
  nsIRenderingContext* GetRenderingContext() const { return mRenderingContext; }

  void PushStackMemory() { PresShell()->PushStackMemory(); ++mReflowDepth; }
  void PopStackMemory()  { PresShell()->PopStackMemory(); --mReflowDepth; }
  void* AllocateStackMemory(size_t aSize)
  { return PresShell()->AllocateStackMemory(aSize); }

  PRUint16 GetReflowDepth() { return mReflowDepth; }
  
private:
  nsRefPtr<nsPresContext> mPresContext;
  nsIRenderingContext *mRenderingContext;
  PRUint32 mLayoutFlags;
  PRUint16 mReflowDepth; 
  PRPackedBool mPaintingDisabled;
};

#endif


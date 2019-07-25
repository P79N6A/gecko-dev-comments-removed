











































#ifndef nsBoxLayoutState_h___
#define nsBoxLayoutState_h___

#include "nsIFrame.h"
#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"

class nsRenderingContext;
class nsCalculatedBoxInfo;
struct nsHTMLReflowMetrics;
class nsString;
class nsHTMLReflowCommand;

class NS_STACK_CLASS nsBoxLayoutState
{
public:
  nsBoxLayoutState(nsPresContext* aPresContext,
                   nsRenderingContext* aRenderingContext = nsnull,
                   
                   const nsHTMLReflowState* aOuterReflowState = nsnull,
                   PRUint16 aReflowDepth = 0) NS_HIDDEN;
  nsBoxLayoutState(const nsBoxLayoutState& aState) NS_HIDDEN;

  nsPresContext* PresContext() const { return mPresContext; }
  nsIPresShell* PresShell() const { return mPresContext->PresShell(); }

  PRUint32 LayoutFlags() const { return mLayoutFlags; }
  void SetLayoutFlags(PRUint32 aFlags) { mLayoutFlags = aFlags; }

  
  void SetPaintingDisabled(bool aDisable) { mPaintingDisabled = aDisable; }
  bool PaintingDisabled() const { return mPaintingDisabled; }

  
  
  
  
  nsRenderingContext* GetRenderingContext() const { return mRenderingContext; }

  void PushStackMemory() { PresShell()->PushStackMemory(); ++mReflowDepth; }
  void PopStackMemory()  { PresShell()->PopStackMemory(); --mReflowDepth; }
  void* AllocateStackMemory(size_t aSize)
  { return PresShell()->AllocateStackMemory(aSize); }

  
  
  const nsHTMLReflowState* OuterReflowState() { return mOuterReflowState; }

  PRUint16 GetReflowDepth() { return mReflowDepth; }
  
private:
  nsRefPtr<nsPresContext> mPresContext;
  nsRenderingContext *mRenderingContext;
  const nsHTMLReflowState *mOuterReflowState;
  PRUint32 mLayoutFlags;
  PRUint16 mReflowDepth; 
  bool mPaintingDisabled;
};

#endif


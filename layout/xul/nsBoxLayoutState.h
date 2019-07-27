











#ifndef nsBoxLayoutState_h___
#define nsBoxLayoutState_h___

#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"

class nsRenderingContext;
struct nsHTMLReflowState;

class MOZ_STACK_CLASS nsBoxLayoutState
{
public:
  explicit nsBoxLayoutState(nsPresContext* aPresContext,
                            nsRenderingContext* aRenderingContext = nullptr,
                            
                            const nsHTMLReflowState* aOuterReflowState = nullptr,
                            uint16_t aReflowDepth = 0);
  nsBoxLayoutState(const nsBoxLayoutState& aState);

  nsPresContext* PresContext() const { return mPresContext; }
  nsIPresShell* PresShell() const { return mPresContext->PresShell(); }

  uint32_t LayoutFlags() const { return mLayoutFlags; }
  void SetLayoutFlags(uint32_t aFlags) { mLayoutFlags = aFlags; }

  
  void SetPaintingDisabled(bool aDisable) { mPaintingDisabled = aDisable; }
  bool PaintingDisabled() const { return mPaintingDisabled; }

  
  
  
  
  nsRenderingContext* GetRenderingContext() const { return mRenderingContext; }

  struct AutoReflowDepth {
    explicit AutoReflowDepth(nsBoxLayoutState& aState)
      : mState(aState) { ++mState.mReflowDepth; }
    ~AutoReflowDepth() { --mState.mReflowDepth; }
    nsBoxLayoutState& mState;
  };

  
  
  const nsHTMLReflowState* OuterReflowState() { return mOuterReflowState; }

  uint16_t GetReflowDepth() { return mReflowDepth; }
  
private:
  nsRefPtr<nsPresContext> mPresContext;
  nsRenderingContext *mRenderingContext;
  const nsHTMLReflowState *mOuterReflowState;
  uint32_t mLayoutFlags;
  uint16_t mReflowDepth; 
  bool mPaintingDisabled;
};

#endif


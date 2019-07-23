











































#include "nsProgressMeterFrame.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsCOMPtr.h"
#include "nsBoxLayoutState.h"
#include "nsIReflowCallback.h"
#include "nsContentUtils.h"





nsIFrame*
NS_NewProgressMeterFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsProgressMeterFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsProgressMeterFrame)






nsProgressMeterFrame :: ~nsProgressMeterFrame ( )
{
}

class nsAsyncProgressMeterInit : public nsIReflowCallback
{
public:
  nsAsyncProgressMeterInit(nsIFrame* aFrame) : mWeakFrame(aFrame) {}

  virtual PRBool ReflowFinished()
  {
    PRBool shouldFlush = PR_FALSE;
    nsIFrame* frame = mWeakFrame.GetFrame();
    if (frame) {
      nsAutoScriptBlocker scriptBlocker;
      frame->AttributeChanged(kNameSpaceID_None, nsGkAtoms::value, 0);
      shouldFlush = PR_TRUE;
    }
    delete this;
    return shouldFlush;
  }

  virtual void ReflowCallbackCanceled()
  {
    delete this;
  }

  nsWeakFrame mWeakFrame;
};

NS_IMETHODIMP
nsProgressMeterFrame::DoLayout(nsBoxLayoutState& aState)
{
  if (mNeedsReflowCallback) {
    nsIReflowCallback* cb = new nsAsyncProgressMeterInit(this);
    if (cb) {
      PresContext()->PresShell()->PostReflowCallback(cb);
    }
    mNeedsReflowCallback = PR_FALSE;
  }
  return nsBoxFrame::DoLayout(aState);
}

NS_IMETHODIMP
nsProgressMeterFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                       nsIAtom* aAttribute,
                                       PRInt32 aModType)
{
  NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
      "Scripts not blocked in nsProgressMeterFrame::AttributeChanged!");
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);
  if (NS_OK != rv) {
    return rv;
  }

  
  if (nsGkAtoms::value == aAttribute || nsGkAtoms::max == aAttribute) {
    nsIFrame* barChild = GetFirstChild(nsnull);
    if (!barChild) return NS_OK;
    nsIFrame* remainderChild = barChild->GetNextSibling();
    if (!remainderChild) return NS_OK;
    nsCOMPtr<nsIContent> remainderContent = remainderChild->GetContent();
    if (!remainderContent) return NS_OK;

    nsAutoString value, maxValue;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, value);
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::max, maxValue);

    PRInt32 error;
    PRInt32 flex = value.ToInteger(&error);
    PRInt32 maxFlex = maxValue.ToInteger(&error);
    if (NS_FAILED(error) || maxValue.IsEmpty()) {
      maxFlex = 100;
    }
    if (maxFlex < 1) {
      maxFlex = 1;
    }
    if (flex < 0) {
      flex = 0;
    }
    if (flex > maxFlex) {
      flex = maxFlex;
    }

    PRInt32 remainder = maxFlex - flex;

    nsAutoString leftFlex, rightFlex;
    leftFlex.AppendInt(flex);
    rightFlex.AppendInt(remainder);

    nsContentUtils::AddScriptRunner(new nsSetAttrRunnable(
      barChild->GetContent(), nsGkAtoms::flex, leftFlex));
    nsContentUtils::AddScriptRunner(new nsSetAttrRunnable(
      remainderContent, nsGkAtoms::flex, rightFlex));
    nsContentUtils::AddScriptRunner(new nsReflowFrameRunnable(
      this, nsIPresShell::eTreeChange, NS_FRAME_IS_DIRTY));
  }
  return NS_OK;
}

#ifdef NS_DEBUG
NS_IMETHODIMP
nsProgressMeterFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ProgressMeter"), aResult);
}
#endif

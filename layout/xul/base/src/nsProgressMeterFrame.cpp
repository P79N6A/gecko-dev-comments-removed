











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
#include "mozilla/Attributes.h"





nsIFrame*
NS_NewProgressMeterFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsProgressMeterFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsProgressMeterFrame)






nsProgressMeterFrame :: ~nsProgressMeterFrame ( )
{
}

class nsAsyncProgressMeterInit MOZ_FINAL : public nsIReflowCallback
{
public:
  nsAsyncProgressMeterInit(nsIFrame* aFrame) : mWeakFrame(aFrame) {}

  virtual bool ReflowFinished()
  {
    bool shouldFlush = false;
    nsIFrame* frame = mWeakFrame.GetFrame();
    if (frame) {
      nsAutoScriptBlocker scriptBlocker;
      frame->AttributeChanged(kNameSpaceID_None, nsGkAtoms::mode, 0);
      shouldFlush = true;
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
    mNeedsReflowCallback = false;
  }
  return nsBoxFrame::DoLayout(aState);
}

NS_IMETHODIMP
nsProgressMeterFrame::AttributeChanged(int32_t aNameSpaceID,
                                       nsIAtom* aAttribute,
                                       int32_t aModType)
{
  NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
      "Scripts not blocked in nsProgressMeterFrame::AttributeChanged!");
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);
  if (NS_OK != rv) {
    return rv;
  }

  
  bool undetermined = mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::mode,
                                            nsGkAtoms::undetermined, eCaseMatters);
  if (nsGkAtoms::mode == aAttribute ||
      (!undetermined &&
       (nsGkAtoms::value == aAttribute || nsGkAtoms::max == aAttribute))) {
    nsIFrame* barChild = GetFirstPrincipalChild();
    if (!barChild) return NS_OK;
    nsIFrame* remainderChild = barChild->GetNextSibling();
    if (!remainderChild) return NS_OK;
    nsCOMPtr<nsIContent> remainderContent = remainderChild->GetContent();
    if (!remainderContent) return NS_OK;

    int32_t flex = 1, maxFlex = 1;
    if (!undetermined) {
      nsAutoString value, maxValue;
      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, value);
      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::max, maxValue);

      nsresult error;
      flex = value.ToInteger(&error);
      maxFlex = maxValue.ToInteger(&error);
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
    }

    nsContentUtils::AddScriptRunner(new nsSetAttrRunnable(
      barChild->GetContent(), nsGkAtoms::flex, flex));
    nsContentUtils::AddScriptRunner(new nsSetAttrRunnable(
      remainderContent, nsGkAtoms::flex, maxFlex - flex));
    nsContentUtils::AddScriptRunner(new nsReflowFrameRunnable(
      this, nsIPresShell::eTreeChange, NS_FRAME_IS_DIRTY));
  }
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsProgressMeterFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ProgressMeter"), aResult);
}
#endif

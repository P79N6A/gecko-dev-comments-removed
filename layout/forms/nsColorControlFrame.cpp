




#include "nsColorControlFrame.h"

#include "nsContentCreatorFunctions.h"
#include "nsContentList.h"
#include "nsContentUtils.h"
#include "nsFormControlFrame.h"
#include "nsGkAtoms.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMNode.h"
#include "nsIFormControl.h"
#include "nsStyleSet.h"

using mozilla::dom::Element;

nsColorControlFrame::nsColorControlFrame(nsStyleContext* aContext):
  nsColorControlFrameSuper(aContext)
{
}

nsIFrame*
NS_NewColorControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsColorControlFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsColorControlFrame)

NS_QUERYFRAME_HEAD(nsColorControlFrame)
  NS_QUERYFRAME_ENTRY(nsColorControlFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsColorControlFrameSuper)


void nsColorControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), false);
  nsContentUtils::DestroyAnonymousContent(&mColorContent);
  nsColorControlFrameSuper::DestroyFrom(aDestructRoot);
}

nsIAtom*
nsColorControlFrame::GetType() const
{
  return nsGkAtoms::colorControlFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsColorControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ColorControl"), aResult);
}
#endif



nsresult
nsColorControlFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  nsCOMPtr<nsIDocument> doc = mContent->GetComposedDoc();
  mColorContent = doc->CreateHTMLElement(nsGkAtoms::div);

  
  mColorContent->SetIsNativeAnonymousRoot();

  nsresult rv = UpdateColor();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCSSPseudoElements::Type pseudoType = nsCSSPseudoElements::ePseudo_mozColorSwatch;
  nsRefPtr<nsStyleContext> newStyleContext = PresContext()->StyleSet()->
    ResolvePseudoElementStyle(mContent->AsElement(), pseudoType,
                              StyleContext(), mColorContent->AsElement());
  if (!aElements.AppendElement(ContentInfo(mColorContent, newStyleContext))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

void
nsColorControlFrame::AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                              uint32_t aFilter)
{
  if (mColorContent) {
    aElements.AppendElement(mColorContent);
  }
}

nsresult
nsColorControlFrame::UpdateColor()
{
  
  
  nsAutoString color;
  nsCOMPtr<nsIDOMHTMLInputElement> elt = do_QueryInterface(mContent);
  elt->GetValue(color);
  MOZ_ASSERT(!color.IsEmpty(),
             "Content node's GetValue() should return a valid color string "
             "(the default color, in case no valid color is set)");

  
  return mColorContent->SetAttr(kNameSpaceID_None, nsGkAtoms::style,
      NS_LITERAL_STRING("background-color:") + color, true);
}

nsresult
nsColorControlFrame::AttributeChanged(int32_t  aNameSpaceID,
                                      nsIAtom* aAttribute,
                                      int32_t  aModType)
{
  NS_ASSERTION(mColorContent, "The color div must exist");

  
  
  
  nsCOMPtr<nsIFormControl> fctrl = do_QueryInterface(GetContent());
  if (fctrl->GetType() == NS_FORM_INPUT_COLOR &&
      aNameSpaceID == kNameSpaceID_None && nsGkAtoms::value == aAttribute) {
    UpdateColor();
  }
  return nsColorControlFrameSuper::AttributeChanged(aNameSpaceID, aAttribute,
                                                    aModType);
}

nsContainerFrame*
nsColorControlFrame::GetContentInsertionFrame()
{
  return this;
}

Element*
nsColorControlFrame::GetPseudoElement(nsCSSPseudoElements::Type aType)
{
  if (aType == nsCSSPseudoElements::ePseudo_mozColorSwatch) {
    return mColorContent;
  }

  return nsContainerFrame::GetPseudoElement(aType);
}






#include "nsGfxButtonControlFrame.h"
#include "nsIFormControl.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsContentUtils.h"

#include "nsContentList.h"

#include "nsIDOMHTMLInputElement.h"
#include "nsTextNode.h"

using namespace mozilla;

nsGfxButtonControlFrame::nsGfxButtonControlFrame(nsStyleContext* aContext):
  nsHTMLButtonControlFrame(aContext)
{
}

nsContainerFrame*
NS_NewGfxButtonControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsGfxButtonControlFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsGfxButtonControlFrame)

void nsGfxButtonControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsContentUtils::DestroyAnonymousContent(&mTextContent);
  nsHTMLButtonControlFrame::DestroyFrom(aDestructRoot);
}

nsIAtom*
nsGfxButtonControlFrame::GetType() const
{
  return nsGkAtoms::gfxButtonControlFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsGfxButtonControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ButtonControl"), aResult);
}
#endif



nsresult
nsGfxButtonControlFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  nsXPIDLString label;
  GetLabel(label);

  
  mTextContent = new nsTextNode(mContent->NodeInfo()->NodeInfoManager());

  
  mTextContent->SetText(label, false);
  aElements.AppendElement(mTextContent);

  return NS_OK;
}

void
nsGfxButtonControlFrame::AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                                  uint32_t aFilter)
{
  if (mTextContent) {
    aElements.AppendElement(mTextContent);
  }
}



nsIFrame*
nsGfxButtonControlFrame::CreateFrameFor(nsIContent*      aContent)
{
  nsIFrame * newFrame = nullptr;

  if (aContent == mTextContent) {
    nsContainerFrame* parentFrame = do_QueryFrame(mFrames.FirstChild());

    nsPresContext* presContext = PresContext();
    nsRefPtr<nsStyleContext> textStyleContext;
    textStyleContext = presContext->StyleSet()->
      ResolveStyleForNonElement(mStyleContext);

    newFrame = NS_NewTextFrame(presContext->PresShell(), textStyleContext);
    
    newFrame->Init(mTextContent, parentFrame, nullptr);
    mTextContent->SetPrimaryFrame(newFrame);
  }

  return newFrame;
}

NS_QUERYFRAME_HEAD(nsGfxButtonControlFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLButtonControlFrame)







nsresult
nsGfxButtonControlFrame::GetDefaultLabel(nsXPIDLString& aString) const
{
  nsCOMPtr<nsIFormControl> form = do_QueryInterface(mContent);
  NS_ENSURE_TRUE(form, NS_ERROR_UNEXPECTED);

  int32_t type = form->GetType();
  const char *prop;
  if (type == NS_FORM_INPUT_RESET) {
    prop = "Reset";
  }
  else if (type == NS_FORM_INPUT_SUBMIT) {
    prop = "Submit";
  }
  else {
    aString.Truncate();
    return NS_OK;
  }

  return nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                            prop, aString);
}

nsresult
nsGfxButtonControlFrame::GetLabel(nsXPIDLString& aLabel)
{
  
  
  nsresult rv;
  nsCOMPtr<nsIDOMHTMLInputElement> elt = do_QueryInterface(mContent);
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::value) && elt) {
    rv = elt->GetValue(aLabel);
  } else {
    
    
    
    
    rv = GetDefaultLabel(aLabel);
  }

  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!StyleText()->WhiteSpaceIsSignificant()) {
    aLabel.CompressWhitespace();
  } else if (aLabel.Length() > 2 && aLabel.First() == ' ' &&
             aLabel.CharAt(aLabel.Length() - 1) == ' ') {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    aLabel.Cut(0, 1);
    aLabel.Truncate(aLabel.Length() - 1);
  }

  return NS_OK;
}

nsresult
nsGfxButtonControlFrame::AttributeChanged(int32_t         aNameSpaceID,
                                          nsIAtom*        aAttribute,
                                          int32_t         aModType)
{
  nsresult rv = NS_OK;

  
  if (nsGkAtoms::value == aAttribute) {
    if (mTextContent && mContent) {
      nsXPIDLString label;
      rv = GetLabel(label);
      NS_ENSURE_SUCCESS(rv, rv);
    
      mTextContent->SetText(label, true);
    } else {
      rv = NS_ERROR_UNEXPECTED;
    }

  
  } else {
    rv = nsHTMLButtonControlFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
  }
  return rv;
}

bool
nsGfxButtonControlFrame::IsLeaf() const
{
  return true;
}

nsContainerFrame*
nsGfxButtonControlFrame::GetContentInsertionFrame()
{
  return this;
}

nsresult
nsGfxButtonControlFrame::HandleEvent(nsPresContext* aPresContext, 
                                     WidgetGUIEvent* aEvent,
                                     nsEventStatus* aEventStatus)
{
  
  
  

  
  const nsStyleUserInterface* uiStyle = StyleUserInterface();
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE || uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  
  return NS_OK;
}

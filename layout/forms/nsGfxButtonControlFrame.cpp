




#include "nsGfxButtonControlFrame.h"
#include "nsWidgetsCID.h"
#include "nsFormControlFrame.h"
#include "nsIFormControl.h"
#include "nsINameSpaceManager.h"
#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsContentUtils.h"

#include "nsGUIEvent.h"
#include "nsContentList.h"
#include "nsContentCreatorFunctions.h"

#include "nsNodeInfoManager.h"
#include "nsIDOMHTMLInputElement.h"

const nscoord kSuggestedNotSet = -1;

nsGfxButtonControlFrame::nsGfxButtonControlFrame(nsStyleContext* aContext):
  nsHTMLButtonControlFrame(aContext)
{
}

nsIFrame*
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





bool
nsGfxButtonControlFrame::IsFileBrowseButton(int32_t type) const
{
  bool rv = false;
  if (NS_FORM_INPUT_BUTTON == type) {
    
    nsCOMPtr<nsIFormControl> formCtrl =
      do_QueryInterface(mContent->GetParent());

    rv = formCtrl && formCtrl->GetType() == NS_FORM_INPUT_FILE;
  }
  return rv;
}

#ifdef DEBUG
NS_IMETHODIMP
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

  
  NS_NewTextNode(getter_AddRefs(mTextContent),
                 mContent->NodeInfo()->NodeInfoManager());
  if (!mTextContent)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mTextContent->SetText(label, false);
  if (!aElements.AppendElement(mTextContent))
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

void
nsGfxButtonControlFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                                  uint32_t aFilter)
{
  aElements.MaybeAppendElement(mTextContent);
}



nsIFrame*
nsGfxButtonControlFrame::CreateFrameFor(nsIContent*      aContent)
{
  nsIFrame * newFrame = nullptr;

  if (aContent == mTextContent) {
    nsIFrame * parentFrame = mFrames.FirstChild();

    nsPresContext* presContext = PresContext();
    nsRefPtr<nsStyleContext> textStyleContext;
    textStyleContext = presContext->StyleSet()->
      ResolveStyleForNonElement(mStyleContext);

    if (textStyleContext) {
      newFrame = NS_NewTextFrame(presContext->PresShell(), textStyleContext);
      if (newFrame) {
        
        newFrame->Init(mTextContent, parentFrame, nullptr);
        mTextContent->SetPrimaryFrame(newFrame);
      }
    }
  }

  return newFrame;
}

nsresult
nsGfxButtonControlFrame::GetFormProperty(nsIAtom* aName, nsAString& aValue) const
{
  nsresult rv = NS_OK;
  if (nsGkAtoms::defaultLabel == aName) {
    
    
    nsXPIDLString temp;
    rv = GetDefaultLabel(temp);
    aValue = temp;
  } else {
    aValue.Truncate();
  }
  return rv;
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
  else if (IsFileBrowseButton(type)) {
    prop = "Browse";
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

  
  if (!GetStyleText()->WhiteSpaceIsSignificant()) {
    aLabel.CompressWhitespace();
  } else if (aLabel.Length() > 2 && aLabel.First() == ' ' &&
             aLabel.CharAt(aLabel.Length() - 1) == ' ') {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    aLabel.Cut(0, 1);
    aLabel.Truncate(aLabel.Length() - 1);
  }

  return NS_OK;
}

NS_IMETHODIMP
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

nsIFrame*
nsGfxButtonControlFrame::GetContentInsertionFrame()
{
  return this;
}

NS_IMETHODIMP
nsGfxButtonControlFrame::HandleEvent(nsPresContext* aPresContext, 
                                      nsGUIEvent*     aEvent,
                                      nsEventStatus*  aEventStatus)
{
  
  
  

  
  const nsStyleUserInterface* uiStyle = GetStyleUserInterface();
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE || uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  
  return NS_OK;
}

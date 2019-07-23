




































#include "nsGfxButtonControlFrame.h"
#include "nsWidgetsCID.h"
#include "nsIFontMetrics.h"
#include "nsFormControlFrame.h"
#include "nsIFormControl.h"
#include "nsINameSpaceManager.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsContentUtils.h"

#include "nsGUIEvent.h"
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

void nsGfxButtonControlFrame::Destroy()
{
  nsContentUtils::DestroyAnonymousContent(&mTextContent);
  nsHTMLButtonControlFrame::Destroy();
}

nsIAtom*
nsGfxButtonControlFrame::GetType() const
{
  return nsGkAtoms::gfxButtonControlFrame;
}





PRBool
nsGfxButtonControlFrame::IsFileBrowseButton(PRInt32 type)
{
  PRBool rv = PR_FALSE;
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
nsGfxButtonControlFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  nsXPIDLString label;
  GetLabel(label);

  
  NS_NewTextNode(getter_AddRefs(mTextContent),
                 mContent->NodeInfo()->NodeInfoManager());
  if (!mTextContent)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mTextContent->SetText(label, PR_FALSE);
  if (!aElements.AppendElement(mTextContent))
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}



nsIFrame*
nsGfxButtonControlFrame::CreateFrameFor(nsIContent*      aContent)
{
  nsIFrame * newFrame = nsnull;

  if (aContent == mTextContent) {
    nsIFrame * parentFrame = mFrames.FirstChild();

    nsPresContext* presContext = PresContext();
    nsRefPtr<nsStyleContext> textStyleContext;
    textStyleContext = presContext->StyleSet()->
      ResolveStyleForNonElement(mStyleContext);

    if (textStyleContext) {
      newFrame = NS_NewTextFrame(presContext->PresShell(), textStyleContext);
      if (newFrame) {
        
        newFrame->Init(mTextContent, parentFrame, nsnull);
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
    rv = const_cast<nsGfxButtonControlFrame*>(this)->GetDefaultLabel(temp);
    aValue = temp;
  } else {
    aValue.Truncate();
  }
  return rv;
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsGfxButtonControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLButtonAccessible(static_cast<nsIFrame*>(this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

NS_QUERYFRAME_HEAD(nsGfxButtonControlFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLButtonControlFrame)







nsresult
nsGfxButtonControlFrame::GetDefaultLabel(nsXPIDLString& aString)
{
  nsCOMPtr<nsIFormControl> form = do_QueryInterface(mContent);
  NS_ENSURE_TRUE(form, NS_ERROR_UNEXPECTED);

  PRInt32 type = form->GetType();
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
nsGfxButtonControlFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                          nsIAtom*        aAttribute,
                                          PRInt32         aModType)
{
  nsresult rv = NS_OK;

  
  if (nsGkAtoms::value == aAttribute) {
    if (mTextContent && mContent) {
      nsXPIDLString label;
      rv = GetLabel(label);
      NS_ENSURE_SUCCESS(rv, rv);
    
      mTextContent->SetText(label, PR_TRUE);
    } else {
      rv = NS_ERROR_UNEXPECTED;
    }

  
  } else {
    rv = nsHTMLButtonControlFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
  }
  return rv;
}

PRBool
nsGfxButtonControlFrame::IsLeaf() const
{
  return PR_TRUE;
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






#include "nsNumberControlFrame.h"

#include "HTMLInputElement.h"
#include "ICUUtils.h"
#include "nsIFocusManager.h"
#include "nsIPresShell.h"
#include "nsFocusManager.h"
#include "nsFontMetrics.h"
#include "nsFormControlFrame.h"
#include "nsGkAtoms.h"
#include "nsNameSpaceManager.h"
#include "nsThemeConstants.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/EventStates.h"
#include "nsContentUtils.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentList.h"
#include "nsStyleSet.h"
#include "nsIDOMMutationEvent.h"
#include "nsThreadUtils.h"

#ifdef ACCESSIBILITY
#include "mozilla/a11y/AccTypes.h"
#endif

using namespace mozilla;
using namespace mozilla::dom;

nsIFrame*
NS_NewNumberControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsNumberControlFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsNumberControlFrame)

NS_QUERYFRAME_HEAD(nsNumberControlFrame)
  NS_QUERYFRAME_ENTRY(nsNumberControlFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsITextControlFrame)
  NS_QUERYFRAME_ENTRY(nsIFormControlFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

nsNumberControlFrame::nsNumberControlFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext)
  , mHandlingInputEvent(false)
{
}

void
nsNumberControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  NS_ASSERTION(!GetPrevContinuation() && !GetNextContinuation(),
               "nsNumberControlFrame should not have continuations; if it does we "
               "need to call RegUnregAccessKey only for the first");
  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), false);
  nsContentUtils::DestroyAnonymousContent(&mOuterWrapper);
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

nscoord
nsNumberControlFrame::GetMinISize(nsRenderingContext* aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  nsIFrame* kid = mFrames.FirstChild();
  if (kid) { 
    result = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                                  kid,
                                                  nsLayoutUtils::MIN_ISIZE);
  } else {
    result = 0;
  }

  return result;
}

nscoord
nsNumberControlFrame::GetPrefISize(nsRenderingContext* aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);

  nsIFrame* kid = mFrames.FirstChild();
  if (kid) { 
    result = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                                  kid,
                                                  nsLayoutUtils::PREF_ISIZE);
  } else {
    result = 0;
  }

  return result;
}

void
nsNumberControlFrame::Reflow(nsPresContext* aPresContext,
                             nsHTMLReflowMetrics& aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsNumberControlFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  NS_ASSERTION(mOuterWrapper, "Outer wrapper div must exist!");

  NS_ASSERTION(!GetPrevContinuation() && !GetNextContinuation(),
               "nsNumberControlFrame should not have continuations; if it does we "
               "need to call RegUnregAccessKey only for the first");

  NS_ASSERTION(!mFrames.FirstChild() ||
               !mFrames.FirstChild()->GetNextSibling(),
               "We expect at most one direct child frame");

  if (mState & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(this, true);
  }

  
  
  const nscoord contentBoxWidth = aReflowState.ComputedWidth();
  nscoord contentBoxHeight = aReflowState.ComputedHeight();

  nsIFrame* outerWrapperFrame = mOuterWrapper->GetPrimaryFrame();

  if (!outerWrapperFrame) { 
    if (contentBoxHeight == NS_INTRINSICSIZE) {
      contentBoxHeight = 0;
    }
  } else {
    NS_ASSERTION(outerWrapperFrame == mFrames.FirstChild(), "huh?");

    nsHTMLReflowMetrics wrappersDesiredSize(aReflowState);

    WritingMode wm = outerWrapperFrame->GetWritingMode();
    LogicalSize availSize = aReflowState.ComputedSize(wm);
    availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;

    nsHTMLReflowState wrapperReflowState(aPresContext, aReflowState,
                                         outerWrapperFrame, availSize);

    
    nscoord xoffset = aReflowState.ComputedPhysicalBorderPadding().left +
                        wrapperReflowState.ComputedPhysicalMargin().left;
    nscoord yoffset = aReflowState.ComputedPhysicalBorderPadding().top +
                        wrapperReflowState.ComputedPhysicalMargin().top;

    nsReflowStatus childStatus;
    ReflowChild(outerWrapperFrame, aPresContext, wrappersDesiredSize,
                wrapperReflowState, xoffset, yoffset, 0, childStatus);
    MOZ_ASSERT(NS_FRAME_IS_FULLY_COMPLETE(childStatus),
               "We gave our child unconstrained height, so it should be complete");

    nscoord wrappersMarginBoxHeight = wrappersDesiredSize.Height() +
      wrapperReflowState.ComputedPhysicalMargin().TopBottom();

    if (contentBoxHeight == NS_INTRINSICSIZE) {
      
      
      contentBoxHeight = wrappersMarginBoxHeight;

      
      
      
      
      
      contentBoxHeight =
        NS_CSS_MINMAX(contentBoxHeight,
                      aReflowState.ComputedMinHeight(),
                      aReflowState.ComputedMaxHeight());
    }

    
    nscoord extraSpace = contentBoxHeight - wrappersMarginBoxHeight;
    yoffset += std::max(0, extraSpace / 2);

    
    FinishReflowChild(outerWrapperFrame, aPresContext, wrappersDesiredSize,
                      &wrapperReflowState, xoffset, yoffset, 0);

    aDesiredSize.SetBlockStartAscent(
       wrappersDesiredSize.BlockStartAscent() +
       outerWrapperFrame->BStart(aReflowState.GetWritingMode(),
                                 contentBoxWidth));
  }

  aDesiredSize.Width() = contentBoxWidth +
                         aReflowState.ComputedPhysicalBorderPadding().LeftRight();
  aDesiredSize.Height() = contentBoxHeight +
                          aReflowState.ComputedPhysicalBorderPadding().TopBottom();

  aDesiredSize.SetOverflowAreasToDesiredBounds();

  if (outerWrapperFrame) {
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, outerWrapperFrame);
  }

  FinishAndStoreOverflow(&aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

void
nsNumberControlFrame::SyncDisabledState()
{
  EventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED)) {
    mTextField->SetAttr(kNameSpaceID_None, nsGkAtoms::disabled, EmptyString(),
                        true);
  } else {
    mTextField->UnsetAttr(kNameSpaceID_None, nsGkAtoms::disabled, true);
  }
}

nsresult
nsNumberControlFrame::AttributeChanged(int32_t  aNameSpaceID,
                                       nsIAtom* aAttribute,
                                       int32_t  aModType)
{
  
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::placeholder ||
        aAttribute == nsGkAtoms::readonly ||
        aAttribute == nsGkAtoms::tabindex) {
      if (aModType == nsIDOMMutationEvent::REMOVAL) {
        mTextField->UnsetAttr(aNameSpaceID, aAttribute, true);
      } else {
        MOZ_ASSERT(aModType == nsIDOMMutationEvent::ADDITION ||
                   aModType == nsIDOMMutationEvent::MODIFICATION);
        nsAutoString value;
        mContent->GetAttr(aNameSpaceID, aAttribute, value);
        mTextField->SetAttr(aNameSpaceID, aAttribute, value, true);
      }
    }
  }

  return nsContainerFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                            aModType);
}

void
nsNumberControlFrame::ContentStatesChanged(EventStates aStates)
{
  if (aStates.HasState(NS_EVENT_STATE_DISABLED)) {
    nsContentUtils::AddScriptRunner(new SyncDisabledStateEvent(this));
  }
}

nsITextControlFrame*
nsNumberControlFrame::GetTextFieldFrame()
{
  return do_QueryFrame(GetAnonTextControl()->GetPrimaryFrame());
}

class FocusTextField : public nsRunnable
{
public:
  FocusTextField(nsIContent* aNumber, nsIContent* aTextField)
    : mNumber(aNumber),
      mTextField(aTextField)
  {}

  NS_IMETHODIMP Run() MOZ_OVERRIDE
  {
    if (mNumber->AsElement()->State().HasState(NS_EVENT_STATE_FOCUS)) {
      HTMLInputElement::FromContent(mTextField)->Focus();
    }

    return NS_OK;
  }

private:
  nsCOMPtr<nsIContent> mNumber;
  nsCOMPtr<nsIContent> mTextField;
};

nsresult
nsNumberControlFrame::MakeAnonymousElement(Element** aResult,
                                           nsTArray<ContentInfo>& aElements,
                                           nsIAtom* aTagName,
                                           nsCSSPseudoElements::Type aPseudoType,
                                           nsStyleContext* aParentContext)
{
  
  nsCOMPtr<nsIDocument> doc = mContent->GetDocument();
  nsRefPtr<Element> resultElement = doc->CreateHTMLElement(aTagName);

  
  
  
  
  NS_ASSERTION(aPseudoType != nsCSSPseudoElements::ePseudo_NotPseudoElement,
               "Expecting anonymous children to all be pseudo-elements");
  
  nsRefPtr<nsStyleContext> newStyleContext =
    PresContext()->StyleSet()->ResolvePseudoElementStyle(mContent->AsElement(),
                                                         aPseudoType,
                                                         aParentContext,
                                                         resultElement);

  if (!aElements.AppendElement(ContentInfo(resultElement, newStyleContext))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (aPseudoType == nsCSSPseudoElements::ePseudo_mozNumberSpinDown ||
      aPseudoType == nsCSSPseudoElements::ePseudo_mozNumberSpinUp) {
    resultElement->SetAttr(kNameSpaceID_None, nsGkAtoms::role,
                           NS_LITERAL_STRING("button"), false);
  }

  resultElement.forget(aResult);
  return NS_OK;
}

nsresult
nsNumberControlFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  nsresult rv;

  
  
  
  
  
  
  
  
  
  
  
  


  
  rv = MakeAnonymousElement(getter_AddRefs(mOuterWrapper),
                            aElements,
                            nsGkAtoms::div,
                            nsCSSPseudoElements::ePseudo_mozNumberWrapper,
                            mStyleContext);
  NS_ENSURE_SUCCESS(rv, rv);

  ContentInfo& outerWrapperCI = aElements.LastElement();

  
  rv = MakeAnonymousElement(getter_AddRefs(mTextField),
                            outerWrapperCI.mChildren,
                            nsGkAtoms::input,
                            nsCSSPseudoElements::ePseudo_mozNumberText,
                            outerWrapperCI.mStyleContext);
  NS_ENSURE_SUCCESS(rv, rv);

  mTextField->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                      NS_LITERAL_STRING("text"), PR_FALSE);

  HTMLInputElement* content = HTMLInputElement::FromContent(mContent);
  HTMLInputElement* textField = HTMLInputElement::FromContent(mTextField);

  
  nsAutoString value;
  content->GetValue(value);
  SetValueOfAnonTextControl(value);

  
  nsAutoString readonly;
  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::readonly, readonly)) {
    mTextField->SetAttr(kNameSpaceID_None, nsGkAtoms::readonly, readonly, false);
  }

  
  int32_t tabIndex;
  content->GetTabIndex(&tabIndex);
  textField->SetTabIndex(tabIndex);

  
  nsAutoString placeholder;
  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::placeholder, placeholder)) {
    mTextField->SetAttr(kNameSpaceID_None, nsGkAtoms::placeholder, placeholder, false);
  }

  if (mContent->AsElement()->State().HasState(NS_EVENT_STATE_FOCUS)) {
    
    nsRefPtr<FocusTextField> focusJob = new FocusTextField(mContent, mTextField);
    nsContentUtils::AddScriptRunner(focusJob);
  }

  if (StyleDisplay()->mAppearance == NS_THEME_TEXTFIELD) {
    
    
    return rv;
  }

  
  rv = MakeAnonymousElement(getter_AddRefs(mSpinBox),
                            outerWrapperCI.mChildren,
                            nsGkAtoms::div,
                            nsCSSPseudoElements::ePseudo_mozNumberSpinBox,
                            outerWrapperCI.mStyleContext);
  NS_ENSURE_SUCCESS(rv, rv);

  ContentInfo& spinBoxCI = outerWrapperCI.mChildren.LastElement();

  
  rv = MakeAnonymousElement(getter_AddRefs(mSpinUp),
                            spinBoxCI.mChildren,
                            nsGkAtoms::div,
                            nsCSSPseudoElements::ePseudo_mozNumberSpinUp,
                            spinBoxCI.mStyleContext);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = MakeAnonymousElement(getter_AddRefs(mSpinDown),
                            spinBoxCI.mChildren,
                            nsGkAtoms::div,
                            nsCSSPseudoElements::ePseudo_mozNumberSpinDown,
                            spinBoxCI.mStyleContext);

  SyncDisabledState();

  return rv;
}

nsIAtom*
nsNumberControlFrame::GetType() const
{
  return nsGkAtoms::numberControlFrame;
}

NS_IMETHODIMP
nsNumberControlFrame::GetEditor(nsIEditor **aEditor)
{
  return GetTextFieldFrame()->GetEditor(aEditor);
}

NS_IMETHODIMP
nsNumberControlFrame::SetSelectionStart(int32_t aSelectionStart)
{
  return GetTextFieldFrame()->SetSelectionStart(aSelectionStart);
}

NS_IMETHODIMP
nsNumberControlFrame::SetSelectionEnd(int32_t aSelectionEnd)
{
  return GetTextFieldFrame()->SetSelectionEnd(aSelectionEnd);
}

NS_IMETHODIMP
nsNumberControlFrame::SetSelectionRange(int32_t aSelectionStart,
                                        int32_t aSelectionEnd,
                                        SelectionDirection aDirection)
{
  return GetTextFieldFrame()->SetSelectionRange(aSelectionStart, aSelectionEnd,
                                                aDirection);
}

NS_IMETHODIMP
nsNumberControlFrame::GetSelectionRange(int32_t* aSelectionStart,
                                        int32_t* aSelectionEnd,
                                        SelectionDirection* aDirection)
{
  return GetTextFieldFrame()->GetSelectionRange(aSelectionStart, aSelectionEnd,
                                                aDirection);
}

NS_IMETHODIMP
nsNumberControlFrame::GetOwnedSelectionController(nsISelectionController** aSelCon)
{
  return GetTextFieldFrame()->GetOwnedSelectionController(aSelCon);
}

nsFrameSelection*
nsNumberControlFrame::GetOwnedFrameSelection()
{
  return GetTextFieldFrame()->GetOwnedFrameSelection();
}

nsresult
nsNumberControlFrame::GetPhonetic(nsAString& aPhonetic)
{
  return GetTextFieldFrame()->GetPhonetic(aPhonetic);
}

nsresult
nsNumberControlFrame::EnsureEditorInitialized()
{
  return GetTextFieldFrame()->EnsureEditorInitialized();
}

nsresult
nsNumberControlFrame::ScrollSelectionIntoView()
{
  return GetTextFieldFrame()->ScrollSelectionIntoView();
}

void
nsNumberControlFrame::SetFocus(bool aOn, bool aRepaint)
{
  GetTextFieldFrame()->SetFocus(aOn, aRepaint);
}

nsresult
nsNumberControlFrame::SetFormProperty(nsIAtom* aName, const nsAString& aValue)
{
  return GetTextFieldFrame()->SetFormProperty(aName, aValue);
}

HTMLInputElement*
nsNumberControlFrame::GetAnonTextControl()
{
  return mTextField ? HTMLInputElement::FromContent(mTextField) : nullptr;
}

 nsNumberControlFrame*
nsNumberControlFrame::GetNumberControlFrameForTextField(nsIFrame* aFrame)
{
  
  
  
  
  
  nsIContent* content = aFrame->GetContent();
  if (content->IsInNativeAnonymousSubtree() &&
      content->GetParent() && content->GetParent()->GetParent()) {
    nsIContent* grandparent = content->GetParent()->GetParent();
    if (grandparent->IsHTML(nsGkAtoms::input) &&
        grandparent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                 nsGkAtoms::number, eCaseMatters)) {
      return do_QueryFrame(grandparent->GetPrimaryFrame());
    }
  }
  return nullptr;
}

 nsNumberControlFrame*
nsNumberControlFrame::GetNumberControlFrameForSpinButton(nsIFrame* aFrame)
{
  
  
  
  
  
  nsIContent* content = aFrame->GetContent();
  if (content->IsInNativeAnonymousSubtree() &&
      content->GetParent() && content->GetParent()->GetParent() &&
      content->GetParent()->GetParent()->GetParent()) {
    nsIContent* greatgrandparent = content->GetParent()->GetParent()->GetParent();
    if (greatgrandparent->IsHTML(nsGkAtoms::input) &&
        greatgrandparent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                      nsGkAtoms::number, eCaseMatters)) {
      return do_QueryFrame(greatgrandparent->GetPrimaryFrame());
    }
  }
  return nullptr;
}

int32_t
nsNumberControlFrame::GetSpinButtonForPointerEvent(WidgetGUIEvent* aEvent) const
{
  MOZ_ASSERT(aEvent->mClass == eMouseEventClass, "Unexpected event type");

  if (!mSpinBox) {
    
    return eSpinButtonNone;
  }
  if (aEvent->originalTarget == mSpinUp) {
    return eSpinButtonUp;
  }
  if (aEvent->originalTarget == mSpinDown) {
    return eSpinButtonDown;
  }
  if (aEvent->originalTarget == mSpinBox) {
    
    
    
    
    
    LayoutDeviceIntPoint absPoint = aEvent->refPoint;
    nsPoint point =
      nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                       LayoutDeviceIntPoint::ToUntyped(absPoint),
                       mSpinBox->GetPrimaryFrame());
    if (point != nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE)) {
      if (point.y < mSpinBox->GetPrimaryFrame()->GetSize().height / 2) {
        return eSpinButtonUp;
      }
      return eSpinButtonDown;
    }
  }
  return eSpinButtonNone;
}

void
nsNumberControlFrame::SpinnerStateChanged() const
{
  MOZ_ASSERT(mSpinUp && mSpinDown,
             "We should not be called when we have no spinner");

  nsIFrame* spinUpFrame = mSpinUp->GetPrimaryFrame();
  if (spinUpFrame && spinUpFrame->IsThemed()) {
    spinUpFrame->InvalidateFrame();
  }
  nsIFrame* spinDownFrame = mSpinDown->GetPrimaryFrame();
  if (spinDownFrame && spinDownFrame->IsThemed()) {
    spinDownFrame->InvalidateFrame();
  }
}

bool
nsNumberControlFrame::SpinnerUpButtonIsDepressed() const
{
  return HTMLInputElement::FromContent(mContent)->
           NumberSpinnerUpButtonIsDepressed();
}

bool
nsNumberControlFrame::SpinnerDownButtonIsDepressed() const
{
  return HTMLInputElement::FromContent(mContent)->
           NumberSpinnerDownButtonIsDepressed();
}

bool
nsNumberControlFrame::IsFocused() const
{
  
  
  
  return mTextField->AsElement()->State().HasState(NS_EVENT_STATE_FOCUS) ||
         mContent->AsElement()->State().HasState(NS_EVENT_STATE_FOCUS);
}

void
nsNumberControlFrame::HandleFocusEvent(WidgetEvent* aEvent)
{
  if (aEvent->originalTarget != mTextField) {
    
    HTMLInputElement::FromContent(mTextField)->Focus();
  }
}

nsresult
nsNumberControlFrame::HandleSelectCall()
{
  return HTMLInputElement::FromContent(mTextField)->Select();
}

#define STYLES_DISABLING_NATIVE_THEMING \
  NS_AUTHOR_SPECIFIED_BACKGROUND | \
  NS_AUTHOR_SPECIFIED_PADDING | \
  NS_AUTHOR_SPECIFIED_BORDER

bool
nsNumberControlFrame::ShouldUseNativeStyleForSpinner() const
{
  MOZ_ASSERT(mSpinUp && mSpinDown,
             "We should not be called when we have no spinner");

  nsIFrame* spinUpFrame = mSpinUp->GetPrimaryFrame();
  nsIFrame* spinDownFrame = mSpinDown->GetPrimaryFrame();

  return spinUpFrame &&
    spinUpFrame->StyleDisplay()->mAppearance == NS_THEME_SPINNER_UP_BUTTON &&
    !PresContext()->HasAuthorSpecifiedRules(spinUpFrame,
                                            STYLES_DISABLING_NATIVE_THEMING) &&
    spinDownFrame &&
    spinDownFrame->StyleDisplay()->mAppearance == NS_THEME_SPINNER_DOWN_BUTTON &&
    !PresContext()->HasAuthorSpecifiedRules(spinDownFrame,
                                            STYLES_DISABLING_NATIVE_THEMING);
}

void
nsNumberControlFrame::AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                               uint32_t aFilter)
{
  
  if (mOuterWrapper) {
    aElements.AppendElement(mOuterWrapper);
  }
}

void
nsNumberControlFrame::SetValueOfAnonTextControl(const nsAString& aValue)
{
  if (mHandlingInputEvent) {
    
    
    
    
    
    
    
    
    
    return;
  }

  
  
  
  nsAutoString localizedValue(aValue);

#ifdef ENABLE_INTL_API
  
  Decimal val = HTMLInputElement::StringToDecimal(aValue);
  if (val.isFinite()) {
    ICUUtils::LanguageTagIterForContent langTagIter(mContent);
    ICUUtils::LocalizeNumber(val.toDouble(), langTagIter, localizedValue);
  }
#endif

  
  
  
  
  HTMLInputElement::FromContent(mTextField)->SetValue(localizedValue);
}

void
nsNumberControlFrame::GetValueOfAnonTextControl(nsAString& aValue)
{
  if (!mTextField) {
    aValue.Truncate();
    return;
  }

  HTMLInputElement::FromContent(mTextField)->GetValue(aValue);

#ifdef ENABLE_INTL_API
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  ICUUtils::LanguageTagIterForContent langTagIter(mContent);
  double value = ICUUtils::ParseNumber(aValue, langTagIter);
  if (NS_finite(value) &&
      value != HTMLInputElement::StringToDecimal(aValue).toDouble()) {
    aValue.Truncate();
    aValue.AppendFloat(value);
  }
#endif
  
  
}

bool
nsNumberControlFrame::AnonTextControlIsEmpty()
{
  if (!mTextField) {
    return true;
  }
  nsAutoString value;
  HTMLInputElement::FromContent(mTextField)->GetValue(value);
  return value.IsEmpty();
}

Element*
nsNumberControlFrame::GetPseudoElement(nsCSSPseudoElements::Type aType)
{
  if (aType == nsCSSPseudoElements::ePseudo_mozNumberWrapper) {
    return mOuterWrapper;
  }

  if (aType == nsCSSPseudoElements::ePseudo_mozNumberText) {
    return mTextField;
  }

  if (aType == nsCSSPseudoElements::ePseudo_mozNumberSpinBox) {
    MOZ_ASSERT(mSpinBox);
    return mSpinBox;
  }

  if (aType == nsCSSPseudoElements::ePseudo_mozNumberSpinUp) {
    MOZ_ASSERT(mSpinUp);
    return mSpinUp;
  }

  if (aType == nsCSSPseudoElements::ePseudo_mozNumberSpinDown) {
    MOZ_ASSERT(mSpinDown);
    return mSpinDown;
  }

  return nsContainerFrame::GetPseudoElement(aType);
}

#ifdef ACCESSIBILITY
a11y::AccType
nsNumberControlFrame::AccessibleType()
{
  return a11y::eHTMLSpinnerType;
}
#endif

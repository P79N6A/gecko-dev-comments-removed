




#ifndef nsNumberControlFrame_h__
#define nsNumberControlFrame_h__

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsIFormControlFrame.h"
#include "nsITextControlFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsCOMPtr.h"

class nsPresContext;

namespace mozilla {
class WidgetEvent;
class WidgetGUIEvent;
namespace dom {
class HTMLInputElement;
}
}




class nsNumberControlFrame MOZ_FINAL : public nsContainerFrame
                                     , public nsIAnonymousContentCreator
                                     , public nsITextControlFrame
{
  friend nsIFrame*
  NS_NewNumberControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  typedef mozilla::dom::Element Element;
  typedef mozilla::dom::HTMLInputElement HTMLInputElement;
  typedef mozilla::WidgetEvent WidgetEvent;
  typedef mozilla::WidgetGUIEvent WidgetGUIEvent;

  explicit nsNumberControlFrame(nsStyleContext* aContext);

public:
  NS_DECL_QUERYFRAME_TARGET(nsNumberControlFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;
  virtual void ContentStatesChanged(mozilla::EventStates aStates) MOZ_OVERRIDE;
  virtual bool IsLeaf() const MOZ_OVERRIDE { return true; }

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  virtual nscoord GetMinISize(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;

  virtual nscoord GetPrefISize(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual nsresult AttributeChanged(int32_t  aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t  aModType) MOZ_OVERRIDE;

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE {
    return MakeFrameName(NS_LITERAL_STRING("NumberControl"), aResult);
  }
#endif

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  
  NS_IMETHOD    GetEditor(nsIEditor **aEditor) MOZ_OVERRIDE;

  NS_IMETHOD    SetSelectionStart(int32_t aSelectionStart) MOZ_OVERRIDE;
  NS_IMETHOD    SetSelectionEnd(int32_t aSelectionEnd) MOZ_OVERRIDE;

  NS_IMETHOD    SetSelectionRange(int32_t aSelectionStart,
                                  int32_t aSelectionEnd,
                                  SelectionDirection aDirection = eNone) MOZ_OVERRIDE;

  NS_IMETHOD    GetSelectionRange(int32_t* aSelectionStart,
                                  int32_t* aSelectionEnd,
                                  SelectionDirection* aDirection = nullptr) MOZ_OVERRIDE;

  NS_IMETHOD    GetOwnedSelectionController(nsISelectionController** aSelCon) MOZ_OVERRIDE;
  virtual nsFrameSelection* GetOwnedFrameSelection() MOZ_OVERRIDE;

  virtual nsresult GetPhonetic(nsAString& aPhonetic) MOZ_OVERRIDE;

  




  virtual nsresult EnsureEditorInitialized() MOZ_OVERRIDE;

  virtual nsresult ScrollSelectionIntoView() MOZ_OVERRIDE;

  
  virtual void SetFocus(bool aOn, bool aRepaint) MOZ_OVERRIDE;
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) MOZ_OVERRIDE;

  





  void SetValueOfAnonTextControl(const nsAString& aValue);

  






  void GetValueOfAnonTextControl(nsAString& aValue);

  bool AnonTextControlIsEmpty();

  



  void HandlingInputEvent(bool aHandlingEvent)
  {
    mHandlingInputEvent = aHandlingEvent;
  }

  HTMLInputElement* GetAnonTextControl();

  



  static nsNumberControlFrame* GetNumberControlFrameForTextField(nsIFrame* aFrame);

  



  static nsNumberControlFrame* GetNumberControlFrameForSpinButton(nsIFrame* aFrame);

  enum SpinButtonEnum {
    eSpinButtonNone,
    eSpinButtonUp,
    eSpinButtonDown
  };

  




  int32_t GetSpinButtonForPointerEvent(WidgetGUIEvent* aEvent) const;

  void SpinnerStateChanged() const;

  bool SpinnerUpButtonIsDepressed() const;
  bool SpinnerDownButtonIsDepressed() const;

  bool IsFocused() const;

  void HandleFocusEvent(WidgetEvent* aEvent);

  


  nsresult HandleSelectCall();

  virtual Element* GetPseudoElement(nsCSSPseudoElements::Type aType) MOZ_OVERRIDE;

  bool ShouldUseNativeStyleForSpinner() const;

private:

  nsITextControlFrame* GetTextFieldFrame();
  nsresult MakeAnonymousElement(Element** aResult,
                                nsTArray<ContentInfo>& aElements,
                                nsIAtom* aTagName,
                                nsCSSPseudoElements::Type aPseudoType,
                                nsStyleContext* aParentContext);

  class SyncDisabledStateEvent;
  friend class SyncDisabledStateEvent;
  class SyncDisabledStateEvent : public nsRunnable
  {
  public:
    explicit SyncDisabledStateEvent(nsNumberControlFrame* aFrame)
    : mFrame(aFrame)
    {}

    NS_IMETHOD Run() MOZ_OVERRIDE
    {
      nsNumberControlFrame* frame =
        static_cast<nsNumberControlFrame*>(mFrame.GetFrame());
      NS_ENSURE_STATE(frame);

      frame->SyncDisabledState();
      return NS_OK;
    }

  private:
    nsWeakFrame mFrame;
  };

  


  void SyncDisabledState();

  



  nsCOMPtr<Element> mOuterWrapper;
  nsCOMPtr<Element> mTextField;
  nsCOMPtr<Element> mSpinBox;
  nsCOMPtr<Element> mSpinUp;
  nsCOMPtr<Element> mSpinDown;
  bool mHandlingInputEvent;
};

#endif 

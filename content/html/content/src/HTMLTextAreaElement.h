





#ifndef mozilla_dom_HTMLTextAreaElement_h
#define mozilla_dom_HTMLTextAreaElement_h

#include "nsIDOMHTMLTextAreaElement.h"
#include "nsITextControlElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsCOMPtr.h"
#include "nsGenericHTMLElement.h"
#include "nsEventStates.h"
#include "nsStubMutationObserver.h"
#include "nsIConstraintValidation.h"
#include "nsHTMLFormElement.h"

#include "nsTextEditorState.h"

class nsFormSubmission;
class nsIControllers;
class nsIDocument;
class nsPresContext;
class nsPresState;

namespace mozilla {
namespace dom {

class HTMLTextAreaElement MOZ_FINAL : public nsGenericHTMLFormElement,
                                      public nsIDOMHTMLTextAreaElement,
                                      public nsITextControlElement,
                                      public nsIDOMNSEditableElement,
                                      public nsStubMutationObserver,
                                      public nsIConstraintValidation
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  HTMLTextAreaElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                      FromParser aFromParser = NOT_FROM_PARSER);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLTEXTAREAELEMENT

  
  NS_IMETHOD GetEditor(nsIEditor** aEditor)
  {
    return nsGenericHTMLElement::GetEditor(aEditor);
  }
  NS_IMETHOD SetUserInput(const nsAString& aInput);

  
  NS_IMETHOD_(uint32_t) GetType() const { return NS_FORM_TEXTAREA; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);
  NS_IMETHOD SaveState();
  virtual bool RestoreState(nsPresState* aState);
  virtual bool IsDisabledForEvents(uint32_t aMessage);

  virtual void FieldSetDisabledChanged(bool aNotify);

  virtual nsEventStates IntrinsicState() const;

  
  NS_IMETHOD SetValueChanged(bool aValueChanged);
  NS_IMETHOD_(bool) IsSingleLineTextControl() const;
  NS_IMETHOD_(bool) IsTextArea() const;
  NS_IMETHOD_(bool) IsPlainTextControl() const;
  NS_IMETHOD_(bool) IsPasswordTextControl() const;
  NS_IMETHOD_(int32_t) GetCols();
  NS_IMETHOD_(int32_t) GetWrapCols();
  NS_IMETHOD_(int32_t) GetRows();
  NS_IMETHOD_(bool) ValueChanged() const;
  NS_IMETHOD_(void) GetTextEditorValue(nsAString& aValue, bool aIgnoreWrap) const;
  NS_IMETHOD_(nsIEditor*) GetTextEditor();
  NS_IMETHOD_(nsISelectionController*) GetSelectionController();
  NS_IMETHOD_(nsFrameSelection*) GetConstFrameSelection();
  NS_IMETHOD BindToFrame(nsTextControlFrame* aFrame);
  NS_IMETHOD_(void) UnbindFromFrame(nsTextControlFrame* aFrame);
  NS_IMETHOD CreateEditor();
  NS_IMETHOD_(nsIContent*) GetRootEditorNode();
  NS_IMETHOD_(nsIContent*) CreatePlaceholderNode();
  NS_IMETHOD_(nsIContent*) GetPlaceholderNode();
  NS_IMETHOD_(void) UpdatePlaceholderVisibility(bool aNotify);
  NS_IMETHOD_(bool) GetPlaceholderVisibility();
  NS_IMETHOD_(void) InitializeKeyboardEventListeners();
  NS_IMETHOD_(void) OnValueChanged(bool aNotify);
  NS_IMETHOD_(bool) HasCachedSelection();

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex);

  virtual void DoneAddingChildren(bool aHaveNotified);
  virtual bool IsDoneAddingChildren();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsresult CopyInnerTo(Element* aDest);

  


  virtual nsresult BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify);

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLTextAreaElement,
                                           nsGenericHTMLFormElement)

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  bool     IsTooLong();
  bool     IsValueMissing() const;
  void     UpdateTooLongValidityState();
  void     UpdateValueMissingValidityState();
  void     UpdateBarredFromConstraintValidation();
  nsresult GetValidationMessage(nsAString& aValidationMessage,
                                ValidityStateType aType);

protected:
  using nsGenericHTMLFormElement::IsSingleLineTextControl; 

  nsCOMPtr<nsIControllers> mControllers;
  
  bool                     mValueChanged;
  
  bool                     mHandlingSelect;
  

  bool                     mDoneAddingChildren;
  
  bool                     mInhibitStateRestoration;
  
  bool                     mDisabledChanged;
  
  bool                     mCanShowInvalidUI;
  
  bool                     mCanShowValidUI;
  
  void FireChangeEventIfNeeded();
  
  nsString mFocusedValue;

  
  nsTextEditorState mState;

  NS_IMETHOD SelectAll(nsPresContext* aPresContext);
  






  void GetValueInternal(nsAString& aValue, bool aIgnoreWrap) const;

  nsresult SetValueInternal(const nsAString& aValue,
                            bool aUserInput);
  nsresult GetSelectionRange(int32_t* aSelectionStart, int32_t* aSelectionEnd);

  




  void ContentChanged(nsIContent* aContent);

  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom *aName,
                                const nsAttrValue* aValue, bool aNotify);

  





  bool ShouldShowValidityUI() const {
    






    if (mForm && mForm->HasEverTriedInvalidSubmit()) {
      return true;
    }

    return mValueChanged;
  }

  


  bool IsMutable() const;

  




  bool IsValueEmpty() const;
};

} 
} 

#endif


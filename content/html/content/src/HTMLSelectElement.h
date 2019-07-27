



#ifndef mozilla_dom_HTMLSelectElement_h
#define mozilla_dom_HTMLSelectElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIConstraintValidation.h"

#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/HTMLOptionsCollection.h"
#include "mozilla/ErrorResult.h"
#include "nsCheapSets.h"
#include "nsCOMPtr.h"
#include "nsError.h"
#include "mozilla/dom/HTMLFormElement.h"
#include "nsContentUtils.h"

class nsContentList;
class nsIDOMHTMLOptionElement;
class nsIHTMLCollection;
class nsISelectControlFrame;
class nsPresState;

namespace mozilla {

class EventChainPostVisitor;
class EventChainPreVisitor;

namespace dom {

class HTMLSelectElement;

#define NS_SELECT_STATE_IID                        \
{ /* 4db54c7c-d159-455f-9d8e-f60ee466dbf3 */       \
  0x4db54c7c,                                      \
  0xd159,                                          \
  0x455f,                                          \
  {0x9d, 0x8e, 0xf6, 0x0e, 0xe4, 0x66, 0xdb, 0xf3} \
}




class SelectState : public nsISupports
{
public:
  SelectState()
  {
  }
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_SELECT_STATE_IID)
  NS_DECL_ISUPPORTS

  void PutOption(int32_t aIndex, const nsAString& aValue)
  {
    
    if (aValue.IsEmpty()) {
      mIndices.Put(aIndex);
    } else {
      mValues.Put(aValue);
    }
  }

  bool ContainsOption(int32_t aIndex, const nsAString& aValue)
  {
    return mValues.Contains(aValue) || mIndices.Contains(aIndex);
  }

private:
  virtual ~SelectState()
  {
  }

  nsCheapSet<nsStringHashKey> mValues;
  nsCheapSet<nsUint32HashKey> mIndices;
};

NS_DEFINE_STATIC_IID_ACCESSOR(SelectState, NS_SELECT_STATE_IID)

class MOZ_STACK_CLASS SafeOptionListMutation
{
public:
  







  SafeOptionListMutation(nsIContent* aSelect, nsIContent* aParent,
                         nsIContent* aKid, uint32_t aIndex, bool aNotify);
  ~SafeOptionListMutation();
  void MutationFailed() { mNeedsRebuild = true; }
private:
  static void* operator new(size_t) CPP_THROW_NEW { return 0; }
  static void operator delete(void*, size_t) {}
  
  nsRefPtr<HTMLSelectElement> mSelect;
  
  bool                       mTopLevelMutation;
  
  bool                       mNeedsRebuild;
  
  nsMutationGuard            mGuard;
};





class HTMLSelectElement MOZ_FINAL : public nsGenericHTMLFormElementWithState,
                                    public nsIDOMHTMLSelectElement,
                                    public nsIConstraintValidation
{
public:
  










  enum OptionType {
    IS_SELECTED   = 1 << 0,
    CLEAR_ALL     = 1 << 1,
    SET_DISABLED  = 1 << 2,
    NOTIFY        = 1 << 3
  };

  using nsIConstraintValidation::GetValidationMessage;

  explicit HTMLSelectElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                             FromParser aFromParser = NOT_FROM_PARSER);

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLSelectElement, select)

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLSELECTELEMENT

  
  bool Autofocus() const
  {
    return GetBoolAttr(nsGkAtoms::autofocus);
  }
  void SetAutofocus(bool aVal, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::autofocus, aVal, aRv);
  }
  void GetAutocomplete(DOMString& aValue);
  void SetAutocomplete(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::autocomplete, aValue, aRv);
  }
  bool Disabled() const
  {
    return GetBoolAttr(nsGkAtoms::disabled);
  }
  void SetDisabled(bool aVal, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::disabled, aVal, aRv);
  }
  HTMLFormElement* GetForm() const
  {
    return nsGenericHTMLFormElementWithState::GetForm();
  }
  bool Multiple() const
  {
    return GetBoolAttr(nsGkAtoms::multiple);
  }
  void SetMultiple(bool aVal, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::multiple, aVal, aRv);
  }
  
  void SetName(const nsAString& aName, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::name, aName, aRv);
  }
  bool Required() const
  {
    return GetBoolAttr(nsGkAtoms::required);
  }
  void SetRequired(bool aVal, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::required, aVal, aRv);
  }
  uint32_t Size() const
  {
    return GetUnsignedIntAttr(nsGkAtoms::size, 0);
  }
  void SetSize(uint32_t aSize, ErrorResult& aRv)
  {
    SetUnsignedIntAttr(nsGkAtoms::size, aSize, aRv);
  }

  

  HTMLOptionsCollection* Options() const
  {
    return mOptions;
  }
  uint32_t Length() const
  {
    return mOptions->Length();
  }
  void SetLength(uint32_t aLength, ErrorResult& aRv);
  Element* IndexedGetter(uint32_t aIdx, bool& aFound) const
  {
    return mOptions->IndexedGetter(aIdx, aFound);
  }
  HTMLOptionElement* Item(uint32_t aIdx) const
  {
    return mOptions->ItemAsOption(aIdx);
  }
  HTMLOptionElement* NamedItem(const nsAString& aName) const
  {
    return mOptions->GetNamedItem(aName);
  }
  void Add(const HTMLOptionElementOrHTMLOptGroupElement& aElement,
           const Nullable<HTMLElementOrLong>& aBefore,
           ErrorResult& aRv);
  
  void IndexedSetter(uint32_t aIndex, HTMLOptionElement* aOption,
                     ErrorResult& aRv)
  {
    mOptions->IndexedSetter(aIndex, aOption, aRv);
  }

  static bool MatchSelectedOptions(nsIContent* aContent, int32_t, nsIAtom*,
                                   void*);

  nsIHTMLCollection* SelectedOptions();

  int32_t SelectedIndex() const
  {
    return mSelectedIndex;
  }
  void SetSelectedIndex(int32_t aIdx, ErrorResult& aRv)
  {
    aRv = SetSelectedIndexInternal(aIdx, true);
  }
  void GetValue(DOMString& aValue);
  

  
  
  
  
  using nsIConstraintValidation::CheckValidity;
  

  using nsINode::Remove;


  
  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

  
  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) MOZ_OVERRIDE;
  virtual nsresult PostHandleEvent(
                     EventChainPostVisitor& aVisitor) MOZ_OVERRIDE;

  virtual bool IsHTMLFocusable(bool aWithMouse, bool* aIsFocusable, int32_t* aTabIndex) MOZ_OVERRIDE;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify) MOZ_OVERRIDE;
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) MOZ_OVERRIDE;

  
  NS_IMETHOD_(uint32_t) GetType() const MOZ_OVERRIDE { return NS_FORM_SELECT; }
  NS_IMETHOD Reset() MOZ_OVERRIDE;
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission) MOZ_OVERRIDE;
  NS_IMETHOD SaveState() MOZ_OVERRIDE;
  virtual bool RestoreState(nsPresState* aState) MOZ_OVERRIDE;
  virtual bool IsDisabledForEvents(uint32_t aMessage) MOZ_OVERRIDE;

  virtual void FieldSetDisabledChanged(bool aNotify) MOZ_OVERRIDE;

  EventStates IntrinsicState() const MOZ_OVERRIDE;

  









  NS_IMETHOD WillAddOptions(nsIContent* aOptions,
                            nsIContent* aParent,
                            int32_t aContentIndex,
                            bool aNotify);

  







  NS_IMETHOD WillRemoveOptions(nsIContent* aParent,
                               int32_t aContentIndex,
                               bool aNotify);

  





  NS_IMETHOD IsOptionDisabled(int32_t aIndex,
                              bool* aIsDisabled);
  bool IsOptionDisabled(HTMLOptionElement* aOption);

  












  bool SetOptionsSelectedByIndex(int32_t aStartIndex,
                                 int32_t aEndIndex,
                                 uint32_t aOptionsMask);

  







  NS_IMETHOD GetOptionIndex(nsIDOMHTMLOptionElement* aOption,
                            int32_t aStartIndex,
                            bool aForward,
                            int32_t* aIndex);

  


  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep, bool aNullParent) MOZ_OVERRIDE;
  virtual nsresult BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify) MOZ_OVERRIDE;
  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) MOZ_OVERRIDE;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify) MOZ_OVERRIDE;
  
  virtual void DoneAddingChildren(bool aHaveNotified) MOZ_OVERRIDE;
  virtual bool IsDoneAddingChildren() MOZ_OVERRIDE {
    return mIsDoneAddingChildren;
  }

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) MOZ_OVERRIDE;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const MOZ_OVERRIDE;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;

  virtual nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLSelectElement,
                                           nsGenericHTMLFormElementWithState)

  HTMLOptionsCollection* GetOptions()
  {
    return mOptions;
  }

  
  nsresult GetValidationMessage(nsAString& aValidationMessage,
                                ValidityStateType aType) MOZ_OVERRIDE;

  void UpdateValueMissingValidityState();
  


  void Add(nsGenericHTMLElement& aElement, nsGenericHTMLElement* aBefore,
           ErrorResult& aError);
  void Add(nsGenericHTMLElement& aElement, int32_t aIndex, ErrorResult& aError)
  {
    
    
    nsIContent* beforeContent = mOptions->GetElementAt(aIndex);
    return Add(aElement, nsGenericHTMLElement::FromContentOrNull(beforeContent),
               aError);
  }

  


  bool IsCombobox() const
  {
    return !Multiple() && Size() <= 1;
  }

protected:
  virtual ~HTMLSelectElement();

  friend class SafeOptionListMutation;

  
  




  bool IsOptionSelectedByIndex(int32_t aIndex);
  




  void FindSelectedIndex(int32_t aStartIndex, bool aNotify);
  



  bool SelectSomething(bool aNotify);
  




  bool CheckSelectSomething(bool aNotify);
  










  void OnOptionSelected(nsISelectControlFrame* aSelectFrame,
                        int32_t aIndex,
                        bool aSelected,
                        bool aChangeOptionState,
                        bool aNotify);
  



  void RestoreStateTo(SelectState* aNewSelected);

  
  





  void InsertOptionsIntoList(nsIContent* aOptions,
                             int32_t aListIndex,
                             int32_t aDepth,
                             bool aNotify);
  





  nsresult RemoveOptionsFromList(nsIContent* aOptions,
                                 int32_t aListIndex,
                                 int32_t aDepth,
                                 bool aNotify);
  





  void InsertOptionsIntoListRecurse(nsIContent* aOptions,
                                    int32_t* aInsertIndex,
                                    int32_t aDepth);
  






  nsresult RemoveOptionsFromListRecurse(nsIContent* aOptions,
                                        int32_t aRemoveIndex,
                                        int32_t* aNumRemoved,
                                        int32_t aDepth);

  
  void UpdateBarredFromConstraintValidation();
  bool IsValueMissing();

  




  int32_t GetContentDepth(nsIContent* aContent);
  





  int32_t GetOptionIndexAt(nsIContent* aOptions);
  






  int32_t GetOptionIndexAfter(nsIContent* aOptions);
  




  int32_t GetFirstOptionIndex(nsIContent* aOptions);
  







  int32_t GetFirstChildOptionIndex(nsIContent* aOptions,
                                   int32_t aStartIndex,
                                   int32_t aEndIndex);

  



  nsISelectControlFrame* GetSelectFrame();

  



  void DispatchContentReset();

  


  void RebuildOptionsArray(bool aNotify);

#ifdef DEBUG
  void VerifyOptionsArray();
#endif

  nsresult SetSelectedIndexInternal(int32_t aIndex, bool aNotify);

  void SetSelectionChanged(bool aValue, bool aNotify);

  



  void UpdateSelectedOptions();

  





  bool ShouldShowValidityUI() const {
    





    if (mForm && mForm->HasEverTriedInvalidSubmit()) {
      return true;
    }

    return mSelectionHasChanged;
  }

  
  nsRefPtr<HTMLOptionsCollection> mOptions;
  nsContentUtils::AutocompleteAttrState mAutocompleteAttrState;
  
  bool            mIsDoneAddingChildren;
  
  bool            mDisabledChanged;
  


  bool            mMutating;
  


  bool            mInhibitStateRestoration;
  


  bool            mSelectionHasChanged;
  


  bool            mDefaultSelectionSet;
  


  bool            mCanShowInvalidUI;
  


  bool            mCanShowValidUI;

  
  uint32_t  mNonOptionChildren;
  
  uint32_t  mOptGroupCount;
  



  int32_t   mSelectedIndex;
  



  nsCOMPtr<SelectState> mRestoreState;

  


  nsRefPtr<nsContentList> mSelectedOptions;

private:
  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif

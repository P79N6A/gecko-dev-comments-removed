



#ifndef mozilla_dom_HTMLSelectElement_h
#define mozilla_dom_HTMLSelectElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIConstraintValidation.h"

#include "mozilla/dom/HTMLOptionsCollection.h"
#include "mozilla/ErrorResult.h"
#include "nsCheapSets.h"
#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsHTMLFormElement.h"

class nsIDOMHTMLOptionElement;
class nsISelectControlFrame;
class nsPresState;

namespace mozilla {
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
  virtual ~SelectState()
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
  nsCheapSet<nsStringHashKey> mValues;
  nsCheapSet<nsUint32HashKey> mIndices;
};

class NS_STACK_CLASS SafeOptionListMutation
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





class HTMLSelectElement : public nsGenericHTMLFormElement,
                          public nsIDOMHTMLSelectElement,
                          public nsIConstraintValidation
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  HTMLSelectElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                    FromParser aFromParser = NOT_FROM_PARSER);
  virtual ~HTMLSelectElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLSelectElement, select)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

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
  bool Disabled() const
  {
    return GetBoolAttr(nsGkAtoms::disabled);
  }
  void SetDisabled(bool aVal, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::disabled, aVal, aRv);
  }
  nsHTMLFormElement* GetForm() const
  {
    return nsGenericHTMLFormElement::GetForm();
  }
  bool Multiple() const
  {
    return GetBoolAttr(nsGkAtoms::multiple);
  }
  void SetMultiple(bool aVal, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::multiple, aVal, aRv);
  }
  uint32_t Size()
  {
    return GetUnsignedIntAttr(nsGkAtoms::size, 0);
  }
  void SetSize(uint32_t aSize, ErrorResult& aRv)
  {
    SetUnsignedIntAttr(nsGkAtoms::size, aSize, aRv);
  }
  void GetName(nsString& aName, ErrorResult& aRv) const
  {
    GetHTMLAttr(nsGkAtoms::name, aName);
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
  HTMLOptionsCollection* Options() const
  {
    return mOptions;
  }
  void Remove(int32_t aIdx, ErrorResult& aRv)
  {
    aRv = Remove(aIdx);
  }
  int32_t SelectedIndex() const
  {
    return mSelectedIndex;
  }
  void SetSelectedIndex(int32_t aIdx, ErrorResult& aRv)
  {
    aRv = SetSelectedIndexInternal(aIdx, true);
  }
  Element* IndexedGetter(uint32_t aIdx, bool& aFound) const
  {
    return mOptions->IndexedGetter(aIdx, aFound);
  }
  uint32_t Length() const
  {
    return mOptions->Length();
  }

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  virtual bool IsHTMLFocusable(bool aWithMouse, bool* aIsFocusable, int32_t* aTabIndex);
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify);
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify);

  
  NS_IMETHOD_(uint32_t) GetType() const { return NS_FORM_SELECT; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);
  NS_IMETHOD SaveState();
  virtual bool RestoreState(nsPresState* aState);
  virtual bool IsDisabledForEvents(uint32_t aMessage);

  virtual void FieldSetDisabledChanged(bool aNotify);

  nsEventStates IntrinsicState() const;

  









  NS_IMETHOD WillAddOptions(nsIContent* aOptions,
                            nsIContent* aParent,
                            int32_t aContentIndex,
                            bool aNotify);

  







  NS_IMETHOD WillRemoveOptions(nsIContent* aParent,
                               int32_t aContentIndex,
                               bool aNotify);

  





  NS_IMETHOD IsOptionDisabled(int32_t aIndex,
                              bool* aIsDisabled);

  
















  NS_IMETHOD SetOptionsSelectedByIndex(int32_t aStartIndex,
                                       int32_t aEndIndex,
                                       bool aIsSelected,
                                       bool aClearAll,
                                       bool aSetDisabled,
                                       bool aNotify,
                                       bool* aChangedSomething);

  







  NS_IMETHOD GetOptionIndex(nsIDOMHTMLOptionElement* aOption,
                            int32_t aStartIndex,
                            bool aForward,
                            int32_t* aIndex);

  


  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep, bool aNullParent);
  virtual nsresult BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify);
  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify);
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);
  
  virtual void DoneAddingChildren(bool aHaveNotified);
  virtual bool IsDoneAddingChildren() {
    return mIsDoneAddingChildren;
  }

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLSelectElement,
                                           nsGenericHTMLFormElement)

  HTMLOptionsCollection* GetOptions()
  {
    return mOptions;
  }

  static HTMLSelectElement* FromSupports(nsISupports* aSupports)
  {
    return static_cast<HTMLSelectElement*>(static_cast<nsINode*>(aSupports));
  }

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  nsresult GetValidationMessage(nsAString& aValidationMessage,
                                ValidityStateType aType);

  


  void Add(nsGenericHTMLElement& aElement, nsGenericHTMLElement* aBefore,
           ErrorResult& aError);
  void Add(nsGenericHTMLElement& aElement, int32_t aIndex, ErrorResult& aError)
  {
    
    
    nsIContent* beforeContent = mOptions->GetElementAt(aIndex);
    return Add(aElement, nsGenericHTMLElement::FromContentOrNull(beforeContent),
               aError);
  }

protected:
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

  
  





  nsresult InsertOptionsIntoList(nsIContent* aOptions,
                                 int32_t aListIndex,
                                 int32_t aDepth,
                                 bool aNotify);
  





  nsresult RemoveOptionsFromList(nsIContent* aOptions,
                                 int32_t aListIndex,
                                 int32_t aDepth,
                                 bool aNotify);
  





  nsresult InsertOptionsIntoListRecurse(nsIContent* aOptions,
                                        int32_t* aInsertIndex,
                                        int32_t aDepth);
  






  nsresult RemoveOptionsFromListRecurse(nsIContent* aOptions,
                                        int32_t aRemoveIndex,
                                        int32_t* aNumRemoved,
                                        int32_t aDepth);

  
  void UpdateBarredFromConstraintValidation();
  bool IsValueMissing();
  void UpdateValueMissingValidityState();

  




  int32_t GetContentDepth(nsIContent* aContent);
  





  int32_t GetOptionIndexAt(nsIContent* aOptions);
  






  int32_t GetOptionIndexAfter(nsIContent* aOptions);
  




  int32_t GetFirstOptionIndex(nsIContent* aOptions);
  







  int32_t GetFirstChildOptionIndex(nsIContent* aOptions,
                                   int32_t aStartIndex,
                                   int32_t aEndIndex);

  



  nsISelectControlFrame* GetSelectFrame();

  


  bool IsCombobox() {
    if (HasAttr(kNameSpaceID_None, nsGkAtoms::multiple)) {
      return false;
    }

    uint32_t size = 1;
    GetSize(&size);
    return size <= 1;
  }

  



  void DispatchContentReset();

  


  void RebuildOptionsArray(bool aNotify);

#ifdef DEBUG
  void VerifyOptionsArray();
#endif

  nsresult SetSelectedIndexInternal(int32_t aIndex, bool aNotify);

  void SetSelectionChanged(bool aValue, bool aNotify);

  





  bool ShouldShowValidityUI() const {
    





    if (mForm && mForm->HasEverTriedInvalidSubmit()) {
      return true;
    }

    return mSelectionHasChanged;
  }

  
  nsRefPtr<HTMLOptionsCollection> mOptions;
  
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
};

} 
} 

#endif 

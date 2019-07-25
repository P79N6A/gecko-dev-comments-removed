






































#ifndef nsHTMLSelectElement_h___
#define nsHTMLSelectElement_h___

#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsGenericHTMLElement.h"
#include "nsISelectElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMHTMLOptionsCollection.h"
#include "nsIDOMNSHTMLOptionCollectn.h"
#include "nsISelectControlFrame.h"
#include "nsContentUtils.h"
#include "nsIHTMLCollection.h"
#include "nsIConstraintValidation.h"


#include "nsXPCOM.h"
#include "nsPresState.h"
#include "nsIComponentManager.h"
#include "nsCheapSets.h"
#include "nsLayoutErrors.h"
#include "nsHTMLOptionElement.h"

class nsHTMLSelectElement;





class nsHTMLOptionCollection: public nsIDOMHTMLOptionsCollection,
                              public nsIDOMNSHTMLOptionCollection,
                              public nsIHTMLCollection
{
public:
  nsHTMLOptionCollection(nsHTMLSelectElement* aSelect);
  virtual ~nsHTMLOptionCollection();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMHTMLOPTIONSCOLLECTION

  
  NS_DECL_NSIDOMNSHTMLOPTIONCOLLECTION

  
  

  virtual nsIContent* GetNodeAt(PRUint32 aIndex, nsresult* aResult);
  virtual nsISupports* GetNamedItem(const nsAString& aName,
                                    nsWrapperCache** aCache,
                                    nsresult* aResult);

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsHTMLOptionCollection,
                                           nsIHTMLCollection)

  
  




  PRBool InsertOptionAt(nsHTMLOptionElement* aOption, PRUint32 aIndex)
  {
    return !!mElements.InsertElementAt(aIndex, aOption);
  }

  



  void RemoveOptionAt(PRUint32 aIndex)
  {
    mElements.RemoveElementAt(aIndex);
  }

  




  nsHTMLOptionElement *ItemAsOption(PRUint32 aIndex)
  {
    return mElements.SafeElementAt(aIndex, nsnull);
  }

  


  void Clear()
  {
    mElements.Clear();
  }

  


  PRBool AppendOption(nsHTMLOptionElement* aOption)
  {
    return !!mElements.AppendElement(aOption);
  }

  


  void DropReference();

  


  nsresult GetOptionIndex(mozilla::dom::Element* aOption,
                          PRInt32 aStartIndex, PRBool aForward,
                          PRInt32* aIndex);

private:
  
  nsTArray<nsRefPtr<nsHTMLOptionElement> > mElements;
  
  nsHTMLSelectElement* mSelect;
};

#define NS_SELECT_STATE_IID                        \
{ /* 4db54c7c-d159-455f-9d8e-f60ee466dbf3 */       \
  0x4db54c7c,                                      \
  0xd159,                                          \
  0x455f,                                          \
  {0x9d, 0x8e, 0xf6, 0x0e, 0xe4, 0x66, 0xdb, 0xf3} \
}




class nsSelectState : public nsISupports {
public:
  nsSelectState()
  {
  }
  virtual ~nsSelectState()
  {
  }

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_SELECT_STATE_IID)
  NS_DECL_ISUPPORTS

  void PutOption(PRInt32 aIndex, const nsAString& aValue)
  {
    
    if (aValue.IsEmpty()) {
      mIndices.Put(aIndex);
    } else {
      mValues.Put(aValue);
    }
  }

  PRBool ContainsOption(PRInt32 aIndex, const nsAString& aValue)
  {
    return mValues.Contains(aValue) || mIndices.Contains(aIndex);
  }

private:
  nsCheapStringSet mValues;
  nsCheapInt32Set mIndices;
};

class NS_STACK_CLASS nsSafeOptionListMutation
{
public:
  







  nsSafeOptionListMutation(nsIContent* aSelect, nsIContent* aParent,
                           nsIContent* aKid, PRUint32 aIndex);
  ~nsSafeOptionListMutation();
  void MutationFailed() { mNeedsRebuild = PR_TRUE; }
private:
  static void* operator new(size_t) CPP_THROW_NEW { return 0; }
  static void operator delete(void*, size_t) {}
  
  nsCOMPtr<nsISelectElement> mSelect;
  
  PRBool                     mTopLevelMutation;
  
  PRBool                     mNeedsRebuild;
  
  nsMutationGuard            mGuard;
};





class nsHTMLSelectElement : public nsGenericHTMLFormElement,
                            public nsIDOMHTMLSelectElement,
                            public nsISelectElement,
                            public nsIConstraintValidation
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  nsHTMLSelectElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                      PRUint32 aFromParser = 0);
  virtual ~nsHTMLSelectElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLSELECTELEMENT

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  virtual PRBool IsHTMLFocusable(PRBool aWithMouse, PRBool *aIsFocusable, PRInt32 *aTabIndex);
  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify);
  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify, PRBool aMutationEvent = PR_TRUE);

  
  NS_IMETHOD_(PRUint32) GetType() const { return NS_FORM_SELECT; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);
  NS_IMETHOD SaveState();
  virtual PRBool RestoreState(nsPresState* aState);

  PRInt32 IntrinsicState() const;

  
  NS_DECL_NSISELECTELEMENT

  


  virtual nsresult BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 const nsAString* aValue, PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);
  
  virtual nsresult DoneAddingChildren(PRBool aHaveNotified);
  virtual PRBool IsDoneAddingChildren();

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLSelectElement,
                                                     nsGenericHTMLFormElement)

  nsHTMLOptionCollection *GetOptions()
  {
    return mOptions;
  }

  static nsHTMLSelectElement *FromSupports(nsISupports *aSupports)
  {
    return static_cast<nsHTMLSelectElement*>(static_cast<nsINode*>(aSupports));
  }

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  friend class nsSafeOptionListMutation;

  
  




  PRBool IsOptionSelectedByIndex(PRInt32 aIndex);
  




  void FindSelectedIndex(PRInt32 aStartIndex);
  



  PRBool SelectSomething();
  




  PRBool CheckSelectSomething();
  










  void OnOptionSelected(nsISelectControlFrame* aSelectFrame,
                        PRInt32 aIndex,
                        PRBool aSelected,
                        PRBool aChangeOptionState,
                        PRBool aNotify);
  



  void RestoreStateTo(nsSelectState* aNewSelected);

  
  





  nsresult InsertOptionsIntoList(nsIContent* aOptions,
                                 PRInt32 aListIndex,
                                 PRInt32 aDepth);
  





  nsresult RemoveOptionsFromList(nsIContent* aOptions,
                                 PRInt32 aListIndex,
                                 PRInt32 aDepth);
  





  nsresult InsertOptionsIntoListRecurse(nsIContent* aOptions,
                                        PRInt32* aInsertIndex,
                                        PRInt32 aDepth);
  






  nsresult RemoveOptionsFromListRecurse(nsIContent* aOptions,
                                        PRInt32 aRemoveIndex,
                                        PRInt32* aNumRemoved,
                                        PRInt32 aDepth);
  




  PRInt32 GetContentDepth(nsIContent* aContent);
  





  PRInt32 GetOptionIndexAt(nsIContent* aOptions);
  






  PRInt32 GetOptionIndexAfter(nsIContent* aOptions);
  




  PRInt32 GetFirstOptionIndex(nsIContent* aOptions);
  







  PRInt32 GetFirstChildOptionIndex(nsIContent* aOptions,
                                   PRInt32 aStartIndex,
                                   PRInt32 aEndIndex);

  



  nsISelectControlFrame *GetSelectFrame();

  


  PRBool IsCombobox() {
    PRBool isMultiple = PR_TRUE;
    PRInt32 size = 1;
    GetSize(&size);
    GetMultiple(&isMultiple);
    return !isMultiple && size <= 1;
  }

  



  void DispatchContentReset();

  


  void RebuildOptionsArray();

#ifdef DEBUG
  void VerifyOptionsArray();
#endif

  virtual PRBool AcceptAutofocus() const
  {
    return PR_TRUE;
  }

  


  PRInt32 GetDefaultSize() const
  {
    return HasAttr(kNameSpaceID_None, nsGkAtoms::multiple) ? 4 : 1;
  }

  
  nsRefPtr<nsHTMLOptionCollection> mOptions;
  
  PRPackedBool    mIsDoneAddingChildren;
  
  PRPackedBool    mDisabledChanged;
  


  PRPackedBool    mMutating;
  


  PRPackedBool    mInhibitStateRestoration;
  
  PRUint32  mNonOptionChildren;
  
  PRUint32  mOptGroupCount;
  



  PRInt32   mSelectedIndex;
  



  nsCOMPtr<nsSelectState> mRestoreState;
};

#endif

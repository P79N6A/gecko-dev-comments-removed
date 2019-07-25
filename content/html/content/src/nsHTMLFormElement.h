




































#ifndef nsHTMLFormElement_h__
#define nsHTMLFormElement_h__

#include "nsCOMPtr.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsFormSubmission.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIWebProgressListener.h"
#include "nsIRadioGroupContainer.h"
#include "nsIURI.h"
#include "nsIWeakReferenceUtils.h"
#include "nsPIDOMWindow.h"
#include "nsUnicharUtils.h"
#include "nsThreadUtils.h"
#include "nsInterfaceHashtable.h"
#include "nsDataHashtable.h"

class nsFormControlList;
class nsIMutableArray;






class nsStringCaseInsensitiveHashKey : public PLDHashEntryHdr
{
public:
  typedef const nsAString& KeyType;
  typedef const nsAString* KeyTypePointer;
  nsStringCaseInsensitiveHashKey(KeyTypePointer aStr) : mStr(*aStr) { } 
  nsStringCaseInsensitiveHashKey(const nsStringCaseInsensitiveHashKey& toCopy) : mStr(toCopy.mStr) { }
  ~nsStringCaseInsensitiveHashKey() { }

  KeyType GetKey() const { return mStr; }
  PRBool KeyEquals(const KeyTypePointer aKey) const
  {
    return mStr.Equals(*aKey,nsCaseInsensitiveStringComparator());
  }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(const KeyTypePointer aKey)
  {
      nsAutoString tmKey(*aKey);
      ToLowerCase(tmKey);
      return HashString(tmKey);
  }
  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  const nsString mStr;
};

class nsHTMLFormElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLFormElement,
                          public nsIWebProgressListener,
                          public nsIForm,
                          public nsIRadioGroupContainer_MOZILLA_2_0_BRANCH
{
public:
  nsHTMLFormElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLFormElement();

  nsresult Init();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLFORMELEMENT

  
  NS_DECL_NSIWEBPROGRESSLISTENER

  
  NS_IMETHOD_(nsIFormControl*) GetElementAt(PRInt32 aIndex) const;
  NS_IMETHOD_(PRUint32) GetElementCount() const;
  NS_IMETHOD_(already_AddRefed<nsISupports>) ResolveName(const nsAString& aName);
  NS_IMETHOD_(PRInt32) IndexOfControl(nsIFormControl* aControl);
  NS_IMETHOD_(nsIFormControl*) GetDefaultSubmitElement() const;

  
  NS_IMETHOD SetCurrentRadioButton(const nsAString& aName,
                                   nsIDOMHTMLInputElement* aRadio);
  NS_IMETHOD GetCurrentRadioButton(const nsAString& aName,
                                   nsIDOMHTMLInputElement** aRadio);
  NS_IMETHOD GetPositionInGroup(nsIDOMHTMLInputElement *aRadio,
                                PRInt32 *aPositionIndex,
                                PRInt32 *aItemsInGroup);
  NS_IMETHOD GetNextRadioButton(const nsAString& aName,
                                const PRBool aPrevious,
                                nsIDOMHTMLInputElement*  aFocusedRadio,
                                nsIDOMHTMLInputElement** aRadioOut);
  NS_IMETHOD WalkRadioGroup(const nsAString& aName, nsIRadioVisitor* aVisitor,
                            PRBool aFlushContent);
  NS_IMETHOD AddToRadioGroup(const nsAString& aName,
                             nsIFormControl* aRadio);
  NS_IMETHOD RemoveFromRadioGroup(const nsAString& aName,
                                  nsIFormControl* aRadio);
  virtual PRUint32 GetRequiredRadioCount(const nsAString& aName) const;
  virtual void RadioRequiredChanged(const nsAString& aName,
                                    nsIFormControl* aRadio);
  virtual bool GetValueMissingState(const nsAString& aName) const;
  virtual void SetValueMissingState(const nsAString& aName, bool aValue);

  
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult WillHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                const nsAString* aValue, PRBool aNotify);

  



  void ForgetCurrentSubmission();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLFormElement,
                                                     nsGenericHTMLElement)

  






  nsresult RemoveElement(nsGenericHTMLFormElement* aElement,
                         bool aUpdateValidity);

  










  nsresult RemoveElementFromTable(nsGenericHTMLFormElement* aElement,
                                  const nsAString& aName);
  







  nsresult AddElement(nsGenericHTMLFormElement* aElement, bool aUpdateValidity,
                      PRBool aNotify);

  







  nsresult AddElementToTable(nsGenericHTMLFormElement* aChild,
                             const nsAString& aName);
   




  PRBool HasSingleTextControl() const;

  






  PRBool IsDefaultSubmitElement(const nsIFormControl* aControl) const;

  




  void OnSubmitClickBegin(nsIContent* aOriginatingElement);
  void OnSubmitClickEnd();

  











  void UpdateValidity(PRBool aElementValidityState);

  






  PRBool GetValidity() const { return !mInvalidElementsCount; }

  








  bool CheckValidFormSubmission();

  virtual nsXPCClassInfo* GetClassInfo();

  





  nsresult WalkFormElements(nsFormSubmission* aFormSubmission);

  






  bool HasEverTriedInvalidSubmit() const { return mEverTriedInvalidSubmit; }

protected:
  class RemoveElementRunnable;
  friend class RemoveElementRunnable;

  class RemoveElementRunnable : public nsRunnable {
  public:
    RemoveElementRunnable(nsHTMLFormElement* aForm)
      : mForm(aForm)
    {}

    NS_IMETHOD Run() {
      mForm->HandleDefaultSubmitRemoval();
      return NS_OK;
    }

  private:
    nsRefPtr<nsHTMLFormElement> mForm;
  };

  nsresult DoSubmitOrReset(nsEvent* aEvent,
                           PRInt32 aMessage);
  nsresult DoReset();

  
  void HandleDefaultSubmitRemoval();

  
  
  
  
  






  nsresult DoSubmit(nsEvent* aEvent);

  





  nsresult BuildSubmission(nsFormSubmission** aFormSubmission, 
                           nsEvent* aEvent);
  




  nsresult SubmitSubmission(nsFormSubmission* aFormSubmission);

  






  nsresult NotifySubmitObservers(nsIURI* aActionURL, PRBool* aCancelSubmit,
                                 PRBool aEarlyNotify);

  


  already_AddRefed<nsISupports> DoResolveName(const nsAString& aName, PRBool aFlushContent);

  





  nsresult GetActionURL(nsIURI** aActionURL, nsIContent* aOriginatingElement);

  








  PRBool CheckFormValidity(nsIMutableArray* aInvalidElements) const;

public:
  





  void FlushPendingSubmission();
protected:

  
  
  
  
  nsRefPtr<nsFormControlList> mControls;
  
  nsInterfaceHashtable<nsStringCaseInsensitiveHashKey,nsIDOMHTMLInputElement> mSelectedRadioButtons;
  
  nsDataHashtable<nsStringCaseInsensitiveHashKey,PRUint32> mRequiredRadioButtonCounts;
  
  nsDataHashtable<nsStringCaseInsensitiveHashKey,bool> mValueMissingRadioGroups;
  
  PRPackedBool mGeneratingSubmit;
  
  PRPackedBool mGeneratingReset;
  
  PRPackedBool mIsSubmitting;
  
  PRPackedBool mDeferSubmission;
  
  PRPackedBool mNotifiedObservers;
  
  PRPackedBool mNotifiedObserversResult;
  
  PopupControlState mSubmitPopupState;
  
  PRBool mSubmitInitiatedFromUserInput;

  
  nsAutoPtr<nsFormSubmission> mPendingSubmission;
  
  nsCOMPtr<nsIRequest> mSubmittingRequest;
  
  nsWeakPtr mWebProgress;

  
  nsGenericHTMLFormElement* mDefaultSubmitElement;

  
  nsGenericHTMLFormElement* mFirstSubmitInElements;

  
  nsGenericHTMLFormElement* mFirstSubmitNotInElements;

  




  PRInt32 mInvalidElementsCount;

  



  bool mEverTriedInvalidSubmit;

protected:
  
  static PRBool gFirstFormSubmitted;
  
  static PRBool gPasswordManagerInitialized;
};

#endif 

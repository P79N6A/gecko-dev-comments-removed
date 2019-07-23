



































#include "nsCOMPtr.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsIFormSubmission.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMNSHTMLFormElement.h"
#include "nsIWebProgressListener.h"
#include "nsIRadioGroupContainer.h"
#include "nsIURI.h"
#include "nsIWeakReferenceUtils.h"
#include "nsPIDOMWindow.h"
#include "nsUnicharUtils.h"
#include "nsThreadUtils.h"

class nsFormControlList;






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
                          public nsIDOMNSHTMLFormElement,
                          public nsIWebProgressListener,
                          public nsIForm,
                          public nsIRadioGroupContainer
{
public:
  nsHTMLFormElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLFormElement();

  nsresult Init();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLFORMELEMENT

  
  NS_DECL_NSIDOMNSHTMLFORMELEMENT  

  
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

  



  void ForgetCurrentSubmission();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLFormElement,
                                                     nsGenericHTMLElement)

  






  nsresult RemoveElement(nsGenericHTMLFormElement* aElement, PRBool aNotify);

  










  nsresult RemoveElementFromTable(nsGenericHTMLFormElement* aElement,
                                  const nsAString& aName);
  






  nsresult AddElement(nsGenericHTMLFormElement* aElement, PRBool aNotify);

  







  nsresult AddElementToTable(nsGenericHTMLFormElement* aChild,
                             const nsAString& aName);
   




  PRBool HasSingleTextControl() const;

  






  PRBool IsDefaultSubmitElement(const nsIFormControl* aControl) const;

  




  void OnSubmitClickBegin();
  void OnSubmitClickEnd();

protected:
  class RemoveElementRunnable;
  friend class RemoveElementRunnable;

  class RemoveElementRunnable : public nsRunnable {
  public:
    RemoveElementRunnable(nsHTMLFormElement* aForm, PRBool aNotify):
      mForm(aForm), mNotify(aNotify)
    {}

    NS_IMETHOD Run() {
      mForm->HandleDefaultSubmitRemoval(mNotify);
      return NS_OK;
    }

  private:
    nsRefPtr<nsHTMLFormElement> mForm;
    PRBool mNotify;
  };

  nsresult DoSubmitOrReset(nsEvent* aEvent,
                           PRInt32 aMessage);
  nsresult DoReset();

  
  void HandleDefaultSubmitRemoval(PRBool aNotify);

  
  
  
  
  






  nsresult DoSubmit(nsEvent* aEvent);

  





  nsresult BuildSubmission(nsCOMPtr<nsIFormSubmission>& aFormSubmission, 
                           nsEvent* aEvent);
  




  nsresult SubmitSubmission(nsIFormSubmission* aFormSubmission);
  






  nsresult WalkFormElements(nsIFormSubmission* aFormSubmission,
                            nsIContent* aSubmitElement);

  






  nsresult NotifySubmitObservers(nsIURI* aActionURL, PRBool* aCancelSubmit,
                                 PRBool aEarlyNotify);

  


  already_AddRefed<nsISupports> DoResolveName(const nsAString& aName, PRBool aFlushContent);

  




  nsresult GetActionURL(nsIURI** aActionURL);

public:
  





  void FlushPendingSubmission();
protected:
  





  void ForgetPendingSubmission();

  
  
  
  
  nsRefPtr<nsFormControlList> mControls;
  
  nsInterfaceHashtable<nsStringCaseInsensitiveHashKey,nsIDOMHTMLInputElement> mSelectedRadioButtons;
  
  PRPackedBool mGeneratingSubmit;
  
  PRPackedBool mGeneratingReset;
  
  PRPackedBool mIsSubmitting;
  
  PRPackedBool mDeferSubmission;
  
  PRPackedBool mNotifiedObservers;
  
  PRPackedBool mNotifiedObserversResult;
  
  PopupControlState mSubmitPopupState;
  
  PRBool mSubmitInitiatedFromUserInput;

  
  nsCOMPtr<nsIFormSubmission> mPendingSubmission;
  
  nsCOMPtr<nsIRequest> mSubmittingRequest;
  
  nsWeakPtr mWebProgress;

  
  nsGenericHTMLFormElement* mDefaultSubmitElement;

  
  nsGenericHTMLFormElement* mFirstSubmitInElements;

  
  nsGenericHTMLFormElement* mFirstSubmitNotInElements;

protected:
  
  static PRBool gFirstFormSubmitted;
  
  static PRBool gPasswordManagerInitialized;
};

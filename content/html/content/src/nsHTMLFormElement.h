




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
#include "nsIWeakReferenceUtils.h"
#include "nsThreadUtils.h"
#include "nsInterfaceHashtable.h"
#include "nsDataHashtable.h"
#include "nsAsyncDOMEvent.h"

class nsFormControlList;
class nsIMutableArray;
class nsIURI;

class nsHTMLFormElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLFormElement,
                          public nsIWebProgressListener,
                          public nsIForm,
                          public nsIRadioGroupContainer
{
public:
  nsHTMLFormElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLFormElement();

  nsresult Init();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLFORMELEMENT

  
  NS_DECL_NSIWEBPROGRESSLISTENER

  
  NS_IMETHOD_(nsIFormControl*) GetElementAt(int32_t aIndex) const;
  NS_IMETHOD_(uint32_t) GetElementCount() const;
  NS_IMETHOD_(already_AddRefed<nsISupports>) ResolveName(const nsAString& aName);
  NS_IMETHOD_(int32_t) IndexOfControl(nsIFormControl* aControl);
  NS_IMETHOD_(nsIFormControl*) GetDefaultSubmitElement() const;

  
  void SetCurrentRadioButton(const nsAString& aName,
                             nsIDOMHTMLInputElement* aRadio);
  nsIDOMHTMLInputElement* GetCurrentRadioButton(const nsAString& aName);
  NS_IMETHOD GetNextRadioButton(const nsAString& aName,
                                const bool aPrevious,
                                nsIDOMHTMLInputElement*  aFocusedRadio,
                                nsIDOMHTMLInputElement** aRadioOut);
  NS_IMETHOD WalkRadioGroup(const nsAString& aName, nsIRadioVisitor* aVisitor,
                            bool aFlushContent);
  void AddToRadioGroup(const nsAString& aName, nsIFormControl* aRadio);
  void RemoveFromRadioGroup(const nsAString& aName, nsIFormControl* aRadio);
  virtual uint32_t GetRequiredRadioCount(const nsAString& aName) const;
  virtual void RadioRequiredChanged(const nsAString& aName,
                                    nsIFormControl* aRadio);
  virtual bool GetValueMissingState(const nsAString& aName) const;
  virtual void SetValueMissingState(const nsAString& aName, bool aValue);

  virtual nsEventStates IntrinsicState() const;

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult WillHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify);

  



  void ForgetCurrentSubmission();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLFormElement,
                                                     nsGenericHTMLElement)

  






  nsresult RemoveElement(nsGenericHTMLFormElement* aElement,
                         bool aUpdateValidity);

  










  nsresult RemoveElementFromTable(nsGenericHTMLFormElement* aElement,
                                  const nsAString& aName);
  







  nsresult AddElement(nsGenericHTMLFormElement* aElement, bool aUpdateValidity,
                      bool aNotify);

  







  nsresult AddElementToTable(nsGenericHTMLFormElement* aChild,
                             const nsAString& aName);
   




  bool HasSingleTextControl() const;

  






  bool IsDefaultSubmitElement(const nsIFormControl* aControl) const;

  




  void OnSubmitClickBegin(nsIContent* aOriginatingElement);
  void OnSubmitClickEnd();

  











  void UpdateValidity(bool aElementValidityState);

  






  bool GetValidity() const { return !mInvalidElementsCount; }

  








  bool CheckValidFormSubmission();

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  





  nsresult WalkFormElements(nsFormSubmission* aFormSubmission);

  






  bool HasEverTriedInvalidSubmit() const { return mEverTriedInvalidSubmit; }

protected:
  void PostPasswordEvent();
  void EventHandled() { mFormPasswordEvent = nullptr; }

  class FormPasswordEvent : public nsAsyncDOMEvent
  {
  public:
    FormPasswordEvent(nsHTMLFormElement* aEventNode,
                      const nsAString& aEventType)
      : nsAsyncDOMEvent(aEventNode, aEventType, true, true)
    {}

    NS_IMETHOD Run()
    {
      static_cast<nsHTMLFormElement*>(mEventNode.get())->EventHandled();
      return nsAsyncDOMEvent::Run();
    }
  };

  nsRefPtr<FormPasswordEvent> mFormPasswordEvent;

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
                           int32_t aMessage);
  nsresult DoReset();

  
  void HandleDefaultSubmitRemoval();

  
  
  
  
  






  nsresult DoSubmit(nsEvent* aEvent);

  





  nsresult BuildSubmission(nsFormSubmission** aFormSubmission, 
                           nsEvent* aEvent);
  




  nsresult SubmitSubmission(nsFormSubmission* aFormSubmission);

  






  nsresult NotifySubmitObservers(nsIURI* aActionURL, bool* aCancelSubmit,
                                 bool aEarlyNotify);

  


  already_AddRefed<nsISupports> DoResolveName(const nsAString& aName, bool aFlushContent);

  





  nsresult GetActionURL(nsIURI** aActionURL, nsIContent* aOriginatingElement);

  








  bool CheckFormValidity(nsIMutableArray* aInvalidElements) const;

public:
  





  void FlushPendingSubmission();
protected:

  
  
  
  
  nsRefPtr<nsFormControlList> mControls;
  
  nsInterfaceHashtable<nsStringCaseInsensitiveHashKey,nsIDOMHTMLInputElement> mSelectedRadioButtons;
  
  nsDataHashtable<nsStringCaseInsensitiveHashKey,uint32_t> mRequiredRadioButtonCounts;
  
  nsDataHashtable<nsStringCaseInsensitiveHashKey,bool> mValueMissingRadioGroups;
  
  bool mGeneratingSubmit;
  
  bool mGeneratingReset;
  
  bool mIsSubmitting;
  
  bool mDeferSubmission;
  
  bool mNotifiedObservers;
  
  bool mNotifiedObserversResult;
  
  PopupControlState mSubmitPopupState;
  
  bool mSubmitInitiatedFromUserInput;

  
  nsAutoPtr<nsFormSubmission> mPendingSubmission;
  
  nsCOMPtr<nsIRequest> mSubmittingRequest;
  
  nsWeakPtr mWebProgress;

  
  nsGenericHTMLFormElement* mDefaultSubmitElement;

  
  nsGenericHTMLFormElement* mFirstSubmitInElements;

  
  nsGenericHTMLFormElement* mFirstSubmitNotInElements;

  




  int32_t mInvalidElementsCount;

  



  bool mEverTriedInvalidSubmit;

protected:
  
  static bool gFirstFormSubmitted;
  
  static bool gPasswordManagerInitialized;
};

#endif 

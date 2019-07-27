




#ifndef mozilla_dom_HTMLFormElement_h
#define mozilla_dom_HTMLFormElement_h

#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/Attributes.h"
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
#include "nsRefPtrHashtable.h"
#include "nsDataHashtable.h"
#include "jsfriendapi.h" 

class nsIMutableArray;
class nsIURI;

namespace mozilla {
class EventChainPostVisitor;
class EventChainPreVisitor;
namespace dom {
class HTMLFormControlsCollection;
class HTMLImageElement;

class HTMLFormElement MOZ_FINAL : public nsGenericHTMLElement,
                                  public nsIDOMHTMLFormElement,
                                  public nsIWebProgressListener,
                                  public nsIForm,
                                  public nsIRadioGroupContainer
{
  friend class HTMLFormControlsCollection;

public:
  explicit HTMLFormElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  enum {
    FORM_CONTROL_LIST_HASHTABLE_LENGTH = 8
  };

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLFORMELEMENT

  
  NS_DECL_NSIWEBPROGRESSLISTENER

  
  NS_IMETHOD_(nsIFormControl*) GetElementAt(int32_t aIndex) const;
  NS_IMETHOD_(uint32_t) GetElementCount() const MOZ_OVERRIDE;
  NS_IMETHOD_(int32_t) IndexOfControl(nsIFormControl* aControl) MOZ_OVERRIDE;
  NS_IMETHOD_(nsIFormControl*) GetDefaultSubmitElement() const MOZ_OVERRIDE;

  
  void SetCurrentRadioButton(const nsAString& aName,
                             HTMLInputElement* aRadio) MOZ_OVERRIDE;
  HTMLInputElement* GetCurrentRadioButton(const nsAString& aName) MOZ_OVERRIDE;
  NS_IMETHOD GetNextRadioButton(const nsAString& aName,
                                const bool aPrevious,
                                HTMLInputElement* aFocusedRadio,
                                HTMLInputElement** aRadioOut) MOZ_OVERRIDE;
  NS_IMETHOD WalkRadioGroup(const nsAString& aName, nsIRadioVisitor* aVisitor,
                            bool aFlushContent) MOZ_OVERRIDE;
  void AddToRadioGroup(const nsAString& aName, nsIFormControl* aRadio) MOZ_OVERRIDE;
  void RemoveFromRadioGroup(const nsAString& aName, nsIFormControl* aRadio) MOZ_OVERRIDE;
  virtual uint32_t GetRequiredRadioCount(const nsAString& aName) const MOZ_OVERRIDE;
  virtual void RadioRequiredChanged(const nsAString& aName,
                                    nsIFormControl* aRadio) MOZ_OVERRIDE;
  virtual bool GetValueMissingState(const nsAString& aName) const MOZ_OVERRIDE;
  virtual void SetValueMissingState(const nsAString& aName, bool aValue) MOZ_OVERRIDE;

  virtual EventStates IntrinsicState() const MOZ_OVERRIDE;

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) MOZ_OVERRIDE;
  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) MOZ_OVERRIDE;
  virtual nsresult WillHandleEvent(
                     EventChainPostVisitor& aVisitor) MOZ_OVERRIDE;
  virtual nsresult PostHandleEvent(
                     EventChainPostVisitor& aVisitor) MOZ_OVERRIDE;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) MOZ_OVERRIDE;
  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) MOZ_OVERRIDE;

  



  void ForgetCurrentSubmission();

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(HTMLFormElement,
                                                         nsGenericHTMLElement)

  






  nsresult RemoveElement(nsGenericHTMLFormElement* aElement,
                         bool aUpdateValidity);

  














  enum RemoveElementReason {
    AttributeUpdated,
    ElementRemoved
  };
  nsresult RemoveElementFromTable(nsGenericHTMLFormElement* aElement,
                                  const nsAString& aName,
                                  RemoveElementReason aRemoveReason);

  







  nsresult AddElement(nsGenericHTMLFormElement* aElement, bool aUpdateValidity,
                      bool aNotify);

  







  nsresult AddElementToTable(nsGenericHTMLFormElement* aChild,
                             const nsAString& aName);

  





  nsresult RemoveImageElement(mozilla::dom::HTMLImageElement* aElement);

  










  nsresult RemoveImageElementFromTable(mozilla::dom::HTMLImageElement* aElement,
                                      const nsAString& aName,
                                      RemoveElementReason aRemoveReason);
  





  nsresult AddImageElement(mozilla::dom::HTMLImageElement* aElement);

  






  nsresult AddImageElementToTable(mozilla::dom::HTMLImageElement* aChild,
                                 const nsAString& aName);

   





  bool ImplicitSubmissionIsDisabled() const;

  






  bool IsDefaultSubmitElement(const nsIFormControl* aControl) const;

  




  void OnSubmitClickBegin(nsIContent* aOriginatingElement);
  void OnSubmitClickEnd();

  











  void UpdateValidity(bool aElementValidityState);

  






  bool GetValidity() const { return !mInvalidElementsCount; }

  








  bool CheckValidFormSubmission();

  





  nsresult WalkFormElements(nsFormSubmission* aFormSubmission);

  






  bool HasEverTriedInvalidSubmit() const { return mEverTriedInvalidSubmit; }

  



  already_AddRefed<nsISupports>
  FindNamedItem(const nsAString& aName, nsWrapperCache** aCache);

  

  void GetAcceptCharset(DOMString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::acceptcharset, aValue);
  }

  void SetAcceptCharset(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::acceptcharset, aValue, aRv);
  }

  
  void SetAction(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::action, aValue, aRv);
  }

  
  void SetAutocomplete(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::autocomplete, aValue, aRv);
  }

  
  void SetEnctype(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::enctype, aValue, aRv);
  }

  
  void SetEncoding(const nsAString& aValue, ErrorResult& aRv)
  {
    SetEnctype(aValue, aRv);
  }

  
  void SetMethod(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::method, aValue, aRv);
  }

  void GetName(DOMString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::name, aValue);
  }

  void SetName(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::name, aValue, aRv);
  }

  bool NoValidate() const
  {
    return GetBoolAttr(nsGkAtoms::novalidate);
  }

  void SetNoValidate(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::novalidate, aValue, aRv);
  }

  void GetTarget(DOMString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::target, aValue);
  }

  void SetTarget(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::target, aValue, aRv);
  }

  
  
  nsIHTMLCollection* Elements();

  int32_t Length();

  void Submit(ErrorResult& aRv);

  

  bool CheckValidity()
  {
    return CheckFormValidity(nullptr);
  }

  Element*
  IndexedGetter(uint32_t aIndex, bool &aFound);

  already_AddRefed<nsISupports>
  NamedGetter(const nsAString& aName, bool &aFound);

  bool NameIsEnumerable(const nsAString& aName);

  void GetSupportedNames(unsigned, nsTArray<nsString >& aRetval);

  static int32_t
  CompareFormControlPosition(Element* aElement1, Element* aElement2,
                             const nsIContent* aForm);
#ifdef DEBUG
  static void
  AssertDocumentOrder(const nsTArray<nsGenericHTMLFormElement*>& aControls,
                      nsIContent* aForm);
#endif

  js::ExpandoAndGeneration mExpandoAndGeneration;

  void RequestAutocomplete();

protected:
  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

  void PostPasswordEvent();
  void EventHandled() { mFormPasswordEventDispatcher = nullptr; }

  class FormPasswordEventDispatcher MOZ_FINAL : public AsyncEventDispatcher
  {
  public:
    FormPasswordEventDispatcher(HTMLFormElement* aEventNode,
                                const nsAString& aEventType)
      : AsyncEventDispatcher(aEventNode, aEventType, true, true)
    {}

    NS_IMETHOD Run() MOZ_OVERRIDE
    {
      static_cast<HTMLFormElement*>(mTarget.get())->EventHandled();
      return AsyncEventDispatcher::Run();
    }
  };

  nsRefPtr<FormPasswordEventDispatcher> mFormPasswordEventDispatcher;

  class RemoveElementRunnable;
  friend class RemoveElementRunnable;

  class RemoveElementRunnable : public nsRunnable {
  public:
    explicit RemoveElementRunnable(HTMLFormElement* aForm)
      : mForm(aForm)
    {}

    NS_IMETHOD Run() MOZ_OVERRIDE {
      mForm->HandleDefaultSubmitRemoval();
      return NS_OK;
    }

  private:
    nsRefPtr<HTMLFormElement> mForm;
  };

  nsresult DoSubmitOrReset(WidgetEvent* aEvent,
                           int32_t aMessage);
  nsresult DoReset();

  
  void HandleDefaultSubmitRemoval();

  
  
  
  
  






  nsresult DoSubmit(WidgetEvent* aEvent);

  





  nsresult BuildSubmission(nsFormSubmission** aFormSubmission,
                           WidgetEvent* aEvent);
  




  nsresult SubmitSubmission(nsFormSubmission* aFormSubmission);

  






  nsresult NotifySubmitObservers(nsIURI* aActionURL, bool* aCancelSubmit,
                                 bool aEarlyNotify);

  



  already_AddRefed<nsISupports> DoResolveName(const nsAString& aName, bool aFlushContent);

  





  nsresult GetActionURL(nsIURI** aActionURL, nsIContent* aOriginatingElement);

  








  bool CheckFormValidity(nsIMutableArray* aInvalidElements) const;

  
  void Clear();

  
  void AddToPastNamesMap(const nsAString& aName, nsISupports* aChild);

  nsresult
  AddElementToTableInternal(
    nsInterfaceHashtable<nsStringHashKey,nsISupports>& aTable,
    nsIContent* aChild, const nsAString& aName);

  nsresult
  RemoveElementFromTableInternal(
    nsInterfaceHashtable<nsStringHashKey,nsISupports>& aTable,
    nsIContent* aChild, const nsAString& aName);

public:
  





  void FlushPendingSubmission();
protected:

  
  
  
  
  nsRefPtr<HTMLFormControlsCollection> mControls;
  
  nsRefPtrHashtable<nsStringCaseInsensitiveHashKey, HTMLInputElement> mSelectedRadioButtons;
  
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

  
  
  

  nsTArray<mozilla::dom::HTMLImageElement*> mImageElements;  

  
  
  
  
  

  nsInterfaceHashtable<nsStringHashKey,nsISupports> mImageNameLookupTable;

  
  

  nsInterfaceHashtable<nsStringHashKey,nsISupports> mPastNameLookupTable;

  




  int32_t mInvalidElementsCount;

  



  bool mEverTriedInvalidSubmit;

protected:
  
  static bool gFirstFormSubmitted;
  
  static bool gPasswordManagerInitialized;

private:
  ~HTMLFormElement();
};

} 

} 

#endif 






#ifndef NS_SMILTIMEVALUESPEC_H_
#define NS_SMILTIMEVALUESPEC_H_

#include "mozilla/Attributes.h"
#include "nsSMILTimeValueSpecParams.h"
#include "nsReferencedElement.h"
#include "nsAutoPtr.h"
#include "nsIDOMEventListener.h"

class nsAString;
class nsSMILTimeValue;
class nsSMILTimedElement;
class nsSMILTimeContainer;
class nsSMILInstanceTime;
class nsSMILInterval;

namespace mozilla {
class EventListenerManager;
} 












class nsSMILTimeValueSpec
{
public:
  typedef mozilla::dom::Element Element;

  nsSMILTimeValueSpec(nsSMILTimedElement& aOwner, bool aIsBegin);
  ~nsSMILTimeValueSpec();

  nsresult SetSpec(const nsAString& aStringSpec, Element* aContextNode);
  void     ResolveReferences(nsIContent* aContextNode);
  bool     IsEventBased() const;

  void     HandleNewInterval(nsSMILInterval& aInterval,
                             const nsSMILTimeContainer* aSrcContainer);
  void     HandleTargetElementChange(Element* aNewTarget);

  
  bool     DependsOnBegin() const;
  void     HandleChangedInstanceTime(const nsSMILInstanceTime& aBaseTime,
                                     const nsSMILTimeContainer* aSrcContainer,
                                     nsSMILInstanceTime& aInstanceTimeToUpdate,
                                     bool aObjectChanged);
  void     HandleDeletedInstanceTime(nsSMILInstanceTime& aInstanceTime);

  
  void Traverse(nsCycleCollectionTraversalCallback* aCallback);
  void Unlink();

protected:
  void UpdateReferencedElement(Element* aFrom, Element* aTo);
  void UnregisterFromReferencedElement(Element* aElement);
  nsSMILTimedElement* GetTimedElement(Element* aElement);
  bool IsWhitelistedEvent();
  void RegisterEventListener(Element* aElement);
  void UnregisterEventListener(Element* aElement);
  mozilla::EventListenerManager* GetEventListenerManager(Element* aElement);
  void HandleEvent(nsIDOMEvent* aEvent);
  bool CheckEventDetail(nsIDOMEvent* aEvent);
  bool CheckRepeatEventDetail(nsIDOMEvent* aEvent);
  bool CheckAccessKeyEventDetail(nsIDOMEvent* aEvent);
  nsSMILTimeValue ConvertBetweenTimeContainers(const nsSMILTimeValue& aSrcTime,
                                      const nsSMILTimeContainer* aSrcContainer);
  bool ApplyOffset(nsSMILTimeValue& aTime) const;

  nsSMILTimedElement*           mOwner;
  bool                          mIsBegin; 
                                          
                                          
                                          
                                          
  nsSMILTimeValueSpecParams     mParams;

  class TimeReferenceElement : public nsReferencedElement
  {
  public:
    explicit TimeReferenceElement(nsSMILTimeValueSpec* aOwner) : mSpec(aOwner) { }
    void ResetWithElement(Element* aTo) {
      nsRefPtr<Element> from = get();
      Unlink();
      ElementChanged(from, aTo);
    }

  protected:
    virtual void ElementChanged(Element* aFrom, Element* aTo) MOZ_OVERRIDE
    {
      nsReferencedElement::ElementChanged(aFrom, aTo);
      mSpec->UpdateReferencedElement(aFrom, aTo);
    }
    virtual bool IsPersistent() MOZ_OVERRIDE { return true; }
  private:
    nsSMILTimeValueSpec* mSpec;
  };

  TimeReferenceElement mReferencedElement;

  class EventListener MOZ_FINAL : public nsIDOMEventListener
  {
    ~EventListener() {}
  public:
    explicit EventListener(nsSMILTimeValueSpec* aOwner) : mSpec(aOwner) { }
    void Disconnect()
    {
      mSpec = nullptr;
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMEVENTLISTENER

  private:
    nsSMILTimeValueSpec* mSpec;
  };
  nsRefPtr<EventListener> mEventListener;
};

#endif 






































#ifndef NS_SMILTIMEVALUESPEC_H_
#define NS_SMILTIMEVALUESPEC_H_

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












class nsSMILTimeValueSpec
{
public:
  typedef mozilla::dom::Element Element;

  nsSMILTimeValueSpec(nsSMILTimedElement& aOwner, PRBool aIsBegin);
  ~nsSMILTimeValueSpec();

  nsresult SetSpec(const nsAString& aStringSpec, Element* aContextNode);
  void     ResolveReferences(nsIContent* aContextNode);
  PRBool   IsEventBased() const;

  void     HandleNewInterval(nsSMILInterval& aInterval,
                             const nsSMILTimeContainer* aSrcContainer);
  void     HandleTargetElementChange(Element* aNewTarget);

  
  PRBool   DependsOnBegin() const;
  void     HandleChangedInstanceTime(const nsSMILInstanceTime& aBaseTime,
                                     const nsSMILTimeContainer* aSrcContainer,
                                     nsSMILInstanceTime& aInstanceTimeToUpdate,
                                     PRBool aObjectChanged);
  void     HandleDeletedInstanceTime(nsSMILInstanceTime& aInstanceTime);

  
  void Traverse(nsCycleCollectionTraversalCallback* aCallback);
  void Unlink();

protected:
  void UpdateReferencedElement(Element* aFrom, Element* aTo);
  void UnregisterFromReferencedElement(Element* aElement);
  nsSMILTimedElement* GetTimedElement(Element* aElement);
  void RegisterEventListener(Element* aElement);
  void UnregisterEventListener(Element* aElement);
  nsEventListenerManager* GetEventListenerManager(Element* aElement);
  void HandleEvent(nsIDOMEvent* aEvent);
  PRBool CheckEventDetail(nsIDOMEvent* aEvent);
  PRBool CheckRepeatEventDetail(nsIDOMEvent* aEvent);
  PRBool CheckAccessKeyEventDetail(nsIDOMEvent* aEvent);
  nsSMILTimeValue ConvertBetweenTimeContainers(const nsSMILTimeValue& aSrcTime,
                                      const nsSMILTimeContainer* aSrcContainer);

  nsSMILTimedElement*           mOwner;
  PRPackedBool                  mIsBegin; 
                                          
                                          
                                          
                                          
  nsSMILTimeValueSpecParams     mParams;

  class TimeReferenceElement : public nsReferencedElement
  {
  public:
    TimeReferenceElement(nsSMILTimeValueSpec* aOwner) : mSpec(aOwner) { }
    void ResetWithElement(Element* aTo) {
      nsRefPtr<Element> from = get();
      Unlink();
      ElementChanged(from, aTo);
    }

  protected:
    virtual void ElementChanged(Element* aFrom, Element* aTo)
    {
      nsReferencedElement::ElementChanged(aFrom, aTo);
      mSpec->UpdateReferencedElement(aFrom, aTo);
    }
    virtual PRBool IsPersistent() { return PR_TRUE; }
  private:
    nsSMILTimeValueSpec* mSpec;
  };

  TimeReferenceElement mReferencedElement;

  class EventListener : public nsIDOMEventListener
  {
  public:
    EventListener(nsSMILTimeValueSpec* aOwner) : mSpec(aOwner) { }
    void Disconnect()
    {
      mSpec = nsnull;
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMEVENTLISTENER

  private:
    nsSMILTimeValueSpec* mSpec;
  };
  nsCOMPtr<EventListener> mEventListener;
};

#endif 

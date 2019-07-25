




































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
  nsSMILTimeValueSpec(nsSMILTimedElement& aOwner, PRBool aIsBegin);
  ~nsSMILTimeValueSpec();

  nsresult SetSpec(const nsAString& aStringSpec, nsIContent* aContextNode);
  void     ResolveReferences(nsIContent* aContextNode);
  PRBool   IsEventBased() const;

  void     HandleNewInterval(nsSMILInterval& aInterval,
                             const nsSMILTimeContainer* aSrcContainer);

  
  PRBool   DependsOnBegin() const;
  void     HandleChangedInstanceTime(const nsSMILInstanceTime& aBaseTime,
                                     const nsSMILTimeContainer* aSrcContainer,
                                     nsSMILInstanceTime& aInstanceTimeToUpdate,
                                     PRBool aObjectChanged);
  void     HandleDeletedInstanceTime(nsSMILInstanceTime& aInstanceTime);

  
  void Traverse(nsCycleCollectionTraversalCallback* aCallback);
  void Unlink();

protected:
  void UpdateReferencedElement(nsIContent* aFrom, nsIContent* aTo);
  void UnregisterFromReferencedElement(nsIContent* aContent);
  nsSMILTimedElement* GetTimedElementFromContent(nsIContent* aContent);
  void RegisterEventListener(nsIContent* aTarget);
  void UnregisterEventListener(nsIContent* aTarget);
  nsIEventListenerManager* GetEventListenerManager(nsIContent* aTarget,
      nsIDOMEventGroup** aSystemGroup);
  void HandleEvent(nsIDOMEvent* aEvent);
  nsSMILTimeValue ConvertBetweenTimeContainers(const nsSMILTimeValue& aSrcTime,
                                      const nsSMILTimeContainer* aSrcContainer);

  nsSMILTimedElement*           mOwner;
  PRPackedBool                  mIsBegin; 
                                          
                                          
                                          
                                          
  nsSMILTimeValueSpecParams     mParams;

  class TimeReferenceElement : public nsReferencedElement
  {
  public:
    TimeReferenceElement(nsSMILTimeValueSpec* aOwner) : mSpec(aOwner) { }

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

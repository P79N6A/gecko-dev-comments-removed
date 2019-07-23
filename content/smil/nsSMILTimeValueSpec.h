




































#ifndef NS_SMILTIMEVALUESPEC_H_
#define NS_SMILTIMEVALUESPEC_H_

#include "nsSMILTimeValueSpecParams.h"
#include "nsReferencedElement.h"
#include "nsAutoPtr.h"

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
  void UpdateTimebase(nsIContent* aFrom, nsIContent* aTo);
  void UnregisterFromTimebase(nsSMILTimedElement* aTimedElement);
  nsSMILTimedElement* GetTimedElementFromContent(nsIContent* aContent);
  nsSMILTimedElement* GetTimebaseElement();
  nsSMILTimeValue ConvertBetweenTimeContainers(const nsSMILTimeValue& aSrcTime,
                                      const nsSMILTimeContainer* aSrcContainer);

  nsSMILTimedElement*           mOwner;
  PRPackedBool                  mIsBegin; 
                                          
                                          
                                          
                                          
  nsSMILTimeValueSpecParams     mParams;

  class TimebaseElement : public nsReferencedElement {
  public:
    TimebaseElement(nsSMILTimeValueSpec* aOwner) : mSpec(aOwner) { }

  protected:
    virtual void ContentChanged(nsIContent* aFrom, nsIContent* aTo) {
      nsReferencedElement::ContentChanged(aFrom, aTo);
      mSpec->UpdateTimebase(aFrom, aTo);
    }
    virtual PRBool IsPersistent() { return PR_TRUE; }
  private:
    nsSMILTimeValueSpec* mSpec;
  };

  TimebaseElement mTimebase;
};

#endif 

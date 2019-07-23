




































#ifndef NS_SMILTIMEDELEMENT_H_
#define NS_SMILTIMEDELEMENT_H_

#include "nsSMILInterval.h"
#include "nsSMILInstanceTime.h"
#include "nsSMILMilestone.h"
#include "nsSMILTimeValueSpec.h"
#include "nsSMILRepeatCount.h"
#include "nsSMILTypes.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsAttrValue.h"

class nsISMILAnimationElement;
class nsSMILAnimationFunction;
class nsSMILTimeContainer;
class nsSMILTimeValue;
class nsIAtom;




class nsSMILTimedElement
{
public:
  nsSMILTimedElement();

  



  void SetAnimationElement(nsISMILAnimationElement* aElement);

  



  nsSMILTimeContainer* GetTimeContainer();

  



  







  nsresult BeginElementAt(double aOffsetSeconds);

  







  nsresult EndElementAt(double aOffsetSeconds);

  



  








  nsSMILTimeValue GetStartTime() const;

  




  nsSMILTimeValue GetSimpleDuration() const
  {
    return mSimpleDur;
  }

  



  









  void AddInstanceTime(const nsSMILInstanceTime& aInstanceTime,
                       PRBool aIsBegin);

  











  void SetTimeClient(nsSMILAnimationFunction* aClient);

  






  void SampleAt(nsSMILTime aContainerTime);

  











  void SampleEndAt(nsSMILTime aContainerTime);

  



  void Reset();

  







  void HardReset();

  
















  PRBool SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                 nsAttrValue& aResult, nsresult* aParseResult = nsnull);

  









  PRBool UnsetAttr(nsIAtom* aAttribute);

  


  void BindToTree();

protected:
  
  
  

  nsresult          SetBeginSpec(const nsAString& aBeginSpec);
  nsresult          SetEndSpec(const nsAString& aEndSpec);
  nsresult          SetSimpleDuration(const nsAString& aDurSpec);
  nsresult          SetMin(const nsAString& aMinSpec);
  nsresult          SetMax(const nsAString& aMaxSpec);
  nsresult          SetRestart(const nsAString& aRestartSpec);
  nsresult          SetRepeatCount(const nsAString& aRepeatCountSpec);
  nsresult          SetRepeatDur(const nsAString& aRepeatDurSpec);
  nsresult          SetFillMode(const nsAString& aFillModeSpec);

  void              UnsetBeginSpec();
  void              UnsetEndSpec();
  void              UnsetSimpleDuration();
  void              UnsetMin();
  void              UnsetMax();
  void              UnsetRestart();
  void              UnsetRepeatCount();
  void              UnsetRepeatDur();
  void              UnsetFillMode();

  nsresult          SetBeginOrEndSpec(const nsAString& aSpec, PRBool aIsBegin);
  void              DoSampleAt(nsSMILTime aContainerTime, PRBool aEndOnly);

  




  nsresult          GetNextInterval(const nsSMILInterval* aPrevInterval,
                                    nsSMILInterval& aResult);
  PRBool            GetNextGreater(const nsTArray<nsSMILInstanceTime>& aList,
                                   const nsSMILTimeValue& aBase,
                                   PRInt32& aPosition,
                                   nsSMILTimeValue& aResult) const;
  PRBool            GetNextGreaterOrEqual(
                                   const nsTArray<nsSMILInstanceTime>& aList,
                                   const nsSMILTimeValue& aBase,
                                   PRInt32& aPosition,
                                   nsSMILTimeValue& aResult) const;
  nsSMILTimeValue   CalcActiveEnd(const nsSMILTimeValue& aBegin,
                                  const nsSMILTimeValue& aEnd) const;
  nsSMILTimeValue   GetRepeatDuration() const;
  nsSMILTimeValue   ApplyMinAndMax(const nsSMILTimeValue& aDuration) const;
  nsSMILTime        ActiveTimeToSimpleTime(nsSMILTime aActiveTime,
                                           PRUint32& aRepeatIteration);
  nsSMILTimeValue   CheckForEarlyEnd(
                        const nsSMILTimeValue& aContainerTime) const;
  void              UpdateCurrentInterval();
  void              SampleSimpleTime(nsSMILTime aActiveTime);
  void              SampleFillValue();
  void              AddInstanceTimeFromCurrentTime(nsSMILTime aCurrentTime,
                        double aOffsetSeconds, PRBool aIsBegin);
  void              RegisterMilestone();
  PRBool            GetNextMilestone(nsSMILMilestone& aNextMilestone) const;

  
  typedef nsTArray<nsRefPtr<nsSMILTimeValueSpec> >  SMILTimeValueSpecList;

  
  
  
  nsISMILAnimationElement* mAnimationElement; 
  SMILTimeValueSpecList mBeginSpecs;
  SMILTimeValueSpecList mEndSpecs;

  nsSMILTimeValue                 mSimpleDur;

  nsSMILRepeatCount               mRepeatCount;
  nsSMILTimeValue                 mRepeatDur;

  nsSMILTimeValue                 mMin;
  nsSMILTimeValue                 mMax;

  enum nsSMILFillMode
  {
    FILL_REMOVE,
    FILL_FREEZE
  };
  nsSMILFillMode                  mFillMode;
  static nsAttrValue::EnumTable   sFillModeTable[];

  enum nsSMILRestartMode
  {
    RESTART_ALWAYS,
    RESTART_WHENNOTACTIVE,
    RESTART_NEVER
  };
  nsSMILRestartMode               mRestartMode;
  static nsAttrValue::EnumTable   sRestartModeTable[];

  
  
  
  
  
  
  PRPackedBool                    mBeginSpecSet;

  PRPackedBool                    mEndHasEventConditions;

  nsTArray<nsSMILInstanceTime>    mBeginInstances;
  nsTArray<nsSMILInstanceTime>    mEndInstances;

  nsSMILAnimationFunction*        mClient;
  nsSMILInterval                  mCurrentInterval;
  nsTArray<nsSMILInterval>        mOldIntervals;
  nsSMILMilestone                 mPrevRegisteredMilestone;
  static const nsSMILMilestone    sMaxMilestone;

  



  enum nsSMILElementState
  {
    STATE_STARTUP,
    STATE_WAITING,
    STATE_ACTIVE,
    STATE_POSTACTIVE
  };
  nsSMILElementState              mElementState;
};

#endif 






































#ifndef NS_SMILTIMEDELEMENT_H_
#define NS_SMILTIMEDELEMENT_H_

#include "nsSMILInterval.h"
#include "nsSMILInstanceTime.h"
#include "nsSMILTimeValueSpec.h"
#include "nsSMILRepeatCount.h"
#include "nsSMILTypes.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsAttrValue.h"

class nsSMILAnimationFunction;
class nsSMILTimeContainer;
class nsSMILTimeValue;
class nsIAtom;




class nsSMILTimedElement
{
public:
  nsSMILTimedElement();

  



  










  nsresult BeginElementAt(double aOffsetSeconds,
                          const nsSMILTimeContainer* aContainer);

  










  nsresult EndElementAt(double aOffsetSeconds,
                        const nsSMILTimeContainer* aContainer);

  



  








  nsSMILTimeValue GetStartTime() const;

  




  nsSMILTimeValue GetSimpleDuration() const
  {
    return mSimpleDur;
  }

  



  










  void AddInstanceTime(const nsSMILInstanceTime& aInstanceTime,
                       PRBool aIsBegin);

  











  void SetTimeClient(nsSMILAnimationFunction* aClient);

  






  void SampleAt(nsSMILTime aDocumentTime);

  



  void Reset();

  







  void HardReset();

  
















  PRBool SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                 nsAttrValue& aResult, nsresult* aParseResult = nsnull);

  









  PRBool UnsetAttr(nsIAtom* aAttribute);

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

  




  nsresult          GetNextInterval(const nsSMILInterval* aPrevInterval,
                                    nsSMILInterval& aResult);
  PRBool            GetNextGreater(const nsTArray<nsSMILInstanceTime>& aList,
                                   const nsSMILTimeValue& aBase,
                                   PRInt32& aPosition,
                                   nsSMILTimeValue& aResult);
  PRBool            GetNextGreaterOrEqual(
                                   const nsTArray<nsSMILInstanceTime>& aList,
                                   const nsSMILTimeValue& aBase,
                                   PRInt32& aPosition,
                                   nsSMILTimeValue& aResult);
  nsSMILTimeValue   CalcActiveEnd(const nsSMILTimeValue& aBegin,
                                  const nsSMILTimeValue& aEnd);
  nsSMILTimeValue   GetRepeatDuration();
  nsSMILTimeValue   ApplyMinAndMax(const nsSMILTimeValue& aDuration);
  nsSMILTime        ActiveTimeToSimpleTime(nsSMILTime aActiveTime,
                                           PRUint32& aRepeatIteration);
  void              CheckForEarlyEnd(const nsSMILTimeValue& aDocumentTime);
  void              UpdateCurrentInterval();
  void              SampleSimpleTime(nsSMILTime aActiveTime);
  void              SampleFillValue();
  void              AddInstanceTimeFromCurrentTime(nsSMILTime aCurrentTime,
                        double aOffsetSeconds, PRBool aIsBegin);

  
  typedef nsTArray<nsRefPtr<nsSMILTimeValueSpec> >  SMILTimeValueSpecList;

  
  
  
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

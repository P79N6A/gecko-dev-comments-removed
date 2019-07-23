




































#ifndef NS_SMILTIMECONTAINER_H_
#define NS_SMILTIMECONTAINER_H_

#include "nscore.h"
#include "nsSMILTypes.h"






class nsSMILTimeContainer
{
public:
  nsSMILTimeContainer();
  virtual ~nsSMILTimeContainer();

  


  enum {
    PAUSE_BEGIN    = 1,
    PAUSE_SCRIPT   = 2,
    PAUSE_PAGEHIDE = 4,
    PAUSE_USERPREF = 8
  };

  


  void Begin();

  







  virtual void Pause(PRUint32 aType);

  






  virtual void Resume(PRUint32 aType);

  







  PRBool IsPausedByType(PRUint32 aType) const { return mPauseState & aType; }

  



  nsSMILTime GetCurrentTime() const;

  





  void SetCurrentTime(nsSMILTime aSeekTo);

  


  virtual nsSMILTime GetParentTime() const;

  



  void Sample();

  





  PRBool NeedsSample() const { return !mPauseState || mNeedsPauseSample; }

  




  nsresult SetParent(nsSMILTimeContainer* aParent);

protected:
  



  virtual void DoSample() { }

  




  


  virtual nsresult AddChild(nsSMILTimeContainer& aChild)
  {
    return NS_ERROR_FAILURE;
  }

  


  virtual void RemoveChild(nsSMILTimeContainer& aChild) { }

  


  void UpdateCurrentTime();

  
  nsSMILTimeContainer* mParent;

  
  nsSMILTime mCurrentTime;

  
  
  
  
  
  
  nsSMILTime mParentOffset;

  
  
  nsSMILTime mPauseStart;

  
  PRPackedBool mNeedsPauseSample;

  
  PRUint32 mPauseState;
};

#endif 

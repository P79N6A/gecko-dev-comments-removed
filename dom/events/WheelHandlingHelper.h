





#ifndef mozilla_WheelHandlingHelper_h_
#define mozilla_WheelHandlingHelper_h_

#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "nsCoord.h"
#include "nsIFrame.h"
#include "nsPoint.h"

class nsIScrollableFrame;
class nsITimer;

namespace mozilla {

class EventStateManager;






struct DeltaValues
{
  DeltaValues()
    : deltaX(0.0)
    , deltaY(0.0)
  {
  }

  DeltaValues(double aDeltaX, double aDeltaY)
    : deltaX(aDeltaX)
    , deltaY(aDeltaY)
  {
  }

  explicit DeltaValues(WidgetWheelEvent* aEvent);

  double deltaX;
  double deltaY;
};






class WheelHandlingUtils
{
public:
  



  static bool CanScrollOn(nsIScrollableFrame* aScrollFrame,
                          double aDirectionX, double aDirectionY);

private:
  static bool CanScrollInRange(nscoord aMin, nscoord aValue, nscoord aMax,
                               double aDirection);
};








class ScrollbarsForWheel
{
public:
  static void PrepareToScrollText(EventStateManager* aESM,
                                  nsIFrame* aTargetFrame,
                                  WidgetWheelEvent* aEvent);
  static void SetActiveScrollTarget(nsIScrollableFrame* aScrollTarget);
  
  static void MayInactivate();
  static void Inactivate();
  static bool IsActive();
  static void OwnWheelTransaction(bool aOwn);

protected:
  static const size_t kNumberOfTargets = 4;
  static const DeltaValues directions[kNumberOfTargets];
  static nsWeakFrame sActiveOwner;
  static nsWeakFrame sActivatedScrollTargets[kNumberOfTargets];
  static bool sHadWheelStart;
  static bool sOwnWheelTransaction;


  



  static void TemporarilyActivateAllPossibleScrollTargets(
                EventStateManager* aESM,
                nsIFrame* aTargetFrame,
                WidgetWheelEvent* aEvent);
  static void DeactivateAllTemporarilyActivatedScrollTargets();
};









class WheelTransaction
{
public:
  static nsIFrame* GetTargetFrame() { return sTargetFrame; }
  static void BeginTransaction(nsIFrame* aTargetFrame,
                               WidgetWheelEvent* aEvent);
  
  
  static bool UpdateTransaction(WidgetWheelEvent* aEvent);
  static void MayEndTransaction();
  static void EndTransaction();
  static void OnEvent(WidgetEvent* aEvent);
  static void Shutdown();
  static uint32_t GetTimeoutTime();

  static void OwnScrollbars(bool aOwn);

  static DeltaValues AccelerateWheelDelta(WidgetWheelEvent* aEvent,
                                          bool aAllowScrollSpeedOverride);

protected:
  static const uint32_t kScrollSeriesTimeout = 80; 
  static nsIntPoint GetScreenPoint(WidgetGUIEvent* aEvent);
  static void OnFailToScrollTarget();
  static void OnTimeout(nsITimer* aTimer, void* aClosure);
  static void SetTimeout();
  static uint32_t GetIgnoreMoveDelayTime();
  static int32_t GetAccelerationStart();
  static int32_t GetAccelerationFactor();
  static DeltaValues OverrideSystemScrollSpeed(WidgetWheelEvent* aEvent);
  static double ComputeAcceleratedWheelDelta(double aDelta, int32_t aFactor);
  static bool OutOfTime(uint32_t aBaseTime, uint32_t aThreshold);

  static nsWeakFrame sTargetFrame;
  static uint32_t sTime; 
  static uint32_t sMouseMoved; 
  static nsITimer* sTimer;
  static int32_t sScrollSeriesCounter;
  static bool sOwnScrollbars;
};

} 

#endif 

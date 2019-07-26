

















#ifndef ProcessOrientation_h
#define ProcessOrientation_h

#include "mozilla/Hal.h"

namespace mozilla {


#define TILT_HISTORY_SIZE 40

class ProcessOrientation {
public:
  ProcessOrientation() {};
  ~ProcessOrientation() {};

  int OnSensorChanged(const mozilla::hal::SensorData& event, int deviceCurrentRotation);
  int Reset();

private:
  int GetProposedRotation();

  
  
  bool IsTiltAngleAcceptable(int rotation, int tiltAngle);

  
  
  
  bool IsOrientationAngleAcceptable(int rotation, int orientationAngle,
                                    int currentRotation);

  
  
  bool IsPredictedRotationAcceptable(long now);

  void ClearPredictedRotation();
  void UpdatePredictedRotation(long now, int rotation);
  bool IsAccelerating(float magnitude);
  void ClearTiltHistory();
  void AddTiltHistoryEntry(long now, float tilt);
  bool IsFlat(long now);
  bool IsSwinging(long now, float tilt);
  int NextTiltHistoryIndex(int index);
  float RemainingMS(long now, long until);

  
  
  
  
  
  
  
  
  
  
  static const int tiltTolerance[][4];

  
  long mLastFilteredTimestampNanos;
  float mLastFilteredX, mLastFilteredY, mLastFilteredZ;

  
  int mProposedRotation;

  
  int mPredictedRotation;

  
  long mPredictedRotationTimestampNanos;

  
  
  long mFlatTimestampNanos;

  
  long mSwingTimestampNanos;

  
  
  long mAccelerationTimestampNanos;

  struct {
    struct {
      float tiltAngle;
      long timestampNanos;
    } history[TILT_HISTORY_SIZE];
    int index;
  } mTiltHistory;
};

} 
#endif

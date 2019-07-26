

















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

  
  
  bool IsPredictedRotationAcceptable(int64_t now);

  void ClearPredictedRotation();
  void UpdatePredictedRotation(int64_t now, int rotation);
  bool IsAccelerating(float magnitude);
  void ClearTiltHistory();
  void AddTiltHistoryEntry(int64_t now, float tilt);
  bool IsFlat(int64_t now);
  bool IsSwinging(int64_t now, float tilt);
  int NextTiltHistoryIndex(int index);
  float RemainingMS(int64_t now, int64_t until);

  
  
  
  
  
  
  
  
  
  
  static const int tiltTolerance[][4];

  
  int64_t mLastFilteredTimestampNanos;
  float mLastFilteredX, mLastFilteredY, mLastFilteredZ;

  
  int mProposedRotation;

  
  int mPredictedRotation;

  
  int64_t mPredictedRotationTimestampNanos;

  
  
  int64_t mFlatTimestampNanos;

  
  int64_t mSwingTimestampNanos;

  
  
  int64_t mAccelerationTimestampNanos;

  struct {
    struct {
      float tiltAngle;
      int64_t timestampNanos;
    } history[TILT_HISTORY_SIZE];
    int index;
  } mTiltHistory;
};

} 
#endif

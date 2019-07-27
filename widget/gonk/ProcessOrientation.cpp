

















#include "base/basictypes.h"
#include "mozilla/Hal.h"
#include "mozilla/unused.h"
#include "nsIScreen.h"
#include "nsIScreenManager.h"
#include "OrientationObserver.h"
#include "ProcessOrientation.h"
#include "mozilla/HalSensor.h"
#include "math.h"
#include "limits.h"
#include "android/log.h"

#if 0
#define LOGD(args...)  __android_log_print(ANDROID_LOG_DEBUG, "ProcessOrientation" , ## args)
#else
#define LOGD(args...)
#endif

namespace mozilla {


#define RADIANS_TO_DEGREES (180/M_PI)


#define NANOS_PER_MS 1000000


#define ACCELEROMETER_DATA_X 0
#define ACCELEROMETER_DATA_Y 1
#define ACCELEROMETER_DATA_Z 2






#define PROPOSAL_SETTLE_TIME_NANOS (40*NANOS_PER_MS)




#define PROPOSAL_MIN_TIME_SINCE_FLAT_ENDED_NANOS (500*NANOS_PER_MS)




#define PROPOSAL_MIN_TIME_SINCE_SWING_ENDED_NANOS (300*NANOS_PER_MS)



#define PROPOSAL_MIN_TIME_SINCE_ACCELERATION_ENDED_NANOS (500*NANOS_PER_MS)




#define FLAT_ANGLE 75
#define FLAT_TIME_NANOS (1000*NANOS_PER_MS)




#define SWING_AWAY_ANGLE_DELTA 20
#define SWING_TIME_NANOS (300*NANOS_PER_MS)






#define MAX_FILTER_DELTA_TIME_NANOS (1000*NANOS_PER_MS)
























#define FILTER_TIME_CONSTANT_MS 200.0f




















#define NEAR_ZERO_MAGNITUDE 1 // m/s^2
#define ACCELERATION_TOLERANCE 4 // m/s^2
#define STANDARD_GRAVITY 9.80665f
#define MIN_ACCELERATION_MAGNITUDE (STANDARD_GRAVITY-ACCELERATION_TOLERANCE)
#define MAX_ACCELERATION_MAGNITUDE (STANDARD_GRAVITY+ACCELERATION_TOLERANCE)




#define MAX_TILT 75






#define ADJACENT_ORIENTATION_ANGLE_GAP 45

const int
ProcessOrientation::tiltTolerance[][4] = {
  {-25, 70}, 
  {-25, 65}, 
  {-25, 60}, 
  {-25, 65}  
};

int
ProcessOrientation::GetProposedRotation()
{
  return mProposedRotation;
}

int
ProcessOrientation::OnSensorChanged(const SensorData& event,
                                    int deviceCurrentRotation)
{
  
  
  
  const InfallibleTArray<float>& values = event.values();
  float x = values[ACCELEROMETER_DATA_X];
  float y = values[ACCELEROMETER_DATA_Y];
  float z = values[ACCELEROMETER_DATA_Z];

  LOGD
    ("ProcessOrientation: Raw acceleration vector: x = %f, y = %f, z = %f,"
     "magnitude = %f\n", x, y, z, sqrt(x * x + y * y + z * z));
  
  
  
  
  const int64_t now = (int64_t) event.timestamp();
  const int64_t then = mLastFilteredTimestampNanos;
  const float timeDeltaMS = (now - then) * 0.000001f;
  bool skipSample = false;
  if (now < then
      || now > then + MAX_FILTER_DELTA_TIME_NANOS
      || (x == 0 && y == 0 && z == 0)) {
    LOGD
      ("ProcessOrientation: Resetting orientation listener.");
    Reset();
    skipSample = true;
  } else {
    const float alpha = timeDeltaMS / (FILTER_TIME_CONSTANT_MS + timeDeltaMS);
    x = alpha * (x - mLastFilteredX) + mLastFilteredX;
    y = alpha * (y - mLastFilteredY) + mLastFilteredY;
    z = alpha * (z - mLastFilteredZ) + mLastFilteredZ;
    LOGD
      ("ProcessOrientation: Filtered acceleration vector: x=%f, y=%f, z=%f,"
       "magnitude=%f", z, y, z, sqrt(x * x + y * y + z * z));
    skipSample = false;
  }
  mLastFilteredTimestampNanos = now;
  mLastFilteredX = x;
  mLastFilteredY = y;
  mLastFilteredZ = z;

  bool isAccelerating = false;
  bool isFlat = false;
  bool isSwinging = false;
  if (skipSample) {
    return -1;
  }

  
  const float magnitude = sqrt(x * x + y * y + z * z);
  if (magnitude < NEAR_ZERO_MAGNITUDE) {
    LOGD
      ("ProcessOrientation: Ignoring sensor data, magnitude too close to"
       " zero.");
    ClearPredictedRotation();
  } else {
    
    
    if (this->IsAccelerating(magnitude)) {
      isAccelerating = true;
      mAccelerationTimestampNanos = now;
    }
    
    
    
    
    
    
    const int tiltAngle =
      static_cast<int>(roundf(asin(z / magnitude) * RADIANS_TO_DEGREES));
    AddTiltHistoryEntry(now, tiltAngle);

    
    if (this->IsFlat(now)) {
      isFlat = true;
      mFlatTimestampNanos = now;
    }
    if (this->IsSwinging(now, tiltAngle)) {
      isSwinging = true;
      mSwingTimestampNanos = now;
    }
    
    
    if (abs(tiltAngle) > MAX_TILT) {
      LOGD
        ("ProcessOrientation: Ignoring sensor data, tilt angle too high:"
         " tiltAngle=%d", tiltAngle);
      ClearPredictedRotation();
    } else {
      
      
      
      int orientationAngle =
        static_cast<int>(roundf(-atan2f(-x, y) * RADIANS_TO_DEGREES));
      if (orientationAngle < 0) {
        
        orientationAngle += 360;
      }
      
      int nearestRotation = (orientationAngle + 45) / 90;
      if (nearestRotation == 4) {
        nearestRotation = 0;
      }
      
      if (IsTiltAngleAcceptable(nearestRotation, tiltAngle)
          &&
          IsOrientationAngleAcceptable
          (nearestRotation, orientationAngle, deviceCurrentRotation)) {
        UpdatePredictedRotation(now, nearestRotation);
        LOGD
          ("ProcessOrientation: Predicted: tiltAngle=%d, orientationAngle=%d,"
           " predictedRotation=%d, predictedRotationAgeMS=%f",
           tiltAngle,
           orientationAngle,
           mPredictedRotation,
           ((now - mPredictedRotationTimestampNanos) * 0.000001f));
      } else {
        LOGD
          ("ProcessOrientation: Ignoring sensor data, no predicted rotation:"
           " tiltAngle=%d, orientationAngle=%d",
           tiltAngle,
           orientationAngle);
        ClearPredictedRotation();
      }
    }
  }

  
  const int oldProposedRotation = mProposedRotation;
  if (mPredictedRotation < 0 || IsPredictedRotationAcceptable(now)) {
    mProposedRotation = mPredictedRotation;
  }
  
  
  LOGD
    ("ProcessOrientation: Result: oldProposedRotation=%d,currentRotation=%d, "
     "proposedRotation=%d, predictedRotation=%d, timeDeltaMS=%f, "
     "isAccelerating=%d, isFlat=%d, isSwinging=%d, timeUntilSettledMS=%f, "
     "timeUntilAccelerationDelayExpiredMS=%f, timeUntilFlatDelayExpiredMS=%f, "
     "timeUntilSwingDelayExpiredMS=%f",
     oldProposedRotation,
     deviceCurrentRotation, mProposedRotation,
     mPredictedRotation, timeDeltaMS, isAccelerating, isFlat,
     isSwinging, RemainingMS(now,
                             mPredictedRotationTimestampNanos +
                             PROPOSAL_SETTLE_TIME_NANOS),
     RemainingMS(now,
                 mAccelerationTimestampNanos +
                 PROPOSAL_MIN_TIME_SINCE_ACCELERATION_ENDED_NANOS),
     RemainingMS(now,
                 mFlatTimestampNanos +
                 PROPOSAL_MIN_TIME_SINCE_FLAT_ENDED_NANOS),
     RemainingMS(now,
                 mSwingTimestampNanos +
                 PROPOSAL_MIN_TIME_SINCE_SWING_ENDED_NANOS));

  
  
  unused << isAccelerating;
  unused << isFlat;
  unused << isSwinging;

  
  if (mProposedRotation != oldProposedRotation && mProposedRotation >= 0) {
    LOGD
      ("ProcessOrientation: Proposed rotation changed!  proposedRotation=%d, "
       "oldProposedRotation=%d",
       mProposedRotation,
       oldProposedRotation);
    return mProposedRotation;
  }
  
  return -1;
}

bool
ProcessOrientation::IsTiltAngleAcceptable(int rotation, int tiltAngle)
{
  return (tiltAngle >= tiltTolerance[rotation][0]
          && tiltAngle <= tiltTolerance[rotation][1]);
}

bool
ProcessOrientation::IsOrientationAngleAcceptable(int rotation,
                                                 int orientationAngle,
                                                 int currentRotation)
{
  
  
  
  if (currentRotation < 0) {
    return true;
  }
  
  
  
  
  if (rotation == currentRotation || rotation == (currentRotation + 1) % 4) {
    int lowerBound = rotation * 90 - 45 + ADJACENT_ORIENTATION_ANGLE_GAP / 2;
    if (rotation == 0) {
      if (orientationAngle >= 315 && orientationAngle < lowerBound + 360) {
        return false;
      }
    } else {
      if (orientationAngle < lowerBound) {
        return false;
      }
    }
  }
  
  
  
  
  if (rotation == currentRotation || rotation == (currentRotation + 3) % 4) {
    int upperBound = rotation * 90 + 45 - ADJACENT_ORIENTATION_ANGLE_GAP / 2;
    if (rotation == 0) {
      if (orientationAngle <= 45 && orientationAngle > upperBound) {
        return false;
      }
    } else {
      if (orientationAngle > upperBound) {
        return false;
      }
    }
  }
  return true;
}

bool
ProcessOrientation::IsPredictedRotationAcceptable(int64_t now)
{
  
  if (now < mPredictedRotationTimestampNanos + PROPOSAL_SETTLE_TIME_NANOS) {
    return false;
  }
  
  
  if (now < mFlatTimestampNanos + PROPOSAL_MIN_TIME_SINCE_FLAT_ENDED_NANOS) {
    return false;
  }
  
  
  if (now < mSwingTimestampNanos + PROPOSAL_MIN_TIME_SINCE_SWING_ENDED_NANOS) {
    return false;
  }
  
  if (now < mAccelerationTimestampNanos
      + PROPOSAL_MIN_TIME_SINCE_ACCELERATION_ENDED_NANOS) {
    return false;
  }
  
  return true;
}

int
ProcessOrientation::Reset()
{
  mLastFilteredTimestampNanos = std::numeric_limits<int64_t>::min();
  mProposedRotation = -1;
  mFlatTimestampNanos = std::numeric_limits<int64_t>::min();
  mSwingTimestampNanos = std::numeric_limits<int64_t>::min();
  mAccelerationTimestampNanos = std::numeric_limits<int64_t>::min();
  ClearPredictedRotation();
  ClearTiltHistory();
  return -1;
}

void
ProcessOrientation::ClearPredictedRotation()
{
  mPredictedRotation = -1;
  mPredictedRotationTimestampNanos = std::numeric_limits<int64_t>::min();
}

void
ProcessOrientation::UpdatePredictedRotation(int64_t now, int rotation)
{
  if (mPredictedRotation != rotation) {
    mPredictedRotation = rotation;
    mPredictedRotationTimestampNanos = now;
  }
}

bool
ProcessOrientation::IsAccelerating(float magnitude)
{
  return magnitude < MIN_ACCELERATION_MAGNITUDE
    || magnitude > MAX_ACCELERATION_MAGNITUDE;
}

void
ProcessOrientation::ClearTiltHistory()
{
  mTiltHistory.history[0].timestampNanos = std::numeric_limits<int64_t>::min();
  mTiltHistory.index = 1;
}

void
ProcessOrientation::AddTiltHistoryEntry(int64_t now, float tilt)
{
  mTiltHistory.history[mTiltHistory.index].tiltAngle = tilt;
  mTiltHistory.history[mTiltHistory.index].timestampNanos = now;
  mTiltHistory.index = (mTiltHistory.index + 1) % TILT_HISTORY_SIZE;
  mTiltHistory.history[mTiltHistory.index].timestampNanos = std::numeric_limits<int64_t>::min();
}

bool
ProcessOrientation::IsFlat(int64_t now)
{
  for (int i = mTiltHistory.index; (i = NextTiltHistoryIndex(i)) >= 0;) {
    if (mTiltHistory.history[i].tiltAngle < FLAT_ANGLE) {
      break;
    }
    if (mTiltHistory.history[i].timestampNanos + FLAT_TIME_NANOS <= now) {
      
      return true;
    }
  }
  return false;
}

bool
ProcessOrientation::IsSwinging(int64_t now, float tilt)
{
  for (int i = mTiltHistory.index; (i = NextTiltHistoryIndex(i)) >= 0;) {
    if (mTiltHistory.history[i].timestampNanos + SWING_TIME_NANOS < now) {
      break;
    }
    if (mTiltHistory.history[i].tiltAngle + SWING_AWAY_ANGLE_DELTA <= tilt) {
      
      return true;
    }
  }
  return false;
}

int
ProcessOrientation::NextTiltHistoryIndex(int index)
{
  index = (index == 0 ? TILT_HISTORY_SIZE : index) - 1;
  return mTiltHistory.history[index].timestampNanos != std::numeric_limits<int64_t>::min() ? index : -1;
}

float
ProcessOrientation::RemainingMS(int64_t now, int64_t until)
{
  return now >= until ? 0 : (until - now) * 0.000001f;
}

} 

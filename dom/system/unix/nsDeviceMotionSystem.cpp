




































#include <unistd.h>
#include "nsDeviceMotionSystem.h"
#include "nsIServiceManager.h"

typedef struct {
  const char* mPosition;
  const char* mCalibrate;
  nsAccelerometerSystemDriver mToken;
} AccelerometerData;

static const AccelerometerData gAccelerometers[] = {
  
  {"/sys/devices/platform/applesmc.768/position",
   "/sys/devices/platform/applesmc.768/calibrate",
   eAppleSensor},
  
  {"/sys/devices/platform/hdaps/position",
   "/sys/devices/platform/hdaps/calibrate",
   eIBMSensor},
  
  {"/sys/class/i2c-adapter/i2c-3/3-001d/coord",
   NULL,
   eMaemoSensor},
  
  {"/sys/devices/platform/lis3lv02d/position",
   "/sys/devices/platform/lis3lv02d/calibrate",
   eHPdv7Sensor},
};

nsDeviceMotionSystem::nsDeviceMotionSystem() :
  mPositionFile(NULL),
  mCalibrateFile(NULL),
  mType(eNoSensor)
{
}

nsDeviceMotionSystem::~nsDeviceMotionSystem()
{
}

void
nsDeviceMotionSystem::UpdateHandler(nsITimer *aTimer, void *aClosure)
{
  nsDeviceMotionSystem *self = reinterpret_cast<nsDeviceMotionSystem *>(aClosure);
  if (!self) {
    NS_ERROR("no self");
    return;
  }

  float xf, yf, zf;

  switch (self->mType) {
    case eAppleSensor:
    {
      int x, y, z, calibrate_x, calibrate_y;
      fflush(self->mCalibrateFile);
      rewind(self->mCalibrateFile);

      fflush(self->mPositionFile);
      rewind(self->mPositionFile);

      if (fscanf(self->mCalibrateFile, "(%d, %d)", &calibrate_x, &calibrate_y) <= 0)
        return;

      if (fscanf(self->mPositionFile, "(%d, %d, %d)", &x, &y, &z) <= 0)
        return;

      
      
      
      
      xf = ((float)(x + calibrate_x)) / 255.0;
      yf = ((float)(y - calibrate_y)) / 255.0;
      zf = ((float)z) / -255.0;
      break;
    }
    case eIBMSensor:
    {
      int x, y, calibrate_x, calibrate_y;
      fflush(self->mCalibrateFile);
      rewind(self->mCalibrateFile);

      fflush(self->mPositionFile);
      rewind(self->mPositionFile);

      if (fscanf(self->mCalibrateFile, "(%d, %d)", &calibrate_x, &calibrate_y) <= 0)
        return;

      if (fscanf(self->mPositionFile, "(%d, %d)", &x, &y) <= 0)
        return;

      xf = ((float)(x - calibrate_x)) / 180.0;
      yf = ((float)(y - calibrate_y)) / 180.0;
      zf = 1.0f;
      break;
    }
    case eMaemoSensor:
    {
      int x, y, z;
      fflush(self->mPositionFile);
      rewind(self->mPositionFile);

      if (fscanf(self->mPositionFile, "%d %d %d", &x, &y, &z) <= 0)
        return;

      xf = ((float)x) / -1000.0;
      yf = ((float)y) / -1000.0;
      zf = ((float)z) / -1000.0;
      break;
    }
    case eHPdv7Sensor:
    {
      int x, y, z, calibrate_x, calibrate_y, calibrate_z;
      fflush(self->mCalibrateFile);
      rewind(self->mCalibrateFile);

      fflush(self->mPositionFile);
      rewind(self->mPositionFile);

      if (fscanf(self->mCalibrateFile, "(%d,%d,%d)", &calibrate_x, &calibrate_y, &calibrate_z) <= 0)
        return;

      if (fscanf(self->mPositionFile, "(%d,%d,%d)", &x, &y, &z) <= 0)
        return;

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      xf = ((float)(x - calibrate_x)) / 60.0;
      yf = ((float)(y - calibrate_y)) / 60.0;
      zf = ((float)(z)) / 60.0;
      break;
    }

    case eNoSensor:
    default:
      return;
  }

  self->DeviceMotionChanged(nsIDeviceMotionData::TYPE_ACCELERATION, xf, yf, zf );
}

void nsDeviceMotionSystem::Startup()
{
  
  
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(gAccelerometers); i++) {
    if (!(mPositionFile = fopen(gAccelerometers[i].mPosition, "r")))
      continue;

    mType = gAccelerometers[i].mToken;
    if (gAccelerometers[i].mCalibrate) {
      mCalibrateFile = fopen(gAccelerometers[i].mCalibrate, "r");
      if (!mCalibrateFile) {
        fclose(mPositionFile);
        mPositionFile = nsnull;
        return;
      }
    }

    break;
  }

  if (mType == eNoSensor)
    return;

  mUpdateTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mUpdateTimer)
    mUpdateTimer->InitWithFuncCallback(UpdateHandler,
                                       this,
                                       mUpdateInterval,
                                       nsITimer::TYPE_REPEATING_SLACK);
}

void nsDeviceMotionSystem::Shutdown()
{
  if (mPositionFile) {
    fclose(mPositionFile);
    mPositionFile = nsnull;
  }

  
  
  if (mCalibrateFile) {
    fclose(mCalibrateFile);
    mCalibrateFile = nsnull;
  }

  mType = eNoSensor;

  if (mUpdateTimer) {
    mUpdateTimer->Cancel();
    mUpdateTimer = nsnull;
  }
}


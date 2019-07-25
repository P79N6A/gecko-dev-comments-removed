




































#include "nsDeviceMotionSystem.h"
#include "nsIServiceManager.h"
#include "windows.h"

#define DEFAULT_SENSOR_POLL 100





typedef struct {
  int status; 
  unsigned short x; 
  unsigned short y; 
  unsigned short xx; 
  unsigned short yy; 
  char temp; 
  unsigned short x0; 
  unsigned short y0; 
} ThinkPadAccelerometerData;

typedef void (__stdcall *ShockproofGetAccelerometerData)(ThinkPadAccelerometerData*);

ShockproofGetAccelerometerData gShockproofGetAccelerometerData = nsnull;

class ThinkPadSensor : public Sensor
{
public:
  ThinkPadSensor();
  ~ThinkPadSensor();
  bool Startup();
  void Shutdown();
  void GetValues(double *x, double *y, double *z);
private:
  HMODULE mLibrary;
};

ThinkPadSensor::ThinkPadSensor()
{
}

ThinkPadSensor::~ThinkPadSensor()
{
}

bool
ThinkPadSensor::Startup()
{
  mLibrary = LoadLibraryW(L"sensor.dll");
  if (!mLibrary)
    return false;

  gShockproofGetAccelerometerData = (ShockproofGetAccelerometerData)
    GetProcAddress(mLibrary, "ShockproofGetAccelerometerData");
  if (!gShockproofGetAccelerometerData) {
    FreeLibrary(mLibrary);
    mLibrary = nsnull;
    return false;
  }
  return true;
}

void
ThinkPadSensor::Shutdown()
{
  if (mLibrary == nsnull)
    return;

  FreeLibrary(mLibrary);
  mLibrary = nsnull;
  gShockproofGetAccelerometerData = nsnull;
}

void
ThinkPadSensor::GetValues(double *x, double *y, double *z)
{
  ThinkPadAccelerometerData accelData;

  gShockproofGetAccelerometerData(&accelData);

  
  
  
  *x = ((double)(accelData.y - 526)) / 144;
  *y = ((double)(accelData.x - 528)) / 144;
  *z = 1.0;
}

nsDeviceMotionSystem::nsDeviceMotionSystem(){}
nsDeviceMotionSystem::~nsDeviceMotionSystem(){}

void
nsDeviceMotionSystem::UpdateHandler(nsITimer *aTimer, void *aClosure)
{
  nsDeviceMotionSystem *self = reinterpret_cast<nsDeviceMotionSystem *>(aClosure);
  if (!self || !self->mSensor) {
    NS_ERROR("no self or sensor");
    return;
  }
  double x, y, z;
  self->mSensor->GetValues(&x, &y, &z);
  self->DeviceMotionChanged(nsIDeviceMotionData::TYPE_ACCELERATION, x, y, z);
}

void nsDeviceMotionSystem::Startup()
{
  NS_ASSERTION(!mSensor, "mSensor should be null.  Startup called twice?");

  bool started = false;

  mSensor = new ThinkPadSensor();
  if (mSensor)
    started = mSensor->Startup();

  if (!started) {
    mSensor = nsnull;
    return;
  }

  mUpdateTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mUpdateTimer)
    mUpdateTimer->InitWithFuncCallback(UpdateHandler,
                                       this,
                                       DEFAULT_SENSOR_POLL,
                                       nsITimer::TYPE_REPEATING_SLACK);
}

void nsDeviceMotionSystem::Shutdown()
{
  if (mUpdateTimer) {
    mUpdateTimer->Cancel();
    mUpdateTimer = nsnull;
  }

  if (mSensor)
    mSensor->Shutdown();
}


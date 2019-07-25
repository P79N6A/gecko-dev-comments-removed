




































#include "nsDeviceMotionSystem.h"
#include "nsIServiceManager.h"
#include "windows.h"





typedef struct {
  int status; 
  unsigned short x; 
  unsigned short y; 
  unsigned short xx; 
  unsigned short yy; 
  char temp; 
  unsigned short x0; 
  unsigned short y0; 
} ThinkPadDeviceMotionData;

typedef void (__stdcall *ShockproofGetDeviceMotionData)(ThinkPadDeviceMotionData*);

ShockproofGetDeviceMotionData gShockproofGetDeviceMotionData = nsnull;

class ThinkPadSensor : public Sensor
{
public:
  ThinkPadSensor();
  ~ThinkPadSensor();
  PRBool Startup();
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

PRBool
ThinkPadSensor::Startup()
{
  mLibrary = LoadLibraryW(L"sensor.dll");
  if (!mLibrary)
    return PR_FALSE;

  gShockproofGetDeviceMotionData = (ShockproofGetDeviceMotionData)
    GetProcAddress(mLibrary, "ShockproofGetDeviceMotionData");
  if (!gShockproofGetDeviceMotionData) {
    FreeLibrary(mLibrary);
    mLibrary = nsnull;
    return PR_FALSE;
  }
  return PR_TRUE;
}

void
ThinkPadSensor::Shutdown()
{
  NS_ASSERTION(mLibrary, "Shutdown called when mLibrary is null?");
  FreeLibrary(mLibrary);
  mLibrary = nsnull;
  gShockproofGetDeviceMotionData = nsnull;
}

void
ThinkPadSensor::GetValues(double *x, double *y, double *z)
{
  ThinkPadDeviceMotionData accelData;

  gShockproofGetDeviceMotionData(&accelData);

  
  
  
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

  PRBool started = PR_FALSE;

  mSensor = new ThinkPadSensor();
  if (mSensor)
    started = mSensor->Startup();

  if (!started)
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
  if (mUpdateTimer) {
    mUpdateTimer->Cancel();
    mUpdateTimer = nsnull;
  }

  if (mSensor)
    mSensor->Shutdown();
}


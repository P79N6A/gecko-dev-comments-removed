




































#include "nsAccelerometerWin.h"
#include "nsIServiceManager.h"
#include "windows.h"

#ifdef WINCE_WINDOWS_MOBILE






typedef struct _SENSORDATA
{
    SHORT   TiltX;          
    SHORT   TiltY;          
    SHORT   Orientation;    

    WORD    Unknown1;       
    DWORD   AngleY;         
    DWORD   AngleX;         
    DWORD   Unknown2;       
} SENSORDATA, *PSENSORDATA;

typedef HANDLE (WINAPI * HTCSensorOpen)(DWORD);
typedef void   (WINAPI * HTCSensorClose)(HANDLE);
typedef DWORD  (WINAPI * HTCSensorGetDataOutput)(HANDLE, PSENSORDATA);

HTCSensorOpen           gHTCSensorOpen = nsnull;
HTCSensorClose          gHTCSensorClose = nsnull;
HTCSensorGetDataOutput  gHTCSensorGetDataOutput = nsnull;

class HTCSensor : public Sensor
{
public:
  HTCSensor();
  ~HTCSensor();
  PRBool Startup();
  void Shutdown();
  void GetValues(double *x, double *y, double *z);
private:
  HMODULE mLibrary;
  HANDLE  mHTCHandle;
};

HTCSensor::HTCSensor()
{
}

HTCSensor::~HTCSensor()
{
}

PRBool
HTCSensor::Startup()
{
  HMODULE hSensorLib = LoadLibraryW(L"HTCSensorSDK.dll");
  
  if (!hSensorLib)
    return PR_FALSE;

  gHTCSensorOpen = (HTCSensorOpen) GetProcAddressW(hSensorLib, L"HTCSensorOpen");
  gHTCSensorClose = (HTCSensorClose) GetProcAddressW(hSensorLib, L"HTCSensorClose");
  gHTCSensorGetDataOutput = (HTCSensorGetDataOutput) GetProcAddressW(hSensorLib,
                                                                           L"HTCSensorGetDataOutput");

  if (gHTCSensorOpen != nsnull && gHTCSensorClose != nsnull &&
      gHTCSensorGetDataOutput != nsnull) {
      mHTCHandle = gHTCSensorOpen(1);
      if (mHTCHandle)
       return PR_TRUE;
    }

  FreeLibrary(hSensorLib);
  mLibrary = nsnull;
  gHTCSensorOpen = nsnull;
  gHTCSensorClose = nsnull;
  gHTCSensorGetDataOutput = nsnull;
  return PR_FALSE;
}

void
HTCSensor::Shutdown()
{
  NS_ASSERTION(mHTCHandle, "mHTCHandle should not be null at shutdown!");
  gHTCSensorClose(mHTCHandle);

  NS_ASSERTION(mLibrary, "Shutdown called when mLibrary is null?");
  FreeLibrary(mLibrary);
  mLibrary = nsnull;
  gHTCSensorOpen = nsnull;
  gHTCSensorClose = nsnull;
  gHTCSensorGetDataOutput = nsnull;
}

void
HTCSensor::GetValues(double *x, double *y, double *z)
{
  if (!mHTCHandle)
    return;

  static const double htcScalingFactor = 1.0 / 1000.0 * 9.8;

  SENSORDATA sd;
  gHTCSensorGetDataOutput(mHTCHandle, &sd);

  *x = ((double)sd.TiltX) / 980 ;
  *y = ((double)sd.TiltY) / 980 ;
  *z = ((double)sd.Orientation) / 1000;
}









typedef UINT SMI_RESULT;




typedef enum
{
  SMI_SUCCESS                     = 0x00000000,
  SMI_ERROR_UNKNOWN               = 0x00000001,
  SMI_ERROR_DEVICE_NOT_FOUND      = 0x00000002,
  SMI_ERROR_DEVICE_DISABLED       = 0x00000003,
  SMI_ERROR_PERMISSION_DENIED     = 0x00000004,
  SMI_ERROR_INVALID_PARAMETER     = 0x00000005,
  SMI_ERROR_CANNOT_ACTIVATE_SERVER= 0x00000006,
  SMI_ACCELEROMETER_RESULT_BASE   = 0x10010000,
  SMI_HAPTICS_RESULT_BASE         = 0x10020000,
  SMI_LED_RESULT_BASE             = 0x10030000
} SmiResultCode;





typedef struct
{
  FLOAT x;     
  FLOAT y;     
  FLOAT z;     
} SmiAccelerometerVector;




typedef struct
{
  UINT callbackPeriod;     
} SmiAccelerometerCapabilities;

typedef SMI_RESULT (WINAPI * SmiAccelerometerGetVector)(SmiAccelerometerVector *);
typedef void (*SmiAccelerometerHandler)(SmiAccelerometerVector accel);

SmiAccelerometerGetVector         gSmiAccelerometerGetVector = nsnull;

class SMISensor : public Sensor
{
public:
  SMISensor();
  ~SMISensor();
  PRBool Startup();
  void Shutdown();
  void GetValues(double *x, double *y, double *z);
private:
  HMODULE mLibrary;
};

SMISensor::SMISensor()
  :mLibrary(nsnull)
{
}

SMISensor::~SMISensor()
{
}

PRBool
SMISensor::Startup()
{
  HMODULE hSensorLib = LoadLibraryW(L"SamsungMobileSDK_1.dll");

  if (!hSensorLib)
    return PR_FALSE;

  gSmiAccelerometerGetVector = (SmiAccelerometerGetVector)
    GetProcAddressW(hSensorLib, L"SmiAccelerometerGetVector");

  if (gSmiAccelerometerGetVector == nsnull) {
    FreeLibrary(hSensorLib);
    mLibrary = nsnull;
    gSmiAccelerometerGetVector = nsnull;
    return PR_FALSE;
  }

  mLibrary = hSensorLib;
  return PR_TRUE;

}

void
SMISensor::Shutdown()
{
  NS_ASSERTION(mLibrary, "Shutdown called when mLibrary is null?");
  FreeLibrary(mLibrary);
  mLibrary = nsnull;
  gSmiAccelerometerGetVector = nsnull;
}

void
SMISensor::GetValues(double *x, double *y, double *z)
{
  NS_ASSERTION(mLibrary, "mLibrary should not be null when GetValues is called");

  SmiAccelerometerVector vector;
  vector.x = vector.y = vector.z = 0;
  SMI_RESULT result = gSmiAccelerometerGetVector(&vector);

  
  *x = vector.x;
  *y = vector.y;
  *z = vector.z;
}

#endif 

#if !defined(WINCE) && !defined(WINCE_WINDOWS_MOBILE) 





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
  mLibrary = LoadLibrary("sensor.dll");
  if (!mLibrary)
    return PR_FALSE;

  gShockproofGetAccelerometerData = (ShockproofGetAccelerometerData)
    GetProcAddress(mLibrary, "ShockproofGetAccelerometerData");
  if (!gShockproofGetAccelerometerData) {
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

#endif

nsAccelerometerWin::nsAccelerometerWin(){}
nsAccelerometerWin::~nsAccelerometerWin(){}

void
nsAccelerometerWin::UpdateHandler(nsITimer *aTimer, void *aClosure)
{
  nsAccelerometerWin *self = reinterpret_cast<nsAccelerometerWin *>(aClosure);
  if (!self || !self->mSensor) {
    NS_ERROR("no self or sensor");
    return;
  }
  double x, y, z;
  self->mSensor->GetValues(&x, &y, &z);
  self->AccelerationChanged(x, y, z);
}

void nsAccelerometerWin::Startup()
{
  NS_ASSERTION(!mSensor, "mSensor should be null.  Startup called twice?");

  PRBool started = PR_FALSE;

#ifdef WINCE_WINDOWS_MOBILE

  mSensor = new SMISensor();
  if (mSensor)
    started = mSensor->Startup();

  if (!started) {
    mSensor = new HTCSensor();
    if (mSensor)
      started = mSensor->Startup();
  }

#endif

#if !defined(WINCE) && !defined(WINCE_WINDOWS_MOBILE) 

  mSensor = new ThinkPadSensor();
  if (mSensor)
    started = mSensor->Startup();

#endif
  
  if (!started)
    return;

  mUpdateTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mUpdateTimer)
    mUpdateTimer->InitWithFuncCallback(UpdateHandler,
                                       this,
                                       mUpdateInterval,
                                       nsITimer::TYPE_REPEATING_SLACK);
}

void nsAccelerometerWin::Shutdown()
{
  if (mUpdateTimer) {
    mUpdateTimer->Cancel();
    mUpdateTimer = nsnull;
  }

  if (mSensor)
    mSensor->Shutdown();
}


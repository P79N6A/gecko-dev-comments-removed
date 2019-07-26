
















#include "base/basictypes.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/Hal.h"
#include "nsIScreen.h"
#include "nsIScreenManager.h"
#include "OrientationObserver.h"
#include "mozilla/HalSensor.h"

using namespace mozilla;
using namespace dom;

namespace {

struct OrientationMapping {
  uint32_t mScreenRotation;
  ScreenOrientation mDomOrientation;
};

static OrientationMapping sOrientationMappings[] = {
  {nsIScreen::ROTATION_0_DEG,   eScreenOrientation_PortraitPrimary},
  {nsIScreen::ROTATION_180_DEG, eScreenOrientation_PortraitSecondary},
  {nsIScreen::ROTATION_90_DEG,  eScreenOrientation_LandscapePrimary},
  {nsIScreen::ROTATION_270_DEG, eScreenOrientation_LandscapeSecondary},
};

const static int sDefaultLandscape = 2;
const static int sDefaultPortrait = 0;

static uint32_t sOrientationOffset = 0;

static already_AddRefed<nsIScreen>
GetPrimaryScreen()
{
  nsCOMPtr<nsIScreenManager> screenMgr =
    do_GetService("@mozilla.org/gfx/screenmanager;1");
  NS_ENSURE_TRUE(screenMgr, nullptr);

  nsCOMPtr<nsIScreen> screen;
  screenMgr->GetPrimaryScreen(getter_AddRefs(screen));
  return screen.forget();
}

static void
DetectDefaultOrientation()
{
  nsCOMPtr<nsIScreen> screen = GetPrimaryScreen();
  if (!screen) {
    return;
  }

  int32_t left, top, width, height;
  if (NS_FAILED(screen->GetRect(&left, &top, &width, &height))) {
    return;
  }

  uint32_t rotation;
  if (NS_FAILED(screen->GetRotation(&rotation))) {
    return;
  }

  if (width < height) {
    if (rotation == nsIScreen::ROTATION_0_DEG ||
        rotation == nsIScreen::ROTATION_180_DEG) {
      sOrientationOffset = sDefaultPortrait;
    } else {
      sOrientationOffset = sDefaultLandscape;
    }
  } else {
    if (rotation == nsIScreen::ROTATION_0_DEG ||
        rotation == nsIScreen::ROTATION_180_DEG) {
      sOrientationOffset = sDefaultLandscape;
    } else {
      sOrientationOffset = sDefaultPortrait;
    }
  }
}











static nsresult
ConvertToScreenRotation(ScreenOrientation aOrientation, uint32_t *aResult)
{
  for (int i = 0; i < ArrayLength(sOrientationMappings); i++) {
    if (aOrientation & sOrientationMappings[i].mDomOrientation) {
      
      
      int adjusted = (i + sOrientationOffset) %
                     ArrayLength(sOrientationMappings);
      *aResult = sOrientationMappings[adjusted].mScreenRotation;
      return NS_OK;
    }
  }

  *aResult = nsIScreen::ROTATION_0_DEG;
  return NS_ERROR_ILLEGAL_VALUE;
}









nsresult
ConvertToDomOrientation(uint32_t aRotation, ScreenOrientation *aResult)
{
  for (int i = 0; i < ArrayLength(sOrientationMappings); i++) {
    if (aRotation == sOrientationMappings[i].mScreenRotation) {
      
      
      int adjusted = (i + sOrientationOffset) %
                     ArrayLength(sOrientationMappings);
      *aResult = sOrientationMappings[adjusted].mDomOrientation;
      return NS_OK;
    }
  }

  *aResult = eScreenOrientation_None;
  return NS_ERROR_ILLEGAL_VALUE;
}



static StaticAutoPtr<OrientationObserver> sOrientationSensorObserver;

} 

OrientationObserver*
OrientationObserver::GetInstance()
{
  if (!sOrientationSensorObserver) {
    sOrientationSensorObserver = new OrientationObserver();
    ClearOnShutdown(&sOrientationSensorObserver);
  }

  return sOrientationSensorObserver;
}

OrientationObserver::OrientationObserver()
  : mAutoOrientationEnabled(false)
  , mLastUpdate(0)
  , mAllowedOrientations(sDefaultOrientations)
{
  DetectDefaultOrientation();

  EnableAutoOrientation();
}

OrientationObserver::~OrientationObserver()
{
  if (mAutoOrientationEnabled) {
    DisableAutoOrientation();
  }
}

 void
OrientationObserver::ShutDown()
{
  if (!sOrientationSensorObserver) {
    return;
  }

  if (sOrientationSensorObserver->mAutoOrientationEnabled) {
    sOrientationSensorObserver->DisableAutoOrientation();
  }
}

void
OrientationObserver::Notify(const hal::SensorData& aSensorData)
{
  
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aSensorData.sensor() == hal::SensorType::SENSOR_ORIENTATION);

  const InfallibleTArray<float>& values = aSensorData.values();
  
  
  float pitch = values[1];
  float roll = values[2];

  uint32_t rotation;
  if (roll > 45) {
    rotation = nsIScreen::ROTATION_90_DEG;
  } else if (roll < -45) {
    rotation = nsIScreen::ROTATION_270_DEG;
  } else if (pitch < -45) {
    rotation = nsIScreen::ROTATION_0_DEG;
  } else if (pitch > 45) {
    rotation = nsIScreen::ROTATION_180_DEG;
  } else {
    
    return;
  }

  nsCOMPtr<nsIScreen> screen = GetPrimaryScreen();
  if (!screen) {
    return;
  }

  uint32_t currRotation;
  if (NS_FAILED(screen->GetRotation(&currRotation)) ||
      rotation == currRotation) {
    return;
  }

  ScreenOrientation orientation;
  if (NS_FAILED(ConvertToDomOrientation(rotation, &orientation))) {
    return;
  }

  if ((mAllowedOrientations & orientation) == eScreenOrientation_None) {
    
    return;
  }

  PRTime now = PR_Now();
  MOZ_ASSERT(now > mLastUpdate);
  if (now - mLastUpdate < sMinUpdateInterval) {
    return;
  }
  mLastUpdate = now;

  if (NS_FAILED(screen->SetRotation(rotation))) {
    
    return;
  }
}




void
OrientationObserver::EnableAutoOrientation()
{
  MOZ_ASSERT(NS_IsMainThread() && !mAutoOrientationEnabled);

  hal::RegisterSensorObserver(hal::SENSOR_ORIENTATION, this);
  mAutoOrientationEnabled = true;
}




void
OrientationObserver::DisableAutoOrientation()
{
  MOZ_ASSERT(NS_IsMainThread() && mAutoOrientationEnabled);

  hal::UnregisterSensorObserver(hal::SENSOR_ORIENTATION, this);
  mAutoOrientationEnabled = false;
}

bool
OrientationObserver::LockScreenOrientation(ScreenOrientation aOrientation)
{
  MOZ_ASSERT(aOrientation | (eScreenOrientation_PortraitPrimary |
                             eScreenOrientation_PortraitSecondary |
                             eScreenOrientation_LandscapePrimary |
                             eScreenOrientation_LandscapeSecondary));

  
  
  if (aOrientation != eScreenOrientation_LandscapePrimary &&
      aOrientation != eScreenOrientation_LandscapeSecondary &&
      aOrientation != eScreenOrientation_PortraitPrimary &&
      aOrientation != eScreenOrientation_PortraitSecondary) {
    if (!mAutoOrientationEnabled) {
      EnableAutoOrientation();
    }
  } else if (mAutoOrientationEnabled) {
    DisableAutoOrientation();
  }

  mAllowedOrientations = aOrientation;

  nsCOMPtr<nsIScreen> screen = GetPrimaryScreen();
  NS_ENSURE_TRUE(screen, false);

  uint32_t currRotation;
  nsresult rv = screen->GetRotation(&currRotation);
  NS_ENSURE_SUCCESS(rv, false);

  ScreenOrientation currOrientation = eScreenOrientation_None;
  rv = ConvertToDomOrientation(currRotation, &currOrientation);
  NS_ENSURE_SUCCESS(rv, false);

  
  
  if (currOrientation & aOrientation) {
    return true;
  }

  
  uint32_t rotation;
  rv = ConvertToScreenRotation(aOrientation, &rotation);
  NS_ENSURE_SUCCESS(rv, false);

  rv = screen->SetRotation(rotation);
  NS_ENSURE_SUCCESS(rv, false);

  
  ScreenOrientation orientation;
  rv = ConvertToDomOrientation(rotation, &orientation);
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

void
OrientationObserver::UnlockScreenOrientation()
{
  if (!mAutoOrientationEnabled) {
    EnableAutoOrientation();
  }

  mAllowedOrientations = sDefaultOrientations;
}






#include <math.h>

#include "prlink.h"
#include "prmem.h"
#include "prenv.h"
#include "nsString.h"

#include "gfxPrefs.h"
#include "gfxVR.h"
#include "gfxVROculus.h"

#include "nsServiceManagerUtils.h"
#include "nsIScreenManager.h"

using namespace mozilla::gfx;


class FakeScreen : public nsIScreen
{
public:
  explicit FakeScreen(const IntRect& aScreenRect)
    : mScreenRect(aScreenRect)
  { }

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetRect(int32_t *l, int32_t *t, int32_t *w, int32_t *h) override {
    *l = mScreenRect.x;
    *t = mScreenRect.y;
    *w = mScreenRect.width;
    *h = mScreenRect.height;
    return NS_OK;
  }
  NS_IMETHOD GetAvailRect(int32_t *l, int32_t *t, int32_t *w, int32_t *h) override {
    return GetRect(l, t, w, h);
  }
  NS_IMETHOD GetRectDisplayPix(int32_t *l, int32_t *t, int32_t *w, int32_t *h) override {
    return GetRect(l, t, w, h);
  }
  NS_IMETHOD GetAvailRectDisplayPix(int32_t *l, int32_t *t, int32_t *w, int32_t *h) override {
    return GetAvailRect(l, t, w, h);
  }

  NS_IMETHOD GetId(uint32_t* aId) override { *aId = (uint32_t)-1; return NS_OK; }
  NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth) override { *aPixelDepth = 24; return NS_OK; }
  NS_IMETHOD GetColorDepth(int32_t* aColorDepth) override { *aColorDepth = 24; return NS_OK; }

  NS_IMETHOD LockMinimumBrightness(uint32_t aBrightness) override { return NS_ERROR_NOT_AVAILABLE; }
  NS_IMETHOD UnlockMinimumBrightness(uint32_t aBrightness) override { return NS_ERROR_NOT_AVAILABLE; }
  NS_IMETHOD GetRotation(uint32_t* aRotation) override {
    *aRotation = nsIScreen::ROTATION_0_DEG;
    return NS_OK;
  }
  NS_IMETHOD SetRotation(uint32_t aRotation) override { return NS_ERROR_NOT_AVAILABLE; }
  NS_IMETHOD GetContentsScaleFactor(double* aContentsScaleFactor) override {
    *aContentsScaleFactor = 1.0;
    return NS_OK;
  }

protected:
  virtual ~FakeScreen() {}

  IntRect mScreenRect;
};

NS_IMPL_ISUPPORTS(FakeScreen, nsIScreen)

VRHMDManager::VRHMDManagerArray *VRHMDManager::sManagers = nullptr;

 void
VRHMDManager::ManagerInit()
{
  if (sManagers)
    return;

  sManagers = new VRHMDManagerArray();

  nsRefPtr<VRHMDManager> mgr;

  mgr = new VRHMDManagerOculus();
  if (mgr->PlatformInit())
    sManagers->AppendElement(mgr);
}

 void
VRHMDManager::ManagerDestroy()
{
  if (!sManagers)
    return;

  for (uint32_t i = 0; i < sManagers->Length(); ++i) {
    (*sManagers)[i]->Destroy();
  }

  delete sManagers;
  sManagers = nullptr;
}

 void
VRHMDManager::GetAllHMDs(nsTArray<nsRefPtr<VRHMDInfo>>& aHMDResult)
{
  if (!sManagers)
    return;

  for (uint32_t i = 0; i < sManagers->Length(); ++i) {
    (*sManagers)[i]->GetHMDs(aHMDResult);
  }
}

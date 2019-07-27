






#include "mozilla/WidgetUtils.h"
#ifdef XP_WIN
#include "WinUtils.h"
#endif

namespace mozilla {

gfx::Matrix
ComputeTransformForRotation(const nsIntRect& aBounds,
                              ScreenRotation aRotation)
{
    gfx::Matrix transform;
    static const gfx::Float floatPi = static_cast<gfx::Float>(M_PI);

    switch (aRotation) {
    case ROTATION_0:
        break;
    case ROTATION_90:
        transform.PreTranslate(aBounds.width, 0);
        transform.PreRotate(floatPi / 2);
        break;
    case ROTATION_180:
        transform.PreTranslate(aBounds.width, aBounds.height);
        transform.PreRotate(floatPi);
        break;
    case ROTATION_270:
        transform.PreTranslate(0, aBounds.height);
        transform.PreRotate(floatPi * 3 / 2);
        break;
    default:
        MOZ_CRASH("Unknown rotation");
    }
    return transform;
}

gfx::Matrix
ComputeTransformForUnRotation(const nsIntRect& aBounds,
                              ScreenRotation aRotation)
{
    gfx::Matrix transform;
    static const gfx::Float floatPi = static_cast<gfx::Float>(M_PI);

    switch (aRotation) {
    case ROTATION_0:
        break;
    case ROTATION_90:
        transform.PreTranslate(0, aBounds.height);
        transform.PreRotate(floatPi * 3 / 2);
        break;
    case ROTATION_180:
        transform.PreTranslate(aBounds.width, aBounds.height);
        transform.PreRotate(floatPi);
        break;
    case ROTATION_270:
        transform.PreTranslate(aBounds.width, 0);
        transform.PreRotate(floatPi / 2);
        break;
    default:
        MOZ_CRASH("Unknown rotation");
    }
    return transform;
}

nsIntRect RotateRect(nsIntRect aRect,
                     const nsIntRect& aBounds,
                     ScreenRotation aRotation)
{
  switch (aRotation) {
    case ROTATION_0:
      return aRect;
    case ROTATION_90:
      return nsIntRect(aRect.y,
                       aBounds.width - aRect.x - aRect.width,
                       aRect.height, aRect.width);
    case ROTATION_180:
      return nsIntRect(aBounds.width - aRect.x - aRect.width,
                       aBounds.height - aRect.y - aRect.height,
                       aRect.width, aRect.height);
    case ROTATION_270:
      return nsIntRect(aBounds.height - aRect.y - aRect.height,
                       aRect.x,
                       aRect.height, aRect.width);
    default:
      MOZ_CRASH("Unknown rotation");
      return aRect;
  }
}

uint32_t
WidgetUtils::IsTouchDeviceSupportPresent()
{
#ifdef XP_WIN
  return WinUtils::IsTouchDeviceSupportPresent();
#else
  return 0;
#endif
}

} 

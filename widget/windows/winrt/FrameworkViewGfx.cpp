




#include "base/basictypes.h"
#include "FrameworkView.h"
#include "MetroWidget.h"
#include "mozilla/AutoRestore.h"
#include "MetroUtils.h"
#include "nsIWidgetListener.h"

#include <windows.ui.xaml.media.dxinterop.h>

using namespace mozilla::gfx;

namespace mozilla {
namespace widget {
namespace winrt {

static bool
IsRenderMode(gfxWindowsPlatform::RenderMode aRMode)
{
  return gfxWindowsPlatform::GetPlatform()->GetRenderMode() == aRMode;
}

bool
FrameworkView::Render()
{
  Rect msrect;
  mWindow->get_Bounds(&msrect);
  nsIntRegion region(nsIntRect(0, 0, (uint32_t)ceil(msrect.Width),
                     (uint32_t)ceil(msrect.Height)));
  return Render(region);
}

bool
FrameworkView::Render(const nsIntRegion& aInvalidRegion)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

  if (mShuttingDown || mPainting || !mWidget) {
    return false;
  }

  
  
  if (!mWidget->mLayerManager) {
    (void)mWidget->GetLayerManager();
    if (!mWidget->mLayerManager) {
      NS_WARNING("mWidget->GetLayerManager() failed!");
      return false;
    }
  }

  if (IsRenderMode(gfxWindowsPlatform::RENDER_GDI) ||
      IsRenderMode(gfxWindowsPlatform::RENDER_IMAGE_STRETCH32) ||
      IsRenderMode(gfxWindowsPlatform::RENDER_IMAGE_STRETCH24)) {
    NS_WARNING("Unsupported render mode, can't draw. Needs to be D2D.");
    return false;
  }

  if (mWidget->GetTransparencyMode() != eTransparencyOpaque) {
    NS_WARNING("transparency modes other than eTransparencyOpaque unsupported, can't draw.");
    return false;
  }

  AutoRestore<bool> painting(mPainting);
  mPainting = true;
  gfxWindowsPlatform::GetPlatform()->UpdateRenderMode();
  mWidget->Paint(aInvalidRegion);
  return true;
}

} } }






#include "2D.h"
#include "Logging.h"

#include "DrawTargetCapture.h"

namespace mozilla {
namespace gfx {

TemporaryRef<DrawTargetCapture>
DrawTarget::CreateCaptureDT(const IntSize& aSize)
{
  RefPtr<DrawTargetCaptureImpl> dt = new DrawTargetCaptureImpl();

  if (!dt->Init(aSize, this)) {
    gfxWarning() << "Failed to initialize Capture DrawTarget!";
    return nullptr;
  }

  return dt;
}

void
DrawTarget::DrawCapturedDT(DrawTargetCapture *aCaptureDT,
                           const Matrix& aTransform)
{
  if (aTransform.HasNonIntegerTranslation()) {
    gfxWarning() << "Non integer translations are not supported for DrawCaptureDT at this time!";
    return;
  }
  static_cast<DrawTargetCaptureImpl*>(aCaptureDT)->ReplayToDrawTarget(this, aTransform);
}

}
}

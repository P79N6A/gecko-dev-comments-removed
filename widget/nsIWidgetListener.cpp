




#include "nsIWidgetListener.h"

#include "nsRegion.h"
#include "nsView.h"
#include "nsIPresShell.h"
#include "nsIWidget.h"
#include "nsIXULWindow.h"

#include "mozilla/BasicEvents.h"

using namespace mozilla;

nsIXULWindow*
nsIWidgetListener::GetXULWindow()
{
  return nullptr;
}

nsView*
nsIWidgetListener::GetView()
{
  return nullptr;
}

nsIPresShell*
nsIWidgetListener::GetPresShell()
{
  return nullptr;
}

bool
nsIWidgetListener::WindowMoved(nsIWidget* aWidget,
                               int32_t aX,
                               int32_t aY)
{
  return false;
}

bool
nsIWidgetListener::WindowResized(nsIWidget* aWidget,
                                 int32_t aWidth,
                                 int32_t aHeight)
{
  return false;
}

void
nsIWidgetListener::SizeModeChanged(nsSizeMode aSizeMode)
{
}

void
nsIWidgetListener::FullscreenChanged(bool aInFullscreen)
{
}

bool
nsIWidgetListener::ZLevelChanged(bool aImmediate,
                                 nsWindowZ* aPlacement,
                                 nsIWidget* aRequestBelow,
                                 nsIWidget** aActualBelow)
{
  return false;
}

void
nsIWidgetListener::WindowActivated()
{
}

void
nsIWidgetListener::WindowDeactivated()
{
}

void
nsIWidgetListener::OSToolbarButtonPressed()
{
}

bool
nsIWidgetListener::RequestWindowClose(nsIWidget* aWidget)
{
  return false;
}

void
nsIWidgetListener::WillPaintWindow(nsIWidget* aWidget)
{
}

bool
nsIWidgetListener::PaintWindow(nsIWidget* aWidget,
                               nsIntRegion aRegion)
{
  return false;
}

void
nsIWidgetListener::DidPaintWindow()
{
}

void
nsIWidgetListener::DidCompositeWindow()
{
}

void
nsIWidgetListener::RequestRepaint()
{
}

nsEventStatus
nsIWidgetListener::HandleEvent(WidgetGUIEvent* aEvent,
                               bool aUseAttachedEvents)
{
  return nsEventStatus_eIgnore;
}

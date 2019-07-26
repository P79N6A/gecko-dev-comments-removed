




#include "nsWindowBase.h"

#include "mozilla/MiscEvents.h"
#include "npapi.h"

using namespace mozilla;

bool
nsWindowBase::DispatchPluginEvent(const MSG& aMsg)
{
  if (!PluginHasFocus()) {
    return false;
  }
  WidgetPluginEvent pluginEvent(true, NS_PLUGIN_INPUT_EVENT, this);
  nsIntPoint point(0, 0);
  InitEvent(pluginEvent, &point);
  NPEvent npEvent;
  npEvent.event = aMsg.message;
  npEvent.wParam = aMsg.wParam;
  npEvent.lParam = aMsg.lParam;
  pluginEvent.pluginEvent = &npEvent;
  pluginEvent.retargetToFocusedDocument = true;
  return DispatchWindowEvent(&pluginEvent);
}






#include "mozilla/BasicEvents.h"
#include "mozilla/MiscEvents.h"
#include "mozilla/MouseEvents.h"

namespace mozilla {







bool
WidgetEvent::IsInputDerivedEvent() const
{
  switch (eventStructType) {
    case NS_INPUT_EVENT:
    case NS_MOUSE_EVENT:
    case NS_KEY_EVENT:
    case NS_TOUCH_EVENT:
    case NS_DRAG_EVENT:
    case NS_MOUSE_SCROLL_EVENT:
    case NS_WHEEL_EVENT:
    case NS_SIMPLE_GESTURE_EVENT:
      return true;
    default:
      return false;
  }
}

bool
WidgetEvent::IsMouseDerivedEvent() const
{
  return eventStructType == NS_MOUSE_EVENT ||
         eventStructType == NS_DRAG_EVENT;
}

bool
WidgetEvent::IsQueryContentEvent() const
{
  return eventStructType == NS_QUERY_CONTENT_EVENT;
}

bool
WidgetEvent::IsSelectionEvent() const
{
  return eventStructType == NS_SELECTION_EVENT;
}

bool
WidgetEvent::IsContentCommandEvent() const
{
  return eventStructType == NS_CONTENT_COMMAND_EVENT;
}

bool
WidgetEvent::IsNativeEventDelivererForPlugin() const
{
  return eventStructType == NS_PLUGIN_EVENT;
}








bool
WidgetEvent::HasMouseEventMessage() const
{
  switch (message) {
    case NS_MOUSE_BUTTON_DOWN:
    case NS_MOUSE_BUTTON_UP:
    case NS_MOUSE_CLICK:
    case NS_MOUSE_DOUBLECLICK:
    case NS_MOUSE_ENTER:
    case NS_MOUSE_EXIT:
    case NS_MOUSE_ACTIVATE:
    case NS_MOUSE_ENTER_SYNTH:
    case NS_MOUSE_EXIT_SYNTH:
    case NS_MOUSE_MOZHITTEST:
    case NS_MOUSE_MOVE:
      return true;
    default:
      return false;
  }
}

bool
WidgetEvent::HasDragEventMessage() const
{
  switch (message) {
    case NS_DRAGDROP_ENTER:
    case NS_DRAGDROP_OVER:
    case NS_DRAGDROP_EXIT:
    case NS_DRAGDROP_DRAGDROP:
    case NS_DRAGDROP_GESTURE:
    case NS_DRAGDROP_DRAG:
    case NS_DRAGDROP_END:
    case NS_DRAGDROP_START:
    case NS_DRAGDROP_DROP:
    case NS_DRAGDROP_LEAVE_SYNTH:
      return true;
    default:
      return false;
  }
}

bool
WidgetEvent::HasKeyEventMessage() const
{
  switch (message) {
    case NS_KEY_DOWN:
    case NS_KEY_PRESS:
    case NS_KEY_UP:
      return true;
    default:
      return false;
  }
}

bool
WidgetEvent::HasIMEEventMessage() const
{
  switch (message) {
    case NS_TEXT_TEXT:
    case NS_COMPOSITION_START:
    case NS_COMPOSITION_END:
    case NS_COMPOSITION_UPDATE:
      return true;
    default:
      return false;
  }
}

bool
WidgetEvent::HasPluginActivationEventMessage() const
{
  return message == NS_PLUGIN_ACTIVATE ||
         message == NS_PLUGIN_FOCUS;
}







bool
WidgetEvent::IsLeftClickEvent() const
{
  return eventStructType == NS_MOUSE_EVENT &&
         message == NS_MOUSE_CLICK &&
         static_cast<const WidgetMouseEvent*>(this)->button ==
           WidgetMouseEvent::eLeftButton;
}

bool
WidgetEvent::IsContextMenuKeyEvent() const
{
  return eventStructType == NS_MOUSE_EVENT &&
         message == NS_CONTEXTMENU &&
         static_cast<const WidgetMouseEvent*>(this)->context ==
            WidgetMouseEvent::eContextMenuKey;
}

bool
WidgetEvent::IsRetargetedNativeEventDelivererForPlugin() const
{
  return IsNativeEventDelivererForPlugin() &&
    static_cast<const WidgetPluginEvent*>(this)->retargetToFocusedDocument;
}

bool
WidgetEvent::IsNonRetargetedNativeEventDelivererForPlugin() const
{
  return IsNativeEventDelivererForPlugin() &&
    !static_cast<const WidgetPluginEvent*>(this)->retargetToFocusedDocument;
}

bool
WidgetEvent::IsIMERelatedEvent() const
{
  return HasIMEEventMessage() || IsQueryContentEvent() || IsSelectionEvent();
}

bool
WidgetEvent::IsUsingCoordinates() const
{
  return !HasKeyEventMessage() && !IsIMERelatedEvent() &&
         !IsContextMenuKeyEvent() &&
         !HasPluginActivationEventMessage() &&
         !IsNativeEventDelivererForPlugin() &&
         !IsContentCommandEvent() &&
         message != NS_PLUGIN_RESOLUTION_CHANGED;
}

bool
WidgetEvent::IsTargetedAtFocusedWindow() const
{
  return HasKeyEventMessage() || IsIMERelatedEvent() ||
         IsContextMenuKeyEvent() ||
         IsContentCommandEvent() ||
         IsRetargetedNativeEventDelivererForPlugin();
}

bool
WidgetEvent::IsTargetedAtFocusedContent() const
{
  return HasKeyEventMessage() || IsIMERelatedEvent() ||
         IsContextMenuKeyEvent() ||
         IsRetargetedNativeEventDelivererForPlugin();
}

bool
WidgetEvent::IsAllowedToDispatchDOMEvent() const
{
  switch (eventStructType) {
    case NS_MOUSE_EVENT:
      
      
      
      
      
      return static_cast<const WidgetMouseEvent*>(this)->reason ==
               WidgetMouseEvent::eReal;

    case NS_WHEEL_EVENT: {
      
      
      const WidgetWheelEvent* wheelEvent =
        static_cast<const WidgetWheelEvent*>(this);
      return wheelEvent->deltaX != 0.0 || wheelEvent->deltaY != 0.0 ||
             wheelEvent->deltaZ != 0.0;
    }

    default:
      return true;
  }
}

} 

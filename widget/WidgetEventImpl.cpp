




#include "mozilla/BasicEvents.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/InternalMutationEvent.h"
#include "mozilla/MiscEvents.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Preferences.h"
#include "mozilla/TextEvents.h"
#include "mozilla/TouchEvents.h"

namespace mozilla {





#define NS_ROOT_EVENT_CLASS(aPrefix, aName)
#define NS_EVENT_CLASS(aPrefix, aName) \
aPrefix##aName* \
WidgetEvent::As##aName() \
{ \
  return nullptr; \
} \
\
const aPrefix##aName* \
WidgetEvent::As##aName() const \
{ \
  return const_cast<WidgetEvent*>(this)->As##aName(); \
}

#include "mozilla/EventClassList.h"

#undef NS_EVENT_CLASS
#undef NS_ROOT_EVENT_CLASS







bool
WidgetEvent::IsQueryContentEvent() const
{
  return mClass == eQueryContentEventClass;
}

bool
WidgetEvent::IsSelectionEvent() const
{
  return mClass == eSelectionEventClass;
}

bool
WidgetEvent::IsContentCommandEvent() const
{
  return mClass == eContentCommandEventClass;
}

bool
WidgetEvent::IsNativeEventDelivererForPlugin() const
{
  return mClass == ePluginEventClass;
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
    case NS_KEY_BEFORE_DOWN:
    case NS_KEY_BEFORE_UP:
    case NS_KEY_AFTER_DOWN:
    case NS_KEY_AFTER_UP:
      return true;
    default:
      return false;
  }
}

bool
WidgetEvent::HasIMEEventMessage() const
{
  switch (message) {
    case NS_COMPOSITION_START:
    case NS_COMPOSITION_END:
    case NS_COMPOSITION_UPDATE:
    case NS_COMPOSITION_CHANGE:
    case NS_COMPOSITION_COMMIT_AS_IS:
    case NS_COMPOSITION_COMMIT:
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
WidgetEvent::IsRetargetedNativeEventDelivererForPlugin() const
{
  const WidgetPluginEvent* pluginEvent = AsPluginEvent();
  return pluginEvent && pluginEvent->retargetToFocusedDocument;
}

bool
WidgetEvent::IsNonRetargetedNativeEventDelivererForPlugin() const
{
  const WidgetPluginEvent* pluginEvent = AsPluginEvent();
  return pluginEvent && !pluginEvent->retargetToFocusedDocument;
}

bool
WidgetEvent::IsIMERelatedEvent() const
{
  return HasIMEEventMessage() || IsQueryContentEvent() || IsSelectionEvent();
}

bool
WidgetEvent::IsUsingCoordinates() const
{
  const WidgetMouseEvent* mouseEvent = AsMouseEvent();
  if (mouseEvent) {
    return !mouseEvent->IsContextMenuKeyEvent();
  }
  return !HasKeyEventMessage() && !IsIMERelatedEvent() &&
         !HasPluginActivationEventMessage() &&
         !IsNativeEventDelivererForPlugin() &&
         !IsContentCommandEvent();
}

bool
WidgetEvent::IsTargetedAtFocusedWindow() const
{
  const WidgetMouseEvent* mouseEvent = AsMouseEvent();
  if (mouseEvent) {
    return mouseEvent->IsContextMenuKeyEvent();
  }
  return HasKeyEventMessage() || IsIMERelatedEvent() ||
         IsContentCommandEvent() ||
         IsRetargetedNativeEventDelivererForPlugin();
}

bool
WidgetEvent::IsTargetedAtFocusedContent() const
{
  const WidgetMouseEvent* mouseEvent = AsMouseEvent();
  if (mouseEvent) {
    return mouseEvent->IsContextMenuKeyEvent();
  }
  return HasKeyEventMessage() || IsIMERelatedEvent() ||
         IsRetargetedNativeEventDelivererForPlugin();
}

bool
WidgetEvent::IsAllowedToDispatchDOMEvent() const
{
  switch (mClass) {
    case eMouseEventClass:
    case ePointerEventClass:
      
      
      
      
      
      return AsMouseEvent()->reason == WidgetMouseEvent::eReal;

    case eWheelEventClass: {
      
      
      const WidgetWheelEvent* wheelEvent = AsWheelEvent();
      return wheelEvent->deltaX != 0.0 || wheelEvent->deltaY != 0.0 ||
             wheelEvent->deltaZ != 0.0;
    }

    
    
    case eQueryContentEventClass:
    case eSelectionEventClass:
    case eContentCommandEventClass:
      return false;

    default:
      return true;
  }
}






Modifier
WidgetInputEvent::AccelModifier()
{
  static Modifier sAccelModifier = MODIFIER_NONE;
  if (sAccelModifier == MODIFIER_NONE) {
    switch (Preferences::GetInt("ui.key.accelKey", 0)) {
      case nsIDOMKeyEvent::DOM_VK_META:
        sAccelModifier = MODIFIER_META;
        break;
      case nsIDOMKeyEvent::DOM_VK_WIN:
        sAccelModifier = MODIFIER_OS;
        break;
      case nsIDOMKeyEvent::DOM_VK_ALT:
        sAccelModifier = MODIFIER_ALT;
        break;
      case nsIDOMKeyEvent::DOM_VK_CONTROL:
        sAccelModifier = MODIFIER_CONTROL;
        break;
      default:
#ifdef XP_MACOSX
        sAccelModifier = MODIFIER_META;
#else
        sAccelModifier = MODIFIER_CONTROL;
#endif
        break;
    }
  }
  return sAccelModifier;
}





#define NS_DEFINE_KEYNAME(aCPPName, aDOMKeyName) MOZ_UTF16(aDOMKeyName),
const char16_t* WidgetKeyboardEvent::kKeyNames[] = {
#include "mozilla/KeyNameList.h"
};
#undef NS_DEFINE_KEYNAME

#define NS_DEFINE_PHYSICAL_KEY_CODE_NAME(aCPPName, aDOMCodeName) \
    MOZ_UTF16(aDOMCodeName),
const char16_t* WidgetKeyboardEvent::kCodeNames[] = {
#include "mozilla/PhysicalKeyCodeNameList.h"
};
#undef NS_DEFINE_PHYSICAL_KEY_CODE_NAME

WidgetKeyboardEvent::KeyNameIndexHashtable*
  WidgetKeyboardEvent::sKeyNameIndexHashtable = nullptr;
WidgetKeyboardEvent::CodeNameIndexHashtable*
  WidgetKeyboardEvent::sCodeNameIndexHashtable = nullptr;

bool
WidgetKeyboardEvent::ShouldCauseKeypressEvents() const
{
  
  switch (mKeyNameIndex) {
    case KEY_NAME_INDEX_Alt:
    case KEY_NAME_INDEX_AltGraph:
    case KEY_NAME_INDEX_CapsLock:
    case KEY_NAME_INDEX_Control:
    case KEY_NAME_INDEX_Fn:
    
    
    case KEY_NAME_INDEX_Meta:
    case KEY_NAME_INDEX_NumLock:
    case KEY_NAME_INDEX_OS:
    case KEY_NAME_INDEX_ScrollLock:
    case KEY_NAME_INDEX_Shift:
    
    case KEY_NAME_INDEX_Symbol:
    
      return false;
    default:
      return true;
  }
}

 void
WidgetKeyboardEvent::Shutdown()
{
  delete sKeyNameIndexHashtable;
  sKeyNameIndexHashtable = nullptr;
  delete sCodeNameIndexHashtable;
  sCodeNameIndexHashtable = nullptr;
}

 void
WidgetKeyboardEvent::GetDOMKeyName(KeyNameIndex aKeyNameIndex,
                                   nsAString& aKeyName)
{
  if (aKeyNameIndex >= KEY_NAME_INDEX_USE_STRING) {
    aKeyName.Truncate();
    return;
  }

  MOZ_RELEASE_ASSERT(static_cast<size_t>(aKeyNameIndex) <
                       ArrayLength(kKeyNames),
                     "Illegal key enumeration value");
  aKeyName = kKeyNames[aKeyNameIndex];
}

 void
WidgetKeyboardEvent::GetDOMCodeName(CodeNameIndex aCodeNameIndex,
                                    nsAString& aCodeName)
{
  if (aCodeNameIndex >= CODE_NAME_INDEX_USE_STRING) {
    aCodeName.Truncate();
    return;
  }

  MOZ_RELEASE_ASSERT(static_cast<size_t>(aCodeNameIndex) <
                       ArrayLength(kCodeNames),
                     "Illegal physical code enumeration value");
  aCodeName = kCodeNames[aCodeNameIndex];
}

 KeyNameIndex
WidgetKeyboardEvent::GetKeyNameIndex(const nsAString& aKeyValue)
{
  if (!sKeyNameIndexHashtable) {
    sKeyNameIndexHashtable =
      new KeyNameIndexHashtable(ArrayLength(kKeyNames));
    for (size_t i = 0; i < ArrayLength(kKeyNames); i++) {
      sKeyNameIndexHashtable->Put(nsDependentString(kKeyNames[i]),
                                  static_cast<KeyNameIndex>(i));
    }
  }
  KeyNameIndex result = KEY_NAME_INDEX_USE_STRING;
  sKeyNameIndexHashtable->Get(aKeyValue, &result);
  return result;
}

 CodeNameIndex
WidgetKeyboardEvent::GetCodeNameIndex(const nsAString& aCodeValue)
{
  if (!sCodeNameIndexHashtable) {
    sCodeNameIndexHashtable =
      new CodeNameIndexHashtable(ArrayLength(kCodeNames));
    for (size_t i = 0; i < ArrayLength(kCodeNames); i++) {
      sCodeNameIndexHashtable->Put(nsDependentString(kCodeNames[i]),
                                   static_cast<CodeNameIndex>(i));
    }
  }
  CodeNameIndex result = CODE_NAME_INDEX_USE_STRING;
  sCodeNameIndexHashtable->Get(aCodeValue, &result);
  return result;
}

 const char*
WidgetKeyboardEvent::GetCommandStr(Command aCommand)
{
#define NS_DEFINE_COMMAND(aName, aCommandStr) , #aCommandStr
  static const char* kCommands[] = {
    "" 
#include "mozilla/CommandList.h"
  };
#undef NS_DEFINE_COMMAND

  MOZ_RELEASE_ASSERT(static_cast<size_t>(aCommand) < ArrayLength(kCommands),
                     "Illegal command enumeration value");
  return kCommands[aCommand];
}

 uint32_t
WidgetKeyboardEvent::ComputeLocationFromCodeValue(CodeNameIndex aCodeNameIndex)
{
  
  
  
  switch (aCodeNameIndex) {
    case CODE_NAME_INDEX_AltLeft:
    case CODE_NAME_INDEX_ControlLeft:
    case CODE_NAME_INDEX_OSLeft:
    case CODE_NAME_INDEX_ShiftLeft:
      return nsIDOMKeyEvent::DOM_KEY_LOCATION_LEFT;
    case CODE_NAME_INDEX_AltRight:
    case CODE_NAME_INDEX_ControlRight:
    case CODE_NAME_INDEX_OSRight:
    case CODE_NAME_INDEX_ShiftRight:
      return nsIDOMKeyEvent::DOM_KEY_LOCATION_RIGHT;
    case CODE_NAME_INDEX_Numpad0:
    case CODE_NAME_INDEX_Numpad1:
    case CODE_NAME_INDEX_Numpad2:
    case CODE_NAME_INDEX_Numpad3:
    case CODE_NAME_INDEX_Numpad4:
    case CODE_NAME_INDEX_Numpad5:
    case CODE_NAME_INDEX_Numpad6:
    case CODE_NAME_INDEX_Numpad7:
    case CODE_NAME_INDEX_Numpad8:
    case CODE_NAME_INDEX_Numpad9:
    case CODE_NAME_INDEX_NumpadAdd:
    case CODE_NAME_INDEX_NumpadBackspace:
    
    
    case CODE_NAME_INDEX_NumpadComma:
    case CODE_NAME_INDEX_NumpadDecimal:
    case CODE_NAME_INDEX_NumpadDivide:
    case CODE_NAME_INDEX_NumpadEnter:
    case CODE_NAME_INDEX_NumpadEqual:
    
    
    
    
    case CODE_NAME_INDEX_NumpadMemorySubtract:
    case CODE_NAME_INDEX_NumpadMultiply:
    
    
    case CODE_NAME_INDEX_NumpadSubtract:
      return nsIDOMKeyEvent::DOM_KEY_LOCATION_NUMPAD;
    default:
      return nsIDOMKeyEvent::DOM_KEY_LOCATION_STANDARD;
  }
}

} 

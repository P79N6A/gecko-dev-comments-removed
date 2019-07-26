




#include "base/basictypes.h"
#include "assert.h"
#include "ANPBase.h"
#include <android/log.h>
#include "AndroidBridge.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIPluginInstanceOwner.h"
#include "nsPluginInstanceOwner.h"
#include "nsWindow.h"
#include "mozilla/dom/ScreenOrientation.h"

#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#define ASSIGN(obj, name)   (obj)->name = anp_window_##name

using namespace mozilla;
using namespace mozilla::widget;
using namespace mozilla::dom;

static nsresult GetOwner(NPP instance, nsPluginInstanceOwner** owner) {
  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

  return pinst->GetOwner((nsIPluginInstanceOwner**)owner);
}

void
anp_window_setVisibleRects(NPP instance, const ANPRectI rects[], int32_t count)
{
  NOT_IMPLEMENTED();
}

void
anp_window_clearVisibleRects(NPP instance)
{
  NOT_IMPLEMENTED();
}

void
anp_window_showKeyboard(NPP instance, bool value)
{
  InputContext context;
  context.mIMEState.mEnabled = value ? IMEState::PLUGIN : IMEState::DISABLED;
  context.mIMEState.mOpen = value ? IMEState::OPEN : IMEState::CLOSED;
  context.mActionHint.Assign(EmptyString());

  InputContextAction action;
  action.mCause = InputContextAction::CAUSE_UNKNOWN;
  action.mFocusChange = InputContextAction::FOCUS_NOT_CHANGED;

  nsWindow* window = nsWindow::TopWindow();
  if (!window) {
    LOG("Couldn't get top window?");
    return;
  }

  window->SetInputContext(context, action);
}

void
anp_window_requestFullScreen(NPP instance)
{
  nsRefPtr<nsPluginInstanceOwner> owner;
  if (NS_FAILED(GetOwner(instance, getter_AddRefs(owner)))) {
    return;
  }

  owner->RequestFullScreen();
}

void
anp_window_exitFullScreen(NPP instance)
{
  nsRefPtr<nsPluginInstanceOwner> owner;
  if (NS_FAILED(GetOwner(instance, getter_AddRefs(owner)))) {
    return;
  }

  owner->ExitFullScreen();
}

void
anp_window_requestCenterFitZoom(NPP instance)
{
  NOT_IMPLEMENTED();
}

ANPRectI
anp_window_visibleRect(NPP instance)
{
  ANPRectI rect = { 0, 0, 0, 0 };

  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

  nsIntSize currentSize = pinst->CurrentSize();
  rect.left = rect.top = 0;
  rect.right = currentSize.width;
  rect.bottom = currentSize.height;

  return rect;
}

void anp_window_requestFullScreenOrientation(NPP instance, ANPScreenOrientation orientation)
{
  short newOrientation;

  
  switch (orientation) {
    case kFixedLandscape_ANPScreenOrientation:
      newOrientation = eScreenOrientation_LandscapePrimary;
      break;
    case kFixedPortrait_ANPScreenOrientation:
      newOrientation = eScreenOrientation_PortraitPrimary;
      break;
    case kLandscape_ANPScreenOrientation:
      newOrientation = eScreenOrientation_LandscapePrimary |
                       eScreenOrientation_LandscapeSecondary;
      break;
    case kPortrait_ANPScreenOrientation:
      newOrientation = eScreenOrientation_PortraitPrimary |
                       eScreenOrientation_PortraitSecondary;
      break;
    default:
      newOrientation = eScreenOrientation_None;
      break;
  }

  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);
  pinst->SetFullScreenOrientation(newOrientation);
}

void InitWindowInterface(ANPWindowInterfaceV0 *i) {
  _assert(i->inSize == sizeof(*i));
  ASSIGN(i, setVisibleRects);
  ASSIGN(i, clearVisibleRects);
  ASSIGN(i, showKeyboard);
  ASSIGN(i, requestFullScreen);
  ASSIGN(i, exitFullScreen);
  ASSIGN(i, requestCenterFitZoom);
}

void InitWindowInterfaceV1(ANPWindowInterfaceV1 *i) {
  _assert(i->inSize == sizeof(*i));
  ASSIGN(i, setVisibleRects);
  ASSIGN(i, clearVisibleRects);
  ASSIGN(i, showKeyboard);
  ASSIGN(i, requestFullScreen);
  ASSIGN(i, exitFullScreen);
  ASSIGN(i, requestCenterFitZoom);
  ASSIGN(i, visibleRect);
}

void InitWindowInterfaceV2(ANPWindowInterfaceV2 *i) {
  _assert(i->inSize == sizeof(*i));
  ASSIGN(i, setVisibleRects);
  ASSIGN(i, clearVisibleRects);
  ASSIGN(i, showKeyboard);
  ASSIGN(i, requestFullScreen);
  ASSIGN(i, exitFullScreen);
  ASSIGN(i, requestCenterFitZoom);
  ASSIGN(i, visibleRect);
  ASSIGN(i, requestFullScreenOrientation);
}


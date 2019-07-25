





































#include "assert.h"
#include "ANPBase.h"
#include <android/log.h>
#include "AndroidBridge.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIPluginInstanceOwner.h"
#include "nsPluginInstanceOwner.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#define ASSIGN(obj, name)   (obj)->name = anp_window_##name

using namespace mozilla;

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
  NOT_IMPLEMENTED();
}

void
anp_window_requestFullScreen(NPP instance)
{
  NOT_IMPLEMENTED();
}

void
anp_window_exitFullScreen(NPP instance)
{
  NOT_IMPLEMENTED();
}

void
anp_window_requestCenterFitZoom(NPP instance)
{
  NOT_IMPLEMENTED();
}

static nsresult GetOwner(NPP instance, nsPluginInstanceOwner** owner) {
  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

  return pinst->GetOwner((nsIPluginInstanceOwner**)owner);
}

ANPRectI
anp_window_visibleRect(NPP instance)
{
  ANPRectI rect = { 0, 0, 0, 0 };

  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

  nsRefPtr<nsPluginInstanceOwner> owner;
  if (NS_FAILED(GetOwner(instance, getter_AddRefs(owner)))) {
    return rect;
  }

  nsIntRect visibleRect = owner->GetVisibleRect();
  rect.left = visibleRect.x;
  rect.top = visibleRect.y;
  rect.right = visibleRect.x + visibleRect.width;
  rect.bottom = visibleRect.y + visibleRect.height;

  return rect;
}

void anp_window_requestFullScreenOrientation(NPP instance, ANPScreenOrientation orientation)
{
  NOT_IMPLEMENTED();
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


































#include <QWidget>
#include <QPainter>

#include "nptest_platform.h"
#include "npapi.h"

struct _PlatformData {
#ifdef MOZ_X11
  Display* display;
  Visual* visual;
  Colormap colormap;
#endif
};
 using namespace std;

bool
pluginSupportsWindowMode()
{
  return false;
}

bool
pluginSupportsWindowlessMode()
{
  return true;
}

NPError
pluginInstanceInit(InstanceData* instanceData)
{
#ifdef MOZ_X11
  instanceData->platformData = static_cast<PlatformData*>
    (NPN_MemAlloc(sizeof(PlatformData)));
  if (!instanceData->platformData){
    
    return NPERR_OUT_OF_MEMORY_ERROR;
  }

  instanceData->platformData->display = NULL;
  instanceData->platformData->visual = NULL;
  instanceData->platformData->colormap = None;

  return NPERR_NO_ERROR;
#else
  printf("NPERR_INCOMPATIBLE_VERSION_ERROR\n");
  return NPERR_INCOMPATIBLE_VERSION_ERROR;
#endif
  return NPERR_NO_ERROR;
}

void
pluginInstanceShutdown(InstanceData* instanceData)
{
  NPN_MemFree(instanceData->platformData);
  instanceData->platformData = 0;
}

void
pluginDoSetWindow(InstanceData* instanceData, NPWindow* newWindow)
{
  instanceData->window = *newWindow;
}

void
pluginWidgetInit(InstanceData* instanceData, void* oldWindow)
{
  
}

static void
pluginDrawWindow(InstanceData* instanceData, void* event)
{
  NPWindow& window = instanceData->window;
  
  
  int x = instanceData->hasWidget ? 0 : window.x;
  int y = instanceData->hasWidget ? 0 : window.y;
  int width = window.width;
  int height = window.height;

  XEvent* nsEvent = (XEvent*)event;
  const XGraphicsExposeEvent& expose = nsEvent->xgraphicsexpose;

  QColor drawColor((QColor)instanceData->scriptableObject->drawColor);
  QPixmap pixmap = QPixmap::fromX11Pixmap(expose.drawable, QPixmap::ExplicitlyShared);

  QRect exposeRect(expose.x, expose.y, expose.width, expose.height);
  if (instanceData->scriptableObject->drawMode == DM_SOLID_COLOR) {
    
    
    QPainter painter(&pixmap);
    painter.fillRect(exposeRect, drawColor);
    notifyDidPaint(instanceData);
    return;

  }

  NPP npp = instanceData->npp;
  if (!npp)
    return;

  QString text (NPN_UserAgent(npp));
  if (text.isEmpty())
    return;

  
  
  QColor color;
  QPainter painter(&pixmap);
  QRect theRect(x, y, width, height);
  QRect clipRect(QPoint(window.clipRect.left, window.clipRect.top),
                 QPoint(window.clipRect.right, window.clipRect.bottom));
  painter.setClipRect(clipRect);
  painter.fillRect(theRect, QColor(128,128,128,255));
  painter.drawRect(theRect);
  painter.drawText(QRect(theRect), Qt::AlignCenter, text);
  notifyDidPaint(instanceData);
  return;
}

int16_t
pluginHandleEvent(InstanceData* instanceData, void* event)
{
#ifdef MOZ_X11
  XEvent* nsEvent = (XEvent*)event;
  
  switch (nsEvent->type) {
  case GraphicsExpose: {
    

    pluginDrawWindow(instanceData, event);
    break;
  }
  case MotionNotify: {
    
    XMotionEvent* motion = &nsEvent->xmotion;
    instanceData->lastMouseX = motion->x;
    instanceData->lastMouseY = motion->y;
    break;
  }
  case ButtonPress:{
    
    break;
  }
  case ButtonRelease: {
    
    XButtonEvent* button = &nsEvent->xbutton;
    instanceData->lastMouseX = button->x;
    instanceData->lastMouseY = button->y;
    break;
  }
  default:
    break;
  }
#endif

  return 0;
}

int32_t pluginGetEdge(InstanceData* instanceData, RectEdge edge)
{
  
  return NPTEST_INT32_ERROR;
}

int32_t pluginGetClipRegionRectCount(InstanceData* instanceData)
{
  
  return NPTEST_INT32_ERROR;
}

int32_t pluginGetClipRegionRectEdge(InstanceData* instanceData, 
    int32_t rectIndex, RectEdge edge)
{
  
  return NPTEST_INT32_ERROR;
}

void pluginDoInternalConsistencyCheck(InstanceData* instanceData, string& error)
{
}



































#include "nptest_platform.h"

#include <windows.h>
#include <unknwn.h>
#include <gdiplus.h>
using namespace Gdiplus;

#pragma comment(lib, "gdiplus.lib")

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
  return NPERR_NO_ERROR;
}

void
pluginInstanceShutdown(InstanceData* instanceData)
{
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

static Color
GetColorsFromRGBA(PRUint32 rgba)
{
  BYTE r, g, b, a;
  b = (rgba & 0xFF);
  g = ((rgba & 0xFF00) >> 8);
  r = ((rgba & 0xFF0000) >> 16);
  a = ((rgba & 0xFF000000) >> 24);
  return Color(a, r, g, b);
}

void
pluginDraw(InstanceData* instanceData)
{
  NPP npp = instanceData->npp;
  if (!npp)
    return;

  const char* uaString = NPN_UserAgent(npp);
  if (!uaString)
    return;

  HDC hdc = (HDC)instanceData->window.window;

  if (hdc == NULL)
    return;

  
  
  int savedDCID = SaveDC(hdc);

  
  SelectClipRgn(hdc, NULL);

  
  GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

  
  Rect rect(instanceData->window.x, instanceData->window.y, 
    instanceData->window.width, instanceData->window.height);

  
  RectF boundRect((float)instanceData->window.x, (float)instanceData->window.y, 
    (float)instanceData->window.width, (float)instanceData->window.height);
  boundRect.Inflate(-10.0, -10.0);

  switch(instanceData->scriptableObject->drawMode) {
    case DM_DEFAULT:
    {
      Graphics graphics(hdc);

      
      Pen blackPen(Color(255, 0, 0, 0), 5);
      SolidBrush grayBrush(Color(255, 192, 192, 192));

      graphics.FillRectangle(&grayBrush, rect);
      graphics.DrawRectangle(&blackPen, rect);

      
      FontFamily fontFamily(L"Helvetica");
      Font font(&fontFamily, 20, FontStyleBold, UnitPoint);
      StringFormat stringFormat;
      SolidBrush solidBrush(Color(255, 0, 0, 0));

      
      stringFormat.SetAlignment(StringAlignmentCenter);
      stringFormat.SetLineAlignment(StringAlignmentCenter);

      
      graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

      WCHAR wBuf[1024];
      memset(&wBuf, 0, sizeof(wBuf));
      MultiByteToWideChar(CP_ACP, 0, uaString, -1, wBuf, 1024);

      
      graphics.DrawString(wBuf, -1, &font, boundRect, &stringFormat, &solidBrush);
    }
    break;

    case DM_SOLID_COLOR:
    {
      
      Graphics graphics(hdc);
      SolidBrush brush(GetColorsFromRGBA(instanceData->scriptableObject->drawColor));
      graphics.FillRectangle(&brush, rect.X, rect.Y, rect.Width, rect.Height);
    }
    break;
  }

  
  GdiplusShutdown(gdiplusToken);

  
  RestoreDC(hdc, savedDCID);
}

int16_t
pluginHandleEvent(InstanceData* instanceData, void* event)
{
  NPEvent * pe = (NPEvent*) event;

  if (pe == NULL || instanceData == NULL ||
      instanceData->window.type != NPWindowTypeDrawable)
    return 0;   

  switch((UINT)pe->event) {
    case WM_PAINT:   
      pluginDraw(instanceData);   
      return 1;
  }
  
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

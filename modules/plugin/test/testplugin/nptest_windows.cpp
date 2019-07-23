

































#include "nptest_platform.h"

#include <windows.h>
#include <unknwn.h>
#include <gdiplus.h>
using namespace Gdiplus;

#pragma comment(lib, "gdiplus.lib")

void SetSubclass(HWND hWnd, InstanceData* instanceData);
void ClearSubclass(HWND hWnd);
Color GetColorsFromRGBA(PRUint32 rgba);
LRESULT CALLBACK PluginWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool
pluginSupportsWindowMode()
{
  return true;
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
  HWND hWnd = (HWND)instanceData->window.window;
  if (oldWindow) {
    HWND hWndOld = (HWND)oldWindow;
    ClearSubclass(hWndOld);
  }
  SetSubclass(hWnd, instanceData);
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

  HDC hdc = NULL;
  PAINTSTRUCT ps;

  if (instanceData->hasWidget)
    hdc = ::BeginPaint((HWND)instanceData->window.window, &ps);
  else
    hdc = (HDC)instanceData->window.window;

  if (hdc == NULL)
    return;

  
  
  int savedDCID = SaveDC(hdc);

  
  SelectClipRgn(hdc, NULL);

  
  GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

  
  
  int x = instanceData->hasWidget ? 0 : instanceData->window.x;
  int y = instanceData->hasWidget ? 0 : instanceData->window.y;
  int width = instanceData->window.width;
  int height = instanceData->window.height;

  
  Rect rect(x, y, width, height);

  
  RectF boundRect((float)x, (float)y, (float)width, (float)height);
  boundRect.Inflate(-10.0, -10.0);

  switch(instanceData->scriptableObject->drawMode) {
    case DM_DEFAULT:
    {
      Graphics graphics(hdc);

      
      Pen blackPen(Color(255, 0, 0, 0), 1);
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

  if (instanceData->hasWidget)
    ::EndPaint((HWND)instanceData->window.window, &ps);
}



int32_t
pluginGetEdge(InstanceData* instanceData, RectEdge edge)
{
  if (!instanceData || !instanceData->hasWidget)
    return NPTEST_INT32_ERROR;

  HWND hWnd = GetAncestor((HWND)instanceData->window.window, GA_ROOT);

  if (!hWnd)
    return NPTEST_INT32_ERROR;

  RECT rect = {0};
  GetClientRect((HWND)instanceData->window.window, &rect);
  MapWindowPoints((HWND)instanceData->window.window, hWnd, (LPPOINT)&rect, 2);

  switch (edge) {
  case EDGE_LEFT:
    return rect.left;
  case EDGE_TOP:
    return rect.top;
  case EDGE_RIGHT:
    return rect.right;
  case EDGE_BOTTOM:
    return rect.bottom;
  }

  return NPTEST_INT32_ERROR;
}

int32_t
pluginGetClipRegionRectCount(InstanceData* instanceData)
{
  return 1;
}

int32_t
pluginGetClipRegionRectEdge(InstanceData* instanceData, 
    int32_t rectIndex, RectEdge edge)
{
  return pluginGetEdge(instanceData, edge);
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



LRESULT CALLBACK PluginWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC wndProc = (WNDPROC)GetProp(hWnd, "MozillaWndProc");
  if (!wndProc)
    return 0;
  InstanceData* pInstance = (InstanceData*)GetProp(hWnd, "InstanceData");
  if (!pInstance)
    return 0;

  if (uMsg == WM_PAINT) {
    pluginDraw(pInstance);
    return 0;
  }

  if (uMsg == WM_CLOSE) {
    ClearSubclass((HWND)pInstance->window.window);
  }

  return CallWindowProc(wndProc, hWnd, uMsg, wParam, lParam);
}

void
ClearSubclass(HWND hWnd)
{
  if (GetProp(hWnd, "MozillaWndProc")) {
    ::SetWindowLong(hWnd, GWL_WNDPROC, (long)GetProp(hWnd, "MozillaWndProc"));
    RemoveProp(hWnd, "MozillaWndProc");
    RemoveProp(hWnd, "InstanceData");
  }
}

void
SetSubclass(HWND hWnd, InstanceData* instanceData)
{
  
  SetProp(hWnd, "InstanceData", (HANDLE)instanceData);
  WNDPROC origProc = (WNDPROC)::SetWindowLong(hWnd, GWL_WNDPROC, (long)PluginWndProc);
  SetProp(hWnd, "MozillaWndProc", (HANDLE)origProc);
}



Color
GetColorsFromRGBA(PRUint32 rgba)
{
  BYTE r, g, b, a;
  b = (rgba & 0xFF);
  g = ((rgba & 0xFF00) >> 8);
  r = ((rgba & 0xFF0000) >> 16);
  a = ((rgba & 0xFF000000) >> 24);
  return Color(a, r, g, b);
}

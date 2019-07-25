



































#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gdiplus.h>

using namespace Gdiplus;


static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
  UINT  num = 0;          
  UINT  size = 0;         

  ImageCodecInfo* pImageCodecInfo = NULL;

  GetImageEncodersSize(&num, &size);
  if(size == 0)
    return -1;  

  pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
  if(pImageCodecInfo == NULL)
    return -1;  

  GetImageEncoders(num, size, pImageCodecInfo);

  for(UINT j = 0; j < num; ++j)
    {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
        {
          *pClsid = pImageCodecInfo[j].Clsid;
          free(pImageCodecInfo);
          return j;  
        }    
    }

  free(pImageCodecInfo);
  return -1;  
}

#ifdef __MINGW32__
extern "C"
#endif
int wmain(int argc, wchar_t** argv)
{
  GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

  HWND desktop = GetDesktopWindow();
  HDC desktopdc = GetDC(desktop);
  HDC mydc = CreateCompatibleDC(desktopdc);
  int width = GetSystemMetrics(SM_CXSCREEN);
  int height = GetSystemMetrics(SM_CYSCREEN);
  HBITMAP mybmp = CreateCompatibleBitmap(desktopdc, width, height);
  HBITMAP oldbmp = (HBITMAP)SelectObject(mydc, mybmp);
  BitBlt(mydc,0,0,width,height,desktopdc,0,0, SRCCOPY|CAPTUREBLT);
  SelectObject(mydc, oldbmp);

  const wchar_t* filename = (argc > 1) ? argv[1] : L"screenshot.png";
  Bitmap* b = Bitmap::FromHBITMAP(mybmp, NULL);
  CLSID  encoderClsid;
  Status stat = GenericError;
  if (b && GetEncoderClsid(L"image/png", &encoderClsid) != -1) {
    stat = b->Save(filename, &encoderClsid, NULL);
  }
  if (b)
    delete b;
  
  
  GdiplusShutdown(gdiplusToken);
  ReleaseDC(desktop, desktopdc);
  DeleteObject(mybmp);
  DeleteDC(mydc);
  return stat == Ok ? 0 : 1;
}







































#include "nsSystemFontsWin.h"
#include "gfxWindowsSurface.h"

nsresult nsSystemFontsWin::CopyLogFontToNSFont(HDC* aHDC, const LOGFONTW* ptrLogFont,
                                               nsString *aFontName,
                                               gfxFontStyle *aFontStyle) const
{
  PRUnichar name[LF_FACESIZE];
  name[0] = 0;
  memcpy(name, ptrLogFont->lfFaceName, LF_FACESIZE*sizeof(PRUnichar));
  *aFontName = name;

  
  aFontStyle->style = FONT_STYLE_NORMAL;
  if (ptrLogFont->lfItalic)
  {
    aFontStyle->style = FONT_STYLE_ITALIC;
  }
  

  
  aFontStyle->weight = (ptrLogFont->lfWeight == FW_BOLD ? 
            FONT_WEIGHT_BOLD : FONT_WEIGHT_NORMAL);

  
  aFontStyle->stretch = NS_FONT_STRETCH_NORMAL;

  
  float mPixelScale = 1.0f;
  
  
  
  
  
  
  
  
  
  
  
  float pixelHeight = -ptrLogFont->lfHeight;
  if (pixelHeight < 0) {
    HFONT hFont = ::CreateFontIndirectW(ptrLogFont);
    if (!hFont)
      return NS_ERROR_OUT_OF_MEMORY;
    HGDIOBJ hObject = ::SelectObject(*aHDC, hFont);
    TEXTMETRIC tm;
    ::GetTextMetrics(*aHDC, &tm);
    ::SelectObject(*aHDC, hObject);
    ::DeleteObject(hFont);
    pixelHeight = tm.tmAscent;
  }

  pixelHeight *= mPixelScale;

  
  
  
  
  if ((pixelHeight < 12) && 
      (936 == ::GetACP())) 
    pixelHeight = 12;

  aFontStyle->size = pixelHeight;
  return NS_OK;
}

nsresult nsSystemFontsWin::GetSysFontInfo(HDC aHDC, nsSystemFontID anID,
                                          nsString *aFontName,
                                          gfxFontStyle *aFontStyle) const
{
  HGDIOBJ hGDI;

  LOGFONTW logFont;
  LOGFONTW* ptrLogFont = NULL;

  NONCLIENTMETRICSW ncm;

  BOOL status;
  if (anID == eSystemFont_Icon) 
  {
    status = ::SystemParametersInfoW(SPI_GETICONTITLELOGFONT,
                                     sizeof(logFont),
                                     (PVOID)&logFont,
                                     0);
  }
  else
  {
    ncm.cbSize = sizeof(NONCLIENTMETRICSW);
    status = ::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 
                                     sizeof(ncm),  
                                     (PVOID)&ncm, 
                                     0);
  }

  if (!status)
  {
    return NS_ERROR_FAILURE;
  }

  switch (anID)
  {
    
    
    
    

    case eSystemFont_Icon: 
      ptrLogFont = &logFont;
      break;

    case eSystemFont_Menu: 
      ptrLogFont = &ncm.lfMenuFont;
      break;

    case eSystemFont_MessageBox: 
      ptrLogFont = &ncm.lfMessageFont;
      break;

    case eSystemFont_SmallCaption: 
      ptrLogFont = &ncm.lfSmCaptionFont;
      break;

    case eSystemFont_StatusBar: 
    case eSystemFont_Tooltips: 
      ptrLogFont = &ncm.lfStatusFont;
      break;

    case eSystemFont_Widget:

    case eSystemFont_Window:      
    case eSystemFont_Document:
    case eSystemFont_Workspace:
    case eSystemFont_Desktop:
    case eSystemFont_Info:
    case eSystemFont_Dialog:
    case eSystemFont_Button:
    case eSystemFont_PullDownMenu:
    case eSystemFont_List:
    case eSystemFont_Field:
    case eSystemFont_Caption: 
      hGDI = ::GetStockObject(DEFAULT_GUI_FONT);
      if (hGDI != NULL)
      {
        if (::GetObjectW(hGDI, sizeof(logFont), &logFont) > 0)
        { 
          ptrLogFont = &logFont;
        }
      }
      break;
  } 

  if (nsnull == ptrLogFont)
  {
    return NS_ERROR_FAILURE;
  }

  aFontStyle->systemFont = true;

  return CopyLogFontToNSFont(&aHDC, ptrLogFont, aFontName, aFontStyle);
}

nsresult nsSystemFontsWin::GetSystemFont(nsSystemFontID anID,
                                         nsString *aFontName,
                                         gfxFontStyle *aFontStyle) const
{
  nsresult status = NS_OK;

  switch (anID) {
    case eSystemFont_Caption: 
    case eSystemFont_Icon: 
    case eSystemFont_Menu: 
    case eSystemFont_MessageBox: 
    case eSystemFont_SmallCaption: 
    case eSystemFont_StatusBar: 
    case eSystemFont_Tooltips: 
    case eSystemFont_Widget:

    case eSystemFont_Window:      
    case eSystemFont_Document:
    case eSystemFont_Workspace:
    case eSystemFont_Desktop:
    case eSystemFont_Info:
    case eSystemFont_Dialog:
    case eSystemFont_Button:
    case eSystemFont_PullDownMenu:
    case eSystemFont_List:
    case eSystemFont_Field:
    {
      HWND hwnd = nsnull;
      HDC tdc = GetDC(hwnd);

      status = GetSysFontInfo(tdc, anID, aFontName, aFontStyle);

      ReleaseDC(hwnd, tdc);

      break;
    }
  }

  return status;
}

nsSystemFontsWin::nsSystemFontsWin()
{

}


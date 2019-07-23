




































#include "nsDeviceContextWin.h"
#include "nsRenderingContextWin.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIScreenManager.h"
#include "nsIScreen.h"
#include "nsGfxCIID.h"
#include "nsReadableUtils.h"
#include "nsISupportsPrimitives.h"
#include "nsString.h"

#if defined(DEBUG_rods) && defined(MOZ_LAYOUTDEBUG)
#include "nsIDebugObject.h"
#endif

#define DOC_TITLE_LENGTH      64

#include "prlog.h"
#ifdef PR_LOGGING 
PRLogModuleInfo * kGfxPrintingLogMod = PR_NewLogModule("printing-gfx");
#define PR_PL(_p1)  PR_LOG(kGfxPrintingLogMod, PR_LOG_DEBUG, _p1)
#else
#define PR_PL(_p1)
#endif

nsDeviceContextWin :: nsDeviceContextWin()
  : DeviceContextImpl()
{
  mSurface = NULL;
  mPaletteInfo.isPaletteDevice = PR_FALSE;
  mPaletteInfo.sizePalette = 0;
  mPaletteInfo.numReserved = 0;
  mPaletteInfo.palette = NULL;
  mDC = NULL;
  mPixelScale = 1.0f;
  mWidth = -1;
  mHeight = -1;
  mSpec = nsnull;
  mCachedClientRect = PR_FALSE;
  mCachedFullRect = PR_FALSE;
}

nsDeviceContextWin :: ~nsDeviceContextWin()
{
  nsDrawingSurfaceWin *surf = (nsDrawingSurfaceWin *)mSurface;

  NS_IF_RELEASE(surf);    
  mSurface = nsnull;

  if (NULL != mPaletteInfo.palette)
    ::DeleteObject((HPALETTE)mPaletteInfo.palette);

  if (NULL != mDC)
  {
    ::DeleteDC(mDC);
    mDC = NULL;
  }

  NS_IF_RELEASE(mSpec);
}

NS_IMETHODIMP nsDeviceContextWin :: Init(nsNativeWidget aWidget)
{
  mWidget = aWidget;

  HWND  hwnd = (HWND)aWidget;
  HDC   hdc = ::GetDC(hwnd);

  CommonInit(hdc);

  ::ReleaseDC(hwnd, hdc);

  return NS_OK;
}



nsresult nsDeviceContextWin :: Init(nsNativeDeviceContext aContext, nsIDeviceContext *aOrigContext)
{
  float origscale, newscale;
  float t2d, a2d;

  mDC = (HDC)aContext;

  CommonInit(mDC);


  newscale = TwipsToDevUnits();
  origscale = aOrigContext->TwipsToDevUnits();

  mPixelScale = newscale / origscale;

  t2d = aOrigContext->TwipsToDevUnits();
  a2d = aOrigContext->AppUnitsToDevUnits();

  mAppUnitsToDevUnits = (a2d / t2d) * mTwipsToPixels;
  mDevUnitsToAppUnits = 1.0f / mAppUnitsToDevUnits;

  return NS_OK;
}

void nsDeviceContextWin :: CommonInit(HDC aDC)
{
  int   rasterCaps = ::GetDeviceCaps(aDC, RASTERCAPS);

  mDepth = (PRUint32)::GetDeviceCaps(aDC, BITSPIXEL);
  mPaletteInfo.isPaletteDevice = RC_PALETTE == (rasterCaps & RC_PALETTE);
  mPaletteInfo.sizePalette = (PRUint16)::GetDeviceCaps(aDC, SIZEPALETTE);
  mPaletteInfo.numReserved = (PRUint16)::GetDeviceCaps(aDC, NUMRESERVED);

  mWidth = ::GetDeviceCaps(aDC, HORZRES);
  mHeight = ::GetDeviceCaps(aDC, VERTRES);

  mPixelsToTwips = (float)NSIntPointsToTwips(72) / ((float)::GetDeviceCaps(aDC, LOGPIXELSY));
  if (::GetDeviceCaps(aDC, TECHNOLOGY) == DT_RASDISPLAY)
  {
    
    mPixelsToTwips = NSToIntRound(mPixelsToTwips);

    
    
    nsresult ignore;
    mScreenManager = do_GetService("@mozilla.org/gfx/screenmanager;1", &ignore);   
  } 
  mTwipsToPixels = 1.0 / mPixelsToTwips;

  DeviceContextImpl::CommonInit();
}


void
nsDeviceContextWin :: ComputeClientRectUsingScreen ( nsRect* outRect )
{
  
  
  
  
  nsCOMPtr<nsIScreen> screen;
  FindScreen ( getter_AddRefs(screen) );
  if ( screen ) {
    PRInt32 x, y, width, height;
    screen->GetAvailRect ( &x, &y, &width, &height );
  
    
    outRect->y = NSToIntRound(y * mDevUnitsToAppUnits);
    outRect->x = NSToIntRound(x * mDevUnitsToAppUnits);
    outRect->width = NSToIntRound(width * mDevUnitsToAppUnits);
    outRect->height = NSToIntRound(height * mDevUnitsToAppUnits);

    mCachedClientRect = PR_TRUE;
    mClientRect = *outRect;
  }

} 


void
nsDeviceContextWin :: ComputeFullAreaUsingScreen ( nsRect* outRect )
{
  
  
  
  
  nsCOMPtr<nsIScreen> screen;
  FindScreen ( getter_AddRefs(screen) );
  if ( screen ) {
    PRInt32 x, y, width, height;
    screen->GetRect ( &x, &y, &width, &height );
  
    
    outRect->y = NSToIntRound(y * mDevUnitsToAppUnits);
    outRect->x = NSToIntRound(x * mDevUnitsToAppUnits);
    outRect->width = NSToIntRound(width * mDevUnitsToAppUnits);
    outRect->height = NSToIntRound(height * mDevUnitsToAppUnits);

    mWidth = width;
    mHeight = height;
    mCachedFullRect = PR_TRUE;
  }
 
} 







void
nsDeviceContextWin :: FindScreen ( nsIScreen** outScreen )
{
  
  
  HWND window = reinterpret_cast<HWND>(mWidget);
  if ( window ) {
    RECT globalPosition;
    ::GetWindowRect ( window, &globalPosition ); 
    if ( mScreenManager )
      mScreenManager->ScreenForRect ( globalPosition.left, globalPosition.top, 
                                       globalPosition.right - globalPosition.left,
                                       globalPosition.bottom - globalPosition.top, outScreen );
  }

} 


static NS_DEFINE_CID(kRCCID,NS_RENDERING_CONTEXT_CID);

NS_IMETHODIMP nsDeviceContextWin :: CreateRenderingContext(nsIRenderingContext *&aContext)
{
#ifdef NS_PRINT_PREVIEW
  
  if (mAltDC && ((mUseAltDC & kUseAltDCFor_CREATERC_PAINT) || (mUseAltDC & kUseAltDCFor_CREATERC_REFLOW))) {
    return mAltDC->CreateRenderingContext(aContext);
  }
#endif

  nsIRenderingContext *pContext;
  nsresult             rv;
  nsDrawingSurfaceWin  *surf;

  rv = CallCreateInstance(kRCCID, &pContext);

  if ( (NS_SUCCEEDED(rv)) && (nsnull != pContext))
  {
    surf = new nsDrawingSurfaceWin();

    if (nsnull != surf)
    {
      rv = surf->Init(mDC);

      if (NS_OK == rv)
        rv = pContext->Init(this, surf);
    }
    else
      rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else
    rv = NS_ERROR_OUT_OF_MEMORY;

  if (NS_OK != rv)
  {
    NS_IF_RELEASE(pContext);
  }

  aContext = pContext;

  return rv;
}

NS_IMETHODIMP nsDeviceContextWin :: SupportsNativeWidgets(PRBool &aSupportsWidgets)
{
  if (nsnull == mDC)
    aSupportsWidgets = PR_TRUE;
  else
    aSupportsWidgets = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextWin :: GetCanonicalPixelScale(float &aScale) const
{
  aScale = mPixelScale;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextWin :: SetCanonicalPixelScale(float aScale)
{
  DeviceContextImpl::SetCanonicalPixelScale(aScale);
  mPixelScale = aScale;
  return NS_OK;
}


nsresult nsDeviceContextWin::CopyLogFontToNSFont(HDC* aHDC, const LOGFONT* ptrLogFont,
                                                 nsFont* aFont, PRBool aIsWide) const
{
  PRUnichar name[LF_FACESIZE];
  name[0] = 0;
  if (aIsWide)
    memcpy(name, ptrLogFont->lfFaceName, LF_FACESIZE*2);
  else {
    MultiByteToWideChar(CP_ACP, 0, ptrLogFont->lfFaceName,
      strlen(ptrLogFont->lfFaceName) + 1, name, sizeof(name)/sizeof(name[0]));
  }
  aFont->name = name;

  
  aFont->style = NS_FONT_STYLE_NORMAL;
  if (ptrLogFont->lfItalic)
  {
    aFont->style = NS_FONT_STYLE_ITALIC;
  }
  

  aFont->variant = NS_FONT_VARIANT_NORMAL;

  
  aFont->weight = (ptrLogFont->lfWeight == FW_BOLD ? 
            NS_FONT_WEIGHT_BOLD : NS_FONT_WEIGHT_NORMAL);

  
  aFont->decorations = NS_FONT_DECORATION_NONE;
  if (ptrLogFont->lfUnderline)
  {
    aFont->decorations |= NS_FONT_DECORATION_UNDERLINE;
  }
  if (ptrLogFont->lfStrikeOut)
  {
    aFont->decorations |= NS_FONT_DECORATION_LINE_THROUGH;
  }

  
  
  
  
  
  
  
  
  
  
  
  float pixelHeight = -ptrLogFont->lfHeight;
  if (pixelHeight < 0) {
    HFONT hFont = ::CreateFontIndirect(ptrLogFont);
    if (!hFont)
      return NS_ERROR_OUT_OF_MEMORY;
    HGDIOBJ hObject = ::SelectObject(*aHDC, hFont);
    TEXTMETRIC tm;
    ::GetTextMetrics(*aHDC, &tm);
    ::SelectObject(*aHDC, hObject);
    ::DeleteObject(hFont);
    pixelHeight = tm.tmAscent;
  }

  float pointSize = pixelHeight * mPixelScale * 72 / ::GetDeviceCaps(*aHDC, LOGPIXELSY);

  
  
  
  if ((pointSize < 9) && 
      (936 == ::GetACP())) 
    pointSize = 9;

  aFont->size = NSFloatPointsToTwips(pointSize);
  return NS_OK;
}

nsresult nsDeviceContextWin :: GetSysFontInfo(HDC aHDC, nsSystemFontID anID, nsFont* aFont) const
{
  HGDIOBJ hGDI;

  LOGFONT logFont;
  LOGFONT* ptrLogFont = NULL;

#ifdef WINCE
  hGDI = ::GetStockObject(SYSTEM_FONT);
  if (hGDI == NULL)
    return NS_ERROR_UNEXPECTED;
  
  if (::GetObject(hGDI, sizeof(logFont), &logFont) > 0)
    ptrLogFont = &logFont;
#else

  NONCLIENTMETRICS ncm;

  BOOL status;
  if (anID == eSystemFont_Icon) 
  {
    status = ::SystemParametersInfo(SPI_GETICONTITLELOGFONT,
                                  sizeof(logFont),
                                  (PVOID)&logFont,
                                  0);
  }
  else
  {
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    status = ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 
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
        if (::GetObject(hGDI, sizeof(logFont), &logFont) > 0)
        { 
          ptrLogFont = &logFont;
        }
      }
      break;
  } 

#endif 

  if (nsnull == ptrLogFont)
  {
    return NS_ERROR_FAILURE;
  }

  aFont->systemFont = PR_TRUE;

  return CopyLogFontToNSFont(&aHDC, ptrLogFont, aFont);
}

NS_IMETHODIMP nsDeviceContextWin :: GetSystemFont(nsSystemFontID anID, nsFont *aFont) const
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
      HWND  hwnd;
      HDC   tdc;

      if (nsnull == mDC)
      {
        hwnd = (HWND)mWidget;
        tdc = ::GetDC(hwnd);
      }
      else
        tdc = mDC;

      status = GetSysFontInfo(tdc, anID, aFont);

      if (nsnull == mDC)
        ::ReleaseDC(hwnd, tdc);

      break;
    }
  }

  return status;
}

NS_IMETHODIMP nsDeviceContextWin :: GetDrawingSurface(nsIRenderingContext &aContext, nsIDrawingSurface* &aSurface)
{
  if (NULL == mSurface) {
    nsRect empty(0,0,0,0); 
    aContext.CreateDrawingSurface(empty, 0, mSurface);
  }

  aSurface = mSurface;
  return NS_OK;
}

int CALLBACK fontcallback(ENUMLOGFONT FAR *lpelf, NEWTEXTMETRIC FAR *lpntm,
                          int FontType, LPARAM lParam)  
{
  if (NULL != lpelf)
    *((PRBool *)lParam) = PR_TRUE;

  return 0;
}

NS_IMETHODIMP nsDeviceContextWin :: CheckFontExistence(const nsString& aFontName)
{
  HWND    hwnd = (HWND)mWidget;
  HDC     hdc = ::GetDC(hwnd);
  PRBool  isthere = PR_FALSE;

  LOGFONT logFont;
  logFont.lfCharSet = DEFAULT_CHARSET;
  logFont.lfPitchAndFamily = 0;
  int outlen = WideCharToMultiByte(CP_ACP, 0, aFontName.get(), aFontName.Length() + 1,
                                   logFont.lfFaceName, sizeof(logFont.lfFaceName), nsnull, nsnull);

  
  if(0 == outlen) {
    nsFixedCString logFontStr(logFont.lfFaceName, LF_FACESIZE);
    LossyCopyUTF16toASCII(aFontName, logFontStr);
    if (logFontStr.get() != logFont.lfFaceName) {
      return NS_ERROR_FAILURE; 
    }
  }

  ::EnumFontFamiliesEx(hdc, &logFont, (FONTENUMPROC)fontcallback, (LPARAM)&isthere, 0);

  ::ReleaseDC(hwnd, hdc);

  if (PR_TRUE == isthere)
    return NS_OK;
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsDeviceContextWin::GetDepth(PRUint32& aDepth)
{
  aDepth = mDepth;
  return NS_OK;
}


NS_IMETHODIMP nsDeviceContextWin::GetPaletteInfo(nsPaletteInfo& aPaletteInfo)
{
  aPaletteInfo.isPaletteDevice = mPaletteInfo.isPaletteDevice;
  aPaletteInfo.sizePalette = mPaletteInfo.sizePalette;
  aPaletteInfo.numReserved = mPaletteInfo.numReserved;

  if (NULL == mPaletteInfo.palette) {
#ifndef WINCE
    HWND    hwnd = (HWND)mWidget;
    HDC     hdc = ::GetDC(hwnd);
    mPaletteInfo.palette = ::CreateHalftonePalette(hdc);  
    ::ReleaseDC(hwnd, hdc);                                                     
#else
    mPaletteInfo.palette = (HPALETTE) GetStockObject(DEFAULT_PALETTE);
#endif
  }

  aPaletteInfo.palette = mPaletteInfo.palette;
                                         
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextWin :: GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight)
{
#ifdef NS_PRINT_PREVIEW
  
  if (mAltDC && (mUseAltDC & kUseAltDCFor_SURFACE_DIM)) {
    return mAltDC->GetDeviceSurfaceDimensions(aWidth, aHeight);
  }
#endif

  if ( mSpec )
  {
    
    aWidth = NSToIntRound(mWidth * mDevUnitsToAppUnits);
    aHeight = NSToIntRound(mHeight * mDevUnitsToAppUnits);
  } 
  else 
  {
    nsRect area;
    ComputeFullAreaUsingScreen ( &area );
    aWidth = area.width;
    aHeight = area.height;
  }

  return NS_OK;
}


NS_IMETHODIMP nsDeviceContextWin :: GetRect(nsRect &aRect)
{
  if ( mSpec )
  {
    
    aRect.x = 0;
    aRect.y = 0;
    aRect.width = NSToIntRound(mWidth * mDevUnitsToAppUnits);
    aRect.height = NSToIntRound(mHeight * mDevUnitsToAppUnits);
  }
  else
    ComputeFullAreaUsingScreen ( &aRect );

  return NS_OK;
}


NS_IMETHODIMP nsDeviceContextWin :: GetClientRect(nsRect &aRect)
{
  if ( mSpec )
  {
    
    aRect.x = 0;
    aRect.y = 0;
    aRect.width = NSToIntRound(mWidth * mDevUnitsToAppUnits);
    aRect.height = NSToIntRound(mHeight * mDevUnitsToAppUnits);
  }
  else
    ComputeClientRectUsingScreen ( &aRect );

  return NS_OK;
}

BOOL CALLBACK abortproc( HDC hdc, int iError )
{
  return TRUE;
} 
 
NS_IMETHODIMP nsDeviceContextWin :: GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                                        nsIDeviceContext *&aContext)
{
  nsDeviceContextWin* devConWin = new nsDeviceContextWin(); 
  if (devConWin != nsnull) {
    
    nsresult rv = devConWin->QueryInterface(NS_GET_IID(nsIDeviceContext), (void**)&aContext);
    NS_ASSERTION(NS_SUCCEEDED(rv), "This has to support nsIDeviceContext");
  } else {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  devConWin->mSpec = aDevice;

  NS_ADDREF(aDevice);

  HDC dc = NULL;
  nsCOMPtr<nsISupportsVoid> supVoid = do_QueryInterface(aDevice);
  supVoid->GetData((void**)&dc);
 
  return devConWin->Init(dc, this); 
}

#if defined(DEBUG_rods) || defined(DEBUG_dcone)
static void DisplayLastError()
{
  LPVOID lpMsgBuf;
  DWORD errCode = GetLastError();

  FormatMessage( 
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
      (LPTSTR) &lpMsgBuf,
      0,
      NULL 
  );

  
  MessageBox( NULL, (const char *)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );
}
#define DISPLAY_LAST_ERROR DisplayLastError();
#else
#define DISPLAY_LAST_ERROR 
#endif


NS_IMETHODIMP nsDeviceContextWin :: BeginDocument(PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage)
{
  nsresult rv = NS_ERROR_GFX_PRINTER_STARTDOC;

  if (NULL != mDC) {
    DOCINFO docinfo;

    nsString titleStr;
    titleStr = aTitle;
    if (titleStr.Length() > DOC_TITLE_LENGTH) {
      titleStr.SetLength(DOC_TITLE_LENGTH-3);
      titleStr.AppendLiteral("...");
    }
    char *title = GetACPString(titleStr);

    char* docName = nsnull;
    nsAutoString str(aPrintToFileName);
    if (!str.IsEmpty()) {
      docName = ToNewCString(str);
    }
    docinfo.cbSize = sizeof(docinfo);
    docinfo.lpszDocName = title != nsnull?title:"Mozilla Document";

#ifdef DEBUG_rods
    docinfo.lpszOutput = "\\p.ps";

#ifdef MOZ_LAYOUTDEBUG
    
    char * tempFileName = nsnull;
    nsCOMPtr<nsIDebugObject>debugObj = do_GetService("@mozilla.org/debug/debugobject;1");
    if (debugObj) {
      PRBool isDoingTests;
      if (NS_SUCCEEDED(debugObj->GetDoRuntimeTests(&isDoingTests)) && isDoingTests) {
        PRUnichar * name;
        debugObj->GetPrintFileName(&name);
        if (name) {
          if (*name) {
            nsCString cStrName;
            cStrName.AssignWithConversion(name);
            tempFileName = ToNewCString(cStrName);
          }
          nsMemory::Free(name);
        }
        docinfo.lpszOutput = tempFileName;
      }
    }
#endif

#else
    docinfo.lpszOutput = docName;
#endif

    docinfo.lpszDatatype = NULL;
    docinfo.fwType = 0;

    if (::StartDoc(mDC, &docinfo) > 0) {
      rv = NS_OK;
    } else {
      DISPLAY_LAST_ERROR
      rv = NS_ERROR_GFX_PRINTER_STARTDOC;
      PR_PL(("nsDeviceContextWin::BeginDocument - StartDoc Error!\n"));
    }

    if (title != nsnull) delete [] title;
    if (docName != nsnull) nsMemory::Free(docName);

#if defined(DEBUG_rods) && defined(MOZ_LAYOUTDEBUG)
    if (tempFileName) {
      nsMemory::Free(tempFileName);
    }
#endif
  }

  return rv;
}

NS_IMETHODIMP nsDeviceContextWin :: EndDocument(void)
{
  if (NULL != mDC)
  {
    if (::EndDoc(mDC) > 0) {
      return NS_OK;
    } else {
      DISPLAY_LAST_ERROR
      PR_PL(("nsDeviceContextWin::EndDocument - EndDoc Error!\n"));
      return NS_ERROR_GFX_PRINTER_ENDDOC;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextWin :: AbortDocument(void)
{
  if (NULL != mDC)
  {
    if (::AbortDoc(mDC) > 0) {
      return NS_OK;
    } else {
      DISPLAY_LAST_ERROR
      PR_PL(("nsDeviceContextWin::AbortDocument - AbortDoc Error!\n"));
      return NS_ERROR_ABORT;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextWin :: BeginPage(void)
{
  if (NULL != mDC)
  {
    if (::StartPage(mDC) > 0)
      return NS_OK;
    else {
      DISPLAY_LAST_ERROR
      PR_PL(("nsDeviceContextWin::BeginPage - StartPage Error!\n"));
      return NS_ERROR_GFX_PRINTER_STARTPAGE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextWin :: EndPage(void)
{
  if (NULL != mDC)
  {
    if (::EndPage(mDC) > 0) {
      return NS_OK;
    } else {
      DISPLAY_LAST_ERROR
      PR_PL(("nsDeviceContextWin::EndPage - EndPage Error!\n"));
      return NS_ERROR_GFX_PRINTER_ENDPAGE;
    }
  }

  return NS_OK;
}

char*
nsDeviceContextWin :: GetACPString(const nsAString& aStr)
{
   int acplen = aStr.Length() * 2 + 1;
   char * acp = new char[acplen];
   if(acp)
   {
      int outlen = ::WideCharToMultiByte( CP_ACP, 0, 
                      PromiseFlatString(aStr).get(), aStr.Length(),
                      acp, acplen, NULL, NULL);
      if ( outlen > 0)
         acp[outlen] = '\0';  
   }
   return acp;
}

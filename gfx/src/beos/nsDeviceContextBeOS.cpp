






































#include <math.h>
#include <Menu.h>
#include "nspr.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "nsCRT.h"
#include "nsReadableUtils.h"

#include "nsDeviceContextBeOS.h"
#include "nsFontMetricsBeOS.h"
#include "nsGfxCIID.h"

#include <ScrollBar.h>
#include <Screen.h>

#include "nsIScreenManager.h"

nscoord nsDeviceContextBeOS::mDpi = 96; 

nsDeviceContextBeOS::nsDeviceContextBeOS()
  : DeviceContextImpl()
{
  mTwipsToPixels = 1.0;
  mPixelsToTwips = 1.0;
  mDepth = 0 ;
  mNumCells = 0;
  
  mWidthFloat = 0.0f; 
  mHeightFloat = 0.0f; 
  mWidth = -1; 
  mHeight = -1; 
}

nsDeviceContextBeOS::~nsDeviceContextBeOS()
{
  nsresult rv; 
  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID, &rv); 
  if (NS_SUCCEEDED(rv))
    prefs->UnregisterCallback("layout.css.dpi", prefChanged, (void *)this); 
}

NS_IMETHODIMP nsDeviceContextBeOS::Init(nsNativeWidget aNativeWidget)
{
  
  

  nsresult ignore; 
  mWidget = aNativeWidget;
  nsCOMPtr<nsIScreenManager> sm ( do_GetService("@mozilla.org/gfx/screenmanager;1", &ignore) ); 
  if (sm) 
  { 
    nsCOMPtr<nsIScreen> screen; 
    sm->GetPrimaryScreen(getter_AddRefs(screen)); 
    if (screen)
    { 
      PRInt32 x, y, width, height, depth; 
      screen->GetAvailRect ( &x, &y, &width, &height ); 
      screen->GetPixelDepth ( &depth ); 
      mWidthFloat = float(width); 
      mHeightFloat = float(height); 
      mDepth = NS_STATIC_CAST ( PRUint32, depth ); 
    } 
  } 
  
  static int initialized = 0; 
  if (!initialized) 
  {
    initialized = 1; 

    
    
    
    
    
    PRInt32 prefVal = -1; 
    nsresult res; 

    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &res)); 
    if (NS_SUCCEEDED(res) && prefs)
    { 
      res = prefs->GetIntPref("layout.css.dpi", &prefVal); 
      if (! NS_SUCCEEDED(res))
      { 
        prefVal = -1; 
      } 
      prefs->RegisterCallback("layout.css.dpi", prefChanged, (void *)this); 
    } 
 
    
    
    PRInt32 OSVal = 85;
    if (prefVal > 0)
    { 
      
      
      mDpi = prefVal; 
    }
    else if ((prefVal == 0) || (OSVal > 96))
    { 
      
      
      mDpi = OSVal; 
    }
    else
    { 
      
      
      mDpi = 96; 
    } 
  } 
 
  SetDPI(mDpi); 

  menu_info info;
  get_menu_info(&info);
  mMenuFont.SetFamilyAndStyle(info.f_family,info.f_style);
  mMenuFont.SetSize(info.font_size);
  
#ifdef DEBUG 
  static PRBool once = PR_TRUE; 
  if (once)
  { 
    printf("GFX: dpi=%d t2p=%g p2t=%g depth=%d\n", mDpi, mTwipsToPixels, mPixelsToTwips,mDepth); 
    once = PR_FALSE; 
  } 
#endif 

  DeviceContextImpl::CommonInit();
  
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextBeOS::CreateRenderingContext(nsIRenderingContext *&aContext) 
{ 
  nsIRenderingContext *pContext; 
  nsresult             rv; 
  nsDrawingSurfaceBeOS  *surf; 
  BView *w; 

  w = (BView*)mWidget;

  
  pContext = new nsRenderingContextBeOS(); 
 
  if (nsnull != pContext) 
  { 
    NS_ADDREF(pContext); 
 
    
    surf = new nsDrawingSurfaceBeOS(); 
 
    if (surf && w) 
    {
      
      rv = surf->Init(w);
      if (NS_OK == rv)
        
        rv = pContext->Init(this, surf);
    } 
    else
    {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }
  else
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }

  if (NS_OK != rv)
    NS_IF_RELEASE(pContext); 

  aContext = pContext; 
 
  return rv; 
} 
 
NS_IMETHODIMP nsDeviceContextBeOS::SupportsNativeWidgets(PRBool &aSupportsWidgets)
{
  
  
  aSupportsWidgets = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextBeOS::GetSystemFont(nsSystemFontID aID, nsFont *aFont) const
{
  nsresult status = NS_OK;

  switch (aID) 
  {
    case eSystemFont_PullDownMenu: 
    case eSystemFont_Menu:
      status = GetSystemFontInfo(&mMenuFont, aID, aFont); 
      break;
    case eSystemFont_Caption:             
      status = GetSystemFontInfo(be_bold_font, aID, aFont); 
      break;
    case eSystemFont_List:   
    case eSystemFont_Field:
    case eSystemFont_Icon : 
    case eSystemFont_MessageBox : 
    case eSystemFont_SmallCaption : 
    case eSystemFont_StatusBar : 
    case eSystemFont_Window:              
    case eSystemFont_Document: 
    case eSystemFont_Workspace: 
    case eSystemFont_Desktop: 
    case eSystemFont_Info: 
    case eSystemFont_Dialog: 
    case eSystemFont_Button: 
    case eSystemFont_Tooltips:            
    case eSystemFont_Widget: 
    default:
      status = GetSystemFontInfo(be_plain_font, aID, aFont);
  }

  return status;
}

NS_IMETHODIMP nsDeviceContextBeOS::CheckFontExistence(const nsString& aFontName)
{
  return nsFontMetricsBeOS::FamilyExists(aFontName); 
} 


































NS_IMETHODIMP nsDeviceContextBeOS::GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight)
{
  if (mWidth == -1) 
    mWidth = NSToIntRound(mWidthFloat * mDevUnitsToAppUnits);

  if (mHeight == -1) 
    mHeight = NSToIntRound(mHeightFloat * mDevUnitsToAppUnits); 
 
  aWidth = mWidth; 
  aHeight = mHeight; 
 
  return NS_OK; 
}

NS_IMETHODIMP nsDeviceContextBeOS::GetRect(nsRect &aRect)
{
  PRInt32 width, height; 
  nsresult rv; 
  rv = GetDeviceSurfaceDimensions(width, height);
  aRect.x = 0;
  aRect.y = 0;
  aRect.width = width; 
  aRect.height = height; 
  return rv; 
} 
 
NS_IMETHODIMP nsDeviceContextBeOS::GetClientRect(nsRect &aRect) 
{ 

  return GetRect(aRect);
}

NS_IMETHODIMP nsDeviceContextBeOS::GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                                      nsIDeviceContext *&aContext)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsDeviceContextBeOS::BeginDocument(PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextBeOS::EndDocument(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextBeOS::BeginPage(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextBeOS::EndPage(void)
{
  return NS_OK;
} 
 
NS_IMETHODIMP nsDeviceContextBeOS::GetDepth(PRUint32& aDepth) 
{ 
  aDepth = mDepth; 
  return NS_OK; 
} 
 
nsresult 
nsDeviceContextBeOS::SetDPI(PRInt32 aDpi) 
{ 
  mDpi = aDpi; 
  
  int pt2t = 72; 

  mPixelsToTwips = float(NSToIntRound(float(NSIntPointsToTwips(pt2t)) / float(aDpi))); 
  mTwipsToPixels = 1.0f / mPixelsToTwips; 
 
  
  return NS_OK; 
} 
 
int nsDeviceContextBeOS::prefChanged(const char *aPref, void *aClosure) 
{ 
  nsDeviceContextBeOS *context = (nsDeviceContextBeOS*)aClosure; 
  nsresult rv; 
  
  if (nsCRT::strcmp(aPref, "layout.css.dpi")==0)
  {
    PRInt32 dpi; 
    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv)); 
    rv = prefs->GetIntPref(aPref, &dpi); 
    if (NS_SUCCEEDED(rv)) 
      context->SetDPI(dpi); 
  } 
  
  return 0; 
} 
 
nsresult 
nsDeviceContextBeOS::GetSystemFontInfo(const BFont *theFont, nsSystemFontID anID, nsFont* aFont) const 
{ 
  nsresult status = NS_OK; 
 
  aFont->style       = NS_FONT_STYLE_NORMAL; 
  aFont->weight      = NS_FONT_WEIGHT_NORMAL; 
  aFont->decorations = NS_FONT_DECORATION_NONE; 
  
  
  
  if (!theFont)
  {
    switch (anID) 
  	{
      case eSystemFont_Menu:
        status = GetSystemFontInfo(&mMenuFont, anID, aFont); 
        break;
      case eSystemFont_List:
      case eSystemFont_Field:
        theFont = be_plain_font;
        break;
      case eSystemFont_Caption:
        theFont = be_bold_font;
        break;
      default:
        theFont = be_plain_font; 
    }
  }
  
  if (!theFont) 
  { 
    status = NS_ERROR_FAILURE; 
  } 
  else 
  { 
    font_family family; 
    font_style style; 
    font_height height;
    uint16 face; 
 
    theFont->GetFamilyAndStyle(&family, &style);

    face = theFont->Face();
    aFont->name.Assign(NS_ConvertUTF8toUTF16(family));
    aFont->size = NSIntPixelsToTwips(uint32(theFont->Size()), mPixelsToTwips); 

    if(face & B_ITALIC_FACE)
      aFont->style = NS_FONT_STYLE_ITALIC;
    
    if(face & B_BOLD_FACE)
      aFont->weight = NS_FONT_WEIGHT_BOLD;

    if(face & B_UNDERSCORE_FACE)
      aFont->decorations |= NS_FONT_DECORATION_UNDERLINE;

    if(face & B_STRIKEOUT_FACE)
      aFont->decorations |= NS_FONT_DECORATION_LINE_THROUGH;

    aFont->systemFont = PR_TRUE;

    status = NS_OK; 
  } 
  return (status); 
}








































#include <math.h>

#include "nspr.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "nsCRT.h"

#include "nsDeviceContextGTK.h"
#include "nsGfxCIID.h"

#ifdef USE_POSTSCRIPT
#include "nsGfxPSCID.h"
#include "nsIDeviceContextPS.h"
#endif 
#ifdef USE_XPRINT
#include "nsGfxXPrintCID.h"
#include "nsIDeviceContextXPrint.h"
#endif 

#include "nsFontMetricsUtils.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#ifdef MOZ_WIDGET_GTK
#include "gdksuperwin.h"
#endif 

#ifdef MOZ_WIDGET_GTK2
#include <pango/pango.h>
#include <pango/pangox.h>
#include <pango/pango-fontmap.h>
#endif

#ifdef MOZ_ENABLE_XFT
#include "nsFontMetricsUtils.h"
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

static PRInt32 GetXftDPI(void);
#endif

#include <X11/Xatom.h>

#include "nsIDeviceContextSpec.h"

static PRInt32 GetOSDPI(void);

#define GDK_DEFAULT_FONT1 "-*-helvetica-medium-r-*--*-120-*-*-*-*-iso8859-1"
#define GDK_DEFAULT_FONT2 "-*-fixed-medium-r-*-*-*-120-*-*-*-*-*-*"

#ifdef MOZ_WIDGET_GTK

extern NS_IMPORT_(GdkFont *) default_font;
#endif 





class nsSystemFontsGTK {

  public:
    nsSystemFontsGTK(float aPixelsToTwips);

    const nsFont& GetDefaultFont() { return mDefaultFont; }
    const nsFont& GetMenuFont() { return mMenuFont; }
    const nsFont& GetFieldFont() { return mFieldFont; }
    const nsFont& GetButtonFont() { return mButtonFont; }

  private:
    nsresult GetSystemFontInfo(GtkWidget *aWidget, nsFont* aFont,
                               float aPixelsToTwips) const;

    















    nsFont mDefaultFont;
    nsFont mButtonFont;
    nsFont mFieldFont;
    nsFont mMenuFont;
};


nscoord nsDeviceContextGTK::mDpi = 96;
static nsSystemFontsGTK *gSystemFonts = nsnull;

nsDeviceContextGTK::nsDeviceContextGTK()
  : DeviceContextImpl()
{
  mTwipsToPixels = 1.0;
  mPixelsToTwips = 1.0;
  mDepth = 0 ;
  mNumCells = 0;

  mDeviceWindow = nsnull;
}

nsDeviceContextGTK::~nsDeviceContextGTK()
{
  nsresult rv;
  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    prefs->UnregisterCallback("layout.css.dpi",
                              prefChanged, (void *)this);
  }
}

 void nsDeviceContextGTK::Shutdown()
{
  if (gSystemFonts) {
    delete gSystemFonts;
    gSystemFonts = nsnull;
  }
}

NS_IMETHODIMP nsDeviceContextGTK::Init(nsNativeWidget aNativeWidget)
{
  GtkRequisition req;
  GtkWidget *sb;
  
  
  

  if (!mScreenManager)
    mScreenManager = do_GetService("@mozilla.org/gfx/screenmanager;1");
  if (!mScreenManager) {
    return NS_ERROR_FAILURE;
  }

#ifdef MOZ_WIDGET_GTK

  if (aNativeWidget) {
    
    if (GDK_IS_SUPERWIN(aNativeWidget)) {
      mDeviceWindow = GDK_SUPERWIN(aNativeWidget)->shell_window;
    }
    
    else if (GTK_IS_WIDGET(aNativeWidget)) {
      mDeviceWindow = GTK_WIDGET(aNativeWidget)->window;
    }
    
    else {
      mDeviceWindow = NS_STATIC_CAST(GdkWindow *, aNativeWidget);
    }
  }

#endif 

#ifdef MOZ_WIDGET_GTK2

  if (aNativeWidget) {
    
    if (GDK_IS_WINDOW(aNativeWidget))
      mDeviceWindow = GDK_WINDOW(aNativeWidget);
    else 
      NS_WARNING("unsupported native widget type!");
  }

#endif

  nsCOMPtr<nsIScreen> screen;
  mScreenManager->GetPrimaryScreen ( getter_AddRefs(screen) );
  if ( screen ) {
    PRInt32 depth;
    screen->GetPixelDepth ( &depth );
    mDepth = NS_STATIC_CAST ( PRUint32, depth );
  }
    
  static int initialized = 0;
  PRInt32 prefVal = -1;
  if (!initialized) {
    initialized = 1;

    
    
    
    
    
    
    
    nsresult res;

    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &res));
    if (NS_SUCCEEDED(res) && prefs) {
      res = prefs->GetIntPref("layout.css.dpi", &prefVal);
      if (NS_FAILED(res)) {
        prefVal = -1;
      }
      prefs->RegisterCallback("layout.css.dpi", prefChanged,
                              (void *)this);
    }

    SetDPI(prefVal);
  } else {
    SetDPI(mDpi); 
  }

#ifdef DEBUG
  static PRBool once = PR_TRUE;
  if (once) {
    printf("GFX: dpi=%d t2p=%g p2t=%g depth=%d\n", mDpi, mTwipsToPixels, mPixelsToTwips,mDepth);
    once = PR_FALSE;
  }
#endif

  DeviceContextImpl::CommonInit();

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::CreateRenderingContext(nsIRenderingContext *&aContext)
{
#ifdef NS_PRINT_PREVIEW
  
  if (mAltDC && ((mUseAltDC & kUseAltDCFor_CREATERC_PAINT) || (mUseAltDC & kUseAltDCFor_CREATERC_REFLOW))) {
    return mAltDC->CreateRenderingContext(aContext);
  }
#endif

  nsresult             rv;
  GtkWidget *w = (GtkWidget*)mWidget;

  
  nsIRenderingContext* pContext = new nsRenderingContextGTK();

  if (nsnull != pContext)
  {
    NS_ADDREF(pContext);

    
    nsDrawingSurfaceGTK* surf = new nsDrawingSurfaceGTK();

    if (surf && w)
      {
        GdkDrawable *gwin = nsnull;
        GdkDrawable *win = nsnull;
        
        if (GTK_IS_LAYOUT(w))
          gwin = (GdkDrawable*)GTK_LAYOUT(w)->bin_window;
        else
          gwin = (GdkDrawable*)(w)->window;

        
        if (gwin)
          gdk_window_ref(gwin);
        else {
          win = gdk_pixmap_new(nsnull,
                               w->allocation.width,
                               w->allocation.height,
                               gdk_rgb_get_visual()->depth);
#ifdef MOZ_WIDGET_GTK2
          gdk_drawable_set_colormap(win, gdk_rgb_get_colormap());
#endif
        }

        GdkGC *gc = gdk_gc_new(win);

        
        rv = surf->Init(win,gc);

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

NS_IMETHODIMP nsDeviceContextGTK::CreateRenderingContextInstance(nsIRenderingContext *&aContext)
{
  nsCOMPtr<nsIRenderingContext> renderingContext = new nsRenderingContextGTK();
  if (!renderingContext)
    return NS_ERROR_OUT_OF_MEMORY;
         
  aContext = renderingContext;
  NS_ADDREF(aContext);
  
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::SupportsNativeWidgets(PRBool &aSupportsWidgets)
{
  
  
  aSupportsWidgets = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::GetSystemFont(nsSystemFontID aID, nsFont *aFont) const
{
  nsresult status = NS_OK;

  if (!gSystemFonts) {
    gSystemFonts = new nsSystemFontsGTK(mPixelsToTwips);
  }

  switch (aID) {
    case eSystemFont_Menu:         
    case eSystemFont_PullDownMenu: 
        *aFont = gSystemFonts->GetMenuFont();
        break;

    case eSystemFont_Field:        
    case eSystemFont_List:         
        *aFont = gSystemFonts->GetFieldFont();
        break;

    case eSystemFont_Button:       
        *aFont = gSystemFonts->GetButtonFont();
        break;

    case eSystemFont_Caption:      
    case eSystemFont_Icon:         
    case eSystemFont_MessageBox:   
    case eSystemFont_SmallCaption: 
    case eSystemFont_StatusBar:    
    case eSystemFont_Window:       
    case eSystemFont_Document:     
    case eSystemFont_Workspace:    
    case eSystemFont_Desktop:      
    case eSystemFont_Info:         
    case eSystemFont_Dialog:       
    case eSystemFont_Tooltips:     
    case eSystemFont_Widget:       
        *aFont = gSystemFonts->GetDefaultFont();
        break;
  }

  return status;
}

NS_IMETHODIMP nsDeviceContextGTK::CheckFontExistence(const nsString& aFontName)
{
  return NS_FontMetricsFamilyExists(this, aFontName);
}

NS_IMETHODIMP nsDeviceContextGTK::GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight)
{
#ifdef NS_PRINT_PREVIEW
  
  if (mAltDC && (mUseAltDC & kUseAltDCFor_SURFACE_DIM)) {
    return mAltDC->GetDeviceSurfaceDimensions(aWidth, aHeight);
  }
#endif

  PRInt32 width = 0, height = 0;

  nsCOMPtr<nsIScreen> screen;
  mScreenManager->GetPrimaryScreen(getter_AddRefs(screen));
  if (screen) {
    PRInt32 x, y;
    screen->GetRect(&x, &y, &width, &height);
  }

  aWidth = NSToIntRound(float(width) * mDevUnitsToAppUnits);
  aHeight = NSToIntRound(float(height) * mDevUnitsToAppUnits);

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::GetRect(nsRect &aRect)
{
  
  
  if (mDeviceWindow) {
    gint x, y, width, height, depth;
    x = y = width = height = 0;

    gdk_window_get_geometry(mDeviceWindow, &x, &y, &width, &height,
                            &depth);
    gdk_window_get_origin(mDeviceWindow, &x, &y);

    nsCOMPtr<nsIScreen> screen;
    mScreenManager->ScreenForRect(x, y, width, height, getter_AddRefs(screen));
    screen->GetRect(&aRect.x, &aRect.y, &aRect.width, &aRect.height);
    aRect.x = NSToIntRound(mDevUnitsToAppUnits * aRect.x);
    aRect.y = NSToIntRound(mDevUnitsToAppUnits * aRect.y);
    aRect.width = NSToIntRound(mDevUnitsToAppUnits * aRect.width);
    aRect.height = NSToIntRound(mDevUnitsToAppUnits * aRect.height);
  }
  else {
    PRInt32 width, height;
    GetDeviceSurfaceDimensions(width, height);
    aRect.x = 0;
    aRect.y = 0;
    aRect.width = width;
    aRect.height = height;
  }
  return NS_OK;
}


NS_IMETHODIMP nsDeviceContextGTK::GetClientRect(nsRect &aRect)
{
  
  
  if (mDeviceWindow) {
    gint x, y, width, height, depth;
    x = y = width = height = 0;

    gdk_window_get_geometry(mDeviceWindow, &x, &y, &width, &height,
                            &depth);
    gdk_window_get_origin(mDeviceWindow, &x, &y);

    nsCOMPtr<nsIScreen> screen;
    mScreenManager->ScreenForRect(x, y, width, height, getter_AddRefs(screen));
    screen->GetAvailRect(&aRect.x, &aRect.y, &aRect.width, &aRect.height);
    aRect.x = NSToIntRound(mDevUnitsToAppUnits * aRect.x);
    aRect.y = NSToIntRound(mDevUnitsToAppUnits * aRect.y);
    aRect.width = NSToIntRound(mDevUnitsToAppUnits * aRect.width);
    aRect.height = NSToIntRound(mDevUnitsToAppUnits * aRect.height);
  }
  else {
    PRInt32 width, height;
    GetDeviceSurfaceDimensions(width, height);
    aRect.x = 0;
    aRect.y = 0;
    aRect.width = width;
    aRect.height = height;
  }

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                                       nsIDeviceContext *&aContext)
{
  nsresult                 rv;

#if 0
  PrintMethod              method;

  nsDeviceContextSpecGTK  *spec = NS_STATIC_CAST(nsDeviceContextSpecGTK *, aDevice);
  
  rv = spec->GetPrintMethod(method);
  if (NS_FAILED(rv)) 
    return rv;

#ifdef USE_XPRINT
  if (method == pmXprint) { 
    static NS_DEFINE_CID(kCDeviceContextXp, NS_DEVICECONTEXTXP_CID);
    nsCOMPtr<nsIDeviceContextXp> dcxp(do_CreateInstance(kCDeviceContextXp, &rv));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't create Xp Device context.");    
    if (NS_FAILED(rv)) 
      return NS_ERROR_GFX_COULD_NOT_LOAD_PRINT_MODULE;
    
    rv = dcxp->SetSpec(aDevice);
    if (NS_FAILED(rv)) 
      return rv;
    
    rv = dcxp->InitDeviceContextXP((nsIDeviceContext*)aContext,
                                   (nsIDeviceContext*)this);
    if (NS_FAILED(rv)) 
      return rv;
      
    rv = dcxp->QueryInterface(NS_GET_IID(nsIDeviceContext),
                              (void **)&aContext);
    return rv;
  }
  else
#endif 
#endif
#ifdef USE_POSTSCRIPT

  {

    
    static NS_DEFINE_CID(kCDeviceContextPS, NS_DEVICECONTEXTPS_CID);
  
    
    nsCOMPtr<nsIDeviceContextPS> dcps(do_CreateInstance(kCDeviceContextPS, &rv));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't create PS Device context.");
    if (NS_FAILED(rv)) 
      return NS_ERROR_GFX_COULD_NOT_LOAD_PRINT_MODULE;
  
    rv = dcps->SetSpec(aDevice);
    if (NS_FAILED(rv)) 
      return rv;
      
    rv = dcps->InitDeviceContextPS((nsIDeviceContext*)aContext,
                                   (nsIDeviceContext*)this);
    if (NS_FAILED(rv)) 
      return rv;

    rv = dcps->QueryInterface(NS_GET_IID(nsIDeviceContext),
                              (void **)&aContext);
    return rv;
  }

#endif 
  NS_WARNING("no print module created.");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP nsDeviceContextGTK::BeginDocument(PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::EndDocument(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::AbortDocument(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::BeginPage(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::EndPage(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::GetDepth(PRUint32& aDepth)
{
  aDepth = mDepth;
  return NS_OK;
}

nsresult
nsDeviceContextGTK::SetDPI(PRInt32 aPrefDPI)
{
  PRInt32 OSVal = GetOSDPI();

  if (aPrefDPI > 0) {
    
    
    mDpi = aPrefDPI;
  } else if ((aPrefDPI == 0) || (OSVal > 96)) {
    
    
    mDpi = OSVal;
  } else {
    
    
    mDpi = 96;
  }
  
  int pt2t = 72;

  
  mPixelsToTwips = float(NSToIntRound(float(NSIntPointsToTwips(pt2t)) / float(mDpi)));
  mTwipsToPixels = 1.0f / mPixelsToTwips;

  
  return NS_OK;
}

static void DoClearCachedSystemFonts()
{
  
  if (gSystemFonts) {
    delete gSystemFonts;
    gSystemFonts = nsnull;
  }
}

NS_IMETHODIMP
nsDeviceContextGTK::ClearCachedSystemFonts()
{
  DoClearCachedSystemFonts();
  return NS_OK;
}

int nsDeviceContextGTK::prefChanged(const char *aPref, void *aClosure)
{
  nsDeviceContextGTK *context = (nsDeviceContextGTK*)aClosure;
  nsresult rv;
  
  if (nsCRT::strcmp(aPref, "layout.css.dpi")==0) {
    PRInt32 dpi;
    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
    rv = prefs->GetIntPref(aPref, &dpi);
    if (NS_SUCCEEDED(rv))
      context->SetDPI(dpi);

    
    
    DoClearCachedSystemFonts();
  }

  return 0;
}

#define DEFAULT_TWIP_FONT_SIZE 240

nsSystemFontsGTK::nsSystemFontsGTK(float aPixelsToTwips)
  : mDefaultFont("sans-serif", NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
                 NS_FONT_WEIGHT_NORMAL, NS_FONT_DECORATION_NONE,
                 DEFAULT_TWIP_FONT_SIZE),
    mButtonFont("sans-serif", NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
                NS_FONT_WEIGHT_NORMAL, NS_FONT_DECORATION_NONE,
                DEFAULT_TWIP_FONT_SIZE),
    mFieldFont("sans-serif", NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
               NS_FONT_WEIGHT_NORMAL, NS_FONT_DECORATION_NONE,
               DEFAULT_TWIP_FONT_SIZE),
    mMenuFont("sans-serif", NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
               NS_FONT_WEIGHT_NORMAL, NS_FONT_DECORATION_NONE,
               DEFAULT_TWIP_FONT_SIZE)
{
  




  
  GtkWidget *label = gtk_label_new("M");
  GtkWidget *parent = gtk_fixed_new();
  GtkWidget *window = gtk_window_new(GTK_WINDOW_POPUP);

  gtk_container_add(GTK_CONTAINER(parent), label);
  gtk_container_add(GTK_CONTAINER(window), parent);

  gtk_widget_ensure_style(label);

  GetSystemFontInfo(label, &mDefaultFont, aPixelsToTwips);

  gtk_widget_destroy(window);  

  
  GtkWidget *entry = gtk_entry_new();
  parent = gtk_fixed_new();
  window = gtk_window_new(GTK_WINDOW_POPUP);

  gtk_container_add(GTK_CONTAINER(parent), entry);
  gtk_container_add(GTK_CONTAINER(window), parent);
  gtk_widget_ensure_style(entry);

  GetSystemFontInfo(entry, &mFieldFont, aPixelsToTwips);

  gtk_widget_destroy(window);  

  
  GtkWidget *accel_label = gtk_accel_label_new("M");
  GtkWidget *menuitem = gtk_menu_item_new();
  GtkWidget *menu = gtk_menu_new();
  gtk_object_ref(GTK_OBJECT(menu));
  gtk_object_sink(GTK_OBJECT(menu));

  gtk_container_add(GTK_CONTAINER(menuitem), accel_label);
  gtk_menu_append(GTK_MENU(menu), menuitem);

  gtk_widget_ensure_style(accel_label);

  GetSystemFontInfo(accel_label, &mMenuFont, aPixelsToTwips);

  gtk_widget_unref(menu);

  
  parent = gtk_fixed_new();
  GtkWidget *button = gtk_button_new();
  label = gtk_label_new("M");
  window = gtk_window_new(GTK_WINDOW_POPUP);
          
  gtk_container_add(GTK_CONTAINER(button), label);
  gtk_container_add(GTK_CONTAINER(parent), button);
  gtk_container_add(GTK_CONTAINER(window), parent);

  gtk_widget_ensure_style(label);

  GetSystemFontInfo(label, &mButtonFont, aPixelsToTwips);

  gtk_widget_destroy(window);  

}

#if 0 
static void
ListFontProps(XFontStruct *aFont, Display *aDisplay)
{
  printf("\n\n");
  for (int i = 0, n = aFont->n_properties; i < n; ++i) {
    XFontProp *prop = aFont->properties + i;
    char *atomName = ::XGetAtomName(aDisplay, prop->name);
    
    char *cardName = (prop->card32 > 0 && prop->card32 < 500)
                       ? ::XGetAtomName(aDisplay, prop->card32)
                       : 0;
    printf("%s : %ld (%s)\n", atomName, prop->card32, cardName?cardName:"");
    ::XFree(atomName);
    if (cardName)
      ::XFree(cardName);
  }
  printf("\n\n");
}
#endif

#if defined(MOZ_ENABLE_COREXFONTS) || defined(MOZ_WIDGET_GTK)

#define LOCATE_MINUS(pos, str)  { \
   pos = str.FindChar('-'); \
   if (pos < 0) \
     return ; \
  }
#define NEXT_MINUS(pos, str) { \
   pos = str.FindChar('-', pos+1); \
   if (pos < 0) \
     return ; \
  }  

static void
AppendFontFFREName(nsString& aString, const char* aXLFDName)
{
  
  
  
  
  nsCAutoString nameStr(aXLFDName);
  PRInt32 pos1, pos2;
  
  LOCATE_MINUS(pos1, nameStr);
  nameStr.Cut(0, pos1+1);

  
  LOCATE_MINUS(pos1, nameStr);
  NEXT_MINUS(pos1, nameStr);
  pos2 = pos1;

  
  for (PRInt32 i=0; i < 10; i++) {
    NEXT_MINUS(pos2, nameStr);
  }

  
  nameStr.Cut(pos1, pos2-pos1);

  aString.AppendWithConversion(nameStr.get());
}
#endif 

#ifdef MOZ_WIDGET_GTK
static void
AppendFontName(XFontStruct* aFontStruct, nsString& aString, Display *aDisplay)
{
  unsigned long pr = 0;
  
  unsigned long font_atom = gdk_atom_intern("FONT", FALSE);
  if (::XGetFontProperty(aFontStruct, font_atom, &pr) && pr) {
    char* xlfdName = ::XGetAtomName(aDisplay, pr);
    AppendFontFFREName(aString, xlfdName);
    ::XFree(xlfdName);
  }
 
  aString.Append(PRUnichar(','));

  
  if ((::XGetFontProperty(aFontStruct, XA_FAMILY_NAME, &pr) ||
       ::XGetFontProperty(aFontStruct, XA_FULL_NAME, &pr)) &&
      pr) {
    char *fontName = ::XGetAtomName(aDisplay, pr);
    aString.AppendWithConversion(fontName);
    ::XFree(fontName);
  }
}

static PRUint16
GetFontWeight(XFontStruct* aFontStruct, Display *aDisplay)
{
  PRUint16 weight = NS_FONT_WEIGHT_NORMAL;

  
  
  unsigned long pr = 0;
  Atom weightName = ::XInternAtom(aDisplay, "WEIGHT_NAME", True);
  if (weightName != None) {
    if (::XGetFontProperty(aFontStruct, weightName, &pr) && pr) {
      char *weightString = ::XGetAtomName(aDisplay, pr);
      if (nsCRT::strcasecmp(weightString, "bold") == 0)
        weight = NS_FONT_WEIGHT_BOLD;
      ::XFree(weightString);
    }
  }

  pr = 0;
  if (::XGetFontProperty(aFontStruct, XA_WEIGHT, &pr) && pr > 10 )
    weight = NS_FONT_WEIGHT_BOLD;

  return weight;
}

static nscoord
GetFontSize(XFontStruct *aFontStruct, float aPixelsToTwips)
{
  unsigned long pr = 0;
  Atom pixelSizeAtom = ::XInternAtom(GDK_DISPLAY(), "PIXEL_SIZE", 0);
  if (!::XGetFontProperty(aFontStruct, pixelSizeAtom, &pr) || !pr)
    return DEFAULT_TWIP_FONT_SIZE;
  return NSIntPixelsToTwips(pr, aPixelsToTwips);
}

nsresult
nsSystemFontsGTK::GetSystemFontInfo(GtkWidget *aWidget, nsFont* aFont,
                                    float aPixelsToTwips) const
{
  GtkStyle *style = gtk_widget_get_style(aWidget);

  GdkFont *theFont = style->font;

  aFont->style       = NS_FONT_STYLE_NORMAL;
  aFont->weight      = NS_FONT_WEIGHT_NORMAL;
  aFont->decorations = NS_FONT_DECORATION_NONE;
  
  
  
  
  if (!theFont)
    theFont = default_font; 

  if (!theFont)
    theFont = ::gdk_font_load( GDK_DEFAULT_FONT1 );
  
  if (!theFont)
    theFont = ::gdk_font_load( GDK_DEFAULT_FONT2 );
  
  if (!theFont)
    return NS_ERROR_FAILURE;

  Display *fontDisplay = GDK_FONT_XDISPLAY(theFont);
  if (theFont->type == GDK_FONT_FONT) {
    XFontStruct *fontStruct =
        NS_STATIC_CAST(XFontStruct*, GDK_FONT_XFONT(theFont));

    aFont->name.Truncate();
    AppendFontName(fontStruct, aFont->name, fontDisplay);
    aFont->weight = GetFontWeight(fontStruct, fontDisplay);
    aFont->size = GetFontSize(fontStruct, aPixelsToTwips);
  } else {
    NS_ASSERTION(theFont->type == GDK_FONT_FONTSET,
                 "theFont->type can only have two values");

    XFontSet fontSet = NS_REINTERPRET_CAST(XFontSet, GDK_FONT_XFONT(theFont));
    XFontStruct **fontStructs;
    char **fontNames;
    int numFonts = ::XFontsOfFontSet(fontSet, &fontStructs, &fontNames);
    if (numFonts == 0)
      return NS_ERROR_FAILURE;

    
    
    aFont->weight = GetFontWeight(*fontStructs, fontDisplay);
    aFont->size = GetFontSize(*fontStructs, aPixelsToTwips);
    nsString& fontName = aFont->name;
    fontName.Truncate();
    for (;;) {
      
      AppendFontFFREName(fontName, *fontNames);
      ++fontNames;
      --numFonts;
      if (numFonts == 0)
        break;
      fontName.Append(PRUnichar(','));
    }
  }
  return NS_OK;
}
#endif 

#ifdef MOZ_WIDGET_GTK2

#ifdef MOZ_ENABLE_COREXFONTS
static void xlfd_from_pango_font_description(GtkWidget *aWidget,
                                             const PangoFontDescription *aFontDesc,
                                             nsString& aFontName);
#endif 

nsresult
nsSystemFontsGTK::GetSystemFontInfo(GtkWidget *aWidget, nsFont* aFont,
                                    float aPixelsToTwips) const
{
  GtkSettings *settings = gtk_widget_get_settings(aWidget);

  aFont->style       = NS_FONT_STYLE_NORMAL;
  aFont->decorations = NS_FONT_DECORATION_NONE;

  gchar *fontname;
  g_object_get(settings, "gtk-font-name", &fontname, NULL);

  PangoFontDescription *desc;
  desc = pango_font_description_from_string(fontname);

  aFont->systemFont = PR_TRUE;

  g_free(fontname);

  aFont->name.Truncate();
#ifdef MOZ_ENABLE_XFT
  if (NS_IsXftEnabled()) {
    aFont->name.Assign(PRUnichar('"'));
    aFont->name.AppendWithConversion(pango_font_description_get_family(desc));
    aFont->name.Append(PRUnichar('"'));
  }
#endif 

#ifdef MOZ_ENABLE_COREXFONTS
  
  if (!aFont->name.Length()) {
    xlfd_from_pango_font_description(aWidget, desc, aFont->name);
  }
#endif 
  aFont->weight = pango_font_description_get_weight(desc);

  float size = float(pango_font_description_get_size(desc) / PANGO_SCALE);
#ifdef MOZ_ENABLE_XFT
  if (NS_IsXftEnabled()) {
    PRInt32 dpi = GetXftDPI();
    if (dpi != 0) {
      
      
      size *= float(dpi) * aPixelsToTwips * (1.0f/1440.0f);
    }
  }
#endif 
  aFont->size = NSFloatPointsToTwips(size);
  
  pango_font_description_free(desc);

  return NS_OK;
}
#endif 

#ifdef MOZ_WIDGET_GTK

PRInt32
GetOSDPI(void)
{

#ifdef MOZ_ENABLE_XFT
  
  if (NS_IsXftEnabled()) {
    PRInt32 xftdpi = GetXftDPI();
    if (xftdpi)
      return xftdpi;
  }
#endif 

  
  float screenWidthIn = float(::gdk_screen_width_mm()) / 25.4f;
  return NSToCoordRound(float(::gdk_screen_width()) / screenWidthIn);
}
#endif 

#ifdef MOZ_WIDGET_GTK2

PRInt32
GetOSDPI(void)
{
  GtkSettings *settings = gtk_settings_get_default();

  
  gint dpi = 0;

  
  
  
  
  
  GParamSpec *spec;
  spec = g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(settings)),
                                      "gtk-xft-dpi");
  if (spec) {
    g_object_get(G_OBJECT(settings),
                 "gtk-xft-dpi", &dpi,
                 NULL);
  }

  if (dpi > 0)
    return NSToCoordRound(dpi / 1024.0);

#ifdef MOZ_ENABLE_XFT
  
  PRInt32 xftdpi = GetXftDPI();

  if (xftdpi)
    return xftdpi;
#endif 
  
  
  float screenWidthIn = float(::gdk_screen_width_mm()) / 25.4f;
  return NSToCoordRound(float(::gdk_screen_width()) / screenWidthIn);
}
#endif 

#ifdef MOZ_ENABLE_XFT

PRInt32
GetXftDPI(void)
{
  char *val = XGetDefault(GDK_DISPLAY(), "Xft", "dpi");
  if (val) {
    char *e;
    double d = strtod(val, &e);

    if (e != val)
      return NSToCoordRound(d);
  }

  return 0;
}
#endif 

#if defined(MOZ_WIDGET_GTK2) && defined(MOZ_ENABLE_COREXFONTS)


static void
xlfd_from_pango_font_description(GtkWidget *aWidget,
         const PangoFontDescription *aFontDesc,
                                 nsString& aFontName)
{
  char *spec;
  PangoContext *context;
  PangoFont *font;
  PangoXSubfont *subfont_ids;
  PangoFontMap *fontmap;
  int *subfont_charsets, i, count = 0;
  char *tmp, *subfont;
  char *encodings[] = {
    "ascii-0",
    "big5-0",
    "dos-437",
    "dos-737",
    "gb18030.2000-0",
    "gb18030.2000-1",
    "gb2312.1980-0",
    "iso8859-1",
    "iso8859-2",
    "iso8859-3",
    "iso8859-4",
    "iso8859-5",
    "iso8859-7",
    "iso8859-8",
    "iso8859-9",
    "iso8859-10",
    "iso8859-15",
    "iso10646-0",
    "iso10646-1",
    "jisx0201.1976-0",
    "jisx0208.1983-0",
    "jisx0208.1990-0",
    "jisx0208.1997-0",
    "jisx0212.1990-0",
    "jisx0213.2000-1",
    "jisx0213.2000-2",
    "koi8-r",
    "koi8-u",
    "koi8-ub",
    "ksc5601.1987-0",
    "ksc5601.1992-3",
    "tis620-0",
    "iso8859-13",
    "microsoft-cp1251"
    "misc-fontspecific",
  };
#if XlibSpecificationRelease >= 6
  XOM xom;
#endif
  if (!aFontDesc) {
    return;
  }

  context = gtk_widget_get_pango_context(GTK_WIDGET(aWidget));

  pango_context_set_language (context, gtk_get_default_language ());
  fontmap = pango_x_font_map_for_display(GDK_DISPLAY());

  if (!fontmap) {
    return;
  }

  font = pango_font_map_load_font(fontmap, context, aFontDesc);
  if (!font) {
    return;
  }

#if XlibSpecificationRelease >= 6
  xom = XOpenOM (GDK_DISPLAY(), NULL, NULL, NULL);
  if (xom) {
    XOMCharSetList cslist;
    int n_encodings = 0;
    cslist.charset_count = 0;
    XGetOMValues (xom,
      XNRequiredCharSet, &cslist,
      NULL);
    n_encodings = cslist.charset_count;
    if (n_encodings) {
      char **xom_encodings = (char**) g_malloc (sizeof(char*) * n_encodings);

      for (i = 0; i < n_encodings; i++) {
        xom_encodings[i] = g_ascii_strdown (cslist.charset_list[i], -1);
      }
      count = pango_x_list_subfonts(font, xom_encodings, n_encodings,
            &subfont_ids, &subfont_charsets);

      for(i = 0; i < n_encodings; i++) {
        g_free (xom_encodings[i]);
      }
      g_free (xom_encodings);
    }
    XCloseOM (xom);
  }
#endif
  if (count == 0) {
    count = pango_x_list_subfonts(font, encodings, G_N_ELEMENTS(encodings),
          &subfont_ids, &subfont_charsets);
  }

  for (i = 0; i < count; i++) {
    subfont = pango_x_font_subfont_xlfd(font, subfont_ids[i]);
    AppendFontFFREName(aFontName, subfont);
    g_free(subfont);
    aFontName.Append(PRUnichar(','));
  }

  spec = pango_font_description_to_string(aFontDesc);

  if (subfont_ids != NULL) {
    g_free(subfont_ids);
  }
  if (subfont_charsets != NULL) {
    g_free(subfont_charsets);
  }
  g_free(spec);
  g_object_unref(font);
}
#endif 

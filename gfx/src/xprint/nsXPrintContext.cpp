












































#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h> 

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1 /* Allow logging in the release build */
#endif 
#include "prlog.h"

#include "imgScaler.h"
#include "nsXPrintContext.h"
#include "nsDeviceContextXP.h"
#include "xprintutil.h"
#include "prenv.h" 
#include "prprf.h"
#include "plstr.h"
#include "nsPrintfCString.h"
#include "nsIServiceManager.h" 
#include "nsIEnvironment.h"
 








#define NS_XPRINT_RGB_DITHER \
    (((mDepth >  12 && mVisual->c_class==TrueColor)  || \
      (mDepth >=  7 && mVisual->c_class==GrayScale)  || \
      (mDepth >=  7 && mVisual->c_class==StaticGray) || \
      (mIsGrayscale == PR_TRUE)) ?(XLIB_RGB_DITHER_NONE):(XLIB_RGB_DITHER_MAX))

#ifdef PR_LOGGING 




static PRLogModuleInfo *nsXPrintContextLM = PR_NewLogModule("nsXPrintContext");
#endif 

PR_BEGIN_EXTERN_C
static 
int xerror_handler( Display *display, XErrorEvent *ev )
{
    
    char errmsg[80];
    XGetErrorText(display, ev->error_code, errmsg, sizeof(errmsg));
    fprintf(stderr, "nsGfxXprintModule: Warning (X Error) -  %s\n", errmsg);
    return 0;
}
PR_END_EXTERN_C




nsXPrintContext::nsXPrintContext() :
  mXlibRgbHandle(nsnull),
  mPDisplay(nsnull),
  mScreen(nsnull),
  mVisual(nsnull),
  mDrawable(None),
  mGC(nsnull),
  mDepth(0),
  mPContext(None),
  mJobStarted(PR_FALSE),
  mIsGrayscale(PR_FALSE), 
  mIsAPrinter(PR_TRUE),   
  mPrintFile(nsnull),
  mXpuPrintToFileHandle(nsnull),
  mPrintXResolution(0L),
  mPrintYResolution(0L),
  mContext(nsnull)
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::nsXPrintContext()\n"));
}




nsXPrintContext::~nsXPrintContext()
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::~nsXPrintContext()\n"));
 
  if (mPDisplay)
  {
    if (mJobStarted)
    {
      
      AbortDocument();
    }

    if (mGC)
    {
      mGC->Release();
      mGC = nsnull;
    }
       
    if (mXlibRgbHandle)
    {
      xxlib_rgb_destroy_handle(mXlibRgbHandle);
      mXlibRgbHandle = nsnull;
    }
    
    XPU_TRACE(XpuClosePrinterDisplay(mPDisplay, mPContext));           
    mPDisplay = nsnull;
    mPContext = None;
  }
  
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::~nsXPrintContext() done.\n"));
}

NS_IMPL_ISUPPORTS1(nsXPrintContext, nsIDrawingSurfaceXlib)


static nsresult
AlertBrokenXprt(Display *pdpy)
{
  





  if (!(strstr(XServerVendor(pdpy), "XFree86") ))
    return NS_OK;

  
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG,
         ("nsXPrintContext::AlertBrokenXprt: vendor: '%s', release=%ld\n",
          XServerVendor(pdpy), (long)XVendorRelease(pdpy)));

  
  if (PR_GetEnv("MOZILLA_XPRINT_DISABLE_BROKEN_XFREE86_WARNING") != nsnull)
    return NS_OK;
   
  return NS_ERROR_GFX_PRINTER_XPRINT_BROKEN_XPRT;
}

NS_IMETHODIMP 
nsXPrintContext::Init(nsDeviceContextXp *dc, nsIDeviceContextSpecXp *aSpec)
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::Init()\n"));
  nsresult rv = NS_ERROR_FAILURE;

 
  unsigned short      width,
                      height;
  XRectangle          rect;
  const char         *colorspace; 
  XpuColorspaceList   cslist;
  int                 cscount;
  XpuColorspaceRec   *cs;
  VisualID            csvid;
  int                 cs_class;

  if (NS_FAILED(XPU_TRACE(rv = SetupPrintContext(aSpec))))
    return rv;

  mScreen = XpGetScreenOfContext(mPDisplay, mPContext);
  mScreenNumber = XScreenNumberOfScreen(mScreen);

  
  aSpec->GetColorspace(&colorspace);
  cslist = XpuGetColorspaceList(mPDisplay, mPContext, &cscount);
  if (!cslist) {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuGetColorspaceList() failed.\n"));
    return NS_ERROR_GFX_PRINTER_COLORSPACE_NOT_SUPPORTED;
  }
  cs = XpuFindColorspaceByName(cslist, cscount, colorspace);
  if (!cs) {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuFindColorspaceByName() failed.\n"));
    XpuFreeColorspaceList(cslist);
    return NS_ERROR_GFX_PRINTER_COLORSPACE_NOT_SUPPORTED;
  }
  csvid    = cs->visualinfo.visualid;
  cs_class = cs->visualinfo.c_class;
  XpuFreeColorspaceList(cslist);

  XlibRgbArgs xargs;
  memset(&xargs, 0, sizeof(xargs));
  xargs.handle_name           = nsnull;
  xargs.disallow_image_tiling = True; 

  
  if (mIsGrayscale)
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("printing grayscale\n"));

    





    if ((cs_class | GrayScale) == GrayScale)
    {
      PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("using selected gray visual\n"));
      xargs.xtemplate_mask     = VisualIDMask;
      xargs.xtemplate.visualid = csvid;
      mXlibRgbHandle = xxlib_rgb_create_handle(mPDisplay, mScreen, &xargs);
    }
    else
    { 
      PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("using fallback codepath\n"));
      
      xargs.xtemplate.c_class = StaticGray;
      xargs.xtemplate.depth   = 8;
      xargs.xtemplate_mask    = VisualClassMask|VisualDepthMask;
      mXlibRgbHandle = xxlib_rgb_create_handle(mPDisplay, mScreen, &xargs);             

      if (!mXlibRgbHandle)
      { 
        
        xargs.xtemplate.c_class = GrayScale;
        xargs.xtemplate.depth   = 8;
        xargs.xtemplate_mask    = VisualClassMask|VisualDepthMask;
        mXlibRgbHandle = xxlib_rgb_create_handle(mPDisplay, mScreen, &xargs);             
        if (!mXlibRgbHandle)
        {
          
          xargs.xtemplate_mask  = 0L;
          xargs.xtemplate.depth = 0;
          xargs.pseudogray      = True;
          mXlibRgbHandle = xxlib_rgb_create_handle(mPDisplay, mScreen, &xargs);
        }

        if (!mXlibRgbHandle)
        {
          PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("trying black/white\n"));

          
          xargs.xtemplate.c_class = StaticGray;
          xargs.xtemplate.depth   = 1;
          xargs.xtemplate_mask    = VisualClassMask|VisualDepthMask;
          xargs.pseudogray        = False;
          mXlibRgbHandle = xxlib_rgb_create_handle(mPDisplay, mScreen, &xargs);
        }
      }
    }  
  }
  else
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("printing color\n"));
    xargs.xtemplate_mask     = VisualIDMask;
    xargs.xtemplate.visualid = csvid;
    mXlibRgbHandle = xxlib_rgb_create_handle(mPDisplay, mScreen, &xargs);
  }
  
  
  if (!mXlibRgbHandle)
    return NS_ERROR_GFX_PRINTER_COLORSPACE_NOT_SUPPORTED;

  XpGetPageDimensions(mPDisplay, mPContext, &width, &height, &rect);

  rv = SetupWindow(rect.x, rect.y, rect.width, rect.height);
  if (NS_FAILED(rv))
    return rv;

  XMapWindow(mPDisplay, mDrawable); 
  
  mContext = dc;
    
  


  (void)XSetErrorHandler(xerror_handler);

  
  if( PR_GetEnv("MOZILLA_XPRINT_EXPERIMENTAL_SYNCHRONIZE") != nsnull )
  {
    XSynchronize(mPDisplay, True);
  }  
  
  return NS_OK;
}

NS_IMETHODIMP
nsXPrintContext::GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight)
{
  *aWidth = mWidth;
  *aHeight = mHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsXPrintContext::IsOffscreen(PRBool *aOffScreen)
{
  *aOffScreen = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsXPrintContext::IsPixelAddressable(PRBool *aAddressable)
{
  *aAddressable = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsXPrintContext::GetPixelFormat(nsPixelFormat *aFormat)
{
  *aFormat = mPixFormat;
  return NS_OK;
}

PRUint8 
nsXPrintContext::GetShiftForMask(unsigned long val)
{
  PRUint8 cur_bit = 0;
  
  while (cur_bit < (sizeof(unsigned long) * 8)) {
    if ((val >> cur_bit) & 0x1) {
      return cur_bit;
    }
    cur_bit++;
  }
  return cur_bit;
}

PRUint8 
nsXPrintContext::ConvertMaskToCount(unsigned long val)
{
  PRUint8 retval = 0;
  PRUint8 cur_bit = 0;
  
  
  while (cur_bit < (sizeof(unsigned long) * 8)) {
    if ((val >> cur_bit) & 0x1) {
      retval++;
    }
    cur_bit++;
  }
  return retval;
}


nsresult
nsXPrintContext::SetupWindow(int x, int y, int width, int height)
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG,
         ("nsXPrintContext::SetupWindow: x=%d y=%d width=%d height=%d\n",
         x, y, width, height));

  Window                 parent_win;
  XVisualInfo           *visual_info;
  XSetWindowAttributes   xattributes;
  long                   xattributes_mask;
  unsigned long          background,
                         foreground;
  
  mWidth  = width;
  mHeight = height;

  visual_info = xxlib_rgb_get_visual_info(mXlibRgbHandle);
  mVisual     = xxlib_rgb_get_visual(mXlibRgbHandle);
  mDepth      = xxlib_rgb_get_depth(mXlibRgbHandle);
  
  mPixFormat.mRedMask    = visual_info->red_mask;
  mPixFormat.mGreenMask  = visual_info->green_mask;
  mPixFormat.mBlueMask   = visual_info->blue_mask;
  mPixFormat.mAlphaMask  = 0;
  mPixFormat.mRedCount   = ConvertMaskToCount(visual_info->red_mask);
  mPixFormat.mGreenCount = ConvertMaskToCount(visual_info->green_mask);
  mPixFormat.mBlueCount  = ConvertMaskToCount(visual_info->blue_mask);
  mPixFormat.mAlphaCount = 0;
  mPixFormat.mRedShift   = GetShiftForMask(visual_info->red_mask);
  mPixFormat.mGreenShift = GetShiftForMask(visual_info->green_mask);
  mPixFormat.mBlueShift  = GetShiftForMask(visual_info->blue_mask);
  mPixFormat.mAlphaShift = 0;

  background = xxlib_rgb_xpixel_from_rgb(mXlibRgbHandle, NS_TO_XXLIB_RGB(NS_RGB(0xFF, 0xFF, 0xFF))); 
  foreground = xxlib_rgb_xpixel_from_rgb(mXlibRgbHandle, NS_TO_XXLIB_RGB(NS_RGB(0x00, 0x00, 0x00))); 
  parent_win = XRootWindow(mPDisplay, mScreenNumber);                                         
                                             
  xattributes.background_pixel = background;
  xattributes.border_pixel     = foreground;
  xattributes.colormap         = xxlib_rgb_get_cmap(mXlibRgbHandle);
  xattributes_mask             = CWBorderPixel | CWBackPixel;
  if( xattributes.colormap != None )
  {
    xattributes_mask |= CWColormap;

    

    if (mDepth > 12)
    {
      XInstallColormap(mPDisplay, xattributes.colormap);
    }  
  }

  mDrawable = (Drawable)XCreateWindow(mPDisplay, parent_win, x, y,
                                      width, height, 0,
                                      mDepth, InputOutput, mVisual, xattributes_mask,
                                      &xattributes);

  
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG,
         ("nsXPrintContext::SetupWindow: mVisual->visualid=%x, mVisual->c_class=%x, mDepth=%d, mScreenNumber=%d, colormap=%lx, mDrawable=%lx\n",
         (int)mVisual->visualid, (int)mVisual->c_class, (int)mDepth, (int)mScreenNumber, (long)xattributes.colormap, (long)mDrawable));
         
  return NS_OK;
}


nsresult nsXPrintContext::SetMediumSize(const char *aPaperName)
{
  nsresult                rv = NS_ERROR_GFX_PRINTER_PAPER_SIZE_NOT_SUPPORTED;
  XpuMediumSourceSizeList mlist;
  int                     mlist_count;
  int                     i;
  char                   *paper_name,
                         *alloc_paper_name; 
  paper_name = alloc_paper_name = strdup(aPaperName);
  if (!paper_name)
    return NS_ERROR_OUT_OF_MEMORY;
  
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("SetMediumSize: Requested page '%s'\n", paper_name));

  mlist = XpuGetMediumSourceSizeList(mPDisplay, mPContext, &mlist_count);
  if( !mlist )
  {
    return NS_ERROR_GFX_PRINTER_PAPER_SIZE_NOT_SUPPORTED;
  }

  XpuMediumSourceSizeRec *match = nsnull;

#ifdef PR_LOGGING 
  
  for( i = 0 ; i < mlist_count ; i++ )
  {
    XpuMediumSourceSizeRec *curr = &mlist[i];
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("got '%s'/'%s'\t%d %f %f %f %f\n", 
           XPU_NULLXSTR(curr->tray_name), curr->medium_name, curr->mbool, 
           curr->ma1, curr->ma2, curr->ma3, curr->ma4));
  }
#endif 

  char *s;
  
  
  if ((s = strchr(paper_name, '/')) != nsnull)
  {
    const char *tray_name;
    *s = '\0';
    tray_name  = paper_name;
    paper_name = s+1;
    
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("SetMediumSize: searching for '%s'/'%s'\n", tray_name, paper_name));
    match = XpuFindMediumSourceSizeByName(mlist, mlist_count, tray_name, paper_name);
  }
  else
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("SetMediumSize: searching for '%s'\n", paper_name));
    match = XpuFindMediumSourceSizeByName(mlist, mlist_count, nsnull, paper_name);
  }
  
  
  if (match)
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG,
           ("match '%s'/'%s' !\n", XPU_NULLXSTR(match->tray_name), match->medium_name));
           
    
    if( XpuSetDocMediumSourceSize(mPDisplay, mPContext, match) == 1 )
      rv = NS_OK;  
  }
  
  XpuFreeMediumSourceSizeList(mlist);
  free(alloc_paper_name);
  
  return rv;
}

nsresult nsXPrintContext::SetOrientation(int landscape)
{
  const char         *orientation;
  XpuOrientationList  list;
  int                 list_count;
  XpuOrientationRec  *match;

  
  switch( landscape )
  {
    case 1 : orientation = "landscape"; break;
    case 0 :  orientation = "portrait";  break;
    default:  
      PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, 
             ("Unsupported orientation %d.\n", landscape));  
      return NS_ERROR_GFX_PRINTER_ORIENTATION_NOT_SUPPORTED;
  }
  
  
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("orientation=%s\n", orientation));    

  
  list = XpuGetOrientationList(mPDisplay, mPContext, &list_count);
  if( !list )
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuGetOrientationList() failure.\n"));  
    return NS_ERROR_GFX_PRINTER_ORIENTATION_NOT_SUPPORTED;
  }

#ifdef PR_LOGGING 
  int i;
  
  for( i = 0 ; i < list_count ; i++ )
  {
    XpuOrientationRec *curr = &list[i];
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("got orientation='%s'\n", curr->orientation));
  }
#endif 

  
  match = XpuFindOrientationByName(list, list_count, orientation);
  if (!match)
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuFindOrientationByName() failure.\n"));  
    XpuFreeOrientationList(list);
    return NS_ERROR_GFX_PRINTER_ORIENTATION_NOT_SUPPORTED;
  }

  
  if (XpuSetDocOrientation(mPDisplay, mPContext, match) != 1)
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuSetDocOrientation() failure.\n"));  
    
    




    if (list_count != 1)
    {
      
      XpuFreeOrientationList(list);
      return NS_ERROR_GFX_PRINTER_ORIENTATION_NOT_SUPPORTED;
    }
  }
  
  XpuFreeOrientationList(list);

  return NS_OK;
}

nsresult nsXPrintContext::SetPlexMode(const char *plexname)
{
  XpuPlexList  list;
  int          list_count;
  XpuPlexRec  *match;

  
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("plexname=%s\n", plexname));    

  
  list = XpuGetPlexList(mPDisplay, mPContext, &list_count);
  if( !list )
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuGetPlexList() failure.\n"));  
    return NS_ERROR_GFX_PRINTER_PLEX_NOT_SUPPORTED;
  }

#ifdef PR_LOGGING 
  int i;
  
  for( i = 0 ; i < list_count ; i++ )
  {
    XpuPlexRec *curr = &list[i];
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("got plex='%s'\n", curr->plex));
  }
#endif 

  
  match = XpuFindPlexByName(list, list_count, plexname);
  if (!match)
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuFindPlexByName() failure.\n"));  
    XpuFreePlexList(list);
    return NS_ERROR_GFX_PRINTER_PLEX_NOT_SUPPORTED;
  }

  
  if (XpuSetDocPlex(mPDisplay, mPContext, match) != 1)
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuSetDocPlex() failure.\n"));  
    
    




    if (list_count != 1)
    {
      
      XpuFreePlexList(list);
      return NS_ERROR_GFX_PRINTER_PLEX_NOT_SUPPORTED;
    }
  }
  
  XpuFreePlexList(list);

  return NS_OK;
}


nsresult
nsXPrintContext::SetupPrintContext(nsIDeviceContextSpecXp *aSpec)
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::SetupPrintContext()\n"));
  
  float        top, bottom, left, right;
  int          landscape;
  int          num_copies;
  const char  *printername;
  nsresult     rv;

  nsCOMPtr<nsIEnvironment> uEnv = do_GetService("@mozilla.org/process/environment;1", &rv);
  if (NS_FAILED(rv))
    return rv;

  
  aSpec->GetToPrinter(mIsAPrinter);
  aSpec->GetGrayscale(mIsGrayscale);
  aSpec->GetTopMargin(top);
  aSpec->GetBottomMargin(bottom);
  aSpec->GetLeftMargin(left);
  aSpec->GetRightMargin(right);
  aSpec->GetLandscape(landscape);
  aSpec->GetCopies(num_copies);
  
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, 
         ("nsXPrintContext::SetupPrintContext: borders top=%f, bottom=%f, left=%f, right=%f\n", 
         top, bottom, left, right));

  


  aSpec->GetPrinterName(&printername);
  
  
  if (!mIsAPrinter) 
  {
    




    aSpec->GetPath(&mPrintFile);

    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("print to file '%s'\n", XPU_NULLXSTR(mPrintFile)));
    
    if(!mPrintFile || !*mPrintFile)
      return NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE;
  }

  



  uEnv->Set(NS_LITERAL_STRING("XSUNTRANSPORT"), NS_LITERAL_STRING("xxx"));
     
  
  if( XpuGetPrinter(printername, &mPDisplay, &mPContext) != 1 )
    return NS_ERROR_GFX_PRINTER_NAME_NOT_FOUND;

  

  uEnv->Set(NS_LITERAL_STRING("XPRINTER"), NS_ConvertUTF8toUTF16(printername));

  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, 
         ("nsXPrintContext::SetupPrintContext: name='%s', display='%s', vendor='%s', release=%ld\n",
          printername,
          XDisplayString(mPDisplay), 
          XServerVendor(mPDisplay),
          (long)XVendorRelease(mPDisplay)));
          
  if (NS_FAILED(rv = AlertBrokenXprt(mPDisplay)))
    return rv;
    
  if( XpQueryExtension(mPDisplay, &mXpEventBase, &mXpErrorBase) == False )
    return NS_ERROR_UNEXPECTED;
    
#ifdef XPRINT_DEBUG_SOMETIMES_USEFULL
  dumpXpAttributes(mPDisplay, mPContext);
#endif 
  
  const char *paper_name      = nsnull,
             *plex_name       = nsnull,
             *resolution_name = nsnull;
  PRBool      downloadfonts   = PR_TRUE;
  aSpec->GetPaperName(&paper_name);
  aSpec->GetPlexName(&plex_name);
  aSpec->GetResolutionName(&resolution_name);
  aSpec->GetDownloadFonts(downloadfonts);
  
  if (NS_FAILED(XPU_TRACE(rv = SetMediumSize(paper_name))))
    return rv;
  
  if (NS_FAILED(XPU_TRACE(rv = SetOrientation(landscape))))
    return rv;

  if (NS_FAILED(XPU_TRACE(rv = SetPlexMode(plex_name))))
    return rv;
    
  if (NS_FAILED(XPU_TRACE(rv = SetResolution(resolution_name))))
    return rv;
   
  if (XPU_TRACE(XpuSetDocumentCopies(mPDisplay, mPContext, num_copies)) != 1)
    return NS_ERROR_GFX_PRINTER_TOO_MANY_COPIES;

  if (XPU_TRACE(XpuSetEnableFontDownload(mPDisplay, mPContext, downloadfonts)) != 1)
    return NS_ERROR_GFX_PRINTER_DRIVER_CONFIGURATION_ERROR;
        
  





  XPU_TRACE(XpSetContext(mPDisplay, mPContext));

#ifdef XPRINT_DEBUG_SOMETIMES_USEFULL
  dumpXpAttributes(mPDisplay, mPContext);
#endif 

  
  if( XpuGetResolution(mPDisplay, mPContext, &mPrintXResolution, &mPrintYResolution) != 1 )
    return NS_ERROR_GFX_PRINTER_DRIVER_CONFIGURATION_ERROR;

  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG,
         ("print resolution %ld%x%ld\n", mPrintXResolution, mPrintYResolution));
  
    
  XpSelectInput(mPDisplay, mPContext, XPPrintMask);  

  return NS_OK;
}

NS_IMETHODIMP
nsXPrintContext::SetResolution(const char *resolution_name)
{
  XpuResolutionList list;
  int               list_count;
  XpuResolutionRec *match;
  int               i;
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::SetResolution('resolution_name=%s').\n", resolution_name));  

  list = XpuGetResolutionList(mPDisplay, mPContext, &list_count);
  if( !list )
    return NS_ERROR_GFX_PRINTER_DRIVER_CONFIGURATION_ERROR;

#ifdef PR_LOGGING 
  
  for( i = 0 ; i < list_count ; i++ )
  {
    XpuResolutionRec *curr = &list[i];
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG,
           ("got resolution='%s'/%ldx%ld\n", curr->name, curr->x_dpi, curr->y_dpi));
  }
#endif 

  
  match = XpuFindResolutionByName(list, list_count, resolution_name);
  if (!match)
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuFindResolutionByName() failure.\n"));  
    XpuFreeResolutionList(list);
    return NS_ERROR_GFX_PRINTER_RESOLUTION_NOT_SUPPORTED;
  } 

  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("setting resolution to '%s'/%ldx%ld DPI.\n", 
         match->name, match->x_dpi, match->y_dpi));  
    
  if (XpuSetDocResolution(mPDisplay, mPContext, match) != 1)
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuSetDocResolution() failure.\n"));
 
    




    if (list_count != 1)
    {
      
      XpuFreeResolutionList(list);
      return NS_ERROR_GFX_PRINTER_DRIVER_CONFIGURATION_ERROR;
    }
  }
  
  XpuFreeResolutionList(list);
  
  return NS_OK;
}
  
NS_IMETHODIMP
nsXPrintContext::BeginDocument( PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage )
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::BeginDocument(aTitle='%s')\n", ((aTitle)?(NS_ConvertUTF16toUTF8(aTitle).get()):("<NULL>"))));
  
  nsXPIDLCString job_title;
       
  if (aTitle)
  {
    


    job_title.Assign(NS_ConvertUTF16toUTF8(aTitle));
  }
  else
  {
    job_title.AssignLiteral("Document without title");
  }
 
  
  XpuSetJobTitle(mPDisplay, mPContext, job_title);
  
  
  if(mIsAPrinter) 
  {
    XPU_TRACE(XpuStartJobToSpooler(mPDisplay));
  } 
  else 
  {   
    if( XPU_TRACE(mXpuPrintToFileHandle = XpuStartJobToFile(mPDisplay, mPContext, mPrintFile)) == nsnull )
    {
      PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, 
             ("nsXPrintContext::BeginDocument(): XpuPrintToFile failure %s/(%d)\n", 
             strerror(errno), errno));

      return NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE;
    }    
  } 

  XPU_TRACE(XpuWaitForPrintNotify(mPDisplay, mXpEventBase, XPStartJobNotify));

  mJobStarted = PR_TRUE;
  
  return NS_OK;
}  

NS_IMETHODIMP
nsXPrintContext::BeginPage()
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::BeginPage()\n"));
   
  XPU_TRACE(XpStartPage(mPDisplay, mDrawable));
  XPU_TRACE(XpuWaitForPrintNotify(mPDisplay, mXpEventBase, XPStartPageNotify));

  return NS_OK;
}

NS_IMETHODIMP 
nsXPrintContext::EndPage()
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::EndPage()\n"));

  XPU_TRACE(XpEndPage(mPDisplay));
  XPU_TRACE(XpuWaitForPrintNotify(mPDisplay, mXpEventBase, XPEndPageNotify));
  
  return NS_OK;
}

NS_IMETHODIMP 
nsXPrintContext::EndDocument()
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::EndDocument()\n"));

  XPU_TRACE(XpEndJob(mPDisplay));
  XPU_TRACE(XpuWaitForPrintNotify(mPDisplay, mXpEventBase, XPEndJobNotify));

  
  if( !mIsAPrinter )
  {
    NS_ASSERTION(nsnull != mXpuPrintToFileHandle, "mXpuPrintToFileHandle is null.");
    
    if( XPU_TRACE(XpuWaitForPrintFileChild(mXpuPrintToFileHandle)) == XPGetDocFinished )
    {
      PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuWaitForPrintFileChild returned success.\n"));
    }
    else
    {
      PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuWaitForPrintFileChild returned failure.\n"));
    }
    
    mXpuPrintToFileHandle = nsnull;
  }

  


  const char *results,
             *lresults;
  results = XpGetOneAttribute(mPDisplay, mPContext, XPJobAttr,
  			      "xp-spooler-command-results");

  if( results &&
      (strlen(results) != 0) )
  {
    lresults = XpuCompoundTextToXmb(mPDisplay, results);
  }
  else
  {
    lresults = NULL;
  }
  


  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG,
         ("Printing complete - spooler command result '%s'/'%s'\n",
  	  results  ? results  : "<no message>",
	  lresults ? lresults : ""));
  if( lresults )
    XpuFreeXmbString(lresults);

  if( results )
    XFree((void *)results);

  mJobStarted = PR_FALSE;
    
  return NS_OK;
}

NS_IMETHODIMP 
nsXPrintContext::AbortDocument()
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::AbortDocument()\n"));

  if( mJobStarted )
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("canceling...\n"));
    XPU_TRACE(XpCancelJob(mPDisplay, True));
  }  

  
  if( !mIsAPrinter && mXpuPrintToFileHandle )
  {   
    if( XPU_TRACE(XpuWaitForPrintFileChild(mXpuPrintToFileHandle)) == XPGetDocFinished )
    {
      PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuWaitForPrintFileChild returned success.\n"));
    }
    else
    {
      PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("XpuWaitForPrintFileChild returned failure.\n"));
    }

    mXpuPrintToFileHandle = nsnull;
  }

  mJobStarted = PR_FALSE;

  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("print job aborted.\n"));

  return NS_OK;
}

static
PRUint8 *ComposeAlphaImage(
               PRUint8 *alphaBits, PRInt32  alphaRowBytes, PRUint8 alphaDepth,
               PRUint8 *image_bits, PRInt32  row_bytes,
               PRInt32 aWidth, PRInt32 aHeight)
{
  PRUint8 *composed_bits; 
  if (!(composed_bits = (PRUint8 *)PR_Malloc(aHeight * row_bytes)))
    return nsnull;


#ifdef XPRINT_NOT_NOW
  XGCValues gcv;
  memset(&gcv, 0, sizeof(XGCValues)); 
  XGetGCValues(mPDisplay, *xgc, GCForeground, &gcv);
  
  
  XColor color;
  color.pixel = gcv.foreground;
  XQueryColor(mPDisplay, xxlib_rgb_get_cmap(mXlibRgbHandle), &color);
  
  unsigned long background = NS_RGB(color.red>>8, color.green>>8, color.blue>>8);
#else
  unsigned long background = NS_RGB(0xFF,0xFF,0xFF); 
#endif 
  long x, y;
    
  switch(alphaDepth)
  {
    case 1:
    {
#define NS_GET_BIT(rowptr, x) (rowptr[(x)>>3] &  (1<<(7-(x)&0x7)))
        unsigned short r = NS_GET_R(background),
                       g = NS_GET_R(background),
                       b = NS_GET_R(background);
                     
        for(y = 0 ; y < aHeight ; y++)
        {
          unsigned char *imageRow  = image_bits    + y * row_bytes;
          unsigned char *destRow   = composed_bits + y * row_bytes;
          unsigned char *alphaRow  = alphaBits     + y * alphaRowBytes;
          for(x = 0 ; x < aWidth ; x++)
          {
            if(NS_GET_BIT(alphaRow, x))
            {
              
              destRow[3*x  ] = imageRow[3*x  ];
              destRow[3*x+1] = imageRow[3*x+1];
              destRow[3*x+2] = imageRow[3*x+2];
            }
            else
            {
              
              destRow[3*x  ] = r;
              destRow[3*x+1] = g;
              destRow[3*x+2] = b;
            }
          }
        }        
    }  
        break;
    case 8:
    {
        unsigned short r = NS_GET_R(background),
                       g = NS_GET_R(background),
                       b = NS_GET_R(background);
        
        for(y = 0 ; y < aHeight ; y++)
        {
          unsigned char *imageRow  = image_bits    + y * row_bytes;
          unsigned char *destRow   = composed_bits + y * row_bytes;
          unsigned char *alphaRow  = alphaBits     + y * alphaRowBytes;
          for(x = 0 ; x < aWidth ; x++)
          {
            unsigned short alpha = alphaRow[x];
            MOZ_BLEND(destRow[3*x  ], r, imageRow[3*x  ], alpha);
            MOZ_BLEND(destRow[3*x+1], g, imageRow[3*x+1], alpha);
            MOZ_BLEND(destRow[3*x+2], b, imageRow[3*x+2], alpha);
          }
        }
    }  
        break;
    default:
    {
        NS_WARNING("alpha depth x not supported");
        PR_Free(composed_bits);
        return nsnull;
    }  
        break;
  }
  
  return composed_bits;      
}


NS_IMETHODIMP
nsXPrintContext::DrawImage(Drawable aDrawable, xGC *xgc, nsIImage *aImage,
                 PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                 PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, 
         ("nsXPrintContext::DrawImage(%lx, %d/%d/%d/%d - %d/%d/%d/%d)\n",
          (long)aDrawable,
          (int)aSX, (int)aSY, (int)aSWidth, (int)aSHeight,
          (int)aDX, (int)aDY, (int)aDWidth, (int)aDHeight));
  
  double   scalingFactor;
  int      prev_res = 0,
           dummy;
  long     imageResolution;
  PRInt32  aDWidth_scaled,
           aDHeight_scaled;
  nsresult rv = NS_OK;
  
  PRInt32 aSrcWidth  = aImage->GetWidth();
  PRInt32 aSrcHeight = aImage->GetHeight();
  
  if( (aSrcWidth == 0) || (aSrcHeight == 0) ||
      (aSWidth == 0)   || (aSHeight == 0) ||
      (aDWidth == 0)   || (aDHeight == 0) )
  {
    NS_ASSERTION((aSrcWidth != 0) && (aSrcHeight != 0) &&
      (aSWidth != 0)   && (aSHeight != 0) ||
      (aDWidth != 0)   && (aDHeight != 0), 
      "nsXPrintContext::DrawImage(): Image with zero source||dest width||height suppressed\n");
    return NS_OK;
  }

  
  float pixelscale = 1.0;
  mContext->GetCanonicalPixelScale(pixelscale);
  scalingFactor = 1.0 / pixelscale;

  
  double scale_x = double(aSWidth)  / (double(aDWidth)  * scalingFactor);
  double scale_y = double(aSHeight) / (double(aDHeight) * scalingFactor);
  
  




  scalingFactor *= PR_MIN(scale_x, scale_y);
  
  
  imageResolution = long(   double(mPrintXResolution) * scalingFactor);
  aDWidth_scaled  = PRInt32(double(aDWidth)           * scalingFactor);
  aDHeight_scaled = PRInt32(double(aDHeight)          * scalingFactor);

  

  if( (aDWidth_scaled <= 0) || (aDHeight_scaled <= 0) )
  {
    
    scalingFactor = 1.0 / pixelscale; 
    scalingFactor *= PR_MAX(scale_x, scale_y);

    
    imageResolution = long(   double(mPrintXResolution) * scalingFactor);
    aDWidth_scaled  = PRInt32(double(aDWidth)           * scalingFactor);
    aDHeight_scaled = PRInt32(double(aDHeight)          * scalingFactor);
  }

  
  NS_ASSERTION(!((aDWidth_scaled <= 0) || (aDHeight_scaled <= 0)),
               "Image scaled to zero width/height");
  if( (aDWidth_scaled <= 0) || (aDHeight_scaled <= 0) )
    return NS_OK;

  
  NS_ASSERTION(imageResolution != 0, "Image resolution must not be 0");
  NS_ASSERTION(imageResolution >= 0, "Image resolution must not be negative");
  if( imageResolution <= 0 )
    return NS_OK;

  
  if( XpSetImageResolution(mPDisplay, mPContext, imageResolution, &prev_res) )
  {
    
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, 
           ("Xp scaling res=%d, aSWidth=%d, aSHeight=%d, aDWidth_scaled=%d, aDHeight_scaled=%d\n", 
            (int)imageResolution, (int)aSWidth, (int)aSHeight, (int)aDWidth_scaled, (int)aDHeight_scaled));
    
    if( (aSX != 0) || (aSY != 0) || (aSWidth != aDWidth_scaled) || (aSHeight != aDHeight_scaled) )
    {
      PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("using DrawImageBitsScaled()\n"));
      rv = DrawImageBitsScaled(aDrawable, xgc, aImage, aSX, aSY, aSWidth, aSHeight, aDX, aDY, aDWidth_scaled, aDHeight_scaled);
    }
    else
    {
      PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("using DrawImage() [shortcut]\n"));
      rv = DrawImage(aDrawable, xgc, aImage, aDX, aDY, aDWidth_scaled, aDHeight_scaled);
    }
    
    
    (void)XpSetImageResolution(mPDisplay, mPContext, prev_res, &dummy);
  }
  else 
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("BAD BAD local scaling... ;-((\n"));
    
    (void)XpSetImageResolution(mPDisplay, mPContext, prev_res, &dummy);
    
    
    rv = DrawImageBitsScaled(aDrawable, xgc, aImage, aSX, aSY, aSWidth, aSHeight, aDX, aDY, aDWidth, aDHeight);
  }
  
  return rv;
}  



nsresult
nsXPrintContext::DrawImageBitsScaled(Drawable aDrawable, xGC *xgc, nsIImage *aImage,
                 PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                 PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, 
         ("nsXPrintContext::DrawImageBitsScaled(%lx, %d/%d/%d/%d - %d/%d/%d/%d)\n",
          (long)aDrawable,
          (int)aSX, (int)aSY, (int)aSWidth, (int)aSHeight,
          (int)aDX, (int)aDY, (int)aDWidth, (int)aDHeight));

  nsresult rv = NS_OK;
  
  if (aDWidth==0 || aDHeight==0)
  {
    NS_ASSERTION((aDWidth==0 || aDHeight==0), 
                 "nsXPrintContext::DrawImageBitsScaled(): Image with zero dest width||height suppressed\n");
    return NS_OK;
  }
  
  aImage->LockImagePixels(PR_FALSE);
  
  PRUint8 *image_bits    = aImage->GetBits();
  PRInt32  row_bytes     = aImage->GetLineStride();
  PRUint8  imageDepth    = 24; 
  PRUint8 *alphaBits     = aImage->GetAlphaBits();
  PRInt32  alphaRowBytes = aImage->GetAlphaLineStride();
  int      alphaDepth    = aImage->GetAlphaDepth();
  PRInt32  aSrcWidth     = aImage->GetWidth();
  PRInt32  aSrcHeight    = aImage->GetHeight();
  PRUint8 *composed_bits = nsnull;

 
  if (!image_bits)
  {
    aImage->UnlockImagePixels(PR_FALSE);
    return NS_OK;
  }
 
  
  
  if( alphaBits != nsnull )
  {
    composed_bits = ComposeAlphaImage(alphaBits, alphaRowBytes, alphaDepth,
                                      image_bits, row_bytes,
                                      aSrcWidth, aSrcHeight);
    if (!composed_bits)
    {
      aImage->UnlockImagePixels(PR_FALSE);
      return NS_ERROR_FAILURE;
    }

    image_bits = composed_bits;
    alphaBits = nsnull; 





  }

#define ROUNDUP(nbytes, pad) ((((nbytes) + ((pad)-1)) / (pad)) * ((pad)>>3))

  PRInt32  srcimg_bytes_per_line = row_bytes;
  PRInt32  dstimg_bytes_per_line = ROUNDUP((imageDepth * aDWidth), 32);
  PRUint8 *srcimg_data           = image_bits;
  PRUint8 *dstimg_data           = (PRUint8 *)PR_Malloc((aDHeight+1) * dstimg_bytes_per_line); 
  if (!dstimg_data)
  {
    aImage->UnlockImagePixels(PR_FALSE);
    return NS_ERROR_FAILURE;
  }

  RectStretch(aSWidth, aSHeight,
              aDWidth, aDHeight,
              0, 0, aDWidth-1, aDHeight-1,
              srcimg_data, srcimg_bytes_per_line,
              dstimg_data, dstimg_bytes_per_line,
              imageDepth);

#ifdef XPRINT_SERVER_SIDE_ALPHA_COMPOSING

#endif 
    
  rv = DrawImageBits(aDrawable, xgc, alphaBits, alphaRowBytes, alphaDepth,
                     dstimg_data, dstimg_bytes_per_line, 
                     aDX, aDY, aDWidth, aDHeight);

  if (dstimg_data)   
    PR_Free(dstimg_data);
  if (composed_bits) 
    PR_Free(composed_bits);
  
  aImage->UnlockImagePixels(PR_FALSE);

  return rv;
}


NS_IMETHODIMP
nsXPrintContext::DrawImage(Drawable aDrawable, xGC *xgc, nsIImage *aImage,
                           PRInt32 aX, PRInt32 aY,
                           PRInt32 dummy1, PRInt32 dummy2)
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::DrawImage(%lx, %d/%d/%d(=dummy)/%d(=dummy))\n",
         (long)aDrawable,
         (int)aX, (int)aY, (int)dummy1, (int)dummy2));

  aImage->LockImagePixels(PR_FALSE);

  nsresult rv;
  PRInt32  width         = aImage->GetWidth();
  PRInt32  height        = aImage->GetHeight();
  PRUint8 *alphaBits     = aImage->GetAlphaBits();
  PRInt32  alphaRowBytes = aImage->GetAlphaLineStride();
  PRInt32  alphaDepth    = aImage->GetAlphaDepth();
  PRUint8 *image_bits    = aImage->GetBits();
  PRUint8 *composed_bits = nsnull;
  PRInt32  row_bytes     = aImage->GetLineStride();

  
  if (!image_bits)
  {
    aImage->UnlockImagePixels(PR_FALSE);
    return NS_OK;
  }
  
  
  
  if( alphaBits != nsnull )
  {
    composed_bits = ComposeAlphaImage(alphaBits, alphaRowBytes, alphaDepth,
                                      image_bits, row_bytes,
                                      width, height);
    if (!composed_bits)
    {
      aImage->UnlockImagePixels(PR_FALSE);
      return NS_ERROR_FAILURE;
    }

    image_bits = composed_bits;
    alphaBits = nsnull;
  }
               
  rv = DrawImageBits(aDrawable, xgc, alphaBits, alphaRowBytes, alphaDepth,
                     image_bits, row_bytes,
                     aX, aY, width, height);

  if (composed_bits)
    PR_Free(composed_bits);

  aImage->UnlockImagePixels(PR_FALSE);

  return rv;                     
}

                         

nsresult
nsXPrintContext::DrawImageBits(Drawable aDrawable, xGC *xgc,
                               PRUint8 *alphaBits, PRInt32  alphaRowBytes, PRUint8 alphaDepth,
                               PRUint8 *image_bits, PRInt32  row_bytes,
                               PRInt32 aX, PRInt32 aY,
                               PRInt32 aWidth, PRInt32 aHeight)
{ 
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::DrawImageBits(%lx, %d/%d/%d/%d)\n",
         (long)aDrawable, (int)aX, (int)aY, (int)aWidth, (int)aHeight));

  Pixmap alpha_pixmap  = None;
  GC     image_gc;

  if( (aWidth == 0) || (aHeight == 0) )
  {
    NS_ASSERTION((aWidth != 0) && (aHeight != 0), "Image with zero width||height suppressed.");
    return NS_OK;
  }
  

#ifdef XPRINT_SERVER_SIDE_ALPHA_COMPOSING
  
  if( alphaBits != nsnull )
  {
    XImage    *x_image = nsnull;
    GC         gc;
    XGCValues  gcv;
    
    alpha_pixmap = XCreatePixmap(mPDisplay, 
                                 aDrawable,
                                 aWidth, aHeight, 1); 
  
    
    x_image = XCreateImage(mPDisplay, mVisual,
                           1,                 
                           XYPixmap,
                           0,                 
                           (char *)alphaBits, 
                           aWidth,
                           aHeight,
                           32,                
                           alphaRowBytes);    

    x_image->bits_per_pixel = 1;

    
    x_image->bitmap_bit_order = MSBFirst;

    



    x_image->byte_order = MSBFirst;

    
    
    memset(&gcv, 0, sizeof(XGCValues)); 
    XGetGCValues(mPDisplay, *xgc, GCForeground|GCBackground, &gcv);
    gcv.function = GXcopy;
    gc = XCreateGC(mPDisplay, alpha_pixmap, GCForeground|GCBackground|GCFunction, &gcv);

    XPutImage(mPDisplay, alpha_pixmap, gc, x_image, 0, 0, 0, 0,
              aWidth, aHeight);
    XFreeGC(mPDisplay, gc);

    
    x_image->data = nsnull; 
    XDestroyImage(x_image);
  }
#endif   
  
  if( alpha_pixmap != None )
  {
    
    XGCValues gcv;  
    memset(&gcv, 0, sizeof(XGCValues)); 
    XGetGCValues(mPDisplay, *xgc, GCForeground|GCBackground, &gcv);
    gcv.function      = GXcopy;
    gcv.clip_mask     = alpha_pixmap;
    gcv.clip_x_origin = aX;
    gcv.clip_y_origin = aY;

    image_gc = XCreateGC(mPDisplay, aDrawable, 
                         (GCForeground|GCBackground|GCFunction|
                          GCClipXOrigin|GCClipYOrigin|GCClipMask),
                         &gcv);
  }
  else
  {
    

    image_gc = *xgc;
  }


  xxlib_draw_xprint_scaled_rgb_image(mXlibRgbHandle,
                                     aDrawable,
                                     mPrintXResolution, XpGetImageResolution(mPDisplay, mPContext),
                                     image_gc,
                                     aX, aY, aWidth, aHeight,
                                     NS_XPRINT_RGB_DITHER,
                                     image_bits, row_bytes);
  
  if( alpha_pixmap != None ) 
  {   
    XFreeGC(mPDisplay, image_gc);
    XFreePixmap(mPDisplay, alpha_pixmap);
  }
    
  return NS_OK;
}

NS_IMETHODIMP nsXPrintContext::GetPrintResolution(int &aXres, int &aYres)
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, 
         ("nsXPrintContext::GetPrintResolution() res=%ldx%ld, mPContext=%lx\n",
          mPrintXResolution, mPrintYResolution,(long)mPContext));
  
  if(mPContext!=None)
  {
    aXres = mPrintXResolution;
    aYres = mPrintYResolution;
    return NS_OK;
  }
  
  aXres = aYres = 0;
  return NS_ERROR_FAILURE;    
}







class nsEPSObjectXp {
  public:
      


      nsEPSObjectXp(const unsigned char *aData, unsigned long aDataLength) :
        mStatus(NS_ERROR_INVALID_ARG),
        mData(nsnull),
        mDataLength(0UL),
        mCurrPos(nsnull),
        mBBllx(0.0),
        mBBlly(0.0),
        mBBurx(0.0),
        mBBury(0.0)
      {
        mData       = NS_REINTERPRET_CAST(const char*, aData);
        mDataLength = aDataLength;

        NS_PRECONDITION(aData != nsnull,   "aData == nsnull");
        NS_PRECONDITION(aDataLength > 0UL, "No data");    

        Reset();
        Parse();
      }
      
      static inline
      PRBool IsEPSF(const unsigned char *aData, unsigned long aDataLength)
      {
        


        return (PL_strnstr(NS_REINTERPRET_CAST(const char*, aData), " EPSF-", PR_MIN(aDataLength, 256)) != nsnull);
      }
      

      




      nsresult GetStatus() { return mStatus; };

      



      inline void GetBoundingBox(PRFloat64 &aBBllx,
                          PRFloat64 &aBBlly,
                          PRFloat64 &aBBurx,
                          PRFloat64 &aBBury)
      {
        aBBllx = mBBllx;
        aBBlly = mBBlly;
        aBBurx = mBBurx;
        aBBury = mBBury;
      };

      


      void AppendTo(nsACString& aDestBuffer)
      {
        nsCAutoString line;
        PRBool        inPreview = PR_FALSE;

        Reset();
        while (EPSFFgets(line)) {
          if (inPreview) {
            
            if (StringBeginsWith(line, NS_LITERAL_CSTRING("%%EndPreview")))
                inPreview = PR_FALSE;
            continue;
          }
          else if (StringBeginsWith(line, NS_LITERAL_CSTRING("%%BeginPreview:"))){
            inPreview = PR_TRUE;
            continue;
          }

          
          aDestBuffer.Append(line.get(), line.Length());
          aDestBuffer.Append(NS_LITERAL_CSTRING("\n"));
        }
      }
  private:
      nsresult             mStatus;
      const char          *mData;
      unsigned long        mDataLength;
      const char          *mCurrPos;
      PRFloat64            mBBllx,
                           mBBlly,
                           mBBurx,
                           mBBury;

      void Parse()
      {
        nsCAutoString line;

        Reset();   
        while (EPSFFgets(line)) {
          if (PR_sscanf(line.get(), "%%%%BoundingBox: %lf %lf %lf %lf",
                        &mBBllx, &mBBlly, &mBBurx, &mBBury) == 4) {
            mStatus = NS_OK;
            return;
          }
        }
        mStatus = NS_ERROR_INVALID_ARG;
      }
      
      inline void Reset()
      {
        mCurrPos = mData;
      }

      PRBool EPSFFgets(nsACString& aBuffer)
      {
        aBuffer.Truncate();
        
        if (!mCurrPos)
          return PR_FALSE;
        
        while (1) {
          int ch = *mCurrPos++;
          if ('\n' == ch) {
            
            ch = *mCurrPos++;
            if ((mCurrPos < (mData + mDataLength)) && ('\r' != ch))
              mCurrPos--;
            return PR_TRUE;
          }
          else if ('\r' == ch) {
            
            ch = *mCurrPos++;
            if ((mCurrPos < (mData + mDataLength)) && ('\n' != ch))
              mCurrPos--;
            return PR_TRUE;
          }
          else if (mCurrPos >= (mData + mDataLength)) {
            
            return !aBuffer.IsEmpty();
          }

          
          aBuffer.Append((char)ch);
        }
      }
};

NS_IMETHODIMP nsXPrintContext::RenderEPS(Drawable aDrawable,
                                         const nsRect& aRect,
                                         const unsigned char *aData, unsigned long aDatalen)
{
  PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, 
         ("nsXPrintContext::EPS(aData, aDatalen=%d)\n", aDatalen));
  
  char xp_formats_supported[] = "xp-embedded-formats-supported";
  const char *embedded_formats_supported = XpGetOneAttribute(mPDisplay, mPContext, XPPrinterAttr, xp_formats_supported);

  







  if (embedded_formats_supported == NULL)
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, ("nsXPrintContext::RenderPostScriptDataFragment(): Embedding data not supported for this DDX/Printer\n"));
    return NS_ERROR_FAILURE;    
  }

  if( PL_strcasestr(embedded_formats_supported, "PostScript 2") == NULL )
  {
    PR_LOG(nsXPrintContextLM, PR_LOG_DEBUG, 
           ("nsXPrintContext::RenderPostScriptDataFragment(): Embedding data not supported for this DDX/Printer "
            "(supported embedding formats are '%s')\n", embedded_formats_supported));
    XFree((void *)embedded_formats_supported);
    return NS_ERROR_FAILURE;    
  }

  


  nsXPIDLCString  aBuffer;
  const unsigned char *embedData;
  unsigned long        embedDataLength;
    
  
  if (nsEPSObjectXp::IsEPSF(aData, aDatalen))
  {
    PRFloat64 boxLLX, 
              boxLLY,
              boxURX,
              boxURY;

    nsEPSObjectXp epsfData(aData, aDatalen);
    
    if (NS_FAILED(epsfData.GetStatus()))
      return NS_ERROR_INVALID_ARG;  

    epsfData.GetBoundingBox(boxLLX, boxLLY, boxURX, boxURY);

    
    aBuffer.SetCapacity(aDatalen + 1024); 
    aBuffer.Assign("%%BeginDocument: Mozilla EPSF plugin data\n"
                   "/b4_Inc_state save def\n"
                   "/dict_count countdictstack def\n"
                   "/op_count count 1 sub def\n"
                   "userdict begin\n"
                   "/showpage { } def\n"
                   "0 setgray 0 setlinecap 1 setlinewidth 0 setlinejoin\n"
                   "10 setmiterlimit [ ] 0 setdash newpath\n"
                   "/languagelevel where\n"
                   "{pop languagelevel\n"
                   "  1 ne\n"
                   "  {false setstrokeadjust false setoverprint\n"
                   "  } if\n"
                   "} if\n");

    
    aBuffer.Append(nsPrintfCString(64, "%f %f translate\n", 
                   double(aRect.x),
                   double(aRect.y + aRect.height)));

    
    aBuffer.Append(nsPrintfCString(64, "%f %f scale\n", 
                   double(aRect.width / (boxURX - boxLLX)),
                   double(-(aRect.height / (boxURY - boxLLY)))));

    


    aBuffer.Append(nsPrintfCString(64, "%f %f translate\n", double(-boxLLX), double(-boxLLY)));

    epsfData.AppendTo(aBuffer);
    aBuffer.Append("count op_count sub { pop } repeat\n"
                   "countdictstack dict_count sub { end } repeat\n"
                   "b4_Inc_state restore\n"
                   "%%EndDocument\n");
    embedData       = NS_REINTERPRET_CAST(const unsigned char*, aBuffer.get());
    embedDataLength = aBuffer.Length();
  }
  else
  {
    
    embedData       = aData;
    embedDataLength = aDatalen;
  }
  
  


  const char *type     = "PostScript 2"; 



  const char *option   = "";             



  
  XpPutDocumentData(mPDisplay, aDrawable, (unsigned char *)embedData, embedDataLength, (char *)type, (char *)option);

  XFree((void *)embedded_formats_supported);
  
  return NS_OK;
}



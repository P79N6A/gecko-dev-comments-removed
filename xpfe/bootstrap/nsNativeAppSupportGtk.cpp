








































#include "nsStringSupport.h"

#include "nsNativeAppSupportBase.h"
#include "gdk/gdk.h"
#include "prenv.h"
#ifdef MOZ_XUL_APP
extern char* splash_xpm[];
#else
#include SPLASH_XPM
#endif

class nsSplashScreenGtk : public nsISplashScreen {
public:
  nsSplashScreenGtk();
  virtual ~nsSplashScreenGtk();

  NS_IMETHOD Show();
  NS_IMETHOD Hide();

  NS_DECL_ISUPPORTS

private:
  GdkWindow *mDialog;
}; 

class nsNativeAppSupportGtk : public nsNativeAppSupportBase {
  
};

NS_IMPL_ISUPPORTS1(nsSplashScreenGtk, nsISplashScreen)

nsSplashScreenGtk::nsSplashScreenGtk()
{
}

nsSplashScreenGtk::~nsSplashScreenGtk()
{
  Hide();
}

NS_IMETHODIMP nsSplashScreenGtk::Show()
{
#ifdef MOZ_XUL_APP
  if (!splash_xpm[0])
    return NS_OK;
#endif

  nsCAutoString path(PR_GetEnv("MOZILLA_FIVE_HOME"));

  if (path.IsEmpty()) {
    path.Assign("splash.xpm");
  } else {
    path.Append("/splash.xpm");
  }

  
  GdkPixmap* pmap = gdk_pixmap_colormap_create_from_xpm(NULL,
                                                    gdk_colormap_get_system(),
                                                    NULL, NULL, path.get());

  if (!pmap) {
    
    pmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
                                                    gdk_colormap_get_system(),
                                                    NULL, NULL, splash_xpm);
  }

  if (!pmap) {
    gdk_window_destroy(mDialog);
    mDialog = nsnull;
    return NS_ERROR_FAILURE;
  }

  gint width, height;
  gdk_window_get_size(pmap, &width, &height);

  GdkWindowAttr attr;
  attr.window_type = GDK_WINDOW_TEMP;
  attr.wclass = GDK_INPUT_OUTPUT;
  attr.x = (gdk_screen_width() >> 1) - (width >> 1);
  attr.y = (gdk_screen_height() >> 1) - (height >> 1);
  attr.width = width;
  attr.height = height;
  attr.event_mask = GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK;
  mDialog = gdk_window_new(NULL, &attr, GDK_WA_X | GDK_WA_Y);

  gdk_window_set_back_pixmap(mDialog, pmap, FALSE);
  gdk_pixmap_unref(pmap);

  gdk_window_show(mDialog);

  return NS_OK;
}

NS_IMETHODIMP nsSplashScreenGtk::Hide()
{
  if (mDialog) {
    gdk_window_destroy(mDialog);
    mDialog = nsnull;
  }
  return NS_OK;
}

nsresult NS_CreateNativeAppSupport(nsINativeAppSupport** aNativeApp) {
  *aNativeApp = new nsNativeAppSupportGtk;
  NS_ADDREF(*aNativeApp);
  return NS_OK;
}

nsresult NS_CreateSplashScreen(nsISplashScreen** aSplash) {
  *aSplash = new nsSplashScreenGtk;
  NS_ADDREF(*aSplash);
  return NS_OK;
}

PRBool NS_CanRun()
{
  return PR_TRUE;
}

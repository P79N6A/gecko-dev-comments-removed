




































#include "nsScreenManagerGtk.h"
#include "nsScreenGtk.h"
#include "nsIComponentManager.h"
#include "nsRect.h"
#include "nsAutoPtr.h"

#include <gdk/gdkx.h>

#ifdef MOZ_ENABLE_XINERAMA

extern "C"
{
#include <X11/extensions/Xinerama.h>
}
#endif 

nsScreenManagerGtk :: nsScreenManagerGtk ( )
{
  
  
  
  mNumScreens = 0;
}


nsScreenManagerGtk :: ~nsScreenManagerGtk()
{
  
}



NS_IMPL_ISUPPORTS1(nsScreenManagerGtk, nsIScreenManager)



nsresult
nsScreenManagerGtk :: EnsureInit(void)
{
  if (!mCachedScreenArray) {
    mCachedScreenArray = do_CreateInstance("@mozilla.org/supports-array;1");
    if (!mCachedScreenArray) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
#ifdef MOZ_ENABLE_XINERAMA
    
    XineramaScreenInfo *screenInfo;
    if (XineramaIsActive(GDK_DISPLAY())) {
      screenInfo = XineramaQueryScreens(GDK_DISPLAY(), &mNumScreens);
    }
    else {
      screenInfo = NULL;
      mNumScreens = 1;
    }
#else
    mNumScreens = 1;
#endif
    
    
    
    if (mNumScreens < 2) {
      mNumScreens = 1;
      nsRefPtr<nsScreenGtk> screen = new nsScreenGtk();
      if (!screen)
        return NS_ERROR_OUT_OF_MEMORY;

      screen->Init();

      nsISupports *supportsScreen = screen;
      mCachedScreenArray->AppendElement(supportsScreen);
    }
    
    
    
#ifdef MOZ_ENABLE_XINERAMA
    else {
#ifdef DEBUG
      printf("Xinerama superpowers activated for %d screens!\n", mNumScreens);
#endif
      int i;
      for (i=0; i < mNumScreens; i++) {
        nsRefPtr<nsScreenGtk> screen = new nsScreenGtk();
        if (!screen) {
          return NS_ERROR_OUT_OF_MEMORY;
        }

        
        screen->Init(&screenInfo[i]);

        nsISupports *screenSupports = screen;
        mCachedScreenArray->AppendElement(screenSupports);
      }
    }
#endif 
  }

  return NS_OK;;
}










NS_IMETHODIMP
nsScreenManagerGtk :: ScreenForRect ( PRInt32 aX, PRInt32 aY,
                                      PRInt32 aWidth, PRInt32 aHeight,
                                      nsIScreen **aOutScreen )
{
  nsresult rv;
  rv = EnsureInit();
  if (NS_FAILED(rv)) {
    NS_ERROR("nsScreenManagerGtk::EnsureInit() failed from ScreenForRect\n");
    return rv;
  }
  
  PRUint32 which = 0;
  
  
  
  if (mNumScreens > 1) {
    
    
    PRUint32 count;
    mCachedScreenArray->Count(&count);
    PRUint32 i;
    PRUint32 area = 0;
    nsRect   windowRect(aX, aY, aWidth, aHeight);
    for (i=0; i < count; i++) {
      PRInt32  x, y, width, height;
      x = y = width = height = 0;
      nsCOMPtr<nsIScreen> screen;
      mCachedScreenArray->GetElementAt(i, getter_AddRefs(screen));
      screen->GetRect(&x, &y, &width, &height);
      
      nsRect screenRect(x, y, width, height);
      screenRect.IntersectRect(screenRect, windowRect);
      PRUint32 tempArea = screenRect.width * screenRect.height;
      if (tempArea >= area) {
        which = i;
        area = tempArea;
      }
    }
  }
  nsCOMPtr<nsIScreen> outScreen;
  mCachedScreenArray->GetElementAt(which, getter_AddRefs(outScreen));
  *aOutScreen = outScreen.get();
  NS_IF_ADDREF(*aOutScreen);
  return NS_OK;
    
} 








NS_IMETHODIMP 
nsScreenManagerGtk :: GetPrimaryScreen(nsIScreen * *aPrimaryScreen) 
{
  nsresult rv;
  rv =  EnsureInit();
  if (NS_FAILED(rv)) {
    NS_ERROR("nsScreenManagerGtk::EnsureInit() failed from GetPrimaryScreen\n");
    return rv;
  }
  nsCOMPtr <nsIScreen> screen;
  mCachedScreenArray->GetElementAt(0, getter_AddRefs(screen));
  *aPrimaryScreen = screen.get();
  NS_IF_ADDREF(*aPrimaryScreen);
  return NS_OK;
  
} 







NS_IMETHODIMP
nsScreenManagerGtk :: GetNumberOfScreens(PRUint32 *aNumberOfScreens)
{
  nsresult rv;
  rv = EnsureInit();
  if (NS_FAILED(rv)) {
    NS_ERROR("nsScreenManagerGtk::EnsureInit() failed from GetNumberOfScreens\n");
    return rv;
  }
  *aNumberOfScreens = mNumScreens;
  return NS_OK;
  
} 

NS_IMETHODIMP
nsScreenManagerGtk :: ScreenForNativeWidget(void *nativeWidget, nsIScreen **aScreen)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

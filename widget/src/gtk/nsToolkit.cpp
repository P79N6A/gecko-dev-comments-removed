




































#include "nscore.h"  
#include "nsToolkit.h"
#include "nsWidgetAtoms.h"





static PRUintn gToolkitTLSIndex = 0;






nsToolkit::nsToolkit()
{
  mSharedGC = nsnull;
}






nsToolkit::~nsToolkit()
{
  if (mSharedGC) {
    gdk_gc_unref(mSharedGC);
  }

  
  PR_SetThreadPrivate(gToolkitTLSIndex, nsnull);
}







NS_IMPL_ISUPPORTS1(nsToolkit, nsIToolkit)

void nsToolkit::CreateSharedGC(void)
{
  GdkPixmap *pixmap;

  if (mSharedGC)
    return;

  pixmap = gdk_pixmap_new(NULL, 1, 1, gdk_rgb_get_visual()->depth);
  mSharedGC = gdk_gc_new(pixmap);
  gdk_pixmap_unref(pixmap);
}

GdkGC *nsToolkit::GetSharedGC(void)
{
  return gdk_gc_ref(mSharedGC);
}





NS_IMETHODIMP nsToolkit::Init(PRThread *aThread)
{
  CreateSharedGC();

  nsWidgetAtoms::RegisterAtoms();

  return NS_OK;
}








NS_METHOD NS_GetCurrentToolkit(nsIToolkit* *aResult)
{
  nsIToolkit* toolkit = nsnull;
  nsresult rv = NS_OK;
  PRStatus status;

  
  if (0 == gToolkitTLSIndex) {
    status = PR_NewThreadPrivateIndex(&gToolkitTLSIndex, NULL);
    if (PR_FAILURE == status) {
      rv = NS_ERROR_FAILURE;
    }
  }

  if (NS_SUCCEEDED(rv)) {
    toolkit = (nsIToolkit*)PR_GetThreadPrivate(gToolkitTLSIndex);

    
    
    
    if (!toolkit) {
      toolkit = new nsToolkit();

      if (!toolkit) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      } else {
        NS_ADDREF(toolkit);
        toolkit->Init(PR_GetCurrentThread());
        
        
        
        
        PR_SetThreadPrivate(gToolkitTLSIndex, (void*)toolkit);
      }
    } else {
      NS_ADDREF(toolkit);
    }
    *aResult = toolkit;
  }

  return rv;
}



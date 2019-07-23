




































#include "nscore.h" 
#include "xlibrgb.h"
#include "nsToolkit.h"
#include "nsGCCache.h"
#include "nsAppShell.h" 
#include "nsWidgetAtoms.h"



static PRUintn gToolkitTLSIndex = 0;

nsToolkit::nsToolkit()
{
  mGC = nsnull;
  mDisplay = xxlib_rgb_get_display(nsAppShell::GetXlibRgbHandle());
}

nsToolkit::~nsToolkit()
{
  if (mGC)
    mGC->Release();

  
  PR_SetThreadPrivate(gToolkitTLSIndex, nsnull);
}

NS_IMPL_ISUPPORTS1(nsToolkit, nsIToolkit)

void nsToolkit::CreateSharedGC()
{
  if (mGC)
    return;

  mGC = new xGC(mDisplay, XDefaultRootWindow(mDisplay), 0, nsnull);
  mGC->AddRef(); 
}

xGC *nsToolkit::GetSharedGC()
{
  if (!mGC)
    CreateSharedGC();

  mGC->AddRef();
  return mGC;
}

NS_METHOD nsToolkit::Init(PRThread *aThread)
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

  
  if (gToolkitTLSIndex == 0)
  {
    status = PR_NewThreadPrivateIndex(&gToolkitTLSIndex, nsnull);
    if (PR_FAILURE == status)
    {
      rv = NS_ERROR_FAILURE;
    }
  }

  if (NS_SUCCEEDED(rv))
  {
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

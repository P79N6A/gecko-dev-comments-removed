






































#include "nsToolkit.h"
#include "nsWidgetAtoms.h"

NS_IMPL_ISUPPORTS1(nsToolkit, nsIToolkit)





static PRUintn gToolkitTLSIndex = 0;






nsToolkit::nsToolkit()  
{
}







nsToolkit::~nsToolkit()
{
    
    PR_SetThreadPrivate(gToolkitTLSIndex, nsnull);
}





NS_METHOD nsToolkit::Init(PRThread *aThread)
{
    NS_ASSERTION(aThread, "Can only initialize toolkit on the current thread");

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







































#include "nsToolkit.h"
#include "nsGUIEvent.h"
#include "nsGkAtoms.h"

NS_IMPL_ISUPPORTS1(nsToolkit, nsIToolkit)


static PRUintn gToolkitTLSIndex = 0;

nsToolkit::nsToolkit()
{
    MOZ_COUNT_CTOR(nsToolkit);
}

nsToolkit::~nsToolkit()
{
    MOZ_COUNT_DTOR(nsToolkit);
    PR_SetThreadPrivate(gToolkitTLSIndex, nsnull);
}

NS_IMETHODIMP
nsToolkit::Init(PRThread *aThread)
{
    return NS_OK;
}

NS_METHOD
NS_GetCurrentToolkit(nsIToolkit* *aResult)
{
    nsCOMPtr<nsIToolkit> toolkit = nsnull;
    nsresult rv = NS_OK;
    PRStatus status;

    if (gToolkitTLSIndex == 0) {
        status = PR_NewThreadPrivateIndex(&gToolkitTLSIndex, NULL);
        if (PR_FAILURE == status)
            rv = NS_ERROR_FAILURE;
    }

    if (NS_FAILED(rv))
        return rv;

    toolkit = (nsIToolkit*) PR_GetThreadPrivate(gToolkitTLSIndex);
    if (!toolkit) {
        toolkit = new nsToolkit();

        if (toolkit) {
            toolkit->Init(PR_GetCurrentThread());

            PR_SetThreadPrivate(gToolkitTLSIndex, (void*)toolkit.get());
        } else {
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
    }

    NS_IF_ADDREF(*aResult = toolkit);

    return rv;
}

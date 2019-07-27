



#include "xpctest_private.h"
#include "xpctest_interfaces.h"
#include "nsComponentManagerUtils.h"

NS_IMPL_ISUPPORTS(nsXPCTestReturnCodeParent, nsIXPCTestReturnCodeParent)

nsXPCTestReturnCodeParent::nsXPCTestReturnCodeParent()
{
}

nsXPCTestReturnCodeParent::~nsXPCTestReturnCodeParent()
{
}


NS_IMETHODIMP nsXPCTestReturnCodeParent::CallChild(int32_t childBehavior, nsresult *_retval)
{
    nsresult rv;
    nsCOMPtr<nsIXPCTestReturnCodeChild> child(do_CreateInstance("@mozilla.org/js/xpc/test/js/ReturnCodeChild;1", &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = child->DoIt(childBehavior);
    *_retval = rv;
    return NS_OK;
}





#include "nsDocShellTransferableHooks.h"
#include "nsIClipboardDragDropHooks.h"
#include "nsIClipboardDragDropHookList.h"
#include "nsArrayEnumerator.h"

nsTransferableHookData::nsTransferableHookData()
{
}


nsTransferableHookData::~nsTransferableHookData()
{
}





NS_IMPL_ISUPPORTS1(nsTransferableHookData, nsIClipboardDragDropHookList)

NS_IMETHODIMP
nsTransferableHookData::AddClipboardDragDropHooks(
                                        nsIClipboardDragDropHooks *aOverrides)
{
    NS_ENSURE_ARG(aOverrides);

    
    if (mHookList.IndexOfObject(aOverrides) == -1)
    {
        if (!mHookList.AppendObject(aOverrides))
            return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsTransferableHookData::RemoveClipboardDragDropHooks(
                                         nsIClipboardDragDropHooks *aOverrides)
{
    NS_ENSURE_ARG(aOverrides);
    if (!mHookList.RemoveObject(aOverrides))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsTransferableHookData::GetHookEnumerator(nsISimpleEnumerator **aResult)
{
    return NS_NewArrayEnumerator(aResult, mHookList);
}

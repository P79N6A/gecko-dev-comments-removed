




































#ifndef nsDocShellTransferableHooks_h__
#define nsDocShellTransferableHooks_h__

#ifndef nsCOMPtr_h___
#include "nsCOMPtr.h"
#endif

#include "nsIClipboardDragDropHooks.h"
#include "nsIClipboardDragDropHookList.h"
#include "nsCOMArray.h"


class nsTransferableHookData : public nsIClipboardDragDropHookList
{
public:
    nsTransferableHookData();
    virtual ~nsTransferableHookData();
    NS_DECL_ISUPPORTS
    NS_DECL_NSICLIPBOARDDRAGDROPHOOKLIST

protected:
    nsCOMArray<nsIClipboardDragDropHooks> mHookList;
};

#endif 

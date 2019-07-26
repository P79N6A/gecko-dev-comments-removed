



#ifndef nsDocShellTransferableHooks_h__
#define nsDocShellTransferableHooks_h__

#include "nsIClipboardDragDropHookList.h"
#include "nsCOMArray.h"

class nsIClipboardDragDropHooks;

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

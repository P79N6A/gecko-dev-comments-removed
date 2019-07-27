





#ifndef nsDocShellTransferableHooks_h__
#define nsDocShellTransferableHooks_h__

#include "nsIClipboardDragDropHookList.h"
#include "nsCOMArray.h"

class nsIClipboardDragDropHooks;

class nsTransferableHookData : public nsIClipboardDragDropHookList
{
public:
  nsTransferableHookData();
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLIPBOARDDRAGDROPHOOKLIST

protected:
  virtual ~nsTransferableHookData();

  nsCOMArray<nsIClipboardDragDropHooks> mHookList;
};

#endif 

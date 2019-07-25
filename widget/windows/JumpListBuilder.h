




#ifndef __JumpListBuilder_h__
#define __JumpListBuilder_h__

#include <windows.h>

#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN7

#include <shobjidl.h>

#include "nsString.h"
#include "nsIMutableArray.h"

#include "nsIJumpListBuilder.h"
#include "nsIJumpListItem.h"
#include "JumpListItem.h"
#include "nsIObserver.h"
#include "nsIFaviconService.h"
#include "nsThreadUtils.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace widget {

class JumpListBuilder : public nsIJumpListBuilder, 
                        public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIJUMPLISTBUILDER
  NS_DECL_NSIOBSERVER

  JumpListBuilder();
  virtual ~JumpListBuilder();

protected:
  static bool sBuildingList; 

private:
  nsRefPtr<ICustomDestinationList> mJumpListMgr;
  uint32_t mMaxItems;
  bool mHasCommit;
  nsCOMPtr<nsIThread> mIOThread;

  bool IsSeparator(nsCOMPtr<nsIJumpListItem>& item);
  nsresult TransferIObjectArrayToIMutableArray(IObjectArray *objArray, nsIMutableArray *removedItems);
  nsresult RemoveIconCacheForItems(nsIMutableArray *removedItems);
  nsresult RemoveIconCacheForAllItems();

  friend class WinTaskbar;
};

} 
} 

#endif 


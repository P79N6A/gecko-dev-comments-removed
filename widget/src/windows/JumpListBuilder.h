





































#ifndef __JumpListBuilder_h__
#define __JumpListBuilder_h__

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7

#include <windows.h>

#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN7

#include <shobjidl.h>

#include "nsString.h"
#include "nsIMutableArray.h"

#include "nsIJumpListBuilder.h"
#include "nsIJumpListItem.h"
#include "JumpListItem.h"

namespace mozilla {
namespace widget {

class JumpListBuilder : public nsIJumpListBuilder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIJUMPLISTBUILDER

  JumpListBuilder();
  virtual ~JumpListBuilder();

protected:
  static PRPackedBool sBuildingList; 

private:
  nsRefPtr<ICustomDestinationList> mJumpListMgr;
  PRUint32 mMaxItems;
  PRBool mHasCommit;

  PRBool IsSeparator(nsCOMPtr<nsIJumpListItem>& item);
  nsresult TransferIObjectArrayToIMutableArray(IObjectArray *objArray, nsIMutableArray *removedItems);

  friend class WinTaskbar;
};

} 
} 

#endif 

#endif 


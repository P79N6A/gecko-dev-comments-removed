






































#ifndef __nsDownloadHistory_h__
#define __nsDownloadHistory_h__

#include "nsIDownloadHistory.h"

#define NS_DOWNLOADHISTORY_CID \
  {0x2ee83680, 0x2af0, 0x4bcb, {0xbf, 0xa0, 0xc9, 0x70, 0x5f, 0x65, 0x54, 0xf1}}

class nsDownloadHistory : public nsIDownloadHistory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOWNLOADHISTORY

  NS_DEFINE_STATIC_CID_ACCESSOR(NS_DOWNLOADHISTORY_CID)
};

#endif

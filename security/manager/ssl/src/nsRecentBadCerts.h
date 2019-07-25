






































#ifndef __RECENTBADCERTS_H__
#define __RECENTBADCERTS_H__

#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"

#include "nsIRecentBadCertsService.h"
#include "nsTHashtable.h"
#include "nsString.h"
#include "secitem.h"

class RecentBadCert
{
public:

  RecentBadCert()
  {
    mDERCert.len = 0;
    mDERCert.data = nsnull;
    isDomainMismatch = false;
    isNotValidAtThisTime = false;
    isUntrusted = false;
  }

  ~RecentBadCert()
  {
    Clear();
  }

  void Clear()
  {
    mHostWithPort.Truncate();
    if (mDERCert.len)
      nsMemory::Free(mDERCert.data);
    mDERCert.len = 0;
    mDERCert.data = nsnull;
  }

  nsString mHostWithPort;
  SECItem mDERCert;
  bool isDomainMismatch;
  bool isNotValidAtThisTime;
  bool isUntrusted;

private:
  RecentBadCert(const RecentBadCert &other) MOZ_DELETE;
  RecentBadCert &operator=(const RecentBadCert &other) MOZ_DELETE;
};

class nsRecentBadCertsService : public nsIRecentBadCertsService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRECENTBADCERTSSERVICE

  nsRecentBadCertsService();
  ~nsRecentBadCertsService();

  nsresult Init();

protected:
    mozilla::ReentrantMonitor monitor;

    enum {const_recently_seen_list_size = 5};
    RecentBadCert mCerts[const_recently_seen_list_size];

    
    PRUint32 mNextStorePosition;
};

#define NS_RECENTBADCERTS_CID { /* e7caf8c0-3570-47fe-aa1b-da47539b5d07 */ \
    0xe7caf8c0,                                                        \
    0x3570,                                                            \
    0x47fe,                                                            \
    {0xaa, 0x1b, 0xda, 0x47, 0x53, 0x9b, 0x5d, 0x07}                   \
  }

#endif

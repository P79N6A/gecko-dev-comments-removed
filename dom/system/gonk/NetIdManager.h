



#ifndef NetIdManager_h
#define NetIdManager_h

#include "nsString.h"
#include "nsDataHashtable.h"







class NetIdManager {
public:
  
  enum {
    MIN_NET_ID = 100,
    MAX_NET_ID = 65535,
  };

  
  
  struct NetIdInfo {
    int mNetId;
    int mRefCnt;
  };

public:
  NetIdManager();

  bool lookup(const nsString& aInterfaceName, NetIdInfo* aNetIdInfo);
  void acquire(const nsString& aInterfaceName, NetIdInfo* aNetIdInfo);
  bool release(const nsString& aInterfaceName, NetIdInfo* aNetIdInfo);

private:
  int getNextNetId();
  int mNextNetId;
  nsDataHashtable<nsStringHashKey, NetIdInfo> mInterfaceToNetIdHash;
};

#endif
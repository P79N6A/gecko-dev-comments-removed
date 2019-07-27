



#include "NetIdManager.h"

NetIdManager::NetIdManager()
  : mNextNetId(MIN_NET_ID)
{
}

int NetIdManager::getNextNetId()
{
  
  
  

  int netId = mNextNetId;
  if (++mNextNetId > MAX_NET_ID) {
    mNextNetId = MIN_NET_ID;
  }

  return netId;
}

void NetIdManager::acquire(const nsString& aInterfaceName,
                           NetIdInfo* aNetIdInfo)
{
  
  if (!mInterfaceToNetIdHash.Get(aInterfaceName, aNetIdInfo)) {
    aNetIdInfo->mNetId = getNextNetId();
    aNetIdInfo->mRefCnt = 1;
  } else {
    aNetIdInfo->mRefCnt++;
  }

  
  mInterfaceToNetIdHash.Put(aInterfaceName, *aNetIdInfo);

  return;
}

bool NetIdManager::lookup(const nsString& aInterfaceName,
                          NetIdInfo* aNetIdInfo)
{
  return mInterfaceToNetIdHash.Get(aInterfaceName, aNetIdInfo);
}

bool NetIdManager::release(const nsString& aInterfaceName,
                           NetIdInfo* aNetIdInfo)
{
  if (!mInterfaceToNetIdHash.Get(aInterfaceName, aNetIdInfo)) {
    return false; 
  }

  aNetIdInfo->mRefCnt--;

  
  if (aNetIdInfo->mRefCnt > 0) {
    mInterfaceToNetIdHash.Put(aInterfaceName, *aNetIdInfo);
    return true;
  }

  
  mInterfaceToNetIdHash.Remove(aInterfaceName);

  return true;
}
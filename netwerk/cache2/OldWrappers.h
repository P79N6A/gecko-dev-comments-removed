

#ifndef OLDWRAPPERS__H__
#define OLDWRAPPERS__H__

#include "nsICacheEntry.h"
#include "nsICacheListener.h"
#include "nsICacheStorage.h"

#include "nsCOMPtr.h"
#include "nsICacheEntryOpenCallback.h"
#include "nsICacheEntryDescriptor.h"
#include "nsThreadUtils.h"
#include "mozilla/TimeStamp.h"

class nsIURI;
class nsICacheEntryOpenCallback;
class nsIApplicationCache;
class nsILoadContextInfo;

namespace mozilla { namespace net {

class CacheStorage;

class _OldCacheEntryWrapper : public nsICacheEntry
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_FORWARD_SAFE_NSICACHEENTRYDESCRIPTOR(mOldDesc)
  NS_FORWARD_NSICACHEENTRYINFO(mOldInfo->)

  NS_IMETHOD AsyncDoom(nsICacheEntryDoomCallback* listener);
  NS_IMETHOD GetPersistToDisk(bool *aPersistToDisk);
  NS_IMETHOD SetPersistToDisk(bool aPersistToDisk);
  NS_IMETHOD SetValid() { return NS_OK; }
  NS_IMETHOD MetaDataReady() { return NS_OK; }
  NS_IMETHOD Recreate(nsICacheEntry**);
  NS_IMETHOD GetDataSize(int64_t *size);
  NS_IMETHOD OpenInputStream(int64_t offset, nsIInputStream * *_retval);
  NS_IMETHOD OpenOutputStream(int64_t offset, nsIOutputStream * *_retval);
  NS_IMETHOD MaybeMarkValid();
  NS_IMETHOD HasWriteAccess(bool aWriteOnly, bool *aWriteAccess);

  _OldCacheEntryWrapper(nsICacheEntryDescriptor* desc);
  _OldCacheEntryWrapper(nsICacheEntryInfo* info);

  virtual ~_OldCacheEntryWrapper();

private:
  _OldCacheEntryWrapper() MOZ_DELETE;
  nsICacheEntryDescriptor* mOldDesc; 
  nsCOMPtr<nsICacheEntryInfo> mOldInfo;
};


class _OldCacheLoad : public nsRunnable
                    , public nsICacheListener
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSICACHELISTENER

  _OldCacheLoad(nsCSubstring const& aCacheKey,
                nsICacheEntryOpenCallback* aCallback,
                nsIApplicationCache* aAppCache,
                nsILoadContextInfo* aLoadInfo,
                bool aWriteToDisk,
                uint32_t aFlags);
  virtual ~_OldCacheLoad();

  nsresult Start();

private:
  void Check();

  nsCOMPtr<nsIEventTarget> mCacheThread;

  nsCString mCacheKey;
  nsCOMPtr<nsICacheEntryOpenCallback> mCallback;
  nsCOMPtr<nsILoadContextInfo> mLoadInfo;
  uint32_t mFlags;

  bool const mWriteToDisk : 1;
  bool mMainThreadOnly : 1;
  bool mNew : 1;

  nsCOMPtr<nsICacheEntry> mCacheEntry;
  nsresult mStatus;
  uint32_t mRunCount;
  nsCOMPtr<nsIApplicationCache> mAppCache;

  mozilla::TimeStamp mLoadStart;
};


class _OldStorage : public nsICacheStorage
{
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSICACHESTORAGE

public:
  _OldStorage(nsILoadContextInfo* aInfo,
              bool aAllowDisk,
              bool aLookupAppCache,
              bool aOfflineStorage,
              nsIApplicationCache* aAppCache);

private:
  virtual ~_OldStorage();
  nsresult AssembleCacheKey(nsIURI *aURI, nsACString const & aIdExtension, nsACString & _result);
  nsresult ChooseApplicationCache(nsCSubstring const &cacheKey, nsIApplicationCache** aCache);

  nsCOMPtr<nsILoadContextInfo> mLoadInfo;
  nsCOMPtr<nsIApplicationCache> mAppCache;
  bool const mWriteToDisk : 1;
  bool const mLookupAppCache : 1;
  bool const mOfflineStorage : 1;
};

}}

#endif

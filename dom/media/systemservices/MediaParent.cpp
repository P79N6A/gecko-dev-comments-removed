





#include "MediaParent.h"

#include "mozilla/Base64.h"
#include <mozilla/StaticMutex.h>

#include "MediaUtils.h"
#include "MediaEngine.h"
#include "VideoUtils.h"
#include "nsThreadUtils.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsIOutputStream.h"
#include "nsILineInputStream.h"
#include "nsISafeOutputStream.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsISupportsImpl.h"
#include "mozilla/Logging.h"

#undef LOG
PRLogModuleInfo *gMediaParentLog;
#define LOG(args) MOZ_LOG(gMediaParentLog, mozilla::LogLevel::Debug, args)




#define ORIGINKEYS_FILE "enumerate_devices.txt"
#define ORIGINKEYS_VERSION "1"

namespace mozilla {
namespace media {

static Parent<PMediaParent>* sIPCServingParent;

static OriginKeyStore* sOriginKeyStore = nullptr;

class OriginKeyStore : public nsISupports
{
  NS_DECL_THREADSAFE_ISUPPORTS
  class OriginKey
  {
  public:
    static const size_t DecodedLength = 18;
    static const size_t EncodedLength = DecodedLength * 4 / 3;

    OriginKey(const nsACString& aKey, int64_t aSecondsStamp)
    : mKey(aKey)
    , mSecondsStamp(aSecondsStamp) {}

    nsCString mKey; 
    int64_t mSecondsStamp;
  };

  class OriginKeysTable
  {
  public:
    OriginKeysTable() {}

    nsresult
    GetOriginKey(const nsACString& aOrigin, nsCString& result)
    {
      OriginKey* key;
      if (!mKeys.Get(aOrigin, &key)) {
        nsCString salt; 
        nsresult rv = GenerateRandomName(salt, key->EncodedLength);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }
        key = new OriginKey(salt, PR_Now() / PR_USEC_PER_SEC);
        mKeys.Put(aOrigin, key);
      }
      result = key->mKey;
      return NS_OK;
    }

    static PLDHashOperator
    HashCleaner(const nsACString& aOrigin, nsAutoPtr<OriginKey>& aOriginKey,
                void *aUserArg)
    {
      OriginKey* since = static_cast<OriginKey*>(aUserArg);

      LOG((((aOriginKey->mSecondsStamp >= since->mSecondsStamp)?
            "%s: REMOVE %lld >= %lld" :
            "%s: KEEP   %lld < %lld"),
            __FUNCTION__, aOriginKey->mSecondsStamp, since->mSecondsStamp));

      return (aOriginKey->mSecondsStamp >= since->mSecondsStamp)?
          PL_DHASH_REMOVE : PL_DHASH_NEXT;
    }

    void Clear(int64_t aSinceWhen)
    {
      
      OriginKey since(nsCString(), aSinceWhen  / PR_USEC_PER_SEC);
      mKeys.Enumerate(HashCleaner, &since);
    }

  protected:
    nsClassHashtable<nsCStringHashKey, OriginKey> mKeys;
  };

  class OriginKeysLoader : public OriginKeysTable
  {
  public:
    OriginKeysLoader() {}

    nsresult
    GetOriginKey(const nsACString& aOrigin, nsCString& result)
    {
      auto before = mKeys.Count();
      OriginKeysTable::GetOriginKey(aOrigin, result);
      if (mKeys.Count() != before) {
        Save();
      }
      return NS_OK;
    }

    already_AddRefed<nsIFile>
    GetFile()
    {
      MOZ_ASSERT(mProfileDir);
      nsCOMPtr<nsIFile> file;
      nsresult rv = mProfileDir->Clone(getter_AddRefs(file));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return nullptr;
      }
      file->Append(NS_LITERAL_STRING(ORIGINKEYS_FILE));
      return file.forget();
    }

    
    
    
    
    
    

    nsresult Read()
    {
      nsCOMPtr<nsIFile> file = GetFile();
      if (NS_WARN_IF(!file)) {
        return NS_ERROR_UNEXPECTED;
      }
      bool exists;
      nsresult rv = file->Exists(&exists);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
      if (!exists) {
        return NS_OK;
      }

      nsCOMPtr<nsIInputStream> stream;
      rv = NS_NewLocalFileInputStream(getter_AddRefs(stream), file);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
      nsCOMPtr<nsILineInputStream> i = do_QueryInterface(stream);
      MOZ_ASSERT(i);

      nsCString line;
      bool hasMoreLines;
      rv = i->ReadLine(line, &hasMoreLines);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
      if (!line.EqualsLiteral(ORIGINKEYS_VERSION)) {
        
        return NS_OK;
      }

      while (hasMoreLines) {
        rv = i->ReadLine(line, &hasMoreLines);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }
        
        
        int32_t f = line.FindChar(' ');
        if (f < 0) {
          continue;
        }
        const nsACString& key = Substring(line, 0, f);
        const nsACString& s = Substring(line, f+1);
        f = s.FindChar(' ');
        if (f < 0) {
          continue;
        }
        int64_t secondsstamp = nsCString(Substring(s, 0, f)).ToInteger64(&rv);
        if (NS_FAILED(rv)) {
          continue;
        }
        const nsACString& origin = Substring(s, f+1);

        
        if (key.Length() != OriginKey::EncodedLength) {
          continue;
        }
        nsCString dummy;
        rv = Base64Decode(key, dummy);
        if (NS_FAILED(rv)) {
          continue;
        }
        mKeys.Put(origin, new OriginKey(key, secondsstamp));
      }
      return NS_OK;
    }

    static PLDHashOperator
    HashWriter(const nsACString& aOrigin, OriginKey* aOriginKey, void *aUserArg)
    {
      auto* stream = static_cast<nsIOutputStream *>(aUserArg);

      nsCString buffer;
      buffer.Append(aOriginKey->mKey);
      buffer.Append(' ');
      buffer.AppendInt(aOriginKey->mSecondsStamp);
      buffer.Append(' ');
      buffer.Append(aOrigin);
      buffer.Append('\n');

      uint32_t count;
      nsresult rv = stream->Write(buffer.Data(), buffer.Length(), &count);
      if (NS_WARN_IF(NS_FAILED(rv)) || count != buffer.Length()) {
        return PL_DHASH_STOP;
      }
      return PL_DHASH_NEXT;
    }

    nsresult
    Write()
    {
      nsCOMPtr<nsIFile> file = GetFile();
      if (NS_WARN_IF(!file)) {
        return NS_ERROR_UNEXPECTED;
      }

      nsCOMPtr<nsIOutputStream> stream;
      nsresult rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(stream), file);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
      nsAutoCString buffer;
      buffer.AppendLiteral(ORIGINKEYS_VERSION);
      buffer.Append('\n');

      uint32_t count;
      rv = stream->Write(buffer.Data(), buffer.Length(), &count);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
      if (count != buffer.Length()) {
        return NS_ERROR_UNEXPECTED;
      }
      mKeys.EnumerateRead(HashWriter, stream.get());

      nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(stream);
      MOZ_ASSERT(safeStream);

      rv = safeStream->Finish();
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
      return NS_OK;
    }

    nsresult Load()
    {
      nsresult rv = Read();
      if (NS_WARN_IF(NS_FAILED(rv))) {
        Delete();
      }
      return rv;
    }

    nsresult Save()
    {
      nsresult rv = Write();
      if (NS_WARN_IF(NS_FAILED(rv))) {
        NS_WARNING("Failed to write data for EnumerateDevices id-persistence.");
        Delete();
      }
      return rv;
    }

    void Clear(int64_t aSinceWhen)
    {
      OriginKeysTable::Clear(aSinceWhen);
      Delete();
      Save();
    }

    nsresult Delete()
    {
      nsCOMPtr<nsIFile> file = GetFile();
      if (NS_WARN_IF(!file)) {
        return NS_ERROR_UNEXPECTED;
      }
      nsresult rv = file->Remove(false);
      if (rv == NS_ERROR_FILE_NOT_FOUND) {
        return NS_OK;
      }
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
      return NS_OK;
    }

    void
    SetProfileDir(nsIFile* aProfileDir)
    {
      MOZ_ASSERT(!NS_IsMainThread());
      bool first = !mProfileDir;
      mProfileDir = aProfileDir;
      
      if (first) {
        Load();
      }
    }
  private:
    nsCOMPtr<nsIFile> mProfileDir;
  };

private:
  virtual ~OriginKeyStore()
  {
    sOriginKeyStore = nullptr;
    LOG((__FUNCTION__));
  }

public:
  static OriginKeyStore* Get()
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (!sOriginKeyStore) {
      sOriginKeyStore = new OriginKeyStore();
    }
    return sOriginKeyStore;
  }

  
  OriginKeysLoader mOriginKeys;
  OriginKeysTable mPrivateBrowsingOriginKeys;
};

NS_IMPL_ISUPPORTS0(OriginKeyStore)

template<> 
Parent<PMediaParent>* Parent<PMediaParent>::GetSingleton()
{
  return sIPCServingParent;
}

template<> 
Parent<NonE10s>* Parent<NonE10s>::GetSingleton()
{
  nsRefPtr<MediaManager> mgr = MediaManager::GetInstance();
  if (!mgr) {
    return nullptr;
  }
  return mgr->GetNonE10sParent();
}




template<class Super> static
Parent<Super>* GccGetSingleton() { return Parent<Super>::GetSingleton(); };


template<class Super> bool
Parent<Super>::RecvGetOriginKey(const uint32_t& aRequestId,
                         const nsCString& aOrigin,
                         const bool& aPrivateBrowsing)
{
  MOZ_ASSERT(NS_IsMainThread());

  

  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIFile> profileDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(profileDir));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  
  

  nsRefPtr<Pledge<nsCString>> p = new Pledge<nsCString>();
  uint32_t id = mOutstandingPledges.Append(*p);

  nsCOMPtr<nsIEventTarget> sts = do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID);
  MOZ_ASSERT(sts);
  nsRefPtr<OriginKeyStore> store(mOriginKeyStore);
  bool sameProcess = mSameProcess;

  rv = sts->Dispatch(NewRunnableFrom([id, profileDir, store, sameProcess,
                                      aOrigin, aPrivateBrowsing]() -> nsresult {
    MOZ_ASSERT(!NS_IsMainThread());
    store->mOriginKeys.SetProfileDir(profileDir);
    nsCString result;
    if (aPrivateBrowsing) {
      store->mPrivateBrowsingOriginKeys.GetOriginKey(aOrigin, result);
    } else {
      store->mOriginKeys.GetOriginKey(aOrigin, result);
    }

    
    nsresult rv;
    rv = NS_DispatchToMainThread(NewRunnableFrom([id, store, sameProcess,
                                                  result]() -> nsresult {
      Parent* parent = GccGetSingleton<Super>(); 
      if (!parent) {
        return NS_OK;
      }
      nsRefPtr<Pledge<nsCString>> p = parent->mOutstandingPledges.Remove(id);
      if (!p) {
        return NS_ERROR_UNEXPECTED;
      }
      p->Resolve(result);
      return NS_OK;
    }), NS_DISPATCH_NORMAL);

    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    return NS_OK;
  }), NS_DISPATCH_NORMAL);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  p->Then([aRequestId, sameProcess](const nsCString& aKey) mutable {
    if (!sameProcess) {
      if (!sIPCServingParent) {
        return NS_OK;
      }
      unused << sIPCServingParent->SendGetOriginKeyResponse(aRequestId, aKey);
    } else {
      nsRefPtr<MediaManager> mgr = MediaManager::GetInstance();
      if (!mgr) {
        return NS_OK;
      }
      nsRefPtr<Pledge<nsCString>> pledge =
          mgr->mGetOriginKeyPledges.Remove(aRequestId);
      if (pledge) {
        pledge->Resolve(aKey);
      }
    }
    return NS_OK;
  });
  return true;
}

template<class Super> bool
Parent<Super>::RecvSanitizeOriginKeys(const uint64_t& aSinceWhen)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIFile> profileDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                         getter_AddRefs(profileDir));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }
  

  nsCOMPtr<nsIEventTarget> sts = do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID);
  MOZ_ASSERT(sts);
  nsRefPtr<OriginKeyStore> store(mOriginKeyStore);

  rv = sts->Dispatch(NewRunnableFrom([profileDir, store, aSinceWhen]() -> nsresult {
    MOZ_ASSERT(!NS_IsMainThread());
    store->mOriginKeys.SetProfileDir(profileDir);
    store->mPrivateBrowsingOriginKeys.Clear(aSinceWhen);
    store->mOriginKeys.Clear(aSinceWhen);
    return NS_OK;
  }), NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }
  return true;
}

template<class Super> void
Parent<Super>::ActorDestroy(ActorDestroyReason aWhy)
{
  
  mDestroyed = true;
  LOG((__FUNCTION__));
}

template<class Super>
Parent<Super>::Parent(bool aSameProcess)
  : mOriginKeyStore(OriginKeyStore::Get())
  , mDestroyed(false)
  , mSameProcess(aSameProcess)
{
  if (!gMediaParentLog)
    gMediaParentLog = PR_NewLogModule("MediaParent");
  LOG(("media::Parent: %p", this));
}

template<class Super>
Parent<Super>::~Parent()
{
  LOG(("~media::Parent: %p", this));
}

PMediaParent*
AllocPMediaParent()
{
  MOZ_ASSERT(!sIPCServingParent);
  sIPCServingParent = new Parent<PMediaParent>();
  return sIPCServingParent;
}

bool
DeallocPMediaParent(media::PMediaParent *aActor)
{
  MOZ_ASSERT(sIPCServingParent == static_cast<Parent<PMediaParent>*>(aActor));
  delete sIPCServingParent;
  return true;
}

}
}


template class mozilla::media::Parent<mozilla::media::NonE10s>;

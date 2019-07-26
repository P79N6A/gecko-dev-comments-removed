



#ifndef CacheFileIOManager__h__
#define CacheFileIOManager__h__

#include "CacheIOThread.h"
#include "CacheEntriesEnumerator.h"
#include "nsIEventTarget.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "mozilla/SHA1.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsTHashtable.h"
#include "prio.h"



class nsIFile;

namespace mozilla {
namespace net {

#ifdef DEBUG_HANDLES
class CacheFileHandlesEntry;
#endif

const char kEntriesDir[] = "entries";
const char kDoomedDir[]  = "doomed";


class CacheFileHandle : public nsISupports
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  bool DispatchRelease();

  CacheFileHandle(const SHA1Sum::Hash *aHash, bool aPriority);
  CacheFileHandle(const nsACString &aKey, bool aPriority);
  CacheFileHandle(const CacheFileHandle &aOther);
  void Log();
  bool IsDoomed() { return mIsDoomed; }
  const SHA1Sum::Hash *Hash() { return mHash; }
  int64_t FileSize() { return mFileSize; }
  uint32_t FileSizeInK();
  bool IsPriority() { return mPriority; }
  bool FileExists() { return mFileExists; }
  bool IsClosed() { return mClosed; }
  bool IsSpecialFile() { return !mHash; }
  nsCString & Key() { return mKey; }

private:
  friend class CacheFileIOManager;
  friend class CacheFileHandles;
  friend class ReleaseNSPRHandleEvent;

  virtual ~CacheFileHandle();

  const SHA1Sum::Hash *mHash;
  bool                 mIsDoomed;
  bool                 mPriority;
  bool                 mClosed;
  bool                 mInvalid;
  bool                 mFileExists; 
                                    
                                    
                                    
  nsCOMPtr<nsIFile>    mFile;
  int64_t              mFileSize;
  PRFileDesc          *mFD;  
  nsCString            mKey;
};

class CacheFileHandles {
public:
  CacheFileHandles();
  ~CacheFileHandles();

  nsresult GetHandle(const SHA1Sum::Hash *aHash, bool aReturnDoomed, CacheFileHandle **_retval);
  nsresult NewHandle(const SHA1Sum::Hash *aHash, bool aPriority, CacheFileHandle **_retval);
  void     RemoveHandle(CacheFileHandle *aHandlle);
  void     GetAllHandles(nsTArray<nsRefPtr<CacheFileHandle> > *_retval);
  void     ClearAll();
  uint32_t HandleCount();

#ifdef DEBUG_HANDLES
  void     Log(CacheFileHandlesEntry *entry);
#endif

  class HandleHashKey : public PLDHashEntryHdr
  {
  public:
    typedef const SHA1Sum::Hash& KeyType;
    typedef const SHA1Sum::Hash* KeyTypePointer;

    HandleHashKey(KeyTypePointer aKey)
    {
      MOZ_COUNT_CTOR(HandleHashKey);
      mHash = (SHA1Sum::Hash*)new uint8_t[SHA1Sum::HashSize];
      memcpy(mHash, aKey, sizeof(SHA1Sum::Hash));
    }
    HandleHashKey(const HandleHashKey& aOther)
    {
      NS_NOTREACHED("HandleHashKey copy constructor is forbidden!");
    }
    ~HandleHashKey()
    {
      MOZ_COUNT_DTOR(HandleHashKey);
    }

    bool KeyEquals(KeyTypePointer aKey) const
    {
      return memcmp(mHash, aKey, sizeof(SHA1Sum::Hash)) == 0;
    }
    static KeyTypePointer KeyToPointer(KeyType aKey)
    {
      return &aKey;
    }
    static PLDHashNumber HashKey(KeyTypePointer aKey)
    {
      return (reinterpret_cast<const uint32_t *>(aKey))[0];
    }

    void AddHandle(CacheFileHandle* aHandle);
    void RemoveHandle(CacheFileHandle* aHandle);
    already_AddRefed<CacheFileHandle> GetNewestHandle();
    void GetHandles(nsTArray<nsRefPtr<CacheFileHandle> > &aResult);

    SHA1Sum::Hash *Hash() { return mHash; }
    bool IsEmpty() { return mHandles.Length() == 0; }

    enum { ALLOW_MEMMOVE = true };

#ifdef DEBUG
    void AssertHandlesState();
#endif

  private:
    nsAutoArrayPtr<SHA1Sum::Hash> mHash;
    
    
    
    
    nsTArray<CacheFileHandle*> mHandles;
  };

private:
  nsTHashtable<HandleHashKey> mTable;
};



class OpenFileEvent;
class CloseFileEvent;
class ReadEvent;
class WriteEvent;
class MetadataWriteScheduleEvent;

#define CACHEFILEIOLISTENER_IID \
{ /* dcaf2ddc-17cf-4242-bca1-8c86936375a5 */       \
  0xdcaf2ddc,                                      \
  0x17cf,                                          \
  0x4242,                                          \
  {0xbc, 0xa1, 0x8c, 0x86, 0x93, 0x63, 0x75, 0xa5} \
}

class CacheFileIOListener : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(CACHEFILEIOLISTENER_IID)

  NS_IMETHOD OnFileOpened(CacheFileHandle *aHandle, nsresult aResult) = 0;
  NS_IMETHOD OnDataWritten(CacheFileHandle *aHandle, const char *aBuf,
                           nsresult aResult) = 0;
  NS_IMETHOD OnDataRead(CacheFileHandle *aHandle, char *aBuf,
                        nsresult aResult) = 0;
  NS_IMETHOD OnFileDoomed(CacheFileHandle *aHandle, nsresult aResult) = 0;
  NS_IMETHOD OnEOFSet(CacheFileHandle *aHandle, nsresult aResult) = 0;
  NS_IMETHOD OnFileRenamed(CacheFileHandle *aHandle, nsresult aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(CacheFileIOListener, CACHEFILEIOLISTENER_IID)


class CacheFileIOManager : public nsITimerCallback
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  enum {
    OPEN         = 0U,
    CREATE       = 1U,
    CREATE_NEW   = 2U,
    PRIORITY     = 4U,
    NOHASH       = 8U,
    SPECIAL_FILE = 16U
  };

  CacheFileIOManager();

  static nsresult Init();
  static nsresult Shutdown();
  static nsresult OnProfile();
  static already_AddRefed<nsIEventTarget> IOTarget();
  static already_AddRefed<CacheIOThread> IOThread();
  static bool IsOnIOThread();
  static bool IsOnIOThreadOrCeased();
  static bool IsShutdown();

  
  
  static nsresult ScheduleMetadataWrite(CacheFile * aFile);
  
  
  static nsresult UnscheduleMetadataWrite(CacheFile * aFile);
  
  static nsresult ShutdownMetadataWriteScheduling();

  static nsresult OpenFile(const nsACString &aKey,
                           uint32_t aFlags,
                           CacheFileIOListener *aCallback);
  static nsresult Read(CacheFileHandle *aHandle, int64_t aOffset,
                       char *aBuf, int32_t aCount,
                       CacheFileIOListener *aCallback);
  static nsresult Write(CacheFileHandle *aHandle, int64_t aOffset,
                        const char *aBuf, int32_t aCount, bool aValidate,
                        CacheFileIOListener *aCallback);
  static nsresult DoomFile(CacheFileHandle *aHandle,
                           CacheFileIOListener *aCallback);
  static nsresult DoomFileByKey(const nsACString &aKey,
                                CacheFileIOListener *aCallback);
  static nsresult ReleaseNSPRHandle(CacheFileHandle *aHandle);
  static nsresult TruncateSeekSetEOF(CacheFileHandle *aHandle,
                                     int64_t aTruncatePos, int64_t aEOFPos,
                                     CacheFileIOListener *aCallback);
  static nsresult RenameFile(CacheFileHandle *aHandle,
                             const nsACString &aNewName,
                             CacheFileIOListener *aCallback);
  static nsresult InitIndexEntry(CacheFileHandle *aHandle,
                                 uint32_t         aAppId,
                                 bool             aAnonymous,
                                 bool             aInBrowser);
  static nsresult UpdateIndexEntry(CacheFileHandle *aHandle,
                                   const uint32_t  *aFrecency,
                                   const uint32_t  *aExpirationTime);

  static nsresult UpdateIndexEntry();

  enum EEnumerateMode {
    ENTRIES,
    DOOMED
  };

  static nsresult EnumerateEntryFiles(EEnumerateMode aMode,
                                      CacheEntriesEnumerator** aEnumerator);

  static void GetCacheDirectory(nsIFile** result);

private:
  friend class CacheFileHandle;
  friend class CacheFileChunk;
  friend class CacheFile;
  friend class ShutdownEvent;
  friend class OpenFileEvent;
  friend class CloseHandleEvent;
  friend class ReadEvent;
  friend class WriteEvent;
  friend class DoomFileEvent;
  friend class DoomFileByKeyEvent;
  friend class ReleaseNSPRHandleEvent;
  friend class TruncateSeekSetEOFEvent;
  friend class RenameFileEvent;
  friend class CacheIndex;
  friend class MetadataWriteScheduleEvent;

  virtual ~CacheFileIOManager();

  nsresult InitInternal();
  nsresult ShutdownInternal();

  nsresult OpenFileInternal(const SHA1Sum::Hash *aHash,
                            uint32_t aFlags,
                            CacheFileHandle **_retval);
  nsresult OpenSpecialFileInternal(const nsACString &aKey,
                                   uint32_t aFlags,
                                   CacheFileHandle **_retval);
  nsresult CloseHandleInternal(CacheFileHandle *aHandle);
  nsresult ReadInternal(CacheFileHandle *aHandle, int64_t aOffset,
                        char *aBuf, int32_t aCount);
  nsresult WriteInternal(CacheFileHandle *aHandle, int64_t aOffset,
                         const char *aBuf, int32_t aCount, bool aValidate);
  nsresult DoomFileInternal(CacheFileHandle *aHandle);
  nsresult DoomFileByKeyInternal(const SHA1Sum::Hash *aHash);
  nsresult ReleaseNSPRHandleInternal(CacheFileHandle *aHandle);
  nsresult TruncateSeekSetEOFInternal(CacheFileHandle *aHandle,
                                      int64_t aTruncatePos, int64_t aEOFPos);
  nsresult RenameFileInternal(CacheFileHandle *aHandle,
                              const nsACString &aNewName);

  nsresult CreateFile(CacheFileHandle *aHandle);
  static void HashToStr(const SHA1Sum::Hash *aHash, nsACString &_retval);
  static nsresult StrToHash(const nsACString &aHash, SHA1Sum::Hash *_retval);
  nsresult GetFile(const SHA1Sum::Hash *aHash, nsIFile **_retval);
  nsresult GetSpecialFile(const nsACString &aKey, nsIFile **_retval);
  nsresult GetDoomedFile(nsIFile **_retval);
  nsresult CheckAndCreateDir(nsIFile *aFile, const char *aDir);
  nsresult CreateCacheTree();
  nsresult OpenNSPRHandle(CacheFileHandle *aHandle, bool aCreate = false);
  void     NSPRHandleUsed(CacheFileHandle *aHandle);

  nsresult ScheduleMetadataWriteInternal(CacheFile * aFile);
  nsresult UnscheduleMetadataWriteInternal(CacheFile * aFile);
  nsresult ShutdownMetadataWriteSchedulingInternal();

  static CacheFileIOManager           *gInstance;
  bool                                 mShuttingDown;
  nsRefPtr<CacheIOThread>              mIOThread;
  nsCOMPtr<nsIFile>                    mCacheDirectory;
  bool                                 mTreeCreated;
  CacheFileHandles                     mHandles;
  nsTArray<CacheFileHandle *>          mHandlesByLastUsed;
  nsTArray<nsRefPtr<CacheFileHandle> > mSpecialHandles;
  nsTArray<nsRefPtr<CacheFile> >       mScheduledMetadataWrites;
  nsCOMPtr<nsITimer>                   mMetadataWritesTimer;
};

} 
} 

#endif

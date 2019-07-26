





#ifndef nsJAR_h__
#define nsJAR_h__

#include "nscore.h"
#include "prio.h"
#include "plstr.h"
#include "prlog.h"
#include "prtypes.h"
#include "prinrval.h"

#include "mozilla/Mutex.h"
#include "nsIComponentManager.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIFile.h"
#include "nsStringEnumerator.h"
#include "nsHashtable.h"
#include "nsIZipReader.h"
#include "nsZipArchive.h"
#include "nsICertificatePrincipal.h"
#include "nsISignatureVerifier.h"
#include "nsIObserverService.h"
#include "nsWeakReference.h"
#include "nsIObserver.h"
#include "mozilla/Attributes.h"

class nsIInputStream;
class nsJARManifestItem;
class nsZipReaderCache;


typedef enum
{
  JAR_MANIFEST_NOT_PARSED = 0,
  JAR_VALID_MANIFEST      = 1,
  JAR_INVALID_SIG         = 2,
  JAR_INVALID_UNKNOWN_CA  = 3,
  JAR_INVALID_MANIFEST    = 4,
  JAR_INVALID_ENTRY       = 5,
  JAR_NO_MANIFEST         = 6,
  JAR_NOT_SIGNED          = 7
} JARManifestStatusType;






class nsJAR : public nsIZipReader
{
  
  friend class nsJARInputStream;
  
  friend class nsZipReaderCache;

  public:

    nsJAR();
    virtual ~nsJAR();
    
    NS_DEFINE_STATIC_CID_ACCESSOR( NS_ZIPREADER_CID )
  
    NS_DECL_THREADSAFE_ISUPPORTS

    NS_DECL_NSIZIPREADER

    nsresult GetJarPath(nsACString& aResult);

    PRIntervalTime GetReleaseTime() {
        return mReleaseTime;
    }
    
    bool IsReleased() {
        return mReleaseTime != PR_INTERVAL_NO_TIMEOUT;
    }

    void SetReleaseTime() {
      mReleaseTime = PR_IntervalNow();
    }
    
    void ClearReleaseTime() {
      mReleaseTime = PR_INTERVAL_NO_TIMEOUT;
    }
    
    void SetZipReaderCache(nsZipReaderCache* cache) {
      mCache = cache;
    }

  protected:
    
    nsCOMPtr<nsIFile>        mZipFile;        
    nsCString                mOuterZipEntry;  
    nsRefPtr<nsZipArchive>   mZip;            
    nsObjectHashtable        mManifestData;   
    bool                     mParsedManifest; 
    nsCOMPtr<nsICertificatePrincipal> mPrincipal; 
    int16_t                  mGlobalStatus;   
    PRIntervalTime           mReleaseTime;    
    nsZipReaderCache*        mCache;          
    mozilla::Mutex           mLock;	
    int64_t                  mMtime;
    int32_t                  mTotalItemsInManifest;
    bool                     mOpened;

    nsresult ParseManifest();
    void     ReportError(const nsACString &aFilename, int16_t errorCode);
    nsresult LoadEntry(const nsACString &aFilename, char** aBuf, 
                       uint32_t* aBufLen = nullptr);
    int32_t  ReadLine(const char** src); 
    nsresult ParseOneFile(const char* filebuf, int16_t aFileType);
    nsresult VerifyEntry(nsJARManifestItem* aEntry, const char* aEntryData, 
                         uint32_t aLen);

    nsresult CalculateDigest(const char* aInBuf, uint32_t aInBufLen,
                             nsCString& digest);
};







class nsJARItem : public nsIZipEntry
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIZIPENTRY
    
    nsJARItem(nsZipItem* aZipItem);
    virtual ~nsJARItem() {}

private:
    uint32_t     mSize;             
    uint32_t     mRealsize;         
    uint32_t     mCrc32;
    PRTime       mLastModTime;
    uint16_t     mCompression;
    bool mIsDirectory; 
    bool mIsSynthetic;
};







class nsJAREnumerator MOZ_FINAL : public nsIUTF8StringEnumerator
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIUTF8STRINGENUMERATOR

    nsJAREnumerator(nsZipFind *aFind) : mFind(aFind), mName(nullptr) { 
      NS_ASSERTION(mFind, "nsJAREnumerator: Missing zipFind.");
    }

private:
    nsZipFind    *mFind;
    const char*   mName;    
    uint16_t      mNameLen;

    ~nsJAREnumerator() { delete mFind; }
};



#if defined(DEBUG_warren) || defined(DEBUG_jband)
#define ZIP_CACHE_HIT_RATE
#endif

class nsZipReaderCache : public nsIZipReaderCache, public nsIObserver,
                         public nsSupportsWeakReference
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIZIPREADERCACHE
  NS_DECL_NSIOBSERVER

  nsZipReaderCache();
  virtual ~nsZipReaderCache();

  nsresult ReleaseZip(nsJAR* reader);

protected:
  mozilla::Mutex        mLock;
  int32_t               mCacheSize;
  nsSupportsHashtable   mZips;

#ifdef ZIP_CACHE_HIT_RATE
  uint32_t              mZipCacheLookups;
  uint32_t              mZipCacheHits;
  uint32_t              mZipCacheFlushes;
  uint32_t              mZipSyncMisses;
#endif

};



#endif 

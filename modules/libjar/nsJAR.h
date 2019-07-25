









































#ifndef nsJAR_h__
#define nsJAR_h__

#include "nscore.h"
#include "pratom.h"
#include "prmem.h"
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
#include "nsIPrincipal.h"
#include "nsISignatureVerifier.h"
#include "nsIObserverService.h"
#include "nsWeakReference.h"
#include "nsIObserver.h"

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
  
    NS_DECL_ISUPPORTS

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
    nsAutoPtr<nsZipArchive>  mZip;            
    nsObjectHashtable        mManifestData;   
    bool                     mParsedManifest; 
    nsCOMPtr<nsIPrincipal>   mPrincipal;      
    PRInt16                  mGlobalStatus;   
    PRIntervalTime           mReleaseTime;    
    nsZipReaderCache*        mCache;          
    mozilla::Mutex           mLock;	
    PRInt64                  mMtime;
    PRInt32                  mTotalItemsInManifest;
    bool                     mOpened;

    nsresult ParseManifest();
    void     ReportError(const char* aFilename, PRInt16 errorCode);
    nsresult LoadEntry(const nsACString &aFilename, char** aBuf, 
                       PRUint32* aBufLen = nsnull);
    PRInt32  ReadLine(const char** src); 
    nsresult ParseOneFile(const char* filebuf, PRInt16 aFileType);
    nsresult VerifyEntry(nsJARManifestItem* aEntry, const char* aEntryData, 
                         PRUint32 aLen);

    nsresult CalculateDigest(const char* aInBuf, PRUint32 aInBufLen,
                             nsCString& digest);

    
    void DumpMetadata(const char* aMessage);
};







class nsJARItem : public nsIZipEntry
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIZIPENTRY
    
    nsJARItem(nsZipItem* aZipItem);
    virtual ~nsJARItem() {}

private:
    PRUint32     mSize;             
    PRUint32     mRealsize;         
    PRUint32     mCrc32;
    PRTime       mLastModTime;
    PRUint16     mCompression;
    bool mIsDirectory; 
    bool mIsSynthetic;
};







class nsJAREnumerator : public nsIUTF8StringEnumerator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIUTF8STRINGENUMERATOR

    nsJAREnumerator(nsZipFind *aFind) : mFind(aFind), mName(nsnull) { 
      NS_ASSERTION(mFind, "nsJAREnumerator: Missing zipFind.");
    }

private:
    nsZipFind    *mFind;
    const char*   mName;    
    PRUint16      mNameLen;

    ~nsJAREnumerator() { delete mFind; }
};



#if defined(DEBUG_warren) || defined(DEBUG_jband)
#define ZIP_CACHE_HIT_RATE
#endif

class nsZipReaderCache : public nsIZipReaderCache, public nsIObserver,
                         public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIZIPREADERCACHE
  NS_DECL_NSIOBSERVER

  nsZipReaderCache();
  virtual ~nsZipReaderCache();

  nsresult ReleaseZip(nsJAR* reader);

protected:
  mozilla::Mutex        mLock;
  PRInt32               mCacheSize;
  nsSupportsHashtable   mZips;

#ifdef ZIP_CACHE_HIT_RATE
  PRUint32              mZipCacheLookups;
  PRUint32              mZipCacheHits;
  PRUint32              mZipCacheFlushes;
  PRUint32              mZipSyncMisses;
#endif

};



#endif 






#ifndef nsZipArchive_h_
#define nsZipArchive_h_

#include "mozilla/Attributes.h"

#define ZIP_TABSIZE   256
#define ZIP_BUFLEN    (4*1024)      /* Used as output buffer when deflating items to a file */

#ifndef PL_ARENA_CONST_ALIGN_MASK
#define PL_ARENA_CONST_ALIGN_MASK  (sizeof(void*)-1)
#endif
#include "plarena.h"

#include "zlib.h"
#include "zipstruct.h"
#include "nsAutoPtr.h"
#include "nsIFile.h"
#include "mozilla/FileUtils.h"
#include "mozilla/FileLocation.h"

#if defined(XP_WIN) && defined(_MSC_VER)
#define MOZ_WIN_MEM_TRY_BEGIN __try {
#define MOZ_WIN_MEM_TRY_CATCH(cmd) }                                \
  __except(GetExceptionCode()==EXCEPTION_IN_PAGE_ERROR ?            \
           EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)   \
  {                                                                 \
    NS_WARNING("EXCEPTION_IN_PAGE_ERROR in " __FUNCTION__);         \
    cmd;                                                            \
  }
#else
#define MOZ_WIN_MEM_TRY_BEGIN {
#define MOZ_WIN_MEM_TRY_CATCH(cmd) }
#endif

class nsZipFind;
struct PRFileDesc;






















class nsZipItem
{
public:
  const char* Name() { return ((const char*)central) + ZIPCENTRAL_SIZE; }

  uint32_t LocalOffset();
  uint32_t Size();
  uint32_t RealSize();
  uint32_t CRC32();
  uint16_t Date();
  uint16_t Time();
  uint16_t Compression();
  bool     IsDirectory();
  uint16_t Mode();
  const uint8_t* GetExtraField(uint16_t aTag, uint16_t *aBlockSize);
  PRTime   LastModTime();

#ifdef XP_UNIX
  bool     IsSymlink();
#endif

  nsZipItem*         next;
  const ZipCentral*  central;
  uint16_t           nameLength;
  bool               isSynthetic;
};

class nsZipHandle;





class nsZipArchive 
{
  friend class nsZipFind;

public:
  
  nsZipArchive();

  
  ~nsZipArchive();

  









  nsresult OpenArchive(nsZipHandle *aZipHandle);

  







  nsresult OpenArchive(nsIFile *aFile);

  








  nsresult Test(const char *aEntryName);

  


  nsresult CloseArchive();

  



  
  nsZipItem* GetItem(const char * aEntryName);
  
  







  nsresult ExtractFile(nsZipItem * zipEntry, const char *outname, PRFileDesc * outFD);

  












  nsresult FindInit(const char * aPattern, nsZipFind** aFind);

  


  nsZipHandle* GetFD();

  




  const uint8_t* GetData(nsZipItem* aItem);

  bool GetComment(nsACString &aComment);

  



  int64_t SizeOfMapping();

  


  NS_METHOD_(nsrefcnt) AddRef(void);
  NS_METHOD_(nsrefcnt) Release(void);

private:
  
  nsrefcnt      mRefCnt; 

  nsZipItem*    mFiles[ZIP_TABSIZE];
  PLArenaPool   mArena;

  const char*   mCommentPtr;
  uint16_t      mCommentLen;

  
  bool          mBuiltSynthetics;

  
  nsRefPtr<nsZipHandle> mFd;

  
  nsCString mURI;

private:
  
  nsZipItem*        CreateZipItem();
  nsresult          BuildFileList();
  nsresult          BuildSynthetics();

  nsZipArchive& operator=(const nsZipArchive& rhs) MOZ_DELETE;
  nsZipArchive(const nsZipArchive& rhs) MOZ_DELETE;
};






class nsZipFind
{
public:
  nsZipFind(nsZipArchive* aZip, char* aPattern, bool regExp);
  ~nsZipFind();

  nsresult      FindNext(const char** aResult, uint16_t* aNameLen);

private:
  nsRefPtr<nsZipArchive> mArchive;
  char*         mPattern;
  nsZipItem*    mItem;
  uint16_t      mSlot;
  bool          mRegExp;

  nsZipFind& operator=(const nsZipFind& rhs) MOZ_DELETE;
  nsZipFind(const nsZipFind& rhs) MOZ_DELETE;
};




class nsZipCursor {
public:
  









  nsZipCursor(nsZipItem *aItem, nsZipArchive *aZip, uint8_t* aBuf = NULL, uint32_t aBufSize = 0, bool doCRC = false);

  ~nsZipCursor();

  






  uint8_t* Read(uint32_t *aBytesRead) {
    return ReadOrCopy(aBytesRead, false);
  }

  





  uint8_t* Copy(uint32_t *aBytesRead) {
    return ReadOrCopy(aBytesRead, true);
  }

private:
  
  uint8_t* ReadOrCopy(uint32_t *aBytesRead, bool aCopy);

  nsZipItem *mItem; 
  uint8_t  *mBuf; 
  uint32_t  mBufSize; 
  z_stream  mZs;
  uint32_t mCRC;
  bool mDoCRC;
};







class nsZipItemPtr_base {
public:
  






  nsZipItemPtr_base(nsZipArchive *aZip, const char *aEntryName, bool doCRC);

  uint32_t Length() const {
    return mReadlen;
  }

protected:
  nsRefPtr<nsZipHandle> mZipHandle;
  nsAutoArrayPtr<uint8_t> mAutoBuf;
  uint8_t *mReturnBuf;
  uint32_t mReadlen;
};

template <class T>
class nsZipItemPtr : public nsZipItemPtr_base {
public:
  nsZipItemPtr(nsZipArchive *aZip, const char *aEntryName, bool doCRC = false) : nsZipItemPtr_base(aZip, aEntryName, doCRC) { }
  



  const T* Buffer() const {
    return (const T*)mReturnBuf;
  }

  operator const T*() const {
    return Buffer();
  }

  




  T* Forget() {
    if (!mReturnBuf)
      return NULL;
    
    if (mAutoBuf.get() == mReturnBuf) {
      mReturnBuf = NULL;
      return (T*) mAutoBuf.forget();
    }
    T *ret = (T*) malloc(Length());
    memcpy(ret, mReturnBuf, Length());
    mReturnBuf = NULL;
    return ret;
  }
};

class nsZipHandle {
friend class nsZipArchive;
friend class mozilla::FileLocation;
public:
  static nsresult Init(nsIFile *file, nsZipHandle **ret);
  static nsresult Init(nsZipArchive *zip, const char *entry,
                       nsZipHandle **ret);

  NS_METHOD_(nsrefcnt) AddRef(void);
  NS_METHOD_(nsrefcnt) Release(void);

  int64_t SizeOfMapping();

protected:
  const uint8_t * mFileData; 
  uint32_t        mLen;      
  mozilla::FileLocation mFile; 

private:
  nsZipHandle();
  ~nsZipHandle();

#if defined(XP_WIN)
  PRFileDesc *                      mFd;     
#endif
  PRFileMap *                       mMap;    
  nsAutoPtr<nsZipItemPtr<uint8_t> > mBuf;
  nsrefcnt                          mRefCnt; 
};

nsresult gZlibInit(z_stream *zs);

#endif 











































#ifndef nsZipArchive_h_
#define nsZipArchive_h_

#define ZIP_TABSIZE   256
#define ZIP_BUFLEN    (4*1024)      /* Used as output buffer when deflating items to a file */

#ifndef PL_ARENA_CONST_ALIGN_MASK
#define PL_ARENA_CONST_ALIGN_MASK  (sizeof(void*)-1)
#endif
#include "plarena.h"

#include "zlib.h"
#include "zipstruct.h"
#include "nsAutoPtr.h"
#include "nsILocalFile.h"
#include "mozilla/FileUtils.h"

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

  PRUint32 LocalOffset();
  PRUint32 Size();
  PRUint32 RealSize();
  PRUint32 CRC32();
  PRUint16 Date();
  PRUint16 Time();
  PRUint16 Compression();
  bool     IsDirectory();
  PRUint16 Mode();
  const PRUint8* GetExtraField(PRUint16 aTag, PRUint16 *aBlockSize);
  PRTime   LastModTime();

#ifdef XP_UNIX
  bool     IsSymlink();
#endif

  nsZipItem*         next;
  const ZipCentral*  central;
  PRUint16           nameLength;
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

  












  PRInt32 FindInit(const char * aPattern, nsZipFind** aFind);

  


  nsZipHandle* GetFD();

  




  const PRUint8* GetData(nsZipItem* aItem);

private:
  

  nsZipItem*    mFiles[ZIP_TABSIZE];
  PLArenaPool   mArena;

  
  bool          mBuiltSynthetics;

  
  nsRefPtr<nsZipHandle> mFd;

  
  mozilla::AutoFDClose mLog;

  
  
  nsZipArchive& operator=(const nsZipArchive& rhs); 
  nsZipArchive(const nsZipArchive& rhs);            

  nsZipItem*        CreateZipItem();
  nsresult          BuildFileList();
  nsresult          BuildSynthetics();
};






class nsZipFind
{
public:
  nsZipFind(nsZipArchive* aZip, char* aPattern, bool regExp);
  ~nsZipFind();

  nsresult      FindNext(const char** aResult, PRUint16* aNameLen);

private:
  nsZipArchive* mArchive;
  char*         mPattern;
  nsZipItem*    mItem;
  PRUint16      mSlot;
  bool          mRegExp;

  
  nsZipFind& operator=(const nsZipFind& rhs);
  nsZipFind(const nsZipFind& rhs);
};




class nsZipCursor {
public:
  









  nsZipCursor(nsZipItem *aItem, nsZipArchive *aZip, PRUint8* aBuf = NULL, PRUint32 aBufSize = 0, bool doCRC = false);

  ~nsZipCursor();

  






  PRUint8* Read(PRUint32 *aBytesRead);

private:
  nsZipItem *mItem; 
  PRUint8  *mBuf; 
  PRUint32  mBufSize; 
  z_stream  mZs;
  PRUint32 mCRC;
  bool mDoCRC;
};







class nsZipItemPtr_base {
public:
  






  nsZipItemPtr_base(nsZipArchive *aZip, const char *aEntryName, bool doCRC);

  PRUint32 Length() const {
    return mReadlen;
  }

protected:
  nsRefPtr<nsZipHandle> mZipHandle;
  nsAutoArrayPtr<PRUint8> mAutoBuf;
  PRUint8 *mReturnBuf;
  PRUint32 mReadlen;
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
public:
  static nsresult Init(nsILocalFile *file, nsZipHandle **ret NS_OUTPARAM);
  static nsresult Init(nsZipArchive *zip, const char *entry,
                       nsZipHandle **ret NS_OUTPARAM);

  NS_METHOD_(nsrefcnt) AddRef(void);
  NS_METHOD_(nsrefcnt) Release(void);

protected:
  const PRUint8 * mFileData; 
  PRUint32        mLen;      
  nsCOMPtr<nsILocalFile> mFile; 

private:
  nsZipHandle();
  ~nsZipHandle();

  PRFileMap *                       mMap;    
  nsAutoPtr<nsZipItemPtr<PRUint8> > mBuf;
  nsrefcnt                          mRefCnt; 
};

nsresult gZlibInit(z_stream *zs);

#endif 

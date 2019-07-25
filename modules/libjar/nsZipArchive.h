









































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

#if defined(XP_UNIX) || defined(XP_BEOS)
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

  









  nsresult OpenArchive(nsIFile *aZipFile);

  








  nsresult Test(const char *aEntryName);

  


  nsresult CloseArchive();

  



  
  nsZipItem* GetItem(const char * aEntryName);
  
  







  nsresult ExtractFile(nsZipItem * zipEntry, const char *outname, PRFileDesc * outFD);

  












  PRInt32 FindInit(const char * aPattern, nsZipFind** aFind);

  


  nsZipHandle* GetFD();

  




  PRUint8* GetData(nsZipItem* aItem);

  PRBool CheckCRC(nsZipItem* aItem, PRUint8* aData);

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

class nsZipHandle {
friend class nsZipArchive;
public:
  static nsresult Init(PRFileDesc *fd, nsZipHandle **ret NS_OUTPARAM);

  NS_METHOD_(nsrefcnt) AddRef(void);
  NS_METHOD_(nsrefcnt) Release(void);

protected:
  PRUint8 *    mFileData; 
  PRUint32     mLen;      

private:
  nsZipHandle();
  ~nsZipHandle();

  PRFileMap *  mMap;      
  nsrefcnt     mRefCnt;   
};







class nsZipFind
{
public:
  nsZipFind(nsZipArchive* aZip, char* aPattern, PRBool regExp);
  ~nsZipFind();

  nsresult      FindNext(const char** aResult, PRUint16* aNameLen);

private:
  nsZipArchive* mArchive;
  char*         mPattern;
  nsZipItem*    mItem;
  PRUint16      mSlot;
  PRPackedBool  mRegExp;

  
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
};

nsresult gZlibInit(z_stream *zs);

#endif 

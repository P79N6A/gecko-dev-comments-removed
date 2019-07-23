









































#ifndef nsZipArchive_h_
#define nsZipArchive_h_

#define ZIP_MAGIC     0x5A49505FL   /* "ZIP_" */
#define ZIPFIND_MAGIC 0x5A495046L   /* "ZIPF" */
#define ZIP_TABSIZE   256

#define ZIP_BUFLEN    (4 * 1024 - 1)

#define PL_ARENA_CONST_ALIGN_MASK 7
#include "plarena.h"
#define ZIP_Seek(fd,p,m) (PR_Seek((fd),((PROffset32)p),(m))==((PROffset32)p))

#include "zlib.h"
#include "nsAutoPtr.h"

class nsZipFind;
class nsZipReadState;
class nsZipItemMetadata;

struct PRFileDesc;






















struct nsZipItem
{
  nsZipItem*  next;

  PRUint32    headerOffset;
  PRUint32    dataOffset;
  PRUint32    size;             
  PRUint32    realsize;         
  PRUint32    crc32;

  


  PRUint16     time;
  PRUint16     date;
  PRUint16     mode;
  PRUint8      compression;
  PRPackedBool hasDataOffset : 1;
  PRPackedBool isDirectory : 1; 
  PRPackedBool isSynthetic : 1;  



#if defined(XP_UNIX) || defined(XP_BEOS)
  PRPackedBool isSymlink : 1;
#endif

  char        name[1]; 
};

class nsZipHandle;
class nsSeekableZipHandle;




class nsZipArchive 
{
  friend class nsZipFind;

public:
  
  nsZipArchive();

  
  ~nsZipArchive();

  









  nsresult OpenArchive(PRFileDesc* fd);

  








  nsresult Test(const char *aEntryName);

  


  nsresult CloseArchive();

  



  
  nsZipItem* GetItem(const char * aEntryName);
  
  







  nsresult ExtractFile(nsZipItem * zipEntry, const char *outname, PRFileDesc * outFD);

  












  PRInt32 FindInit(const char * aPattern, nsZipFind** aFind);

  


  nsZipHandle* GetFD(nsZipItem* aItem);

private:
  

  nsZipItem*    mFiles[ZIP_TABSIZE];
  PLArenaPool   mArena;

  




  bool MaybeReadItem(nsZipItem* aItem);

  
  PRPackedBool  mBuiltSynthetics;

  
  nsRefPtr<nsZipHandle> mFd;
  
  
  nsZipArchive& operator=(const nsZipArchive& rhs); 
  nsZipArchive(const nsZipArchive& rhs);            

  nsZipItem*        CreateZipItem(PRUint16 namelen);
  nsresult          BuildFileList();
  nsresult          BuildSynthetics();

  nsresult  CopyItemToDisk(PRUint32 size, PRUint32 crc, nsSeekableZipHandle &fd, PRFileDesc* outFD);
  nsresult  InflateItem(const nsZipItem* aItem, nsSeekableZipHandle &fd, PRFileDesc* outFD);
};

class nsZipHandle {
friend class nsZipArchive;
friend class nsSeekableZipHandle;
public:
  static nsresult Init(PRFileDesc *fd, nsZipHandle **ret NS_OUTPARAM);

  




  PRInt32 Read(PRUint32 aPosition, void *aBuffer, PRUint32 aCount);

  NS_METHOD_(nsrefcnt) AddRef(void);
  NS_METHOD_(nsrefcnt) Release(void);

protected:
  PRFileDesc *mFd; 
  PRUint8 *mFileData; 
  PRUint32 mLen; 

private:
  nsZipHandle();
  ~nsZipHandle();

  PRFileMap *mMap; 
  nsrefcnt mRefCnt; 
};




class nsSeekableZipHandle {
  
public:
  nsSeekableZipHandle()
    : mOffset(0)
    , mRemaining(0)
  {
  }

  



  bool Open(nsZipHandle *aHandle, PRUint32 aOffset, PRUint32 aLength) {
    NS_ABORT_IF_FALSE (aHandle, "Argument must not be NULL");
    if (aOffset > aHandle->mLen)
      return false;
    mFd = aHandle;
    mOffset = aOffset;
    mRemaining = aLength;
    return true;
  }

  
  void Close()
  {
    mFd = NULL;
  }

  



  PRInt32 Read(void *aBuffer, PRUint32 aCount)
  {
    if (!mFd.get())
      return -1;
    aCount = PR_MIN(mRemaining, aCount);
    PRInt32 ret = mFd->Read(mOffset, aBuffer, aCount);
    if (ret > 0) {
      mOffset += ret;
      mRemaining -= ret;
    }
    return ret;
  }

private:
  nsRefPtr<nsZipHandle> mFd; 
  PRUint32 mOffset; 
  PRUint32 mRemaining; 
};







class nsZipFind
{
public:

  nsZipFind(nsZipArchive* aZip, char* aPattern, PRBool regExp);
  ~nsZipFind();

  nsresult      FindNext(const char ** aResult);

private:
  nsZipArchive* mArchive;
  char*         mPattern;
  nsZipItem*    mItem;
  PRUint16      mSlot;
  PRPackedBool  mRegExp;

  
  nsZipFind& operator=(const nsZipFind& rhs);
  nsZipFind(const nsZipFind& rhs);
};

nsresult gZlibInit(z_stream *zs);

#endif 

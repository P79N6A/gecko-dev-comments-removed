









































#ifndef nsZipArchive_h_
#define nsZipArchive_h_

#define ZIP_TABSIZE   256
#define ZIP_BUFLEN    (4*1024)      /* Used as output buffer when deflating items to a file */

#define PL_ARENA_CONST_ALIGN_MASK  (sizeof(void*)-1)
#include "plarena.h"

#include "zlib.h"
#include "zipstruct.h"
#include "nsAutoPtr.h"

class nsZipFind;

struct PRFileDesc;






















class nsZipItem
{
public:
  const char* Name() { return ((const char*)central) + ZIPCENTRAL_SIZE; }

  PRUint32 const LocalOffset();
  PRUint32 const Size();
  PRUint32 const RealSize();
  PRUint32 const CRC32();
  PRUint16 const Date();
  PRUint16 const Time();
  PRUint16 const Compression();
  bool     const IsDirectory();
  PRUint16 const Mode();

#if defined(XP_UNIX) || defined(XP_BEOS)
  bool     const IsSymlink();
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

  









  nsresult OpenArchive(PRFileDesc* fd);

  








  nsresult Test(const char *aEntryName);

  


  nsresult CloseArchive();

  



  
  nsZipItem* GetItem(const char * aEntryName);
  
  







  nsresult ExtractFile(nsZipItem * zipEntry, const char *outname, PRFileDesc * outFD);

  












  PRInt32 FindInit(const char * aPattern, nsZipFind** aFind);

  


  nsZipHandle* GetFD();

  




  PRUint8* GetData(nsZipItem* aItem);

private:
  

  nsZipItem*    mFiles[ZIP_TABSIZE];
  PLArenaPool   mArena;

  
  bool          mBuiltSynthetics;

  
  nsRefPtr<nsZipHandle> mFd;
  
  
  nsZipArchive& operator=(const nsZipArchive& rhs); 
  nsZipArchive(const nsZipArchive& rhs);            

  nsZipItem*        CreateZipItem();
  nsresult          BuildFileList();
  nsresult          BuildSynthetics();

  nsresult  CopyItemToDisk(nsZipItem* item, PRFileDesc* outFD);
  nsresult  InflateItem(nsZipItem* item, PRFileDesc* outFD);
};

class nsZipHandle {
friend class nsZipArchive;
public:
  static nsresult Init(PRFileDesc *fd, nsZipHandle **ret NS_OUTPARAM);

  NS_METHOD_(nsrefcnt) AddRef(void);
  NS_METHOD_(nsrefcnt) Release(void);

protected:
  PRFileDesc * mFd;       
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

nsresult gZlibInit(z_stream *zs);

#endif 

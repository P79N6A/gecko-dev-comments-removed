









































#ifndef nsZipArchive_h_
#define nsZipArchive_h_

#define ZIP_MAGIC     0x5A49505FL   /* "ZIP_" */
#define ZIPFIND_MAGIC 0x5A495046L   /* "ZIPF" */
#define ZIP_TABSIZE   256

#define ZIP_BUFLEN    (4*1024)      /* Used as output buffer when deflating items to a file */

#define PL_ARENA_CONST_ALIGN_MASK 7
#include "plarena.h"

#include "zlib.h"
#include "nsAutoPtr.h"

class nsZipFind;

struct PRFileDesc;






















struct nsZipItem
{
  nsZipItem*  next;

  PRUint32    headerOffset;
  PRUint32    size;             
  PRUint32    realsize;         
  PRUint32    crc32;

  


  PRUint16     time;
  PRUint16     date;
  PRUint16     mode;
  PRUint8      compression;
  bool         isDirectory;
  bool         isSynthetic;     

#if defined(XP_UNIX) || defined(XP_BEOS)
  bool         isSymlink;
#endif

  char         name[1];         
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

  nsZipItem*        CreateZipItem(PRUint16 namelen);
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

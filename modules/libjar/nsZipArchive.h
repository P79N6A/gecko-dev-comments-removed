








































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

  




  nsresult  SeekToItem(nsZipItem* aItem, PRFileDesc* aFd);

private:
  

  nsZipItem*    mFiles[ZIP_TABSIZE];
  PLArenaPool   mArena;

  
  PRFileDesc    *mFd;

  
  PRPackedBool  mBuiltSynthetics;

  
  
  nsZipArchive& operator=(const nsZipArchive& rhs); 
  nsZipArchive(const nsZipArchive& rhs);            

  nsZipItem*        CreateZipItem(PRUint16 namelen);
  nsresult          BuildFileList();
  nsresult          BuildSynthetics();

  nsresult  CopyItemToDisk(PRUint32 size, PRUint32 crc, PRFileDesc* outFD);
  nsresult  InflateItem(const nsZipItem* aItem, PRFileDesc* outFD);
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
